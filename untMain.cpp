//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "untMain.h"
#include "untSetMAC.h"
#include "untFlashReader.h"
#include "untUtils.h"
#include "untTextCmdParse.h"
#include "TextCmdParse.h"
#include "frmInputPassword.h"

#include <winsock2.h>
#include <IPHlpApi.h>  
#include <algorithm>
#include <stddef.h>
#include <assert.h>
#include <stdio.h>
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma link "CSPIN"
#pragma link "AdvTrackBar"
#pragma link "SpeedButtonNoFrame"
#pragma link "AdvGDIPicture"
#pragma comment(lib,"Msimg32.lib")
#pragma link "CGAUGES"
#pragma resource "*.dfm"
#pragma comment(lib, "gdiplus.lib")

#define PANEL_WIDTH 48

#define DEFAULT_MASK 0x00FFFFFF

#define CONTROL_TIMEOUT_COUNT 5

static int delay_send_cmd_check_count = 5;
static int restor_delay_check_count = 5;

String inner_mac[7] = {"10-0B-A9-2F-55-90", "00-5A-39-FF-49-28","00-E0-4C-39-17-31","74-D0-2B-95-48-02","00-E0-4C-15-1B-C0", "38-2C-4A-BA-EF-54", "28-C6-3F-D2-55-C8"};

const String compile_time = __DATE__ " " __TIME__;

static String GetTime()
{
    return FormatDateTime("nn:ss.zzz", Now())+" ";
}

struct IpInfo
{
    String ip;
    DWORD mask;
    bool is_dhcp;
};
vector<IpInfo> ip_info;

TForm1 *Form1;
ConfigMap all_config_map[PRESET_NUM];
ConfigMap config_map;
GlobalConfig global_config = {0};
DeviceSetting device_setting = {0};
static bool global_config_loaded = false;
int REAL_INPUT_DSP_NUM = 16;
int REAL_OUTPUT_DSP_NUM = 16;

static vector<int> loose_msg_id;

extern String GetMacList();

// 用于复制设备的flaah数据
struct SmcConfig
{
    GlobalConfig global_config;
    DeviceSetting device_setting;
    UINT file_version;
    UINT device_version;
    UINT cpu_id[3];        // cpu_id 位空表示是脱机文件
    unsigned char mac[6];
    char pad1[2048-sizeof(GlobalConfig) - sizeof(UINT)-sizeof(UINT)-sizeof(UINT)*3-6];
    char device_flash_dump[128*1024];
};
static SmcConfig smc_config;
bool IsDeviceFlashEmpty()
{
    // 判断是否在线保存
    for (int i=0;i<sizeof(smc_config.device_flash_dump);i++)
    {
        if (smc_config.device_flash_dump[i] != 0)
            return true;
    }
    return false;
}


static bool on_loading = false;
static String last_device_id;
static Word enter_key = VK_RETURN;
//static char head[] = "\x53\x65\x74\x50\x61\x72\x61\x00\x00\x00\x4D\x41\x54\x31\x36\x31\x30\x43\x6F\x6E\x66\x69\x67\x44\x61\x74\x61\x00\x00\x00";
static BLENDFUNCTION blend = {AC_SRC_OVER, 0, 200, 0};
static TTime startup_time;

bool IsKeepliveCmd(char * str);
//===================================
static short UdpPackageVerifyDiff(unsigned char * udp_data, int udp_length)
{
	int i;
	short verify = 0;
	for (i=0;i<udp_length;i++)
	{
		verify = verify + udp_data[i];
	}
	return verify;
}
//====================================
//-----------------------------


enum CHANEL_TYPE {ctNone, ctInput, ctOutput};
struct Channel
{
     CHANEL_TYPE channel_type;
     int channel_id;// 1-16
};
static Channel copied_channel = {ctNone, 0};
static Channel selected_channel = {ctNone, 0};

struct T_slp_pack_Str
{
    String ip;
    String mac;
    String sn;
	String name;
	String default_device_name;
    unsigned int version;
    int links;
    int delay;
};

struct DeviceData
{
    int count;
    T_slp_pack_Str data;
};
DeviceData last_connection;

static unsigned int received_cmd_seq = 0;

//------------------------------------------------

static String Event2Sring(Event event);
//------------------------------------------------

// 版本兼容信息
static UINT version = 0x04000001;
static UINT file_version = 0x00000003;
// 返回YES或者下位机版本号
// version_list以0结尾
String IsCompatibility(unsigned int device_version)
{
    UINT ctrl_app_protocol_version = version & 0xFF000000;
    UINT app_protocol_version = device_version & 0xFF000000;
    if (ctrl_app_protocol_version == app_protocol_version)
    {
        ctrl_app_protocol_version = version & 0xFFFF0000;
        app_protocol_version = device_version & 0xFFFF0000;
        if (ctrl_app_protocol_version == app_protocol_version)
        {
            return "YES";
        }
        return "YELLOW";
    }
    else
    {
        return IntToHex((int)device_version, 8);
    }
}
String VersionToStr(UINT version_value)
{
    unsigned char * p_version = (unsigned char *)&version_value;
    String str_version;
    str_version.sprintf("%d.%d.%d.%d", p_version[3], p_version[2], p_version[1], p_version[0]);

    return str_version;
}
//------------------------------------------------
unsigned int GetOffsetOfData(void * p_data)
{
    static char * p_config_map = (char*)&config_map;
    int result = (char*)p_data - p_config_map;
    return result;
}

String FormatFloat(double value, int precise)
{
    if ((value <= 0.000001) && (value >= -0.000001))
        return "0";
    // 保留n个数字精度
    int count = 0;
    value *= pow10(precise);
    count = log10(abs(value));
    value = (int)(value / pow10(count-precise+1));
    value = value * pow10(count-precise+1);
    value /= pow10(precise);
    return String::FormatFloat("0.###", value);
}
struct CompConfig
{
    int min_value;
    int max_value;
    int default_value;
    float scale;
    int precise;
    //int step;
};

CompConfig ratio_config = {0, 100, 100, 100, 3};
CompConfig threshold_config = {-320, 0, 0, 10, 3};
CompConfig attack_config = {1, 20000, 640, 10, 3};
CompConfig release_config = {10, 50000, 10000, 10, 3};
CompConfig gain_config = {0, 240, 0, 10, 3};
VersionFunction TForm1::GetVersionConfig()
{
    if (global_config_loaded && String(device_setting.version_function.name) == "")
    {
        ShowMessage("读取配置表失败");
        keep_live_count = CONTROL_TIMEOUT_COUNT;
    }
    return device_setting.version_function;
}

String Ration2String(double ratio)
{
    if (ratio == 0)
        return "∞";
    else
        return FormatFloat(100.0 / ratio, ratio_config.precise);
}
//---------------------------------------------------------------------------
static String GetTextCmdVar(String cmd_text)
{
    // 比较 VarName 部分
    int eq_index = String(cmd_text).Pos("=");
    return cmd_text.SubString(1, eq_index);
}
//---------------------------------------------------------------------------
void SelectNullControl()
{
    Form1->ActiveControl = NULL;
}
//---------------------------------------------------------------------------
static int SetBit(int old_value, int bit, bool is_set)
{
    int value;
    if (is_set)
    {
        value = old_value | (1<<(bit-1));
    }
    else
    {
        value = old_value & (~(1<<(bit-1)));
    }
    return value;
}
//---------------------------------------------------------------------------
//多网卡 将IP地址写入到列表 WinSock
void GetLocalIpListEx(vector<IpInfo> & ip_info)
{
    ip_info.clear();

    String szMark;

    PIP_ADAPTER_INFO pAdapterInfo = NULL;
    PIP_ADAPTER_INFO pAdapter = NULL;   
  
    pAdapterInfo = ( IP_ADAPTER_INFO * ) malloc( sizeof( IP_ADAPTER_INFO ) );  
    ULONG ulOutBufLen;   
    ulOutBufLen = sizeof(IP_ADAPTER_INFO);   
  
    // 第一次调用GetAdapterInfo获取ulOutBufLen大小   
    if (GetAdaptersInfo( pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW)   
    {   
        free(pAdapterInfo);  
        pAdapterInfo = (IP_ADAPTER_INFO *) malloc (ulOutBufLen);
    }   

    if (GetAdaptersInfo( pAdapterInfo, &ulOutBufLen) == NO_ERROR)
    {   
        pAdapter = pAdapterInfo;
        while (pAdapter)
        {
            PIP_ADDR_STRING pIPAddr;
            pIPAddr = &pAdapter->IpAddressList;
            while (pIPAddr)
            {
                IpInfo ip;
                ip.ip = pIPAddr->IpAddress.String;
                ip.mask = inet_addr(pIPAddr->IpMask.String);
                ip.is_dhcp = pAdapter->DhcpEnabled
                         && (pAdapter->DhcpServer.IpAddress.String[0]!='0')
                         && (pAdapter->DhcpServer.IpAddress.String[0]!='\0');
                if (ip.ip != "0.0.0.0" && ip.ip != "" && ip.mask != INADDR_NONE)
                    ip_info.push_back(ip);
                pIPAddr = pIPAddr->Next;  
            }  
            pAdapter = pAdapter->Next;
        }
        free(pAdapterInfo);
    }
}

void GetLocalIpList(vector<IpInfo> & ip_info)
{
    ip_info.clear();

    try
    {
        char HostName[MAX_PATH + 1] = {0};
        int NameLen = gethostname(HostName, MAX_PATH);
        if (NameLen == SOCKET_ERROR)
        {
            return;
        }
        
        hostent * lpHostEnt = gethostbyname(HostName);

        if (lpHostEnt == NULL)
        {
            return;
        }

        int i = 0;
        char * * pPtr = lpHostEnt->h_addr_list;
        
        while (pPtr[i] != NULL)
        {
            IpInfo ip;
            ip.ip = inet_ntoa(*(in_addr*)pPtr[i]);
            ip.mask = DEFAULT_MASK;
            ip.is_dhcp = false;
            ip_info.push_back(ip);
            i++;
        }
    }
    __finally
    {
        Form1->AppendLog(__FUNC__);
    }
}

static DWORD GetMaskOfIp(String ip)
{
    for (UINT i=0;i<ip_info.size();i++)
    {
        if (ip_info[i].ip == ip)
            return ip_info[i].mask;
    }

    return DEFAULT_MASK;
}
static bool GetDhcpOfIp(String ip)
{
    for (UINT i=0;i<ip_info.size();i++)
    {
        if (ip_info[i].ip == ip)
            return ip_info[i].is_dhcp;
    }

    return false;
}
//---------------------------------------------------------------------------
static Gdipicture::TGDIPPicture* MixPicture[16] = {NULL};
static void LoadMixBmp()
{
    for (int i=0;i<REAL_OUTPUT_DSP_NUM;i++)
    {
        MixPicture[i] = new Gdipicture::TGDIPPicture();
        MixPicture[i]->LoadFromResourceName(NULL, "mix"+IntToStr(i+1));
    }
}
static TSpeedButton * CopyInputPanelButton(TSpeedButtonNoFrame * src_btn, int dsp_id, Graphics::TBitmap* bmp=NULL)
{
    TSpeedButtonNoFrame * dsp_btn = new TSpeedButtonNoFrame(src_btn->Owner);
    dsp_btn->Caption = src_btn->Caption;
    dsp_btn->BoundsRect = src_btn->BoundsRect;
    dsp_btn->Left = src_btn->Left + (dsp_id-1) * PANEL_WIDTH;
    dsp_btn->AllowAllUp = src_btn->AllowAllUp;
    dsp_btn->Flat = src_btn->Flat;
    dsp_btn->SpeedCanvas->Font->Size = src_btn->SpeedCanvas->Font->Size;
    if (bmp == NULL)
    {
        dsp_btn->Glyph = src_btn->Glyph;
    }
    else
    {
        dsp_btn->Glyph = bmp;
    }
    dsp_btn->NumGlyphs = src_btn->NumGlyphs;
    dsp_btn->Layout = src_btn->Layout;
    dsp_btn->OnClick = src_btn->OnClick;
    dsp_btn->Parent = src_btn->Parent;

    dsp_btn->Tag = dsp_id;
    dsp_btn->GroupIndex = (int)dsp_btn; // 所有按钮互不影响

    dsp_btn->Down = src_btn->Down;

    return dsp_btn;
}
static TAdvTrackBar * CopyInputPanelTrackbar(TAdvTrackBar * src_trackbar, int dsp_id)
{
    TAdvTrackBar * trackbar = new TAdvTrackBar(src_trackbar->Parent);
    trackbar->Parent = src_trackbar->Parent;

    trackbar->OnChange = src_trackbar->OnChange;
    trackbar->OnKeyDown = src_trackbar->OnKeyDown;
    trackbar->OnEnter = src_trackbar->OnEnter;
    trackbar->OnExit = src_trackbar->OnExit;
    trackbar->OnMouseMove = src_trackbar->OnMouseMove;

    trackbar->Buttons->Size = src_trackbar->Buttons->Size;
    trackbar->Buttons->Spacing = src_trackbar->Buttons->Spacing;
    trackbar->Direction  = src_trackbar->Direction ;
    trackbar->Max        = src_trackbar->Max       ;
    trackbar->Min        = src_trackbar->Min       ;
    trackbar->Orientation = src_trackbar->Orientation;

    trackbar->Slider->Visible = src_trackbar->Slider->Visible;
    trackbar->Slider->Offset = src_trackbar->Slider->Offset;
    trackbar->Slider->Size = src_trackbar->Slider->Size;

    trackbar->TickMark->Style = src_trackbar->TickMark->Style;

    trackbar->BackGround = src_trackbar->BackGround;
    trackbar->BackGroundStretched = src_trackbar->BackGroundStretched;

    trackbar->Thumb->Picture = src_trackbar->Thumb->Picture;
    trackbar->Thumb->PictureHot = src_trackbar->Thumb->PictureHot;
    trackbar->Thumb->PictureDown = src_trackbar->Thumb->PictureDown;

    trackbar->BoundsRect = src_trackbar->BoundsRect;
    trackbar->Left = src_trackbar->Left + (dsp_id-1) * PANEL_WIDTH;

    trackbar->Tag = dsp_id;

    trackbar->OnChange(trackbar);

    return trackbar;
}
static TEdit * CopyInputPanelEdit(TEdit * src_edit, int dsp_id)
{
    TEdit * edit = new TEdit(src_edit->Parent);
    edit->BoundsRect = src_edit->BoundsRect;
    edit->Left = src_edit->Left + (dsp_id-1) * PANEL_WIDTH;
    edit->BorderStyle = src_edit->BorderStyle;
    edit->Color = src_edit->Color;
    edit->Font = src_edit->Font;
    edit->Parent = src_edit->Parent;
    edit->OnClick = src_edit->OnClick;
    edit->OnKeyDown = src_edit->OnKeyDown;
    edit->OnExit = src_edit->OnExit;
    edit->Tag = dsp_id;
    SetWindowLong(edit->Handle, GWL_STYLE, GetWindowLong(edit->Handle, GWL_STYLE) | ES_RIGHT);
    return edit;
}
static TStaticText * CopyInputPanelLabel(TStaticText * src_label, int dsp_id)
{
    TStaticText * label = new TStaticText(src_label->Parent);
    label->BoundsRect = src_label->BoundsRect;
    label->Left = src_label->Left + (dsp_id-1) * PANEL_WIDTH;
    label->Color = src_label->Color;
    label->Font = src_label->Font;
    label->Parent = src_label->Parent;
    label->OnClick = src_label->OnClick;
    label->Tag = dsp_id-1;
    return label;
}
static void CreateInputPanel(int panel_id, TForm1 * form)
{
    form->input_dsp_btn[panel_id-1] = CopyInputPanelButton(form->input_panel_dsp_btn, panel_id);
        form->input_dsp_btn[panel_id-1]->Caption = "DSP " + String(char('A'-1+panel_id));
    form->input_eq_btn[panel_id-1] = CopyInputPanelButton(form->input_panel_eq_btn, panel_id);
    //form->input_comp_btn[panel_id-1] = CopyInputPanelButton(form->input_panel_comp_btn, panel_id);
    //form->input_auto_btn[panel_id-1] = CopyInputPanelButton(form->input_panel_auto_btn, panel_id);
    //form->input_default_btn[panel_id-1] = CopyInputPanelButton(form->input_panel_default_btn, panel_id);
    form->input_invert_btn[panel_id-1] = CopyInputPanelButton(form->input_panel_invert_btn, panel_id);
    form->input_noise_btn[panel_id-1] = CopyInputPanelButton(form->input_panel_noise_btn, panel_id);
    form->input_mute_btn[panel_id-1] = CopyInputPanelButton(form->input_panel_mute_btn, panel_id);
    form->input_level_edit[panel_id-1] = CopyInputPanelEdit(form->input_panel_level_edit, panel_id);
//        form->input_level_edit[panel_id-1]->Color =
//            form->input_panel_bkground->Canvas->Pixels[form->input_level_edit[panel_id-1]->Left+19]
//                                                      [form->input_level_edit[panel_id-1]->Top-140];
    form->input_level_trackbar[panel_id-1] = CopyInputPanelTrackbar(form->input_panel_trackbar, panel_id);
    form->input_dsp_name[panel_id-1] = CopyInputPanelLabel(form->input_panel_dsp_num, panel_id);
        form->input_dsp_name[panel_id-1]->Caption = String(char('A'-1+panel_id));

    form->input_thumb[panel_id-1] = new TPaintBox(form);
        form->input_thumb[panel_id-1]->Visible = form->input_panel_thumb->Visible;
        form->input_thumb[panel_id-1]->Parent = form->input_panel_thumb->Parent;
        form->input_thumb[panel_id-1]->BoundsRect = form->input_panel_thumb->BoundsRect;
        form->input_thumb[panel_id-1]->Left = form->input_panel_thumb->Left + (panel_id-1) * PANEL_WIDTH;
        form->input_thumb[panel_id-1]->OnPaint = form->input_panel_thumb->OnPaint;
        form->input_thumb[panel_id-1]->Tag = panel_id;
}
static void CreateOutputPanel(int panel_id, TForm1 * form)
{
    form->output_dsp_btn[panel_id-1] = CopyInputPanelButton(form->output_panel_dsp_btn, panel_id);
        form->output_dsp_btn[panel_id-1]->Caption = "DSP" + IntToStr(panel_id);
        form->output_dsp_btn[panel_id-1]->Tag = 100 + panel_id;
    form->output_eq_btn[panel_id-1] = CopyInputPanelButton(form->output_panel_eq_btn, panel_id);
    form->output_comp_btn[panel_id-1] = CopyInputPanelButton(form->output_panel_comp_btn, panel_id);
    form->output_invert_btn[panel_id-1] = CopyInputPanelButton(form->output_panel_invert_btn, panel_id);
    form->output_mute_btn[panel_id-1] = CopyInputPanelButton(form->output_panel_mute_btn, panel_id);

    Graphics::TBitmap * bmp = new Graphics::TBitmap();
    form->ImageList1->GetBitmap(panel_id-1, bmp);
    form->output_number_btn[panel_id-1] = CopyInputPanelButton(form->output_panel_number_btn, panel_id, bmp);
    delete bmp;

    form->output_level_edit[panel_id-1] = CopyInputPanelEdit(form->output_panel_level_edit, panel_id);
    form->output_level_trackbar[panel_id-1] = CopyInputPanelTrackbar(form->output_panel_trackbar, panel_id);
    form->output_dsp_name[panel_id-1] = CopyInputPanelLabel(form->output_panel_dsp_num, panel_id);
        form->output_dsp_name[panel_id-1]->Caption = IntToStr(panel_id);
        
    form->output_thumb[panel_id-1] = new TPaintBox(form);
        form->output_thumb[panel_id-1]->Visible = form->output_panel_thumb->Visible;
        form->output_thumb[panel_id-1]->Parent = form->output_panel_thumb->Parent;
        form->output_thumb[panel_id-1]->BoundsRect = form->output_panel_thumb->BoundsRect;
        form->output_thumb[panel_id-1]->Left = form->output_panel_thumb->Left + (panel_id-1) * PANEL_WIDTH;
        form->output_thumb[panel_id-1]->OnPaint = form->output_panel_thumb->OnPaint;
        form->output_thumb[panel_id-1]->Tag = panel_id;
}
static void CopyWatchPanel(int panel_id, TForm1 * form, String label, int left)
{
    TPanel * watch_panel = new TPanel(form->pnlOprX);
    watch_panel->SetBounds(left, form->watch_panel->Top, form->watch_panel->Width, form->watch_panel->Height);
    watch_panel->BevelOuter = form->watch_panel->BevelOuter;
    watch_panel->Parent = form->pnlOprX;
    watch_panel->Color = form->watch_panel->Color;
    form->watch_panel_inner[panel_id-1] = watch_panel;

    Graphics::TBitmap * tmp_bmp = new Graphics::TBitmap();

    TImage * bk_image = new TImage(watch_panel);
    bk_image->BoundsRect = form->imgWatch->BoundsRect;
    bk_image->Parent = watch_panel;
    bk_image->Picture->Bitmap = tmp_bmp;
    bk_image->Picture->Bitmap->Height = bk_image->Height;
    bk_image->Picture->Bitmap->Width = bk_image->Width;
    bk_image->Canvas->Draw(-watch_panel->Left, -watch_panel->Top, form->imgBody->Picture->Graphic);
    delete tmp_bmp;

    ::AlphaBlend(bk_image->Canvas->Handle,
        0,0,bk_image->Width,bk_image->Height,
        form->imgWatch->Canvas->Handle, 0,0,bk_image->Width,bk_image->Height, blend);

    // 空白图片
    TImage * bk_blank_image = new TImage(watch_panel);
    bk_blank_image->BoundsRect = form->imgWatch->BoundsRect;
    bk_blank_image->Parent = watch_panel;
    tmp_bmp = new Graphics::TBitmap();
    bk_blank_image->Picture->Bitmap = tmp_bmp;
    bk_blank_image->Picture->Bitmap->Height = bk_blank_image->Height;
    bk_blank_image->Picture->Bitmap->Width = bk_blank_image->Width;
    bk_blank_image->Canvas->Draw(-watch_panel->Left, -watch_panel->Top, form->imgBody->Picture->Graphic);
    delete tmp_bmp;
    form->imgBlankWatch_list[panel_id-1] = bk_blank_image;

    ::AlphaBlend(bk_blank_image->Canvas->Handle,
        0,0,bk_blank_image->Width,bk_blank_image->Height,
        form->imgBlankWatchLevel->Canvas->Handle, 0,0,bk_blank_image->Width,bk_blank_image->Height, blend);


    TPaintBox * pb_level = new TPaintBox(watch_panel);
    pb_level->BoundsRect = form->pb_watch->BoundsRect;
    pb_level->OnPaint = form->pb_watch->OnPaint;
    pb_level->Parent = watch_panel;
    pb_level->Tag = panel_id-1;
    form->pb_watch_list[panel_id-1] = pb_level;

    TLabel * label_watch = new TLabel(watch_panel);
    label_watch->Caption = label;
    label_watch->Font = form->label_watch->Font;
    label_watch->Transparent = form->label_watch->Transparent;
    label_watch->BoundsRect = form->label_watch->BoundsRect;
    label_watch->Parent = watch_panel;
    form->label_watch_list[panel_id-1] = label_watch;

    TLabel * input_type = new TLabel(watch_panel);
    input_type->Caption = "MIC";
    input_type->AutoSize = form->input_type->AutoSize;
    input_type->BoundsRect = form->input_type->BoundsRect;
    input_type->OnMouseDown = form->input_type->OnMouseDown;
    input_type->Font = form->input_type->Font;
    input_type->Transparent = form->input_type->Transparent;
    input_type->Alignment = form->input_type->Alignment;
    input_type->Parent = watch_panel;
    input_type->Tag = panel_id;
    if (panel_id<=16)
    {
        form->input_type_lbl[panel_id-1] = input_type;
    }
    else
    {
        form->output_type_lbl[panel_id-17] = input_type;
    }
}
static void CreatePnlMix(int panel_id, TForm1 * form)
{
    form->mix_mute_btn[panel_id-1] = CopyInputPanelButton(form->pnlmix_mute, panel_id);
    form->mix_level_edit[panel_id-1] = CopyInputPanelEdit(form->pnlmix_level_edit, panel_id);
    form->mix_level_trackbar[panel_id-1] = CopyInputPanelTrackbar(form->pnlmix_level_trackbar, panel_id);
    CopyInputPanelLabel(form->pnlmix_dsp_num, panel_id)->Caption = String(char('A'-1+panel_id));
}

__fastcall TForm1::TForm1(TComponent* Owner)
    : TForm(Owner)
{
    input_panel_dsp_btn->SpeedCanvas->Font->Size = input_panel_dsp_btn->Font->Size;
    output_panel_dsp_btn->SpeedCanvas->Font->Size = output_panel_dsp_btn->Font->Size;

    startup_time = Now();

    // 参数处理
    on_loading = true;

    for (int i=0;i<32;i++)
    {
        level_meter[i][0] = -49;
        level_meter[i][1] = -100;
    }

    pnlOperator->Width = REAL_INPUT_DSP_NUM * PANEL_WIDTH + imgPresetBg->Width + REAL_OUTPUT_DSP_NUM * PANEL_WIDTH;
    pnlOperator->Width = max(pnlOperator->Width, Width-16);
        //HorzScrollBar->Visible = (pnlOperator->Width > Width);
    pnlOperator->Height = 650;//-(728-584);
    pnlOperator->Top = pnlHeader->Height;

    pnlMix->Width = REAL_INPUT_DSP_NUM * PANEL_WIDTH;                               

    pb_watch_list[0] = pb_watch;

    //----------------------------------------
    // 输入
    int input_panel_left = (pnlOperator->Width - (16 * PANEL_WIDTH + imgPresetBg->Width + REAL_OUTPUT_DSP_NUM * PANEL_WIDTH))/2;
    //watch_panel->Left = input_panel_left;
    //input_type->Left = input_panel_left;
    input_panel_bkground->Left = input_panel_left;
    input_panel_dsp_btn->Left = input_panel_left+4;
    input_panel_eq_btn->Left = input_panel_left+4;
    input_panel_invert_btn->Left = input_panel_left+4;
    input_panel_noise_btn->Left = input_panel_left+4;
    input_panel_mute_btn->Left = input_panel_left+4;
    input_panel_eq_btn->Left = input_panel_left+4;
    input_panel_level_edit->Left = input_panel_left+4;
    input_panel_trackbar->Left = input_panel_left;
    input_panel_dsp_num->Left = input_panel_left+4;
    input_panel_thumb->Left = input_panel_left+4;

    input_panel_bkground->Width = REAL_INPUT_DSP_NUM * PANEL_WIDTH;
    input_panel_bkground->Picture->Bitmap->Width = input_panel_bkground->Width;
    input_panel_bkground->Picture->Bitmap->Height = imgInputTemplate->Height;

    input_panel_bkground->Canvas->Draw(
        -input_panel_bkground->Left,
        -input_panel_bkground->Top,
        imgBody->Picture->Graphic);

    for (int i=0;i<REAL_INPUT_DSP_NUM;i++)
    {
        ::AlphaBlend(input_panel_bkground->Canvas->Handle,
            i*PANEL_WIDTH,imgInputTemplate->Top,PANEL_WIDTH,imgInputTemplate->Height,
            imgInputTemplate->Canvas->Handle, 0, 0, PANEL_WIDTH,imgInputTemplate->Height, blend);
    }

    input_level_edit[0] = input_panel_level_edit;
    input_level_trackbar[0] = input_panel_trackbar;
    SetWindowLong(input_panel_level_edit->Handle, GWL_STYLE, GetWindowLong(input_panel_level_edit->Handle, GWL_STYLE) | ES_RIGHT);
    input_panel_trackbar->OnChange(input_panel_trackbar);

    // 生成InputPanel
    input_type_lbl[0] = input_type;
    input_eq_btn[0] = input_panel_eq_btn;
    //input_comp_btn[0] = input_panel_comp_btn;
    //input_auto_btn[0] = input_panel_auto_btn;
    //input_default_btn[0] = input_panel_default_btn;
    input_invert_btn[0] = input_panel_invert_btn;
    input_noise_btn[0] = input_panel_noise_btn;
    input_mute_btn[0] = input_panel_mute_btn;
    input_dsp_name[0] = input_panel_dsp_num;
    input_thumb[0] = input_panel_thumb;
    for (int i=2;i<=INPUT_DSP_NUM;i++)
    {
        CreateInputPanel(i, this);
        CopyWatchPanel(i, this, String((char)('A'-1+i))+" ("+IntToStr(i)+")", (i-1) * PANEL_WIDTH + input_panel_left);
    }
    CopyWatchPanel(1, this, String((char)('A'-1+1))+" ("+IntToStr(1)+")", (1-1) * PANEL_WIDTH + input_panel_left);


    //------------------------------------
    // mix和master
    int mix_panel_left = input_panel_left + REAL_INPUT_DSP_NUM * PANEL_WIDTH;
    imgMasterMixBg->Left = mix_panel_left;
    imgPresetBg->Left = mix_panel_left;
    lblMaster->Left = mix_panel_left + PANEL_WIDTH;
    mix_panel_trackbar->Left = mix_panel_left;
    master_panel_trackbar->Left = mix_panel_left+mix_panel_trackbar->Width;
    mix_panel_level_edit->Left = mix_panel_left+5;
    btnMixMute->Left = mix_panel_left+4;
    master_panel_level_edit->Left = mix_panel_left+53;
    btnMasterMute->Left = mix_panel_left+52;
    mix_panel_dsp_num->Left = mix_panel_left+6;
    master_panel_dsp_num->Left = mix_panel_left+49;
    lblPresetName->Left = mix_panel_left+53;
    edtPreset->Left = mix_panel_left+51;


    imgMasterMixBg->Canvas->Draw(
        -imgMasterMixBg->Left,
        -imgMasterMixBg->Top,
        imgBody->Picture->Graphic);
    ::AlphaBlend(imgMasterMixBg->Canvas->Handle,
        0,0,imgMasterMixBg->Width,imgMasterMixBg->Height,
        imgMasterMix->Canvas->Handle, 0,0,imgMasterMixBg->Width,imgMasterMixBg->Height, blend);

    imgPresetBg->Canvas->Draw(
        -imgPresetBg->Left,
        -imgPresetBg->Top,
        imgBody->Picture->Graphic);
    ::AlphaBlend(imgPresetBg->Canvas->Handle,
        0,0,imgMasterMixBg->Width,imgPresetBg->Height,
        imgPreset->Canvas->Handle, 0,0,imgPreset->Width,imgPreset->Height, blend);

    mix_panel_trackbar->Thumb->Picture = input_panel_trackbar->Thumb->Picture;
    mix_panel_trackbar->Thumb->PictureHot = input_panel_trackbar->Thumb->PictureHot;
    mix_panel_trackbar->Thumb->PictureDown = input_panel_trackbar->Thumb->PictureDown;

    master_panel_trackbar->Thumb->Picture = input_panel_trackbar->Thumb->Picture;
    master_panel_trackbar->Thumb->PictureHot = input_panel_trackbar->Thumb->PictureHot;
    master_panel_trackbar->Thumb->PictureDown = input_panel_trackbar->Thumb->PictureDown;


    //------------------------------------
    // 输出
    int output_panel_left = mix_panel_left + imgPresetBg->Width;
    output_panel_bkground->Left = output_panel_left;
    output_panel_dsp_btn->Left = output_panel_left+4;
    output_panel_eq_btn->Left = output_panel_left+4;
    output_panel_comp_btn->Left = output_panel_left+4;
    output_panel_number_btn->Left = output_panel_left+4;
    output_panel_invert_btn->Left = output_panel_left+4;
    output_panel_mute_btn->Left = output_panel_left+4;
    output_panel_level_edit->Left = output_panel_left+4;
    output_panel_trackbar->Left = output_panel_left;
    output_panel_dsp_num->Left = output_panel_left+4;
    output_panel_thumb->Left = output_panel_left+4;

    output_panel_bkground->Width = REAL_OUTPUT_DSP_NUM * PANEL_WIDTH;
    output_panel_bkground->Picture->Bitmap->Width = output_panel_bkground->Width;
    output_panel_bkground->Canvas->Draw(
        -output_panel_bkground->Left,
        -output_panel_bkground->Top,
        imgBody->Picture->Graphic);

    {
        // 插入WatchLevel的补充
        imgWatchLevelBg->Height = pnlOperator->Height;
        imgWatchLevelBg->Width = pnlOperator->Width;
        imgWatchLevelBg->Picture->Bitmap->Width = imgWatchLevelBg->Width;
        imgWatchLevelBg->Picture->Bitmap->Height = imgWatchLevelBg->Height;
        imgWatchLevelBg->Canvas->Draw(
            -imgWatchLevelBg->Left,
            -imgWatchLevelBg->Top,
            imgBody->Picture->Graphic);
    }

    for (int i=0;i<REAL_OUTPUT_DSP_NUM;i++)
    {
        ::AlphaBlend(output_panel_bkground->Canvas->Handle,
            i*PANEL_WIDTH,imgOutputTemplate->Top,PANEL_WIDTH,imgOutputTemplate->Height,
            imgOutputTemplate->Canvas->Handle, 0, 0, PANEL_WIDTH,imgOutputTemplate->Height, blend);
/*        TRect templet_image_rect = Image3->BoundsRect;
        TRect dest_rect = TRect(i*PANEL_WIDTH,
                                templet_image_rect.Top,
                                (i+1)*PANEL_WIDTH,
                                templet_image_rect.Bottom);
        output_panel_bkground->Canvas->CopyRect(dest_rect, output_panel_bkground->Canvas, templet_image_rect);
*/
    }


    output_level_edit[0] = output_panel_level_edit;
    output_level_trackbar[0] = output_panel_trackbar;
    output_panel_trackbar->Thumb->PictureDown = input_panel_trackbar->Thumb->PictureDown;
    output_panel_trackbar->Thumb->PictureHot = input_panel_trackbar->Thumb->PictureHot;
    SetWindowLong(output_panel_level_edit->Handle, GWL_STYLE, GetWindowLong(output_panel_level_edit->Handle, GWL_STYLE) | ES_RIGHT);
    output_panel_trackbar->OnChange(output_panel_trackbar);

    output_eq_btn[0] = output_panel_eq_btn;
    output_comp_btn[0] = output_panel_comp_btn;
    output_invert_btn[0] = output_panel_invert_btn;
    output_mute_btn[0] = output_panel_mute_btn;
    output_dsp_name[0] = output_panel_dsp_num;
    output_thumb[0] = output_panel_thumb;

    for (int i=2;i<=REAL_OUTPUT_DSP_NUM;i++)
    {
        CreateOutputPanel(i, this);
    }

    //----------------------------------
    // 生成pnlmix背景
    pnlmix_background->Picture->Bitmap->Width = REAL_INPUT_DSP_NUM * PANEL_WIDTH;  // xDO: 原来是17个，包括automix，现在只按照输入数量
    for (int i=1;i<REAL_INPUT_DSP_NUM;i++)   // xDO: 原来是17个，包括automix，现在只按照输入数量
    {
        TRect templet_image_rect = pnlmix_background->BoundsRect;
        templet_image_rect.Right = PANEL_WIDTH;
        templet_image_rect.Bottom = templet_image_rect.Bottom - templet_image_rect.Top;
        templet_image_rect.Top = 0;

        TRect dest_rect = TRect(i*PANEL_WIDTH,
                                templet_image_rect.Top,
                                (i+1)*PANEL_WIDTH,
                                templet_image_rect.Bottom);
        pnlmix_background->Canvas->CopyRect(dest_rect, pnlmix_background->Canvas, templet_image_rect);
    }

    LoadMixBmp();

    // 生成PnlMix
    mix_mute_btn[0] = pnlmix_mute;
    for (int i=2;i<=REAL_INPUT_DSP_NUM;i++)    // xDO: 原来是17个，包括automix，现在只按照输入数量
    {
        CreatePnlMix(i, this);
    }
    mix_level_edit[0] = pnlmix_level_edit;
    mix_level_trackbar[0] = pnlmix_level_trackbar;
    SetWindowLong(pnlmix_level_edit->Handle, GWL_STYLE, GetWindowLong(pnlmix_level_edit->Handle, GWL_STYLE) | ES_RIGHT);
    pnlmix_level_trackbar->OnChange(pnlmix_level_trackbar);

    //----------------------------------
    for (int i=17;i<17+REAL_OUTPUT_DSP_NUM;i++)
    {
        CopyWatchPanel(i, this, String(1+(i-17)), imgMasterMixBg->Left + imgMasterMixBg->Width + (i-17) * PANEL_WIDTH);
    }


    input_level_edit[16] = mix_panel_level_edit;
    input_level_trackbar[16] = mix_panel_trackbar;
    SetWindowLong(mix_panel_level_edit->Handle, GWL_STYLE, GetWindowLong(mix_panel_level_edit->Handle, GWL_STYLE) | ES_RIGHT);

    SetWindowLong(master_panel_level_edit->Handle, GWL_STYLE, GetWindowLong(master_panel_level_edit->Handle, GWL_STYLE) | ES_RIGHT);

    // DSP详细配置界面的数据
    last_default_btn = NULL;
    last_out_num_btn = NULL;
    last_dsp_btn = NULL;
    mix_panel_state = 0;

    low_freq = 0;
    low_gain = 0;
    high_freq = 0;
    high_gain = 0;

    memset(eq_freq, 0, sizeof(eq_freq));
    memset(eq_gain, 0, sizeof(eq_gain));
    memset(eq_q, 0, sizeof(eq_q));

    //---------------------------------------------
    // DSP详细配置界面
    panel_agent = new PanelAgent(filter_set);
    paint_agent = new PaintAgent(PaintBox1, pbComp, filter_set);
    filter_set.Register(paint_agent, panel_agent);

    pnlOprX->DoubleBuffered = true;
    pnlOperator->DoubleBuffered = true;
    pnlDspDetail->DoubleBuffered = true;
    pnlHeader->DoubleBuffered = true;
    pnlMonitor->DoubleBuffered = true;
    pnlConnect->DoubleBuffered = true;

    // 受到滤波器数量影响
    panel_agent->SetPanel(0, panelBand0, edtFreq0, edtQ0, edtGain0, cbType0, cbBypass0);
    panel_agent->SetPanel(1, panelBand1, edtFreq1, edtQ1, edtGain1, cbType1, cbBypass1);
    panel_agent->SetPanel(2, panelBand2, edtFreq2, edtQ2, edtGain2, cbType2, cbBypass2);
    panel_agent->SetPanel(3, panelBand3, edtFreq3, edtQ3, edtGain3, cbType3, cbBypass3);
    panel_agent->SetPanel(4, panelBand4, edtFreq4, edtQ4, edtGain4, cbType4, cbBypass4);
    panel_agent->SetPanel(5, panelBand5, edtFreq5, edtQ5, edtGain5, cbType5, cbBypass5);
    panel_agent->SetPanel(6, panelBand6, edtFreq6, edtQ6, edtGain6, cbType6, cbBypass6);
    panel_agent->SetPanel(7, panelBand7, edtFreq7, edtQ7, edtGain7, cbType7, cbBypass7);
    //panel_agent->SetPanel(8, panelBand8, edtFreq8, edtQ8, edtGain8, cbType8, cbBypass8);
    panel_agent->SetPanel(10, panelBand9, edtFreq9, edtQ9, edtGain9, cbType9, cbBypass9);
    panel_agent->SetPanel(11, panelBand10, edtFreq10, edtQ10, edtGain10, cbType10, cbBypass10);

    InitConfigMap(config_map);
    btnDspResetEQ->Click();
    ResetLHFilter();

    ApplyConfigToUI();

    SetPresetId(1);
    for (int i=0;i<PRESET_NUM;i++)
    {
        all_config_map[i] = config_map;
    }

    InitGdipus();

    pnlDspDetail->BringToFront();

    mireg0 = 0;

    on_loading = false;

    // 私有变量初始化
    keep_live_count = CONTROL_TIMEOUT_COUNT;
    received_cmd_seq = 0;       pre_received_msg_id = 0;
    device_connected = false;

    // 读取配置
    TIniFile * ini_file = new TIniFile(ExtractFilePath(Application->ExeName) + "SMC.ini");
    last_device_id = ini_file->ReadString("connection", "last_id", "");
    is_manual_disconnect = ini_file->ReadBool("connection", "is_disconnect", false);
    delete ini_file;

    // 需要初始化完成后才开启SLP，否则自动连接后会立即调整界面，导致Input面板还未初始化时就做调整，这样就会出现异常。
    udpSLPList[0] = udpSLP;
    udpSLPList[1] = udpSLP1;
    udpSLPList[2] = udpSLP2;
    tmSLP->Enabled = true;

    int base_order = 0;
    // 整理 TabOrder
    for (int i=0;i<16;i++)
    {
        if (input_level_edit[i] != NULL)
            input_level_edit[i]->TabOrder = base_order+i;
    }
    master_panel_level_edit->TabOrder = base_order+16+0;
    for (int i=0;i<16;i++)
    {
        if (output_level_edit[i] != NULL)
            output_level_edit[i]->TabOrder = base_order+16+1+i;
    }

    for (int i=0;i<16;i++)
    {
        if (input_level_trackbar[i] != NULL)
            input_level_trackbar[i]->TabOrder = base_order+16+1+16+i;
    }
    master_panel_trackbar->TabOrder = base_order+16+1+16+16+0;
    for (int i=0;i<16;i++)
    {
        if (output_level_trackbar[i] != NULL)
            output_level_trackbar[i]->TabOrder = base_order+16+1+16+16+1+i;
    }
}
//---------------------------------------------------------------------------
__fastcall TForm1::~TForm1()
{
    // 给原先的设备发送断链消息
    SendDisconnect();

    if (h_sem != NULL)
    {
        ReleaseSemaphore(h_sem, 1, NULL);
        CloseHandle(h_sem);
    }

    for (int i=0;i<lvDevice->Items->Count;i++)
    {
        TListItem * item = lvDevice->Items->Item[i];
        DeviceData * data = (DeviceData*)item->Data;
        if (data != NULL)
        {
            delete data;
            item->Data = 0;
        }
    }
    for (int i=0;i<16;i++)
    {
        if (MixPicture[i] != NULL)
        {
            delete MixPicture[i];
            MixPicture[i] = NULL;
        }
    }
    delete panel_agent;
    delete paint_agent;
}
//---------------------------------------------------------------------------
LRESULT TForm1::new_pnlSystem_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    LRESULT r = CallWindowProc(Form1->old_pnlSystem_proc, hwnd, msg, wp, lp);

    /*if ( msg == WM_CTLCOLOREDIT )
    {
        if (hwnd == Form1->hIpEdit)
        {
            HDC hDc = (HDC)wp;
            SetTextColor(hDc, clAqua);
            SetBkColor(hDc, 0x004A392C);
        }
    }*/
    return r;
}

void TForm1::ShowLockConfigArea()
{
    Label27              ->Hide();
    Image2               ->Hide();
    Label32              ->Hide();
    Image4               ->Hide();
    Image5               ->Hide();
    Label33              ->Hide();
    Label24              ->Hide();
    imgSystemBg3         ->Hide();
    btnUnlock            ->Hide();
    edtRemainTime        ->Hide();
    edtRemainRebootCount ->Hide();
    edtLockedString1     ->Hide();
    edtUnlockPassword    ->Hide();

    Label19              ->Show();
    Label22              ->Show();
    imgSystemBg4         ->Show();
    imgSystemBg5         ->Show();
    imgSystemBg6         ->Show();
    imgSystemBg7         ->Show();
    imgSystemBg1         ->Show();
    btnKeyPasswordUp     ->Show();
    btnKeyPasswordDown   ->Show();
    btnSetLock           ->Show();
    edtRebootCount       ->Show();
    edtRunningTimer      ->Show();
    cbLockedString       ->Show();
    cbRebootCount        ->Show();
    cbRunningTimer       ->Show();
    edtLockedString      ->Show();
    edtKeyPassword       ->Show();
    edtPassword          ->Show();
}
void TForm1::HideLockConfigArea()
{
    Label27              ->Show();
    Image2               ->Show();
    Label32              ->Show();
    Image4               ->Show();
    Image5               ->Show();
    Label33              ->Show();
    Label24              ->Show();
    imgSystemBg3         ->Show();
    btnUnlock            ->Show();
    edtRemainTime        ->Show();
    edtRemainRebootCount ->Show();
    edtLockedString1     ->Show();
    edtUnlockPassword    ->Show();

    Label19              ->Hide();
    Label22              ->Hide();
    imgSystemBg4         ->Hide();
    imgSystemBg5         ->Hide();
    imgSystemBg6         ->Hide();
    imgSystemBg7         ->Hide();
    imgSystemBg1         ->Hide();
    btnKeyPasswordUp     ->Hide();
    btnKeyPasswordDown   ->Hide();
    btnSetLock           ->Hide();
    edtRebootCount       ->Hide();
    edtRunningTimer      ->Hide();
    cbLockedString       ->Hide();
    cbRebootCount        ->Hide();
    cbRunningTimer       ->Hide();
    edtLockedString      ->Hide();
    edtKeyPassword       ->Hide();
    edtPassword          ->Hide();
}
void __fastcall TForm1::FormCreate(TObject *Sender)
{
    // 设置页的界面调整
    Label19             ->Top = Label19             ->Top - 246;
    Label22             ->Top = Label22             ->Top - 246;
    imgSystemBg4        ->Top = imgSystemBg4        ->Top - 246;
    imgSystemBg5        ->Top = imgSystemBg5        ->Top - 246;
    imgSystemBg6        ->Top = imgSystemBg6        ->Top - 246;
    imgSystemBg7        ->Top = imgSystemBg7        ->Top - 246;
    imgSystemBg1        ->Top = imgSystemBg1        ->Top - 246;
    btnKeyPasswordUp    ->Top = btnKeyPasswordUp    ->Top - 246;
    btnKeyPasswordDown  ->Top = btnKeyPasswordDown  ->Top - 246;
    btnSetLock          ->Top = btnSetLock          ->Top - 246;
    edtRebootCount      ->Top = edtRebootCount      ->Top - 246;
    edtRunningTimer     ->Top = edtRunningTimer     ->Top - 246;
    cbLockedString      ->Top = cbLockedString      ->Top - 246;
    cbRebootCount       ->Top = cbRebootCount       ->Top - 246;
    cbRunningTimer      ->Top = cbRunningTimer      ->Top - 246;
    edtLockedString     ->Top = edtLockedString     ->Top - 246;
    edtKeyPassword      ->Top = edtKeyPassword      ->Top - 246;
    edtPassword         ->Top = edtPassword         ->Top - 246;

    // 移动6个panel
    pnlOperator->Parent = this;
    pnlMonitor->Parent = this;
    pnlSystem->Parent = this;
    pnlMist->Parent = this;
    pnlConnect->Parent = this;
    PageControl1->Hide();

    TInitCommonControlsEx ICC;
    ICC.dwSize = sizeof(TInitCommonControlsEx);
    ICC.dwICC  = ICC_INTERNET_CLASSES;
    if(!InitCommonControlsEx(&ICC))
    {
        ShowMessage("IP地址控件失效");
    }
    hIpEdit = CreateWindow(WC_IPADDRESS,"",WS_CHILD|WS_VISIBLE,
        edtIp->Left, edtIp->Top, edtIp->Width, edtIp->Height, edtIp->Parent->Handle,
        0,HInstance,NULL);

    old_pnlSystem_proc = (WNDPROC)GetWindowLong(hIpEdit, GWL_WNDPROC);
    SetWindowLong(hIpEdit, GWL_WNDPROC, (long)new_pnlSystem_proc);

    //old_pnlSystem_proc = pnlSystem->WindowProc;
    //pnlSystem->WindowProc = new_pnlSystem_proc;

    local_mac_list = GetMacList();

    // 是否调试PC
    is_inner_pc = false;
    if (ParamCount>0 && ParamStr(1)=="debug")
    {
        is_inner_pc = true;
    }
    else if (ParamCount>0 && ParamStr(1)=="nodebug")
    {
    }
    else
    {
        for (int i=0;i<sizeof(inner_mac)/sizeof(String);i++)
        {                       
            if (local_mac_list.Pos(inner_mac[i]) > 0)
            {
                is_inner_pc = true;
                break;
            }
        }
    }

    cbLogEnabled->Checked = is_inner_pc;
    if (is_inner_pc)
    {
        edtDeviceType->Show();
        edtStartBuildTime->Show();

        btnUnlockExt->Show();
        btnLeaveTheFactory->Show();
        cbLedTest->Show();

        btnClearDataAndTime->Show();

        edtDeviceFullName->Show();
        lblKeepLiveCheck->Show();
        lblDeviceRunningTime2->Show();
    }
    else
    {
        // 最多开8个
        h_sem = CreateSemaphore(NULL, 16, 16, "MATRIX");
        if (WaitForSingleObject(h_sem, 1) != WAIT_OBJECT_0)
        {
            ShowMessage("最多只能打开 16 个上位机");
            CloseHandle(h_sem);
            h_sem = NULL;
            Application->Terminate();
        }
    }

    pnlOperator->Show();

    SetWindowLong(edtPreset->Handle, GWL_STYLE, GetWindowLong(edtPreset->Handle, GWL_STYLE) | ES_CENTER);

    // 加载字体
    HRSRC hRsrc = FindResource(NULL, "DIGIFAW", RT_RCDATA);
    DWORD cbSize = SizeofResource(NULL, hRsrc);
    HGLOBAL hMem = LoadResource(NULL, hRsrc);
    LPVOID pvData = LockResource(hMem);
    DWORD nFontsInstalled = 0;
    HANDLE hFontInstalled = AddFontMemResourceEx(pvData, cbSize, NULL, &nFontsInstalled);
    if (hFontInstalled == NULL)
    {
    }

    // 输出结构体大小
    AppendLog(GetTime()+"InputConfigMap:" + IntToStr(sizeof(InputConfigMap)));
    AppendLog(GetTime()+"OutputConfigMap:" + IntToStr(sizeof(OutputConfigMap)));
    //AppendLog(GetTime()+"MasterMixConfigMap:" + IntToStr(sizeof(MasterMixConfigMap)));
    AppendLog(GetTime()+"ConfigMap:" + IntToStr(sizeof(ConfigMap)));
    //AppendLog(GetTime()+"mix_mute:" + IntToStr(sizeof(config_map.master_mix.mix_mute)));
    AppendLog(GetTime()+"NotStorageCmd:" + IntToStr(sizeof(NotStorageCmd)));
    AppendLog(GetTime()+"GlobalConfig:" + IntToStr(sizeof(GlobalConfig)));

    // 根据数量初始化控制器
    // Panel->Tag
    // Panel->Left
    // Label->Caption

    //tsSearch->Show();
    this->Repaint();

    pnlMix->BringToFront();

    // 加载LOGO图片
    if (FileExists("logo.bmp"))
    {
        imgLogo->Picture->LoadFromFile("logo.bmp");
        // Height -- 3, 73px  -- 中线 38
        // imgLogo->Height/2  <-- 38
        imgLogo->Top = 38 - imgLogo->Height/2;
        imgLogo->Left = 1500;
    }

    // 上位机时间
    edtStartBuildTime->Text = " \t \t";
    edtStartBuildTime->Text = edtStartBuildTime->Text + DateTime2Str(GetDateTimeFromMarco(compile_time));

    // 版本号
    lblVersion->Caption = "-------- " +VersionToStr(version)+ " " + DateTime2Str(GetDateTimeFromMarco(compile_time));

    // 默认全局配置
    global_config.avaliable_preset[0] = 1;
    global_config.active_preset_id = 1;
    global_config.version = GLOBAL_CONFIG_VERSION;
    global_config.auto_saved = 1;
    ApplyConfigToUI();
}
//---------------------------------------------------------------------------
void TForm1::SendBuffer(AnsiString AHost, const int APort, void *ABuffer, const int AByteCount)
{
    udpControl->SendBuffer(dst_ip, APort, ABuffer, AByteCount);
}
void TForm1::SendCmd2(String cmd)
{
    if (on_loading || !udpControl->Active || keep_live_count >= CONTROL_TIMEOUT_COUNT)
    {
        return;
    }

    if (!device_connected && !IsKeepliveCmd(cmd.c_str()))
    {
        return;
    }

    // 特殊处理
    try
    {
        udpControl->Send(dst_ip, UDP_PORT_CONTROL, cmd);
    }
    catch(...)
    {
        CloseControlLink("发送消息失败, SendCmd2");
    }
    
// TODO: 上面修改了 return 之后，这一段失效了
#if 0
    if (!on_loading)
    {
        if (cmd.id < GetOffsetOfData(&config_map.op_code))
        {
            SetFileDirty(true);
        }
    }
#endif
}
void TForm1::SendDisconnect()
{
    if (udpControl->Active)
    {
        try
        {
            String cmd_text = D1608CMD_CONTROL_FLAG;
            cmd_text = cmd_text+"config.action=disconnect"+D1608CMD_TAIL;
            udpControl->Send(dst_ip, UDP_PORT_CONTROL, cmd_text);
        }
        catch(...)
        {
            Form1->AppendLog(__FUNC__);
        }
    }
}
void TForm1::SendLogBuff(int udp_port, void * buff, int size)
{
    TPackage package = {0};
    package.udp_port = udp_port;
    memcpy(package.data, buff, size);
    package.data_size = size;

    sendcmd_list.insert(sendcmd_list.begin(), package);

    int sendcmd_list_length = sendcmd_list.size();
    // 保存在队列中
    if (sendcmd_list_length == 1)
    {
        AppendLog(GetTime()+"发出UDP信息");
        try
        {
            SendBuffer(dst_ip, udp_port, buff, size);
        }
        catch(...)
        {
            CloseControlLink("发送消息失败, SendLogBuff");
        }
        sendcmd_delay_count = delay_send_cmd_check_count * 3;
    }
}
void TForm1::SendCmd(String cmd)
{
    if (on_loading || !udpControl->Active || keep_live_count >= CONTROL_TIMEOUT_COUNT)
    {
        return;
    }
    if (!device_connected && !IsKeepliveCmd(cmd.c_str()))
    {
        return;
    }

    // 需要发送的命令id是否在队列中
    if (sendcmd_list.size() > 1)
    {
        for (vector<TPackage>::iterator iter=sendcmd_list.begin();
            iter+1!=sendcmd_list.end();
            iter++)
        {
            if (iter->udp_port != UDP_PORT_CONTROL)
                continue;

            char * package_sendcmd = (char*)(iter+1)->data;
            if (GetTextCmdVar(cmd) == GetTextCmdVar(package_sendcmd))
            {
                sendcmd_list.erase(iter);
                break;
            }
        }
    }

    TPackage package = {0};
    package.udp_port = UDP_PORT_CONTROL;
    memcpy(package.data, cmd.c_str(), cmd.Length());
    package.data_size = cmd.Length();

    // 保存在队列中
    sendcmd_list.insert(sendcmd_list.begin(), package);

    int sendcmd_list_length = sendcmd_list.size();
    // 如果原先队列是空，那么触发一次发送消息
    if (sendcmd_list_length == 1)
    {
        AppendLog(GetTime()+"发出消息:"+cmd);
        SendCmd2(cmd);
        sendcmd_delay_count = delay_send_cmd_check_count * 3;
    }
    else
    {
        TPackage package = sendcmd_list.back();
        char * p_cmd = package.data;

        AppendLog(GetTime()+"追加到队列"+cmd+"|"+"    "+String(p_cmd));
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btnRefreshClick(TObject *Sender)
{
    //AppendLog("刷新设备");

    // 删除上次已经未扫描到的项目
    for (int i=lvDevice->Items->Count-1;i>=0;i--)
    {
        TListItem * item = lvDevice->Items->Item[i];

        DeviceData * data = (DeviceData*)item->Data;
        data->count--;

        if (data->count == 0)
        {
            lvDevice->Items->Delete(i);
            delete data;
        }
    }

    GetLocalIpListEx(ip_info);
    if (ip_info.empty())
    {
        GetLocalIpList(ip_info);
    }

    // ip_info 导出到界面
    cbNetwork->Clear();
    cbNetwork->Items->Add("所有网卡");
    for (UINT i=0; i<ip_info.size(); i++)
    {
        cbNetwork->Items->Add(ip_info[i].ip);
    }
    cbNetwork->ItemIndex = 0;
    // 读取IP地址保存值
    TIniFile * ini_file = new TIniFile(ExtractFilePath(Application->ExeName) + "SMC.ini");
    String saved_network = ini_file->ReadString("connection", "network", "");
    delete ini_file;
    if (saved_network != "")
    {
        // 查找
        for (int i=0; i<cbNetwork->Items->Count; i++)
        {
            if (cbNetwork->Items->Strings[i] == saved_network)
            {
                cbNetwork->ItemIndex = i;
                break;
            }
        }
    }

    bool is_ip_changed = false;
    UINT ip_count = 0;
    for (UINT i=0;i<3;i++)
    {
        if (udpSLPList[i]->Active && udpSLPList[i]->Bindings->Count == 1)
        {
            ip_count++;
            
            String ip = udpSLPList[i]->Bindings->Items[0]->IP;
            bool ip_found = false;
            for (UINT j=0;j<ip_info.size();j++)
            {
                if (ip == ip_info[j].ip)
                {
                    ip_found = true;
                    break;
                }
            }
            if (ip_found == false)
            {
                is_ip_changed = true;
                break;
            }
        }
    }
    if (ip_count != ip_info.size())
        is_ip_changed= true;



    if (is_ip_changed)
    {
        for (int i=0;i<3;i++)
        {
            udpSLPList[i]->Active = false;
            udpSLPList[i]->Bindings->Clear();
        }
        for (UINT i=0;i<ip_info.size() && i<3;i++)
        {
            try
            {
                if (cbNetwork->ItemIndex<=0 || (cbNetwork->Text == ip_info[i].ip))
                {
                    udpSLPList[i]->Bindings->Add();
                    udpSLPList[i]->Bindings->Items[0]->IP = ip_info[i].ip;
                    udpSLPList[i]->Bindings->Items[0]->Port = 0;
                    udpSLPList[i]->Active = true;
                }
            }
            catch(...)
            {
                udpSLPList[i]->Active = false;
                AppendLog(__FUNC__);
            }
        }
    }
    else
    {
        for (UINT i=0;i<ip_info.size() && i<3;i++)
        {
            if (cbNetwork->ItemIndex>0 && (cbNetwork->Text != ip_info[i].ip))
            {
                udpSLPList[i]->Active = false;
                udpSLPList[i]->Bindings->Clear();
            }
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::udpSLPUDPRead(TObject *Sender,
      TStream *AData, TIdSocketHandle *ABinding)
{
    TIdUDPServer * udp_server = (TIdUDPServer*)Sender;
    udp_server->Tag++;

    slp_count++;
    char str[1500] = "";

    AData->ReadBuffer(str, std::min(sizeof(str)-1, AData->Size));

    DictItem dict[20] = {0};
    if (!strnicmp(str, SLP_FLAG, strlen(SLP_FLAG)))
	{
        ParseDict(dict, 20, str+strlen(SLP_FLAG));
	}
    else if (!strnicmp(str, MSLP_FLAG, strlen(MSLP_FLAG)))
    {
        ParseDict(dict, 20, str+strlen(MSLP_FLAG));
    }
    else
    {
        return;
    }

    T_slp_pack_Str slp_pack_str;

    slp_pack_str.ip = GetDictValue(dict, 20, "IP");
    slp_pack_str.mac = GetDictValue(dict, 20, "MAC");
    slp_pack_str.name = GetDictValue(dict, 20, "Name");
    slp_pack_str.default_device_name = GetDictValue(dict, 20, "Device");
    slp_pack_str.sn = GetDictValue(dict, 20, "SN");
    slp_pack_str.links = String(GetDictValue(dict, 20, "Links")).ToIntDef(0);
    String app_time_str = GetDictValue(dict, 20, "APP_Time");
    if (app_time_str == "")
    {
        slp_pack_str.delay = 1000;
        delay_send_cmd_check_count = 5;
        restor_delay_check_count = 5;
    }
    else
    {
        TTime app_time(app_time_str);
        slp_pack_str.delay = ((double)Now()-app_time)*24*3600*1000;

        if (last_connection.data.mac == slp_pack_str.mac)
        {
            last_connection.data.delay = (last_connection.data.delay*0+slp_pack_str.delay*10)/10;

            // 修改 delay_send_cmd_check_count
            int new_delay_send_cmd_check_count = 5 + (last_connection.data.delay+10)/33;
            if (delay_send_cmd_check_count != new_delay_send_cmd_check_count)
            {
                delay_send_cmd_check_count = new_delay_send_cmd_check_count;
            }
            // 修改 restor_delay_check_count
            int new_restor_delay_check_count = 5 + (last_connection.data.delay+10)/60;
            if (restor_delay_check_count != new_restor_delay_check_count)
            {
                restor_delay_check_count = new_restor_delay_check_count;
            }
        }
    }

    String version_string = GetDictValue(dict, 20, "VER");
    TStrings * version_part = new TStringList();
    version_part->Delimiter = '.';
    version_part->DelimitedText = version_string;
    if (version_part->Count == 4)
    {
        int p1 = ("0x"+version_part->Strings[0]).ToIntDef(0);
        int p2 = ("0x"+version_part->Strings[1]).ToIntDef(0);
        int p3 = ("0x"+version_part->Strings[2]).ToIntDef(0);
        int p4 = ("0x"+version_part->Strings[3]).ToIntDef(0);
        slp_pack_str.version = (p1<<24) + (p2<<16) + (p3<<8) + p4;
    }
    else
    {
        return;
    }

    if (slp_pack_str.name == "")
    {
        slp_pack_str.name = slp_pack_str.default_device_name + "-" + slp_pack_str.sn;
    }

    TListItem * item = NULL;
    // 查找是否列表中已经存在
    for (int i=0;i<lvDevice->Items->Count;i++)
    {
        TListItem * find_item = lvDevice->Items->Item[i];
        if (find_item->SubItems->Strings[6] == slp_pack_str.mac)
        {
            // 更新属性
            find_item->Caption = slp_pack_str.name.UpperCase()+"|"+IntToStr(slp_pack_str.links)+"|"+IntToStr(slp_pack_str.delay);
            find_item->SubItems->Strings[0] = slp_pack_str.ip;
            find_item->SubItems->Strings[1] = ABinding->IP;
            //find_item->SubItems->Strings[2] = device_id;
            find_item->SubItems->Strings[3] = slp_pack_str.name;
            find_item->SubItems->Strings[4] = slp_pack_str.default_device_name;
            find_item->SubItems->Strings[5] = "废弃，原先是cpuid";
            find_item->SubItems->Strings[6] = slp_pack_str.mac;
            find_item->SubItems->Strings[7] = slp_pack_str.sn;
            find_item->SubItems->Strings[8] = IsCompatibility(slp_pack_str.version);
            find_item->SubItems->Strings[9] = slp_pack_str.links;
            //find_item->SubItems->Strings[10] = slp_pack_str.delay;

            DeviceData * data = (DeviceData*)find_item->Data;
            data->count = 3;
            data->data = slp_pack_str;

            item = find_item;
            break;
        }
    }

    if (item == NULL)
    {
        item = lvDevice->Items->Add();
        item->Caption = slp_pack_str.name.UpperCase()+"|"+IntToStr(slp_pack_str.links)+"|"+IntToStr(slp_pack_str.delay);
        item->SubItems->Add(slp_pack_str.ip);
        item->SubItems->Add(ABinding->IP);
        item->SubItems->Add(""/*slp_pack.id*/);
        item->SubItems->Add(slp_pack_str.name);
        item->SubItems->Add(slp_pack_str.default_device_name);
        item->SubItems->Add("废弃，原先是cpuid");
        item->SubItems->Add(slp_pack_str.mac);
        item->SubItems->Add(slp_pack_str.sn);
        item->SubItems->Add(IsCompatibility(slp_pack_str.version));
        item->SubItems->Add(slp_pack_str.links);
        //item->SubItems->Add(slp_pack_str.delay);

        DeviceData * data = new DeviceData;
        data->count = 4;
        data->data = slp_pack_str;
        item->Data = data;
    }

    if (!udpControl->Active && !is_manual_disconnect)
    {
        if ((last_device_id == "") || (cbNetwork->ItemIndex>0) || (last_device_id == slp_pack_str.mac) || ( Now() > startup_time+10.0/3600/24 ))
        {
            // 连接第一个 或者 匹配ID 或者 5秒钟 或者 没有找到原先的设备
            item->Selected = true;
            btnSelectClick(NULL);
            startup_time = startup_time + 100000;
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btnSelectClick(TObject *Sender)
{
    TListItem * selected = lvDevice->Selected;
    if (selected == NULL)
    {
        return;
    }

    // PC <--> Device
    if (package_list.size() != 0)
    {
        return;
    }

    // 是否版本兼容
    if (selected->SubItems->Strings[8] != "YES" && selected->SubItems->Strings[8] != "YELLOW")
    {
        if (Sender != NULL)
            ShowMessage("版本不兼容，请更新软件。\n上位机版本："+IntToHex((int)version,8)+"，下位机版本："+selected->SubItems->Strings[8]);
        return;
    }

    DeviceData * data = (DeviceData*)selected->Data;
    if (data != NULL)
    {
        if (data->data.links >= 16)
        {
            if (Sender != NULL)
                ShowMessage("下位机连接池已满（16）");
            return;
        }
    }

    // 给原先的设备发送断链消息
    SendDisconnect();

    is_manual_disconnect = false;
    received_cmd_seq = 0;   pre_received_msg_id = 0;

    dst_ip = selected->SubItems->Strings[0];
    String broadcast_ip = selected->SubItems->Strings[1];
        lvLog->Clear();

    admin_password = "";

    sendcmd_list.clear();
    // 初始化socket
    CloseControlLink("准备连接到新的下位机, 重置连接");
    static int STATIC_PORT = 0;
    udpControl->Bindings->Clear();
    udpControl->Bindings->Add();
    udpControl->Bindings->Items[0]->IP = broadcast_ip;
    udpControl->Bindings->Items[0]->Port = STATIC_PORT;
    udpControl->Active = true;
    STATIC_PORT = udpControl->Bindings->Items[0]->Port;

    lblCtrlPort->Caption = "端口号: "+IntToStr(udpControl->Bindings->Items[0]->Port);

    UpdateCaption();

    StartReadCurrentPreset(true);// 0xFF表示上电时期同步当前Preset，并且会lock住下位机
    restor_delay_count = restor_delay_check_count * 3;
    tmDelayBackup->Enabled = true;

    // 获取下位机PRESET数据，同时这个消息作为下位机认可的消息
    TPackage package = {0};
    package.udp_port = UDP_PORT_READ_PRESET;
    D1608PresetCmd preset_cmd(version);
    strcpy(preset_cmd.flag, D1608PRESETCMD_LINK_FLAG);
    preset_cmd.lock_flag = 1;

    // 其他设备信息
    preset_cmd.preset = 0; // 读取global_config
    preset_cmd.store_page = 1;  // 第2页，指其他设备信息
    memcpy(package.data, &preset_cmd, offsetof(D1608PresetCmd, data));
    package.data_size = offsetof(D1608PresetCmd, data);
    read_one_preset_package_list.insert(read_one_preset_package_list.begin(), package);
    if (read_one_preset_package_list.size() == 1)
        SendBuffer(dst_ip, package.udp_port, package.data, package.data_size);
    // global_config
    preset_cmd.preset = 0; // 读取global_config
    preset_cmd.store_page = 0;  // 第1页，指global_config
    memcpy(package.data, &preset_cmd, offsetof(D1608PresetCmd, data));
    package.data_size = offsetof(D1608PresetCmd, data);
    read_one_preset_package_list.insert(read_one_preset_package_list.begin(), package);
    if (read_one_preset_package_list.size() == 1)
        SendBuffer(dst_ip, package.udp_port, package.data, package.data_size);


    pnlOperator->Show();
    pnlDspDetail->Hide();
    pnlMix->Hide();
    //cbWatch->Down = true;

    keep_live_count = 0;
    global_config_loaded = false;

    last_device_id = selected->SubItems->Strings[6];
    // 记录在配置文件里
    TIniFile * ini_file = new TIniFile(ExtractFilePath(Application->ExeName) + "SMC.ini");
    ini_file->WriteString("connection", "last_id", last_device_id);
    ini_file->WriteBool("connection", "is_disconnect", is_manual_disconnect);
    delete ini_file;

    memset(display_buffer, 0, sizeof(display_buffer));

    edtMAC->Text = selected->SubItems->Strings[6];

    ClearUI();   
    edtDeviceFullName_data = selected->SubItems->Strings[4]+"-"+lvDevice->Selected->SubItems->Strings[7];
    tmWatch->Enabled = false;

    // 从数据中获取版本信息
    last_connection = *(DeviceData*)selected->Data;

    btnGetLog->Enabled = true;

    lblSn->Caption = "sn: " + String(last_connection.data.sn);
    lblConfigFilename->Caption = "file: --";

    // 如果在设置页面，那么应该自动转到主页面
    if (pnlSystem->Visible)
    {
        btnMainUI->OnMouseDown(btnMainUI, mbLeft, TShiftState(), 0, 0);
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::tmSLPTimer(TObject *Sender)
{
    tmSLP->Enabled = false;

    bool need_chcek_ack_count = false;
    static TTime last_check_timr = 0;

    if ((double)(Now() - last_check_timr) > (1.0/24/60/6/2))    // 5秒
    {
        last_check_timr = Now();
        need_chcek_ack_count = true;
        //AppendLog("check slp ack.");
    }

    btnRefresh->Click();
    if (ip_info.size() > 0)
    {
        for (int i=0;i<3;i++)
        {
            if (udpSLPList[i]->Bindings->Count == 1)
            {
                if (need_chcek_ack_count && udpSLPList[i]->Tag == 0)
                {
                    String ip = udpSLPList[i]->Bindings->Items[0]->IP;
                    udpSLPList[i]->Active = false;
                }

                try{
                    if (udpSLPList[i]->Bindings->Count > 0)
                    {
                        udpSLPList[i]->Active = true;
                        String ip = udpSLPList[i]->Bindings->Items[0]->IP;

                        String text_cmd;

                        if (is_inner_pc)
                            text_cmd = MSLP_FLAG;
                        else
                            text_cmd = SLP_FLAG;

                        if (GetDhcpOfIp(ip))
                            text_cmd = text_cmd + "DHCP=On;";
                        else
                            text_cmd = text_cmd + "DHCP=Off;";

                        if (cbLanDebugLed->State == cbGrayed)
                            ;
                        else if (cbLanDebugLed->Checked)
                            text_cmd = text_cmd + "LED_Debug=On;";
                        else
                            text_cmd = text_cmd + "LED_Debug=Off;";

                        if (cbLanDebugOled->State == cbGrayed)
                            ;
                        else if (cbLanDebugOled->Checked)
                            text_cmd = text_cmd + "OLED_Debug=On;";
                        else
                            text_cmd = text_cmd + "OLED_Debug=Off;";

                        // 设置时间格式
                        DateSeparator = '/';
                        TimeSeparator = ':';
                        text_cmd = text_cmd + "APP_Time="+FormatDateTime("yyyy/mm/dd hh:nn:ss.zzz", Now());

                        udpSLPList[i]->Send("255.255.255.255", UDP_PORT_SLP_EX, text_cmd+D1608CMD_TAIL);
                    }
                }
                catch(...)
                {
                    AppendLog(__FUNC__);
                }
            }
        }
    }

    tmSLP->Enabled = true;

    lblKeepLiveCheck->Caption = "发出："+IntToStr(send_keeplive_count)+" 接收："+IntToStr(recv_keeplive_count)+" 差值："+IntToStr(send_keeplive_count-recv_keeplive_count)+" 断开链路次数:"+IntToStr(broken_count)+" 保活收到次数："+IntToStr(slp_count);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::InputVolumeChange(TObject *Sender)
{
    TAdvTrackBar* track = (TAdvTrackBar*)Sender;
    int value = track->Position;
    int dsp_num = track->Tag;

    if (input_level_edit[dsp_num-1] != NULL)
    {
        if (value == -720)
        {
            input_level_edit[dsp_num-1]->Text = "Off";
        }
        else
        {
            String x;
            input_level_edit[dsp_num-1]->Text = x.sprintf("%1.1f", value/10.0);
        }
    }

    if (dsp_num < 17)
    {
        String cmd_text = D1608CMD_FLAG;
        cmd_text = cmd_text+"input<"+IntToStr(dsp_num)+">.volume="+input_level_edit[dsp_num-1]->Text;
        if (input_level_edit[dsp_num-1]->Text != "Off")
            cmd_text = cmd_text + "dB";
        SendCmd(cmd_text+D1608CMD_TAIL);

        config_map.input_dsp[dsp_num-1].level_a = value;
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ToogleMute(TObject *Sender)
{
    TSpeedButton* btn = (TSpeedButton*)Sender;
    int dsp_num = btn->Tag;

    String cmd_text = D1608CMD_FLAG;
    cmd_text = cmd_text + "input<"+IntToStr(dsp_num)+">.mute="+(btn->Down?"on":"off");
    SendCmd(cmd_text+D1608CMD_TAIL);

    config_map.input_dsp[dsp_num-1].mute_switch = btn->Down;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ToogleNoise(TObject *Sender)
{
    TSpeedButton* btn = (TSpeedButton*)Sender;
    int dsp_num = btn->Tag;

    String cmd_text = D1608CMD_FLAG;
    cmd_text = cmd_text + "input<"+IntToStr(dsp_num)+">.noise="+(btn->Down?"on":"off");
    SendCmd(cmd_text+D1608CMD_TAIL);

    config_map.input_dsp[dsp_num-1].noise_switch = btn->Down;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ToogleInvert(TObject *Sender)
{
    TSpeedButton* btn = (TSpeedButton*)Sender;
    int dsp_num = btn->Tag;

    String cmd_text = D1608CMD_FLAG;
    cmd_text = cmd_text+"input<"+IntToStr(dsp_num)+">.invert="+(btn->Down?"on":"off");
    SendCmd(cmd_text+D1608CMD_TAIL);

    config_map.input_dsp[dsp_num-1].invert_switch = btn->Down;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ToogleDefault(TObject *Sender)
{
    if (last_default_btn)
    {
        last_default_btn->Down = false;
    }

    TSpeedButton* btn = (TSpeedButton*)Sender;
    if (btn->Down)
    {
        last_default_btn = btn;
    }
    else
    {
        // default 缺少取消所有 default
        last_default_btn = NULL;
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ToogleAuto(TObject *Sender)
{
#if 0

#endif
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ToogleEQ(TObject *Sender)
{
    TSpeedButton* btn = (TSpeedButton*)Sender;
    int dsp_num = btn->Tag;

    String cmd_text = D1608CMD_FLAG;
    cmd_text = cmd_text+"input<"+IntToStr(dsp_num)+">.eq="+(btn->Down?"on":"off");
    SendCmd(cmd_text+D1608CMD_TAIL);

    config_map.input_dsp[dsp_num-1].eq_switch = btn->Down;

    input_thumb[dsp_num-1]->Invalidate();
}
//---------------------------------------------------------------------------
String IntOrZeroSring(int value)
{
    return String((value>0?value:0));
}

void TForm1::CachePackageMessageFeedback(char * data)
{
    ReplyMsgList reply_msg_list = {0};
    for (int i=0;i<REPLY_TEXT_MSG_SIZE;i++)
    {
        memcpy(&reply_msg_list.reply[i].msg_id, data, 4);
        data += 4;
        // 找到 ]
        char * right = strstr(data, "]");
        if (right != NULL)
        {
            reply_msg_list.count++;
            memcpy(&reply_msg_list.reply[i].text_cmd, data, right-data+1);
            data = right + 1;
        }
        else
        {
            break;
        }
    }

    reply_msg_buf.push_back(reply_msg_list);

    //unsigned int oldest_msg_id = reply_msg_list.reply[0].msg_id;
    unsigned int current_msg_id = reply_msg_list.reply[0].msg_id;

    for (int i=0;i<REPLY_TEXT_MSG_SIZE;i++)
    {
        //oldest_msg_id = min(oldest_msg_id, reply_msg_list.reply[i].msg_id);
        current_msg_id = max(current_msg_id, reply_msg_list.reply[i].msg_id);
    }

    pre_received_msg_id = current_msg_id;
}
void TForm1::ProcessPackageMessageFeedback(ReplyMsg text_syn_msg[REPLY_TEXT_MSG_SIZE], int reply_msg_count, std::vector<UINT> & cmd_id_list)
{
    unsigned int oldest_msg_id = text_syn_msg[0].msg_id;
    unsigned int current_msg_id = text_syn_msg[0].msg_id;
    unsigned int enabled_oldest_msg_id = INT_MAX;

    for (int i=0;i<REPLY_TEXT_MSG_SIZE;i++)
    {
        oldest_msg_id = min(oldest_msg_id, text_syn_msg[i].msg_id);
        current_msg_id = max(current_msg_id, text_syn_msg[i].msg_id);

        if (String("[]") != text_syn_msg[i].text_cmd)
            enabled_oldest_msg_id = min(enabled_oldest_msg_id, text_syn_msg[i].msg_id);
    }
    // 失联判断
    if (received_cmd_seq < oldest_msg_id && (oldest_msg_id-received_cmd_seq>1))
    {
        AppendLog(GetTime()+"失联: local="+IntToStr(received_cmd_seq)+", device="+IntToStr(oldest_msg_id)+"-"+IntToStr(current_msg_id));
        keep_live_count = CONTROL_TIMEOUT_COUNT+1;
    }
    else if (received_cmd_seq >= current_msg_id)
    {
        AppendLog(GetTime()+"syn do not need: device_msg_id="+IntToStr(current_msg_id)+", pc__msg_id="+IntToStr(received_cmd_seq));
    }
    else
    {
        AppendLog(GetTime()
            +"同步: local="+IntToStr(received_cmd_seq)+", " + IntToStr(reply_msg_count)
            + ": device=" +IntToStr(oldest_msg_id)+"-"+IntToStr(enabled_oldest_msg_id)+"-"+IntToStr(current_msg_id));

        for (int i=0;i<REPLY_TEXT_MSG_SIZE;i++)
        {
            if (text_syn_msg[i].msg_id > received_cmd_seq)
            {
                if (String("[]") == text_syn_msg[i].text_cmd)
                {
                    // 失联
                    AppendLog(GetTime()+"失联: local="+IntToStr(received_cmd_seq)+", device_empty_msg="+IntToStr(text_syn_msg[i].msg_id));
                    keep_live_count = CONTROL_TIMEOUT_COUNT+3;
                }

                // 防止自己响应
                if (sendcmd_list.size() > 0)
                {
                    TPackage package = sendcmd_list.back();
                    String package_cmd = (char*)package.data;
                    // replay已经删除 'NJLS_SMC|xxxx|' , package_cmd也需要删除
                        int h1_pos = String(package_cmd).Pos("|");
                        package_cmd = package_cmd.SubString(h1_pos+1, package_cmd.Length());
                        int h2_pos = String(package_cmd).Pos("|");
                        package_cmd = package_cmd.SubString(h2_pos+1, package_cmd.Length());
                        package_cmd = "[" + package_cmd;

                    if (GetTextCmdVar(text_syn_msg[i].text_cmd) == GetTextCmdVar(package_cmd))
                        continue;
                }

                // 命令转数据
                // 更新
                String bare_cmd = text_syn_msg[i].text_cmd;
                bare_cmd = bare_cmd.SubString(2, bare_cmd.Length());
                String full_cmd = String("[NJLS_SMC|parameter|") + bare_cmd;

                if (DelayProcessTextCommand(full_cmd, cmd_id_list) == -1)
                {
                    // 失联
                    AppendLog("收到未知报文: " + full_cmd);
                    keep_live_count = CONTROL_TIMEOUT_COUNT+2;
                }
            }
        }
    }

    received_cmd_seq = max(received_cmd_seq, current_msg_id);

    if (keep_live_count<CONTROL_TIMEOUT_COUNT)
        keep_live_count = 0;
}
bool IsReplyCmd(char * str)
{
    if (strncmp(str, D1608CMD_REPLY_FLAG, strlen(D1608CMD_REPLY_FLAG)) == 0)
		return true;
    return false;
}
bool IsKeepliveCmd(char * str)
{
    if (strncmp(str, D1608CMD_KEEPLIVE_FLAG, strlen(D1608CMD_KEEPLIVE_FLAG)) == 0)
		return true;
    return false;
}
#if 0
bool IsValidTextCmd(char * str, int udp_len)
{
	// 判断头部
	if (strncmp(str, D1608CMD_FLAG, strlen(D1608CMD_FLAG)) == 0)
		return true;
	if (strncmp(str, D1608CMD_CONTROL_FLAG, strlen(D1608CMD_CONTROL_FLAG)) == 0)
		return true;

    return true;
}
#endif
char * TextCmdPtr(char * str)
{
	// 判断头部
	if (strncmp(str, D1608CMD_FLAG, strlen(D1608CMD_FLAG)) == 0)
		return str+strlen(D1608CMD_FLAG);
	if (strncmp(str, D1608CMD_CONTROL_FLAG, strlen(D1608CMD_CONTROL_FLAG)) == 0)
		return str+strlen(D1608CMD_CONTROL_FLAG);
	if (strncmp(str, D1608CMD_KEEPLIVE_FLAG, strlen(D1608CMD_KEEPLIVE_FLAG)) == 0)
		return str+strlen(D1608CMD_KEEPLIVE_FLAG);

	return str;
}
void __fastcall TForm1::udpControlUDPRead(TObject *Sender, TStream *AData,
      TIdSocketHandle *ABinding)
{
    if (ABinding->PeerPort == UDP_GET_ERROR_LOG)//UDP_PORT_GET_DEBUG_INFO_EX
    {
        AlarmInfo alarm_info;
        AData->ReadBuffer(&alarm_info, std::min(sizeof(alarm_info), AData->Size));

        for (int i=0;i<100&&alarm_info.event[i].timer!=0;i++)
        {
            memo_debug_ex->Lines->Add( Event2Sring(alarm_info.event[i]) );
        }

        //memo_debug_ex->Lines->Add();
    }
    else if (ABinding->PeerPort == UDP_PORT_GET_DISPLAY_BUFFER)
    {
        AData->ReadBuffer(display_buffer, std::min(sizeof(display_buffer), AData->Size));
        pbOLED->Refresh();
    }    
    else if (ABinding->PeerPort == UDP_PORT_CONTROL)
    {
        char cmd_text[1500] = {0};
        AData->ReadBuffer(cmd_text, std::min(sizeof(cmd_text), AData->Size));

        if (IsReplyCmd(cmd_text))
        {
            if (reply_msg_buf.empty())
            {
                //AppendLog(GetTime()+": 第一个UDP包");
            }
            CachePackageMessageFeedback(cmd_text+strlen(D1608CMD_REPLY_FLAG));
            // 没有初始化完毕，不处理数据回报
            if (received_cmd_seq != 0)
            {
                // 给自己发消息处理
                tmProcessReply->Enabled = true;
            }
        }
        else if (IsKeepliveCmd(cmd_text) && AData->Size==(strlen(D1608CMD_KEEPLIVE_FLAG)+sizeof(NotStorageCmd)))
        {
            //AppendLog(GetTime()+": begin keeplive");

            static int last_diff = 0;
            recv_keeplive_count++;
            if (send_keeplive_count-recv_keeplive_count != last_diff)
            {
                AppendLog(GetTime()+"保活序号差值变化:"+IntToStr(last_diff)+" -> "+IntToStr(send_keeplive_count-recv_keeplive_count));
                last_diff = send_keeplive_count-recv_keeplive_count;
            }

            if (keep_live_count<CONTROL_TIMEOUT_COUNT)
                keep_live_count = 0;

            NotStorageCmd *keep_alive = (NotStorageCmd*)(cmd_text+strlen(D1608CMD_KEEPLIVE_FLAG));
            if (received_cmd_seq < (UINT)keep_alive->noop)    // noop保存了cmd_seq 原来是 !=
            {
                AppendLog(GetTime()+"同步消息中的序号: local="+IntToStr(received_cmd_seq)+", device="+IntToStr(keep_alive->noop));
                received_cmd_seq = (UINT)keep_alive->noop;
                tmProcessReply->Enabled = true;
            }

            TTime tmp;
            String start_str;
            double diff;
            LockWindowUpdate(this->Handle);

                tmp = Now();
                start_str = GetTime();
                ProcessWatchLevel(keep_alive->watch_level, keep_alive->watch_level_comp);
                diff = (Now()-tmp); diff = diff*24*3600*1000;
                //AppendLog(start_str+":      begin ProcessWatchLevel: "+IntToStr((int)diff) );

                tmp = Now();
                start_str = GetTime();
                ProcessVote(keep_alive->adc_ex, keep_alive->adc_ex_max, keep_alive->adc_ex_min);
                diff = (Now()-tmp); diff = diff*24*3600*1000;
                //AppendLog(start_str+":      begin ProcessVote:       "+IntToStr((int)diff) );

                tmp = Now();
                start_str = GetTime();
                ProcessKeepAlive(keep_alive->switch_preset & 0x7F, keep_alive->switch_preset & 0x80, keep_alive->set_time_ex);
                diff = (Now()-tmp); diff = diff*24*3600*1000;
                //AppendLog(start_str+":      begin ProcessKeepAlive:  "+IntToStr((int)diff) );

            LockWindowUpdate(NULL);

            //AppendLog(GetTime()+": end keeplive");
        }
        else
        {
            String cmd_string = TextCmdPtr(cmd_text);

            if (!cmd_string.AnsiCompareIC("config.action=disconnect]"))
            {
                AppendLog("config.action=disconnect");
                keep_live_count = CONTROL_TIMEOUT_COUNT;
                received_cmd_seq = 0;    pre_received_msg_id = 0;
            }

            // config.admin 与 parameter.admin 高度相似
            else if (!cmd_string.SubString(1,15).AnsiCompareIC("config.admin=ok"))
            {
                admin_password_ok = true;
                UpdateParameterEnabled();
                pnlSystem->Show();
                pnlSystem->BringToFront();
            }
            else if (!cmd_string.SubString(1,18).AnsiCompareIC("config.admin=error"))
            {
                PostMessage(btnConfigPasswordError->Handle, WM_LBUTTONDOWN, MK_LBUTTON, NULL);
                PostMessage(btnConfigPasswordError->Handle, WM_LBUTTONUP, MK_LBUTTON, NULL);
            }

            else if (!cmd_string.SubString(1,18).AnsiCompareIC("parameter.admin=ok"))
            {
                admin_password_ok = true;
                UpdateParameterEnabled();
            }
            else if (!cmd_string.SubString(1,21).AnsiCompareIC("parameter.admin=error"))
            {
                PostMessage(btnParameterPasswordError->Handle, WM_LBUTTONDOWN, MK_LBUTTON, NULL);
                PostMessage(btnParameterPasswordError->Handle, WM_LBUTTONUP, MK_LBUTTON, NULL);
            }

            else if (!cmd_string.AnsiCompareIC("config.action=reboot]")
                  || !cmd_string.AnsiCompareIC("config.action=init]")
                  || !cmd_string.AnsiCompareIC("config.action=clear_preset]"))
            {
                AppendLog(GetTime()+cmd_string);

                String cmd_text = D1608CMD_CONTROL_FLAG;
                cmd_text = cmd_text+   cmd_string.SubString(1, cmd_string.Length()-1)   +"_confirm";
                SendCmd2(cmd_text+D1608CMD_TAIL);

                AppendLog("config.action=reboot");
                keep_live_count = CONTROL_TIMEOUT_COUNT;
                received_cmd_seq = 0;          pre_received_msg_id = 0;
                {
                    sendcmd_list.empty();
                    shape_live->Hide();
                    shape_link->Hide();
                    shape_power->Hide();
                    memset(display_buffer, 0, sizeof(display_buffer));
                    // Level Meter归零
                    for (int i=0;i<32;i++)
                    {
                        UpdateWatchLevel(i, -49);
                    }
                    device_connected = false;
                }
            }
            else if (!(cmd_string.SubString(1, 12).AnsiCompareIC("config.lock="))/*与locl_key有冲突*/
                  || !(cmd_string.SubString(1, 13).AnsiCompareIC("config.unlock"))
                  || !cmd_string.AnsiCompareIC("config.action=Reload_Link]"))
            {
                // 重新获取数据
                TPackage package = {0};
                package.udp_port = UDP_PORT_READ_PRESET;

                D1608PresetCmd preset_cmd(version);
                strcpy(preset_cmd.flag, D1608PRESETCMD_LINK_FLAG);
                preset_cmd.preset = 0; // 读取global_config
                preset_cmd.store_page = 0;
                preset_cmd.lock_flag = 0;
                //preset_cmd.verify -= UdpPackageVerifyDiff((unsigned char*)&preset_cmd, offsetof(D1608PresetCmd, data));
                memcpy(package.data, &preset_cmd, offsetof(D1608PresetCmd, data)/*sizeof(preset_cmd)*/);
                package.data_size = offsetof(D1608PresetCmd, data)/*sizeof(preset_cmd)*/;
                read_one_preset_package_list.insert(read_one_preset_package_list.begin(), package);

                if (read_one_preset_package_list.size() == 1)
                    SendBuffer(dst_ip, package.udp_port, package.data, package.data_size);
            }

            if (!ProcessSendTextCmdAck(cmd_text, AData, ABinding))
            {
                //memcpy(((char*)(&config_map))+cmd.id, (char*)&cmd.data, cmd.length);
                //OnFeedbackData(cmd.id);
            }
        }
    }
    else if (ABinding->PeerPort == UDP_PORT_READ_LOG)
    {
        LogBuff buff = {LOG_FLAG};

        if (AData->Size == sizeof(buff)-8 || AData->Size ==sizeof(buff))
        {
            AData->ReadBuffer(&buff, AData->Size);

            if ((buff.address >= LOG_START_PAGE) && (buff.address < MAC_LIST_START_PAGE))
            {
                int index = (buff.address - LOG_START_PAGE) / sizeof(Event);
                memcpy(event_data_tmp+index, buff.data.event, sizeof(buff.data.event));
            }
            else
            {
                if (buff.address == MAC_LIST_START_PAGE)
                {
                    tail_address = buff.tail_address;
                    ProcessLogData();
                    mac_count = 0;
                }

                ProcessMACLog(buff);

                if (buff.address == MAC_LIST_START_PAGE+1024)
                {
                    btnGetLog->Enabled = true;
                    lvLog->Items->Count = log_count + mac_count;

                    lblLogCount->Caption = "日志数量："+IntToStr(log_count) + "   MAC数量："+IntToStr(mac_count);
                }
            }
        }
        else
        {
            // TODO: 出错了
        }

        if (!ProcessLogBuffAck(buff, AData, ABinding))
        {
            AppendLog(GetTime()+"Reply Buff");
        }
    }
    else if (ABinding->PeerPort == UDP_PORT_GET_DEBUG_INFO)
    {
#pragma pack(1)
        typedef struct
        {
            char desc[10];
            short log_level;
            unsigned __int64 timer;
        }LogData;
#pragma pack()
        LogData log_data[50];
        AData->ReadBuffer(&log_data, sizeof(log_data));
        for (int i=0;i<50;i++)
        {
            if (log_data[i].timer != 0)
            //if (String(log_data[i].desc) != "")
            {
                TListItem * item = lvDebug->Items->Insert(0);

                int ms = log_data[i].timer % 1000;  log_data[i].timer /= 1000;
                int sec = log_data[i].timer % 60; log_data[i].timer /= 60;
                int min = log_data[i].timer % 60; log_data[i].timer /= 60;

                item->Caption = IntToStr((__int64)log_data[i].timer)+":"
                              + IntToStr(min)+":"
                              + IntToStr(sec)+"."
                              + IntToStr(ms);
                item->SubItems->Add(log_data[i].desc);
                item->SubItems->Add(log_data[i].log_level);
            }
        }
    }
    else if (ABinding->PeerPort == UDP_PORT_READ_PRESET)
    {
        D1608PresetCmd preset_cmd(version);
        AData->ReadBuffer(&preset_cmd, std::min(sizeof(preset_cmd), AData->Size));

        // 校验一下
        TPackage package = read_one_preset_package_list.back();
        if (package.udp_port != ABinding->PeerPort)
            return;
        if (package.data_size > AData->Size)
            return;

        D1608PresetCmd * send_preset_cmd = (D1608PresetCmd*)package.data;
        if (send_preset_cmd->preset != preset_cmd.preset || send_preset_cmd->store_page != preset_cmd.store_page)
            return;

        int preset_id = preset_cmd.preset;
        if (preset_id == 0xFF)
            preset_id = cur_preset_id;

        // 保存
        if ((preset_id & 0x7F) == 0 && preset_cmd.store_page == 1)
        {
            memcpy(&device_setting, preset_cmd.data, sizeof(device_setting));
            device_setting.cpu_id[0] = device_setting.cpu_id[0] ^ 0x4E4A4C53;
			device_setting.cpu_id[1] = device_setting.cpu_id[1] ^ 0x5F534D43;
			device_setting.cpu_id[2] = device_setting.cpu_id[2] ^ 0x4E4A4C53;

        }
        else if ((preset_id & 0x7F) == 0 && preset_cmd.store_page == 0)
        {
            // global_config
            memcpy(&global_config, preset_cmd.data, sizeof(global_config));

            global_config_loaded = true;

            UpdateParameterEnabled();

            UpdateCaption();

            UpdateDeviceType();

            UpdateBuildTime();

            //edtBuildTime->Text = global_config.build_time;
            edtDeviceName->Text = global_config.d1616_name;
            //TDateTime d = edtBuildTime->Text;

            btnMonitor->Visible = GetVersionConfig().is_vote_check || is_inner_pc;

            int input_gain = GetVersionConfig().input_gain;
            iMIC->Visible = input_gain & INPUT_GAIN_MIC_VALUE;
            i10dBv->Visible = input_gain & INPUT_GAIN_10dBv_VALUE;
            i22dBu->Visible = input_gain & INPUT_GAIN_22dBu_VALUE;
            i24dBu->Visible = input_gain & INPUT_GAIN_24dBu_VALUE;

            int output_gain = GetVersionConfig().output_gain;
            o10dBv->Visible = output_gain & OUTPUT_GAIN_10dBv_VALUE;
            o22dBu->Visible = output_gain & OUTPUT_GAIN_22dBu_VALUE;
            o24dBu->Visible = output_gain & OUTPUT_GAIN_24dBu_VALUE;

            lblPresetFileName->Caption = global_config.import_filename;

            // 显示preset名称
            clbAvaliablePreset->Clear();
            clbAvaliablePreset->Items->Add(global_config.preset_name[0][0] ? global_config.preset_name[0] : "PRESET1");
            clbAvaliablePreset->Items->Add(global_config.preset_name[1][0] ? global_config.preset_name[1] : "PRESET2");
            clbAvaliablePreset->Items->Add(global_config.preset_name[2][0] ? global_config.preset_name[2] : "PRESET3");
            clbAvaliablePreset->Items->Add(global_config.preset_name[3][0] ? global_config.preset_name[3] : "PRESET4");
            clbAvaliablePreset->Items->Add(global_config.preset_name[4][0] ? global_config.preset_name[4] : "PRESET5");
            clbAvaliablePreset->Items->Add(global_config.preset_name[5][0] ? global_config.preset_name[5] : "PRESET6");
            clbAvaliablePreset->Items->Add(global_config.preset_name[6][0] ? global_config.preset_name[6] : "PRESET7");
            clbAvaliablePreset->Items->Add(global_config.preset_name[7][0] ? global_config.preset_name[7] : "PRESET8");

            {
                int dsp_920_num = 0;
                TPanel* iDsp[8] = {iDsp1, iDsp2, iDsp3, iDsp4, iDsp5, iDsp6, iDsp7, iDsp8};
                for (int i=0;i<8;i++) // xDO: 暂时只考虑8片
                {
                    dsp_920_num += device_setting.yss920[i];
                    iDsp[i]->Caption = (device_setting.yss920[i]?"920":"X");
                }

                TPanel* iAdDa[8] = {iAD_abcd, iAD_efgh, iAD_ijkl, iAD_mnop, iDA_1234, iDA_5678, iDA_9_12, iDA_13_16};
                for (int i=0;i<8;i++)
                    iAdDa[i]->Caption = device_setting.ad_da_card[i];

                iLed->Caption = device_setting.led_control_count;

                REAL_INPUT_DSP_NUM = GetVersionConfig().input_channel_count;
                REAL_OUTPUT_DSP_NUM = GetVersionConfig().output_channel_count;

                SetIOChannelNum();
            }
            cbGlobalDspName->Checked = ((global_config.is_global_name == 1) || (global_config.is_global_name == 0xFF));
            if (global_config.version == GLOBAL_CONFIG_VERSION)
            {
                // 读取'自动保存'配置
                cbPresetAutoSaved->Checked = ((global_config.auto_saved == 1) || (global_config.auto_saved == 0xFF));
                cbLedTest->Checked = (global_config.led_test == 1);
                cbLockParameter->Checked = (global_config.lock_parameter == 1);

                cbUsart1ReceiveAck->Checked = (global_config.usart1_receive_other_ack == 1);
                cbUsart3ReceiveAck->Checked = (global_config.usart3_receive_other_ack == 1);
            }
        }
        else
        {
            ConfigMap * dest_config_map = &all_config_map[preset_id-1];

            switch(preset_cmd.store_page)
            {
            case 0:
            case 1:
            case 2:
            case 3:
                {
                    const OlaList * input_dict = (const OlaList *)preset_cmd.data;
                    char * input_data_base = (char*)input_dict +
                            sizeof(input_dict->length) +
                            sizeof(input_dict->struct_length) +
                            input_dict->length * sizeof(OlaInfo);
                    for (int i=0;i<4;i++)
                    {
                        ReadIODspMem((char*)(dest_config_map->input_dsp + i + preset_cmd.store_page*4),
                            input_data_base + i*input_dict->struct_length,
                            (OlaList*)&input_dsp_ola_list,
                            input_dict);
                    }
                    break;
                }
            case 4:
            case 5:
            case 6:
            case 7:
                {
                    const OlaList * output_dict = (const OlaList *)preset_cmd.data;
                    char * output_data_base = (char*)output_dict +
                            sizeof(output_dict->length) +
                            sizeof(output_dict->struct_length) +
                            output_dict->length * sizeof(OlaInfo);
                    for (int i=0;i<4;i++)
                    {
                        ReadIODspMem((char*)(dest_config_map->output_dsp + i + (preset_cmd.store_page-4)*4),
                            output_data_base + i*output_dict->struct_length,
                            (OlaList*)&output_dsp_ola_list,
                            output_dict);
                    }
                    break;
                }
            }
        }

        read_one_preset_package_list.pop_back();

        if (read_one_preset_package_list.empty())
        {
            if (preset_id == cur_preset_id ||  preset_id==0) // GlobalConfig可能在最后
            {
                config_map = all_config_map[cur_preset_id-1];

                tmDelayUpdateUI->Enabled = false;
                ApplyConfigToUI();
                CloseDspDetail();
                AppendLog(GetTime()+"同步完成");

                lblVersion->Caption = VersionToStr(last_connection.data.version)+ " " +VersionToStr(version)+ " " + DateTime2Str(GetDateTimeFromMarco(compile_time));;
                edtDeviceFullName->Text = edtDeviceFullName_data;
                tmWatch->Enabled = true;
            }
            else
            {
                // 存盘
                TFileStream * file = new TFileStream(save_single_preset_to_file_filename, fmCreate);
                if (!file)
                {
                    ShowMessage("打开文件失败");
                    return;
                }

                //file->WriteBuffer(&config_map, sizeof(config_map));
                char save_buffer[8*1024];
                SavePresetById(1, 0, &all_config_map[preset_id-1].input_dsp[0], save_buffer);
                SavePresetById(1, 1, &all_config_map[preset_id-1].input_dsp[8], save_buffer);
                SavePresetById(1, 2, &all_config_map[preset_id-1].output_dsp[0], save_buffer);
                SavePresetById(1, 3, &all_config_map[preset_id-1].output_dsp[8], save_buffer);
                file->WriteBuffer(&save_buffer, sizeof(save_buffer));

                delete file;
            }
        }
        else
        {
            package = read_one_preset_package_list.back();
            SendBuffer(dst_ip, package.udp_port, package.data, package.data_size);

            restor_delay_count = restor_delay_check_count * 3;
            tmDelayBackup->Enabled = true;
        }
    }
    else if (ABinding->PeerPort == UDP_PORT_STORE_PRESET_PC2FLASH)
    {
        D1608PresetCmd preset_cmd(version);
        AData->ReadBuffer(&preset_cmd, std::min(sizeof(preset_cmd), AData->Size));

        // 校验一下
        TPackage package = package_list.back();
        if (package.udp_port != ABinding->PeerPort)
            return;
        if (AData->Size < offsetof(D1608PresetCmd, data))
            return;
        if (memcmp(package.data, &preset_cmd, AData->Size) != 0)
            return;

        package_list.pop_back();

        if (package_list.empty())
        {
            pbBackup->Position = pbBackup->Max;
            pbBackup->Hide();
            this->Enabled = true;

            if (package.action == ACT_DISCONNECT)
            {
                // 失联
                AppendLog("package_list.empty()");
                keep_live_count = CONTROL_TIMEOUT_COUNT;
            }
            else if (package.action == ACT_RELOAD_PRESET)
            {
            }
        }
        else
        {
            package = package_list.back();
            SendBuffer(dst_ip, package.udp_port, package.data, package.data_size);

            pbBackup->Position = pbBackup->Max - package_list.size();
            restor_delay_count = restor_delay_check_count * 3;
            tmDelayBackup->Enabled = true;
        }
    }
    else if (ABinding->PeerPort == UDP_PORT_READ_FLASH)
    {
        FlashRW_Data flash_rw;
        AData->ReadBuffer(&flash_rw, std::min(sizeof(flash_rw), AData->Size));

        // 校验一下
        TPackage package = package_list.back();
        if (package.udp_port != ABinding->PeerPort)
            return;
        if (package.data_size > AData->Size)
            return;

        FlashRW_Data * send_flash_rw_cmd = (FlashRW_Data*)package.data;
        if (send_flash_rw_cmd->start_address != flash_rw.start_address)
            return;

        int offset = flash_rw.start_address-PRESET_START_PAGE;
        memcpy(smc_config.device_flash_dump+offset, flash_rw.data, sizeof(flash_rw.data));

        package_list.pop_back();

        if (package_list.empty())
        {
            pbBackup->Position = pbBackup->Max;
            pbBackup->Hide();
            this->Enabled = true;

            // 完成，写入文件
            TFileStream * file = new TFileStream(save_device_to_file_filename, fmCreate);
            if (!file)
            {
                ShowMessage("打开文件失败");
                return;
            }

            LoadGlobalConfig(smc_config.global_config, smc_config.device_flash_dump);
#if 0
            // 转换成普通的8个preset数据
            for (int i=1;i<=8;i++)
            {
                ConfigMap config_map;
                LoadPresetById(i, config_map, smc_config.device_flash_dump);
                smc_config.all_config_map[i-1] = config_map;
            }
#endif
            file->WriteBuffer(&smc_config, sizeof(smc_config));

            pbBackup->Position = pbBackup->Max;
            pbBackup->Hide();
            this->Enabled = true;
            delete file;

            // 失联
            AppendLog("package_list.empty2()");
            keep_live_count = CONTROL_TIMEOUT_COUNT;
        }
        else
        {
            package = package_list.back();
            SendBuffer(dst_ip, package.udp_port, package.data, package.data_size);

            pbBackup->Position = pbBackup->Max - package_list.size();
            restor_delay_count = restor_delay_check_count * 3;
            tmDelayBackup->Enabled = true;
        }
    }
}
//---------------------------------------------------------------------------
static ADC_Data_Ex AdjustAdcDataByBootAdcDataEx(ADC_Data_Ex true_data, ADC_Data boot_adc_data)
{
    ADC_Data_Ex result = true_data;

	result._8vdc   = true_data._8vdc * boot_adc_data._8vad / boot_adc_data._8vdc;
	result._8vac   = true_data._8vac * boot_adc_data._8vad / boot_adc_data._8vac;
	result._x16vac = true_data._x16vac * boot_adc_data._x16va / boot_adc_data._x16vac;
	result._50vpc   = true_data._50vpc * boot_adc_data._50vp / boot_adc_data._50vpc;
	result._16vac  = true_data._16vac * boot_adc_data._16va / boot_adc_data._16vac;

    return result;
}

static void UpdateAdc_ValueListEditor(TStrings * vls, ADC_Data_Ex data)
{
    // 用上电电压校准
    String _;
    vls->Strings[0 ] = vls->Strings[0 ] + "\t" + _.sprintf("%10.5f", data._3_3vd  / 1000.0f);
    vls->Strings[1 ] = vls->Strings[1 ] + "\t" + _.sprintf("%10.5f", data.base    / 1000.0f);
    vls->Strings[2 ] = vls->Strings[2 ] + "\t" + _.sprintf("%10.5f", data._5vd    / 1000.0f);
    vls->Strings[3 ] = vls->Strings[3 ] + "\t" + _.sprintf("%10.5f", data._8vdc   / 1000.0f);
    vls->Strings[4 ] = vls->Strings[4 ] + "\t" + _.sprintf("%10.5f", data._8vac   / 1000.0f);
    vls->Strings[5 ] = vls->Strings[5 ] + "\t" + _.sprintf("%10.5f", data._8vad   / 1000.0f);
    vls->Strings[6 ] = vls->Strings[6 ] + "\t" + _.sprintf("%10.5f", data._x16vac / 1000.0f);
    vls->Strings[7 ] = vls->Strings[7 ] + "\t" + _.sprintf("%10.5f", data._x16va  / 1000.0f);
    vls->Strings[8 ] = vls->Strings[8 ] + "\t" + _.sprintf("%10.5f", data._50vpc  / 1000.0f);
    vls->Strings[9 ] = vls->Strings[9 ] + "\t" + _.sprintf("%10.5f", data._50vp   / 1000.0f);
    vls->Strings[10] = vls->Strings[10] + "\t" + _.sprintf("%10.5f", data._48vp   / 1000.0f);
    vls->Strings[11] = vls->Strings[11] + "\t" + _.sprintf("%10.5f", data._5va    / 1000.0f);
    vls->Strings[12] = vls->Strings[12] + "\t" + _.sprintf("%10.5f", data._x12va  / 1000.0f);
    vls->Strings[13] = vls->Strings[13] + "\t" + _.sprintf("%10.5f", data._12va   / 1000.0f);
    vls->Strings[14] = vls->Strings[14] + "\t" + _.sprintf("%10.5f", data._16va   / 1000.0f);
    vls->Strings[15] = vls->Strings[15] + "\t" + _.sprintf("%10.5f", data._16vac  / 1000.0f);

    vls->Strings[17] = vls->Strings[17] + "\t" + _.sprintf("%10.5f", data._8va_current);
    vls->Strings[18] = vls->Strings[18] + "\t" + _.sprintf("%10.5f", data._8vd_current);
    vls->Strings[19] = vls->Strings[19] + "\t" + _.sprintf("%10.5f", data._16v_current);
    vls->Strings[20] = vls->Strings[20] + "\t" + _.sprintf("%10.5f", data._x16v_current);
    vls->Strings[21] = vls->Strings[21] + "\t" + _.sprintf("%10.5f", data._50v_current);
#if 0
    // 用上电电压校准
    vls->Cells[1][0 +1] = String::FormatFloat("0.00000 ", data._3_3vd  / 1000.0f);
    vls->Cells[1][1 +1] = String::FormatFloat("0.00000 ", data.base    / 1000.0f);
    vls->Cells[1][2 +1] = String::FormatFloat("0.00000 ", data._5vd    / 1000.0f);
    vls->Cells[1][3 +1] = String::FormatFloat("0.00000 ", data._8vdc   / 1000.0f);
    vls->Cells[1][4 +1] = String::FormatFloat("0.00000 ", data._8vac   / 1000.0f);
    vls->Cells[1][5 +1] = String::FormatFloat("0.00000 ", data._8vad   / 1000.0f);
    vls->Cells[1][6 +1] = String::FormatFloat("0.00000 ", data._x16vac / 1000.0f);
    vls->Cells[1][7 +1] = String::FormatFloat("0.00000 ", data._x16va  / 1000.0f);
    vls->Cells[1][8 +1] = String::FormatFloat("0.00000 ", data._50vpc  / 1000.0f);
    vls->Cells[1][9 +1] = String::FormatFloat("0.00000 ", data._50vp   / 1000.0f);
    vls->Cells[1][10+1] = String::FormatFloat("0.00000 ", data._48vp   / 1000.0f);
    vls->Cells[1][11+1] = String::FormatFloat("0.00000 ", data._5va    / 1000.0f);
    vls->Cells[1][12+1] = String::FormatFloat("0.00000 ", data._x12va  / 1000.0f);
    vls->Cells[1][13+1] = String::FormatFloat("0.00000 ", data._12va   / 1000.0f);
    vls->Cells[1][14+1] = String::FormatFloat("0.00000 ", data._16va   / 1000.0f);
    vls->Cells[1][15+1] = String::FormatFloat("0.00000 ", data._16vac  / 1000.0f);

    vls->Cells[1][18] = String::FormatFloat("0.00000 ", data._8va_current);
    vls->Cells[1][19] = String::FormatFloat("0.00000 ", data._8vd_current);
    vls->Cells[1][20] = String::FormatFloat("0.00000 ", data._16v_current);
    vls->Cells[1][21] = String::FormatFloat("0.00000 ", data._x16v_current);
    vls->Cells[1][22] = String::FormatFloat("0.00000 ", data._50v_current);
#endif
}
void TForm1::ProcessVote(ADC_Data_Ex adc_ex, ADC_Data_Ex adc_ex_max, ADC_Data_Ex adc_ex_min)
{
    this->adc_ex_max = adc_ex_max;
    this->adc_ex_min = adc_ex_min;

    //-----------------------------
    TStrings * vote_list = new TStringList;
    vote_list->AddStrings(mmVoteOrg->Lines);

    //if (pnlMist->Visible)
    {
        UpdateAdc_ValueListEditor(vote_list, adc_ex);
        UpdateAdc_ValueListEditor(vote_list, adc_ex_max);
        UpdateAdc_ValueListEditor(vote_list, adc_ex_min);
    }

    mmVote->Clear();
    mmVote->Lines->Add("        \t   当前  \t   max   \t    min");
    mmVote->Lines->Add("----------------------------------------------------------");
    mmVote->Lines->AddStrings(vote_list);
    delete vote_list;


    lblDiff->Caption = adc_ex._8vdc-adc_ex._8vad;

    //====================================================================
    lbl3_3V->Caption = String::FormatFloat("0.00 ", adc_ex.base / 1000.0);
    lbl3_3Vd->Caption = String::FormatFloat("0.00 ", adc_ex._3_3vd / 1000.0);
    lbl5Va->Caption = String::FormatFloat("0.00 ", adc_ex._5va / 1000.0);
    lbl5Vd->Caption = String::FormatFloat("0.00 ", adc_ex._5vd / 1000.0);
    lbl8Vac->Caption = String::FormatFloat("0.00 ", adc_ex._8vac / 1000.0);
    lbl8Vdc->Caption = String::FormatFloat("0.00 ", adc_ex._8vdc / 1000.0);
    lbl12Va->Caption = String::FormatFloat("0.00 ", adc_ex._12va / 1000.0);
    lbl_12Va->Caption = String::FormatFloat("0.00 ", adc_ex._x12va / 1000.0);
    lbl16Vac->Caption = String::FormatFloat("0.00 ", adc_ex._16vac / 1000.0);
    lbl_16Vac->Caption = String::FormatFloat("0.00 ", adc_ex._x16vac / 1000.0);
    lbl48Vp->Caption = String::FormatFloat("0.00 ", adc_ex._48vp / 1000.0);

    //====================================================================
    lbl3_3mA->Caption = IntOrZeroSring((int)(adc_ex._8vd_current * 0.1)) + " ";   //8Vd * 0.10
    lbl3_3mAd->Caption = IntOrZeroSring((int)(adc_ex._8vd_current * 0.85)) + " "; //8Vd * 0.85
    lbl5mAa->Caption = IntOrZeroSring((int)(adc_ex._8va_current)) + " ";          // 8Va
    lbl5mAd->Caption = IntOrZeroSring((int)(adc_ex._8vd_current * 0.05)) + " ";   // 8Vd * 0.05
    lbl8mAa->Caption = IntOrZeroSring((int)(adc_ex._8va_current)) + " ";
    lbl8mAd->Caption = IntOrZeroSring((int)(adc_ex._8vd_current)) + " ";
    lbl12mAa->Caption = IntOrZeroSring(adc_ex._16v_current) + " ";               // 16Va
    lbl_12mAa->Caption = IntOrZeroSring(adc_ex._x16v_current) + " ";            // -16Va
    lbl16mAa->Caption = IntOrZeroSring(adc_ex._16v_current) + " ";
    lbl_16mAa->Caption = IntOrZeroSring(adc_ex._x16v_current) + " ";
    lbl46mAa->Caption = IntOrZeroSring(adc_ex._50v_current) + " ";

    //====================================================================
    // 补充到曲线图
    if (active_adc != NULL)
    {
        try {
            double value = active_adc->Caption.ToDouble();

            Series1->Add(value, "", clLime);

            lineUpLimit->Add(up_line_value, "e", clRed);
            lineDownLimit->Add(down_line_value, "b", clRed);

            Chart1->BottomAxis->Scroll(1, false);
        }
        catch(...)
        {
            AppendLog(__FUNC__);
        }
    }
}
//---------------------------------------------------------------------------
bool TForm1::ProcessSendTextCmdAck(String cmd_text, TStream *AData, TIdSocketHandle *ABinding)
{
    if (sendcmd_list.size() == 0)
        return false;

    TPackage package = sendcmd_list.back();

    if (package.udp_port != ABinding->PeerPort)
        return false;

    char* package_text_cmd = package.data;

    /*if (GetTextCmdVar(cmd_text) == GetTextCmdVar(package_text_cmd))
    {
    }
    else
    {
        return false;
    }*/
    if (cmd_text != package_text_cmd)
        return false;
    
    sendcmd_list.pop_back();
    if (sendcmd_list.size() > 0)
    {
        package = sendcmd_list.back();
        String cmd = (char*)package.data;
        AppendLog(GetTime()+" 继续下一个消息:"+cmd);

        SendBuffer(dst_ip, package.udp_port, package.data, package.data_size);
        sendcmd_delay_count = delay_send_cmd_check_count * 3;
    }

    return true;
}
void TForm1::ProcessKeepAlive(int preset_id, bool need_reload, unsigned __int64 timer)
{
    if (preset_id<1 || preset_id>8)
    {
        // 收到的数据包异常
        return;
    }

    if (preset_id != cur_preset_id || need_reload)
    {
        SetPresetId(preset_id);
        StartReadCurrentPreset(false);
        restor_delay_count = restor_delay_check_count * 3;
        tmDelayBackup->Enabled = true;
    }

    // 显示时钟
    unsigned __int64 running_time;
    if (timer > global_config.adjust_running_time)
        running_time = timer - global_config.adjust_running_time;
    else
        running_time = timer - global_config.adjust_running_time + MAX_TIMER_MS;

    running_time = running_time / 1000; // 转换单位为秒

    int hour = running_time / 3600;
    int minute = running_time % 3600;
    minute = minute / 60;
    lblDeviceRunningTime->Caption = lblDeviceRunningTime->Caption.sprintf("%d:%02d", hour, minute);

    // 显示未调整的时钟
    running_time = timer / 1000;    // 单位：秒
    hour = running_time / 3600;
    minute = running_time % 3600;
    minute = minute / 60;
    lblDeviceRunningTime2->Caption = lblDeviceRunningTime2->Caption.sprintf("%d:%02d", hour, minute);

    if (cbRunningTimer->Checked)
    {
        int remain_time = global_config.running_timer_limit - running_time;
        if (remain_time > 0)
        {
            edtRemainTime->Text = FormatFloat("0.00", remain_time/3600.0);
        }
        else
        {
            edtRemainTime->Text = "0";
        }
    }
    else
    {
        edtRemainTime->Text = "--";
    }

    if (cbRebootCount->Checked)
    {
        int remain_count = global_config.reboot_count_limit - global_config.reboot_count;
        if (remain_count > 0)
        {
            edtRemainRebootCount->Text = remain_count;
        }
        else
        {
            edtRemainRebootCount->Text = "0";
        }
    }
    else
    {
        edtRemainRebootCount->Text = "--";
    }
    edtLockedString1->Text = global_config.locked_string;

    if (keep_live_count<CONTROL_TIMEOUT_COUNT)
        keep_live_count = 0;
    shape_live->Show();
    shape_link->Show();
    shape_power->Show();
    device_connected = true;
}

static String GetNameOfAdc(int value, int event_id)
{
    String adc_name[21] = {
        "3.3vd",
        "3.3vm",
        "5vd",
        "8vdc",
        "8vac",
        "8vad",
        "x16vac",
        "x16va",
        "50vpc",
        "50vp",
        "48vp",
        "5va",
        "x12va",
        "12va",
        "16va",
        "16vac",
        "8va_current",
        "8vd_current",
        "16v_current",
        "x16v_current",
        "50v_current"
    };

    int index = value & 0xFF;
    int percent = value >> 8;

    if (percent != 0)   // 老机器残留日志是 0，避免除以0的错误
    {
        if (index == 6 || index == 7 || index == 12) // 负电压和负电流
        {
            if (event_id == EVENT_VOTE_DOWN_OVERFLOW)
                percent = 10000 / percent;
        }
        else
        {
            if (event_id == EVENT_VOTE_UP_OVERFLOW)
                 percent = 10000 / percent;
        }
    }

    if (index < 21)
        return adc_name[index] + ":" + IntToStr(percent) + "%";
    else
        return IntToStr(index) + ":" + IntToStr(percent) + "%";
}
static void ApplyLogData(TListItem* item, Event event, int address, String syn_time)
{
    unsigned int event_timer = event.timer;
    item->Data = (void*)address;

    item->Caption = "0x"+IntToHex(address, 8);

    int ms = event_timer % 10;  event_timer /= 10;
    int sec = event_timer % 60; event_timer /= 60;
    int min = event_timer % 60; event_timer /= 60;

    String item_caption;
    item_caption.sprintf("_%d:%02d:%02d.%d", event_timer, min, sec, ms);
    item->SubItems->Add(item_caption);

    switch (event.event_id)
    {
    case EVENT_POWER_OFF:
        item->SubItems->Add("关闭电源");
        item->SubItems->Add("下次启动次数"+IntToStr(event.event_data));
        break;
    case EVENT_SYSTEM_LIMIT:
        item->SubItems->Add("达到运行次数或时间限制");
        item->SubItems->Add(event.event_data);
        break;
    case EVENT_SAVE_PRESET:
        item->SubItems->Add("保存Preset");

        {
            String page_indexs = "Preset编号:"+IntToStr(((event.event_data>>8) & 0x7F));
            if ((event.event_data & 0x7F) == 0)
            {
                item->SubItems->Add("不需要存盘");
            }
            else
            {
                page_indexs = page_indexs + " 存盘页:";
                if (event.event_data & 1) page_indexs = page_indexs + "1,";
                if (event.event_data & 2) page_indexs = page_indexs + "2,";
                if (event.event_data & 4) page_indexs = page_indexs + "3,";
                if (event.event_data & 8) page_indexs = page_indexs + "4,";
                if (event.event_data & 16) page_indexs = page_indexs + "5,";
                item->SubItems->Add(page_indexs);
            }
        }
        break;
    case EVENT_POWER_SAVE_OK:
        item->SubItems->Add("关机存盘成功");
        item->SubItems->Add("下次启动次数"+IntToStr(event.event_data));
        break;
    case EVENT_INPUT_OVERFLOW:
        item->SubItems->Add("input通道音量满过载警告");
        item->SubItems->Add("通道号"+IntToStr(event.event_data));
        break;
    case EVENT_OUTPUT_OVERFLOW:
        item->SubItems->Add("output通道音量满过载警告");
        item->SubItems->Add("通道号"+IntToStr(event.event_data));
        break;
    case EVENT_WRITE_FLASH_ERROR:
        item->SubItems->Add("写flash错误");
        item->SubItems->Add("地址:0x"+IntToHex(event.event_data * 2048, 8));
        break;
    case EVENT_REBOOT:
        if (event.event_data < 0x10)
        {
            item->SubItems->Add("上位机发起重启");
            if (event.event_data == 0)
            {
                item->SubItems->Add("重启");
            }
            else if (event.event_data == 1)
            {
                item->SubItems->Add("清除PRESER");
            }
            else if (event.event_data == 2)
            {
                item->SubItems->Add("恢复出厂设置");
            }
            else if (event.event_data == 10)
            {
                item->SubItems->Add("进入升级程序");
            }
            else
            {
                item->SubItems->Add(event.event_data);
            }
        }
        else
        {
            item->SubItems->Add("上电时清除");
            if (event.event_data == 0x011)
            {
                item->SubItems->Add("清除PRESER");
            }
            else if (event.event_data == 0x12)
            {
                item->SubItems->Add("恢复出厂设置");
            }
            else
            {
                item->SubItems->Add(event.event_data);
            }
        }
        break;
    case EVENT_SAVE_PRESET_OK:
        item->SubItems->Add("存盘完成");
        {
            String page_indexs = "存盘页";
            if (event.event_data & 1) page_indexs = page_indexs + "1,";
            if (event.event_data & 2) page_indexs = page_indexs + "2,";
            if (event.event_data & 4) page_indexs = page_indexs + "3,";
            if (event.event_data & 8) page_indexs = page_indexs + "4,";
            if (event.event_data & 16) page_indexs = page_indexs + "5,";
            item->SubItems->Add(page_indexs);
        }
        break;
    case EVENT_CHECK_MD5_FAIL:
        item->SubItems->Add("激活码错误");
        item->SubItems->Add("");
        break;
    case EVENT_28J60_REINIT_ERROR:
        item->SubItems->Add("ENC28J60初始化警告");
        item->SubItems->Add("失败次数:"+IntToStr(event.event_data));
        break;
    case EVENT_MAC_ADDRESS_OVERFLOW:
        item->SubItems->Add("MAC地址日志溢出");
        item->SubItems->Add("");
        break;
    case EVENT_NO_KEY:
        item->SubItems->Add("设备授权错误");
        item->SubItems->Add("");
        break;
    case EVENT_DSP_NOT_MATCH_ERROR:
        item->SubItems->Add("YSS920与配置不符错误");
        if (event.event_data > 0x80)
            item->SubItems->Add("缺少左数第"+IntToStr(event.event_data-0x80));
        else
            item->SubItems->Add("多出左数第"+IntToStr(event.event_data));
        break;
    case EVENT_LED_NUM_ERR:
        item->SubItems->Add("LED控制芯片错误");
        item->SubItems->Add(event.event_data);
        break;
    case EVENT_FILENAME_CHANGED:
        item->SubItems->Add("导入/导出配置");
        if (event.event_data == 1)
            item->SubItems->Add("Load From File");
        else
            item->SubItems->Add("Save To File");
        break;
    case EVENT_SET_MAC_ADDRESS:
        item->SubItems->Add("设置了MAC地址");
        item->SubItems->Add(IntToHex(event.event_data, 6));
        break;
    case EVENT_ADDA_ERROR:
        // xxxx xrr rrrtc cccc
        // r表示应有数，t表示da还是ad，c表示当前检测到的数
        if (event.event_data & 32)
            item->SubItems->Add("DA数量与配置不符错误");
        else
            item->SubItems->Add("AD数量与配置不符错误");
        // 计算数量
        {
            int test_count = event.event_data & 0x001F;
            int config_count = (event.event_data>>6) & 0x001F;
            item->SubItems->Add("现有数量："+IntToStr(test_count) + ",应有数量："+IntToStr(config_count));
        }
        break;
    case EVENT_EACH_HOUR:
        item->SubItems->Add("开机每小时标识");
        item->SubItems->Add(IntToStr(event.event_data));
        break;
    case EVENT_TIME_1:
        item->SubItems->Add("时间同步信息1");
        item->SubItems->Add("0x"+IntToHex(event.event_data, 4));
        break;
    case EVENT_TIME_2:
        item->SubItems->Add("时间同步信息2");
        item->SubItems->Add("0x"+IntToHex(event.event_data, 4));
        break;
    case EVENT_TIME_3:
        item->SubItems->Add("时间同步信息3");
        item->SubItems->Add("0x"+IntToHex(event.event_data, 4));
        break;
    case EVENT_TIME_4:
        item->SubItems->Add("时间同步信息4");
        item->SubItems->Add("0x"+IntToHex(event.event_data, 4));
        break;
    case EVENT_SAVE_LOAD_TIMEOUT:
        item->SubItems->Add("存盘或者恢复超时错误");
        item->SubItems->Add(event.event_data==1?"LOAD FROM FILE":"SAVE TO FILE");
        break;
    //case EVENT_48V:
    //    item->SubItems->Add("48V与硬件不匹配");
    //    item->SubItems->Add(event.event_data==0?"缺少":"多出");
    //    break;
    case EVENT_RESET_RUNNING_TIME:
        item->SubItems->Add("重置运行时间警告");
        item->SubItems->Add("");
        break;
    case EVENT_IWDG_REBOOT:
        item->SubItems->Add("上次运行期间看门狗异常错误");
        item->SubItems->Add("");
        break;
    case EVENT_POWER_SAVE_ERROR:
        item->SubItems->Add("掉电存盘错误");
        item->SubItems->Add(IntToStr(event.event_data));
        break;
    case EVENT_28J60_VERSION_ERROR:
        {
            item->SubItems->Add("28J60版本号错误");
            item->SubItems->Add(IntToStr((event.event_data>>8)&0xFF)+" -> " + IntToStr(event.event_data&0xFF));
        }
        break;
    case EVENT_UPGRADE_DATE:
        item->SubItems->Add("升级时版本日期");
        item->SubItems->Add(IntToHex(event.event_data, 4));
        break;
    case EVENT_UPGRADE_TIME:
        item->SubItems->Add("升级时版本时间");
        item->SubItems->Add(IntToHex(event.event_data, 4));
        break;
    case EVENT_MCU_CLK_ERR:
        if (event.event_data == 0)
        {
            item->SubItems->Add("时钟校准");
            item->SubItems->Add("正常");
        }
        else if (event.event_data == 1)
        {
            item->SubItems->Add("时钟校准异常警告");
            item->SubItems->Add("设备时钟快5%以上");
        }
        else if (event.event_data == 2)
        {
            item->SubItems->Add("时钟校准异常警告");
            item->SubItems->Add("设备时钟慢5%以上");
        }
        else
        {
            item->SubItems->Add("时钟校准异常数据");
            item->SubItems->Add(IntToHex(event.event_data, 2));
        }
        break;
    case EVENT_VOTE_UP_OVERFLOW:
        item->SubItems->Add("电压/电流超过上限错误");
        item->SubItems->Add(GetNameOfAdc(event.event_data, EVENT_VOTE_UP_OVERFLOW));
        break;
    case EVENT_VOTE_DOWN_OVERFLOW:
        item->SubItems->Add("电压/电流低于下限错误");
        item->SubItems->Add(GetNameOfAdc(event.event_data, EVENT_VOTE_DOWN_OVERFLOW));
        break;
    case EVENT_MAX_JOB_TIME:
        item->SubItems->Add("任务处理时间过长");
        item->SubItems->Add(IntToStr(event.event_data));
        break;
    case EVENT_ERR_JOB_FUNC_PTR:
        item->SubItems->Add("任务队列出现空指针错误");
        item->SubItems->Add("");
        break;
    case EVENT_ERR_WRITE_LOG:
        item->SubItems->Add("写日志失败错误");
        item->SubItems->Add(IntToStr(event.event_data));
        break;
    case EVENT_ERR_28J60_SENDDATA:
        item->SubItems->Add("28J60发出报文出错");
        item->SubItems->Add(IntToStr(event.event_data));
        break;
    case EVENT_ERR_EXTERN_RAM:
        item->SubItems->Add("RAM检测错误");
        item->SubItems->Add(IntToHex(event.event_data, 4));
        break;
    case EVENT_POWER_ON:
        item->SubItems->Add("开启电源");
        item->SubItems->Add("本次启动次数"+IntToStr(event.event_data));
        break;
    case EVENT_STACK_OVERFLOW:
        item->SubItems->Add("栈溢出错误");
        item->SubItems->Add(IntToStr(event.event_data));
        break;
    case EVENT_SAVE_LOAD_COUNT:
        item->SubItems->Add("存盘或者恢复数量");
        item->SubItems->Add(event.event_data);
        break;
    case EVENT_SPI_REENTER:
        item->SubItems->Add("SPI重入错误");
        item->SubItems->Add(event.event_data);
        break;
    case EVENT_EQ_COEF_ERR:
        item->SubItems->Add("EQ系数错误");
        item->SubItems->Add(event.event_data);
        break;
    case EVENT_POWER_SAVE_OK_TIME:
        item->SubItems->Add("关机存盘成功时间");
        item->SubItems->Add(event.event_data);
        break;
    case EVENT_SWITCH_ERROR:
        item->SubItems->Add("开关错误");
        {
            switch (event.event_data)
            {
            case SE_12VA:
                item->SubItems->Add("+12va开关");
                break;
            case SE_X12VA:
                item->SubItems->Add("-12va开关, 请先检测+12vd开关");
                break;
            case SE_5VA:
                item->SubItems->Add("5va开关");
                break;
            case SE_48VP:
                item->SubItems->Add("48v开关");
                break;
            case SE_3_3VD:
                item->SubItems->Add("3.3vd开关");
                break;
            case SE_5VD:
                item->SubItems->Add("5vd开关");
                break;
            }
        }
        break;
    default:
        item->SubItems->Add(event.event_id);
        item->SubItems->Add(IntToStr(event.event_data) + "(0x"+IntToHex(event.event_data, 2)+")");
        break;
    }

    item->SubItems->Add(syn_time);

    // 记录原始数据
    item->SubItems->Add(IntToHex((int)event.timer, 8) + IntToHex(event.event_id, 4) + IntToHex(event.event_data, 4));
}
static String Event2Sring(Event event)
{
    unsigned int event_timer = event.timer;

    int ms = event_timer % 10;  event_timer /= 10;
    int sec = event_timer % 60; event_timer /= 60;
    int min = event_timer % 60; event_timer /= 60;

    String item_caption;
    item_caption.sprintf("_%d:%02d:%02d.%d", event_timer, min, sec, ms);

    switch (event.event_id)
    {
    case EVENT_POWER_OFF:
        item_caption = item_caption + " " +("关闭电源");
        item_caption = item_caption + " " +("下次启动次数"+IntToStr(event.event_data));
        break;
    case EVENT_SYSTEM_LIMIT:
        item_caption = item_caption + " " +("达到运行次数或时间限制");
        item_caption = item_caption + " " +(event.event_data);
        break;
    case EVENT_SAVE_PRESET:
        item_caption = item_caption + " " +("保存Preset");

        {
            String page_indexs = "Preset编号:"+IntToStr(((event.event_data>>8) & 0x7F));
            if ((event.event_data & 0x7F) == 0)
            {
                item_caption = item_caption + " " +("不需要存盘");
            }
            else
            {
                page_indexs = page_indexs + " 存盘页:";
                if (event.event_data & 1) page_indexs = page_indexs + "1,";
                if (event.event_data & 2) page_indexs = page_indexs + "2,";
                if (event.event_data & 4) page_indexs = page_indexs + "3,";
                if (event.event_data & 8) page_indexs = page_indexs + "4,";
                if (event.event_data & 16) page_indexs = page_indexs + "5,";
                item_caption = item_caption + " " +(page_indexs);
            }
        }
        break;
    case EVENT_POWER_SAVE_OK:
        item_caption = item_caption + " " +("关机存盘成功");
        item_caption = item_caption + " " +("下次启动次数"+IntToStr(event.event_data));
        break;
    case EVENT_INPUT_OVERFLOW:
        item_caption = item_caption + " " +("input通道音量满过载警告");
        item_caption = item_caption + " " +("通道号"+IntToStr(event.event_data));
        break;
    case EVENT_OUTPUT_OVERFLOW:
        item_caption = item_caption + " " +("output通道音量满过载警告");
        item_caption = item_caption + " " +("通道号"+IntToStr(event.event_data));
        break;
    case EVENT_WRITE_FLASH_ERROR:
        item_caption = item_caption + " " +("写flash错误");
        item_caption = item_caption + " " +("地址:0x"+IntToHex(event.event_data * 2048, 8));
        break;
    case EVENT_REBOOT:
        if (event.event_data < 0x10)
        {
            item_caption = item_caption + " " +("上位机发起重启");
            if (event.event_data == 0)
            {
                item_caption = item_caption + " " +("重启");
            }
            else if (event.event_data == 1)
            {
                item_caption = item_caption + " " +("清除PRESER");
            }
            else if (event.event_data == 2)
            {
                item_caption = item_caption + " " +("恢复出厂设置");
            }
            else if (event.event_data == 10)
            {
                item_caption = item_caption + " " +("进入升级程序");
            }
            else
            {
                item_caption = item_caption + " " +(event.event_data);
            }
        }
        else
        {
            item_caption = item_caption + " " +("上电时清除");
            if (event.event_data == 0x011)
            {
                item_caption = item_caption + " " +("清除PRESER");
            }
            else if (event.event_data == 0x12)
            {
                item_caption = item_caption + " " +("恢复出厂设置");
            }
            else
            {
                item_caption = item_caption + " " +(event.event_data);
            }
        }
        break;
    case EVENT_SAVE_PRESET_OK:
        item_caption = item_caption + " " +("存盘完成");
        {
            String page_indexs = "存盘页";
            if (event.event_data & 1) page_indexs = page_indexs + "1,";
            if (event.event_data & 2) page_indexs = page_indexs + "2,";
            if (event.event_data & 4) page_indexs = page_indexs + "3,";
            if (event.event_data & 8) page_indexs = page_indexs + "4,";
            if (event.event_data & 16) page_indexs = page_indexs + "5,";
            item_caption = item_caption + " " +(page_indexs);
        }
        break;
    case EVENT_CHECK_MD5_FAIL:
        item_caption = item_caption + " " +("激活码错误");
        item_caption = item_caption + " " +("");
        break;
    case EVENT_28J60_REINIT_ERROR:
        item_caption = item_caption + " " +("ENC28J60初始化警告");
        item_caption = item_caption + " " +("失败次数:"+IntToStr(event.event_data));
        break;
    case EVENT_MAC_ADDRESS_OVERFLOW:
        item_caption = item_caption + " " +("MAC地址日志溢出");
        item_caption = item_caption + " " +("");
        break;
    case EVENT_NO_KEY:
        item_caption = item_caption + " " +("设备授权错误");
        item_caption = item_caption + " " +("");
        break;
    case EVENT_DSP_NOT_MATCH_ERROR:
        item_caption = item_caption + " " +("YSS920与配置不符错误");
        if (event.event_data > 0x80)
            item_caption = item_caption + " " +("缺少:"+IntToStr(event.event_data-0x80));
        else
            item_caption = item_caption + " " +("多出:"+IntToStr(event.event_data));
        break;
    case EVENT_LED_NUM_ERR:
        item_caption = item_caption + " " +("LED控制芯片错误");
        item_caption = item_caption + " " +(event.event_data);
        break;
    case EVENT_FILENAME_CHANGED:
        item_caption = item_caption + " " +("导入/导出配置");
        if (event.event_data == 1)
            item_caption = item_caption + " " +("Load From File");
        else
            item_caption = item_caption + " " +("Save To File");
        break;
    case EVENT_SET_MAC_ADDRESS:
        item_caption = item_caption + " " +("设置了MAC地址");
        item_caption = item_caption + " " +(IntToHex(event.event_data, 6));
        break;
    case EVENT_ADDA_ERROR:
        if (event.event_data > 32)
            item_caption = item_caption + " " +("AD数量与配置不符错误");
        else
            item_caption = item_caption + " " +("DA数量与配置不符错误");
        item_caption = item_caption + " " +("现有数量："+IntToStr(event.event_data%32));
        break;
    case EVENT_EACH_HOUR:
        item_caption = item_caption + " " +("开机每小时标识");
        item_caption = item_caption + " " +(IntToStr(event.event_data));
        break;
    case EVENT_TIME_1:
        item_caption = item_caption + " " +("时间同步信息1");
        item_caption = item_caption + " " +("0x"+IntToHex(event.event_data, 4));
        break;
    case EVENT_TIME_2:
        item_caption = item_caption + " " +("时间同步信息2");
        item_caption = item_caption + " " +("0x"+IntToHex(event.event_data, 4));
        break;
    case EVENT_TIME_3:
        item_caption = item_caption + " " +("时间同步信息3");
        item_caption = item_caption + " " +("0x"+IntToHex(event.event_data, 4));
        break;
    case EVENT_TIME_4:
        item_caption = item_caption + " " +("时间同步信息4");
        item_caption = item_caption + " " +("0x"+IntToHex(event.event_data, 4));
        break;
    case EVENT_SAVE_LOAD_TIMEOUT:
        item_caption = item_caption + " " +("存盘或者恢复超时错误");
        item_caption = item_caption + " " +(event.event_data==1?"LOAD FROM FILE":"SAVE TO FILE");
        break;
    //case EVENT_48V:
    //    item_caption = item_caption + " " +("48V与硬件不匹配");
    //    item_caption = item_caption + " " +(event.event_data==0?"缺少":"多出");
    //    break;
    case EVENT_RESET_RUNNING_TIME:
        item_caption = item_caption + " " +("重置运行时间警告");
        item_caption = item_caption + " " +("");
        break;
    case EVENT_IWDG_REBOOT:
        item_caption = item_caption + " " +("上次运行期间看门狗异常错误");
        item_caption = item_caption + " " +("");
        break;
    case EVENT_POWER_SAVE_ERROR:
        item_caption = item_caption + " " +("掉电存盘错误");
        item_caption = item_caption + " " +(IntToStr(event.event_data));
        break;
    case EVENT_28J60_VERSION_ERROR:
        {
            item_caption = item_caption + " " +("28J60版本号错误");
            item_caption = item_caption + " " +(IntToStr((event.event_data>>8)&0xFF)+" -> " + IntToStr(event.event_data&0xFF));
        }
        break;
    case EVENT_UPGRADE_DATE:
        item_caption = item_caption + " " +("升级时版本日期");
        item_caption = item_caption + " " +(IntToHex(event.event_data, 4));
        break;
    case EVENT_UPGRADE_TIME:
        item_caption = item_caption + " " +("升级时版本时间");
        item_caption = item_caption + " " +(IntToHex(event.event_data, 4));
        break;
    case EVENT_MCU_CLK_ERR:
        if (event.event_data == 0)
        {
            item_caption = item_caption + " " +("时钟校准");
            item_caption = item_caption + " " +("正常");
        }
        else if (event.event_data == 1)
        {
            item_caption = item_caption + " " +("时钟校准异常警告");
            item_caption = item_caption + " " +("设备时钟快5%以上");
        }
        else if (event.event_data == 2)
        {
            item_caption = item_caption + " " +("时钟校准异常警告");
            item_caption = item_caption + " " +("设备时钟慢5%以上");
        }
        else
        {
            item_caption = item_caption + " " +("时钟校准异常数据");
            item_caption = item_caption + " " +(IntToHex(event.event_data, 2));
        }
        break;
    case EVENT_VOTE_UP_OVERFLOW:
        item_caption = item_caption + " " +("电压/电流超过上限错误");
        item_caption = item_caption + " " +(GetNameOfAdc(event.event_data, EVENT_VOTE_UP_OVERFLOW));
        break;
    case EVENT_VOTE_DOWN_OVERFLOW:
        item_caption = item_caption + " " +("电压/电流低于下限错误");
        item_caption = item_caption + " " +(GetNameOfAdc(event.event_data, EVENT_VOTE_DOWN_OVERFLOW));
        break;
    case EVENT_MAX_JOB_TIME:
        item_caption = item_caption + " " +("任务处理时间过长");
        item_caption = item_caption + " " +(IntToStr(event.event_data));
        break;
    case EVENT_ERR_JOB_FUNC_PTR:
        item_caption = item_caption + " " +("任务队列出现空指针错误");
        item_caption = item_caption + " " +("");
        break;
    case EVENT_ERR_WRITE_LOG:
        item_caption = item_caption + " " +("写日志失败错误");
        item_caption = item_caption + " " +(IntToStr(event.event_data));
        break;
    case EVENT_ERR_28J60_SENDDATA:
        item_caption = item_caption + " " +("28J60发出报文出错");
        item_caption = item_caption + " " +(IntToStr(event.event_data));
        break;
    case EVENT_ERR_EXTERN_RAM:
        item_caption = item_caption + " " +("RAM检测错误");
        item_caption = item_caption + " " +(IntToHex(event.event_data, 4));
        break;
    case EVENT_POWER_ON:
        item_caption = item_caption + " " +("开启电源");
        item_caption = item_caption + " " +("本次启动次数"+IntToStr(event.event_data));
        break;
    case EVENT_STACK_OVERFLOW:
        item_caption = item_caption + " " +("栈溢出错误");
        item_caption = item_caption + " " +(IntToStr(event.event_data));
        break;
    case EVENT_SAVE_LOAD_COUNT:
        item_caption = item_caption + " " +("存盘或者恢复数量");
        item_caption = item_caption + " " +(event.event_data);
        break;
    case EVENT_SPI_REENTER:
        item_caption = item_caption + " " +("SPI重入错误");
        item_caption = item_caption + " " +(event.event_data);
        break;
    case EVENT_EQ_COEF_ERR:
        item_caption = item_caption + " " +("EQ系数错误");
        item_caption = item_caption + " " +(event.event_data);
        break;
    case EVENT_POWER_SAVE_OK_TIME:
        item_caption = item_caption + " " +("关机存盘成功时间");
        item_caption = item_caption + " " +(event.event_data);
        break;
    default:
        item_caption = item_caption + " " +(event.event_id);
        item_caption = item_caption + " " +(IntToStr(event.event_data) + "(0x"+IntToHex(event.event_data, 2)+")");
        break;
    }

    return item_caption;
}
void TForm1::ProcessLogData()
{
    int log_tail_index = ((int)tail_address-LOG_START_PAGE) / sizeof(Event);
    // 移动日志，使得符合顺序，同时记录最后一条有效日志索引
    for (int i=0;i<EVENT_POOL_SIZE;i++)
    {
        // 第i条记录在原始数据中的位置
        int mapped_index = (log_tail_index-i-1+EVENT_POOL_SIZE) % EVENT_POOL_SIZE;
        event_data[i] = event_data_tmp[mapped_index];

        if (event_data[i].timer != 0xFFFFFFFF)
        {
            log_count = i+1;
        }

        // 同时清除同步时间
        event_syn_timer[i] = "";
    }

    unsigned __int64 time_base = 0;
    // 向下计算时间
    for (int i=0;i<log_count;i++)
    {
        // 计算同步时间
        switch (event_data[i].event_id)
        {
            case EVENT_TIME_1:
            {
                if (i>=3 &&
                    event_data[i-1].event_id == EVENT_TIME_2 &&
                    event_data[i-2].event_id == EVENT_TIME_3 &&
                    event_data[i-3].event_id == EVENT_TIME_4)
                {
                    // TODO: 这里做了屏蔽，可能隐藏了问题
                    time_base = 0;//event_data[i].event_data;
                    time_base = time_base << 16 | event_data[i-1].event_data;
                    time_base = time_base << 16 | event_data[i-2].event_data;
                    time_base = time_base << 16 | event_data[i-3].event_data;

                    if (time_base < 0x00FFFFFFFFFFFFFF)
                    {
                        // 记录时间
                        // 从2000-1-1为基准
                        double time_of_real = time_base + event_data[i].timer;
                        time_of_real = time_of_real / (24*3600*10);
                        TDateTime datetime_of_real = time_of_real;
                        datetime_of_real = datetime_of_real + TDateTime(2000, 1, 1);
                        event_syn_timer[i] = datetime_of_real;
                    }
                }
                break;
            }
        case EVENT_POWER_SAVE_OK:
        case EVENT_POWER_OFF:
        case EVENT_IWDG_REBOOT:
        case EVENT_TIME_2:
        case EVENT_TIME_3:
        case EVENT_TIME_4:
            time_base = 0;
            break;
        default:
            if (time_base > 0 && time_base < 0x00FFFFFFFFFFFFFF)
            {
                // 从2000-1-1为基准
                double time_of_real = time_base + event_data[i].timer;
                time_of_real = time_of_real / (24*3600*10);
                TDateTime datetime_of_real = time_of_real;
                datetime_of_real = datetime_of_real + TDateTime(2000, 1, 1);
                event_syn_timer[i] = datetime_of_real;
            }
            break;
        }

        if (event_data[i].event_id == EVENT_POWER_ON)
        {
            time_base = 0;
        }
    }

    // 向上计算时间
    for (int i=log_count-1;i>=0;i--)
    {
        // 计算同步时间
        switch (event_data[i].event_id)
        {
            case EVENT_TIME_1:
            {
                if (i>=3 &&
                    event_data[i-1].event_id == EVENT_TIME_2 &&
                    event_data[i-2].event_id == EVENT_TIME_3 &&
                    event_data[i-3].event_id == EVENT_TIME_4)
                {
                    time_base = 0;//event_data[i].event_data;
                    time_base = time_base << 16 | event_data[i-1].event_data;
                    time_base = time_base << 16 | event_data[i-2].event_data;
                    time_base = time_base << 16 | event_data[i-3].event_data;
                }
                break;
            }
        default:
            if (time_base > 0 && time_base < 0x00FFFFFFFFFFFFFF && event_syn_timer[i]=="")
            {
                // 从2000-1-1为基准
                double time_of_real = time_base + event_data[i].timer;
                time_of_real = time_of_real / (24*3600*10);
                TDateTime datetime_of_real = time_of_real;
                datetime_of_real = datetime_of_real + TDateTime(2000, 1, 1);
                event_syn_timer[i] = datetime_of_real;
            }
            if (event_data[i].event_id == EVENT_POWER_SAVE_OK || event_data[i].event_id == EVENT_POWER_OFF)
            {
                time_base = 0;
            }
            break;
        }
    }
}
bool TForm1::ProcessLogBuffAck(LogBuff& buff, TStream *AData, TIdSocketHandle *ABinding)
{
    TPackage package = sendcmd_list.back();
    if (package.udp_port != ABinding->PeerPort)
        return false;
    if (package.data_size > AData->Size)
        return false;

    LogBuff* package_buff = (LogBuff*)package.data;

    if (buff.address == package_buff->address)
    {
        //AppendLog(GetTime()+"消息匹配");
        sendcmd_list.pop_back();
        if (sendcmd_list.size() > 0)
        {
            package = sendcmd_list.back();
            SendBuffer(dst_ip, package.udp_port, package.data, package.data_size);
            sendcmd_delay_count = delay_send_cmd_check_count * 3;
        }
    }
    return true;
}
//---------------------------------------------------------------------------
void TForm1::ProcessWatchLevel(int watch_level[INPUT_DSP_NUM + OUTPUT_DSP_NUM], int watch_level_comp[OUTPUT_DSP_NUM])
{
    for (int i=0;i<INPUT_DSP_NUM+OUTPUT_DSP_NUM;i++)
    {
        int value = watch_level[i];
        if (value <= 0)
        {
            UpdateWatchLevel(i, -71);
        }
        else
        {
            try{
                double valuex = log10(value);
                double base = log10(0x00FFFFFF);

                if (i < 16) // Input
                {
                    UpdateWatchLevel(i, (valuex - base) * 20 + 1 + 24);
                }
                else // Output
                {
                    UpdateWatchLevel(i,
                        (valuex - base) * 20 + 1 + 24,
                        -100);

                    int comp_level = watch_level_comp[i-16];
                    if (comp_level == -100)
                    {
                        UpdateWatchLevel(i,
                            (valuex - base) * 20 + 1 + 24,
                            -100);
                    }
                    else if(comp_level <= 0)
                    {
                        UpdateWatchLevel(i,
                            (valuex - base) * 20 + 1 + 24,
                            -71);
                    }
                    else
                    {
                        double comp_valuex = log10(comp_level);
                        UpdateWatchLevel(i,
                            (valuex - base) * 20 + 1 + 24,
                            (comp_valuex - base) * 20 + 1 + 24);
                    }
                }

            }
            catch(...)
            {
                UpdateWatchLevel(i, 0);
                AppendLog(__FUNC__);
            }
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::tmWatchTimer(TObject *Sender)
{
    //AppendLog(GetTime()+"保活计数器事件: " + (udpControl->Active?"link":"unlink"));

    bool _disconnected;
    if (is_manual_disconnect)
    {
        _disconnected = true;
    }
    else
    {
        // stroe或者open状态
        if (pbBackup->Visible)
        {
            return;
        }

        // keep alive
        if ((keep_live_count < CONTROL_TIMEOUT_COUNT) && udpControl->Active)
        {
            keep_live_count++;
            _disconnected = false;
            if (keep_live_count >= CONTROL_TIMEOUT_COUNT)
                AppendLog(GetTime()+"保活计数器达到超时阈值: " + IntToStr(keep_live_count));
        }
        else
        {
            _disconnected = true;
        }
    }

    if (_disconnected)
    {
        if (udpControl->Active)
            CloseControlLink("保活超时");
        sendcmd_list.empty();
        shape_live->Hide();
        shape_link->Hide();
        shape_power->Hide();
        if (!is_manual_disconnect)
        {
            memset(display_buffer, 0, sizeof(display_buffer));
        }
        // Level Meter归零
        for (int i=0;i<32;i++)
        {
            UpdateWatchLevel(i, -49);
        }
        device_connected = false;
        received_cmd_seq = 0;       pre_received_msg_id = 0;

        ClearUI();
    }

    String cmd_text = D1608CMD_KEEPLIVE_FLAG;
    cmd_text = cmd_text+"config.action.keeplive="+IntToStr(received_cmd_seq);
    SendCmd2(cmd_text+D1608CMD_TAIL);
    send_keeplive_count++;
    last_keeplive_time = Now();

    // 检测resize事件
    if (need_resize)
    {
        SetIOChannelNum();
        need_resize = false;
        CloseDspDetail();
    }

    Button5->Click();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ToogleOutputMute(TObject *Sender)
{
    TSpeedButton* btn = (TSpeedButton*)Sender;
    int dsp_num = btn->Tag;

    String cmd_text = D1608CMD_FLAG;
    cmd_text = cmd_text+"output<"+IntToStr(dsp_num)+">.mute="+(btn->Down?"on":"off");
    SendCmd(cmd_text+D1608CMD_TAIL);

    config_map.output_dsp[dsp_num-1].mute_switch = btn->Down;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ToogleOutputInvert(TObject *Sender)
{
    TSpeedButton* btn = (TSpeedButton*)Sender;
    int dsp_num = btn->Tag;

    String cmd_text = D1608CMD_FLAG;
    cmd_text = cmd_text+"output<"+IntToStr(dsp_num)+">.invert="+(btn->Down?"on":"off");
    SendCmd(cmd_text+D1608CMD_TAIL);
    
    config_map.output_dsp[dsp_num-1].invert_switch = btn->Down;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::SpeedButton119Click(TObject *Sender)
{
    // DO1
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ToogleDO(TObject *Sender)
{
    // DO2
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ToogleOutputMix(TObject *Sender)
{
    if (last_dsp_btn != NULL)
    {
        last_out_num_btn = NULL;
        last_dsp_btn->OnClick(last_dsp_btn);
    }

    // 1 ~ 8
    if (last_out_num_btn)
    {
        last_out_num_btn->Down = false;
    }

    TSpeedButton* btn = (TSpeedButton*)Sender;
    if (btn->Down)
    {
        last_out_num_btn = btn;
        // 切换按钮颜色

        for (int i=0;i<REAL_INPUT_DSP_NUM;i++)   // xDO: 原来是17个，包括automix，现在只按照输入数量
        {
            TAdvTrackBar* trackbar = mix_level_trackbar[i];
            if (trackbar != NULL)
            {
                trackbar->Thumb->Picture = MixPicture[btn->Tag - 1];
                trackbar->Thumb->PictureDown = MixPicture[btn->Tag - 1];
                trackbar->Thumb->PictureHot = MixPicture[btn->Tag - 1];
            }
        }

        pnlMix->Left = input_panel_dsp_btn->Left;//output_panel_number_btn->Left;//Width - 30 - pnlMix->Width;

        pnlMix->Top = btn->Top + 21;// + btn->Height + 10;//312;
        pnlMix->Show();
        pnlMix->Tag = btn->Tag;

        // 数据
        int out_dsp_num = last_out_num_btn->Tag;

        for (int i=0;i<REAL_INPUT_DSP_NUM;i++)
        {
            mix_mute_btn[i]->Down = config_map.output_dsp[out_dsp_num-1].mix_mute[i];
            mix_level_trackbar[i]->Position = config_map.output_dsp[out_dsp_num-1].mix[i];
        }
    }
    else
    {
        last_out_num_btn = NULL;
        pnlMix->Hide();
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ToogleOutputEQ(TObject *Sender)
{
    TSpeedButton* btn = (TSpeedButton*)Sender;
    int dsp_num = btn->Tag;

    String cmd_text = D1608CMD_FLAG;
    cmd_text = cmd_text+"output<"+IntToStr(dsp_num)+">.eq="+(btn->Down?"on":"off");
    SendCmd(cmd_text+D1608CMD_TAIL);

    config_map.output_dsp[dsp_num-1].eq_switch = btn->Down;

    output_thumb[dsp_num-1]->Invalidate();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::OutputVolumeChange(TObject *Sender)
{
    TAdvTrackBar* track = (TAdvTrackBar*)Sender;
    int value = track->Position;
    int dsp_num = track->Tag;

    if (output_level_edit[dsp_num-1] != NULL)
    {
        if (value == -720)
        {
            output_level_edit[dsp_num-1]->Text = "Off";
        }
        else
        {
            String x;
            output_level_edit[dsp_num-1]->Text = x.sprintf("%1.1f", value/10.0);
        }
    }

    String cmd_text = D1608CMD_FLAG;
    cmd_text = cmd_text+ "output<"+IntToStr(dsp_num)+">.volume="+output_level_edit[dsp_num-1]->Text;
    if (output_level_edit[dsp_num-1]->Text != "Off")
        cmd_text = cmd_text + "dB";
    SendCmd(cmd_text+D1608CMD_TAIL);

    config_map.output_dsp[dsp_num-1].level_a = value;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::lvDeviceDblClick(TObject *Sender)
{
    TListItem * item = lvDevice->Selected;
    if (item != NULL)
    {
        btnSelect->Click();
    }
    lvDevice->Invalidate();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ToggleDSP(TObject *Sender)
{
    if (last_out_num_btn != NULL)
    {
        last_dsp_btn == NULL;
        last_out_num_btn->OnClick(last_out_num_btn);
    }

    // DSP button
    if (last_dsp_btn)
    {
        last_dsp_btn->Down = false;
    }

    TSpeedButton* btn = (TSpeedButton*)Sender;
    pnlDspDetail->Tag = btn->Tag;
    if (btn->Down)
    {
        last_dsp_btn = btn;

        if (btn->Tag < 100)
        {
            img_input_gain_trackbar->Show();
            img_output_gain_trackbar->Hide();
            dsp_gain_trackbar->Max = 240;
            dsp_gain_trackbar->Min = -300;
            lblDSPInfo->Caption = "Input Channel " + IntToStr(btn->Tag) + " DSP Setup";
            pnlDspDetail->Left = input_panel_bkground->Left;

            int dsp_num = btn->Tag;
            dsp_gain_trackbar->Position = config_map.input_dsp[dsp_num-1].level_b;
            dsp_delay_trackbar->Position = config_map.input_dsp[dsp_num-1].delay;
            if (GetVersionConfig().is_48v)
            {
                btnPhanton->Down = config_map.input_dsp[dsp_num-1].phantom_switch;
                btnPhanton->Show();
            }
            else
            {
                btnPhanton->Hide();
            }

            // 默认值
            edtCompAttackTime->Text = 64;
            edtCompReleaseTime->Text = 1000;

            // 调整PaintBox1的尺寸
            PaintBox1->Left = 8;
            PaintBox1->Width = 753;
            pbComp->Hide();

            // 隐藏COMP界面
            pnlComp->Enabled = false;
            pnlComp->Color = clGray;
            btnDspComp->Enabled = false;

            cbCompAutoTime->Checked = true;
            edtCompReleaseTime->Enabled = false;
            edtCompAttackTime->Enabled = false;
            edtCompAttackTime->Text = 64;
            edtCompReleaseTime->Text = 1000;


            btnDspEq->Down = config_map.input_dsp[dsp_num-1].eq_switch;
        }
        else
        {
            img_input_gain_trackbar->Hide();
            img_output_gain_trackbar->Show();
            dsp_gain_trackbar->Max = 120;
            dsp_gain_trackbar->Min = -300;
            lblDSPInfo->Caption = "Output Channel " + IntToStr(btn->Tag-100) + " DSP Setup";
            pnlDspDetail->Left = output_panel_bkground->Left + output_panel_bkground->Width - pnlDspDetail->Width;

            int dsp_num = btn->Tag-100;
            dsp_gain_trackbar->Position = config_map.output_dsp[dsp_num-1].level_b;
            dsp_delay_trackbar->Position = config_map.output_dsp[dsp_num-1].delay;
            btnPhanton->Hide();

            // 调整PaintBox1的尺寸
            if (GetVersionConfig().is_comp)
            {
                PaintBox1->Left = 216;
                PaintBox1->Width = 545;
                pbComp->Show();
            }
            else
            {
                PaintBox1->Left = 8;
                PaintBox1->Width = 753;
                pbComp->Hide();
            }

            // COMP
            AdjustOutputCompParam(dsp_num);
            btnDspComp->Down = config_map.output_dsp[dsp_num-1].comp_switch;
            edtCompRatio->Text = Ration2String(config_map.output_dsp[dsp_num-1].ratio);
            edtCompThreshold->Text = config_map.output_dsp[dsp_num-1].threshold/10.0;
            edtCompGain->Text = config_map.output_dsp[dsp_num-1].comp_gain/10.0;
            cbCompAutoTime->Checked = config_map.output_dsp[dsp_num-1].auto_time;
            if (cbCompAutoTime->Checked)
            {
                edtCompReleaseTime->Enabled = false;
                edtCompAttackTime->Enabled = false;
                edtCompAttackTime->Text = 64;
                edtCompReleaseTime->Text = 1000;
            }
            else
            {
                edtCompAttackTime->Text = config_map.output_dsp[dsp_num-1].attack_time/attack_config.scale;
                edtCompReleaseTime->Text = config_map.output_dsp[dsp_num-1].release_time/release_config.scale;
            }

            if (GetVersionConfig().is_comp)
            {
                pnlComp->Enabled = true;
                pnlComp->Color = TColor(0x0082DDE7);
                btnDspComp->Enabled = true;
            }
            else
            {
                pnlComp->Enabled = false;
                pnlComp->Color = clGray;
                btnDspComp->Enabled = false;
            }

            btnDspEq->Down = config_map.output_dsp[dsp_num-1].eq_switch;
        }

        dsp_gain_trackbar->OnChange(dsp_gain_trackbar);
        dsp_delay_trackbar->OnChange(dsp_delay_trackbar);

        pnlDspDetail->Top = 168;
        pnlDspDetail->Show();

        panel_agent->LoadPreset();
        PaintBox1->Refresh();
        pbComp->Refresh();
    }
    else
    {
        last_dsp_btn = NULL;
        pnlDspDetail->Hide();
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btnMixInvertClick(TObject *Sender)
{
#if 0
#endif
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btnMAXONClick(TObject *Sender)
{
#if 0

#endif
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btnLastOnClick(TObject *Sender)
{
#if 0
#endif
}
//---------------------------------------------------------------------------

void __fastcall TForm1::btnFShiftClick(TObject *Sender)
{
#if 0
#endif
}
//---------------------------------------------------------------------------
void __fastcall TForm1::MasterVolumeChange(TObject *Sender)
{
    TAdvTrackBar* track = (TAdvTrackBar*)Sender;
    int value = track->Position;

    AppendLog(GetTime()+"master推子: "+IntToStr(value)+" "+IntToStr(keep_live_count));

    if (master_panel_level_edit != NULL)
    {
        if (value == -720)
        {
            master_panel_level_edit->Text = "Off";
        }
        else
        {
            String x;
            master_panel_level_edit->Text = x.sprintf("%1.1f", value/10.0);
        }
    }

    String cmd_text = D1608CMD_FLAG;
    cmd_text = cmd_text+ "master.volume="+master_panel_level_edit->Text;
    if (master_panel_level_edit->Text != "Off")
        cmd_text = cmd_text + "dB";
    SendCmd(cmd_text+D1608CMD_TAIL);

    config_map.input_dsp[0].master_level_a = value;
    AppendLog(IntToStr(config_map.input_dsp[0].master_level_a));
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btnMixMuteClick(TObject *Sender)
{
#if 0
#endif
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btnMasterMuteClick(TObject *Sender)
{
    TSpeedButton* btn = (TSpeedButton*)Sender;

    String cmd_text = D1608CMD_FLAG;
    cmd_text = cmd_text+ String("master.mute=")+(btn->Down?"on":"off");
    SendCmd(cmd_text+D1608CMD_TAIL);

    config_map.input_dsp[0].master_mute_switch = btn->Down;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btnPhantonClick(TObject *Sender)
{
    TSpeedButton* btn = (TSpeedButton*)Sender;
    int dsp_num = btn->Parent->Tag;

    String cmd_text = D1608CMD_FLAG;
    cmd_text = cmd_text+ "input<"+IntToStr(dsp_num)+">.phantom="+(btn->Down?"on":"off");
    SendCmd(cmd_text+D1608CMD_TAIL);

    config_map.input_dsp[dsp_num-1].phantom_switch = btn->Down;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::FormMouseWheel(TObject *Sender, TShiftState Shift,
      int WheelDelta, TPoint &MousePos, bool &Handled)
{
    TEdit * edt = dynamic_cast<TEdit*>(ActiveControl);
    if (edt && edt!=edt4Tab)
    {
        TShiftState shift;
        WORD key;
        if (WheelDelta >0)
        {
            key = VK_UP;
        }
        else if (WheelDelta < 0)
        {
            key = VK_DOWN;
        }
        if (edt->OnKeyDown != NULL)
        {
            edt->OnKeyDown(edt, key, shift);
            Handled = true;
        }
        return;
    }

    paint_agent->OnMouseWheel(Sender, Shift, WheelDelta, MousePos, Handled);
    if (!Handled && ActiveControl)
    {
        panel_agent->OnMouseWheel(ActiveControl, Shift, WheelDelta, MousePos, Handled);
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::input_typeMouseDown(TObject *Sender,
      TMouseButton Button, TShiftState Shift, int X, int Y)
{
    if (Button == mbLeft)
    {
        TLabel * input_type = (TLabel*)Sender;
        TPoint p(input_type->Left, input_type->Top+input_type->Height);
        p = input_type->Parent->ClientToScreen(p);
        if (input_type->Tag <= 16)
        {
            PopupMenu1->PopupComponent = input_type;
            PopupMenu1->Popup(p.x, p.y);
        }
        else
        {
            PopupMenu2->PopupComponent = input_type;
            PopupMenu2->Popup(p.x, p.y);
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::i10dBvClick(TObject *Sender)
{
    // 输入菜单
    TLabel * popup_label = (TLabel*)PopupMenu1->PopupComponent;
    popup_label->Caption = ((TMenuItem*)Sender)->Caption;
    int dsp_num = popup_label->Tag;

    if (popup_label->Caption == "MIC")
    {
        config_map.input_dsp[dsp_num-1].gain = INPUT_GAIN_MIC;
    }
    else if (popup_label->Caption == "10dBv")
    {
        config_map.input_dsp[dsp_num-1].gain = INPUT_GAIN_10dBv;
    }
    else if (popup_label->Caption == "22dBu")
    {
        config_map.input_dsp[dsp_num-1].gain = INPUT_GAIN_22dBu;
    }
    else if (popup_label->Caption == "24dBu")
    {
        config_map.input_dsp[dsp_num-1].gain = INPUT_GAIN_24dBu;
    }

    String cmd_text = D1608CMD_FLAG;
    cmd_text = cmd_text+ "input<"+IntToStr(dsp_num)+">.gain="+popup_label->Caption;
    SendCmd(cmd_text+D1608CMD_TAIL);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::MenuItem3Click(TObject *Sender)
{
    // 输出菜单
    TLabel * popup_label = (TLabel*)PopupMenu2->PopupComponent;
    popup_label->Caption = ((TMenuItem*)Sender)->Caption;
    int dsp_num = popup_label->Tag - 16;

    if (popup_label->Caption == "10dBv")
    {
        config_map.output_dsp[dsp_num-1].gain = OUTPUT_GAIN_10dBv;
    }
    else if (popup_label->Caption == "22dBu")
    {
        config_map.output_dsp[dsp_num-1].gain = OUTPUT_GAIN_22dBu;
    }
    else if (popup_label->Caption == "24dBu")
    {
        config_map.output_dsp[dsp_num-1].gain = OUTPUT_GAIN_24dBu;
    }

    String cmd_text = D1608CMD_FLAG;
    cmd_text = cmd_text+ "output<"+IntToStr(dsp_num)+">.gain="+popup_label->Caption;
    SendCmd(cmd_text+D1608CMD_TAIL);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::i10dBvDrawItem(TObject *Sender, TCanvas *ACanvas,
      TRect &ARect, bool Selected)
{
    ACanvas->Font->Name = "MS Sans Serif";
    ACanvas->Font->Color = clAqua;
    ACanvas->Font->Style = TFontStyles()<< fsBold;

    if (Selected)
    {
        ACanvas->Brush->Color = (TColor)0x00808000;
    }
    else
    {
        ACanvas->Brush->Color = (TColor)0x004A392C;
    }
    
    int text_width = ACanvas->TextWidth(((TMenuItem*)Sender)->Caption);
    int left = (ARect.Width() - text_width) / 2;
    int text_height = ACanvas->TextHeight(((TMenuItem*)Sender)->Caption);
    int top = ARect.Top + (ARect.Height() - text_height) / 2;
    ACanvas->TextRect(ARect, left, top, ((TMenuItem*)Sender)->Caption);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::i10dBvMeasureItem(TObject *Sender, TCanvas *ACanvas,
      int &Width, int &Height)
{
    Width = 34;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btnDspResetEQClick(TObject *Sender)
{
    // XDO: 修改滤波器数量，会受到影响
    int preset_freq_list[11] = {20, 50, 100, 200, 500, 1000, 2000, 5000, 7500, 10000, 20000};
    for (int i=FIRST_FILTER+2; i<=LAST_FILTER-2; i++)
    {
        filter_set.GetFilter(i)->ChangFilterParameter("PARAMETRIC", preset_freq_list[i-1], 0, 4.09);
        filter_set.GetFilter(i)->name = IntToStr(i-1);
        filter_set.SetBypass(i, false);
        filter_set.RepaintPaint(i);
        filter_set.SendPeqCmd(i);
        filter_set.SendBypassCmd(i);
    }

    filter_set.GetFilter(HP_FILTER+1)->ChangFilterParameter("LOW_SHELF", preset_freq_list[FIRST_FILTER], 0, 4.09);
        filter_set.GetFilter(HP_FILTER+1)->name = IntToStr(FIRST_FILTER+1-1);
        filter_set.SetBypass(HP_FILTER+1, false);
        filter_set.RepaintPaint(HP_FILTER+1);
        filter_set.SendPeqCmd(HP_FILTER+1);
        filter_set.SendBypassCmd(HP_FILTER+1);

    filter_set.GetFilter(LP_FILTER-1)->ChangFilterParameter("HIGH_SHELF", preset_freq_list[LP_FILTER-1-1], 0, 4.09);
        filter_set.GetFilter(LP_FILTER-1)->name = IntToStr(LAST_FILTER-1-1);    // name按照PEQ编号
        filter_set.SetBypass(LP_FILTER-1, false);
        filter_set.RepaintPaint(LP_FILTER-1);
        filter_set.SendPeqCmd(LP_FILTER-1);
        filter_set.SendBypassCmd(LP_FILTER-1);

    // 压缩参数
    edtCompRatio->Text = 1;
    edtCompRatio->OnKeyDown(edtCompRatio, enter_key, TShiftState());
    edtCompThreshold->Text = -10;
    edtCompThreshold->OnKeyDown(edtCompThreshold, enter_key, TShiftState());
    edtCompAttackTime->Text = 64;
    edtCompAttackTime->OnKeyDown(edtCompAttackTime, enter_key, TShiftState());
    edtCompReleaseTime->Text = 1000;
    edtCompReleaseTime->OnKeyDown(edtCompReleaseTime, enter_key, TShiftState());
    edtCompGain->Text = 0;
    edtCompGain->OnKeyDown(edtCompGain, enter_key, TShiftState());
}
//---------------------------------------------------------------------------
void __fastcall TForm1::input_panel_level_editKeyDown(TObject *Sender,
      WORD &Key, TShiftState Shift)
{
    TEdit * edt = (TEdit*)Sender;
    int dsp_num = edt->Tag;
    if (input_level_trackbar[dsp_num-1] == NULL)
        return;

    if (Key == VK_RETURN)
    {
        try{
            if (edt->Text.UpperCase() == "OFF")
                input_level_trackbar[dsp_num-1]->Position = input_level_trackbar[dsp_num-1]->Min;
            else
                input_level_trackbar[dsp_num-1]->Position = edt->Text.ToDouble() * 10;
        }catch(...){
            AppendLog(__FUNC__);
        }

        //InputVolumeChange(input_level_trackbar[dsp_num-1]);
        edt->SelectAll();
        edt->OnClick = input_panel_level_editClick;
    }
    else if (Key == VK_ESCAPE)
    {
        if (input_level_trackbar[dsp_num-1] != NULL)
        {
            InputVolumeChange(input_level_trackbar[dsp_num-1]);
        }
    }
    else if (Key == VK_UP || Key == VK_DOWN || Key == VK_PRIOR || Key == VK_NEXT)
    {
        input_level_trackbar[dsp_num-1]->Perform(WM_KEYDOWN, Key, 1);
        edt->SelectAll();
        Key = 0;
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::output_panel_level_editKeyDown(TObject *Sender,
      WORD &Key, TShiftState Shift)
{
    TEdit * edt = (TEdit*)Sender;
    int dsp_num = edt->Tag;
    if (output_level_trackbar[dsp_num-1] == NULL)
        return;

    if (Key == VK_RETURN)
    {
        try{
            if (edt->Text.UpperCase() == "OFF")
                output_level_trackbar[dsp_num-1]->Position = output_level_trackbar[dsp_num-1]->Min;
            else
                output_level_trackbar[dsp_num-1]->Position = edt->Text.ToDouble() * 10;
        }catch(...){
            AppendLog(__FUNC__);
        }

        //OutputVolumeChange(output_level_trackbar[dsp_num-1]);
        edt->SelectAll();
        edt->OnClick = input_panel_level_editClick;
    }
    else if (Key == VK_ESCAPE)
    {
        if (output_level_trackbar[dsp_num-1] != NULL)
        {
            OutputVolumeChange(output_level_trackbar[dsp_num-1]);
        }
    }
    else if (Key == VK_UP || Key == VK_DOWN || Key == VK_PRIOR || Key == VK_NEXT)
    {
        output_level_trackbar[dsp_num-1]->Perform(WM_KEYDOWN, Key, 1);
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::master_panel_level_editKeyDown(TObject *Sender,
      WORD &Key, TShiftState Shift)
{
    TEdit * edt = (TEdit*)Sender;

    if (Key == VK_RETURN)
    {
        try{
            if (edt->Text.UpperCase() == "OFF")
                master_panel_trackbar->Position = master_panel_trackbar->Min;
            else
                master_panel_trackbar->Position = edt->Text.ToDouble() * 10;
        }catch(...){
            AppendLog(__FUNC__);
        }

        //MasterVolumeChange(master_panel_trackbar);
        edt->SelectAll();
        edt->OnClick = input_panel_level_editClick;
    }
    else if (Key == VK_ESCAPE)
    {
        MasterVolumeChange(master_panel_trackbar);
        edt->OnClick = input_panel_level_editClick;
    }
    else if (Key == VK_UP || Key == VK_DOWN || Key == VK_PRIOR || Key == VK_NEXT)
    {
        master_panel_trackbar->Perform(WM_KEYDOWN, Key, 1);
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::input_panel_level_editClick(TObject *Sender)
{
    TEdit * edt = (TEdit*)Sender;
    edt->SelectAll();
    edt->OnClick = NULL;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::input_panel_level_editExit(TObject *Sender)
{
    TEdit * edt = (TEdit*)Sender;
    edt->OnKeyDown(Sender, enter_key, TShiftState());
    edt->OnClick = input_panel_level_editClick;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::io_panel_trackbarKeyDown(TObject *Sender,
      WORD &Key, TShiftState Shift)
{
    TAdvTrackBar* track = (TAdvTrackBar*)Sender;
    int value = track->Position;

    if (value > 0)
    {
        if (Key == VK_PRIOR)
        {
            value = (value/5)*5;
        }
        else if (Key == VK_NEXT)
        {
            value = ((value+4)/5)*5;
        }
    }
    else if (value < 0)
    {
        if (Key == VK_PRIOR)
        {
            value = ((value-4)/5)*5;
        }
        else if (Key == VK_NEXT)
        {
            value = (value/5)*5;
        }
    }

    if (Key == VK_PRIOR)
    {
        track->Position = value + 5;
        Key = 0;
    }
    else if (Key == VK_NEXT)
    {
        track->Position = value - 5;
        Key = 0;
    }

    if (Key == VK_HOME || Key == VK_END)
        Key = 0;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::WatchPaint(TObject *Sender)
{
    TPaintBox * pb_watch = (TPaintBox*)Sender;

    //int max_height = pb_watch->Height-2;
    int level = level_meter[pb_watch->Tag][0];

    // bottom - top
    // 72 - 0
    // -48 ~ 24

    TRect r;
    r.left = 0;
    r.right = pb_watch->Width;
    r.top = 1;

    r.bottom = pb_watch->Height-1;
    pb_watch->Canvas->Brush->Color = (TColor)0x00795C46;
    pb_watch->Canvas->FillRect(r);

    r.left+=2;
    r.right-=2;     

    r.top = std::max(24-level+1, 1);
    r.bottom = pb_watch->Height-1;
    if (r.top > r.bottom)
        r.top = r.bottom;
    pb_watch->Canvas->Brush->Color = clLime;
    pb_watch->Canvas->FillRect(r);

    if (level > 17)
    {
        r.bottom = 24-17+1;
        pb_watch->Canvas->Brush->Color = clYellow;
        pb_watch->Canvas->FillRect(r);
    }

    if (level > 21)
    {
        r.bottom = 24-21+1;
        pb_watch->Canvas->Brush->Color = clRed;
        pb_watch->Canvas->FillRect(r);
    }


    int comp_level = level_meter[pb_watch->Tag][1];
    if (comp_level > -100)
    {
        TRect r;
        r.left = pb_watch->Width/2-1;
        r.right = pb_watch->Width;
        r.top = 1;

        r.bottom = pb_watch->Height-1;
        pb_watch->Canvas->Brush->Color = (TColor)0x00795C46;
        pb_watch->Canvas->FillRect(r);

        r.left+=2;
        r.right-=2;

        r.top = std::max(24-comp_level+1, 1);
        r.bottom = pb_watch->Height-1;
        if (r.top > r.bottom)
            r.top = r.bottom;
        pb_watch->Canvas->Brush->Color = clLime;
        pb_watch->Canvas->FillRect(r);

        if (comp_level > 17)
        {
            r.bottom = 24-17+1;
            pb_watch->Canvas->Brush->Color = clYellow;
            pb_watch->Canvas->FillRect(r);
        }

        if (comp_level > 21)
        {
            r.bottom = 24-21+1;
            pb_watch->Canvas->Brush->Color = clRed;
            pb_watch->Canvas->FillRect(r);
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::input_panel_dsp_numClick(TObject *Sender)
{
    TLabel* label = (TLabel*) Sender;
    ShowInputPanel(label, after_input_panel_dsp_numClick, label->Caption);
}
void __fastcall TForm1::after_input_panel_dsp_numClick(TObject *Sender)
{
    TLabel* label = (TLabel*) Sender;
    int dsp_num = label->Tag + 1;

    if (label->Caption == edtInput->Text)
    {
        return;
    }

    label->Caption = edtInput->Text;
    // 补足空白
    while (label->Caption.Length() < 5)
    {
        label->Caption = label->Caption + " ";
    }

    String cmd_text = D1608CMD_FLAG;
    if (cbGlobalDspName->Checked)
    {
        strncpy(global_config.input_dsp_name[label->Tag], label->Caption.c_str(), 6);

        String cmd_text = D1608CMD_FLAG;
        cmd_text = cmd_text+"config.input<"+IntToStr(dsp_num)+">.name="+label->Caption.SubString(1,6);
        SendCmd(cmd_text+D1608CMD_TAIL);
    }
    else
    {
        strncpy(config_map.output_dsp[label->Tag].dsp_name, label->Caption.c_str(), 6);

        String cmd_text = D1608CMD_FLAG;
        cmd_text = cmd_text+"input<"+IntToStr(dsp_num)+">.name="+label->Caption.SubString(1,6);
        SendCmd(cmd_text+D1608CMD_TAIL);
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::output_panel_dsp_numClick(TObject *Sender)
{
    TLabel* label = (TLabel*) Sender;
    ShowInputPanel(label, after_output_panel_dsp_numClick, label->Caption);
}
void __fastcall TForm1::after_output_panel_dsp_numClick(TObject *Sender)
{
    TLabel* label = (TLabel*) Sender;
    int dsp_num = label->Tag + 1;

    if (label->Caption == edtInput->Text)
    {
        return;
    }

    label->Caption = edtInput->Text;
    // 补足空白
    while (label->Caption.Length() < 5)
    {
        label->Caption = label->Caption + " ";
    }
    
    if (cbGlobalDspName->Checked)
    {
        String cmd_text = D1608CMD_FLAG;
        cmd_text = cmd_text+"config.output<"+IntToStr(dsp_num)+">.name="+label->Caption.SubString(1,16);
        SendCmd(cmd_text+D1608CMD_TAIL);

        strncpy(global_config.output_dsp_name[label->Tag], label->Caption.c_str(), 6);
    }
    else
    {
        String cmd_text = D1608CMD_FLAG;
        cmd_text = cmd_text+"output<"+IntToStr(dsp_num)+">.name="+label->Caption;
        SendCmd(cmd_text+D1608CMD_TAIL);
        
        strncpy(config_map.output_dsp[label->Tag].dsp_name, label->Caption.c_str(), 6);
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::input_panel_trackbarEnter(TObject *Sender)
{
    TAdvTrackBar * trackbar = (TAdvTrackBar*)Sender;
    trackbar->Thumb->Picture = trackbar->Thumb->PictureHot;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::input_panel_trackbarExit(TObject *Sender)
{
    TAdvTrackBar * trackbar = (TAdvTrackBar*)Sender;
    trackbar->Thumb->Picture = trackbar->Thumb->PictureDown;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::pnlmix_level_trackbarChange(TObject *Sender)
{
    TAdvTrackBar* track = (TAdvTrackBar*)Sender;
    int value = track->Position;
    int dsp_num = track->Tag;

    if (mix_level_edit[dsp_num-1] != NULL)
    {
        if (value == -720)
        {
            mix_level_edit[dsp_num-1]->Text = "Off";
        }
        else
        {
            String x;
            mix_level_edit[dsp_num-1]->Text = x.sprintf("%1.1f", value/10.0);
        }
    }

    if (last_out_num_btn != NULL)
    {
        int in_dsp_num = dsp_num;
        int out_dsp_num = last_out_num_btn->Tag;

        String cmd_text = D1608CMD_FLAG;
        cmd_text = cmd_text+"output<"+IntToStr(out_dsp_num)+">.route_input<"+IntToStr(in_dsp_num)+">.volume="+mix_level_edit[dsp_num-1]->Text;
        if (mix_level_edit[dsp_num-1]->Text != "Off")
            cmd_text = cmd_text + "dB";
        SendCmd(cmd_text+D1608CMD_TAIL);

        config_map.output_dsp[out_dsp_num-1].mix[in_dsp_num-1] = value;
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::pnlmix_level_editKeyDown(TObject *Sender,
      WORD &Key, TShiftState Shift)
{
    TEdit * edt = (TEdit*)Sender;
    int dsp_num = edt->Tag;
    if (mix_level_trackbar[dsp_num-1] == NULL)
        return;

    if (Key == VK_RETURN)
    {
        try{
            if (edt->Text.UpperCase() == "OFF")
                mix_level_trackbar[dsp_num-1]->Position = mix_level_trackbar[dsp_num-1]->Min;
            else
                mix_level_trackbar[dsp_num-1]->Position = edt->Text.ToDouble() * 10;
        }catch(...){
            AppendLog(__FUNC__);
        }

        //pnlmix_level_trackbarChange(mix_level_trackbar[dsp_num-1]);
        edt->SelectAll();
        edt->OnClick = input_panel_level_editClick;
    }
    else if (Key == VK_ESCAPE)
    {
        if (mix_level_trackbar[dsp_num-1] != NULL)
        {
            pnlmix_level_trackbarChange(mix_level_trackbar[dsp_num-1]);
        }
    }
    else if (Key == VK_UP || Key == VK_DOWN || Key == VK_PRIOR || Key == VK_NEXT)
    {
        mix_level_trackbar[dsp_num-1]->Perform(WM_KEYDOWN, Key, 1);
        edt->SelectAll();
        Key = 0;
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::pnlmix_muteClick(TObject *Sender)
{
    TSpeedButton* btn = (TSpeedButton*)Sender;
    int dsp_num = btn->Tag;

    if (last_out_num_btn != NULL && mix_level_trackbar[dsp_num-1] != NULL)
    {
        int in_dsp_num = dsp_num;
        int out_dsp_num = last_out_num_btn->Tag;

        String cmd_text = D1608CMD_FLAG;
        cmd_text = cmd_text+"output<"+IntToStr(out_dsp_num)+">.route_input<"+IntToStr(in_dsp_num)+">.mute="+(btn->Down?"on":"off");
        SendCmd(cmd_text+D1608CMD_TAIL);

        config_map.output_dsp[out_dsp_num-1].mix_mute[in_dsp_num-1] = btn->Down;
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btnSavePresetToFileClick(TObject *Sender)
{
    // 保存到文件
    SaveDialog1->Filter = "preset|*.preset";
    if (SaveDialog1->Execute())
    {
        int select_preset_id = clbAvaliablePreset->ItemIndex + 1;
        // save config_map to file
        save_single_preset_to_file_filename = SaveDialog1->FileName;

        if (!udpControl->Active)
        {
            TFileStream * file = new TFileStream(save_single_preset_to_file_filename, fmCreate);
            if (!file)
            {
                ShowMessage("打开文件失败");
                return;
            }

            if (cur_preset_id == select_preset_id)
            {
                all_config_map[select_preset_id-1] = config_map;
            }

            //file->WriteBuffer(&config_map, sizeof(config_map));
            char save_buffer[8*1024];
            SavePresetById(1, 0, &all_config_map[select_preset_id-1].input_dsp[0], save_buffer);
            SavePresetById(1, 1, &all_config_map[select_preset_id-1].input_dsp[8], save_buffer);
            SavePresetById(1, 2, &all_config_map[select_preset_id-1].output_dsp[0], save_buffer);
            SavePresetById(1, 3, &all_config_map[select_preset_id-1].output_dsp[8], save_buffer);
            file->WriteBuffer(&save_buffer, sizeof(save_buffer));

            delete file;
        }
        else
        {
            StartReadPreset(select_preset_id);
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btnLoadPresetFromFileClick(TObject *Sender)
{
    int select_preset_id = clbAvaliablePreset->ItemIndex + 1;
    if (select_preset_id == 0)
    {
        return;
    }
    if (package_list.size() != 0)
    {
        ShowMessage("设备忙");
        return;
    }

    OpenDialog1->Filter = "preset|*.preset";
    if (OpenDialog1->Execute())
    {
        // save config_map to file
        TFileStream * file = new TFileStream(OpenDialog1->FileName, fmOpenRead);
        if (!file)
        {
            ShowMessage("打开文件失败");
            return;
        }

        //file->ReadBuffer(&all_config_map[select_preset_id-1], sizeof(config_map));
        unsigned char save_buffer[8*1024];
        file->ReadBuffer(&save_buffer, sizeof(save_buffer));
        LoadSinglePreset(all_config_map[select_preset_id-1], save_buffer);

        delete file;

        if (!udpControl->Active)
        {
            // 脱机
            if (cur_preset_id == select_preset_id)
            {
                CloseDspDetail();
                tmDelayUpdateUITimer(NULL);
            }
        }
        else
        {
            // 联机
            clbAvaliablePreset->Checked[select_preset_id-1] = true;
            clbAvaliablePresetClickCheck(NULL);

            int preset_id = select_preset_id;
            {
                D1608PresetCmd preset_cmd(version);
                strcpy(preset_cmd.flag, D1608PRESETCMD_PC2FLASH_FLAG);
                preset_cmd.preset = 0x80+preset_id;
                for (int store_page=0;store_page<8;store_page++)
                {
                    preset_cmd.store_page = store_page;
                    int page_index = preset_cmd.store_page / 2;
                    int address = 2048 * page_index;
                    switch(preset_cmd.store_page)
                    {
                    case 0:
                    case 1:
                    case 2:
                    case 3:
                        {
                            const OlaList * input_dict = (const OlaList *)(save_buffer+address+sizeof(Page_Header));
                            int input_dict_size = 
                                    sizeof(input_dict->length) +
                                    sizeof(input_dict->struct_length) +
                                    input_dict->length * sizeof(OlaInfo);

                            memcpy(preset_cmd.data, input_dict, input_dict_size);
                            memcpy(preset_cmd.data+input_dict_size,
                                   ((char*)input_dict)+ input_dict_size + ((preset_cmd.store_page%2==0) ? 0 : input_dict->struct_length*4),
                                   input_dict->struct_length*4);
                            break;
                        }
                    case 4:
                    case 5:
                    case 6:
                    case 7:
                        {
                            const OlaList * output_dict = (const OlaList *)(save_buffer+address+sizeof(Page_Header));
                            int output_dict_size = 
                                    sizeof(output_dict->length) +
                                    sizeof(output_dict->struct_length) +
                                    output_dict->length * sizeof(OlaInfo);

                            memcpy(preset_cmd.data, output_dict, output_dict_size);
                            memcpy(preset_cmd.data+output_dict_size,
                                   ((char*)output_dict)+ output_dict_size+ ((preset_cmd.store_page%2==0) ? 0 : output_dict->struct_length*4),
                                   output_dict->struct_length*4);
                            break;
                        }
                    }
                    preset_cmd.lock_flag = 0;
                    //preset_cmd.verify -= UdpPackageVerifyDiff((unsigned char*)&preset_cmd, sizeof(preset_cmd));

                    TPackage package = {0};
                    memcpy(package.data, &preset_cmd, sizeof(preset_cmd));
                    package.udp_port = UDP_PORT_STORE_PRESET_PC2FLASH;
                    package.data_size = sizeof(preset_cmd);

                    package_list.insert(package_list.begin(), package);
                }

                {
                    // 追加一个0x89作为结束标志
                    D1608PresetCmd preset_cmd(version);
                    strcpy(preset_cmd.flag, D1608PRESETCMD_PC2FLASH_FLAG);
                    preset_cmd.preset = 0x89;
                    preset_cmd.lock_flag = 0;
                    //preset_cmd.verify -= UdpPackageVerifyDiff((unsigned char*)&preset_cmd, sizeof(preset_cmd));

                    TPackage package = {0};
                    memcpy(package.data, &preset_cmd, sizeof(preset_cmd));
                    package.udp_port = UDP_PORT_STORE_PRESET_PC2FLASH;
                    package.data_size = sizeof(preset_cmd);

                    package_list.insert(package_list.begin(), package);
                }
            }
























            TPackage package = package_list.back();
            SendBuffer(dst_ip, package.udp_port, package.data, package.data_size);

            //last_command = UDP_PORT_STORE_PRESET_PC2FLASH;
            //last_restore_package = preset_cmd;
            restor_delay_count = restor_delay_check_count * 3;
            tmDelayBackup->Enabled = true;

            pbBackup->Max = package_list.size();
        }
    }
}
//---------------------------------------------------------------------------
static String InputGain2String(int gain)
{
    switch (gain)
    {
    case 3:
        return "MIC";
    case 5:
        return "10dBv";
    case 6:
        return "22dBu";
    case 7:
        return "24dBu";
    }

    return "ERROR";
}

static String OutputGain2String(int gain)
{
    switch (gain)
    {
    case 7:
        return "10dBv";
    case 5:
        return "22dBu";
    case 6:
        return "24dBu";
    }

    return "ERROR";
}
void __fastcall TForm1::ClearUI()
{
    edtDeviceType->Text = "N/A";
    //edtCmdId->Text = "N/A";
    edtDeviceFullName->Text = "N/A";
    edtStartBuildTime->Text = "N/A";
    lblDeviceRunningTime->Caption = "----";
    lblDeviceRunningTime2->Caption = "----";
    lblVersion->Caption = "-------- " +VersionToStr(version)+ " " + DateTime2Str(GetDateTimeFromMarco(compile_time));;

    memset(display_buffer, 0, sizeof(display_buffer));
}

// 按键功能额映射
static int FunctionKey2ItemIndex(int key_value, int default_index)
{
    switch (key_value)
    {
    case 1:
        return 0;
    case 2:
        return 1;
    case 3:
        return 2;
    case 5:
        return 3;
    }
    return default_index;
}
void __fastcall TForm1::ApplyConfigToUI()
{
    on_loading = true;

    lblPresetName->Caption = global_config.preset_name[cur_preset_id-1];
    /*edtMAC->Text.sprintf("%02X:%02X:%02X:%02X:%02X:%02X",
                        global_config.mac_address[0],
                        global_config.mac_address[1],
                        global_config.mac_address[2],
                        global_config.mac_address[3],
                        global_config.mac_address[4],
                        global_config.mac_address[5]);
    */

    for (int i=0;i<PRESET_NUM;i++)
    {
        clbAvaliablePreset->Checked[i] = global_config.avaliable_preset[i];
    }

    cbLockUpDownMenuKey->Checked = global_config.lock_updownmenu;
    cbLockParameter->Checked = global_config.lock_parameter;
    cbPresetAutoSaved->Checked = global_config.auto_saved;

    {
        // 加密区域
        if ((global_config.reboot_count_limit != 0) || (global_config.running_timer_limit != 0))
        {
            HideLockConfigArea();
        }
        else
        {
            ShowLockConfigArea();
        }

        cbRunningTimer->Checked = global_config.running_timer_limit > 0;
        edtRunningTimer->Text = global_config.running_timer_limit / 3600.0;
        cbRebootCount->Checked = global_config.reboot_count_limit > 0;
        edtRebootCount->Text = global_config.reboot_count_limit;
        cbLockedString->Checked = (global_config.locked_string[0] != '\0');
        edtLockedString->Text = global_config.locked_string;
    }

    edtDeviceName->Text = global_config.d1616_name;

    cbGlobalDspName->Checked = ((global_config.is_global_name == 1) || (global_config.is_global_name == 0xFF));

    cbMenuKeyFunction->ItemIndex = FunctionKey2ItemIndex(global_config.menu_key_function, 0);
    cbUpKeyFunction->ItemIndex = FunctionKey2ItemIndex(global_config.up_key_function, 1);
    cbDownKeyFunction->ItemIndex = FunctionKey2ItemIndex(global_config.down_key_function, 2);

    cbLedTest->Checked = (global_config.led_test == 1);
    cbUsart1ReceiveAck->Checked = (global_config.usart1_receive_other_ack == 1);
    cbUsart3ReceiveAck->Checked = (global_config.usart3_receive_other_ack == 1);

    struct in_addr in;
    in.S_un.S_addr = global_config.static_ip_address;
    SetWindowText(hIpEdit, inet_ntoa(in));

    if (global_config.static_ip_address == 0)
        rbDhcpEnabled->Checked = true;
    else
        rbStaticIpEnabled->Checked = true;

    EnableWindow(hIpEdit, rbStaticIpEnabled->Checked);

    UpdateParameterEnabled();

    // 修改界面
    for (int i=0;i<REAL_INPUT_DSP_NUM;i++)
    {
        input_type_lbl[i]->Caption = InputGain2String(config_map.input_dsp[i].gain);

        input_eq_btn[i]->Down = config_map.input_dsp[i].eq_switch;
        //input_comp_btn[i]->Down = config_map.input_dsp[i].comp_switch;
        //input_auto_btn[i]->Down = config_map.input_dsp[i].auto_switch;
        //input_default_btn[i]->Down = config_map.input_dsp[i].;
        input_invert_btn[i]->Down = config_map.input_dsp[i].invert_switch;
        input_noise_btn[i]->Down = config_map.input_dsp[i].noise_switch;
        input_mute_btn[i]->Down = config_map.input_dsp[i].mute_switch;
        input_level_trackbar[i]->Position = config_map.input_dsp[i].level_a;

        const char *dsp_name;
        if (cbGlobalDspName->Checked)
            dsp_name = global_config.input_dsp_name[i];
        else
            dsp_name = config_map.input_dsp[i].dsp_name;

        if (dsp_name[0] == '\0')
            input_dsp_name[i]->Caption = String(char('A'+i));
        else
            input_dsp_name[i]->Caption = dsp_name;

        input_type_lbl[i]->Caption = InputGain2String(config_map.input_dsp[i].gain);
        input_thumb[i]->Invalidate();
    }

    for (int i=0;i<REAL_OUTPUT_DSP_NUM;i++)
    {
        output_type_lbl[i]->Caption = OutputGain2String(config_map.output_dsp[i].gain);

        output_eq_btn[i]->Down = config_map.output_dsp[i].eq_switch;
        output_comp_btn[i]->Down = config_map.output_dsp[i].comp_switch;
        output_invert_btn[i]->Down = config_map.output_dsp[i].invert_switch;
        output_mute_btn[i]->Down = config_map.output_dsp[i].mute_switch;
        output_level_trackbar[i]->Position = config_map.output_dsp[i].level_a;

        const char *dsp_name;
        if (cbGlobalDspName->Checked)
            dsp_name = global_config.output_dsp_name[i];
        else
            dsp_name = config_map.output_dsp[i].dsp_name;

        if (dsp_name[0] == '\0')
            output_dsp_name[i]->Caption = IntToStr(i+1);
        else
            output_dsp_name[i]->Caption = dsp_name;

        output_type_lbl[i]->Caption = OutputGain2String(config_map.output_dsp[i].gain);
        output_thumb[i]->Invalidate();
    }

    // master
    master_panel_trackbar->Position = config_map.input_dsp[0].master_level_a;
    btnMasterMute->Down = (config_map.input_dsp[0].master_mute_switch==1);

    on_loading = false;
}
void TForm1::OnFeedbackData(unsigned int cmd_id)
{
    on_loading = true;

	int ObjectIndex = -1;

    if (cmd_id == GetOffsetOfData(&config_map.input_dsp[0].master_mute_switch))
    {
        btnMasterMute->Down = config_map.input_dsp[0].master_mute_switch;
    }
    else if (cmd_id == GetOffsetOfData((char*)&config_map.input_dsp[0].master_level_a))
    {
        master_panel_trackbar->Position = config_map.input_dsp[0].master_level_a;
    }
	else if ((cmd_id >= GetOffsetOfData(&config_map.input_dsp))
		&& (cmd_id < sizeof(config_map.input_dsp)+GetOffsetOfData(&config_map.input_dsp)))
	{
		ObjectIndex = (cmd_id - GetOffsetOfData(&config_map.input_dsp)) / sizeof(config_map.input_dsp[0]);
		
		if (cmd_id == GetOffsetOfData(&config_map.input_dsp[ObjectIndex].eq_switch))
		{
            input_eq_btn[ObjectIndex]->Down = config_map.input_dsp[ObjectIndex].eq_switch;
            if (pnlDspDetail->Visible && (pnlDspDetail->Tag-1==ObjectIndex))
            {
                btnDspEq->Down = config_map.input_dsp[ObjectIndex].eq_switch;
            }
		}
// 		else if (cmd_id == GetOffsetOfData(&config_map.input_dsp[ObjectIndex].comp_switch))
// 		{
// 		}
// 		else if (cmd_id == GetOffsetOfData(&config_map.input_dsp[ObjectIndex].auto_switch))
// 		{
// 		}
		else if (cmd_id == GetOffsetOfData(&config_map.input_dsp[ObjectIndex].invert_switch))
		{
            input_invert_btn[ObjectIndex]->Down = config_map.input_dsp[ObjectIndex].invert_switch;
		}
		else if (cmd_id == GetOffsetOfData(&config_map.input_dsp[ObjectIndex].noise_switch))
		{
            input_noise_btn[ObjectIndex]->Down = config_map.input_dsp[ObjectIndex].noise_switch;
		}
		else if (cmd_id == GetOffsetOfData(&config_map.input_dsp[ObjectIndex].mute_switch))
		{
            input_mute_btn[ObjectIndex]->Down = config_map.input_dsp[ObjectIndex].mute_switch;
		}
		else if (cmd_id == GetOffsetOfData(&config_map.input_dsp[ObjectIndex].phantom_switch))
		{
            // 小界面
            if (pnlDspDetail->Visible && (pnlDspDetail->Tag-1==ObjectIndex))
            {
                btnPhanton->Down = config_map.input_dsp[ObjectIndex].phantom_switch;
            }
		}
		else if (cmd_id == GetOffsetOfData((char*)&config_map.input_dsp[ObjectIndex].level_a))
		{
            input_level_trackbar[ObjectIndex]->Position = config_map.input_dsp[ObjectIndex].level_a;
		}
		else if (cmd_id == GetOffsetOfData((char*)&config_map.input_dsp[ObjectIndex].level_b))
		{
            // 小界面
            if (pnlDspDetail->Visible && (pnlDspDetail->Tag-1==ObjectIndex))
            {
                dsp_gain_trackbar->Position = config_map.input_dsp[ObjectIndex].level_b;
            }
		}
		else if (cmd_id == GetOffsetOfData((char*)&config_map.input_dsp[ObjectIndex].gain))
		{
            input_type_lbl[ObjectIndex]->Caption = InputGain2String(config_map.input_dsp[ObjectIndex].gain);
		}
 		else if (cmd_id == GetOffsetOfData((char*)&config_map.input_dsp[ObjectIndex].delay))
 		{
            // 小界面
            if (pnlDspDetail->Visible && (pnlDspDetail->Tag-1==ObjectIndex))
            {
                dsp_delay_trackbar->Position = config_map.input_dsp[ObjectIndex].delay;
            }
 		}
		else if (cmd_id >= GetOffsetOfData(&config_map.input_dsp[ObjectIndex].filter)
			&& (cmd_id < GetOffsetOfData(&config_map.input_dsp[ObjectIndex].filter) + sizeof(config_map.input_dsp[ObjectIndex].filter)))
		{
			//int filter = (cmd_id - GetOffsetOfData(&config_map.input_dsp[ObjectIndex].filter)) / sizeof(config_map.input_dsp[ObjectIndex].filter[0]);
            // 小界面
            if (pnlDspDetail->Visible && (pnlDspDetail->Tag-1==ObjectIndex))
            {
                panel_agent->LoadPreset();
                PaintBox1->Refresh();
            }
            // 缩略图
            input_thumb[ObjectIndex]->Invalidate();
		}
        else if (cmd_id == offsetof(ConfigMap, input_dsp[ObjectIndex].dsp_name))
		{
            input_dsp_name[ObjectIndex]->Caption = config_map.input_dsp[ObjectIndex].dsp_name;
		}
	}
	else if ((cmd_id >= GetOffsetOfData(&config_map.output_dsp))
		&& (cmd_id < sizeof(config_map.output_dsp)+GetOffsetOfData(&config_map.output_dsp)))
	{
		// output
		ObjectIndex = (cmd_id - GetOffsetOfData(&config_map.output_dsp)) / sizeof(config_map.output_dsp[0]);

		if (cmd_id == GetOffsetOfData(&config_map.output_dsp[ObjectIndex].eq_switch))
		{
            output_eq_btn[ObjectIndex]->Down = config_map.output_dsp[ObjectIndex].eq_switch;
            if (pnlDspDetail->Visible && (pnlDspDetail->Tag-101==ObjectIndex))
            {
                btnDspEq->Down = config_map.output_dsp[ObjectIndex].eq_switch;
            }
		}
		else if (cmd_id == GetOffsetOfData(&config_map.output_dsp[ObjectIndex].comp_switch))
		{
            output_comp_btn[ObjectIndex]->Down = config_map.output_dsp[ObjectIndex].comp_switch;
            if (pnlDspDetail->Visible && (pnlDspDetail->Tag-101==ObjectIndex))
            {
                btnDspComp->Down = config_map.output_dsp[ObjectIndex].comp_switch;
            }
		}
		else if (cmd_id == GetOffsetOfData(&config_map.output_dsp[ObjectIndex].invert_switch))
		{
            output_invert_btn[ObjectIndex]->Down = config_map.output_dsp[ObjectIndex].invert_switch;
		}
		else if (cmd_id == GetOffsetOfData(&config_map.output_dsp[ObjectIndex].mute_switch))
		{
            output_mute_btn[ObjectIndex]->Down = config_map.output_dsp[ObjectIndex].mute_switch;
		}
		else if (cmd_id == GetOffsetOfData((char*)&config_map.output_dsp[ObjectIndex].level_a))
		{
            output_level_trackbar[ObjectIndex]->Position = config_map.output_dsp[ObjectIndex].level_a;
		}
		else if (cmd_id == GetOffsetOfData((char*)&config_map.output_dsp[ObjectIndex].level_b))
		{
            // 小界面
            if (pnlDspDetail->Visible && (pnlDspDetail->Tag-101==ObjectIndex))
            {
                dsp_gain_trackbar->Position = config_map.output_dsp[ObjectIndex].level_b;
            }
		}
		else if (cmd_id == GetOffsetOfData((char*)&config_map.output_dsp[ObjectIndex].gain))
		{
           output_type_lbl[ObjectIndex]->Caption = OutputGain2String(config_map.output_dsp[ObjectIndex].gain);
		}
 		else if (cmd_id == GetOffsetOfData((char*)&config_map.output_dsp[ObjectIndex].delay))
 		{
            // 小界面
            if (pnlDspDetail->Visible && (pnlDspDetail->Tag-101==ObjectIndex))
            {
                dsp_delay_trackbar->Position = config_map.output_dsp[ObjectIndex].delay;
            }
 		}
		else if (cmd_id == GetOffsetOfData((char*)&config_map.output_dsp[ObjectIndex].ratio))
		{
            if (pnlDspDetail->Visible && (pnlDspDetail->Tag-101==ObjectIndex))
            {
                edtCompRatio->Text = Ration2String(config_map.output_dsp[ObjectIndex].ratio);
                edtCompRatio->OnKeyDown(edtCompRatio, enter_key, TShiftState());
            }
		}
		else if (cmd_id == GetOffsetOfData((char*)&config_map.output_dsp[ObjectIndex].threshold))
		{
            if (pnlDspDetail->Visible && (pnlDspDetail->Tag-101==ObjectIndex))
            {
                edtCompThreshold->Text = config_map.output_dsp[ObjectIndex].threshold/10.0;
                edtCompThreshold->OnKeyDown(edtCompThreshold, enter_key, TShiftState());
            }
		}
		else if (cmd_id == GetOffsetOfData((char*)&config_map.output_dsp[ObjectIndex].attack_time))
		{
            if (pnlDspDetail->Visible && (pnlDspDetail->Tag-101==ObjectIndex))
            {
                edtCompAttackTime->Text = config_map.output_dsp[ObjectIndex].attack_time/10.0;
                edtCompAttackTime->OnKeyDown(edtCompAttackTime, enter_key, TShiftState());
            }
		}
		else if (cmd_id == GetOffsetOfData((char*)&config_map.output_dsp[ObjectIndex].release_time))
		{
            if (pnlDspDetail->Visible && (pnlDspDetail->Tag-101==ObjectIndex))
            {
                edtCompReleaseTime->Text = config_map.output_dsp[ObjectIndex].release_time/10.0;
                edtCompReleaseTime->OnKeyDown(edtCompReleaseTime, enter_key, TShiftState());
            }
		}
		else if (cmd_id == GetOffsetOfData((char*)&config_map.output_dsp[ObjectIndex].comp_gain))
		{
            if (pnlDspDetail->Visible && (pnlDspDetail->Tag-101==ObjectIndex))
            {
                edtCompGain->Text = config_map.output_dsp[ObjectIndex].comp_gain/10.0;
                edtCompGain->OnKeyDown(edtCompGain, enter_key, TShiftState());
            }
		}
		else if (cmd_id == GetOffsetOfData((char*)&config_map.output_dsp[ObjectIndex].auto_time))
		{
            if (pnlDspDetail->Visible && (pnlDspDetail->Tag-101==ObjectIndex))
            {
                cbCompAutoTime->Checked = config_map.output_dsp[ObjectIndex].auto_time;
            }
		}
		else if (cmd_id >= GetOffsetOfData(&config_map.output_dsp[ObjectIndex].filter)
			&& (cmd_id < GetOffsetOfData(&config_map.output_dsp[ObjectIndex].filter) + sizeof(config_map.output_dsp[ObjectIndex].filter)))
		{
			//int filter = (cmd_id - GetOffsetOfData(&config_map.output_dsp[ObjectIndex].filter)) / sizeof(config_map.output_dsp[ObjectIndex].filter[0]);
            if (pnlDspDetail->Visible && (pnlDspDetail->Tag-101==ObjectIndex))
            {
                panel_agent->LoadPreset();
                PaintBox1->Refresh();
            }
            // 缩略图
            output_thumb[ObjectIndex]->Invalidate();
		}
		else if (cmd_id >= GetOffsetOfData(&config_map.output_dsp[ObjectIndex].mix_mute)
			&& (cmd_id < GetOffsetOfData(&config_map.output_dsp[ObjectIndex].mix_mute) + sizeof(config_map.output_dsp[ObjectIndex].mix_mute)))
		{
            // 小界面
            if (pnlMix->Visible && pnlMix->Tag-1==ObjectIndex)
            {
                for (int i=0;i<REAL_INPUT_DSP_NUM;i++)
                {
                    mix_mute_btn[i]->Down = config_map.output_dsp[ObjectIndex].mix_mute[i];
                    mix_level_trackbar[i]->Position = config_map.output_dsp[ObjectIndex].mix[i];
                }
            }
		}
		else if (cmd_id >= GetOffsetOfData(&config_map.output_dsp[ObjectIndex].mix[0])
			&& (cmd_id < GetOffsetOfData(&config_map.output_dsp[ObjectIndex].mix[0]) + sizeof(config_map.output_dsp[ObjectIndex].mix)))
		{
            // 小界面
            if (pnlMix->Visible && pnlMix->Tag-1==ObjectIndex)
            {
                for (int i=0;i<REAL_INPUT_DSP_NUM;i++)
                {
                    mix_mute_btn[i]->Down = config_map.output_dsp[ObjectIndex].mix_mute[i];
                    mix_level_trackbar[i]->Position = config_map.output_dsp[ObjectIndex].mix[i];
                }
            }
		}
        else if (cmd_id == offsetof(ConfigMap, output_dsp[ObjectIndex].dsp_name))
		{
            output_dsp_name[ObjectIndex]->Caption = config_map.output_dsp[ObjectIndex].dsp_name;
		}
	}

    on_loading = false;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::SetPresetLibFilename(String filename)
{
    preset_lib_filename = filename;
    UpdateCaption();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::UpdateCaption()
{
    if (udpControl->Active)
    {
        Caption = dst_ip;
    }
    else
    {
        Caption = "";
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::UpdateBuildTime()
{
    try
    {
        edtStartBuildTime->Text = DateTime2Str(GetDateTimeFromMarco(device_setting.start_build_time));
    }
    catch(...)
    {
        edtStartBuildTime->Text = device_setting.start_build_time;
        AppendLog(__FUNC__);
    }
    edtStartBuildTime->Text = edtStartBuildTime->Text+"\t" + AppBuildTime2Str(device_setting.build_time) + "\t";
    edtStartBuildTime->Text = edtStartBuildTime->Text + DateTime2Str(GetDateTimeFromMarco(compile_time));
}
//---------------------------------------------------------------------------
void __fastcall TForm1::StoreClick(TObject *Sender)
{
    all_config_map[cur_preset_id-1] = config_map;
    if (udpControl->Active)
    {
        StorePresetMemToFlashPreset store_cmd;
        store_cmd.preset_id = 0xFF;
        SendBuffer(dst_ip, UDP_PORT_STORE_PRESET_MEM2FLASH, &store_cmd, sizeof(store_cmd));
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::StoreAsClick(TObject *Sender)
{
    TMenuItem * menu = (TMenuItem*)Sender;
    char preset_id = menu->Tag;
    if (udpControl->Active)
    {
        StorePresetMemToFlashPreset store_cmd;
        store_cmd.preset_id = preset_id;
        SendBuffer(dst_ip, UDP_PORT_STORE_PRESET_MEM2FLASH, &store_cmd, sizeof(store_cmd));
    }
    // 更新上位机内存
    all_config_map[preset_id-1] = config_map;
    global_config.avaliable_preset[preset_id-1] = 1;
    clbAvaliablePreset->Checked[preset_id-1] = true;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::RecallClick(TObject *Sender)
{
    TMenuItem * menu = (TMenuItem*)Sender;

    if (cbPresetAutoSaved->Checked)
    {
        all_config_map[cur_preset_id-1] = config_map;
    }
    global_config.avaliable_preset[menu->Tag-1] = 1;
    clbAvaliablePreset->Checked[menu->Tag-1] = true;

    CloseDspDetail();

    if (udpControl->Active)
    {
        String cmd_text = D1608CMD_FLAG;
        cmd_text = cmd_text+"config.active_preset="+IntToStr(menu->Tag);
        SendCmd2(cmd_text+D1608CMD_TAIL);

        // 延时5秒后强制从本地内存更新到界面
        tmDelayUpdateUI->Enabled = true;
    }
    else
    {
        SetPresetId(menu->Tag);
        tmDelayUpdateUITimer(NULL);
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btnSetIpClick(TObject *Sender)
{
    String cmd_text = D1608CMD_CONTROL_FLAG;
    if (rbStaticIpEnabled->Checked == false)
    {
        cmd_text = cmd_text+"config.ip=auto";
    }
    else
    {
        char ip_text[100];
        GetWindowText(hIpEdit, ip_text, 20);
        unsigned long ip = inet_addr(ip_text);
        if (ip == INADDR_NONE)
        {
            ShowMessage("IP地址无效，请重新输入");
            return;
        }
        cmd_text = cmd_text+"config.ip="+String(ip_text);
    }
        
    SendCmd2(cmd_text+D1608CMD_TAIL);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btnGetLogClick(TObject *Sender)
{
    if (!udpControl->Active)
    {
        ShowMessage("未联机，无法获取日志");
        return;
    }

    // 最后一包一定是MAC地址，所以实在MAC处理的时候恢复这个Enabled
    btnGetLog->Enabled = false;
    lvLog->Clear();
    LogBuff buff = {LOG_FLAG};
    for (buff.address = 0x8000000+128*1024+10*5*2*1024;
        buff.address < MAC_LIST_START_PAGE+MAC_LIST_SIZE;
        buff.address += 1024)
    {
        // 计算 LogBuff 的校验和
        buff.verify -= UdpPackageVerifyDiff((unsigned char*)&buff, offsetof(LogBuff, data));
        SendLogBuff(UDP_PORT_READ_LOG, &buff, offsetof(LogBuff, data));
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btnGetDebugClick(TObject *Sender)
{
    char buf[1024] = {0};
    SendBuffer(dst_ip, UDP_PORT_GET_DEBUG_INFO, buf, 1024);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::lblPresetNameClick(TObject *Sender)
{
    String new_name = InputBox("修改名称", "", lblPresetName->Caption);
    if (new_name == lblPresetName->Caption)
        return;
    else
        lblPresetName->Caption = new_name;

    String cmd_text = D1608CMD_CONTROL_FLAG;
    cmd_text = cmd_text+"config.preset_name<"+IntToStr(cur_preset_id)+">=" + lblPresetName->Caption.SubString(1,16);
    SendCmd(cmd_text+D1608CMD_TAIL);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btnDeviceNameClick(TObject *Sender)
{
    if (edtDeviceName->Text != "" && edtDeviceName->Text.Trim() == "")
    {
        ShowMessage("设备名称不能全部是空格");
        return;
    }
    if (edtDeviceName->Text.Pos(";") != 0)
    {
        ShowMessage("设备名称不能含有分号';'");
        return;
    }

    strncpy(global_config.d1616_name, edtDeviceName->Text.c_str(), 16);

    UpdateCaption();

    String cmd_text = D1608CMD_CONTROL_FLAG;
    cmd_text = cmd_text+"config.name="+edtDeviceName->Text.SubString(1,16);
    SendCmd2(cmd_text+D1608CMD_TAIL);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::clbAvaliablePresetClickCheck(TObject *Sender)
{
    // 不能全部不选中
    bool is_unselect_all = true;
    for (int i=0;i<clbAvaliablePreset->Count;i++)
    {
        if (clbAvaliablePreset->Checked[i])
        {
            is_unselect_all = false;
            break;
        }
    }
    if (is_unselect_all)
    {
        // 恢复
        for (int i=0;i<PRESET_NUM;i++)
        {
            clbAvaliablePreset->Checked[i] = global_config.avaliable_preset[i];
        }
        return;
    }

    for (int i=0;i<PRESET_NUM;i++)
    {
        if (global_config.avaliable_preset[i] !=  clbAvaliablePreset->Checked[i])
        {
            global_config.avaliable_preset[i] =  clbAvaliablePreset->Checked[i];

            String cmd_text = D1608CMD_CONTROL_FLAG;
            cmd_text = cmd_text+"config.avaliable_preset<"+IntToStr(i+1)+">="+(clbAvaliablePreset->Checked[i]?"on":"off");
            SendCmd2(cmd_text+D1608CMD_TAIL);
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btnSetLockClick(TObject *Sender)
{
    ActiveControl = NULL;

    if (edtPassword->Text=="")
    {
        ShowMessage("必须设置解锁密码");
        return;
    }

    String cmd_text = D1608CMD_CONTROL_FLAG;

    if (edtRunningTimer->Enabled && running_timer == 0)
    {
        ShowMessage("运行时间限制必须为大于0的有效值");
        return;
    }
    if (edtRebootCount->Enabled && roboot_count == 0)
    {
        ShowMessage("运行次数限制必须为大于0的有效值");
        return;
    }

	global_config.running_timer_limit = edtRunningTimer->Enabled ? running_timer : 0;
	global_config.reboot_count_limit = edtRebootCount->Enabled ? roboot_count : 0;
    if ((global_config.running_timer_limit==0) && (global_config.reboot_count_limit==0))
    {
        ShowMessage("至少设置一个限制条件");
        return;
    }
    if (edtLockedString->Enabled)
        strncpy(global_config.locked_string, edtLockedString->Text.c_str(), 16);
    else
        global_config.locked_string[0] = '\0';



    // 混淆密码
    edtPassword->Text = edtPassword->Text.SubString(1, 16);
    ConfusionChar(edtPassword->Text.c_str(), global_config.password, D1608CMD_CONTROL_FLAG);
    edtKeyPassword->Text = edtKeyPassword->Text.SubString(1, 16);
    ConfusionChar(edtKeyPassword->Text.c_str(), global_config.password_of_key, D1608CMD_CONTROL_FLAG);

    cmd_text = cmd_text+String("config.lock=")+IntToStr(global_config.running_timer_limit)
                      +","+IntToStr(global_config.reboot_count_limit)
                      +","+global_config.locked_string
                      +","+global_config.password
                      +","+global_config.password_of_key;

    SendCmd(cmd_text+D1608CMD_TAIL);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btnUnlockClick(TObject *Sender)
{
    if (edtUnlockPassword->Text == "")
    {
        ShowMessage("请输入密码");
        return;
    }

    String cmd_text = D1608CMD_CONTROL_FLAG;
    cmd_text = cmd_text+"config.unlock="+edtUnlockPassword->Text.SubString(1,16);
    SendCmd2(cmd_text+D1608CMD_TAIL);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::edtKeyPasswordKeyDown(TObject *Sender, WORD &Key,
      TShiftState Shift)
{
    if (Key == VK_UP)
    {
        btnKeyPasswordUp->Click();
    }
    if (Key == VK_DOWN)
    {
        btnKeyPasswordDown->Click();
    }

    if ((Key == VK_BACK) || (Key == VK_LEFT) || (Key == VK_RIGHT) || (Key == VK_DELETE))
    {
    }
    else
    {
        Key = 0;
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::edtPasswordKeyPress(TObject *Sender, char &Key)
{
    if (islower(Key))
    {
        Key = toupper(Key);
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::cbRunningTimerClick(TObject *Sender)
{
    edtRunningTimer->Enabled = cbRunningTimer->Checked;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::cbRebootCountClick(TObject *Sender)
{
    edtRebootCount->Enabled = cbRebootCount->Checked;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::cbLockedStringClick(TObject *Sender)
{
    edtLockedString->Enabled = cbLockedString->Checked;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::edtRunningTimerExit(TObject *Sender)
{
    TEdit * edt = (TEdit*)Sender;
    try
    {
        float rt = edt->Text.ToDouble();
        if (rt > 10000)
            rt = 10000;
        running_timer = rt * 3600;
        edt->Text = FormatFloat("0.00", running_timer/3600.0);
    }
    catch(...)
    {
        edt->Text = FormatFloat("0.00", running_timer/3600.0);
        AppendLog(__FUNC__);
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::edtRebootCountExit(TObject *Sender)
{
    TEdit * edt = (TEdit*)Sender;
    try
    {
        int rt = edt->Text.ToInt();
        if (rt > 10000)
            rt = 10000;
        roboot_count = rt;
        edt->Text = roboot_count;
    }
    catch(...)
    {
        edt->Text = roboot_count;
        AppendLog(__FUNC__);
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::edtRebootCountKeyDown(TObject *Sender, WORD &Key,
      TShiftState Shift)
{
    if (Key == VK_RETURN)
    {
        TEdit * edt = (TEdit*)Sender;
        edt->OnExit(Sender);
        edt->SelectAll();
    }    
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btnKeyPasswordUpClick(TObject *Sender)
{
    edtKeyPassword->Perform(WM_CHAR, 'U', 0);
    //->Text = edtKeyPassword->Text + "U";
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btnKeyPasswordDownClick(TObject *Sender)
{
    edtKeyPassword->Perform(WM_CHAR, 'D', 0);
    //edtKeyPassword->Text = edtKeyPassword->Text + "D";
}
//---------------------------------------------------------------------------
void __fastcall TForm1::edtKeyPasswordKeyPress(TObject *Sender, char &Key)
{
    if (Key != 'D' && Key != 'U' && Key != '\b')
    {
        Key = 0;
    }
}
//-------------------------------------------------------------------------
void __fastcall TForm1::btnUnlockExtClick(TObject *Sender)
{
    // 后台解锁    
    String cmd_text = D1608CMD_CONTROL_FLAG;
    cmd_text = cmd_text+"config.unlock_ext=on";
/*
    清除
	int running_timer_limit;	// 单位：秒
	int reboot_count_limit;
	char locked_string[20];		// 锁屏界面显示的字符串
	char password_of_key[20];	// 键盘解锁密码
	char password[20];			// 密码
*/
    SendCmd(cmd_text+D1608CMD_TAIL);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btnLeaveTheFactoryClick(TObject *Sender)
{
    // 出厂
    String cmd_text = D1608CMD_CONTROL_FLAG;
    cmd_text = cmd_text+"config.adjust_running_time=on";
    SendCmd2(cmd_text+D1608CMD_TAIL);
}
//---------------------------------------------------------------------------
void TForm1::ShowInputPanel(TControl * Sender, TNotifyEvent event, String default_text)
{
    inputObject = Sender;
    input_event = event;

    TPoint pos(edtInput->Left, edtInput->Top);
    pos = Sender->ClientToParent(pos, this);
    
    pos = edtInput->ParentToClient(pos, this);

    edtInput->Left = pos.x;
    edtInput->Top = pos.y;

    edtInput->Text = default_text;
    edtInput->BringToFront();
    edtInput->Show();
    edtInput->MaxLength = 6;
    edtInput->SetFocus();
    edtInput->SelectAll();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btnInputCancelClick(TObject *Sender)
{
    inputObject = NULL;
    edtInput->Hide();    
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btnInputOKClick(TObject *Sender)
{
    input_event(inputObject);

    inputObject = NULL;
    edtInput->Hide();    
}
//---------------------------------------------------------------------------
void __fastcall TForm1::edtInputExit(TObject *Sender)
{
    btnInputCancelClick(Sender);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::edtInputKeyDown(TObject *Sender, WORD &Key,
      TShiftState Shift)
{
    if (Key == VK_ESCAPE)
    {
        btnInputCancelClick(Sender);
    }
    else if (Key == VK_RETURN)
    {
        btnInputOKClick(Sender);
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btnDspEqClick(TObject *Sender)
{
    TSpeedButton* btn = (TSpeedButton*)Sender;
    int dsp_id = btn->Parent->Tag;

    if (dsp_id < 100)
    {
        input_eq_btn[dsp_id-1]->Down = btn->Down;
        input_eq_btn[dsp_id-1]->Click();
    }
    else
    {
        output_eq_btn[dsp_id-101]->Down = btn->Down;
        output_eq_btn[dsp_id-101]->Click();
    }

    filter_set.SetEqSwitch(btn->Down);
    paint_agent->Repaint();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ToogleDSPCOMP(TObject *Sender)
{
    TSpeedButton* btn = (TSpeedButton*)Sender;
    int dsp_id = btn->Parent->Tag;

    if (dsp_id < 100)
    {
        return;
    }

    output_comp_btn[dsp_id-101]->Down = btn->Down;
    output_comp_btn[dsp_id-101]->Click();

    filter_set.SetCompSwitch(btn->Down);
    paint_agent->Repaint();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ToogleCOMP(TObject *Sender)
{
    TSpeedButton* btn = (TSpeedButton*)Sender;
    int dsp_num = btn->Tag;

    String cmd_text = D1608CMD_FLAG;
    cmd_text = cmd_text+ "output<"+IntToStr(dsp_num)+">.comp="+(btn->Down?"on":"off");
    SendCmd(cmd_text+D1608CMD_TAIL);

    config_map.output_dsp[dsp_num-1].comp_switch = btn->Down;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::edtCompRatioKeyDown(TObject *Sender, WORD &Key,
      TShiftState Shift)
{
    TEdit* edt = (TEdit*)Sender;
    int dsp_num = edt->Parent->Parent->Tag;

    if (dsp_num < 100)
    {
        return;
    }
    dsp_num -= 100;

    if (Key == VK_ESCAPE)
    {
        edt->Text = Ration2String(config_map.output_dsp[dsp_num-1].ratio);
    }
    else if (Key == VK_RETURN || Key == VK_PRIOR || Key == VK_NEXT || Key == VK_UP || Key == VK_DOWN)
    {
        int ratio = config_map.output_dsp[dsp_num-1].ratio;
        try{
            //int value = edt->Text.ToDouble()*threshold_config.scale;
            if (Key == VK_RETURN)
            {
                if (edt->Text == "∞")
                    config_map.output_dsp[dsp_num-1].ratio = 0;
                else
                    config_map.output_dsp[dsp_num-1].ratio = 100.0 / edt->Text.ToDouble();//*ratio_config.scale;
            }
            else if (Key == VK_PRIOR)
            {
                config_map.output_dsp[dsp_num-1].ratio += 5;
            }
            else if (Key == VK_NEXT)
            {
                config_map.output_dsp[dsp_num-1].ratio -= 5;
            }
            else if (Key == VK_UP)
            {
                config_map.output_dsp[dsp_num-1].ratio++;
            }
            else if (Key == VK_DOWN)
            {
                config_map.output_dsp[dsp_num-1].ratio--;
            }

            if (config_map.output_dsp[dsp_num-1].ratio > ratio_config.max_value)
                config_map.output_dsp[dsp_num-1].ratio = ratio_config.max_value;
            else if (config_map.output_dsp[dsp_num-1].ratio < ratio_config.min_value)
                config_map.output_dsp[dsp_num-1].ratio = ratio_config.min_value;

            edt->Text = Ration2String(config_map.output_dsp[dsp_num-1].ratio);
            edt->SelectAll();

            if (ratio != config_map.output_dsp[dsp_num-1].ratio)
            {
                String cmd_text = D1608CMD_FLAG;
                cmd_text = cmd_text+"output<"+IntToStr(dsp_num)+">.comp.ratio="+edtCompRatio->Text;
                SendCmd(cmd_text+D1608CMD_TAIL);
            }

            filter_set.ratio = config_map.output_dsp[dsp_num-1].ratio / 100.0;
            pbComp->Invalidate();
        }
        catch(...)
        {
            AppendLog(__FUNC__);
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::edtCompThresholdKeyDown(TObject *Sender, WORD &Key,
      TShiftState Shift)
{
    TEdit* edt = (TEdit*)Sender;
    int dsp_num = edt->Parent->Parent->Tag;

    if (dsp_num < 100)
    {
        return;
    }
    dsp_num -= 100;

    if (Key == VK_ESCAPE)
    {
        edt->Text = FormatFloat(config_map.output_dsp[dsp_num-1].threshold/threshold_config.scale, threshold_config.precise);
    }
    else if (Key == VK_RETURN || Key == VK_PRIOR || Key == VK_NEXT || Key == VK_UP || Key == VK_DOWN)
    {
        int threshold = config_map.output_dsp[dsp_num-1].threshold;
        try{
            int value = edt->Text.ToDouble()*threshold_config.scale;
            if (Key == VK_RETURN)
            {
                config_map.output_dsp[dsp_num-1].threshold = value;
            }
            else if (Key == VK_PRIOR)
            {
                //if (value > 0)
                //    value = (value/5)*5;
                if (value < 0)
                    value = ((value-4)/5)*5;

                config_map.output_dsp[dsp_num-1].threshold = value + 5;
            }
            else if (Key == VK_NEXT)
            {
                //if (value > 0)
                //    value = ((value+4)/5)*5;
                if (value < 0)
                    value = (value/5)*5;

                config_map.output_dsp[dsp_num-1].threshold = value - 5;
            }
            else if (Key == VK_UP)
            {
                config_map.output_dsp[dsp_num-1].threshold = value + 1;
            }
            else if (Key == VK_DOWN)
            {
                config_map.output_dsp[dsp_num-1].threshold = value - 1;
            }

            if (config_map.output_dsp[dsp_num-1].threshold > threshold_config.max_value)
                config_map.output_dsp[dsp_num-1].threshold = threshold_config.max_value;
            else if (config_map.output_dsp[dsp_num-1].threshold < threshold_config.min_value)
                config_map.output_dsp[dsp_num-1].threshold = threshold_config.min_value;

            edt->Text = FormatFloat(config_map.output_dsp[dsp_num-1].threshold/threshold_config.scale, threshold_config.precise);
            edt->SelectAll();

            if (threshold != config_map.output_dsp[dsp_num-1].threshold)
            {
                String cmd_text = D1608CMD_FLAG;
                cmd_text = cmd_text+"output<"+IntToStr(dsp_num)+">.comp.threshold="+edtCompThreshold->Text+"dB";
                SendCmd(cmd_text+D1608CMD_TAIL);
            }

            filter_set.threshold = config_map.output_dsp[dsp_num-1].threshold / 10.0;
            pbComp->Invalidate();
        }
        catch(...)
        {
            AppendLog(__FUNC__);
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::edtCompAttackTimeKeyDown(TObject *Sender,
      WORD &Key, TShiftState Shift)
{
    TEdit* edt = (TEdit*)Sender;
    int dsp_num = edt->Parent->Parent->Tag;

    if (dsp_num < 100)
    {
        return;
    }
    dsp_num -= 100;

    if (Key == VK_ESCAPE)
    {
        edt->Text = FormatFloat(config_map.output_dsp[dsp_num-1].attack_time/attack_config.scale, attack_config.precise);
    }
    else if (Key == VK_RETURN)
    {
        int attack_time = config_map.output_dsp[dsp_num-1].attack_time;
        try{
            config_map.output_dsp[dsp_num-1].attack_time = edt->Text.ToDouble()*attack_config.scale;

            if (config_map.output_dsp[dsp_num-1].attack_time > attack_config.max_value)
                config_map.output_dsp[dsp_num-1].attack_time = attack_config.max_value;
            else if (config_map.output_dsp[dsp_num-1].attack_time < attack_config.min_value)
                config_map.output_dsp[dsp_num-1].attack_time = attack_config.min_value;

            edt->Text = FormatFloat(config_map.output_dsp[dsp_num-1].attack_time/attack_config.scale, attack_config.precise);
            edt->SelectAll();

            if (attack_time != config_map.output_dsp[dsp_num-1].attack_time)
            {
                String cmd_text = D1608CMD_FLAG;
                cmd_text = cmd_text+"output<"+IntToStr(dsp_num)+">.comp.attack_time="+edtCompAttackTime->Text+"ms";
                SendCmd(cmd_text+D1608CMD_TAIL);
            }
        }
        catch(...)
        {
            AppendLog(__FUNC__);
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::edtCompReleaseTimeKeyDown(TObject *Sender,
      WORD &Key, TShiftState Shift)
{
    TEdit* edt = (TEdit*)Sender;
    int dsp_num = edt->Parent->Parent->Tag;

    if (dsp_num < 100)
    {
        return;
    }
    dsp_num -= 100;

    if (Key == VK_ESCAPE)
    {
        edt->Text = FormatFloat(config_map.output_dsp[dsp_num-1].release_time/release_config.scale, release_config.precise);
    }
    else if (Key == VK_RETURN)
    {
        int release_time = config_map.output_dsp[dsp_num-1].release_time;
        try{
            config_map.output_dsp[dsp_num-1].release_time = edtCompReleaseTime->Text.ToDouble()*release_config.scale;

            if (config_map.output_dsp[dsp_num-1].release_time > release_config.max_value)
                config_map.output_dsp[dsp_num-1].release_time = release_config.max_value;
            else if (config_map.output_dsp[dsp_num-1].release_time < release_config.min_value)
                config_map.output_dsp[dsp_num-1].release_time = release_config.min_value;

            edt->Text = FormatFloat(config_map.output_dsp[dsp_num-1].release_time/release_config.scale, release_config.precise);
            edt->SelectAll();

            if (release_time != config_map.output_dsp[dsp_num-1].release_time)
            {
                String cmd_text = D1608CMD_FLAG;
                cmd_text = cmd_text+"output<"+IntToStr(dsp_num)+">.comp.release_time="+edtCompReleaseTime->Text+"ms";
                SendCmd(cmd_text+D1608CMD_TAIL);
            }
        }
        catch(...)
        {
            AppendLog(__FUNC__);
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::edtCompGainKeyDown(TObject *Sender, WORD &Key,
      TShiftState Shift)
{
    TEdit* edt = (TEdit*)Sender;
    int dsp_num = edt->Parent->Parent->Tag;

    if (dsp_num < 100)
    {
        return;
    }
    dsp_num -= 100;

    if (Key == VK_ESCAPE)
    {
        edt->Text = FormatFloat(config_map.output_dsp[dsp_num-1].comp_gain/gain_config.scale, gain_config.precise);
    }
    else if (Key == VK_RETURN || Key == VK_PRIOR || Key == VK_NEXT || Key == VK_UP || Key == VK_DOWN)
    {
        int comp_gain = config_map.output_dsp[dsp_num-1].comp_gain;
        try{
            int value = edt->Text.ToDouble()*gain_config.scale;
            if (Key == VK_RETURN)
            {
                config_map.output_dsp[dsp_num-1].comp_gain = value;
            }
            else if (Key == VK_PRIOR)
            {
                if (value > 0)
                    value = (value/5)*5;
                if (value < 0)
                    value = ((value-4)/5)*5;

                config_map.output_dsp[dsp_num-1].comp_gain = value + 5;
            }
            else if (Key == VK_NEXT)
            {
                if (value > 0)
                    value = ((value+4)/5)*5;
                if (value < 0)
                    value = (value/5)*5;

                config_map.output_dsp[dsp_num-1].comp_gain = value - 5;
            }
            else if (Key == VK_UP)
            {
                config_map.output_dsp[dsp_num-1].comp_gain = value + 1;
            }
            else if (Key == VK_DOWN)
            {
                config_map.output_dsp[dsp_num-1].comp_gain = value - 1;
            }

            if (config_map.output_dsp[dsp_num-1].comp_gain > gain_config.max_value)
                config_map.output_dsp[dsp_num-1].comp_gain = gain_config.max_value;
            else if (config_map.output_dsp[dsp_num-1].comp_gain < gain_config.min_value)
                config_map.output_dsp[dsp_num-1].comp_gain = gain_config.min_value;

            edt->Text = FormatFloat(config_map.output_dsp[dsp_num-1].comp_gain/gain_config.scale, gain_config.precise);
            edt->SelectAll();

            if (comp_gain != config_map.output_dsp[dsp_num-1].comp_gain)
            {
                String cmd_text = D1608CMD_FLAG;
                cmd_text = cmd_text+"output<"+IntToStr(dsp_num)+">.comp.gain="+edtCompGain->Text+"dB";
                SendCmd(cmd_text+D1608CMD_TAIL);
            }

            filter_set.gain = config_map.output_dsp[dsp_num-1].comp_gain / 10.0;
            pbComp->Invalidate();
        }
        catch(...)
        {
            AppendLog(__FUNC__);
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::cbCompAutoTimeClick(TObject *Sender)
{
    TCheckBox* check_box = (TCheckBox*)Sender;
    int dsp_num = check_box->Parent->Parent->Tag;

    if (dsp_num < 100)
    {
        return;
    }

    dsp_num -= 100;

    AdjustOutputCompParam(dsp_num);

    if (cbCompAutoTime->Checked)
    {
        edtCompReleaseTime->Enabled = false;
        edtCompAttackTime->Enabled = false;
        edtCompAttackTime->Text = 64;
        edtCompReleaseTime->Text = 1000;
    }
    else
    {
        edtCompReleaseTime->Enabled = true;
        edtCompAttackTime->Enabled = true;
        edtCompAttackTime->Text = config_map.output_dsp[dsp_num-1].attack_time/attack_config.scale;
        edtCompReleaseTime->Text = config_map.output_dsp[dsp_num-1].release_time/release_config.scale;
    }

    if (cbCompAutoTime->Checked == config_map.output_dsp[dsp_num-1].auto_time)
    {
        edtCompAttackTime->Enabled = !config_map.output_dsp[dsp_num-1].auto_time;
        edtCompReleaseTime->Enabled = !config_map.output_dsp[dsp_num-1].auto_time;
        return;
    }

    config_map.output_dsp[dsp_num-1].auto_time = check_box->Checked;

    String cmd_text = D1608CMD_FLAG;
    cmd_text = cmd_text+ "output<"+IntToStr(dsp_num)+">.auto_comp="+(cbCompAutoTime->Checked?"on":"off");
    SendCmd(cmd_text+D1608CMD_TAIL);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btnMainUIMouseDown(TObject *Sender,
      TMouseButton Button, TShiftState Shift, int X, int Y)
{
    TControl* control = (TControl*)Sender;
    int tag = control->Tag;
    if (Button == mbRight)
        tag = tag + 4;

    ActiveControl = NULL;

    switch (tag)
    {
    case 0:
        pnlOperator->Show();
        pnlOperator->BringToFront();
        break;
    case 1:
        pnlMonitor->Show();
        pnlMonitor->BringToFront();
        break;
    case 2:
        if (String(global_config.admin_password) == String("need password"))
        {
            if (admin_password == "")
            {
                InputPassword->Edit1->Text = "";
                if (InputPassword->ShowModal() == mrOk)
                {
                    char tmp_admin_password[20];
                    ConfusionChar(InputPassword->Edit1->Text.c_str(), tmp_admin_password, D1608CMD_CONTROL_FLAG);

                    admin_password = tmp_admin_password;
                }
                else
                {
                    return;// break;
                }
            }
            // 发送命令
            String cmd_text = D1608CMD_CONTROL_FLAG;
            cmd_text = cmd_text+ "config.admin="+admin_password;
            SendCmd2(cmd_text+D1608CMD_TAIL);
            SendCmd2(cmd_text);
        }
        else
        {
            pnlSystem->Show();
            pnlSystem->BringToFront();
        }
        break;
    case 3:
        pnlConnect->Show();
        pnlConnect->BringToFront();
        break;
    case 4:
        if (is_inner_pc)
        {
            pnlMist->Show();
            pnlMist->BringToFront();
            // 自动读取日志
            if (lvLog->Items->Count == 0 && udpControl->Active)
            {
                btnGetLog->Click();
            }
        }
        break;
    }
    switch (tag)
    {
    case 4:
        break;
    default:
        // 用pnlMist->Visible表示是否需要刷新电压数据显示
        pnlMist->Hide();
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::lbl5VdClick(TObject *Sender)
{
    // 记录显示的
    TControl* control = (TControl*)Sender;

    shape_active_adc->Top = control->Top;
    shape_active_adc->Show();

    Series1->Clear();
    lineUpLimit->Clear();
    lineDownLimit->Clear();
    Chart1->BottomAxis->SetMinMax(0, 100);

    if (control == lbl3_3V)
    {
        up_line_value = device_setting.adc_range.vote_3v3m_up;
        down_line_value = device_setting.adc_range.vote_3v3m_down;
    }
    else  if (control == lbl3_3Vd)
    {
        up_line_value =device_setting.adc_range.vote_3v3_up;
        down_line_value =device_setting.adc_range.vote_3v3_down;
    }
    else  if (control == lbl5Va)
    {
        up_line_value =device_setting.adc_range.vote_5va_up;
        down_line_value =device_setting.adc_range.vote_5va_down;
    }
    else  if (control == lbl5Vd)
    {
        up_line_value =device_setting.adc_range.vote_5vd_up;
        down_line_value =device_setting.adc_range.vote_5vd_down;
    }
    else  if (control == lbl8Vac)
    {
        up_line_value =device_setting.adc_range.vote_8vac_up;
        down_line_value =device_setting.adc_range.vote_8vac_down;
    }
    else  if (control == lbl8Vdc)
    {
        up_line_value =device_setting.adc_range.vote_8vdc_up;
        down_line_value =device_setting.adc_range.vote_8vdc_down;
    }
    else  if (control == lbl12Va)
    {
        up_line_value =device_setting.adc_range.vote_12va_up;
        down_line_value =device_setting.adc_range.vote_12va_down;
    }
    else  if (control == lbl_12Va)
    {
        up_line_value =device_setting.adc_range.vote_x12va_up;
        down_line_value =device_setting.adc_range.vote_x12va_down;
    }
    else  if (control == lbl16Vac)
    {
        up_line_value =device_setting.adc_range.vote_16vac_up;
        down_line_value =device_setting.adc_range.vote_16vac_down;
    }
    else  if (control == lbl_16Vac)
    {
        up_line_value =device_setting.adc_range.vote_x16vac_up;
        down_line_value =device_setting.adc_range.vote_x16vac_down;
    }
    else  if (control == lbl48Vp)
    {
        up_line_value =device_setting.adc_range.vote_48vp_up;
        down_line_value =device_setting.adc_range.vote_48vp_down;
    }

    up_line_value = up_line_value / 1000;
    down_line_value = down_line_value / 1000;

    for (int i=0;i<100;i++)
    {
        lineUpLimit->AddXY(i, up_line_value, "e", clRed);
        lineDownLimit->AddXY(i, down_line_value, "b", clRed);
        Series1->AddNull("");
    }

    // 纵坐标范围设定为+-1V
    float up_range = up_line_value+(up_line_value-down_line_value)/2;
    float down_range = down_line_value-(up_line_value-down_line_value)/2;
    Chart1->LeftAxis->SetMinMax(down_range, up_range);

    active_adc = (TLabel*)Sender;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::tmLedTimer(TObject *Sender)
{
    static int count = 0;
    count += 32;
    count = count % 512;

    int diff = abs(count - 256);

    if (diff == 256)
        diff = 255;

    shape_live->Brush->Color = TColor(diff * 0x100);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::PaintBox2Paint(TObject *Sender)
{
    PaintBox2->Canvas->Draw(0,0,imgBody->Picture->Graphic);

    Graphics::TBitmap * bmp = new Graphics::TBitmap();
    bmp->Width = Bevel8->Width;
    bmp->Height = Bevel8->Height;

    bmp->Canvas->CopyRect(Rect(0,0,Bevel8->Width,Bevel8->Height),
                                PaintBox2->Canvas,
                                Rect(Bevel8->Left,Bevel8->Top,Bevel8->Width,Bevel8->Height));

    bmp->Canvas->Brush->Color = TColor(0x4A392C);
    bmp->Canvas->Pen->Style = psClear;
    bmp->Canvas->RoundRect(0,0,Bevel8->Width,Bevel8->Height, 25,25);

    BLENDFUNCTION blend;
    blend.BlendOp = AC_SRC_OVER;
    blend.BlendFlags = 0;
    blend.SourceConstantAlpha = 128;
    blend.AlphaFormat = 0;

    ::AlphaBlend(PaintBox2->Canvas->Handle,
        Bevel8->Left,Bevel8->Top,Bevel8->Width,Bevel8->Height,
        bmp->Canvas->Handle, 0, 0, Bevel8->Width,Bevel8->Height, blend);

    delete bmp;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::PaintBox3Paint(TObject *Sender)
{
    Graphics::TBitmap * bmp = new Graphics::TBitmap();
    bmp->Width = 500;
    bmp->Height = 500;
    bmp->Canvas->Brush->Color = TColor(0x4A392C);
    bmp->Canvas->FillRect(Rect(0,0,500,500));


    PaintBox3->Canvas->Draw(0,0,imgBody->Picture->Graphic);

    BLENDFUNCTION blend;
    blend.BlendOp = AC_SRC_OVER;
    blend.BlendFlags = 0;
    blend.SourceConstantAlpha = 200;
    blend.AlphaFormat = 0;

    ::AlphaBlend(PaintBox3->Canvas->Handle,
        Bevel2->Left,Bevel2->Top,Bevel2->Width,Bevel2->Height,
        bmp->Canvas->Handle, 0, 0, Bevel2->Width,Bevel2->Height, blend);

    ::AlphaBlend(PaintBox3->Canvas->Handle,
        Bevel3->Left,Bevel3->Top,Bevel3->Width,Bevel3->Height,
        bmp->Canvas->Handle, 0, 0, Bevel3->Width,Bevel3->Height, blend);

    ::AlphaBlend(PaintBox3->Canvas->Handle,
        Bevel4->Left,Bevel4->Top,Bevel4->Width,Bevel4->Height,
        bmp->Canvas->Handle, 0, 0, Bevel4->Width,Bevel4->Height, blend);

    ::AlphaBlend(PaintBox3->Canvas->Handle,
        Bevel5->Left,Bevel5->Top,Bevel5->Width,Bevel5->Height,
        bmp->Canvas->Handle, 0, 0, Bevel5->Width,Bevel5->Height, blend);

    ::AlphaBlend(PaintBox3->Canvas->Handle,
        Bevel6->Left,Bevel6->Top,Bevel6->Width,Bevel6->Height,
        bmp->Canvas->Handle, 0, 0, Bevel6->Width,Bevel6->Height, blend);

    ::AlphaBlend(PaintBox3->Canvas->Handle,
        Bevel7->Left,Bevel7->Top,Bevel7->Width,Bevel7->Height,
        bmp->Canvas->Handle, 0, 0, Bevel7->Width,Bevel7->Height, blend);

    ::AlphaBlend(PaintBox3->Canvas->Handle,
        Bevel1->Left,Bevel1->Top,Bevel1->Width,Bevel1->Height,
        bmp->Canvas->Handle, 0, 0, Bevel1->Width,Bevel1->Height, blend);


    // 设置CheckBox的背景
    // bmp->Canvas->Pixels[rbDhcpEnabled->Left+1][rbDhcpEnabled->Top+1];
    rbDhcpEnabled->Color = bmp->Canvas->Brush->Color;
    rbStaticIpEnabled->Color = bmp->Canvas->Brush->Color;
    cbRunningTimer->Color = bmp->Canvas->Brush->Color;
    cbRebootCount->Color = bmp->Canvas->Brush->Color;
    cbLockedString->Color = bmp->Canvas->Brush->Color;
    cbGlobalDspName->Color = bmp->Canvas->Brush->Color;
    cbPresetAutoSaved->Color = bmp->Canvas->Brush->Color;
    cbLockParameter->Color = bmp->Canvas->Brush->Color;
    cbLockUpDownMenuKey->Color = bmp->Canvas->Brush->Color;
    cbLedTest->Color = bmp->Canvas->Brush->Color;
    cbUsart1ReceiveAck->Color = bmp->Canvas->Brush->Color;
    cbUsart3ReceiveAck->Color = bmp->Canvas->Brush->Color;

    delete bmp;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::PaintBox4Paint(TObject *Sender)
{
    Graphics::TBitmap * bmp = new Graphics::TBitmap();
    bmp->Width = 500;
    bmp->Height = 500;
    bmp->Canvas->Brush->Color = TColor(0x008E3539);
    bmp->Canvas->Pen->Color = clBtnFace;
    bmp->Canvas->RoundRect(0,0,Bevel9->Width,Bevel9->Height, 10,10);


    PaintBox4->Canvas->Draw(0,0,imgHeader->Picture->Graphic);

    BLENDFUNCTION blend;
    blend.BlendOp = AC_SRC_OVER;
    blend.BlendFlags = 0;
    blend.SourceConstantAlpha = 128;
    blend.AlphaFormat = 0;

    ::AlphaBlend(PaintBox4->Canvas->Handle,
        Bevel9->Left,Bevel9->Top,Bevel9->Width,Bevel9->Height,
        bmp->Canvas->Handle, 0, 0, Bevel9->Width,Bevel9->Height, blend);

    delete bmp;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::dsp_gain_trackbarChange(TObject *Sender)
{
    TAdvTrackBar* track = (TAdvTrackBar*)Sender;
    int value = track->Position;
    int dsp_num = track->Parent->Tag;
    String x;
    dsp_gain_edit->Text = x.sprintf("%1.1f", value/10.0);

    if (dsp_num < 100)
    {
        if (config_map.input_dsp[dsp_num-1].level_b != value)
        {
            // input channel
            String cmd_text = D1608CMD_FLAG;
            cmd_text = cmd_text+"input<"+IntToStr(dsp_num)+">.inside_volume="+dsp_gain_edit->Text+"dB";
            SendCmd(cmd_text+D1608CMD_TAIL);

            config_map.input_dsp[dsp_num-1].level_b = value;
        }
    }
    else
    {
        if (config_map.output_dsp[dsp_num-101].level_b != value)
        {
            // output channel
            String cmd_text = D1608CMD_FLAG;
            cmd_text = cmd_text+"output<"+IntToStr(dsp_num-100)+">.inside_volume="+dsp_gain_edit->Text+"dB";
            SendCmd(cmd_text+D1608CMD_TAIL);

            config_map.output_dsp[dsp_num-101].level_b = value;
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::dsp_gain_editKeyDown(TObject *Sender, WORD &Key,
      TShiftState Shift)
{
    if (Key == VK_RETURN)
    {
        try{
            dsp_gain_trackbar->Position = dsp_gain_edit->Text.ToDouble() * 10;
        }catch(...){
            AppendLog(__FUNC__);
        }

        dsp_gain_trackbar->OnChange(dsp_gain_trackbar);
        dsp_gain_edit->SelectAll();
        dsp_gain_edit->OnClick = input_panel_level_editClick;
    }
    else if (Key == VK_ESCAPE)
    {
        dsp_gain_trackbar->OnChange(dsp_gain_trackbar);
    }
    else if (Key == VK_UP || Key == VK_DOWN || Key == VK_PRIOR || Key == VK_NEXT)
    {
        dsp_gain_trackbar->Perform(WM_KEYDOWN, Key, 1);
        dsp_gain_edit->SelectAll();
        Key = 0;
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::dsp_delay_trackbarChange(TObject *Sender)
{
    TAdvTrackBar* track = (TAdvTrackBar*)Sender;
    UINT value = track->Position;
    int dsp_num = track->Parent->Tag;
    dsp_delay_edit->Text = value/1000.0;

    if (dsp_num < 100)
    {
        if (config_map.input_dsp[dsp_num-1].delay != value)
        {
            // input channel
            String cmd_text = D1608CMD_FLAG;
            cmd_text = cmd_text+"input<"+IntToStr(dsp_num)+">.delay="+dsp_delay_edit->Text+"ms";
            SendCmd(cmd_text+D1608CMD_TAIL);

            config_map.input_dsp[dsp_num-1].delay = value;
        }
    }
    else
    {
        if (config_map.output_dsp[dsp_num-101].delay != value)
        {
            // output channel
            String cmd_text = D1608CMD_FLAG;
            cmd_text = cmd_text+"output<"+IntToStr(dsp_num-100)+">.delay="+dsp_delay_edit->Text+"ms";
            SendCmd(cmd_text+D1608CMD_TAIL);

            config_map.output_dsp[dsp_num-101].delay = value;
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::dsp_delay_editKeyDown(TObject *Sender, WORD &Key,
      TShiftState Shift)
{ 
    if (Key == VK_RETURN)
    {
        try{
            float value = dsp_delay_edit->Text.ToDouble()*1000;
            dsp_delay_trackbar->Position = value;//dsp_delay_edit->Text.ToDouble()*1000;
        }catch(...){
            AppendLog(__FUNC__);
        }

        dsp_delay_trackbar->OnChange(dsp_delay_trackbar);
        dsp_delay_edit->SelectAll();
        dsp_delay_edit->OnClick = input_panel_level_editClick;

    }
    else if (Key == VK_ESCAPE)
    {
        dsp_delay_trackbar->OnChange(dsp_delay_trackbar);
    }
    else if (Key == VK_UP || Key == VK_DOWN || Key == VK_PRIOR || Key == VK_NEXT)
    {
        dsp_delay_trackbar->Perform(WM_KEYDOWN, Key, 1);
        dsp_delay_edit->SelectAll();
        Key = 0;
    }
}
//---------------------------------------------------------------------------
static void MoveInputPanel(int panel_id, TForm1 * form)
{
    form->input_dsp_btn[panel_id-1]->Left         = form->input_panel_dsp_btn->Left + (panel_id-1) * PANEL_WIDTH;
    form->input_eq_btn[panel_id-1]->Left          = form->input_panel_eq_btn->Left + (panel_id-1) * PANEL_WIDTH;
    //form->input_comp_btn[panel_id-1]->Left      = form->input_panel_comp_btn->Left + (panel_id-1) * PANEL_WIDTH;
    //form->input_auto_btn[panel_id-1]->Left      = form->input_panel_auto_btn->Left + (panel_id-1) * PANEL_WIDTH;
    //form->input_default_btn[panel_id-1]->Left   = form->input_panel_default_btn->Left + (panel_id-1) * PANEL_WIDTH;
    form->input_invert_btn[panel_id-1]->Left      = form->input_panel_invert_btn->Left + (panel_id-1) * PANEL_WIDTH;
    form->input_noise_btn[panel_id-1]->Left       = form->input_panel_noise_btn->Left + (panel_id-1) * PANEL_WIDTH;
    form->input_mute_btn[panel_id-1]->Left        = form->input_panel_mute_btn->Left + (panel_id-1) * PANEL_WIDTH;
    form->input_level_edit[panel_id-1]->Left      = form->input_panel_level_edit->Left + (panel_id-1) * PANEL_WIDTH;
    form->input_level_trackbar[panel_id-1]->Left  = form->input_panel_trackbar->Left + (panel_id-1) * PANEL_WIDTH;
    form->input_dsp_name[panel_id-1]->Left        = form->input_panel_dsp_num->Left + (panel_id-1) * PANEL_WIDTH;
    form->input_thumb[panel_id-1]->Left           = form->input_panel_thumb->Left + (panel_id-1) * PANEL_WIDTH;


    form->input_dsp_btn[panel_id-1]->Visible = (panel_id <= REAL_INPUT_DSP_NUM);
    form->input_eq_btn[panel_id-1]->Visible = (panel_id <= REAL_INPUT_DSP_NUM);
    //form->input_comp_btn[panel_id-1]->Visible = (panel_id <= REAL_INPUT_DSP_NUM);
    //form->input_auto_btn[panel_id-1]->Visible = (panel_id <= REAL_INPUT_DSP_NUM);
    //form->input_default_btn[panel_id-1]->Visible = (panel_id <= REAL_INPUT_DSP_NUM);
    form->input_invert_btn[panel_id-1]->Visible = (panel_id <= REAL_INPUT_DSP_NUM);
    form->input_noise_btn[panel_id-1]->Visible = (panel_id <= REAL_INPUT_DSP_NUM);
    form->input_mute_btn[panel_id-1]->Visible = (panel_id <= REAL_INPUT_DSP_NUM);
    form->input_level_edit[panel_id-1]->Visible = (panel_id <= REAL_INPUT_DSP_NUM);
    form->input_level_trackbar[panel_id-1]->Visible = (panel_id <= REAL_INPUT_DSP_NUM);
    form->input_dsp_name[panel_id-1]->Visible = (panel_id <= REAL_INPUT_DSP_NUM);
    form->input_thumb[panel_id-1]->Visible = (panel_id <= REAL_INPUT_DSP_NUM);
}
static void MoveWatchPanel(int panel_id, TForm1 * form, int left, int blank_count)
{
    // 输出需要先流出空白
    if (panel_id <= 16)
    {
        form->watch_panel_inner[panel_id-1]->Left = left;

        int max_count = REAL_INPUT_DSP_NUM + blank_count;
        form->watch_panel_inner[panel_id-1]->Visible = (panel_id <= max_count);
        form->pb_watch_list[panel_id-1]->Visible = (panel_id <= REAL_INPUT_DSP_NUM);
        form->label_watch_list[panel_id-1]->Visible = (panel_id <= REAL_INPUT_DSP_NUM);
        form->imgBlankWatch_list[panel_id-1]->Visible = !(panel_id <= REAL_INPUT_DSP_NUM);

        form->input_type_lbl[panel_id-1]->Visible = (panel_id <= REAL_INPUT_DSP_NUM);
    }
    else
    {
        // 用 8-REAL_OUTPUT_DSP_NUM ~  8 的面板填充到前面
        if (panel_id-16 <= REAL_OUTPUT_DSP_NUM)
        {
            form->watch_panel_inner[panel_id-1]->Left = left;
        }
        else
        {
            form->watch_panel_inner[panel_id-1]->Left = left - 8*PANEL_WIDTH;
        }

        int max_count = REAL_OUTPUT_DSP_NUM + blank_count;
        form->watch_panel_inner[panel_id-1]->Visible = (panel_id-16 <= max_count);
        form->pb_watch_list[panel_id-1]->Visible = (panel_id-16 <= REAL_OUTPUT_DSP_NUM);
        form->label_watch_list[panel_id-1]->Visible = (panel_id-16 <= REAL_OUTPUT_DSP_NUM);
        form->imgBlankWatch_list[panel_id-1]->Visible = !(panel_id-16 <= REAL_OUTPUT_DSP_NUM);

        form->output_type_lbl[panel_id-1-16]->Visible = (panel_id-16 <= REAL_OUTPUT_DSP_NUM);
    }
}
static void MoveOutputPanel(int panel_id, TForm1 * form)
{
    form->output_dsp_btn[panel_id-1]->Left          = form->output_panel_dsp_btn->Left + (panel_id-1) * PANEL_WIDTH;
    form->output_eq_btn[panel_id-1]->Left           = form->output_panel_eq_btn->Left + (panel_id-1) * PANEL_WIDTH;
    form->output_comp_btn[panel_id-1]->Left         = form->output_panel_comp_btn->Left + (panel_id-1) * PANEL_WIDTH;
    form->output_invert_btn[panel_id-1]->Left       = form->output_panel_invert_btn->Left + (panel_id-1) * PANEL_WIDTH;
    form->output_mute_btn[panel_id-1]->Left         = form->output_panel_mute_btn->Left + (panel_id-1) * PANEL_WIDTH;
    form->output_number_btn[panel_id-1]->Left       = form->output_panel_number_btn->Left + (panel_id-1) * PANEL_WIDTH;
    form->output_level_edit[panel_id-1]->Left       = form->output_panel_level_edit->Left + (panel_id-1) * PANEL_WIDTH;
    form->output_level_trackbar[panel_id-1]->Left   = form->output_panel_trackbar->Left + (panel_id-1) * PANEL_WIDTH;
    form->output_dsp_name[panel_id-1]->Left         = form->output_panel_dsp_num->Left + (panel_id-1) * PANEL_WIDTH;
    form->output_thumb[panel_id-1]->Left            = form->output_panel_thumb->Left + (panel_id-1) * PANEL_WIDTH;

    form->output_dsp_btn[panel_id-1]->Visible = (panel_id <= REAL_OUTPUT_DSP_NUM);
    form->output_eq_btn[panel_id-1]->Visible = (panel_id <= REAL_OUTPUT_DSP_NUM);
    form->output_comp_btn[panel_id-1]->Visible = (panel_id <= REAL_OUTPUT_DSP_NUM);
    form->output_invert_btn[panel_id-1]->Visible = (panel_id <= REAL_OUTPUT_DSP_NUM);
    form->output_mute_btn[panel_id-1]->Visible = (panel_id <= REAL_OUTPUT_DSP_NUM);
    form->output_number_btn[panel_id-1]->Visible = (panel_id <= REAL_OUTPUT_DSP_NUM);
    form->output_level_edit[panel_id-1]->Visible = (panel_id <= REAL_OUTPUT_DSP_NUM);
    form->output_level_trackbar[panel_id-1]->Visible = (panel_id <= REAL_OUTPUT_DSP_NUM);
    form->output_dsp_name[panel_id-1]->Visible = (panel_id <= REAL_OUTPUT_DSP_NUM);
    form->output_thumb[panel_id-1]->Visible = (panel_id <= REAL_OUTPUT_DSP_NUM);
}
static void MovePnlMix(int panel_id, TForm1 * form)
{
    form->mix_mute_btn[panel_id-1]->Left        = form->pnlmix_mute->Left + (panel_id-1) * PANEL_WIDTH;
    form->mix_level_edit[panel_id-1]->Left      = form->pnlmix_level_edit->Left + (panel_id-1) * PANEL_WIDTH;
    form->mix_level_trackbar[panel_id-1]->Left  = form->pnlmix_level_trackbar->Left + (panel_id-1) * PANEL_WIDTH;
    //CopyInputPanelLabel(form->pnlmix_dsp_num, panel_id)->Caption = String(char('A'-1+panel_id));
}

void TForm1::SetIOChannelNum()
{
    // 需要补充的白板的数量
    int blank_count = 16 - (REAL_INPUT_DSP_NUM + REAL_OUTPUT_DSP_NUM);
    blank_count = max(blank_count, 0);
    // 输入的白板
    int input_blank_count = 0;
    input_blank_count = max(8 - REAL_INPUT_DSP_NUM, 0);         // 最多可以塞多少白板
    input_blank_count = min(input_blank_count, blank_count);    // 实际塞多少白板
    // 输出的白板
    int output_blank_count = 0;
    output_blank_count = max(8 - REAL_OUTPUT_DSP_NUM, 0);       // 最多可以塞多少白板
    output_blank_count = min(input_blank_count, blank_count-input_blank_count);   // 实际塞多少白板



    // i_m_o_panel_width 指： 输入 master 输出 三个面板总宽度 
    int i_m_o_panel_width = imgPresetBg->Width + (REAL_INPUT_DSP_NUM+REAL_OUTPUT_DSP_NUM + blank_count) * PANEL_WIDTH;
    pnlOperator->Width = max(i_m_o_panel_width, Width-16);
        //HorzScrollBar->Visible = (pnlOperator->Width > Width);

    pnlMix->Width = REAL_INPUT_DSP_NUM * PANEL_WIDTH;                               

    //--------------------------------------------
    // input
    int input_panel_left = (pnlOperator->Width - i_m_o_panel_width)/2;
    //watch_panel->Left = input_panel_left;
    //input_type->Left = input_panel_left;
    input_panel_bkground->Left = input_panel_left;
    input_panel_dsp_btn->Left = input_panel_left+4;
    input_panel_eq_btn->Left = input_panel_left+4;
    input_panel_invert_btn->Left = input_panel_left+4;
    input_panel_noise_btn->Left = input_panel_left+4;
    input_panel_mute_btn->Left = input_panel_left+4;
    input_panel_eq_btn->Left = input_panel_left+4;
    input_panel_level_edit->Left = input_panel_left+4;
    input_panel_trackbar->Left = input_panel_left;
    input_panel_dsp_num->Left = input_panel_left+4;
    input_panel_thumb->Left = input_panel_left+4;


    input_panel_bkground->Width = (REAL_INPUT_DSP_NUM + input_blank_count) * PANEL_WIDTH;
    input_panel_bkground->Picture->Bitmap->Width = input_panel_bkground->Width;
    input_panel_bkground->Picture->Bitmap->Height = imgInputTemplate->Height;

    input_panel_bkground->Canvas->Draw(
        -input_panel_bkground->Left,
        -input_panel_bkground->Top,
        imgBody->Picture->Graphic);

    for (int i=0;i<REAL_INPUT_DSP_NUM;i++)
    {
        ::AlphaBlend(input_panel_bkground->Canvas->Handle,
            i*PANEL_WIDTH,imgInputTemplate->Top,PANEL_WIDTH,imgInputTemplate->Height,
            imgInputTemplate->Canvas->Handle, 0, 0, PANEL_WIDTH,imgInputTemplate->Height, blend);
    }
    for (int i=REAL_INPUT_DSP_NUM;i<REAL_INPUT_DSP_NUM+input_blank_count;i++)
    {
        ::AlphaBlend(input_panel_bkground->Canvas->Handle,
            i*PANEL_WIDTH,imgBlank->Top,PANEL_WIDTH,imgBlank->Height,
            imgBlank->Canvas->Handle, 0, 0, PANEL_WIDTH,imgBlank->Height, blend);
    }


    for (int i=2;i<=INPUT_DSP_NUM;i++)
    {
        MoveInputPanel(i, this);
        MoveWatchPanel(i, this, (i-1) * PANEL_WIDTH + input_panel_left, input_blank_count);
    }
    MoveWatchPanel(1, this, (1-1) * PANEL_WIDTH + input_panel_left, input_blank_count);

    //--------------------------------------
    // mix master
    int mix_panel_left = input_panel_left + (REAL_INPUT_DSP_NUM + input_blank_count) * PANEL_WIDTH;
    imgMasterMixBg->Left = mix_panel_left;
    imgPresetBg->Left = mix_panel_left;
    lblMaster->Left = mix_panel_left + PANEL_WIDTH;
    mix_panel_trackbar->Left = mix_panel_left;
    master_panel_trackbar->Left = mix_panel_left+mix_panel_trackbar->Width;
    mix_panel_level_edit->Left = mix_panel_left+5;
    btnMixMute->Left = mix_panel_left+4;
    master_panel_level_edit->Left = mix_panel_left+53;
    btnMasterMute->Left = mix_panel_left+52;
    mix_panel_dsp_num->Left = mix_panel_left+6;
    master_panel_dsp_num->Left = mix_panel_left+49;
    lblPresetName->Left = mix_panel_left+53;
    edtPreset->Left = mix_panel_left+51;


    imgMasterMixBg->Canvas->Draw(
        -imgMasterMixBg->Left,
        -imgMasterMixBg->Top,
        imgBody->Picture->Graphic);
    ::AlphaBlend(imgMasterMixBg->Canvas->Handle,
        0,0,imgMasterMixBg->Width,imgMasterMixBg->Height,
        imgMasterMix->Canvas->Handle, 0,0,imgMasterMixBg->Width,imgMasterMixBg->Height, blend);

    imgPresetBg->Canvas->Draw(
        -imgPresetBg->Left,
        -imgPresetBg->Top,
        imgBody->Picture->Graphic);
    ::AlphaBlend(imgPresetBg->Canvas->Handle,
        0,0,imgMasterMixBg->Width,imgPresetBg->Height,
        imgPreset->Canvas->Handle, 0,0,imgPreset->Width,imgPreset->Height, blend);

    mix_panel_trackbar->Thumb->Picture = input_panel_trackbar->Thumb->Picture;
    mix_panel_trackbar->Thumb->PictureHot = input_panel_trackbar->Thumb->PictureHot;
    mix_panel_trackbar->Thumb->PictureDown = input_panel_trackbar->Thumb->PictureDown;

    master_panel_trackbar->Thumb->Picture = input_panel_trackbar->Thumb->Picture;
    master_panel_trackbar->Thumb->PictureHot = input_panel_trackbar->Thumb->PictureHot;
    master_panel_trackbar->Thumb->PictureDown = input_panel_trackbar->Thumb->PictureDown;

    //------------------------------------
    // 输出
    int output_panel_left = mix_panel_left + imgPresetBg->Width;
    output_panel_bkground->Left = output_panel_left;
    int output_misc_left = output_panel_left + output_blank_count*PANEL_WIDTH;    // 注意空出白板位置
    output_panel_dsp_btn->Left = output_misc_left+4;
    output_panel_eq_btn->Left = output_misc_left+4;
    output_panel_comp_btn->Left = output_misc_left+4;
    output_panel_number_btn->Left = output_misc_left+4;
    output_panel_invert_btn->Left = output_misc_left+4;
    output_panel_mute_btn->Left = output_misc_left+4;
    output_panel_level_edit->Left = output_misc_left+4;
    output_panel_trackbar->Left = output_misc_left;
    output_panel_dsp_num->Left = output_misc_left+4;
    output_panel_thumb->Left = output_misc_left+4;
                       
    output_panel_bkground->Width = (REAL_OUTPUT_DSP_NUM + output_blank_count) * PANEL_WIDTH;
    output_panel_bkground->Picture->Bitmap->Width = output_panel_bkground->Width;
    output_panel_bkground->Canvas->Draw(
        -output_panel_bkground->Left,
        -output_panel_bkground->Top,
        imgBody->Picture->Graphic);

    {
        // 插入WatchLevel的补充
        imgWatchLevelBg->Height = pnlOperator->Height;
        imgWatchLevelBg->Width = pnlOperator->Width;
        imgWatchLevelBg->Picture->Bitmap->Width = imgWatchLevelBg->Width;
        imgWatchLevelBg->Picture->Bitmap->Height = imgWatchLevelBg->Height;
        imgWatchLevelBg->Canvas->Draw(
            -imgWatchLevelBg->Left,
            -imgWatchLevelBg->Top,
            imgBody->Picture->Graphic);
    }

    // 绘制白板
    for (int i=0;i<output_blank_count;i++)
    {
        ::AlphaBlend(output_panel_bkground->Canvas->Handle,
            i*PANEL_WIDTH,imgBlank->Top,PANEL_WIDTH,imgBlank->Height,
            imgBlank->Canvas->Handle, 0, 0, PANEL_WIDTH,imgBlank->Height, blend);
    }

    // 注意空出白板位置
    for (int i=0;i<REAL_OUTPUT_DSP_NUM;i++)
    {
        ::AlphaBlend(output_panel_bkground->Canvas->Handle,
            (i+output_blank_count) * PANEL_WIDTH,imgOutputTemplate->Top,PANEL_WIDTH,imgOutputTemplate->Height,
            imgOutputTemplate->Canvas->Handle, 0, 0, PANEL_WIDTH,imgOutputTemplate->Height, blend);
    }

    for (int i=2;i<=OUTPUT_DSP_NUM;i++)
    {
        MoveOutputPanel(i, this);
    }
    for (int i=1;i<=OUTPUT_DSP_NUM;i++)
    {
        output_comp_btn[i-1]->Enabled = (GetVersionConfig().is_comp);
    }
    for (int i=1;i<=OUTPUT_DSP_NUM;i++)
    {
        // 注意流出白板位置
        MoveWatchPanel(i+16, this, imgMasterMixBg->Left + imgMasterMixBg->Width + (i-1+output_blank_count) * PANEL_WIDTH, output_blank_count);
    }

    //============================
    //----------------------------------
    // 生成pnlmix背景
    pnlmix_background->Picture->Bitmap->Width = INPUT_DSP_NUM * PANEL_WIDTH;  // xDO: 原来是17个，包括automix，现在只按照输入数量
    for (int i=1;i<REAL_INPUT_DSP_NUM;i++)   // xDO: 原来是17个，包括automix，现在只按照输入数量
    {
        TRect templet_image_rect = pnlmix_background->BoundsRect;
        templet_image_rect.Right = PANEL_WIDTH;

        TRect dest_rect = TRect(i*PANEL_WIDTH,
                                templet_image_rect.Top,
                                (i+1)*PANEL_WIDTH,
                                templet_image_rect.Bottom);
        pnlmix_background->Canvas->CopyRect(dest_rect, pnlmix_background->Canvas, templet_image_rect);
    }

    // 生成PnlMix
    mix_mute_btn[0] = pnlmix_mute;
    for (int i=2;i<=REAL_INPUT_DSP_NUM;i++)    // xDO: 原来是17个，包括automix，现在只按照输入数量
    {
        MovePnlMix(i, this);
    }
    mix_level_edit[0] = pnlmix_level_edit;
    mix_level_trackbar[0] = pnlmix_level_trackbar;
    SetWindowLong(pnlmix_level_edit->Handle, GWL_STYLE, GetWindowLong(pnlmix_level_edit->Handle, GWL_STYLE) | ES_RIGHT);
    pnlmix_level_trackbar->OnChange(pnlmix_level_trackbar);
}
//---------------------------------------------------------------------------
void TForm1::CloseDspDetail()
{
    pnlDspDetail->Hide();
    pnlMix->Hide();
    if (last_out_num_btn != NULL)
    {
        last_out_num_btn->Down =false;
        last_out_num_btn = NULL;
    }

    if (last_dsp_btn != NULL)
    {
        last_dsp_btn->Down =false;
        last_dsp_btn = NULL;
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::rbStaticIpEnabledClick(TObject *Sender)
{
    EnableWindow(hIpEdit, rbStaticIpEnabled->Checked);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::Label51Click(TObject *Sender)
{
    rbDhcpEnabled->Checked = true;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::Label42Click(TObject *Sender)
{
    rbStaticIpEnabled->Checked = true;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::cbGlobalDspNameClick(TObject *Sender)
{
    //ApplyConfigToUI();
    global_config.is_global_name = cbGlobalDspName->Checked;

    String cmd_text = D1608CMD_CONTROL_FLAG;
    cmd_text = cmd_text+String("config.use_global_name=")+(cbGlobalDspName->Checked?"on":"off");
    SendCmd2(cmd_text+D1608CMD_TAIL);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::cbPresetAutoSavedClick(TObject *Sender)
{
    global_config.auto_saved = cbPresetAutoSaved->Checked;

    String cmd_text = D1608CMD_CONTROL_FLAG;
    cmd_text = cmd_text+String("config.auto_saved=")+(cbPresetAutoSaved->Checked?"on":"off");
    SendCmd(cmd_text+D1608CMD_TAIL);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btnRebootDeviceClick(TObject *Sender)
{
    if (Application->MessageBox("本操作会重启设备，确认重启吗？", "确认重启设备操作", MB_OKCANCEL|MB_ICONWARNING) != IDOK)
    {
        return;
    }
    else
    {
        String cmd_text = D1608CMD_CONTROL_FLAG;
        cmd_text = cmd_text+"config.action=reboot";      // 0x80
        SendCmd2(cmd_text+D1608CMD_TAIL);
    }
}
//---------------------------------------------------------------------------
String function_key_value[4] =
{
    "next_preset",
    "volume_up",
    "volume_down",
    "master_mute",
};
void __fastcall TForm1::cbMenuKeyFunctionChange(TObject *Sender)
{
    String cmd_text = D1608CMD_CONTROL_FLAG;
    cmd_text = cmd_text+"config.menu_key_function="+function_key_value[cbMenuKeyFunction->ItemIndex];
    SendCmd2(cmd_text+D1608CMD_TAIL);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::cbUpKeyFunctionChange(TObject *Sender)
{
    String cmd_text = D1608CMD_CONTROL_FLAG;
    cmd_text = cmd_text+"config.up_key_function="+function_key_value[cbUpKeyFunction->ItemIndex];
    SendCmd2(cmd_text+D1608CMD_TAIL);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::cbDownKeyFunctionChange(TObject *Sender)
{
    String cmd_text = D1608CMD_CONTROL_FLAG;
    cmd_text = cmd_text+"config.down_key_function="+function_key_value[cbDownKeyFunction->ItemIndex];
    SendCmd2(cmd_text+D1608CMD_TAIL);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::cbLockUpDownMenuKeyClick(TObject *Sender)
{
    global_config.lock_updownmenu = cbLockUpDownMenuKey->Checked;

    String cmd_text = D1608CMD_CONTROL_FLAG;
    cmd_text = cmd_text+"config.lock_key="+String(cbLockUpDownMenuKey->Checked?"on":"off");
    SendCmd2(cmd_text+D1608CMD_TAIL);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btnSaveFlashToFileClick(TObject *Sender)
{
    if (package_list.size() != 0)
    {
        ShowMessage("设备忙");
        return;
    }

    SaveDialog1->Filter = "smc config file(*.smc)|*.smc";

    if (SaveDialog1->Execute())
    {
        if (!udpControl->Active)
        {
            all_config_map[cur_preset_id-1] = config_map;
            // 脱机，保存为逻辑preset，后缀smc
            // 清除缓存数据
            memset(&smc_config, 0, sizeof(smc_config));
            global_config.active_preset_id = cur_preset_id;

            smc_config.file_version = file_version;
            smc_config.global_config = global_config;
            smc_config.device_setting = device_setting;

            // 前80k是存盘使用，2k一页
            for (int i=0;i<8;i++)
            {
                SavePresetById(i+1, 0, &all_config_map[i].input_dsp[0], smc_config.device_flash_dump);
                SavePresetById(i+1, 1, &all_config_map[i].input_dsp[8], smc_config.device_flash_dump);
                SavePresetById(i+1, 2, &all_config_map[i].output_dsp[0], smc_config.device_flash_dump);
                SavePresetById(i+1, 3, &all_config_map[i].output_dsp[8], smc_config.device_flash_dump);
            }

            // 完成，写入文件
            TFileStream * file = new TFileStream(SaveDialog1->FileName, fmCreate);
            if (!file)
            {
                ShowMessage("打开文件失败");
                return;
            }
            file->WriteBuffer(&smc_config, sizeof(smc_config));
            delete file;
        }
        else
        {
            StoreClick(Sender);

            smc_config.device_setting = device_setting;

            // 联机，保存为flash dump，读取完毕后转换成smc
            save_device_to_file_filename = SaveDialog1->FileName;

            // 替换文件名
            String cmd_text = D1608CMD_CONTROL_FLAG;
            cmd_text = cmd_text+"config.filename=s"+ExtractFileName(SaveDialog1->FileName).SubString(1,16);
            SendCmd2(cmd_text+D1608CMD_TAIL);

            memset(smc_config.device_flash_dump, 0, sizeof(smc_config.device_flash_dump));
            smc_config.file_version = file_version;
            smc_config.device_version = last_connection.data.version;
            memcpy(smc_config.cpu_id, device_setting.cpu_id, sizeof(smc_config.cpu_id));
            memcpy(smc_config.mac, last_connection.data.mac.c_str(), sizeof(smc_config.mac));

            for (int address=PRESET_START_PAGE;address<0x8000000+256*1024;address+=1024)
            {
                // 从设备把数据同步上来
                FlashRW_Data flash_rw;
                strcpy(flash_rw.flag, READ_FLASH_FLAG);
                //  每包2k数据
                flash_rw.start_address = address;
                flash_rw.end_address = 0x8000000+256*1024;
                flash_rw.verify -= UdpPackageVerifyDiff((unsigned char*)&flash_rw, offsetof(FlashRW_Data, data));

                TPackage package = {0};
                memcpy(package.data, &flash_rw, offsetof(FlashRW_Data, data));
                package.udp_port = UDP_PORT_READ_FLASH;
                package.data_size = offsetof(FlashRW_Data, data);//sizeof(flash_rw);

                if (address == address<0x8000000+256*1024-1024)
                    package.action = ACT_DISCONNECT;

                package_list.insert(package_list.begin(), package);
            }
                    
            TPackage package = package_list.back();
            SendBuffer(dst_ip, package.udp_port, package.data, package.data_size);

            restor_delay_count = restor_delay_check_count * 3;
            tmDelayBackup->Enabled = true;

            pbBackup->Max = package_list.size();
            pbBackup->Position = 0;
            pbBackup->Show();
            this->Enabled = false;
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btnLoadFileToFlashClick(TObject *Sender)
{
    if (package_list.size() != 0)
    {
        ShowMessage("设备忙");
        return;
    }

    OpenDialog1->Filter = "smc config file(*.smc)|*.smc";
    if (OpenDialog1->Execute())
    {
        memset(&smc_config, 0, sizeof(smc_config));

        TFileStream * file = new TFileStream(OpenDialog1->FileName, fmOpenRead);
        if (!file)
        {
            ShowMessage("打开文件失败");
            return;
        }

        int tmp_file_version;
        file->Seek(sizeof(GlobalConfig), soFromBeginning);
        file->Read(&tmp_file_version, sizeof(tmp_file_version));
        file->Seek(0, soFromBeginning);

        file->ReadBuffer(&smc_config, sizeof(smc_config));
        delete file;

        // 文件版本判断
        if (smc_config.file_version != file_version)
        {
            ShowMessage("文件版本不兼容");
            return;
        }

        // 替换文件名
        smc_config.global_config.import_filename[0] = 'l';
        strncpy(smc_config.global_config.import_filename+1, ExtractFileName(OpenDialog1->FileName).c_str(), 8);
        global_config = smc_config.global_config;
        UpdateParameterEnabled();

        for (int i=0;i<PRESET_NUM;i++)
        {
            ConfigMap config_map;
            LoadPresetById(i+1, config_map, smc_config.device_flash_dump);
            all_config_map[i] = config_map;
        }

        if (!udpControl->Active)
        {
            // 脱机
            SetPresetId(global_config.active_preset_id);
            CloseDspDetail();
            tmDelayUpdateUITimer(NULL);

            // 读取设备信息
            last_connection.data.version = smc_config.device_version;
            //last_connection.data.cpuid.sprintf("%08X%08X%08X", smc_config.cpu_id[0], smc_config.cpu_id[1], smc_config.cpu_id[2]);
            device_setting = smc_config.device_setting;
                device_setting.cpu_id[0] = smc_config.cpu_id[0];
                device_setting.cpu_id[1] = smc_config.cpu_id[1];
                device_setting.cpu_id[2] = smc_config.cpu_id[2];
            last_connection.data.mac.sprintf("%02X:%02X:%02X:%02X:%02X:%02X",
                        smc_config.mac[0], smc_config.mac[2], smc_config.mac[3],
                        smc_config.mac[3], smc_config.mac[4], smc_config.mac[5]);
            // 更新到界面
            lblVersion->Caption = VersionToStr(last_connection.data.version)+ " " +VersionToStr(version)+ " " + DateTime2Str(GetDateTimeFromMarco(compile_time));;
            UpdateDeviceType();
            UpdateBuildTime();
            edtMAC->Text = last_connection.data.mac;

            lvLog->Clear();

            const T_sn_pack * sn_pack_on_flash = (T_sn_pack*)(smc_config.device_flash_dump+(SN_START_PAGE-PRESET_START_PAGE)+active_code_length);

            String device_cpuid;
            device_cpuid.sprintf("%08X%08X%08X", smc_config.cpu_id[0], smc_config.cpu_id[1], smc_config.cpu_id[2]);
            lblCpuId->Caption = "cpu id: "+device_cpuid;
            lblSn->Caption = "sn: " + String(sn_pack_on_flash->sn);
            lblConfigFilename->Caption = "file: " + String(sn_pack_on_flash->name2);

            // 读取日志
            lvLog->Clear();
            memcpy(event_data_tmp, smc_config.device_flash_dump+PRESET_SIZE, LOG_SIZE);
            Event * tail = GetLogPtr(event_data_tmp);
            tail_address = (Event*)((tail - event_data_tmp)*sizeof(Event)+LOG_START_PAGE);
            ProcessLogData();
            mac_count = 0;

            LogBuff buff = {LOG_FLAG};
            memcpy(&buff, smc_config.device_flash_dump+PRESET_SIZE+LOG_SIZE, sizeof(buff));
            ProcessMACLog(buff);
            memcpy(&buff, smc_config.device_flash_dump+PRESET_SIZE+LOG_SIZE+sizeof(buff), sizeof(buff));
            ProcessMACLog(buff);

            lvLog->Items->Count = log_count + mac_count;

            lblLogCount->Caption = "日志数量："+IntToStr(log_count) + "   MAC数量："+IntToStr(mac_count);
        }
        else
        {
            StoreClick(Sender);

            // 联机
            // Download To Device
            // 准备好所有报文
            //  global_config数据
            {
                D1608PresetCmd preset_cmd(version);
                strcpy(preset_cmd.flag, D1608PRESETCMD_PC2FLASH_FLAG);
                preset_cmd.preset = 0x80;
                preset_cmd.store_page = 0;   // 原先是 preset_cmd.store_page = 8;  // 使用8表示最后一页，TODO: 表达不是很清楚
                memcpy(preset_cmd.data, &smc_config.global_config, sizeof(smc_config.global_config));
                preset_cmd.lock_flag = 0;
                //preset_cmd.verify -= UdpPackageVerifyDiff((unsigned char*)&preset_cmd, sizeof(preset_cmd));

                TPackage package = {0};
                memcpy(package.data, &preset_cmd, sizeof(preset_cmd));
                package.udp_port = UDP_PORT_STORE_PRESET_PC2FLASH;
                package.data_size = sizeof(preset_cmd);

                package_list.insert(package_list.begin(), package);
            }
            // 写入所有的preset数据
            Using_Page using_page;
            GetUsingPage(&using_page, smc_config.device_flash_dump);
            for (int preset_id=1;preset_id<=8;preset_id++)
            {
                D1608PresetCmd preset_cmd(version);
                strcpy(preset_cmd.flag, D1608PRESETCMD_PC2FLASH_FLAG);
                preset_cmd.preset = 0x80+preset_id;
                for (int store_page=0;store_page<8;store_page++)
                {
                    // 没有的页直接清除了 TODO: 下位机需要首先清理一下
                    if (using_page.preset_address[preset_id-1][store_page/2] == 0)
                        continue;

                    preset_cmd.store_page = store_page;
                    switch(preset_cmd.store_page)
                    {
                    case 0:
                    case 1:
                    case 2:
                    case 3:
                        {
                            const OlaList * input_dict = (const OlaList *)(using_page.preset_address[preset_id-1][preset_cmd.store_page/2]+sizeof(Page_Header));
                            int input_dict_size = 
                                    sizeof(input_dict->length) +
                                    sizeof(input_dict->struct_length) +
                                    input_dict->length * sizeof(OlaInfo);

                            memcpy(preset_cmd.data, input_dict, input_dict_size);
                            memcpy(preset_cmd.data+input_dict_size,
                                   ((char*)input_dict)+ input_dict_size + ((preset_cmd.store_page%2==0) ? 0 : input_dict->struct_length*4),
                                   input_dict->struct_length*4);
                            break;
                        }
                    case 4:
                    case 5:
                    case 6:
                    case 7:
                        {
                            const OlaList * output_dict = (const OlaList *)(using_page.preset_address[preset_id-1][preset_cmd.store_page/2]+sizeof(Page_Header));
                            int output_dict_size = 
                                    sizeof(output_dict->length) +
                                    sizeof(output_dict->struct_length) +
                                    output_dict->length * sizeof(OlaInfo);

                            memcpy(preset_cmd.data, output_dict, output_dict_size);
                            memcpy(preset_cmd.data+output_dict_size,
                                   ((char*)output_dict)+ output_dict_size+ ((preset_cmd.store_page%2==0) ? 0 : output_dict->struct_length*4),
                                   output_dict->struct_length*4);
                            break;
                        }
                    }
                    preset_cmd.lock_flag = 0;
                    //preset_cmd.verify -= UdpPackageVerifyDiff((unsigned char*)&preset_cmd, sizeof(preset_cmd));

                    TPackage package = {0};
                    memcpy(package.data, &preset_cmd, sizeof(preset_cmd));
                    package.udp_port = UDP_PORT_STORE_PRESET_PC2FLASH;
                    package.data_size = sizeof(preset_cmd);

                    package_list.insert(package_list.begin(), package);
                }
            }

            {
                // 追加一个0x89作为结束标志
                D1608PresetCmd preset_cmd(version);
                strcpy(preset_cmd.flag, D1608PRESETCMD_PC2FLASH_FLAG);
                preset_cmd.preset = 0x89;
                preset_cmd.lock_flag = 0;
                //preset_cmd.verify -= UdpPackageVerifyDiff((unsigned char*)&preset_cmd, sizeof(preset_cmd));

                TPackage package = {0};
                package.action = ACT_DISCONNECT;
                memcpy(package.data, &preset_cmd, sizeof(preset_cmd));
                package.udp_port = UDP_PORT_STORE_PRESET_PC2FLASH;
                package.data_size = sizeof(preset_cmd);

                package_list.insert(package_list.begin(), package);
            }

            TPackage package = package_list.back();
            SendBuffer(dst_ip, package.udp_port, package.data, package.data_size);

            //last_command = UDP_PORT_STORE_PRESET_PC2FLASH;
            //last_restore_package = preset_cmd;
            restor_delay_count = restor_delay_check_count * 3;
            tmDelayBackup->Enabled = true;

            pbBackup->Max = package_list.size();
            pbBackup->Position = 0;
            pbBackup->Show();
            this->Enabled = false;
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::tmDelayUpdateUITimer(TObject *Sender)
{
    tmDelayUpdateUI->Enabled = false;
    config_map = all_config_map[cur_preset_id-1];
    ApplyConfigToUI();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btnClearDebugClick(TObject *Sender)
{
    lvDebug->Clear();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::tmDelayBackupTimer(TObject *Sender)
{
    if (restor_delay_count > 0)
    {
        restor_delay_count--;
    }

    if (restor_delay_count == 0)
    {
        // 超时，终止本次同步
        package_list.clear();
        pbBackup->Hide();
        this->Enabled = true;

        // 如果存在读取preset失败，那么需要重新连接
        if (read_one_preset_package_list.size() != 0)
        {
            CloseControlLink("read_one_preset_package_list 超时太多");
        }
    }
    else
    {
        if (package_list.size() != 0 && (restor_delay_count % restor_delay_check_count == 1))
        {
            TPackage package = package_list.back();
            SendBuffer(dst_ip, package.udp_port, package.data, package.data_size);

            // 备份 恢复 流程使用的延时计时器
            AppendLog(GetTime()+"retry package_list");
        }
        else if (read_one_preset_package_list.size() != 0 && (restor_delay_count%restor_delay_check_count == 1))
        {
            TPackage package = read_one_preset_package_list.back();
            SendBuffer(dst_ip, package.udp_port, package.data, package.data_size);

            // 备份 恢复 流程使用的延时计时器
            AppendLog(GetTime()+"retry read_one_preset_package_list");
        }
        else if (restor_delay_count%restor_delay_check_count == 1)
        {
            AppendLog(GetTime()+"列表空");
            tmDelayBackup->Enabled = false;
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::pmPresetSaveLoadPopup(TObject *Sender)
{
    if (clbAvaliablePreset->ItemIndex != -1)
    {
        String select_item_text = clbAvaliablePreset->Items->Strings[clbAvaliablePreset->ItemIndex];
        SavePresetAs->Caption = "Save " + select_item_text + " to file...";
        LoadPreset->Caption = "Load " + select_item_text + " from file...";
    }
    else
    {
        //pmPresetSaveLoad->
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::clbAvaliablePresetMouseDown(TObject *Sender,
      TMouseButton Button, TShiftState Shift, int X, int Y)
{
    if (Button == mbRight)
    {
        TPoint popup_point(X, Y);
        clbAvaliablePreset->ItemIndex = clbAvaliablePreset->ItemAtPos(popup_point, true);
        if (clbAvaliablePreset->ItemIndex != -1)
        {
            popup_point = clbAvaliablePreset->ClientToScreen(popup_point);
            pmPresetSaveLoad->Popup(popup_point.x, popup_point.y);
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btnResetAllConfigClick(TObject *Sender)
{
    if (Application->MessageBox("本操作会恢复出厂设置，确认操作吗？", "确认恢复出厂设置操作", MB_OKCANCEL|MB_ICONWARNING) != IDOK)
    {
        return;
    }
    else
    {
        String cmd_text = D1608CMD_CONTROL_FLAG;
        cmd_text = cmd_text+"config.action=init";
        SendCmd2(cmd_text+D1608CMD_TAIL);
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::pbOLEDDblClick(TObject *Sender)
{
    // 闪动屏幕
    String cmd_text = D1608CMD_CONTROL_FLAG;
    cmd_text = cmd_text+"config.action=flash_oled";
    SendCmd(cmd_text+D1608CMD_TAIL);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btnResetKeyFunctionClick(TObject *Sender)
{
    //if (cbMenuKeyFunction->ItemIndex != 0)
    {
        cbMenuKeyFunction->ItemIndex = 0;
        cbMenuKeyFunction->OnChange(NULL);
    }

    //if (cbUpKeyFunction->ItemIndex != 1)
    {
        cbUpKeyFunction->ItemIndex = 1;
        cbUpKeyFunction->OnChange(NULL);
    }

    //if (cbDownKeyFunction->ItemIndex != 2)
    {
        cbDownKeyFunction->ItemIndex = 2;
        cbDownKeyFunction->OnChange(NULL);
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btnClearAllPresetClick(TObject *Sender)
{
    if (Application->MessageBox("本操作会清除所有PRESET，确认操作吗？", "确认清除所有PRESET操作", MB_OKCANCEL|MB_ICONWARNING) != IDOK)
    {
        return;
    }
    else
    {
        String cmd_text = D1608CMD_CONTROL_FLAG;
        cmd_text = cmd_text+"config.action=clear_preset";
        SendCmd2(cmd_text+D1608CMD_TAIL);
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::tmDelaySendCmdTimer(TObject *Sender)
{
    if (sendcmd_list.size() == 0)
        return;
        
    if (sendcmd_delay_count > 0)
    {
        sendcmd_delay_count--;
    }

    if (sendcmd_delay_count == 0)
    {
        // 超时，终止本次同步
        sendcmd_list.clear();
        // 设置为失联
        AppendLog("sendcmd_delay_count == 0");
        keep_live_count = CONTROL_TIMEOUT_COUNT;
    }
    else if ((sendcmd_delay_count % delay_send_cmd_check_count) == 1)
    {
        TPackage package = sendcmd_list.back();

        // 重试命令
        SendBuffer(dst_ip, package.udp_port, package.data, package.data_size);

        // 备份 恢复 流程使用的延时计时器
        String cmd = (char*)(sendcmd_list.end()-1)->data;
        AppendLog(GetTime()+" retry sendcmd:"+cmd);
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btnSaveLogClick(TObject *Sender)
{
    TStrings * log_strs = new TStringList();
    TStrings * mac_strs = new TStringList();

    for (int i=0;i<lvLog->Items->Count;i++)
    {
        if (lvLog->Items->Item[i]->Caption != "")
        {
            String line = lvLog->Items->Item[i]->Caption;
            line = line + "\t" + lvLog->Items->Item[i]->SubItems->Strings[0];
            line = line + "\t" + lvLog->Items->Item[i]->SubItems->Strings[1];
            line = line + "\t" + lvLog->Items->Item[i]->SubItems->Strings[2];
            if (lvLog->Items->Item[i]->SubItems->Count > 3)
            {
                line = line + "\t" + lvLog->Items->Item[i]->SubItems->Strings[3];
            }
            if (lvLog->Items->Item[i]->SubItems->Count > 4)
            {
                line = line + "\tHEX " + lvLog->Items->Item[i]->SubItems->Strings[4];
            }
            log_strs->Add(line);
        }
        else
        {
            String line = "\t"+lvLog->Items->Item[i]->SubItems->Strings[0]+"\t"+lvLog->Items->Item[i]->SubItems->Strings[1]+"\t"+lvLog->Items->Item[i]->SubItems->Strings[2];
            mac_strs->Add(line);
        }
    }

    String path = ExtractFilePath(Application->ExeName);

    // 合并日志
    String device_cpuid;
    device_cpuid.sprintf("%08X%08X%08X", device_setting.cpu_id[0], device_setting.cpu_id[1], device_setting.cpu_id[2]);

    // 工厂型号-串号加-cpuid
    String log_file_name = path+edtDeviceType->Text + "-" + last_connection.data.sn + "-" + device_cpuid + ".log";
    String old_log_file_name = path+device_cpuid + ".log";


    // 合并日志
    TStrings * log_data = NULL;
    TStrings * mac_data = NULL;
    if (FileExists(log_file_name))
    {
        log_data = new TStringList();
        log_data->LoadFromFile(log_file_name);
        mac_data = new TStringList();
        mac_data->LoadFromFile(log_file_name);
    }
    else if (FileExists(old_log_file_name))
    {
        log_data = new TStringList();
        log_data->LoadFromFile(old_log_file_name);
        mac_data = new TStringList();
        mac_data->LoadFromFile(old_log_file_name);
    }
    
    if (log_data != NULL)
    {
        // 删除抬头
        if (log_data->Count > 0 && log_data->Strings[0].Pos("地址")!=0)
        {
            log_data->Delete(0);
        }

        MergeLog(log_strs, log_data);
        delete log_data;

        // 合并mac
        MergeMac(mac_strs, mac_data);
        delete mac_data;
    }

    log_strs->AddStrings(mac_strs);

    // 追加抬头
    String head = lvLog->Column[0]->Caption + "\t"
                + lvLog->Column[1]->Caption + "\t"
                + lvLog->Column[2]->Caption + "\t"
                + lvLog->Column[3]->Caption + "\t"
                + lvLog->Column[4]->Caption + "\t"
                + "DATA";
    log_strs->Insert(0, head);

    log_strs->SaveToFile(log_file_name);

    delete log_strs;
    delete mac_strs;
}
//---------------------------------------------------------------------------

void __fastcall TForm1::edtMACMouseDown(TObject *Sender,
      TMouseButton Button, TShiftState Shift, int X, int Y)
{
    if (Button == mbRight && Shift.Contains(ssCtrl))
    {
        frmSetMAC->edtFix->Text = edtMAC->Text.SubString(1, 8);
        frmSetMAC->edtVar->Text = edtMAC->Text.SubString(10, 8);
        if (frmSetMAC->ShowModal() == mrCancel)
            return;

        String mac_address = frmSetMAC->edtFix->Text+":"+frmSetMAC->edtVar->Text;

        // 解析MAC地址
        unsigned char mac_array[6]={0,0,0,0,0,0};
        TStrings * mac_split = new TStringList;
        mac_split->Delimiter = ':';
        mac_split->DelimitedText = mac_address;
        if (mac_split->Count == 6)
        {
            mac_array[0] = ("0x"+mac_split->Strings[0]).ToIntDef(0);
            mac_array[1] = ("0x"+mac_split->Strings[1]).ToIntDef(0);
            mac_array[2] = ("0x"+mac_split->Strings[2]).ToIntDef(0);
            mac_array[3] = ("0x"+mac_split->Strings[3]).ToIntDef(0);
            mac_array[4] = ("0x"+mac_split->Strings[4]).ToIntDef(0);
            mac_array[5] = ("0x"+mac_split->Strings[5]).ToIntDef(0);
        }
        delete mac_split;

        if (memcmp(mac_array, "\0\0\0\0\0\0", 6)==0)
        {
            ShowMessage("输入错误");
        }
        else
        {
            String cmd_text = D1608CMD_CONTROL_FLAG;
            cmd_text = cmd_text+"config.mac_address="+mac_address;
            SendCmd2(cmd_text+D1608CMD_TAIL);
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::edtDeviceTypeExit(TObject *Sender)
{
    TEdit * edt = (TEdit*)Sender;
    edt->OnClick = input_panel_level_editClick;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::PopupMenu3Popup(TObject *Sender)
{
    TPoint xy = pnlOperator->ScreenToClient(PopupMenu3->PopupPoint);
    // 是否input
    if (xy.x >= input_panel_trackbar->Left && xy.x <= input_panel_trackbar->Left+REAL_INPUT_DSP_NUM*PANEL_WIDTH)
    {
        int input_dsp_id = (xy.x - input_panel_trackbar->Left) / PANEL_WIDTH + 1;
        if (input_dsp_id >= 1 && input_dsp_id <= REAL_INPUT_DSP_NUM)
        {
            selected_channel.channel_type = ctInput;
            selected_channel.channel_id = input_dsp_id;

            Copy1->Caption = "Copy Input " + String((char)('A'+input_dsp_id-1));//IntToStr(input_dsp_id);
            Copy1->Enabled = true;
            Copy1->Tag = input_dsp_id;
        }
    }
    else if (xy.x >= output_panel_trackbar->Left && xy.x <= output_panel_trackbar->Left+REAL_OUTPUT_DSP_NUM*PANEL_WIDTH)
    {
        // 是否output
        int output_dsp_id = (xy.x - output_panel_trackbar->Left) / PANEL_WIDTH + 1;
        if (output_dsp_id >= 1 && output_dsp_id <= REAL_OUTPUT_DSP_NUM)
        {
            selected_channel.channel_type = ctOutput;
            selected_channel.channel_id = output_dsp_id;

            Copy1->Caption = "Copy Output " + IntToStr(output_dsp_id);
            Copy1->Enabled = true;
        }
    }
    else
    {
        selected_channel.channel_type = ctNone;
        selected_channel.channel_id = 0;

        Copy1->Caption = "Copy";
        Copy1->Enabled = false;
    }

    Paste1->Enabled = (selected_channel.channel_type != ctNone) && (copied_channel.channel_type != ctNone)
                   && ((selected_channel.channel_type != copied_channel.channel_type) || (selected_channel.channel_id != copied_channel.channel_id));
}
//---------------------------------------------------------------------------
void __fastcall TForm1::Copy1Click(TObject *Sender)
{
    copied_channel = selected_channel;
    selected_channel.channel_type = ctNone;
    selected_channel.channel_id = 0;
}
//---------------------------------------------------------------------------
static void CloneChannelData(InputConfigMap &dest, InputConfigMap &src)
{
    // master_level_a和master_mute不能覆盖
    unsigned char master_mute_switch = dest.master_mute_switch;
    short master_level_a = dest.master_level_a;
    // 通道名称不覆盖
    char dsp_name[7]; memcpy(dsp_name, src.dsp_name, sizeof(dsp_name));
    // 不复制gain
    unsigned gain = src.gain;

    dest = src;

    dest.master_mute_switch = master_mute_switch;
    dest.master_level_a = master_level_a;
    memcpy(dest.dsp_name, dsp_name, sizeof(dsp_name));
    dest.gain = gain;
}
static void CloneChannelData(OutputConfigMap &dest, OutputConfigMap &src, bool is_comp)
{
    dest.eq_switch                 = src.eq_switch                ;
    dest.invert_switch             = src.invert_switch            ;
    dest.mute_switch               = src.mute_switch              ;
    dest.eq_mute_switch            = src.eq_mute_switch           ;
    dest.level_a                   = src.level_a                  ;
    dest.level_b                   = src.level_b                  ;
    //dest.gain                      = src.gain                     ;
    dest.delay                     = src.delay                    ;
    memcpy(dest.filter, src.filter, sizeof(dest.filter));
    //dest.dsp_name[7]               = src.dsp_name[7]              ;
	memcpy(dest.mix, src.mix, sizeof(dest.mix));
	memcpy(dest.mix_mute, src.mix_mute, sizeof(dest.mix_mute));

    if (is_comp)
    {
        dest.comp_switch               = src.comp_switch              ;
        dest.ratio                     = src.ratio                    ;
        dest.threshold                 = src.threshold                ;
        dest.attack_time               = src.attack_time              ;
        dest.release_time              = src.release_time             ;
        dest.comp_gain                 = src.comp_gain                ;
        dest.auto_time                 = src.auto_time                ;
    }
}
static void CloneChannelData(InputConfigMap &dest, OutputConfigMap &src)
{
    dest.eq_switch        = src.eq_switch      ;
    //dest.comp_switch      = src.comp_switch    ;
    //dest.auto_switch    =                    ;
    dest.invert_switch    = src.invert_switch  ;
    //dest.noise_switch   =                    ;
    dest.mute_switch      = src.mute_switch    ;
    //dest.phantom_switch =                    ;
    dest.level_a          = src.level_a        ;
    dest.level_b          = src.level_b        ;
    //dest.gain             = src.gain           ;
    dest.delay            = src.delay          ;
    memcpy(dest.filter, src.filter, sizeof(dest.filter));
    //                    = src.ratio          ;
    //                    = src.threshold      ;
    //                    = src.attack_time    ;
    //                    = src.release_time   ;
    //                    = src.comp_gain      ;
    //                    = src.auto_time      ;
    // dest.master_mute_switch = 
}
static void CloneChannelData(OutputConfigMap &dest, InputConfigMap &src)
{
    dest.eq_switch        = src.eq_switch      ;
    //dest.comp_switch      = src.comp_switch    ;
    //dest.auto_switch    =                    ;
    dest.invert_switch    = src.invert_switch  ;
    //dest.noise_switch   =                    ;
    dest.mute_switch      = src.mute_switch    ;
    //dest.phantom_switch =                    ;
    dest.level_a          = src.level_a        ;
    dest.level_b          = src.level_b        ;
    //dest.gain             = src.gain           ;
    dest.delay            = src.delay          ;
    memcpy(dest.filter, src.filter, sizeof(dest.filter));
    //                    = src.ratio          ;
    //                    = src.threshold      ;
    //                    = src.attack_time    ;
    //                    = src.release_time   ;
    //                    = src.comp_gain      ;
    //                    = src.auto_time      ;
}
void __fastcall TForm1::Paste1Click(TObject *Sender)
{
    Paste1->Enabled = (selected_channel.channel_type != ctNone) && (copied_channel.channel_type != ctNone)
                   && ((selected_channel.channel_type != copied_channel.channel_type) || (selected_channel.channel_id != copied_channel.channel_id));

    // 考虑做成界面运动
    if (Paste1->Enabled)
    {
        String message = "";
        if (copied_channel.channel_type == ctInput)
        {
            message = "Copy Input "+String((char)('A'+copied_channel.channel_id-1));
        }
        else
        {
            message = "Copy Output "+IntToStr(copied_channel.channel_id);
        }
        if (selected_channel.channel_type == ctInput)
        {
            message = message + " to Input "+String((char)('A'+selected_channel.channel_id-1));
        }
        else
        {
            message = message + " to Output "+IntToStr(selected_channel.channel_id);
        }
        if (Application->MessageBox(message.c_str(), "确认复制", MB_OKCANCEL) != IDOK)
        {
            return;
        }


        // 需要根据输入和输出的状态进行区别处理
        if (selected_channel.channel_type == ctInput)
        {
            if (copied_channel.channel_type == ctInput)
                CloneChannelData(config_map.input_dsp[selected_channel.channel_id-1],
                                 config_map.input_dsp[copied_channel.channel_id-1]);
            else if (copied_channel.channel_type == ctOutput)
                CloneChannelData(config_map.input_dsp[selected_channel.channel_id-1],
                                 config_map.output_dsp[copied_channel.channel_id-1]);

            ApplyConfigToUI();
            // 发出命令
            int dsp_num = selected_channel.channel_id;

            input_eq_btn[dsp_num-1]->OnClick(input_eq_btn[dsp_num-1]);
            input_invert_btn[dsp_num-1]->OnClick(input_invert_btn[dsp_num-1]);
            input_noise_btn[dsp_num-1]->OnClick(input_noise_btn[dsp_num-1]);
            input_mute_btn[dsp_num-1]->OnClick(input_mute_btn[dsp_num-1]);
            input_level_trackbar[dsp_num-1]->OnChange(input_level_trackbar[dsp_num-1]);
            //after_input_panel_dsp_numClick(input_dsp_name[dsp_num-1]);

            // 不复制gain
            //cmd_text = D1608CMD_FLAG;
            //cmd_text = cmd_text+ "input<"+IntToStr(dsp_num)+">.gain="+InputGain2String(config_map.input_dsp[dsp_num-1].gain);
            //SendCmd(cmd_text+D1608CMD_TAIL);

            String cmd_text;
            cmd_text = D1608CMD_FLAG;
            cmd_text = cmd_text+ "input<"+IntToStr(dsp_num)+">.phantom="+(config_map.input_dsp[dsp_num-1].phantom_switch?"on":"off");
            SendCmd(cmd_text+D1608CMD_TAIL);

            String send_level_b;
            send_level_b = send_level_b.sprintf("%1.1f", config_map.input_dsp[dsp_num-1].level_b/10.0);
            cmd_text = D1608CMD_FLAG;
            cmd_text = cmd_text+"input<"+IntToStr(dsp_num)+">.inside_volume="+send_level_b+"dB";
            SendCmd(cmd_text+D1608CMD_TAIL);

            unsigned int delay;
            cmd_text = D1608CMD_FLAG;
            cmd_text = cmd_text+"input<"+IntToStr(dsp_num)+">.delay="+String(config_map.input_dsp[dsp_num-1].delay / 1000.0)+"ms";
            SendCmd(cmd_text+D1608CMD_TAIL);

            pnlDspDetail->Tag = dsp_num;
            filter_set.SendPeqCmd(HP_FILTER);
            filter_set.SendBypassCmd(HP_FILTER);
            filter_set.SendPeqCmd(LP_FILTER);
            filter_set.SendBypassCmd(LP_FILTER);

            for (int i=FIRST_FILTER+2; i<=LAST_FILTER-2; i++)
            {
                filter_set.SendPeqCmd(i);
                filter_set.SendBypassCmd(i);
            }

            filter_set.SendPeqCmd(HP_FILTER+1);
            filter_set.SendBypassCmd(HP_FILTER+1);

            filter_set.SendPeqCmd(LP_FILTER-1);
            filter_set.SendBypassCmd(LP_FILTER-1);
        }
        else if (selected_channel.channel_type == ctOutput)
        {
            if (copied_channel.channel_type == ctInput)
                CloneChannelData(config_map.output_dsp[selected_channel.channel_id-1],
                                 config_map.input_dsp[copied_channel.channel_id-1]);
            else if (copied_channel.channel_type == ctOutput)
                CloneChannelData(config_map.output_dsp[selected_channel.channel_id-1],
                                 config_map.output_dsp[copied_channel.channel_id-1],
                                 GetVersionConfig().is_comp);

            ApplyConfigToUI();
            // 发出命令
            int dsp_num = selected_channel.channel_id;

            output_eq_btn[dsp_num-1]->OnClick(output_eq_btn[dsp_num-1]);
            output_invert_btn[dsp_num-1]->OnClick(output_invert_btn[dsp_num-1]);
            output_mute_btn[dsp_num-1]->OnClick(output_mute_btn[dsp_num-1]);
            output_level_trackbar[dsp_num-1]->OnChange(output_level_trackbar[dsp_num-1]);
            //after_output_panel_dsp_numClick(output_dsp_name[dsp_num-1]);

            // 不复制gain
            //cmd_text = D1608CMD_FLAG;
            //cmd_text = cmd_text+ "output<"+IntToStr(dsp_num)+">.gain="+OutputGain2String(config_map.output_dsp[dsp_num-1].gain);
            //SendCmd(cmd_text+D1608CMD_TAIL);

            String cmd_text;
            String send_level_b;
            send_level_b = send_level_b.sprintf("%1.1f", config_map.output_dsp[dsp_num-1].level_b/10.0);
            cmd_text = D1608CMD_FLAG;
            cmd_text = cmd_text+"output<"+IntToStr(dsp_num)+">.inside_volume="+send_level_b+"dB";
            SendCmd(cmd_text+D1608CMD_TAIL);

            unsigned int delay;
            cmd_text = D1608CMD_FLAG;
            cmd_text = cmd_text+"output<"+IntToStr(dsp_num)+">.delay="+String(config_map.output_dsp[dsp_num-1].delay / 1000.0)+"ms";
            SendCmd(cmd_text+D1608CMD_TAIL);

            pnlDspDetail->Tag = dsp_num+100;
            filter_set.SendPeqCmd(HP_FILTER);
            filter_set.SendBypassCmd(HP_FILTER);
            filter_set.SendPeqCmd(LP_FILTER);
            filter_set.SendBypassCmd(LP_FILTER);

            for (int i=FIRST_FILTER+2; i<=LAST_FILTER-2; i++)
            {
                filter_set.SendPeqCmd(i);
                filter_set.SendBypassCmd(i);
            }

            filter_set.SendPeqCmd(HP_FILTER+1);
            filter_set.SendBypassCmd(HP_FILTER+1);

            filter_set.SendPeqCmd(LP_FILTER-1);
            filter_set.SendBypassCmd(LP_FILTER-1);

            // 如果没有压缩功能，也不发命令
            if (copied_channel.channel_type == ctOutput && GetVersionConfig().is_comp)
            {
                // 只有 Output -> Output 才需要发压缩命令
                output_comp_btn[dsp_num-1]->OnClick(output_comp_btn[dsp_num-1]);

                // 压缩参数
                cmd_text = D1608CMD_FLAG;
                cmd_text = cmd_text+"output<"+IntToStr(dsp_num)+">.comp.ratio="+Ration2String(config_map.output_dsp[dsp_num-1].ratio);
                SendCmd(cmd_text+D1608CMD_TAIL);

                cmd_text = D1608CMD_FLAG;
                cmd_text = cmd_text+"output<"+IntToStr(dsp_num)+">.comp.threshold="+String(config_map.output_dsp[dsp_num-1].threshold/10.0)+"dB";
                SendCmd(cmd_text+D1608CMD_TAIL);

                cmd_text = D1608CMD_FLAG;
                cmd_text = cmd_text+"output<"+IntToStr(dsp_num)+">.comp.gain="+String(config_map.output_dsp[dsp_num-1].comp_gain/10.0)+"dB";
                SendCmd(cmd_text+D1608CMD_TAIL);

                cmd_text = D1608CMD_FLAG;
                cmd_text = cmd_text+ "output<"+IntToStr(dsp_num+1)+">.auto_comp="+(config_map.output_dsp[dsp_num-1].auto_time?"on":"off");
                SendCmd(cmd_text+D1608CMD_TAIL);

                cmd_text = D1608CMD_FLAG;
                cmd_text = cmd_text+"output<"+IntToStr(dsp_num)+">.comp.attack_time="+String(config_map.output_dsp[dsp_num-1].attack_time/attack_config.scale)+"ms";
                SendCmd(cmd_text+D1608CMD_TAIL);

                cmd_text = D1608CMD_FLAG;
                cmd_text = cmd_text+"output<"+IntToStr(dsp_num)+">.comp.release_time="+String(config_map.output_dsp[dsp_num-1].release_time/release_config.scale)+"ms";
                SendCmd(cmd_text+D1608CMD_TAIL);
            }
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::FormResize(TObject *Sender)
{
    need_resize = true;
}
//---------------------------------------------------------------------------
void TForm1::StartReadCurrentPreset(bool lock_flag)
{
    AppendLog(GetTime()+"同步当前Preset数据");
    read_one_preset_package_list.clear();
    for (int store_page=0;store_page<8;store_page++)
    {
        D1608PresetCmd preset_cmd(version);
        preset_cmd.preset = 0xFF;
        preset_cmd.lock_flag = lock_flag? 1: 0;
        // 从0页读取
        preset_cmd.store_page = store_page;
        //preset_cmd.verify -= UdpPackageVerifyDiff((unsigned char*)&preset_cmd, sizeof(preset_cmd));

        TPackage package = {0};
        package.udp_port = UDP_PORT_READ_PRESET;
        memcpy(package.data, &preset_cmd, offsetof(D1608PresetCmd, data));
        package.data_size = offsetof(D1608PresetCmd, data);

        read_one_preset_package_list.insert(read_one_preset_package_list.begin(), package);
    }

    TPackage package = read_one_preset_package_list.back();
    SendBuffer(dst_ip, package.udp_port, package.data, package.data_size);
}
void TForm1::StartReadPreset(int preset_id)
{
    AppendLog(GetTime()+"同步Preset数据"+IntToStr(preset_id));
    read_one_preset_package_list.clear();
    for (int store_page=0;store_page<8;store_page++)
    {
        D1608PresetCmd preset_cmd(version);
        preset_cmd.preset = preset_id;
        preset_cmd.lock_flag = 0;
        // 从0页读取
        preset_cmd.store_page = store_page;
        //preset_cmd.verify -= UdpPackageVerifyDiff((unsigned char*)&preset_cmd, sizeof(preset_cmd));

        TPackage package = {0};
        package.udp_port = UDP_PORT_READ_PRESET;
        memcpy(package.data, &preset_cmd, offsetof(D1608PresetCmd, data));
        package.data_size = offsetof(D1608PresetCmd, data);

        read_one_preset_package_list.insert(read_one_preset_package_list.begin(), package);
    }

    TPackage package = read_one_preset_package_list.back();
    SendBuffer(dst_ip, package.udp_port, package.data, package.data_size);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::cbLedTestClick(TObject *Sender)
{
    String cmd_text = D1608CMD_CONTROL_FLAG;
    cmd_text = cmd_text+"config.test="+String(cbLedTest->Checked?"on":"off");
    SendCmd2(cmd_text+D1608CMD_TAIL);
}
//---------------------------------------------------------------------------

void __fastcall TForm1::lvDeviceCustomDrawItem(TCustomListView *Sender,
      TListItem *Item, TCustomDrawState State, bool &DefaultDraw)
{
    if (Item->SubItems->Strings[6] == last_device_id && !is_manual_disconnect)
    {
        lvDevice->Canvas->Font->Color = clAqua;
    }    // 大版本不一致，显示红色
    else if (Item->SubItems->Strings[8] == "YELLOW")
    {
        lvDevice->Canvas->Font->Color = clYellow;
    }
    // 次版本号不一致，显示黄色
    else if (Item->SubItems->Strings[8] != "YES")
    {
        lvDevice->Canvas->Font->Color = clRed;
    }
}
//---------------------------------------------------------------------------

static int CompLogTime(UINT a, UINT b, UINT tail, UINT size)
{
    if (a < tail) a = a+size;
    if (b < tail) b = b+size;

    return a-b;
}
void TEST_CompLogTime()
{
    assert(CompLogTime( 34,  50, 500, 1000) < 0);
    assert(CompLogTime(534, 550, 500, 1000) < 0);
    assert(CompLogTime( 34, 550, 500, 1000) > 0);
    assert(CompLogTime(534,  50, 500, 1000) < 0);
    assert(CompLogTime( 34,  34, 500, 1000) == 0);
}
void __fastcall TForm1::btnInsertUserLogClick(TObject *Sender)
{
    String cmd_text = D1608CMD_CONTROL_FLAG;
    cmd_text = cmd_text+"config.insert_log="+edtEventId->Text+","+edtEventData->Text;//+","+edtEventTimer->Text;
    SendCmd2(cmd_text+D1608CMD_TAIL);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::lvLogAdvancedCustomDrawItem(
      TCustomListView *Sender, TListItem *Item, TCustomDrawState State,
      TCustomDrawStage Stage, bool &DefaultDraw)
{
    String event_desc = Item->SubItems->Strings[1];

    if (event_desc.Pos("错误") != 0)
    {
        Sender->Canvas->Font->Color = clRed;
    }
    else if (event_desc.Pos("警告") != 0)
    {
        Sender->Canvas->Font->Color = clBlue;
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btnDisconnectClick(TObject *Sender)
{
    is_manual_disconnect = true;
    TIniFile * ini_file = new TIniFile(ExtractFilePath(Application->ExeName) + "SMC.ini");
    ini_file->WriteBool("connection", "is_disconnect", is_manual_disconnect);
    delete ini_file;

    last_device_id = "";

    // 给原先的设备发送断链消息
    SendDisconnect();

    REAL_INPUT_DSP_NUM = 16;
    REAL_OUTPUT_DSP_NUM = 16;
    need_resize = true;

    // 断开之后，不需要密码了
    memset(global_config.admin_password, 0, sizeof(global_config.admin_password));

    ClearUI();
}
//---------------------------------------------------------------------------
class TValueListEditorEx : public TValueListEditor
{
public:
    virtual void __fastcall DrawCell(int ACol, int ARow, const Types::TRect &ARect, Grids::TGridDrawState AState)
    {
        TValueListEditor::DrawCell(ACol, ARow, ARect, AState);
    }
};
void __fastcall TForm1::vleAdcMaxDrawCell(TObject *Sender, int ACol,
      int ARow, const TRect &Rect, TGridDrawState State)
{
    float data = 0;
    float range = 0;

    switch(ARow-1)
    {
    case 0 :        data = this->adc_ex_max._3_3vd       ;        range =device_setting.adc_range.vote_3v3_up;                 break;
    case 1 :        data = this->adc_ex_max.base         ;        range =device_setting.adc_range.vote_3v3m_up;                break;
    case 2 :        data = this->adc_ex_max._5vd         ;        range =device_setting.adc_range.vote_5vd_up;                 break;
    case 3 :        data = this->adc_ex_max._8vdc        ;        range =device_setting.adc_range.vote_8vdc_up;                break;
    case 4 :        data = this->adc_ex_max._8vac        ;        range =device_setting.adc_range.vote_8vac_up;                break;
    case 5 :        data = this->adc_ex_max._8vad        ;        range =device_setting.adc_range.vote_8vad_up;                break;
    case 6 :        data = this->adc_ex_max._x16vac      ;        range =device_setting.adc_range.vote_x16vac_up;              break;
    case 7 :        data = this->adc_ex_max._x16va       ;        range =device_setting.adc_range.vote_x16va_up;               break;
    case 8 :        data = this->adc_ex_max._50vpc       ;        range =device_setting.adc_range.vote_50vpc_up;               break;
    case 9 :        data = this->adc_ex_max._50vp        ;        range =device_setting.adc_range.vote_50vp_up;                break;
    case 10:        data = this->adc_ex_max._48vp        ;        range =device_setting.adc_range.vote_48vp_up;                break;
    case 11:        data = this->adc_ex_max._5va         ;        range =device_setting.adc_range.vote_5va_up;                 break;
    case 12:        data = this->adc_ex_max._x12va       ;        range =device_setting.adc_range.vote_x12va_up;               break;
    case 13:        data = this->adc_ex_max._12va        ;        range =device_setting.adc_range.vote_12va_up;                break;
    case 14:        data = this->adc_ex_max._16va        ;        range =device_setting.adc_range.vote_16va_up;                break;
    case 15:        data = this->adc_ex_max._16vac       ;        range =device_setting.adc_range.vote_16vac_up;               break;
    case 17:        data = this->adc_ex_max._8va_current ;        range =device_setting.adc_range.current_8vd_up;              break;
    case 18:        data = this->adc_ex_max._8vd_current ;        range =device_setting.adc_range.current_8vd_up;              break;
    case 19:        data = this->adc_ex_max._16v_current ;        range =device_setting.adc_range.current_16v_up;              break;
    case 20:        data = this->adc_ex_max._x16v_current;        range =device_setting.adc_range.current_x16v_up;             break;
    case 21:        data = this->adc_ex_max._50v_current ;        range =device_setting.adc_range.current_50v_up;              break;
    }

    if (data > range)
    {
        TValueListEditorEx * src_obj = ((TValueListEditorEx*)Sender);
        src_obj->OnDrawCell = NULL;
        src_obj->Canvas->Font->Color = clRed;
        src_obj->Canvas->Rectangle(Rect);
        src_obj->DrawCell(ACol, ARow, Rect, State);
        src_obj->OnDrawCell = vleAdcMaxDrawCell;
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::vleAdcMinDrawCell(TObject *Sender, int ACol,
      int ARow, const TRect &Rect, TGridDrawState State)
{
    float data = 0;
    float range = 0;

    switch(ARow-1)
    {
    case 0 :        data = this->adc_ex_min._3_3vd       ;        range =device_setting.adc_range.vote_3v3_down;                 break;
    case 1 :        data = this->adc_ex_min.base         ;        range =device_setting.adc_range.vote_3v3m_down;                break;
    case 2 :        data = this->adc_ex_min._5vd         ;        range =device_setting.adc_range.vote_5vd_down;                 break;
    case 3 :        data = this->adc_ex_min._8vdc        ;        range =device_setting.adc_range.vote_8vdc_down;                break;
    case 4 :        data = this->adc_ex_min._8vac        ;        range =device_setting.adc_range.vote_8vac_down;                break;
    case 5 :        data = this->adc_ex_min._8vad        ;        range =device_setting.adc_range.vote_8vad_down;                break;
    case 6 :        data = this->adc_ex_min._x16vac      ;        range =device_setting.adc_range.vote_x16vac_down;              break;
    case 7 :        data = this->adc_ex_min._x16va       ;        range =device_setting.adc_range.vote_x16va_down;               break;
    case 8 :        data = this->adc_ex_min._50vpc       ;        range =device_setting.adc_range.vote_50vpc_down;               break;
    case 9 :        data = this->adc_ex_min._50vp        ;        range =device_setting.adc_range.vote_50vp_down;                break;
    case 10:        data = this->adc_ex_min._48vp        ;        range =device_setting.adc_range.vote_48vp_down;                break;
    case 11:        data = this->adc_ex_min._5va         ;        range =device_setting.adc_range.vote_5va_down;                 break;
    case 12:        data = this->adc_ex_min._x12va       ;        range =device_setting.adc_range.vote_x12va_down;               break;
    case 13:        data = this->adc_ex_min._12va        ;        range =device_setting.adc_range.vote_12va_down;                break;
    case 14:        data = this->adc_ex_min._16va        ;        range =device_setting.adc_range.vote_16va_down;                break;
    case 15:        data = this->adc_ex_min._16vac       ;        range =device_setting.adc_range.vote_16vac_down;               break;
    case 17:        data = this->adc_ex_min._8va_current ;        range =device_setting.adc_range.current_8vd_down;              break;
    case 18:        data = this->adc_ex_min._8vd_current ;        range =device_setting.adc_range.current_8vd_down;              break;
    case 19:        data = this->adc_ex_min._16v_current ;        range =device_setting.adc_range.current_16v_down;              break;
    case 20:        data = this->adc_ex_min._x16v_current;        range =device_setting.adc_range.current_x16v_down;             break;
    case 21:        data = this->adc_ex_min._50v_current ;        range =device_setting.adc_range.current_50v_down;              break;
    }

    if (data < range)
    {
        TValueListEditorEx * src_obj = ((TValueListEditorEx*)Sender);
        src_obj->OnDrawCell = NULL;
        src_obj->Canvas->Font->Color = clRed;
        src_obj->Canvas->Rectangle(Rect);
        src_obj->DrawCell(ACol, ARow, Rect, State);
        src_obj->OnDrawCell = vleAdcMinDrawCell;
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btnCopyVoteDataToClipClick(TObject *Sender)
{
    mmVote->SelectAll();
    mmVote->CopyToClipboard();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btnClearDataAndTimeClick(TObject *Sender)
{
    if (Application->MessageBox("本操作会恢复出厂设置，确认操作吗？", "确认恢复出厂设置操作", MB_OKCANCEL|MB_ICONWARNING) != IDOK)
    {
        return;
    }

    {                         
        // 出厂
        btnLeaveTheFactory->Click();
    }
    {
        String cmd_text = D1608CMD_FLAG;
        cmd_text = cmd_text+"config.action=init";   // 82
        SendCmd2(cmd_text+D1608CMD_TAIL);
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::input_panel_trackbarMouseMove(TObject *Sender,
      TShiftState Shift, int X, int Y)
{
    TAdvTrackBar * trackbar = (TAdvTrackBar*)Sender;
    trackbar->SetFocus();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btnCopyDebugLogClick(TObject *Sender)
{
    memo_debug->SelectAll();
    memo_debug->CopyToClipboard();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btnCutDebugLogClick(TObject *Sender)
{
    memo_debug->SelectAll();
    memo_debug->CutToClipboard();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::lblKeepLiveCheckDblClick(TObject *Sender)
{
    broken_count = 0;
    send_keeplive_count = 0;
    recv_keeplive_count = 0;
    slp_count = 0;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::lvLogData(TObject *Sender, TListItem *Item)
{
    if (Item->Index < log_count)
    {
        int address = (int)tail_address + LOG_SIZE - (Item->Index+1)*sizeof(Event);
        if (address >= LOG_START_PAGE+LOG_SIZE)
            address -= LOG_SIZE;
        ApplyLogData(Item, event_data[Item->Index], address, event_syn_timer[Item->Index]);
    }
    else
    {
        int mac_index = Item->Index - log_count;

        Item->Caption = "";
        Item->SubItems->Add("mac");
        String mac_string;
        mac_string.sprintf("%02X-%02X-%02X-%02X-%02X-%02X",
                            mac_data[mac_index][0], mac_data[mac_index][1], mac_data[mac_index][2],
                            mac_data[mac_index][3], mac_data[mac_index][4], mac_data[mac_index][5]);
        Item->SubItems->Add(mac_string);
        Item->SubItems->Add(InnerMacInfo(mac_string));
        Item->SubItems->Add("");
        Item->SubItems->Add("");
    }
}
//---------------------------------------------------------------------------
void TForm1::CloseControlLink(String reason)
{
    AppendLog(GetTime()+"上位机关闭连接: " + reason);
    SendDisconnect();
    udpControl->Active = false;
    broken_count++;
    admin_password_ok = false;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::output_panel_level_editEnter(TObject *Sender)
{
    TEdit * edt = (TEdit*)Sender;
    edt->SelectAll();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::edtSelectAllAndCopy(TObject *Sender)
{
    TEdit * edt = (TEdit*)Sender;
    edt->SelectAll();
    edt->CopyToClipboard();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::SpeedButtonNoFrame1Click(TObject *Sender)
{
    CloseDspDetail();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::rgLedTestClick(TObject *Sender)
{
    T_TestLedOled test_led_oled = {0};
    strcpy(test_led_oled.flag, TEST_FLAG);
    int udp_length = sizeof(test_led_oled);

    switch (rgLedTest->ItemIndex)
    {
    case 0:     // ALL ON
        test_led_oled.led_state.green = 0xFFFFFFFF;
        test_led_oled.led_state.red = 0xFFFFFFFF;
        break;
    case 1:     // ALL OFF
        test_led_oled.led_state.green = 0;
        test_led_oled.led_state.red = 0;
        break;
    case 2:     // GREEN
        test_led_oled.led_state.green = 0xFFFFFFFF;
        test_led_oled.led_state.red = 0;
        break;
    case 3:     // RED
        test_led_oled.led_state.green = 0;
        test_led_oled.led_state.red = 0xFFFFFFFF;
        break;
    case 4:     // GREEN OFF
        test_led_oled.led_state.green = 0x55555555;
        test_led_oled.led_state.red = 0;
        break;
    case 5:     // OFF GREEN
        test_led_oled.led_state.green = 0xaaaaaaaa;
        test_led_oled.led_state.red = 0;
        break;
    case 6:     // RED OFF
        test_led_oled.led_state.green = 0;
        test_led_oled.led_state.red = 0x55555555;
        break;
    case 7:     // OFF RED
        test_led_oled.led_state.green = 0;
        test_led_oled.led_state.red = 0xaaaaaaaa;
        break;
    default:    // Closed
        udp_length = 29;
        break;
    }

    for (int i=0;i<3;i++)
    {
        if (udpSLPList[i]->Active && udpSLPList[i]->Bindings->Count == 1)
            udpSLPList[i]->SendBuffer("255.255.255.255", UDP_PORT_SET_LED_OLED_DEBUG, &test_led_oled, udp_length);
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::cbUsart1ReceiveAckClick(TObject *Sender)
{
    String cmd_text = D1608CMD_CONTROL_FLAG;
    cmd_text = cmd_text+"config.usart1_ack="+String(cbUsart1ReceiveAck->Checked?"on":"off");
    SendCmd2(cmd_text+D1608CMD_TAIL);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::cbUsart3ReceiveAckClick(TObject *Sender)
{
    String cmd_text = D1608CMD_CONTROL_FLAG;
    cmd_text = cmd_text+"config.usart3_ack="+String(cbUsart3ReceiveAck->Checked?"on":"off");
    SendCmd2(cmd_text+D1608CMD_TAIL);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btnDebugInfoExClick(TObject *Sender)
{
    String cmd_text = DEBUG_FLAG;
    cmd_text = cmd_text+"probe="+edtDebufExPort->Text;
    SendCmd2(cmd_text+D1608CMD_TAIL);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btnStopDebugInfoExClick(TObject *Sender)
{
    String cmd_text = DEBUG_FLAG;
    cmd_text = cmd_text+"probe=stop";
    SendCmd2(cmd_text+D1608CMD_TAIL);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::Button2Click(TObject *Sender)
{
    memo_debug_ex->SelectAll();
    memo_debug_ex->CopyToClipboard();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::Button3Click(TObject *Sender)
{
    memo_debug_ex->SelectAll();
    memo_debug_ex->CutToClipboard();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::tmProcessReplyTimer(TObject *Sender)
{
    tmProcessReply->Enabled = false;

    //AppendLog(GetTime()+": begin tmProcessReplyTimer");

    String cmd_text = D1608CMD_REPLY_FLAG;
    cmd_text = cmd_text+"config.action.syn_msg_id="+IntToStr(pre_received_msg_id);
    SendCmd2(cmd_text+D1608CMD_TAIL);

    //AppendLog("处理reply消息: " + IntToStr(reply_msg_buf.size()));

    std::vector<UINT> cmd_id_list;
    for (UINT i=0;i<reply_msg_buf.size();i++)
    {
        ProcessPackageMessageFeedback(reply_msg_buf[i].reply, reply_msg_buf[i].count, cmd_id_list);
    }
    reply_msg_buf.clear();

    LockWindowUpdate(this->Handle);
    for (UINT cmd_index=0; cmd_index<cmd_id_list.size(); cmd_index++)
    {
        int cmd_id = cmd_id_list[cmd_index];
        if (cmd_id < 0)
        {
            ApplyConfigToUI();
            break;
        }
        else
        {
            OnFeedbackData(cmd_id);
        }
    }
    LockWindowUpdate(NULL);
    //AppendLog(GetTime()+": end tmProcessReplyTimer");
}
//---------------------------------------------------------------------------
void TForm1::AdjustOutputCompParam(int dsp_num)
{
    if (config_map.output_dsp[dsp_num-1].ratio > ratio_config.max_value)
        config_map.output_dsp[dsp_num-1].ratio = ratio_config.max_value;
    else if (config_map.output_dsp[dsp_num-1].ratio < ratio_config.min_value)
        config_map.output_dsp[dsp_num-1].ratio = ratio_config.min_value;

    if (config_map.output_dsp[dsp_num-1].threshold > threshold_config.max_value)
        config_map.output_dsp[dsp_num-1].threshold = threshold_config.max_value;
    else if (config_map.output_dsp[dsp_num-1].threshold < threshold_config.min_value)
        config_map.output_dsp[dsp_num-1].threshold = threshold_config.min_value;

    if (config_map.output_dsp[dsp_num-1].attack_time > attack_config.max_value)
        config_map.output_dsp[dsp_num-1].attack_time = attack_config.max_value;
    else if (config_map.output_dsp[dsp_num-1].attack_time < attack_config.min_value)
        config_map.output_dsp[dsp_num-1].attack_time = attack_config.min_value;

    if (config_map.output_dsp[dsp_num-1].release_time > release_config.max_value)
        config_map.output_dsp[dsp_num-1].release_time = release_config.max_value;
    else if (config_map.output_dsp[dsp_num-1].release_time < release_config.min_value)
        config_map.output_dsp[dsp_num-1].release_time = release_config.min_value;

    if (config_map.output_dsp[dsp_num-1].comp_gain > gain_config.max_value)
        config_map.output_dsp[dsp_num-1].comp_gain = gain_config.max_value;
    else if (config_map.output_dsp[dsp_num-1].comp_gain < gain_config.min_value)
        config_map.output_dsp[dsp_num-1].comp_gain = gain_config.min_value;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::input_panel_thumbPaint(TObject *Sender)
{
    TPaintBox * thumb_box = (TPaintBox*)Sender;
    int dsp_id = thumb_box->Tag;
    static FilterSet _filter_set;

    for (int band=1;band<12;band++)
    {
        _filter_set.SetBypass(band, config_map.input_dsp[dsp_id-1].filter[band-1].bypass);

        _filter_set.GetFilter(band)->name = IntToStr(band-1);
        String type_name = Coefficient::GetTypeName(config_map.input_dsp[dsp_id-1].filter[band-1].TYPE);
        _filter_set.GetFilter(band)->ChangFilterParameter(
                    type_name,
                    config_map.input_dsp[dsp_id-1].filter[band-1].FREQ / FREQ_RATE,
                    config_map.input_dsp[dsp_id-1].filter[band-1].GAIN / 10.0,
                    config_map.input_dsp[dsp_id-1].filter[band-1].Q / 100.0);
    }

    _filter_set.SetEqSwitch(config_map.input_dsp[dsp_id-1].eq_switch);
    paint_agent->PaintThumbnail(thumb_box, _filter_set);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::output_panel_thumbPaint(TObject *Sender)
{
    TPaintBox * thumb_box = (TPaintBox*)Sender;
    int dsp_id = thumb_box->Tag;
    static FilterSet _filter_set;

    for (int band=1;band<12;band++)
    {
        _filter_set.SetBypass(band, config_map.output_dsp[dsp_id-1].filter[band-1].bypass);

        _filter_set.GetFilter(band)->name = IntToStr(band-1);
        String type_name = Coefficient::GetTypeName(config_map.output_dsp[dsp_id-1].filter[band-1].TYPE);
        _filter_set.GetFilter(band)->ChangFilterParameter(
                    type_name,
                    config_map.output_dsp[dsp_id-1].filter[band-1].FREQ / FREQ_RATE,
                    config_map.output_dsp[dsp_id-1].filter[band-1].GAIN / 10.0,
                    config_map.output_dsp[dsp_id-1].filter[band-1].Q / 100.0);
    }

    _filter_set.SetEqSwitch(config_map.output_dsp[dsp_id-1].eq_switch);
    paint_agent->PaintThumbnail(thumb_box, _filter_set);

    // TODO：暂时没有压缩图示
    //_filter_set.SetCompSwitch(config_map.output_dsp[dsp_id-1].comp_switch);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::PaintBox1Click(TObject *Sender)
{
    if (filter_set.GetActiveBand() == 0)
    {
        filter_set.MoveToNextBand();
        paint_agent->Repaint();
    }

    edt4Tab->SetFocus();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::edt4TabKeyDown(TObject *Sender, WORD &Key,
      TShiftState Shift)
{
    int band = filter_set.GetActiveBand();
    switch(Key)
    {
    case VK_UP:
        if (Shift.Contains(ssShift))
            panel_agent->OnQKeyDown(band, VK_UP);
        else
            panel_agent->OnGainKeyDown(band, VK_UP);
        break;
    case VK_DOWN:
        if (Shift.Contains(ssShift))
            panel_agent->OnQKeyDown(band, VK_DOWN);
        else
            panel_agent->OnGainKeyDown(band, VK_DOWN);
        break;
    case VK_LEFT:
        panel_agent->OnFreqKey(band, VK_DOWN);
        break;
    case VK_RIGHT:
        panel_agent->OnFreqKey(band, VK_UP);
        break;
    }
    Key = 0;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::Edit2Enter(TObject *Sender)
{
    edt4Tab->SetFocus();

    // 移动到下一个band
    filter_set.MoveToNextBand();
    paint_agent->Repaint();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::Edit3Enter(TObject *Sender)
{
    edt4Tab->SetFocus();

    // 移动到前一个band
    filter_set.MoveToPrevBand();
    paint_agent->Repaint();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btnAdminPasswordClick(TObject *Sender)
{
    edtAdminPassword->Text = edtAdminPassword->Text.SubString(1, 12);
    ConfusionChar(edtAdminPassword->Text.c_str(), global_config.admin_password, D1608CMD_CONTROL_FLAG);

    String cmd_text = D1608CMD_CONTROL_FLAG;
    cmd_text = cmd_text+"config.admin_password="+String(global_config.admin_password);
    SendCmd2(cmd_text+D1608CMD_TAIL);

    ProcessTextCommand(cmd_text+D1608CMD_TAIL);

    edtAdminPassword->Text = "";
}
//---------------------------------------------------------------------------
void __fastcall TForm1::cbLockParameterClick(TObject *Sender)
{
    global_config.lock_parameter = cbLockParameter->Checked;

    String cmd_text = D1608CMD_CONTROL_FLAG;
    cmd_text = cmd_text+String("config.lock_parameter=")+(cbLockParameter->Checked?"on":"off");
    SendCmd(cmd_text+D1608CMD_TAIL);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::imgMaskMouseDown(TObject *Sender,
      TMouseButton Button, TShiftState Shift, int X, int Y)
{
    if (admin_password == "")
    {
        InputPassword->Edit1->Text = "";
        if (InputPassword->ShowModal() == mrOk)
            admin_password = InputPassword->Edit1->Text;
        else
            return;// break;
    }
    // 发送命令
    String cmd_text = D1608CMD_CONTROL_FLAG;
    cmd_text = cmd_text+ "parameter.admin="+admin_password;
    SendCmd2(cmd_text+D1608CMD_TAIL);
    SendCmd2(cmd_text);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::PaintBox5Paint(TObject *Sender)
{
    Graphics::TBitmap * bmp = new Graphics::TBitmap();
    bmp->Width = 500;
    bmp->Height = 500;
    bmp->Canvas->Brush->Color = TColor(0x4A392C);
    bmp->Canvas->FillRect(Rect(0,0,500,500));


    PaintBox5->Canvas->Draw(0,0,imgBody->Picture->Graphic);

    BLENDFUNCTION blend;
    blend.BlendOp = AC_SRC_OVER;
    blend.BlendFlags = 0;
    blend.SourceConstantAlpha = 200;
    blend.AlphaFormat = 0;

    ::AlphaBlend(PaintBox5->Canvas->Handle,
        Bevel10->Left,Bevel10->Top,Bevel10->Width,Bevel10->Height,
        bmp->Canvas->Handle, 0, 0, Bevel10->Width,Bevel10->Height, blend);

    delete bmp;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btnConfigPasswordErrorClick(TObject *Sender)
{
    if (Application->MessageBox("密码错误，重新输入吗？", "密码错误", MB_YESNO) == IDYES)
    {
        InputPassword->Edit1->Text = "";
        if (InputPassword->ShowModal() == mrOk)
        {
            char tmp_admin_password[20];
            ConfusionChar(InputPassword->Edit1->Text.c_str(), tmp_admin_password, D1608CMD_CONTROL_FLAG);

            admin_password = tmp_admin_password;
        }
        else
        {
            return;// break;
        }

        // 发送命令
        String cmd_text = D1608CMD_CONTROL_FLAG;
        cmd_text = cmd_text+ "config.admin="+admin_password;
        SendCmd2(cmd_text+D1608CMD_TAIL);
        SendCmd2(cmd_text);
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btnParameterPasswordErrorClick(TObject *Sender)
{
    if (Application->MessageBox("密码错误，重新输入吗？", "密码错误", MB_YESNO) == IDYES)
    {
        InputPassword->Edit1->Text = "";
        if (InputPassword->ShowModal() == mrOk)
        {
            char tmp_admin_password[20];
            ConfusionChar(InputPassword->Edit1->Text.c_str(), tmp_admin_password, D1608CMD_CONTROL_FLAG);

            admin_password = tmp_admin_password;
        }
        else
        {
            return;// break;
        }

        // 发送命令
        String cmd_text = D1608CMD_CONTROL_FLAG;
        cmd_text = cmd_text+ "parameter.admin="+admin_password;
        SendCmd2(cmd_text+D1608CMD_TAIL);
        SendCmd2(cmd_text);
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::Button4Click(TObject *Sender)
{
    memo_debug_ex->Clear();
    char buf[2] = "1";
    udpControl->SendBuffer(dst_ip, UDP_GET_ERROR_LOG, buf, 1);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::Button5Click(TObject *Sender)
{
    if (udpControl->Active)
        udpControl->SendBuffer(dst_ip, UDP_PORT_GET_DISPLAY_BUFFER, "1", 1);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::pbOLEDPaint(TObject *Sender)
{
    for (int page = 0; page < 4; page ++)
    {
        for (int x = 0; x < 128; x++)
        {
            unsigned char data = display_buffer[page*128+x];

            for (int y=0; y<8; y++)
            {
                pbOLED->Canvas->Pixels[x*2  ][(page*8+y)*2  ] = (data & (1<<y)) ? clAqua : 0x69241D;
                pbOLED->Canvas->Pixels[x*2  ][(page*8+y)*2+1] = (data & (1<<y)) ? clAqua : 0x69241D;
                pbOLED->Canvas->Pixels[x*2+1][(page*8+y)*2  ] = (data & (1<<y)) ? clAqua : 0x69241D;
                pbOLED->Canvas->Pixels[x*2+1][(page*8+y)*2+1] = (data & (1<<y)) ? clAqua : 0x69241D;
            }
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::cbNetworkClick(TObject *Sender)
{
    TIniFile * ini_file = new TIniFile(ExtractFilePath(Application->ExeName) + "SMC.ini");
    ini_file->WriteString("connection", "network", cbNetwork->Text);
    delete ini_file;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btn3VCloseClick(TObject *Sender)
{
    String cmd_text = DEBUG_FLAG;
    cmd_text = cmd_text+ "debug.3v=off";
    SendCmd2(cmd_text+D1608CMD_TAIL);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btn3VOpenClick(TObject *Sender)
{
    String cmd_text = DEBUG_FLAG;
    cmd_text = cmd_text+ "debug.3v=on";
    SendCmd2(cmd_text+D1608CMD_TAIL);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btn5VCloseClick(TObject *Sender)
{
    String cmd_text = DEBUG_FLAG;
    cmd_text = cmd_text+ "debug.5v==off";
    SendCmd2(cmd_text+D1608CMD_TAIL);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btn5VOpenClick(TObject *Sender)
{
    String cmd_text = DEBUG_FLAG;
    cmd_text = cmd_text+ "debug.5v=on";
    SendCmd2(cmd_text+D1608CMD_TAIL);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btn16VCloseClick(TObject *Sender)
{
    String cmd_text = DEBUG_FLAG;
    cmd_text = cmd_text+ "debug.16v==off";
    SendCmd2(cmd_text+D1608CMD_TAIL);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btn16VOpenClick(TObject *Sender)
{
    String cmd_text = DEBUG_FLAG;
    cmd_text = cmd_text+ "debug.16v=on";
    SendCmd2(cmd_text+D1608CMD_TAIL);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btn48VCloseClick(TObject *Sender)
{
    String cmd_text = DEBUG_FLAG;
    cmd_text = cmd_text+ "debug.48v==off";
    SendCmd2(cmd_text+D1608CMD_TAIL);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btn48VOpenClick(TObject *Sender)
{
    String cmd_text = DEBUG_FLAG;
    cmd_text = cmd_text+ "debug.48v=on";
    SendCmd2(cmd_text+D1608CMD_TAIL);
}
//---------------------------------------------------------------------------

