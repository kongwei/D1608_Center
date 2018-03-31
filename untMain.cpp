//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "untMain.h"
#include "untSetMAC.h"
#include "untFlashReader.h"
#include "untUtils.h"
#include "untTextCmdParse.h"

#include <winsock2.h>
#include <IPHlpApi.h>  
#include <algorithm>
#include <stddef.h>
#include <assert.h>
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

String inner_mac[6] = {"10-0B-A9-2F-55-90", "00-5A-39-FF-49-28","00-E0-4C-39-17-31","74-D0-2B-95-48-02","00-E0-4C-15-1B-C0", "38-2C-4A-BA-EF-54"};

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
static bool global_config_loaded = false;
int REAL_INPUT_DSP_NUM = 16;
int REAL_OUTPUT_DSP_NUM = 16;

static vector<int> loose_msg_id;

extern String GetMacList();

// ���ڸ����豸��flaah����
struct SmcConfig
{
    GlobalConfig global_config;
    UINT file_version;
    UINT device_version;
    UINT cpu_id[3];        // cpu_id λ�ձ�ʾ���ѻ��ļ�
    unsigned char mac[6];
    char pad1[2048-sizeof(GlobalConfig) - sizeof(UINT)-sizeof(UINT)-sizeof(UINT)*3-6];
    char device_flash_dump[128*1024];
};
static SmcConfig smc_config;
bool IsDeviceFlashEmpty()
{
    // �ж��Ƿ����߱���
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

struct DeviceData
{
    int count;
    T_slp_pack_Ex data;
};
DeviceData last_connection;

static unsigned int received_cmd_seq = 0;
//------------------------------------------------
// �汾������Ϣ
static UINT version = 0x04000001;
static UINT file_version = 0x00000002;
// ����YES������λ���汾��
// version_list��0��β
String IsCompatibility(T_slp_pack_Ex slp_pack)
{
    UINT ctrl_app_protocol_version = version & 0xFF000000;
    UINT app_protocol_version = slp_pack.version & 0xFF000000;
    if (ctrl_app_protocol_version == app_protocol_version)
    {
        ctrl_app_protocol_version = version & 0xFFFF0000;
        app_protocol_version = slp_pack.version & 0xFFFF0000;
        if (ctrl_app_protocol_version == app_protocol_version)
        {
            return "YES";
        }
        return "YELLOW";
    }
    else
    {
        return IntToHex((int)slp_pack.version, 8);
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
    // ����n�����־���
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

CompConfig ratio_config = {0, 100, 100, 100, 2};
CompConfig threshold_config = {-320, 0, 0, 10, 3};
CompConfig attack_config = {1, 20000, 640, 10, 3};
CompConfig release_config = {10, 50000, 10000, 10, 3};
CompConfig gain_config = {0, 240, 0, 10, 3};
VersionFunction TForm1::GetVersionConfig()
{
    if (global_config_loaded && String(global_config.version_function.name) == "")
    {
        ShowMessage("��ȡ���ñ�ʧ��");
        keep_live_count = CONTROL_TIMEOUT_COUNT;
    }
    return global_config.version_function;
}

String Ration2String(double ratio)
{
    if (ratio == 0)
        return "��";
    else
        return FormatFloat(100.0 / ratio, ratio_config.precise);
}
//---------------------------------------------------------------------------
static String GetTextCmdVar(String cmd_text)
{
    // �Ƚ� VarName ����
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
//������ ��IP��ַд�뵽�б� WinSock
void GetLocalIpListEx(vector<IpInfo> & ip_info)
{
    ip_info.clear();

    String szMark;

    PIP_ADAPTER_INFO pAdapterInfo = NULL;
    PIP_ADAPTER_INFO pAdapter = NULL;   
  
    pAdapterInfo = ( IP_ADAPTER_INFO * ) malloc( sizeof( IP_ADAPTER_INFO ) );  
    ULONG ulOutBufLen;   
    ulOutBufLen = sizeof(IP_ADAPTER_INFO);   
  
    // ��һ�ε���GetAdapterInfo��ȡulOutBufLen��С   
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
                if (ip.ip != "0.0.0.0" && ip.mask != INADDR_NONE)
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
static TSpeedButton * CopyInputPanelButton(TSpeedButton * src_btn, int dsp_id, Graphics::TBitmap* bmp=NULL)
{
    TSpeedButton * dsp_btn = new TSpeedButtonNoFrame(src_btn->Owner);
    dsp_btn->Caption = src_btn->Caption;
    dsp_btn->BoundsRect = src_btn->BoundsRect;
    dsp_btn->Left = src_btn->Left + (dsp_id-1) * PANEL_WIDTH;
    dsp_btn->AllowAllUp = src_btn->AllowAllUp;
    dsp_btn->Flat = src_btn->Flat;
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
    dsp_btn->GroupIndex = (int)dsp_btn; // ���а�ť����Ӱ��

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
}
static void CopyWatchPanel(int panel_id, TForm1 * form, String label, int left)
{
    TPanel * watch_panel = new TPanel(form->pnlOperator);
    watch_panel->SetBounds(left, form->watch_panel->Top, form->watch_panel->Width, form->watch_panel->Height);
    watch_panel->BevelOuter = form->watch_panel->BevelOuter;
    watch_panel->Parent = form->pnlOperator;
    watch_panel->Color = form->watch_panel->Color;
    form->watch_panel_inner[panel_id-1] = watch_panel;

    TImage * bk_image = new TImage(watch_panel);
    bk_image->BoundsRect = form->imgWatch->BoundsRect;
    bk_image->Parent = watch_panel;
    Graphics::TBitmap * tmp_bmp = new Graphics::TBitmap();
    bk_image->Picture->Bitmap = tmp_bmp;
    bk_image->Picture->Bitmap->Height = bk_image->Height;
    bk_image->Picture->Bitmap->Width = bk_image->Width;
    bk_image->Canvas->Draw(-watch_panel->Left, -watch_panel->Top, form->imgBody->Picture->Graphic);
    delete tmp_bmp;

    ::AlphaBlend(bk_image->Canvas->Handle,
        0,0,bk_image->Width,bk_image->Height,
        form->imgWatch->Canvas->Handle, 0,0,bk_image->Width,bk_image->Height, blend);

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
static TSpeedButton * CopyPnlMixButton(TSpeedButton * src_btn, int dsp_id, Graphics::TBitmap* bmp=NULL)
{
    TSpeedButton * dsp_btn = new TSpeedButtonNoFrame(src_btn->Owner);
    dsp_btn->Caption = src_btn->Caption;
    dsp_btn->BoundsRect = src_btn->BoundsRect;
    dsp_btn->Left = src_btn->Left + (dsp_id-1) * PANEL_WIDTH;
    dsp_btn->AllowAllUp = src_btn->AllowAllUp;
    dsp_btn->GroupIndex = src_btn->GroupIndex;
    dsp_btn->Flat = src_btn->Flat;
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
    dsp_btn->GroupIndex = (int)dsp_btn; // ���а�ť����Ӱ��

    return dsp_btn;
}
/*static TAdvTrackBar * CopyPnlMixTrackbar(TAdvTrackBar * src_trackbar, int dsp_id)
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
}*/
/*static TEdit * CopyPnlMixEdit(TEdit * src_edit, int dsp_id)
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
}*/
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
    startup_time = Now();

    // ��������
    on_loading = true;

    //x=GetSystemMetrics(SM_CXSCREEN)
    //y=GetSystemMetrics(SM_CYSCREEN)

    // �����ߴ�
    Width = 1664;//1366;
    Height = 778;//798;

    for (int i=0;i<32;i++)
    {
        level_meter[i][0] = -49;
        level_meter[i][1] = -100;
    }

    pnlOperator->Width = REAL_INPUT_DSP_NUM * PANEL_WIDTH + imgPresetBg->Width + REAL_OUTPUT_DSP_NUM * PANEL_WIDTH;
    pnlOperator->Width = Math::Max(pnlOperator->Width, Width-16);
        //HorzScrollBar->Visible = (pnlOperator->Width > Width);
    pnlOperator->Height = 650;//-(728-584);
    pnlOperator->Top = pnlHeader->Height;

    pnlMix->Width = REAL_INPUT_DSP_NUM * PANEL_WIDTH;                               

    pb_watch_list[0] = pb_watch;

    //----------------------------------------
    // ����
    int input_panel_left = (pnlOperator->Width - (REAL_INPUT_DSP_NUM * PANEL_WIDTH + imgPresetBg->Width + REAL_OUTPUT_DSP_NUM * PANEL_WIDTH))/2;
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

    input_panel_bkground->Picture->Bitmap->Width = REAL_INPUT_DSP_NUM * PANEL_WIDTH;
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

    // ����InputPanel
    input_type_lbl[0] = input_type;
    input_eq_btn[0] = input_panel_eq_btn;
    //input_comp_btn[0] = input_panel_comp_btn;
    //input_auto_btn[0] = input_panel_auto_btn;
    //input_default_btn[0] = input_panel_default_btn;
    input_invert_btn[0] = input_panel_invert_btn;
    input_noise_btn[0] = input_panel_noise_btn;
    input_mute_btn[0] = input_panel_mute_btn;
    input_dsp_name[0] = input_panel_dsp_num;
    for (int i=2;i<=INPUT_DSP_NUM;i++)
    {
        CreateInputPanel(i, this);
        CopyWatchPanel(i, this, String((char)('A'-1+i))+" ("+IntToStr(i)+")", (i-1) * PANEL_WIDTH + input_panel_left);
    }
    CopyWatchPanel(1, this, String((char)('A'-1+1))+" ("+IntToStr(1)+")", (1-1) * PANEL_WIDTH + input_panel_left);


    //------------------------------------
    // mix��master
    int mix_panel_left = input_panel_left + REAL_INPUT_DSP_NUM * PANEL_WIDTH;
    imgMasterMixBg->Left = mix_panel_left;
    imgPresetBg->Left = mix_panel_left;
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
    // ���
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

    output_panel_bkground->Width = REAL_OUTPUT_DSP_NUM * PANEL_WIDTH;
    output_panel_bkground->Picture->Bitmap->Width = output_panel_bkground->Width;
    output_panel_bkground->Canvas->Draw(
        -output_panel_bkground->Left,
        -output_panel_bkground->Top,
        imgBody->Picture->Graphic);

    {
        // ����WatchLevel�Ĳ���
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
    for (int i=2;i<=REAL_OUTPUT_DSP_NUM;i++)
    {
        CreateOutputPanel(i, this);
    }

    //----------------------------------
    // ����pnlmix����
    pnlmix_background->Picture->Bitmap->Width = REAL_INPUT_DSP_NUM * PANEL_WIDTH;  // xDO: ԭ����17��������automix������ֻ������������
    for (int i=1;i<REAL_INPUT_DSP_NUM;i++)   // xDO: ԭ����17��������automix������ֻ������������
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

    // ����PnlMix
    mix_mute_btn[0] = pnlmix_mute;
    for (int i=2;i<=REAL_INPUT_DSP_NUM;i++)    // xDO: ԭ����17��������automix������ֻ������������
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

    // DSP��ϸ���ý��������
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
    // DSP��ϸ���ý���
    panel_agent = new PanelAgent(filter_set);
    paint_agent = new PaintAgent(PaintBox1, pbComp, filter_set);
    filter_set.Register(paint_agent, panel_agent);

    pnlOperator->DoubleBuffered = true;
    pnlDspDetail->DoubleBuffered = true;
    pnlHeader->DoubleBuffered = true;
    pnlMonitor->DoubleBuffered = true;

    // �ܵ��˲�������Ӱ��
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

    // ˽�б�����ʼ��
    keep_live_count = CONTROL_TIMEOUT_COUNT;
    received_cmd_seq = 0;
    device_connected = false;

    // ��ȡ����
    TIniFile * ini_file = new TIniFile(ExtractFilePath(Application->ExeName) + "SMC.ini");
    last_device_id = ini_file->ReadString("connection", "last_id", "");
    is_manual_disconnect = ini_file->ReadBool("connection", "is_disconnect", false);
    delete ini_file;

    // ��Ҫ��ʼ����ɺ�ſ���SLP�������Զ����Ӻ�������������棬����Input��廹δ��ʼ��ʱ���������������ͻ�����쳣��
    udpSLPList[0] = udpSLP;
    udpSLPList[1] = udpSLP1;
    udpSLPList[2] = udpSLP2;
    tmSLP->Enabled = true;

    int base_order = 0;
    // ���� TabOrder
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
    // ��ԭ�ȵ��豸���Ͷ�����Ϣ
    SendDisconnect();

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
    // ����ҳ�Ľ������
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

    // �ƶ�6��panel
    pnlOperator->Parent = this;
    pnlMonitor->Parent = this;
    pnlSystem->Parent = this;
    pnlMist->Parent = this;
    PageControl1->Hide();

    TInitCommonControlsEx ICC;
    ICC.dwSize = sizeof(TInitCommonControlsEx);
    ICC.dwICC  = ICC_INTERNET_CLASSES;
    if(!InitCommonControlsEx(&ICC))
    {
        ShowMessage("IP��ַ�ؼ�ʧЧ");
    }
    hIpEdit = CreateWindow(WC_IPADDRESS,"",WS_CHILD|WS_VISIBLE,
        edtIp->Left, edtIp->Top, edtIp->Width, edtIp->Height, edtIp->Parent->Handle,
        0,HInstance,NULL);

    old_pnlSystem_proc = (WNDPROC)GetWindowLong(hIpEdit, GWL_WNDPROC);
    SetWindowLong(hIpEdit, GWL_WNDPROC, (long)new_pnlSystem_proc);

    //old_pnlSystem_proc = pnlSystem->WindowProc;
    //pnlSystem->WindowProc = new_pnlSystem_proc;

    local_mac_list = GetMacList();

    // �Ƿ����PC
    is_inner_pc = false;
    if (ParamCount>0 && ParamStr(1)=="nodebug")
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
        // ��࿪4��
        HANDLE h_sem = CreateSemaphore(NULL, 4, 4, "MATRIX");
        if (WaitForSingleObject(h_sem, 1) != WAIT_OBJECT_0)
        {
            ShowMessage("���ֻ�ܴ� 4 ����λ��");
            Application->Terminate();
        }
    }

    pnlOperator->Show();

    SetWindowLong(edtPreset->Handle, GWL_STYLE, GetWindowLong(edtPreset->Handle, GWL_STYLE) | ES_CENTER);

    // ��������
    HRSRC hRsrc = FindResource(NULL, "DIGIFAW", RT_RCDATA);
    DWORD cbSize = SizeofResource(NULL, hRsrc);
    HGLOBAL hMem = LoadResource(NULL, hRsrc);
    LPVOID pvData = LockResource(hMem);
    DWORD nFontsInstalled = 0;
    HANDLE hFontInstalled = AddFontMemResourceEx(pvData, cbSize, NULL, &nFontsInstalled);
    if (hFontInstalled == NULL)
    {
    }

    // ����ṹ���С
    memo_debug->Lines->Add(GetTime()+"InputConfigMap:" + IntToStr(sizeof(InputConfigMap)));
    memo_debug->Lines->Add(GetTime()+"OutputConfigMap:" + IntToStr(sizeof(OutputConfigMap)));
    //memo_debug->Lines->Add(GetTime()+"MasterMixConfigMap:" + IntToStr(sizeof(MasterMixConfigMap)));
    memo_debug->Lines->Add(GetTime()+"ConfigMap:" + IntToStr(sizeof(ConfigMap)));
    //memo_debug->Lines->Add(GetTime()+"mix_mute:" + IntToStr(sizeof(config_map.master_mix.mix_mute)));
    memo_debug->Lines->Add(GetTime()+"NotStorageCmd:" + IntToStr(sizeof(NotStorageCmd)));
    memo_debug->Lines->Add(GetTime()+"GlobalConfig:" + IntToStr(sizeof(GlobalConfig)));     

    // ����������ʼ��������
    // Panel->Tag
    // Panel->Left
    // Label->Caption

    //tsSearch->Show();
    this->Repaint();

    pnlMix->BringToFront();

    // ����LOGOͼƬ
    if (FileExists("logo.bmp"))
    {
        imgLogo->Picture->LoadFromFile("logo.bmp");
        // Height -- 3, 73px  -- ���� 38
        // imgLogo->Height/2  <-- 38
        imgLogo->Top = 38 - imgLogo->Height/2;
    }

    // ��λ��ʱ��
    edtStartBuildTime->Text = " \t \t";
    edtStartBuildTime->Text = edtStartBuildTime->Text + DateTime2Str(GetDateTimeFromMarco(compile_time));

    // �汾��
    lblVersion->Caption = "-------- " +VersionToStr(version);
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

    // ���⴦��
    try
    {
        udpControl->Send(dst_ip, UDP_PORT_CONTROL, cmd);
    }
    catch(...)
    {
        CloseControlLink("������Ϣʧ��, SendCmd2");
    }
    
// TODO: �����޸��� return ֮����һ��ʧЧ��
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
    // �����ڶ�����
    if (sendcmd_list_length == 1)
    {
        memo_debug->Lines->Add(GetTime()+"����UDP��Ϣ");
        try
        {
            SendBuffer(dst_ip, udp_port, buff, size);
        }
        catch(...)
        {
            CloseControlLink("������Ϣʧ��, SendLogBuff");
        }
        sendcmd_delay_count = 15;
    }
}
void TForm1::SendCmd(String cmd)
{
    // TODO: �޸ĳ�����
    //edtCmdId->Text = cmd.id;

    //if (!udpControl->Active)
    //{
    //    edtCmdId->Text = edtCmdId->Text + "*";
    //    return;
    //}

    if (on_loading || !udpControl->Active || keep_live_count >= CONTROL_TIMEOUT_COUNT)
    {
        return;
    }
    if (!device_connected && !IsKeepliveCmd(cmd.c_str()))
    {
        return;
    }

    // ��Ҫ���͵�����id�Ƿ��ڶ�����
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

    // �����ڶ�����
    sendcmd_list.insert(sendcmd_list.begin(), package);

    int sendcmd_list_length = sendcmd_list.size();
    // ���ԭ�ȶ����ǿգ���ô����һ�η�����Ϣ
    if (sendcmd_list_length == 1)
    {
        memo_debug->Lines->Add(GetTime()+"������Ϣ:"+cmd);
        SendCmd2(cmd);
        sendcmd_delay_count = 15;
    }
    else
    {
        TPackage package = sendcmd_list.back();
        char * p_cmd = package.data;

        memo_debug->Lines->Add(GetTime()+"׷�ӵ�����"+cmd+"|"+"    "+String(p_cmd));
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btnRefreshClick(TObject *Sender)
{
    //AppendLog("ˢ���豸");

    // ɾ���ϴ��Ѿ�δɨ�赽����Ŀ
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


    
    bool is_ip_changed = false;
    UINT ip_count = 0;
    for (int i=0;i<3;i++)
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
                udpSLPList[i]->Bindings->Add();
                udpSLPList[i]->Bindings->Items[0]->IP = ip_info[i].ip;
                udpSLPList[i]->Bindings->Items[0]->Port = 0;
                udpSLPList[i]->Active = true;
            }
            catch(...)
            {
                udpSLPList[i]->Active = false;
            }
        }
    }

    if ((lvDevice->Selected != NULL) && (lvDevice->Selected->SubItems->Strings[6] == last_device_id))
    {
        if (lvDevice->Selected->SubItems->Strings[3] != "")
            lblDeviceName->Caption = lvDevice->Selected->SubItems->Strings[3];
        else
        {
            lblDeviceName->Caption = lvDevice->Selected->SubItems->Strings[4]+"-"+lvDevice->Selected->SubItems->Strings[7];
            //edtDeviceFullName_data = lvDevice->Selected->SubItems->Strings[4]+"-"+lvDevice->Selected->SubItems->Strings[7];
            if (lblDeviceName->Caption.Length() > 16)
                lblDeviceName->Caption = lvDevice->Selected->SubItems->Strings[4];
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::udpSLPUDPRead(TObject *Sender,
      TStream *AData, TIdSocketHandle *ABinding)
{
    T_slp_pack_Ex slp_pack = {0};
    AData->ReadBuffer(&slp_pack, std::min(sizeof(slp_pack), AData->Size));

    String ip_address;
        ip_address.sprintf("%u.%u.%u.%u", (BYTE)slp_pack.ip[0], (BYTE)slp_pack.ip[1], (BYTE)slp_pack.ip[2], (BYTE)slp_pack.ip[3]);
    String mac;
        mac.sprintf("%02X:%02X:%02X:%02X:%02X:%02X", (BYTE)slp_pack.mac[0], (BYTE)slp_pack.mac[1], (BYTE)slp_pack.mac[2], (BYTE)slp_pack.mac[3], (BYTE)slp_pack.mac[4], (BYTE)slp_pack.mac[5]);
    String display_device_name = slp_pack.name;
    if (display_device_name == "")
    {
        display_device_name = slp_pack.default_device_name;
        display_device_name = display_device_name + "-" + slp_pack.sn;
    }

    String cpu_id = GetCpuIdString(slp_pack.cpu_id);

    TListItem * item = NULL;
    // �����Ƿ��б����Ѿ�����
    for (int i=0;i<lvDevice->Items->Count;i++)
    {
        TListItem * find_item = lvDevice->Items->Item[i];
        if (find_item->SubItems->Strings[6] == mac)
        {
            // ��������
            find_item->Caption = display_device_name.UpperCase();
            find_item->SubItems->Strings[0] = ip_address;
            find_item->SubItems->Strings[1] = ABinding->IP;
            //find_item->SubItems->Strings[2] = device_id;
            find_item->SubItems->Strings[3] = slp_pack.name;
            find_item->SubItems->Strings[4] = slp_pack.default_device_name;
            find_item->SubItems->Strings[5] = cpu_id;
            find_item->SubItems->Strings[6] = mac;
            find_item->SubItems->Strings[7] = slp_pack.sn;
            find_item->SubItems->Strings[8] = IsCompatibility(slp_pack);

            DeviceData * data = (DeviceData*)find_item->Data;
            data->count = 3;
            data->data = slp_pack;

            item = find_item;
            break;
        }
    }

    if (item == NULL)
    {
        item = lvDevice->Items->Add();
        item->Caption = display_device_name.UpperCase();
        item->SubItems->Add(ip_address);
        item->SubItems->Add(ABinding->IP);
        item->SubItems->Add(""/*slp_pack.id*/);
        item->SubItems->Add(slp_pack.name);
        item->SubItems->Add(slp_pack.default_device_name);
        item->SubItems->Add(cpu_id);
        item->SubItems->Add(mac);
        item->SubItems->Add(slp_pack.sn);
        item->SubItems->Add(IsCompatibility(slp_pack));

        DeviceData * data = new DeviceData;
        data->count = 4;
        data->data = slp_pack;
        item->Data = data;
    }

    if (!udpControl->Active && !is_manual_disconnect)
    {
        if ((last_device_id == "") || (last_device_id == mac) || ( Now() > startup_time+10.0/3600/24 ))
        {
            // ���ӵ�һ�� ���� ƥ��ID ���� 5���� ���� û���ҵ�ԭ�ȵ��豸
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

    // �Ƿ�汾����
    if (selected->SubItems->Strings[8] != "YES" && selected->SubItems->Strings[8] != "YELLOW")
    {
        if (Sender != NULL)
            ShowMessage("�汾�����ݣ�����������\n��λ���汾��"+IntToHex((int)version,8)+"����λ���汾��"+selected->SubItems->Strings[8]);
        return;
    }

    // ��ԭ�ȵ��豸���Ͷ�����Ϣ
    SendDisconnect();

    is_manual_disconnect = false;

    dst_ip = selected->SubItems->Strings[0];
    String broadcast_ip = selected->SubItems->Strings[1];
    device_cpuid = selected->SubItems->Strings[5];
        lvLog->Clear();

    sendcmd_list.clear();
    // ��ʼ��socket
    CloseControlLink("׼�����ӵ��µ���λ��, ��������");
    static int STATIC_PORT = 0;
    udpControl->Bindings->Clear();
    udpControl->Bindings->Add();
    udpControl->Bindings->Items[0]->IP = broadcast_ip;
    udpControl->Bindings->Items[0]->Port = STATIC_PORT;
    udpControl->Active = true;
    STATIC_PORT = udpControl->Bindings->Items[0]->Port;

    lblCtrlPort->Caption = "�˿ں�: "+IntToStr(udpControl->Bindings->Items[0]->Port);

    UpdateCaption();

    StartReadCurrentPreset();
    restor_delay_count = 15;
    tmDelayBackup->Enabled = true;

    // ��ȡ��λ��PRESET���ݣ�ͬʱ�����Ϣ��Ϊ��λ���Ͽɵ���Ϣ
    TPackage package = {0};
    package.udp_port = UDP_PORT_READ_PRESET;
    D1608PresetCmd preset_cmd(version);
    strcpy(preset_cmd.flag, D1608PRESETCMD_LINK_FLAG);
    preset_cmd.preset = 0; // ��ȡglobal_config
    memcpy(package.data, &preset_cmd, offsetof(D1608PresetCmd, data));
    package.data_size = offsetof(D1608PresetCmd, data);
    read_one_preset_package_list.insert(read_one_preset_package_list.begin(), package);
    if (read_one_preset_package_list.size() == 1)
        SendBuffer(dst_ip, package.udp_port, package.data, package.data_size);

    // ������λ��ʱ��, ��λ100ms, ��2000��1��1�տ�ʼ����
#if 0
    double diffx = TDate(2099,1,1)-TDate(2000,1,1);
    diffx = diffx * 24 * 3600 * 10;
    assert(diffx < 0x7FFFFFFFFFFFFFFF);
#endif

    pnlOperator->Show();
    pnlDspDetail->Hide();
    pnlMix->Hide();
    //cbWatch->Down = true;

    keep_live_count = 0;
    global_config_loaded = false;

    last_device_id = selected->SubItems->Strings[6];
    // ��¼�������ļ���
    TIniFile * ini_file = new TIniFile(ExtractFilePath(Application->ExeName) + "SMC.ini");
    ini_file->WriteString("connection", "last_id", last_device_id);
    ini_file->WriteBool("connection", "is_disconnect", is_manual_disconnect);
    delete ini_file;

    lblDeviceName->Hide();

    edtMAC->Text = selected->SubItems->Strings[6];

    ClearUI();   
    edtDeviceFullName_data = selected->SubItems->Strings[4]+"-"+lvDevice->Selected->SubItems->Strings[7];
    tmWatch->Enabled = false;

    // �������л�ȡ�汾��Ϣ
    last_connection = *(DeviceData*)selected->Data;
    lblDeviceInfo->Caption = "VERSION " + VersionToStr(last_connection.data.version);
    lblDeviceInfo->Hide();

    btnGetLog->Enabled = true;

    lblCpuId->Caption = "cpu id: "+device_cpuid;
    lblSn->Caption = "sn: " + String(last_connection.data.sn);
    lblConfigFilename->Caption = "file: --";
}
//---------------------------------------------------------------------------
void __fastcall TForm1::tmSLPTimer(TObject *Sender)
{
    tmSLP->Enabled = false;

    btnRefresh->Click();
    if (ip_info.size() > 0)
    {
        for (int i=0;i<3;i++)
        {
            if (udpSLPList[i]->Bindings->Count == 1)
            {
                try{
                    udpSLPList[i]->Active = true;
                    String ip = udpSLPList[i]->Bindings->Items[0]->IP;
                    T_slp_pack_Ex slp_pack = {0};
                    strcpy(slp_pack.flag, SLP_FLAG);

                    if (is_inner_pc)
                        memcpy(slp_pack.ip, "ver", 3);
                    else
                        memcpy(slp_pack.ip, "rep", 3);

                    if (GetDhcpOfIp(ip))
                        slp_pack.ip[3] = '_';

                    slp_pack.mask = GetMaskOfIp(ip);

                    if (cbLanDebugLed->State == cbGrayed)
                        slp_pack.led_debug = 0xFF;
                    else
                        slp_pack.led_debug = cbLanDebugLed->Checked;

                    if (cbLanDebugOled->State == cbGrayed)
                        slp_pack.oled_debug = 0xFF;
                    else
                        slp_pack.oled_debug = cbLanDebugOled->Checked;

                    double app_time = Now()-TDateTime(2000,1,1);
                    app_time = app_time * 24 * 3600 * 1000;
                    slp_pack.set_time_ex = app_time;

                    slp_pack.verify -= UdpPackageVerifyDiff((unsigned char*)&slp_pack, offsetof(T_slp_pack_Ex, name));
                    udpSLPList[i]->SendBuffer("255.255.255.255", UDP_PORT_SLP_EX, &slp_pack, offsetof(T_slp_pack_Ex, name));
                }
                catch(...)
                {
                    //AppendLog("�����쳣");
                }
            }
        }
    }

    tmSLP->Enabled = true;
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
        // default ȱ��ȡ������ default
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
}
//---------------------------------------------------------------------------
String IntOrZeroSring(int value)
{
    return String((value>0?value:0));
}
void TForm1::ProcessPackageMessageFeedback(char * data)
{
    TextSynMsg text_syn_msg[RECORD_TEXT_MSG_SIZE] = {0};
    for (int i=0;i<RECORD_TEXT_MSG_SIZE;i++)
    {
        memcpy(&text_syn_msg[i].msg_id, data, 4);
        data += 4;
        // �ҵ� ]
        char * right = strstr(data, "]");
        if (right != NULL)
        {
            memcpy(&text_syn_msg[i].text_cmd, data, right-data+1);
            data = right + 1;
        }
        else
        {
            break;
        }
    }

    unsigned int oldest_msg_id = text_syn_msg[0].msg_id;
    unsigned int current_msg_id = text_syn_msg[0].msg_id;

    for (int i=0;i<RECORD_TEXT_MSG_SIZE;i++)
    {
        oldest_msg_id = min(oldest_msg_id, text_syn_msg[i].msg_id);
        current_msg_id = max(current_msg_id, text_syn_msg[i].msg_id);
    }
    // ʧ���ж�
    if (received_cmd_seq < oldest_msg_id && (oldest_msg_id-received_cmd_seq>1))
    {
        memo_debug->Lines->Add(GetTime()+"ʧ��: local="+IntToStr(received_cmd_seq)+", device="+IntToStr(oldest_msg_id)+"-"+IntToStr(current_msg_id));
        keep_live_count = CONTROL_TIMEOUT_COUNT;
    }
    else if (received_cmd_seq >= current_msg_id)
    {
        memo_debug->Lines->Add(GetTime()+"syn do not need: device_msg_id="+IntToStr(current_msg_id)+", pc__msg_id="+IntToStr(received_cmd_seq));
    }
    else
    {
        memo_debug->Lines->Add(GetTime()+"ͬ��: local="+IntToStr(received_cmd_seq)+", device="+IntToStr(oldest_msg_id)+"-"+IntToStr(current_msg_id));

        for (int i=0;i<RECORD_TEXT_MSG_SIZE;i++)
        {
            if (text_syn_msg[i].msg_id > received_cmd_seq)
            {
                // ��ֹ�Լ���Ӧ
                if (sendcmd_list.size() > 0)
                {
                    TPackage package = sendcmd_list.back();
                    String package_cmd = (char*)package.data;
                    if (GetTextCmdVar(text_syn_msg[i].text_cmd) == GetTextCmdVar(package_cmd))
                        continue;
                }

                // ����ת����
                // ����
                String bare_cmd = text_syn_msg[i].text_cmd;
                bare_cmd = bare_cmd.SubString(2, bare_cmd.Length());
                String full_cmd = String("[NJLS_SMC|parameter|") + bare_cmd;
                std::vector<UINT> cmd_id_list = ProcessTextCommand(full_cmd);
                for (UINT cmd_index=0; cmd_index<cmd_id_list.size(); cmd_index++)
                {
                    int cmd_id = cmd_id_list[cmd_index];
                    OnFeedbackData(cmd_id);
                }
            }
        }
    }
    received_cmd_seq = current_msg_id;

    if (keep_live_count<CONTROL_TIMEOUT_COUNT)
        keep_live_count = 0;

    if ( (double)(Now() - last_keeplive_time) > (1.0f/24/60/60) )
    {
        String cmd_text = D1608CMD_KEEPLIVE_FLAG;
        cmd_text = cmd_text+"config.action.keeplive="+IntToStr(received_cmd_seq);
        SendCmd2(cmd_text+D1608CMD_TAIL);
        send_keeplive_count++;
        last_keeplive_time = Now();
        memo_debug->Lines->Add(GetTime()+"���ͱ���: " + IntToStr(keep_live_count));
    }
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
	// �ж�ͷ��
	if (strncmp(str, D1608CMD_FLAG, strlen(D1608CMD_FLAG)) == 0)
		return true;
	if (strncmp(str, D1608CMD_CONTROL_FLAG, strlen(D1608CMD_CONTROL_FLAG)) == 0)
		return true;

    return true;
}
#endif
char * TextCmdPtr(char * str)
{
	// �ж�ͷ��
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
    if (ABinding->PeerPort == UDP_PORT_GET_DEBUG_INFO_EX)
    {
        T_Debug debug_cmd;
        AData->ReadBuffer(&debug_cmd, std::min(sizeof(debug_cmd), AData->Size));

        String windows_string = debug_cmd.debug_cmd;
        int len = windows_string.Length();        
        if (windows_string[len] == '\n')
        {
            windows_string.SetLength(len-1);
        }

        if (memo_debug_ex->Lines->Count == 0)
        {
            windows_string = StringReplace(windows_string, "\n", "\r\n", TReplaceFlags()<<rfReplaceAll<<rfIgnoreCase);
        }
        else
        {
            int index = windows_string.Pos("\n");
            windows_string = windows_string.SubString(index+1, windows_string.Length());
        }

        memo_debug_ex->Lines->Add(windows_string);
    }
    else if (ABinding->PeerPort == UDP_PORT_CONTROL)
    {
        char cmd_text[1500] = {0};
        AData->ReadBuffer(cmd_text, std::min(sizeof(cmd_text), AData->Size));

        if (IsReplyCmd(cmd_text))
        {
            // û�г�ʼ����ϣ����������ݻر�
            if (received_cmd_seq != 0)
            {
                ProcessPackageMessageFeedback(cmd_text+strlen(D1608CMD_REPLY_FLAG));
            }
        }
        else if (IsKeepliveCmd(cmd_text) && AData->Size==(strlen(D1608CMD_KEEPLIVE_FLAG)+sizeof(NotStorageCmd)))
        {
            static int last_diff = 0;
            recv_keeplive_count++;
            lblKeepLiveCheck->Caption = "������"+IntToStr(send_keeplive_count)+" ���գ�"+IntToStr(recv_keeplive_count)+" ��ֵ��"+IntToStr(send_keeplive_count-recv_keeplive_count)+" �Ͽ���·����:"+IntToStr(broken_count);
            if (send_keeplive_count-recv_keeplive_count != last_diff)
            {
                memo_debug->Lines->Add(GetTime()+"������Ų�ֵ�仯:"+IntToStr(last_diff)+" -> "+IntToStr(send_keeplive_count-recv_keeplive_count));
                last_diff = send_keeplive_count-recv_keeplive_count;
            }

            if (keep_live_count<CONTROL_TIMEOUT_COUNT)
                keep_live_count = 0;

            NotStorageCmd *keep_alive = (NotStorageCmd*)(cmd_text+strlen(D1608CMD_KEEPLIVE_FLAG));
            if (received_cmd_seq < (UINT)keep_alive->noop)    // noop������cmd_seq ԭ���� !=
            {
                memo_debug->Lines->Add(GetTime()+"ͬ����Ϣ�е����: local="+IntToStr(received_cmd_seq)+", device="+IntToStr(keep_alive->noop));
                received_cmd_seq = (UINT)keep_alive->noop;
            }

            ProcessWatchLevel(keep_alive->watch_level, keep_alive->watch_level_comp);
            ProcessVote(keep_alive->adc_ex, keep_alive->adc_ex_max, keep_alive->adc_ex_min);
            ProcessKeepAlive(keep_alive->switch_preset, keep_alive->set_time_ex);
        }
        else
        {
            String cmd_string = TextCmdPtr(cmd_text);

            if (cmd_string=="config.action=disconnect]")
            {
                keep_live_count = CONTROL_TIMEOUT_COUNT;
                received_cmd_seq = 0;
            }
            else if (cmd_string=="config.action=reboot]" || cmd_string=="config.action=init]" || cmd_string=="config.action=clear_preset]")
            {
                memo_debug->Lines->Add(GetTime()+cmd_string);

                String cmd_text = D1608CMD_CONTROL_FLAG;
                cmd_text = cmd_text+   cmd_string.SubString(1, cmd_string.Length()-1)   +"_confirm";
                SendCmd2(cmd_text+D1608CMD_TAIL);

                keep_live_count = CONTROL_TIMEOUT_COUNT;
                received_cmd_seq = 0;
                {
                    sendcmd_list.empty();
                    shape_live->Hide();
                    shape_link->Hide();
                    shape_power->Hide();
                    lblDeviceName->Hide();
                    lblDeviceInfo->Hide();
                    // Level Meter����
                    for (int i=0;i<32;i++)
                    {
                        UpdateWatchLevel(i, -49);
                    }
                    device_connected = false;
                }
            }
            else if (cmd_string.SubString(1, 11) == "config.lock" || cmd_string.SubString(1, 13) == "config.unlock")
            {
                // ���»�ȡ����
                TPackage package = {0};
                package.udp_port = UDP_PORT_READ_PRESET;

                D1608PresetCmd preset_cmd(version);
                preset_cmd.preset = 0; // ��ȡglobal_config
                preset_cmd.verify -= UdpPackageVerifyDiff((unsigned char*)&preset_cmd, offsetof(D1608PresetCmd, data));
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

                    lblLogCount->Caption = "��־������"+IntToStr(log_count) + "   MAC������"+IntToStr(mac_count);
                }
            }
        }
        else
        {
            // TODO: ������
        }

        if (!ProcessLogBuffAck(buff, AData, ABinding))
        {
            memo_debug->Lines->Add(GetTime()+"Reply Buff");
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

        // У��һ��
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

        // ����
        if ((preset_id & 0x7F) == 0)
        {
            // global_config
            memcpy(&global_config, preset_cmd.data, sizeof(global_config));

            global_config_loaded = true;

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

            // ��ʾpreset����
            clbAvaliablePreset->Clear();
            clbAvaliablePreset->Items->Add(global_config.preset_name[0][0] ? global_config.preset_name[0] : "PRESET1");
            clbAvaliablePreset->Items->Add(global_config.preset_name[1][0] ? global_config.preset_name[1] : "PRESET2");
            clbAvaliablePreset->Items->Add(global_config.preset_name[2][0] ? global_config.preset_name[2] : "PRESET3");
            clbAvaliablePreset->Items->Add(global_config.preset_name[3][0] ? global_config.preset_name[3] : "PRESET4");
            clbAvaliablePreset->Items->Add(global_config.preset_name[4][0] ? global_config.preset_name[4] : "PRESET5");
            clbAvaliablePreset->Items->Add(global_config.preset_name[5][0] ? global_config.preset_name[5] : "PRESET6");
            clbAvaliablePreset->Items->Add(global_config.preset_name[6][0] ? global_config.preset_name[6] : "PRESET7");
            clbAvaliablePreset->Items->Add(global_config.preset_name[7][0] ? global_config.preset_name[7] : "PRESET8");

            // �汾У��
            if (global_config.version >= offsetof(GlobalConfig, ad_da_card))
            {
                int dsp_920_num = 0;
                TPanel* iDsp[8] = {iDsp1, iDsp2, iDsp3, iDsp4, iDsp5, iDsp6, iDsp7, iDsp8};
                for (int i=0;i<8;i++) // xDO: ��ʱֻ����8Ƭ
                {
                    dsp_920_num += global_config.yss920[i];
                    iDsp[i]->Caption = (global_config.yss920[i]?"920":"X");
                }

                TPanel* iAdDa[8] = {iAD_abcd, iAD_efgh, iAD_ijkl, iAD_mnop, iDA_1234, iDA_5678, iDA_9_12, iDA_13_16};
                for (int i=0;i<8;i++)
                    iAdDa[i]->Caption = global_config.ad_da_card[i];

                iLed->Caption = global_config.led_control_count;

                REAL_INPUT_DSP_NUM = GetVersionConfig().input_channel_count;
                REAL_OUTPUT_DSP_NUM = GetVersionConfig().output_channel_count;

                SetIOChannelNum();
            }
            cbGlobalDspName->Checked = ((global_config.is_global_name == 1) || (global_config.is_global_name == 0xFF));
            if (global_config.version >= offsetof(GlobalConfig, auto_saved))
            {
                // ��ȡ'�Զ�����'����
                cbPresetAutoSaved->Checked = ((global_config.auto_saved == 1) || (global_config.auto_saved == 0xFF));
                cbLedTest->Checked = (global_config.led_test == 1);

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
                {
                    const OlaList * input_dict = (const OlaList *)preset_cmd.data;
                    char * input_data_base = (char*)input_dict +
                            sizeof(input_dict->length) +
                            sizeof(input_dict->struct_length) +
                            input_dict->length * sizeof(OlaInfo);
                    for (int i=0;i<4;i++)
                    {
                        ReadIODspMem(dest_config_map->input_dsp + i + preset_cmd.store_page*4,
                            input_data_base + i*input_dict->struct_length,
                            (OlaList*)&input_dsp_ola_list,
                            input_dict);
                    }
                    break;
                }
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
                        ReadIODspMem(dest_config_map->input_dsp + i + preset_cmd.store_page*4,
                            input_data_base + i*input_dict->struct_length,
                            (OlaList*)&input_dsp_ola_list,
                            input_dict);
                    }
                    break;
                }
            case 4:
                memcpy(&dest_config_map->output_dsp[0],preset_cmd.data,  sizeof(config_map.output_dsp[0])*4);
                break;
            case 5:
                memcpy(&dest_config_map->output_dsp[4],preset_cmd.data,  sizeof(config_map.output_dsp[0])*4);
                break;
            case 6:
                memcpy(&dest_config_map->output_dsp[8],preset_cmd.data,  sizeof(config_map.output_dsp[0])*4);
                break;
            case 7:
                memcpy(&dest_config_map->output_dsp[12], preset_cmd.data, sizeof(config_map.output_dsp[0])*4);
                break;
            }
        }

        read_one_preset_package_list.pop_back();

        if (read_one_preset_package_list.empty())
        {
            if (preset_id == cur_preset_id || preset_id==0)
                config_map = all_config_map[cur_preset_id-1];

            tmDelayUpdateUI->Enabled = false;
            ApplyConfigToUI();
            CloseDspDetail();
            memo_debug->Lines->Add(GetTime()+"ͬ�����");

            lblVersion->Caption = VersionToStr(last_connection.data.version)+ " " +VersionToStr(version);
            edtDeviceFullName->Text = edtDeviceFullName_data;
            tmWatch->Enabled = true;
        }
        else
        {
            package = read_one_preset_package_list.back();
            SendBuffer(dst_ip, package.udp_port, package.data, package.data_size);

            restor_delay_count = 15;
            tmDelayBackup->Enabled = true;
        }
    }
    else if (ABinding->PeerPort == UDP_PORT_STORE_PRESET_PC2FLASH)
    {
        D1608PresetCmd preset_cmd(version);
        AData->ReadBuffer(&preset_cmd, std::min(sizeof(preset_cmd), AData->Size));

        // У��һ��
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

            // ʧ��
            keep_live_count = CONTROL_TIMEOUT_COUNT;
        }
        else
        {
            package = package_list.back();
            SendBuffer(dst_ip, package.udp_port, package.data, package.data_size);

            pbBackup->Position = pbBackup->Max - package_list.size();
            restor_delay_count = 15;
            tmDelayBackup->Enabled = true;

            lblDeviceName->Show();
            lblDeviceName->Caption = "LOAD FROM FILE......";
        }
    }
    else if (ABinding->PeerPort == UDP_PORT_READ_FLASH)
    {
        FlashRW_Data flash_rw;
        AData->ReadBuffer(&flash_rw, std::min(sizeof(flash_rw), AData->Size));

        // У��һ��
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

            // ��ɣ�д���ļ�
            TFileStream * file = new TFileStream(save_device_to_file_filename, fmCreate);
            if (!file)
            {
                ShowMessage("���ļ�ʧ��");
                return;
            }

            LoadGlobalConfig(smc_config.global_config, smc_config.device_flash_dump);
#if 0
            // ת������ͨ��8��preset����
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

            // ʧ��
            keep_live_count = CONTROL_TIMEOUT_COUNT;
        }
        else
        {
            package = package_list.back();
            SendBuffer(dst_ip, package.udp_port, package.data, package.data_size);

            pbBackup->Position = pbBackup->Max - package_list.size();
            restor_delay_count = 15;
            tmDelayBackup->Enabled = true;

            lblDeviceName->Show();
            lblDeviceName->Caption = "SAVE TO FILE......";
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

static void UpdateAdc_ValueListEditor(TValueListEditor * vls, ADC_Data_Ex data)
{
    // ���ϵ��ѹУ׼
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
}
void TForm1::ProcessVote(ADC_Data_Ex adc_ex, ADC_Data_Ex adc_ex_max, ADC_Data_Ex adc_ex_min)
{
    this->adc_ex_max = adc_ex_max;
    this->adc_ex_min = adc_ex_min;
    UpdateAdc_ValueListEditor(ValueListEditor2, adc_ex);
    UpdateAdc_ValueListEditor(vleAdcMax, adc_ex_max);
    UpdateAdc_ValueListEditor(vleAdcMin, adc_ex_min);


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
    // ���䵽����ͼ
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
        memo_debug->Lines->Add(GetTime()+" ������һ����Ϣ:"+cmd);

        SendBuffer(dst_ip, package.udp_port, package.data, package.data_size);
        sendcmd_delay_count = 15;
    }

    return true;
}
void TForm1::ProcessKeepAlive(int preset_id, unsigned __int64 timer)
{
    if (preset_id<1 || preset_id>8)
    {
        // �յ������ݰ��쳣
        return;
    }

    if (preset_id != cur_preset_id)
    {
        SetPresetId(preset_id);
        StartReadCurrentPreset();
        restor_delay_count = 15;
        tmDelayBackup->Enabled = true;
    }

    // ��ʾʱ��
    unsigned __int64 running_time;
    if (timer > global_config.adjust_running_time)
        running_time = timer - global_config.adjust_running_time;
    else
        running_time = timer - global_config.adjust_running_time + MAX_TIMER_MS;

    running_time = running_time / 1000; // ת����λΪ��

    int hour = running_time / 3600;
    int minute = running_time % 3600;
    minute = minute / 60;
    lblDeviceRunningTime->Caption = lblDeviceRunningTime->Caption.sprintf("%d:%02d", hour, minute);

    // ��ʾδ������ʱ��
    running_time = timer / 1000;    // ��λ����
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
    lblDeviceName->Show();
    lblDeviceInfo->Show();
    device_connected = true;
}

static String GetNameOfAdc(int index)
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

    if (index < 21)
        return adc_name[index];
    else
        return IntToStr(index);
}
static void ApplyLogData( TListItem* item, Event event, int address, String syn_time)
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
        item->SubItems->Add("�رյ�Դ");
        item->SubItems->Add("�´���������"+IntToStr(event.event_data));
        break;
    case EVENT_SYSTEM_LIMIT:
        item->SubItems->Add("�ﵽ���д�����ʱ������");
        item->SubItems->Add(event.event_data);
        break;
    case EVENT_SAVE_PRESET:
        item->SubItems->Add("����Preset");

        {
            String page_indexs = "Preset���:"+IntToStr(((event.event_data>>8) & 0x7F));
            if ((event.event_data & 0x7F) == 0)
            {
                item->SubItems->Add("����Ҫ����");
            }
            else
            {
                page_indexs = page_indexs + " ����ҳ:";
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
        item->SubItems->Add("�ػ����̳ɹ�");
        item->SubItems->Add("�´���������"+IntToStr(event.event_data));
        break;
    case EVENT_INPUT_OVERFLOW:
        item->SubItems->Add("inputͨ�����������ؾ���");
        item->SubItems->Add("ͨ����"+IntToStr(event.event_data));
        break;
    case EVENT_OUTPUT_OVERFLOW:
        item->SubItems->Add("outputͨ�����������ؾ���");
        item->SubItems->Add("ͨ����"+IntToStr(event.event_data));
        break;
    case EVENT_WRITE_FLASH_ERROR:
        item->SubItems->Add("дflash����");
        item->SubItems->Add("��ַ:0x"+IntToHex(event.event_data * 2048, 8));
        break;
    case EVENT_REBOOT:
        if (event.event_data < 0x10)
        {
            item->SubItems->Add("��λ����������");
            if (event.event_data == 0)
            {
                item->SubItems->Add("����");
            }
            else if (event.event_data == 1)
            {
                item->SubItems->Add("���PRESER");
            }
            else if (event.event_data == 2)
            {
                item->SubItems->Add("�ָ���������");
            }
            else if (event.event_data == 10)
            {
                item->SubItems->Add("������������");
            }
            else
            {
                item->SubItems->Add(event.event_data);
            }
        }
        else
        {
            item->SubItems->Add("�ϵ�ʱ���");
            if (event.event_data == 0x011)
            {
                item->SubItems->Add("���PRESER");
            }
            else if (event.event_data == 0x12)
            {
                item->SubItems->Add("�ָ���������");
            }
            else
            {
                item->SubItems->Add(event.event_data);
            }
        }
        break;
    case EVENT_SAVE_PRESET_OK:
        item->SubItems->Add("�������");
        {
            String page_indexs = "����ҳ";
            if (event.event_data & 1) page_indexs = page_indexs + "1,";
            if (event.event_data & 2) page_indexs = page_indexs + "2,";
            if (event.event_data & 4) page_indexs = page_indexs + "3,";
            if (event.event_data & 8) page_indexs = page_indexs + "4,";
            if (event.event_data & 16) page_indexs = page_indexs + "5,";
            item->SubItems->Add(page_indexs);
        }
        break;
    case EVENT_CHECK_MD5_FAIL:
        item->SubItems->Add("���������");
        item->SubItems->Add("");
        break;
    case EVENT_28J60_REINIT_ERROR:
        item->SubItems->Add("ENC28J60��ʼ������");
        item->SubItems->Add("ʧ�ܴ���:"+IntToStr(event.event_data));
        break;
    case EVENT_MAC_ADDRESS_OVERFLOW:
        item->SubItems->Add("MAC��ַ��־���");
        item->SubItems->Add("");
        break;
    case EVENT_NO_KEY:
        item->SubItems->Add("�豸��Ȩ����");
        item->SubItems->Add("");
        break;
    case EVENT_DSP_NOT_MATCH_ERROR:
        item->SubItems->Add("YSS920�����ò�������");
        if (event.event_data > 0x80)
            item->SubItems->Add("ȱ��:"+IntToStr(event.event_data-0x80));
        else
            item->SubItems->Add("���:"+IntToStr(event.event_data));
        break;
    case EVENT_LED_NUM_ERR:
        item->SubItems->Add("LED����оƬ����");
        item->SubItems->Add(event.event_data);
        break;
    case EVENT_FILENAME_CHANGED:
        item->SubItems->Add("����/��������");
        if (event.event_data == 0)
            item->SubItems->Add("Load From File");
        else
            item->SubItems->Add("Save To File");
        break;
    case EVENT_SET_MAC_ADDRESS:
        item->SubItems->Add("������MAC��ַ");
        item->SubItems->Add(IntToHex(event.event_data, 6));
        break;
    case EVENT_ADDA_ERROR:
        if (event.event_data > 32)
            item->SubItems->Add("AD���������ò�������");
        else
            item->SubItems->Add("DA���������ò�������");
        item->SubItems->Add("����������"+IntToStr(event.event_data%32));
        break;
    case EVENT_EACH_HOUR:
        item->SubItems->Add("����ÿСʱ��ʶ");
        item->SubItems->Add(IntToStr(event.event_data));
        break;
    case EVENT_TIME_1:
        item->SubItems->Add("ʱ��ͬ����Ϣ1");
        item->SubItems->Add("0x"+IntToHex(event.event_data, 4));
        break;
    case EVENT_TIME_2:
        item->SubItems->Add("ʱ��ͬ����Ϣ2");
        item->SubItems->Add("0x"+IntToHex(event.event_data, 4));
        break;
    case EVENT_TIME_3:
        item->SubItems->Add("ʱ��ͬ����Ϣ3");
        item->SubItems->Add("0x"+IntToHex(event.event_data, 4));
        break;
    case EVENT_TIME_4:
        item->SubItems->Add("ʱ��ͬ����Ϣ4");
        item->SubItems->Add("0x"+IntToHex(event.event_data, 4));
        break;
    case EVENT_SAVE_LOAD_TIMEOUT:
        item->SubItems->Add("���̻��߻ָ���ʱ����");
        item->SubItems->Add(event.event_data==1?"LOAD FROM FILE":"SAVE TO FILE");
        break;
    case EVENT_48V:
        item->SubItems->Add("48V��Ӳ����ƥ��");
        item->SubItems->Add(event.event_data==0?"ȱ��":"���");
        break;
    case EVENT_RESET_RUNNING_TIME:
        item->SubItems->Add("��������ʱ�侯��");
        item->SubItems->Add("");
        break;
    case EVENT_IWDG_REBOOT:
        item->SubItems->Add("�ϴ������ڼ俴�Ź��쳣����");
        item->SubItems->Add("");
        break;
    case EVENT_POWER_SAVE_ERROR:
        item->SubItems->Add("������̴���");
        item->SubItems->Add("");
        break;
    case EVENT_28J60_VERSION_ERROR:
        {
            item->SubItems->Add("28J60�汾�Ŵ���");
            item->SubItems->Add(IntToStr((event.event_data>>8)&0xFF)+" -> " + IntToStr(event.event_data&0xFF));
        }
        break;
    case EVENT_UPGRADE_DATE:
        item->SubItems->Add("����ʱ�汾����");
        item->SubItems->Add(IntToHex(event.event_data, 4));
        break;
    case EVENT_UPGRADE_TIME:
        item->SubItems->Add("����ʱ�汾ʱ��");
        item->SubItems->Add(IntToHex(event.event_data, 4));
        break;
    case EVENT_MCU_CLK_ERR:
        if (event.event_data == 0)
        {
            item->SubItems->Add("ʱ��У׼");
            item->SubItems->Add("����");
        }
        else if (event.event_data == 1)
        {
            item->SubItems->Add("ʱ��У׼�쳣����");
            item->SubItems->Add("�豸ʱ�ӿ�5%����");
        }
        else if (event.event_data == 2)
        {
            item->SubItems->Add("ʱ��У׼�쳣����");
            item->SubItems->Add("�豸ʱ����5%����");
        }
        else
        {
            item->SubItems->Add("ʱ��У׼�쳣����");
            item->SubItems->Add(IntToHex(event.event_data, 2));
        }
        break;
    case EVENT_VOTE_UP_OVERFLOW:
        item->SubItems->Add("��ѹ/�����������޴���");
        item->SubItems->Add(GetNameOfAdc(event.event_data));
        break;
    case EVENT_VOTE_DOWN_OVERFLOW:
        item->SubItems->Add("��ѹ/�����������޴���");
        item->SubItems->Add(GetNameOfAdc(event.event_data));
        break;
    case EVENT_MAX_JOB_TIME:
        item->SubItems->Add("������ʱ�����");
        item->SubItems->Add(IntToStr(event.event_data));
        break;
    case EVENT_ERR_JOB_FUNC_PTR:
        item->SubItems->Add("������г��ֿ�ָ�����");
        item->SubItems->Add("");
        break;
    case EVENT_ERR_WRITE_LOG:
        item->SubItems->Add("д��־ʧ�ܴ���");
        item->SubItems->Add(IntToStr(event.event_data));
        break;
    case EVENT_ERR_28J60_SENDDATA:
        item->SubItems->Add("28J60�������ĳ���");
        item->SubItems->Add(IntToStr(event.event_data));
        break;
    case EVENT_ERR_EXTERN_RAM:
        item->SubItems->Add("RAM������");
        item->SubItems->Add(IntToHex(event.event_data, 4));
        break;
    case EVENT_POWER_ON:
        item->SubItems->Add("������Դ");
        item->SubItems->Add("������������"+IntToStr(event.event_data));
        break;
    case EVENT_STACK_OVERFLOW:
        item->SubItems->Add("ջ�������");
        item->SubItems->Add(IntToStr(event.event_data));
        break;
    case EVENT_SAVE_LOAD_COUNT:
        item->SubItems->Add("���̻��߻ָ�����");
        item->SubItems->Add(event.event_data);
        break;
    default:
        item->SubItems->Add(event.event_id);
        item->SubItems->Add("0x"+IntToHex(event.event_data, 2));
        break;
    }

    item->SubItems->Add(syn_time);
}
void TForm1::ProcessLogData()
{
    int log_tail_index = ((int)tail_address-LOG_START_PAGE) / sizeof(Event);
    // �ƶ���־��ʹ�÷���˳��ͬʱ��¼���һ����Ч��־����
    for (int i=0;i<EVENT_POOL_SIZE;i++)
    {
        // ��i����¼��ԭʼ�����е�λ��
        int mapped_index = (log_tail_index-i-1+EVENT_POOL_SIZE) % EVENT_POOL_SIZE;
        event_data[i] = event_data_tmp[mapped_index];

        if (event_data[i].timer != 0xFFFFFFFF)
        {
            log_count = i+1;
        }

        // ͬʱ���ͬ��ʱ��
        event_syn_timer[i] = "";
    }

    unsigned __int64 time_base = 0;
    // ���¼���ʱ��
    for (int i=0;i<log_count;i++)
    {
        // ����ͬ��ʱ��
        switch (event_data[i].event_id)
        {
            case EVENT_TIME_1:
            {
                if (i>=3 &&
                    event_data[i-1].event_id == EVENT_TIME_2 &&
                    event_data[i-2].event_id == EVENT_TIME_3 &&
                    event_data[i-3].event_id == EVENT_TIME_4)
                {
                    // TODO: �����������Σ���������������
                    time_base = 0;//event_data[i].event_data;
                    time_base = time_base << 16 | event_data[i-1].event_data;
                    time_base = time_base << 16 | event_data[i-2].event_data;
                    time_base = time_base << 16 | event_data[i-3].event_data;

                    if (time_base < 0x00FFFFFFFFFFFFFF)
                    {
                        // ��¼ʱ��
                        // ��2000-1-1Ϊ��׼
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
                // ��2000-1-1Ϊ��׼
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

    // ���ϼ���ʱ��
    for (int i=log_count-1;i>=0;i--)
    {
        // ����ͬ��ʱ��
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
                // ��2000-1-1Ϊ��׼
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
        //memo_debug->Lines->Add(GetTime()+"��Ϣƥ��");
        sendcmd_list.pop_back();
        if (sendcmd_list.size() > 0)
        {
            package = sendcmd_list.back();
            SendBuffer(dst_ip, package.udp_port, package.data, package.data_size);
            sendcmd_delay_count = 15;
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
            }
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::tmWatchTimer(TObject *Sender)
{
    //memo_debug->Lines->Add(GetTime()+"����������¼�: " + (udpControl->Active?"link":"unlink"));

    bool _disconnected;
    if (is_manual_disconnect)
    {
        _disconnected = true;
    }
    else
    {
        // stroe����open״̬
        if (pbBackup->Visible)
        {
            return;
        }

        // keep alive
        if ((keep_live_count < CONTROL_TIMEOUT_COUNT) && udpControl->Active)
        {
            keep_live_count++;
            //memo_debug->Lines->Add(GetTime()+"���������++: " + IntToStr(keep_live_count));
            _disconnected = false;
        }
        else
        {
            _disconnected = true;
        }
    }

    if (_disconnected)
    {
        CloseControlLink("���ʱ");
        sendcmd_list.empty();
        shape_live->Hide();
        shape_link->Hide();
        shape_power->Hide();
        if (!is_manual_disconnect)
        {
            lblDeviceName->Hide();
            lblDeviceInfo->Hide();
        }
        // Level Meter����
        for (int i=0;i<32;i++)
        {
            UpdateWatchLevel(i, -49);
        }
        device_connected = false;
        received_cmd_seq = 0;

        ClearUI();
    }

    String cmd_text = D1608CMD_KEEPLIVE_FLAG;
    cmd_text = cmd_text+"config.action.keeplive="+IntToStr(received_cmd_seq);
    SendCmd2(cmd_text+D1608CMD_TAIL);
    send_keeplive_count++;
    last_keeplive_time = Now();

    // ���resize�¼�
    if (need_resize)
    {
        SetIOChannelNum();
        need_resize = false;
    }
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
        // �л���ť��ɫ

        for (int i=0;i<REAL_INPUT_DSP_NUM;i++)   // xDO: ԭ����17��������automix������ֻ������������
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

        // ����
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

            // ����PaintBox1�ĳߴ�
            PaintBox1->Left = 8;
            PaintBox1->Width = 753;
            pbComp->Hide();

            // ����COMP����
            pnlComp->Enabled = false;
            pnlComp->Color = clGray;
            btnDspComp->Enabled = false;

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

            // ����PaintBox1�ĳߴ�
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
            btnDspComp->Down = config_map.output_dsp[dsp_num-1].comp_switch;
            edtCompRatio->Text = Ration2String(config_map.output_dsp[dsp_num-1].ratio);
            edtCompThreshold->Text = config_map.output_dsp[dsp_num-1].threshold/10.0;
            edtCompGain->Text = config_map.output_dsp[dsp_num-1].comp_gain/10.0;
            cbCompAutoTime->Checked = config_map.output_dsp[dsp_num-1].auto_time;

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

    memo_debug->Lines->Add(GetTime()+"master����: "+IntToStr(value)+" "+IntToStr(keep_live_count));

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
    memo_debug->Lines->Add(IntToStr(config_map.input_dsp[0].master_level_a));
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
    if (edt)
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
    // ����˵�
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
    // ����˵�
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
    // XDO: �޸��˲������������ܵ�Ӱ��
    int preset_freq_list[11] = {20, 50, 100, 200, 500, 1000, 2000, 5000, 7500, 10000, 20000};
    for (int i=FIRST_FILTER+2; i<=LAST_FILTER-2; i++)
    {
        filter_set.GetFilter(i)->ChangFilterParameter("PARAMETRIC", preset_freq_list[i-1], 0, 4.09);
        filter_set.GetFilter(i)->name = IntToStr(i-1);
        filter_set.SetBypass(i, false);
        filter_set.RepaintPaint(i);
    }

    filter_set.GetFilter(HP_FILTER+1)->ChangFilterParameter("LOW_SHELF", preset_freq_list[FIRST_FILTER], 0, 4.09);
        filter_set.GetFilter(HP_FILTER+1)->name = IntToStr(FIRST_FILTER+1-1);
        filter_set.SetBypass(HP_FILTER+1, false);
        filter_set.RepaintPaint(HP_FILTER+1);

    filter_set.GetFilter(LP_FILTER-1)->ChangFilterParameter("HIGH_SHELF", preset_freq_list[LP_FILTER-1-1], 0, 4.09);
        filter_set.GetFilter(LP_FILTER-1)->name = IntToStr(LAST_FILTER-1-1);    // name����PEQ���
        filter_set.SetBypass(LP_FILTER-1, false);
        filter_set.RepaintPaint(LP_FILTER-1);

    // ѹ������
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

    String cmd_text = D1608CMD_FLAG;
    if (cbGlobalDspName->Checked)
    {
        strncpy(global_config.input_dsp_name[label->Tag], label->Caption.c_str(), 6);

        String cmd_text = D1608CMD_FLAG;
        cmd_text = cmd_text+"config.input<"+IntToStr(dsp_num)+">.name="+label->Caption.SubString(1,16);
        SendCmd(cmd_text+D1608CMD_TAIL);
    }
    else
    {
        strncpy(config_map.output_dsp[label->Tag].dsp_name, label->Caption.c_str(), 6);

        String cmd_text = D1608CMD_FLAG;
        cmd_text = cmd_text+"input<"+IntToStr(dsp_num)+">.name="+label->Caption;
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
    else
    {
        label->Caption = edtInput->Text;
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
void __fastcall TForm1::FormClick(TObject *Sender)
{
    //
}
//---------------------------------------------------------------------------
void __fastcall TForm1::FormMouseDown(TObject *Sender, TMouseButton Button,
      TShiftState Shift, int X, int Y)
{
    //
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
    // ���浽�ļ�
    SaveDialog1->Filter = "preset|*.preset";
    if (SaveDialog1->Execute())
    {
        // save config_map to file
        TFileStream * file = new TFileStream(SaveDialog1->FileName, fmCreate);
        if (!file)
        {
            ShowMessage("���ļ�ʧ��");
            return;
        }

        file->WriteBuffer(&config_map, sizeof(config_map));

        delete file;
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
        ShowMessage("�豸æ");
        return;
    }

    OpenDialog1->Filter = "preset|*.preset";
    if (OpenDialog1->Execute())
    {
        // save config_map to file
        TFileStream * file = new TFileStream(OpenDialog1->FileName, fmOpenRead);
        if (!file)
        {
            ShowMessage("���ļ�ʧ��");
            return;
        }

        file->ReadBuffer(&all_config_map[select_preset_id-1], sizeof(config_map));

        delete file;

        ///ApplyConfigToUI();
        ///CloseDspDetail();

        if (!udpControl->Active)
        {
            // �ѻ�
            //all_config_map[select_preset_id-1] = smc_config.all_config_map[select_preset_id-1].ToConfigMap();
            // TODO: �Ƿ���µ�ǰ���棿
            if (cur_preset_id == select_preset_id)
            {
                CloseDspDetail();
                tmDelayUpdateUITimer(NULL);
            }
        }
        else
        {
            // ����
            clbAvaliablePreset->Checked[select_preset_id-1] = true;
            clbAvaliablePresetClickCheck(NULL);

            // Download To Device
            // д�����е�preset����
            D1608PresetCmd preset_cmd(version);
            strcpy(preset_cmd.flag, D1608PRESETCMD_PC2FLASH_FLAG);
            preset_cmd.preset = select_preset_id;

            //for (int i=select_preset_id-1;i<=select_preset_id-1;i++)
            int i = select_preset_id;
            for (int store_page=0;store_page<8;store_page++)
            {
                preset_cmd.store_page = store_page;
                switch(preset_cmd.store_page)
                {
                case 0:
                    memcpy(preset_cmd.data, &all_config_map[i].input_dsp[0], sizeof(InputConfigMap)*4);
                    break;
                case 1:
                    memcpy(preset_cmd.data, &all_config_map[i].input_dsp[4], sizeof(InputConfigMap)*4);
                    break;
                case 2:
                    memcpy(preset_cmd.data, &all_config_map[i].input_dsp[8], sizeof(InputConfigMap)*4);
                    break;
                case 3:
                    memcpy(preset_cmd.data, &all_config_map[i].input_dsp[12], sizeof(InputConfigMap)*4);
                    break;
                case 4:
                    memcpy(preset_cmd.data, &all_config_map[i].output_dsp[0], sizeof(OutputConfigMap)*4);
                    break;
                case 5:
                    memcpy(preset_cmd.data, &all_config_map[i].output_dsp[4], sizeof(OutputConfigMap)*4);
                    break;
                case 6:
                    memcpy(preset_cmd.data, &all_config_map[i].output_dsp[8], sizeof(OutputConfigMap)*4);
                    break;
                case 7:
                    memcpy(preset_cmd.data, &all_config_map[i].output_dsp[12], sizeof(OutputConfigMap)*4);
                    break;
                }

                TPackage package = {0};
                memcpy(package.data, &preset_cmd, sizeof(preset_cmd));
                package.udp_port = UDP_PORT_STORE_PRESET_PC2FLASH;
                package.data_size = sizeof(preset_cmd);

                package_list.insert(package_list.begin(), package);
            }

            TPackage package = package_list.back();
            SendBuffer(dst_ip, package.udp_port, package.data, package.data_size);

            //last_command = UDP_PORT_STORE_PRESET_PC2FLASH;
            //last_restore_package = preset_cmd;
            restor_delay_count = 15;
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
    lblVersion->Caption = "-------- " +VersionToStr(version);

    lblDeviceName->Caption = "";
    lblDeviceInfo->Caption = "";
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

    {
        // ��������
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

    cbMenuKeyFunction->ItemIndex = global_config.menu_key_function?global_config.menu_key_function-1:0;
    cbUpKeyFunction->ItemIndex = global_config.up_key_function?global_config.up_key_function-1:1;
    cbDownKeyFunction->ItemIndex = global_config.down_key_function?global_config.down_key_function-1:2;

    struct in_addr in;
    in.S_un.S_addr = global_config.static_ip_address;
    SetWindowText(hIpEdit, inet_ntoa(in));

    if (global_config.static_ip_address == 0)
        rbDhcpEnabled->Checked = true;
    else
        rbStaticIpEnabled->Checked = true;

    EnableWindow(hIpEdit, rbStaticIpEnabled->Checked);

    // �޸Ľ���
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
            // С����
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
            // С����
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
            // С����
            if (pnlDspDetail->Visible && (pnlDspDetail->Tag-1==ObjectIndex))
            {
                dsp_delay_trackbar->Position = config_map.input_dsp[ObjectIndex].delay;
            }
 		}
		else if (cmd_id >= GetOffsetOfData(&config_map.input_dsp[ObjectIndex].filter)
			&& (cmd_id < GetOffsetOfData(&config_map.input_dsp[ObjectIndex].filter) + sizeof(config_map.input_dsp[ObjectIndex].filter)))
		{
			//int filter = (cmd_id - GetOffsetOfData(&config_map.input_dsp[ObjectIndex].filter)) / sizeof(config_map.input_dsp[ObjectIndex].filter[0]);
            // С����
            if (pnlDspDetail->Visible && (pnlDspDetail->Tag-1==ObjectIndex))
            {
                panel_agent->LoadPreset();
                PaintBox1->Refresh();
            }
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
            // С����
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
            // С����
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
		}
		else if (cmd_id >= GetOffsetOfData(&config_map.output_dsp[ObjectIndex].mix_mute)
			&& (cmd_id < GetOffsetOfData(&config_map.output_dsp[ObjectIndex].mix_mute) + sizeof(config_map.output_dsp[ObjectIndex].mix_mute)))
		{
            // С����
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
            // С����
            if (pnlMix->Visible && pnlMix->Tag-1==ObjectIndex)
            {
                for (int i=0;i<REAL_INPUT_DSP_NUM;i++)
                {
                    mix_mute_btn[i]->Down = config_map.output_dsp[ObjectIndex].mix_mute[i];
                    mix_level_trackbar[i]->Position = config_map.output_dsp[ObjectIndex].mix[i];
                }
            }
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
        edtStartBuildTime->Text = DateTime2Str(GetDateTimeFromMarco(global_config.start_build_time));
    }
    catch(...)
    {
        edtStartBuildTime->Text = global_config.start_build_time;
    }
    edtStartBuildTime->Text = edtStartBuildTime->Text+"\t" + AppBuildTime2Str(global_config.build_time) + "\t";
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
    // ������λ���ڴ�
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
        String cmd_text = D1608CMD_CONTROL_FLAG;
        cmd_text = cmd_text+"config.active_preset="+IntToStr(menu->Tag);
        SendCmd2(cmd_text+D1608CMD_TAIL);

        // ��ʱ5���ǿ�ƴӱ����ڴ���µ�����
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
            ShowMessage("IP��ַ��Ч������������");
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
        ShowMessage("δ�������޷���ȡ��־");
        return;
    }

    // ���һ��һ����MAC��ַ������ʵ��MAC�����ʱ��ָ����Enabled
    btnGetLog->Enabled = false;
    lvLog->Clear();
    LogBuff buff = {LOG_FLAG};
    for (buff.address = 0x8000000+128*1024+10*5*2*1024;
        buff.address < MAC_LIST_START_PAGE+MAC_LIST_SIZE;
        buff.address += 1024)
    {
        // ���� LogBuff ��У���
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
    String new_name = InputBox("�޸�����", "", lblPresetName->Caption);
    if (new_name == lblPresetName->Caption)
        return;
    else
        lblPresetName->Caption = new_name;

    String cmd_text = D1608CMD_CONTROL_FLAG;
    cmd_text = cmd_text+"config.preset_name<"+IntToStr(cur_preset_id)+"]=" + lblPresetName->Caption.SubString(1,16);
    SendCmd(cmd_text+D1608CMD_TAIL);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btnDeviceNameClick(TObject *Sender)
{
    strncpy(global_config.d1616_name, edtDeviceName->Text.c_str(), 16);

    UpdateCaption();

    String cmd_text = D1608CMD_CONTROL_FLAG;
    cmd_text = cmd_text+"config.name="+edtDeviceName->Text.SubString(1,16);
    SendCmd2(cmd_text+D1608CMD_TAIL);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::clbAvaliablePresetClickCheck(TObject *Sender)
{
    // ����ȫ����ѡ��
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
        // �ָ�
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
            cmd_text = cmd_text+"config.avaliable_preset<"+IntToStr(i+1)+"]="+(clbAvaliablePreset->Checked[i]?"on":"off");
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
        ShowMessage("�������ý�������");
        return;
    }


    String cmd_text = D1608CMD_CONTROL_FLAG;
	global_config.running_timer_limit = edtRunningTimer->Enabled ? running_timer : 0;
	global_config.reboot_count_limit = edtRebootCount->Enabled ? roboot_count : 0;
    if ((global_config.running_timer_limit==0) && (global_config.reboot_count_limit==0))
    {
        ShowMessage("��������һ����������");
        return;
    }
    if (edtLockedString->Enabled)
        strncpy(global_config.locked_string, edtLockedString->Text.c_str(), 16);
    else
        global_config.locked_string[0] = '\0';
    strncpy(global_config.password, edtPassword->Text.c_str(), 16);
    strncpy(global_config.password_of_key, edtKeyPassword->Text.c_str(), 16);

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
        ShowMessage("����������");
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
    // ��̨����    
    String cmd_text = D1608CMD_CONTROL_FLAG;
    cmd_text = cmd_text+"config.unlock_ext=on";
/*
    ���
	int running_timer_limit;	// ��λ����
	int reboot_count_limit;
	char locked_string[20];		// ����������ʾ���ַ���
	char password_of_key[20];	// ���̽�������
	char password[20];			// ����
*/
    SendCmd(cmd_text+D1608CMD_TAIL);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btnLeaveTheFactoryClick(TObject *Sender)
{
    // ����
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
    else if (Key == VK_RETURN)
    {
        try{
            if (edt->Text == "��")
                config_map.output_dsp[dsp_num-1].ratio = 0;
            else
                config_map.output_dsp[dsp_num-1].ratio = 100.0 / edt->Text.ToDouble();//*ratio_config.scale;
            if (config_map.output_dsp[dsp_num-1].ratio > ratio_config.max_value)
                config_map.output_dsp[dsp_num-1].ratio = ratio_config.max_value;
            else if (config_map.output_dsp[dsp_num-1].ratio < ratio_config.min_value)
                config_map.output_dsp[dsp_num-1].ratio = ratio_config.min_value;
            
            String cmd_text = D1608CMD_FLAG;
            cmd_text = cmd_text+"output<"+IntToStr(dsp_num)+">.comp.ratio="+edtCompRatio->Text;
            SendCmd(cmd_text+D1608CMD_TAIL);

            filter_set.ratio = config_map.output_dsp[dsp_num-1].ratio / 100.0;
            pbComp->Invalidate();
        }
        catch(...)
        {
        }

        edt->Text = Ration2String(config_map.output_dsp[dsp_num-1].ratio);
        edt->SelectAll();
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
    else if (Key == VK_RETURN)
    {
        try{
            config_map.output_dsp[dsp_num-1].threshold = edt->Text.ToDouble()*threshold_config.scale;

            if (config_map.output_dsp[dsp_num-1].threshold > threshold_config.max_value)
                config_map.output_dsp[dsp_num-1].threshold = threshold_config.max_value;
            else if (config_map.output_dsp[dsp_num-1].threshold < threshold_config.min_value)
                config_map.output_dsp[dsp_num-1].threshold = threshold_config.min_value;

            String cmd_text = D1608CMD_FLAG;
            cmd_text = cmd_text+"output<"+IntToStr(dsp_num)+">.comp.threshold="+edtCompThreshold->Text+"dB";
            SendCmd(cmd_text+D1608CMD_TAIL);

            filter_set.threshold = config_map.output_dsp[dsp_num-1].threshold / 10.0;
            pbComp->Invalidate();
        }
        catch(...)
        {
        }

        edt->Text = FormatFloat(config_map.output_dsp[dsp_num-1].threshold/threshold_config.scale, threshold_config.precise);
        edt->SelectAll();
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
        try{
            config_map.output_dsp[dsp_num-1].attack_time = edt->Text.ToDouble()*attack_config.scale;

            if (config_map.output_dsp[dsp_num-1].attack_time > attack_config.max_value)
                config_map.output_dsp[dsp_num-1].attack_time = attack_config.max_value;
            else if (config_map.output_dsp[dsp_num-1].attack_time < attack_config.min_value)
                config_map.output_dsp[dsp_num-1].attack_time = attack_config.min_value;
            
            String cmd_text = D1608CMD_FLAG;
            cmd_text = cmd_text+"output<"+IntToStr(dsp_num)+">.comp.attack_time="+edtCompAttackTime->Text;
            SendCmd(cmd_text+D1608CMD_TAIL);
        }
        catch(...)
        {
        }

        edt->Text = FormatFloat(config_map.output_dsp[dsp_num-1].attack_time/attack_config.scale, attack_config.precise);
        edt->SelectAll();
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
        try{
            config_map.output_dsp[dsp_num-1].release_time = edtCompReleaseTime->Text.ToDouble()*release_config.scale;

            if (config_map.output_dsp[dsp_num-1].release_time > release_config.max_value)
                config_map.output_dsp[dsp_num-1].release_time = release_config.max_value;
            else if (config_map.output_dsp[dsp_num-1].release_time < release_config.min_value)
                config_map.output_dsp[dsp_num-1].release_time = release_config.min_value;
            
            String cmd_text = D1608CMD_FLAG;
            cmd_text = cmd_text+"output<"+IntToStr(dsp_num)+">.comp.release_time="+edtCompReleaseTime->Text;
            SendCmd(cmd_text+D1608CMD_TAIL);
        }
        catch(...)
        {
        }

        edt->Text = FormatFloat(config_map.output_dsp[dsp_num-1].release_time/release_config.scale, release_config.precise);
        edt->SelectAll();
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
    else if (Key == VK_RETURN)
    {
        try{
            config_map.output_dsp[dsp_num-1].comp_gain = edt->Text.ToDouble()*gain_config.scale;

            if (config_map.output_dsp[dsp_num-1].comp_gain > gain_config.max_value)
                config_map.output_dsp[dsp_num-1].comp_gain = gain_config.max_value;
            else if (config_map.output_dsp[dsp_num-1].comp_gain < gain_config.min_value)
                config_map.output_dsp[dsp_num-1].comp_gain = gain_config.min_value;

            String cmd_text = D1608CMD_FLAG;
            cmd_text = cmd_text+"output<"+IntToStr(dsp_num)+">.comp.gain="+edtCompGain->Text+"dB";
            SendCmd(cmd_text+D1608CMD_TAIL);

            filter_set.gain = config_map.output_dsp[dsp_num-1].comp_gain / 10.0;
            pbComp->Invalidate();
        }
        catch(...)
        {
        }

        edt->Text = FormatFloat(config_map.output_dsp[dsp_num-1].comp_gain/gain_config.scale, gain_config.precise);
        edt->SelectAll();
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::cbCompAutoTimeClick(TObject *Sender)
{
    TCheckBox* check_box = (TCheckBox*)Sender;
    int dsp_num = check_box->Parent->Parent->Tag;
    dsp_num -= 100;

    if (cbCompAutoTime->Checked == config_map.output_dsp[dsp_num-1].auto_time)
    {
        return;
    }

    config_map.output_dsp[dsp_num].auto_time = check_box->Checked;

    String cmd_text = D1608CMD_FLAG;
    cmd_text = cmd_text+ "output<"+IntToStr(dsp_num)+">.auto_comp="+(cbCompAutoTime->Checked?"on":"off");
    SendCmd(cmd_text+D1608CMD_TAIL);

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
        edtCompAttackTime->Text = config_map.output_dsp[dsp_num].attack_time/10.0;
        edtCompReleaseTime->Text = config_map.output_dsp[dsp_num].release_time/10.0;
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::SpeedButtonNoFrame2MouseDown(TObject *Sender,
      TMouseButton Button, TShiftState Shift, int X, int Y)
{
    TControl* control = (TControl*)Sender;
    int tag = control->Tag;
    if (Button == mbRight)
        tag = tag + 3;

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
        pnlSystem->Show();
        pnlSystem->BringToFront();
        break;
    case 3:
        if (is_inner_pc)
        {
            pnlMist->Show();
            pnlMist->BringToFront();
            // �Զ���ȡ��־
            if (lvLog->Items->Count == 0 && udpControl->Active)
            {
                btnGetLog->Click();
            }
        }
        break;
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::lbl5VdClick(TObject *Sender)
{
    // ��¼��ʾ��
    TControl* control = (TControl*)Sender;

    shape_active_adc->Top = control->Top;
    shape_active_adc->Show();

    Series1->Clear();
    lineUpLimit->Clear();
    lineDownLimit->Clear();
    Chart1->BottomAxis->SetMinMax(0, 100);

    if (control == lbl3_3V)
    {
        up_line_value = global_config.adc_range.vote_3v3m_up;
        down_line_value = global_config.adc_range.vote_3v3m_down;
    }
    else  if (control == lbl3_3Vd)
    {
        up_line_value = global_config.adc_range.vote_3v3_up;
        down_line_value = global_config.adc_range.vote_3v3_down;
    }
    else  if (control == lbl5Va)
    {
        up_line_value = global_config.adc_range.vote_5va_up;
        down_line_value = global_config.adc_range.vote_5va_down;
    }
    else  if (control == lbl5Vd)
    {
        up_line_value = global_config.adc_range.vote_5vd_up;
        down_line_value = global_config.adc_range.vote_5vd_down;
    }
    else  if (control == lbl8Vac)
    {
        up_line_value = global_config.adc_range.vote_8vac_up;
        down_line_value = global_config.adc_range.vote_8vac_down;
    }
    else  if (control == lbl8Vdc)
    {
        up_line_value = global_config.adc_range.vote_8vdc_up;
        down_line_value = global_config.adc_range.vote_8vdc_down;
    }
    else  if (control == lbl12Va)
    {
        up_line_value = global_config.adc_range.vote_12va_up;
        down_line_value = global_config.adc_range.vote_12va_down;
    }
    else  if (control == lbl_12Va)
    {
        up_line_value = global_config.adc_range.vote_x12va_up;
        down_line_value = global_config.adc_range.vote_x12va_down;
    }
    else  if (control == lbl16Vac)
    {
        up_line_value = global_config.adc_range.vote_16vac_up;
        down_line_value = global_config.adc_range.vote_16vac_down;
    }
    else  if (control == lbl_16Vac)
    {
        up_line_value = global_config.adc_range.vote_x16vac_up;
        down_line_value = global_config.adc_range.vote_x16vac_down;
    }
    else  if (control == lbl48Vp)
    {
        up_line_value = global_config.adc_range.vote_48vp_up;
        down_line_value = global_config.adc_range.vote_48vp_down;
    }

    up_line_value = up_line_value / 1000;
    down_line_value = down_line_value / 1000;

    for (int i=0;i<100;i++)
    {
        lineUpLimit->AddXY(i, up_line_value, "e", clRed);
        lineDownLimit->AddXY(i, down_line_value, "b", clRed);
        Series1->AddNull("");
    }

    // �����귶Χ�趨Ϊ+-1V
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
        Bevel10->Left,Bevel10->Top,Bevel10->Width,Bevel10->Height,
        bmp->Canvas->Handle, 0, 0, Bevel10->Width,Bevel10->Height, blend);

    // ����CheckBox�ı���
    // bmp->Canvas->Pixels[rbDhcpEnabled->Left+1][rbDhcpEnabled->Top+1];
    rbDhcpEnabled->Color = bmp->Canvas->Brush->Color;
    rbStaticIpEnabled->Color = bmp->Canvas->Brush->Color;
    cbRunningTimer->Color = bmp->Canvas->Brush->Color;
    cbRebootCount->Color = bmp->Canvas->Brush->Color;
    cbLockedString->Color = bmp->Canvas->Brush->Color;
    cbGlobalDspName->Color = bmp->Canvas->Brush->Color;
    cbPresetAutoSaved->Color = bmp->Canvas->Brush->Color;
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
}
static void MoveWatchPanel(int panel_id, TForm1 * form, String label, int left)
{
    form->watch_panel_inner[panel_id-1]->Left = left;
    if (panel_id <= 16)
        form->watch_panel_inner[panel_id-1]->Visible = (panel_id <= REAL_INPUT_DSP_NUM);
    else
        form->watch_panel_inner[panel_id-1]->Visible = (panel_id-16 <= REAL_OUTPUT_DSP_NUM);
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

    form->output_dsp_btn[panel_id-1]->Visible = (panel_id <= REAL_OUTPUT_DSP_NUM);
    form->output_eq_btn[panel_id-1]->Visible = (panel_id <= REAL_OUTPUT_DSP_NUM);
    form->output_comp_btn[panel_id-1]->Visible = (panel_id <= REAL_OUTPUT_DSP_NUM);
    form->output_invert_btn[panel_id-1]->Visible = (panel_id <= REAL_OUTPUT_DSP_NUM);
    form->output_mute_btn[panel_id-1]->Visible = (panel_id <= REAL_OUTPUT_DSP_NUM);
    form->output_number_btn[panel_id-1]->Visible = (panel_id <= REAL_OUTPUT_DSP_NUM);
    form->output_level_edit[panel_id-1]->Visible = (panel_id <= REAL_OUTPUT_DSP_NUM);
    form->output_level_trackbar[panel_id-1]->Visible = (panel_id <= REAL_OUTPUT_DSP_NUM);
    form->output_dsp_name[panel_id-1]->Visible = (panel_id <= REAL_OUTPUT_DSP_NUM);
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
    pnlOperator->Width = REAL_INPUT_DSP_NUM * PANEL_WIDTH + imgPresetBg->Width + REAL_OUTPUT_DSP_NUM * PANEL_WIDTH;
    pnlOperator->Width = Math::Max(pnlOperator->Width, Width-16);
        //HorzScrollBar->Visible = (pnlOperator->Width > Width);

    pnlMix->Width = REAL_INPUT_DSP_NUM * PANEL_WIDTH;                               

    //--------------------------------------------
    // input
    int input_panel_left = (pnlOperator->Width - (REAL_INPUT_DSP_NUM * PANEL_WIDTH + imgPresetBg->Width + REAL_OUTPUT_DSP_NUM * PANEL_WIDTH))/2;
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

    input_panel_bkground->Picture->Bitmap->Width = REAL_INPUT_DSP_NUM * PANEL_WIDTH;
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

    for (int i=2;i<=INPUT_DSP_NUM;i++)
    {
        MoveInputPanel(i, this);
        MoveWatchPanel(i, this, String((char)('A'-1+i))+" ("+IntToStr(i)+")", (i-1) * PANEL_WIDTH + input_panel_left);
    }
    MoveWatchPanel(1, this, String((char)('A'-1+1))+" ("+IntToStr(1)+")", (1-1) * PANEL_WIDTH + input_panel_left);

    //--------------------------------------
    // mix master
    int mix_panel_left = input_panel_left + REAL_INPUT_DSP_NUM * PANEL_WIDTH;
    imgMasterMixBg->Left = mix_panel_left;
    imgPresetBg->Left = mix_panel_left;
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
    // ���
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

    output_panel_bkground->Width = REAL_OUTPUT_DSP_NUM * PANEL_WIDTH;
    output_panel_bkground->Picture->Bitmap->Width = output_panel_bkground->Width;
    output_panel_bkground->Canvas->Draw(
        -output_panel_bkground->Left,
        -output_panel_bkground->Top,
        imgBody->Picture->Graphic);

    {
        // ����WatchLevel�Ĳ���
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
        MoveWatchPanel(i+16, this, String(i), imgMasterMixBg->Left + imgMasterMixBg->Width + (i-1) * PANEL_WIDTH);
    }

    //============================
    //----------------------------------
    // ����pnlmix����
    pnlmix_background->Picture->Bitmap->Width = REAL_INPUT_DSP_NUM * PANEL_WIDTH;  // xDO: ԭ����17��������automix������ֻ������������
    for (int i=1;i<REAL_INPUT_DSP_NUM;i++)   // xDO: ԭ����17��������automix������ֻ������������
    {
        TRect templet_image_rect = pnlmix_background->BoundsRect;
        templet_image_rect.Right = PANEL_WIDTH;

        TRect dest_rect = TRect(i*PANEL_WIDTH,
                                templet_image_rect.Top,
                                (i+1)*PANEL_WIDTH,
                                templet_image_rect.Bottom);
        pnlmix_background->Canvas->CopyRect(dest_rect, pnlmix_background->Canvas, templet_image_rect);
    }

    // ����PnlMix
    mix_mute_btn[0] = pnlmix_mute;
    for (int i=2;i<=REAL_INPUT_DSP_NUM;i++)    // xDO: ԭ����17��������automix������ֻ������������
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
    ApplyConfigToUI();

    String cmd_text = D1608CMD_CONTROL_FLAG;
    cmd_text = cmd_text+String("config.use_global_name=")+(cbGlobalDspName->Checked?"on":"off");
    SendCmd2(cmd_text+D1608CMD_TAIL);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::cbPresetAutoSavedClick(TObject *Sender)
{
    String cmd_text = D1608CMD_CONTROL_FLAG;
    cmd_text = cmd_text+String("config.auto_saved=")+(cbPresetAutoSaved->Checked?"on":"off");
    SendCmd(cmd_text+D1608CMD_TAIL);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btnRebootDeviceClick(TObject *Sender)
{
    if (Application->MessageBox("�������������豸��ȷ��������", "ȷ�������豸����", MB_OKCANCEL|MB_ICONWARNING) != IDOK)
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
String function_key_value[3] =
{
    "next_preset",
    "volume_up",
    "volume_down",
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
    String cmd_text = D1608CMD_CONTROL_FLAG;

    cmd_text = cmd_text+"config.lock_key="+String(cbLockUpDownMenuKey->Checked?"on":"off");
    SendCmd2(cmd_text+D1608CMD_TAIL);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btnSaveFlashToFileClick(TObject *Sender)
{
    if (package_list.size() != 0)
    {
        ShowMessage("�豸æ");
        return;
    }

    SaveDialog1->Filter = "smc config file(*.smc)|*.smc";

    if (SaveDialog1->Execute())
    {
        if (!udpControl->Active)
        {
            all_config_map[cur_preset_id-1] = config_map;
            // �ѻ�������Ϊ�߼�preset����׺smc
            // �����������
            memset(&smc_config, 0, sizeof(smc_config));
            global_config.active_preset_id = cur_preset_id;

            smc_config.file_version = file_version;
            smc_config.global_config = global_config;

            // ǰ80k�Ǵ���ʹ�ã�2kһҳ
            for (int i=0;i<8;i++)
            {
                SavePresetById(i+1, 0, &all_config_map[i].input_dsp[0], smc_config.device_flash_dump);
                SavePresetById(i+1, 1, &all_config_map[i].input_dsp[8], smc_config.device_flash_dump);
                SavePresetById(i+1, 2, &all_config_map[i].output_dsp[0], smc_config.device_flash_dump);
                SavePresetById(i+1, 3, &all_config_map[i].output_dsp[8], smc_config.device_flash_dump);
            }

            // ��ɣ�д���ļ�
            TFileStream * file = new TFileStream(SaveDialog1->FileName, fmCreate);
            if (!file)
            {
                ShowMessage("���ļ�ʧ��");
                return;
            }
            file->WriteBuffer(&smc_config, sizeof(smc_config));
            delete file;
        }
        else
        {
            // ����������Ϊflash dump����ȡ��Ϻ�ת����smc
            save_device_to_file_filename = SaveDialog1->FileName;

            // �滻�ļ���
            String cmd_text = D1608CMD_CONTROL_FLAG;
            cmd_text = cmd_text+"config.filename=s"+ExtractFileName(SaveDialog1->FileName).SubString(1,16);
            SendCmd2(cmd_text+D1608CMD_TAIL);

            memset(smc_config.device_flash_dump, 0, sizeof(smc_config.device_flash_dump));
            smc_config.file_version = file_version;
            smc_config.device_version = last_connection.data.version;
            memcpy(smc_config.cpu_id, last_connection.data.cpu_id, sizeof(smc_config.cpu_id));
            memcpy(smc_config.mac, last_connection.data.mac, sizeof(smc_config.mac));

            for (int address=PRESET_START_PAGE;address<0x8000000+256*1024;address+=1024)
            {
                // ���豸������ͬ������
                FlashRW_Data flash_rw;
                strcpy(flash_rw.flag, READ_FLASH_FLAG);
                //  ÿ��2k����
                flash_rw.start_address = address;
                flash_rw.end_address = 0x8000000+256*1024;
                flash_rw.verify -= UdpPackageVerifyDiff((unsigned char*)&flash_rw, offsetof(FlashRW_Data, data));

                TPackage package = {0};
                memcpy(package.data, &flash_rw, offsetof(FlashRW_Data, data));
                package.udp_port = UDP_PORT_READ_FLASH;
                package.data_size = offsetof(FlashRW_Data, data);//sizeof(flash_rw);

                package_list.insert(package_list.begin(), package);
            }

            std::reverse(package_list.begin(), package_list.end());

            TPackage package = package_list.back();
            SendBuffer(dst_ip, package.udp_port, package.data, package.data_size);

            restor_delay_count = 15;
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
        ShowMessage("�豸æ");
        return;
    }

    OpenDialog1->Filter = "smc config file(*.smc)|*.smc";
    if (OpenDialog1->Execute())
    {
        memset(&smc_config, 0, sizeof(smc_config));

        TFileStream * file = new TFileStream(OpenDialog1->FileName, fmOpenRead);
        if (!file)
        {
            ShowMessage("���ļ�ʧ��");
            return;
        }

        int tmp_file_version;
        file->Seek(sizeof(GlobalConfig), soFromBeginning);
        file->Read(&tmp_file_version, sizeof(tmp_file_version));
        file->Seek(0, soFromBeginning);

        file->ReadBuffer(&smc_config, sizeof(smc_config));
        delete file;

        // �ļ��汾�ж�
        if (smc_config.file_version != file_version)
        {
            ShowMessage("�ļ��汾������");
            return;
        }

        // �滻�ļ���
        smc_config.global_config.import_filename[0] = 'l';
        strncpy(smc_config.global_config.import_filename+1, ExtractFileName(OpenDialog1->FileName).c_str(), 8);
        global_config = smc_config.global_config;

        for (int i=0;i<PRESET_NUM;i++)
        {
            ConfigMap config_map;
            LoadPresetById(i+1, config_map, smc_config.device_flash_dump);
            all_config_map[i] = config_map;
        }

        if (!udpControl->Active)
        {
            // �ѻ�
            SetPresetId(global_config.active_preset_id);
            CloseDspDetail();
            tmDelayUpdateUITimer(NULL);

            // ��ȡ�豸��Ϣ
            last_connection.data.version = smc_config.device_version;
            memcpy(last_connection.data.cpu_id, smc_config.cpu_id, sizeof(smc_config.cpu_id));
            memcpy(last_connection.data.mac, smc_config.mac, sizeof(smc_config.mac));
            // ���µ�����
            lblVersion->Caption = VersionToStr(last_connection.data.version)+ " " +VersionToStr(version);
            lblDeviceInfo->Caption = "VERSION " + VersionToStr(last_connection.data.version);
            lblDeviceInfo->Show();
            UpdateDeviceType();
            UpdateBuildTime();
            String mac;
                mac.sprintf("%02X:%02X:%02X:%02X:%02X:%02X",
                    last_connection.data.mac[0],
                    last_connection.data.mac[1],
                    last_connection.data.mac[2],
                    last_connection.data.mac[3],
                    last_connection.data.mac[4],
                    last_connection.data.mac[5]);
                edtMAC->Text = mac;

            // device_cpuid������־����
            device_cpuid = GetCpuIdString(last_connection.data.cpu_id);
                lvLog->Clear();

            const T_sn_pack * sn_pack_on_flash = (T_sn_pack*)(smc_config.device_flash_dump+(SN_START_PAGE-PRESET_START_PAGE)+active_code_length);
            lblDeviceName->Caption = sn_pack_on_flash->sn;
            lblDeviceName->Show();

            lblCpuId->Caption = "cpu id: "+device_cpuid;
            lblSn->Caption = "sn: " + String(sn_pack_on_flash->sn);
            lblConfigFilename->Caption = "file: " + String(sn_pack_on_flash->name2);

            // ��ȡ��־
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

            lblLogCount->Caption = "��־������"+IntToStr(log_count) + "   MAC������"+IntToStr(mac_count);
        }
        else
        {
            // ����
            // Download To Device
            // ׼�������б���
            //  global_config����
            {
                D1608PresetCmd preset_cmd(version);
                strcpy(preset_cmd.flag, D1608PRESETCMD_PC2FLASH_FLAG);
                preset_cmd.preset = 0x80;
                preset_cmd.store_page = 8;  // ʹ��8��ʾ���һҳ��TODO: ��ﲻ�Ǻ����
                memcpy(preset_cmd.data, &smc_config.global_config, sizeof(smc_config.global_config));
                preset_cmd.verify -= UdpPackageVerifyDiff((unsigned char*)&preset_cmd, sizeof(preset_cmd));

                TPackage package = {0};
                memcpy(package.data, &preset_cmd, sizeof(preset_cmd));
                package.udp_port = UDP_PORT_STORE_PRESET_PC2FLASH;
                package.data_size = sizeof(preset_cmd);

                package_list.insert(package_list.begin(), package);
            }
            // д�����е�preset����
            Using_Page using_page;
            GetUsingPage(&using_page, smc_config.device_flash_dump);
            for (int preset_id=0;preset_id<8;preset_id++)
            {
                D1608PresetCmd preset_cmd(version);
                strcpy(preset_cmd.flag, D1608PRESETCMD_PC2FLASH_FLAG);
                preset_cmd.preset = 0x80+preset_id+1;
                for (int store_page=0;store_page<8;store_page++)
                {
                    // û�е�ҳֱ������� TODO: ��λ����Ҫ��������һ��
                    if (using_page.preset_address[preset_id][store_page/2] == 0)
                        continue;

                    preset_cmd.store_page = store_page;
                    switch(preset_cmd.store_page)
                    {
                    case 0:
                    {
                        const OlaList * input_dict = (const OlaList *)(using_page.preset_address[preset_id][0]+sizeof(Page_Header));
                        int input_dict_size = 
                                sizeof(input_dict->length) +
                                sizeof(input_dict->struct_length) +
                                input_dict->length * sizeof(OlaInfo);

                        //memcpy(preset_cmd.data, input_dict, input_dict_size + input_dict->struct_length*4);
                        memcpy(preset_cmd.data, input_dict, input_dict_size);
                        memcpy(preset_cmd.data+input_dict_size, ((char*)input_dict)+input_dict_size, input_dict->struct_length*4);
                        break;
                    }
                    case 1:
                    {
                        const OlaList * input_dict = (const OlaList *)(using_page.preset_address[preset_id][0]+sizeof(Page_Header));
                        int input_dict_size = 
                                sizeof(input_dict->length) +
                                sizeof(input_dict->struct_length) +
                                input_dict->length * sizeof(OlaInfo);

                        memmove(preset_cmd.data, input_dict, input_dict_size);
                        memcpy(preset_cmd.data+input_dict_size, ((char*)input_dict)+input_dict_size+input_dict->struct_length*4, input_dict->struct_length*4);
                        break;
                    }
                    case 2:
                    {
                        const OlaList * input_dict = (const OlaList *)(using_page.preset_address[preset_id][1]+sizeof(Page_Header));
                        int input_dict_size = 
                                sizeof(input_dict->length) +
                                sizeof(input_dict->struct_length) +
                                input_dict->length * sizeof(OlaInfo);

                        memcpy(preset_cmd.data, input_dict, input_dict_size);
                        memcpy(preset_cmd.data+input_dict_size, ((char*)input_dict)+input_dict_size, input_dict->struct_length*4);
                        break;
                    }
                    case 3:
                    {
                        const OlaList * input_dict = (const OlaList *)(using_page.preset_address[preset_id][1]+sizeof(Page_Header));
                        int input_dict_size = 
                                sizeof(input_dict->length) +
                                sizeof(input_dict->struct_length) +
                                input_dict->length * sizeof(OlaInfo);

                        memcpy(preset_cmd.data, input_dict, input_dict_size);
                        memcpy(preset_cmd.data+input_dict_size, ((char*)input_dict)+input_dict_size+input_dict->struct_length*4, input_dict->struct_length*4);
                        break;
                    }
                    case 4:
                        memcpy(preset_cmd.data, (char*)(using_page.preset_address[preset_id][2]+sizeof(Page_Header)), sizeof(OutputConfigMap)*4);
                        break;
                    case 5:
                        memcpy(preset_cmd.data, (char*)(using_page.preset_address[preset_id][2]+sizeof(Page_Header))+sizeof(OutputConfigMap)*4, sizeof(OutputConfigMap)*4);
                        break;
                    case 6:
                        memcpy(preset_cmd.data, (char*)(using_page.preset_address[preset_id][3]+sizeof(Page_Header)), sizeof(OutputConfigMap)*4);
                        break;
                    case 7:
                        memcpy(preset_cmd.data, (char*)(using_page.preset_address[preset_id][3]+sizeof(Page_Header))+sizeof(OutputConfigMap)*4, sizeof(OutputConfigMap)*4);
                        break;
                    }
                    preset_cmd.verify -= UdpPackageVerifyDiff((unsigned char*)&preset_cmd, sizeof(preset_cmd));

                    TPackage package = {0};
                    memcpy(package.data, &preset_cmd, sizeof(preset_cmd));
                    package.udp_port = UDP_PORT_STORE_PRESET_PC2FLASH;
                    package.data_size = sizeof(preset_cmd);

                    package_list.insert(package_list.begin(), package);
                }
            }

            {
                // ׷��һ��0x89��Ϊ������־
                D1608PresetCmd preset_cmd(version);
                strcpy(preset_cmd.flag, D1608PRESETCMD_PC2FLASH_FLAG);
                preset_cmd.preset = 0x89;
                preset_cmd.verify -= UdpPackageVerifyDiff((unsigned char*)&preset_cmd, sizeof(preset_cmd));

                TPackage package = {0};
                memcpy(package.data, &preset_cmd, sizeof(preset_cmd));
                package.udp_port = UDP_PORT_STORE_PRESET_PC2FLASH;
                package.data_size = sizeof(preset_cmd);

                package_list.insert(package_list.begin(), package);
            }

            std::reverse(package_list.begin(), package_list.end());
            
            TPackage package = package_list.back();
            SendBuffer(dst_ip, package.udp_port, package.data, package.data_size);

            //last_command = UDP_PORT_STORE_PRESET_PC2FLASH;
            //last_restore_package = preset_cmd;
            restor_delay_count = 15;
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
        // ��ʱ����ֹ����ͬ��
        package_list.clear();
        pbBackup->Hide();
        this->Enabled = true;

        // ������ڶ�ȡpresetʧ�ܣ���ô��Ҫ��������
        if (read_one_preset_package_list.size() != 0)
        {
            SendDisconnect();
            udpControl->Active = false;
        }
    }
    else if ((restor_delay_count%5) == 1)
    {
        if(package_list.size() != 0)
        {
            TPackage package = package_list.back();
            SendBuffer(dst_ip, package.udp_port, package.data, package.data_size);

            // ���� �ָ� ����ʹ�õ���ʱ��ʱ��
            memo_debug->Lines->Add(GetTime()+"retry package_list");
        }
        else if (read_one_preset_package_list.size() != 0)
        {
            TPackage package = read_one_preset_package_list.back();
            SendBuffer(dst_ip, package.udp_port, package.data, package.data_size);

            // ���� �ָ� ����ʹ�õ���ʱ��ʱ��
            memo_debug->Lines->Add(GetTime()+"retry read_one_preset_package_list");
        }
        else
        {
            memo_debug->Lines->Add(GetTime()+"�б��");
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
    if (Application->MessageBox("��������ָ��������ã�ȷ�ϲ�����", "ȷ�ϻָ��������ò���", MB_OKCANCEL|MB_ICONWARNING) != IDOK)
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
void __fastcall TForm1::lblDeviceNameDblClick(TObject *Sender)
{
    // ������Ļ
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
    if (Application->MessageBox("���������������PRESET��ȷ�ϲ�����", "ȷ���������PRESET����", MB_OKCANCEL|MB_ICONWARNING) != IDOK)
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
        // ��ʱ����ֹ����ͬ��
        sendcmd_list.clear();
        // ����Ϊʧ��
        keep_live_count = CONTROL_TIMEOUT_COUNT;
    }
    else if ((sendcmd_delay_count%5) == 1)
    {
        TPackage package = sendcmd_list.back();

        // ��������
        SendBuffer(dst_ip, package.udp_port, package.data, package.data_size);

        // ���� �ָ� ����ʹ�õ���ʱ��ʱ��
        String cmd = (char*)(sendcmd_list.end()-1)->data;
        memo_debug->Lines->Add(GetTime()+" retry sendcmd:"+cmd);
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
            log_strs->Add(line);
        }
        else
        {
            String line = "\t"+lvLog->Items->Item[i]->SubItems->Strings[0]+"\t"+lvLog->Items->Item[i]->SubItems->Strings[1]+"\t"+lvLog->Items->Item[i]->SubItems->Strings[2];
            mac_strs->Add(line);
        }
    }

    String path = ExtractFilePath(Application->ExeName);

    // �ϲ���־
    if (FileExists(path+device_cpuid+".log"))
    {
        // �ϲ���־
        TStrings * log_data = new TStringList();
        log_data->LoadFromFile(path+device_cpuid+".log");

        // ɾ��̧ͷ
        if (log_data->Count > 0 && log_data->Strings[0].Pos("��ַ")!=0)
        {
            log_data->Delete(0);
        }

        MergeLog(log_strs, log_data);
        delete log_data;

        // �ϲ�mac
        TStrings * mac_data = new TStringList();
        mac_data->LoadFromFile(path+device_cpuid+".log");
        MergeMac(mac_strs, mac_data);
        delete mac_data;
    }

    log_strs->AddStrings(mac_strs);

    // ׷��̧ͷ
    String head = lvLog->Column[0]->Caption + "\t"
                + lvLog->Column[1]->Caption + "\t"
                + lvLog->Column[2]->Caption + "\t"
                + lvLog->Column[3]->Caption + "\t"
                + lvLog->Column[4]->Caption;
    log_strs->Insert(0, head);

    log_strs->SaveToFile(path+device_cpuid+".log");

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

        // ����MAC��ַ
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
            ShowMessage("�������");
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
    // �Ƿ�input
    if (xy.x >= input_panel_trackbar->Left && xy.x <= input_panel_trackbar->Left+REAL_INPUT_DSP_NUM*PANEL_WIDTH)
    {
        int input_dsp_id = (xy.x - input_panel_trackbar->Left) / PANEL_WIDTH + 1;
        if (input_dsp_id >= 1 && input_dsp_id <= REAL_INPUT_DSP_NUM)
        {
            selected_channel.channel_type = ctInput;
            selected_channel.channel_id = input_dsp_id;

            Copy1->Caption = "Copy Channel Input " + IntToStr(input_dsp_id);
            Copy1->Enabled = true;
            Copy1->Tag = input_dsp_id;
        }
    }
    else if (xy.x >= output_panel_trackbar->Left && xy.x <= output_panel_trackbar->Left+REAL_OUTPUT_DSP_NUM*PANEL_WIDTH)
    {
        // �Ƿ�output
        int output_dsp_id = (xy.x - output_panel_trackbar->Left) / PANEL_WIDTH + 1;
        if (output_dsp_id >= 1 && output_dsp_id <= REAL_OUTPUT_DSP_NUM)
        {
            selected_channel.channel_type = ctOutput;
            selected_channel.channel_id = output_dsp_id;

            Copy1->Caption = "Copy Channel Output " + IntToStr(output_dsp_id);
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

    Paste1->Enabled = (selected_channel.channel_type != ctNone)
                   //&& (selected_channel.channel_type == copied_channel.channel_type)
                   && (selected_channel.channel_id != copied_channel.channel_id);
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
    dest = src;
}
static void CloneChannelData(OutputConfigMap &dest, OutputConfigMap &src)
{
    dest = src;
}
static void CloneChannelData(InputConfigMap &dest, OutputConfigMap &src)
{
    dest.eq_switch        = src.eq_switch      ;
    dest.comp_switch      = src.comp_switch    ;
    //dest.auto_switch    =                    ;
    dest.invert_switch    = src.invert_switch  ;
    //dest.noise_switch   =                    ;
    dest.mute_switch      = src.mute_switch    ;
    //dest.phantom_switch =                    ;
    dest.level_a          = src.level_a        ;
    dest.level_b          = src.level_b        ;
    dest.gain             = src.gain           ;
    dest.delay            = src.delay          ;
    memcpy(dest.filter, src.filter, sizeof(dest.filter));
    memcpy(dest.dsp_name, src.dsp_name, sizeof(dest.dsp_name));
    //                    = src.ratio          ;
    //                    = src.threshold      ;
    //                    = src.attack_time    ;
    //                    = src.release_time   ;
    //                    = src.comp_gain      ;
    //                    = src.auto_time      ;
}
static void CloneChannelData(OutputConfigMap &dest, InputConfigMap &src)
{
    dest.eq_switch        = src.eq_switch      ;
    dest.comp_switch      = src.comp_switch    ;
    //dest.auto_switch    =                    ;
    dest.invert_switch    = src.invert_switch  ;
    //dest.noise_switch   =                    ;
    dest.mute_switch      = src.mute_switch    ;
    //dest.phantom_switch =                    ;
    dest.level_a          = src.level_a        ;
    dest.level_b          = src.level_b        ;
    dest.gain             = src.gain           ;
    dest.delay            = src.delay          ;
    memcpy(dest.filter, src.filter, sizeof(dest.filter));
    memcpy(dest.dsp_name, src.dsp_name, sizeof(dest.dsp_name));
    //                    = src.ratio          ;
    //                    = src.threshold      ;
    //                    = src.attack_time    ;
    //                    = src.release_time   ;
    //                    = src.comp_gain      ;
    //                    = src.auto_time      ;
}
void __fastcall TForm1::Paste1Click(TObject *Sender)
{
    Paste1->Enabled = (selected_channel.channel_type != ctNone)
                   //&& (selected_channel.channel_type == copied_channel.channel_type)
                   && (selected_channel.channel_id != copied_channel.channel_id);

    // �������ɽ����˶�
    if (Paste1->Enabled)
    {
        ShowMessage("Copy channel "+IntToStr(copied_channel.channel_id)+" to channel "+IntToStr(selected_channel.channel_id));

        // ��Ҫ��������������״̬����������
        if (selected_channel.channel_type == ctInput)
        {
            if (copied_channel.channel_type == ctInput)
                CloneChannelData(config_map.input_dsp[selected_channel.channel_id-1],
                                 config_map.input_dsp[copied_channel.channel_id-1]);
            else if (copied_channel.channel_type == ctOutput)
                CloneChannelData(config_map.input_dsp[selected_channel.channel_id-1],
                                 config_map.output_dsp[copied_channel.channel_id-1]);
        }
        else if (selected_channel.channel_type == ctOutput)
        {
            if (copied_channel.channel_type == ctInput)
                CloneChannelData(config_map.output_dsp[selected_channel.channel_id-1],
                                 config_map.input_dsp[copied_channel.channel_id-1]);
            else if (copied_channel.channel_type == ctOutput)
                CloneChannelData(config_map.output_dsp[selected_channel.channel_id-1],
                                 config_map.output_dsp[copied_channel.channel_id-1]);
        }
        ApplyConfigToUI();
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::FormResize(TObject *Sender)
{
    need_resize = true;
}
//---------------------------------------------------------------------------
void TForm1::StartReadCurrentPreset()
{
    memo_debug->Lines->Add(GetTime()+"ͬ����ǰPreset����");
    read_one_preset_package_list.clear();
    for (int store_page=0;store_page<8;store_page++)
    {
        D1608PresetCmd preset_cmd(version);
        preset_cmd.preset = 0xFF; // ��ȡpreset
        // ��0ҳ��ȡ
        preset_cmd.store_page = store_page;
        preset_cmd.verify -= UdpPackageVerifyDiff((unsigned char*)&preset_cmd, sizeof(preset_cmd));

        TPackage package = {0};
        package.udp_port = UDP_PORT_READ_PRESET;
        memcpy(package.data, &preset_cmd, offsetof(D1608PresetCmd, data));
        package.data_size = offsetof(D1608PresetCmd, data);

        read_one_preset_package_list.insert(read_one_preset_package_list.begin(), package);
    }

    reverse(read_one_preset_package_list.begin(), read_one_preset_package_list.end());

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
    }    // ��汾��һ�£���ʾ��ɫ
    else if (Item->SubItems->Strings[8] == "YELLOW")
    {
        lvDevice->Canvas->Font->Color = clYellow;
    }
    // �ΰ汾�Ų�һ�£���ʾ��ɫ
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

    if (event_desc.Pos("����") != 0)
    {
        Sender->Canvas->Font->Color = clRed;
    }
    else if (event_desc.Pos("����") != 0)
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

    // ��ԭ�ȵ��豸���Ͷ�����Ϣ
    SendDisconnect();

    REAL_INPUT_DSP_NUM = 16;
    REAL_OUTPUT_DSP_NUM = 16;
    need_resize = true;

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
    case 0 :        data = this->adc_ex_max._3_3vd       ;        range = global_config.adc_range.vote_3v3_up;                 break;
    case 1 :        data = this->adc_ex_max.base         ;        range = global_config.adc_range.vote_3v3m_up;                break;
    case 2 :        data = this->adc_ex_max._5vd         ;        range = global_config.adc_range.vote_5vd_up;                 break;
    case 3 :        data = this->adc_ex_max._8vdc        ;        range = global_config.adc_range.vote_8vdc_up;                break;
    case 4 :        data = this->adc_ex_max._8vac        ;        range = global_config.adc_range.vote_8vac_up;                break;
    case 5 :        data = this->adc_ex_max._8vad        ;        range = global_config.adc_range.vote_8vad_up;                break;
    case 6 :        data = this->adc_ex_max._x16vac      ;        range = global_config.adc_range.vote_x16vac_up;              break;
    case 7 :        data = this->adc_ex_max._x16va       ;        range = global_config.adc_range.vote_x16va_up;               break;
    case 8 :        data = this->adc_ex_max._50vpc       ;        range = global_config.adc_range.vote_50vpc_up;               break;
    case 9 :        data = this->adc_ex_max._50vp        ;        range = global_config.adc_range.vote_50vp_up;                break;
    case 10:        data = this->adc_ex_max._48vp        ;        range = global_config.adc_range.vote_48vp_up;                break;
    case 11:        data = this->adc_ex_max._5va         ;        range = global_config.adc_range.vote_5va_up;                 break;
    case 12:        data = this->adc_ex_max._x12va       ;        range = global_config.adc_range.vote_x12va_up;               break;
    case 13:        data = this->adc_ex_max._12va        ;        range = global_config.adc_range.vote_12va_up;                break;
    case 14:        data = this->adc_ex_max._16va        ;        range = global_config.adc_range.vote_16va_up;                break;
    case 15:        data = this->adc_ex_max._16vac       ;        range = global_config.adc_range.vote_16vac_up;               break;
    case 17:        data = this->adc_ex_max._8va_current ;        range = global_config.adc_range.current_8vd_up;              break;
    case 18:        data = this->adc_ex_max._8vd_current ;        range = global_config.adc_range.current_8vd_up;              break;
    case 19:        data = this->adc_ex_max._16v_current ;        range = global_config.adc_range.current_16v_up;              break;
    case 20:        data = this->adc_ex_max._x16v_current;        range = global_config.adc_range.current_x16v_up;             break;
    case 21:        data = this->adc_ex_max._50v_current ;        range = global_config.adc_range.current_50v_up;              break;
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
    case 0 :        data = this->adc_ex_min._3_3vd       ;        range = global_config.adc_range.vote_3v3_down;                 break;
    case 1 :        data = this->adc_ex_min.base         ;        range = global_config.adc_range.vote_3v3m_down;                break;
    case 2 :        data = this->adc_ex_min._5vd         ;        range = global_config.adc_range.vote_5vd_down;                 break;
    case 3 :        data = this->adc_ex_min._8vdc        ;        range = global_config.adc_range.vote_8vdc_down;                break;
    case 4 :        data = this->adc_ex_min._8vac        ;        range = global_config.adc_range.vote_8vac_down;                break;
    case 5 :        data = this->adc_ex_min._8vad        ;        range = global_config.adc_range.vote_8vad_down;                break;
    case 6 :        data = this->adc_ex_min._x16vac      ;        range = global_config.adc_range.vote_x16vac_down;              break;
    case 7 :        data = this->adc_ex_min._x16va       ;        range = global_config.adc_range.vote_x16va_down;               break;
    case 8 :        data = this->adc_ex_min._50vpc       ;        range = global_config.adc_range.vote_50vpc_down;               break;
    case 9 :        data = this->adc_ex_min._50vp        ;        range = global_config.adc_range.vote_50vp_down;                break;
    case 10:        data = this->adc_ex_min._48vp        ;        range = global_config.adc_range.vote_48vp_down;                break;
    case 11:        data = this->adc_ex_min._5va         ;        range = global_config.adc_range.vote_5va_down;                 break;
    case 12:        data = this->adc_ex_min._x12va       ;        range = global_config.adc_range.vote_x12va_down;               break;
    case 13:        data = this->adc_ex_min._12va        ;        range = global_config.adc_range.vote_12va_down;                break;
    case 14:        data = this->adc_ex_min._16va        ;        range = global_config.adc_range.vote_16va_down;                break;
    case 15:        data = this->adc_ex_min._16vac       ;        range = global_config.adc_range.vote_16vac_down;               break;
    case 17:        data = this->adc_ex_min._8va_current ;        range = global_config.adc_range.current_8vd_down;              break;
    case 18:        data = this->adc_ex_min._8vd_current ;        range = global_config.adc_range.current_8vd_down;              break;
    case 19:        data = this->adc_ex_min._16v_current ;        range = global_config.adc_range.current_16v_down;              break;
    case 20:        data = this->adc_ex_min._x16v_current;        range = global_config.adc_range.current_x16v_down;             break;
    case 21:        data = this->adc_ex_min._50v_current ;        range = global_config.adc_range.current_50v_down;              break;
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
    String vote_data_str = "";
    for (int i=0;i<ValueListEditor2->RowCount;i++)
    {
        String name = ValueListEditor2->Cells[0][i];
        String cur_data = ValueListEditor2->Cells[1][i];
        String max = vleAdcMax->Cells[1][i];
        String min = vleAdcMin->Cells[1][i];
        vote_data_str = vote_data_str + name + "\t"
                        + cur_data + "\t"
                        + max + "\t"
                        + min + "\n";
    }

    mmVoteData2Clip->Text = vote_data_str;
    mmVoteData2Clip->SelectAll();
    mmVoteData2Clip->CopyToClipboard();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btnClearDataAndTimeClick(TObject *Sender)
{
    if (Application->MessageBox("��������ָ��������ã�ȷ�ϲ�����", "ȷ�ϻָ��������ò���", MB_OKCANCEL|MB_ICONWARNING) != IDOK)
    {
        return;
    }

    {                         
        // ����
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
    }
}
//---------------------------------------------------------------------------
void TForm1::CloseControlLink(String reason)
{
    memo_debug->Lines->Add(GetTime()+"��λ���ر�����: " + reason);
    SendDisconnect();
    udpControl->Active = false;
    broken_count++;
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
    //SendBuffer(dst_ip, UDP_PORT_GET_DEBUG_INFO_EX, "", 1);
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

