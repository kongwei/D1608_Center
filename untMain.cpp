//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "untMain.h"
#include "untSetMAC.h"
#include "untFlashReader.h"
#include "untUtils.h"

#include <winsock2.h>
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

const String compile_time = __DATE__ " " __TIME__;

static String GetTime()
{
    return FormatDateTime("nn:ss.zzz", Now())+" ";
}

TForm1 *Form1;
ConfigMap all_config_map[PRESET_NUM];
ConfigMap config_map;
GlobalConfig global_config = {0};
int REAL_INPUT_DSP_NUM = 16;
int REAL_OUTPUT_DSP_NUM = 16;

extern String GetMacList();

void operator <<(ConfigMap & result, ConfigMapX config_map)
{
    memcpy(&result, &config_map, sizeof(config_map));
}
// 用于复制设备的flaah数据
struct SmcConfig
{
    GlobalConfig global_config;
    char pad1[2048-sizeof(GlobalConfig)];
    ConfigMapX all_config_map[PRESET_NUM];
    char pad2[80*1024-sizeof(ConfigMap)*PRESET_NUM];
    char device_flash_dump[128*1024];
};
static SmcConfig smc_config;
static bool on_loading = false;
static String last_device_id;
static Word enter_key = VK_RETURN;
//static char head[] = "\x53\x65\x74\x50\x61\x72\x61\x00\x00\x00\x4D\x41\x54\x31\x36\x31\x30\x43\x6F\x6E\x66\x69\x67\x44\x61\x74\x61\x00\x00\x00";
static BLENDFUNCTION blend = {AC_SRC_OVER, 0, 200, 0};
static TTime startup_time;

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
    T_slp_pack data;
};
//------------------------------------------------
// 版本兼容信息
static UINT version = 0x01000002;
/*static UINT version_list[] =
{
    0x01000002,
    0x00000000
};*/
// 返回YES或者NO
// version_list以0结尾
String IsCompatibility(T_slp_pack slp_pack)
{
    UINT ctrl_app_protocol_version = version & 0xFF000000;
    UINT app_protocol_version = slp_pack.version & 0xFF000000;
    if (ctrl_app_protocol_version == app_protocol_version)
    {
        return "YES";
    }
    else
    {
        return "NO";
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
    return (char*)p_data - p_config_map;
}

String FormatFloat(float value, int precise)
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

CompConfig ratio_config = {0, 100, 100, 100, 2};
CompConfig threshold_config = {-320, 0, 0, 10, 3};
CompConfig attack_config = {1, 20000, 640, 10, 3};
CompConfig release_config = {10, 50000, 10000, 10, 3};
CompConfig gain_config = {0, 240, 0, 10, 3};

// 版本配置表
struct VersionFunction
{
    char name[100];
    //48V
    bool is_48v;
    //增益切换
    char gain_function[200];
    //压缩
    bool is_comp;
    //进
    int input_channel_count;
    //出
    int output_channel_count;
    //电压检测
    bool is_vote_check;
    //有没有OLED
    bool is_oled;
    //DSP数量
    int dsp_count;
    //输入PEQ数
    int input_peq_count;
    //输出PEQ数
    int output_peq_count;
    //输入延时
    int input_delay;
    //输出延时
    int output_delay;
};

static VersionFunction version_function_list[] =
{   //name   48v    gain  增益                                        压缩    I   O   电压   OLED DSP i/o PEQ i/o delay
    {"C4D",  false, "i24dBu|o24dBu",                                  false,  4,  4,  false, false, 0, 6, 6, 160, 160},
    {"C4H",  false, "i24dBu|o24dBu",                                  false,  4,  8,  false, false, 0, 6, 6, 160, 160},
    {"C4L",  false, "i24dBu|o24dBu",                                  false,  4,  12, false, false, 0, 6, 6, 160, 160},
    {"C4P",  false, "i24dBu|o24dBu",                                  false,  4,  16, false, false, 0, 6, 6, 160, 160},
    {"C8D",  false, "i24dBu|o24dBu",                                  false,  8,  4,  false, false, 0, 6, 6, 160, 160},
    {"C8H",  false, "i24dBu|o24dBu",                                  false,  8,  8,  false, false, 0, 6, 6, 160, 160},
    {"C8L",  false, "i24dBu|o24dBu",                                  false,  8,  12, false, false, 0, 6, 6, 160, 160},
    {"C8P",  false, "i24dBu|o24dBu",                                  false,  8,  16, false, false, 0, 6, 6, 160, 160},
    {"C12D", false, "i24dBu|o24dBu",                                  false,  12, 4,  false, false, 0, 6, 6, 160, 160},
    {"C12H", false, "i24dBu|o24dBu",                                  false,  12, 8,  false, false, 0, 6, 6, 160, 160},
    {"C12L", false, "i24dBu|o24dBu",                                  false,  12, 12, false, false, 0, 6, 6, 160, 160},
    {"C12P", false, "i24dBu|o24dBu",                                  false,  12, 16, false, false, 0, 6, 6, 160, 160},
    {"C16D", false, "i24dBu|o24dBu",                                  false,  16, 4,  false, false, 0, 6, 6, 160, 160},
    {"C16H", false, "i24dBu|o24dBu",                                  false,  16, 8,  false, false, 0, 6, 6, 160, 160},
    {"C16L", false, "i24dBu|o24dBu",                                  false,  16, 12, false, false, 0, 6, 6, 160, 160},
    {"C16P", false, "i24dBu|o24dBu",                                  false,  16, 16, false, false, 0, 6, 6, 160, 160},
    {"M4D",  false, "i10dBv|i24dBu|o10dBv|o24dBu",                    false,  4,  4,  false, true,  0, 7, 7, 320, 320},
    {"M4H",  false, "i10dBv|i24dBu|o10dBv|o24dBu",                    false,  4,  8,  false, true,  0, 7, 7, 320, 320},
    {"M4L",  false, "i10dBv|i24dBu|o10dBv|o24dBu",                    false,  4,  12, false, true,  0, 7, 7, 320, 320},
    {"M4P",  false, "i10dBv|i24dBu|o10dBv|o24dBu",                    false,  4,  16, false, true,  0, 7, 7, 320, 320},
    {"M8D",  false, "i10dBv|i24dBu|o10dBv|o24dBu",                    false,  8,  4,  false, true,  0, 7, 7, 320, 320},
    {"M8H",  false, "i10dBv|i24dBu|o10dBv|o24dBu",                    false,  8,  8,  false, true,  0, 7, 7, 320, 320},
    {"M8L",  false, "i10dBv|i24dBu|o10dBv|o24dBu",                    false,  8,  12, false, true,  0, 7, 7, 320, 320},
    {"M8P",  false, "i10dBv|i24dBu|o10dBv|o24dBu",                    false,  8,  16, false, true,  0, 7, 7, 320, 320},
    {"M12D", false, "i10dBv|i24dBu|o10dBv|o24dBu",                    false,  12, 4,  false, true,  0, 7, 7, 320, 320},
    {"M12H", false, "i10dBv|i24dBu|o10dBv|o24dBu",                    false,  12, 8,  false, true,  0, 7, 7, 320, 320},
    {"M12L", false, "i10dBv|i24dBu|o10dBv|o24dBu",                    false,  12, 12, false, true,  0, 7, 7, 320, 320},
    {"M12P", false, "i10dBv|i24dBu|o10dBv|o24dBu",                    false,  12, 16, false, true,  0, 7, 7, 320, 320},
    {"M16D", false, "i10dBv|i24dBu|o10dBv|o24dBu",                    false,  16, 4,  false, true,  0, 7, 7, 320, 320},
    {"M16H", false, "i10dBv|i24dBu|o10dBv|o24dBu",                    false,  16, 8,  false, true,  0, 7, 7, 320, 320},
    {"M16L", false, "i10dBv|i24dBu|o10dBv|o24dBu",                    false,  16, 12, false, true,  0, 7, 7, 320, 320},
    {"M16P", false, "i10dBv|i24dBu|o10dBv|o24dBu",                    false,  16, 16, false, true,  0, 7, 7, 320, 320},
    {"S4D",  true,  "iMIC|i10dBv|i22dBu|i24dBu|o10dBv|o22dBu|o24dBu", true,   4,  4,  true,  true,  0, 7, 7, 320, 320},
    {"S4H",  true,  "iMIC|i10dBv|i22dBu|i24dBu|o10dBv|o22dBu|o24dBu", true,   4,  8,  true,  true,  0, 7, 7, 320, 320},
    {"S4L",  true,  "iMIC|i10dBv|i22dBu|i24dBu|o10dBv|o22dBu|o24dBu", true,   4,  12, true,  true,  0, 7, 7, 320, 320},
    {"S4P",  true,  "iMIC|i10dBv|i22dBu|i24dBu|o10dBv|o22dBu|o24dBu", true,   4,  16, true,  true,  0, 7, 7, 320, 320},
    {"S8D",  true,  "iMIC|i10dBv|i22dBu|i24dBu|o10dBv|o22dBu|o24dBu", true,   8,  4,  true,  true,  0, 7, 7, 320, 320},
    {"S8H",  true,  "iMIC|i10dBv|i22dBu|i24dBu|o10dBv|o22dBu|o24dBu", true,   8,  8,  true,  true,  0, 7, 7, 320, 320},
    {"S8L",  true,  "iMIC|i10dBv|i22dBu|i24dBu|o10dBv|o22dBu|o24dBu", true,   8,  12, true,  true,  0, 7, 7, 320, 320},
    {"S8P",  true,  "iMIC|i10dBv|i22dBu|i24dBu|o10dBv|o22dBu|o24dBu", true,   8,  16, true,  true,  0, 7, 7, 320, 320},
    {"S12D", true,  "iMIC|i10dBv|i22dBu|i24dBu|o10dBv|o22dBu|o24dBu", true,   12, 4,  true,  true,  0, 7, 7, 320, 320},
    {"S12H", true,  "iMIC|i10dBv|i22dBu|i24dBu|o10dBv|o22dBu|o24dBu", true,   12, 8,  true,  true,  0, 7, 7, 320, 320},
    {"S12L", true,  "iMIC|i10dBv|i22dBu|i24dBu|o10dBv|o22dBu|o24dBu", true,   12, 12, true,  true,  0, 7, 7, 320, 320},
    {"S12P", true,  "iMIC|i10dBv|i22dBu|i24dBu|o10dBv|o22dBu|o24dBu", true,   12, 16, true,  true,  0, 7, 7, 320, 320},
    {"S16D", true,  "iMIC|i10dBv|i22dBu|i24dBu|o10dBv|o22dBu|o24dBu", true,   16, 4,  true,  true,  0, 7, 7, 320, 320},
    {"S16H", true,  "iMIC|i10dBv|i22dBu|i24dBu|o10dBv|o22dBu|o24dBu", true,   16, 8,  true,  true,  0, 7, 7, 320, 320},
    {"S16L", true,  "iMIC|i10dBv|i22dBu|i24dBu|o10dBv|o22dBu|o24dBu", true,   16, 12, true,  true,  0, 7, 7, 320, 320},
    {"S16P", true,  "iMIC|i10dBv|i22dBu|i24dBu|o10dBv|o22dBu|o24dBu", true,   16, 16, true,  true,  0, 7, 7, 320, 320},
    {"",     false, "iMIC|i10dBv|i22dBu|i24dBu|o10dBv|o22dBu|o24dBu", true,   16, 16, false, false, 0, 7, 7, 320, 320},
};
static VersionFunction GetVersionConfig(String name)
{
    int i=0;
    while (String(version_function_list[i].name) != "")
    {
        if (String(version_function_list[i].name) == name)
            break;
        i++;
    }
    return version_function_list[i];
}


String Ration2String(double ratio)
{
    if (ratio == 0)
        return "∞";
    else
        return FormatFloat(100.0 / ratio, ratio_config.precise);
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
int GetLocalIpList(TStrings * IpList)
{
    int result = 0;

    try
    {
        char HostName[MAX_PATH + 1] = {0};
        int NameLen = gethostname(HostName, MAX_PATH);
        if (NameLen == SOCKET_ERROR)
        {
            return result;
        }
        
        hostent * lpHostEnt = gethostbyname(HostName);

        if (lpHostEnt == NULL)
        {
            return result;
        }

        int i = 0;
        char * * pPtr = lpHostEnt->h_addr_list;
        
        IpList->Clear();
        
        while (pPtr[i] != NULL)
        {
            IpList->Add( inet_ntoa(*(in_addr*)pPtr[i]) );
            i++;
        }
        result = IpList->Count;

        return result;
    }
    __finally
    {
        return result;
    }
}
static String CmdLog(D1608Cmd cmd)
{
    String result;
    result = result + IntToStr(cmd.id) + "。";
    result = result + IntToStr(cmd.data.data_32_array[0]) + ",";
    result = result + IntToStr(cmd.data.data_32_array[1]) + ",";
    result = result + IntToStr(cmd.data.data_32_array[2]) + ",";
    result = result + IntToStr(cmd.data.data_32_array[3]) + ",";
    result = result + IntToStr(cmd.data.data_32_array[4]);

    return result;
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
    dsp_btn->GroupIndex = (int)dsp_btn; // 所有按钮互不影响

    return dsp_btn;
}
static TAdvTrackBar * CopyPnlMixTrackbar(TAdvTrackBar * src_trackbar, int dsp_id)
{
    TAdvTrackBar * trackbar = new TAdvTrackBar(src_trackbar->Parent);
    trackbar->Parent = src_trackbar->Parent;

    trackbar->OnChange = src_trackbar->OnChange;
    trackbar->OnKeyDown = src_trackbar->OnKeyDown;
    trackbar->OnEnter = src_trackbar->OnEnter;
    trackbar->OnExit = src_trackbar->OnExit;

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
static TEdit * CopyPnlMixEdit(TEdit * src_edit, int dsp_id)
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

    // 电阻默认参数
    default_vote_param._5vd_up = 6.81;
    default_vote_param._5vd_down = 6.81;
    default_vote_param._8vdc_up = 10;
    default_vote_param._8vdc_down = 4.75;
    default_vote_param._8vac_up = 10;
    default_vote_param._8vac_down = 4.75;
    default_vote_param._8va_up = 10;
    default_vote_param._8va_down = 4.75;
    default_vote_param._16vac_up = 20;
    default_vote_param._16vac_down = 3.32;
    default_vote_param._x16vac_up = 14.7;
    default_vote_param._x16vac_down = 20;
    default_vote_param._x16va_up = 14.7;
    default_vote_param._x16va_down = 20;
    default_vote_param._46vc_up = 75;
    default_vote_param._46vc_down = 4.75;
    default_vote_param._48va_up = 75;
    default_vote_param._48va_down = 4.75;
    default_vote_param._46va_up = 75;
    default_vote_param._46va_down = 4.75;
    default_vote_param._5va_up = 6.81;
    default_vote_param._5va_down = 6.81;
    default_vote_param._12va_up = 14.7;
    default_vote_param._12va_down = 4.75;
    default_vote_param._x12va_up = 10;
    default_vote_param._x12va_down = 14.7;
    default_vote_param._16va_up = 20;
    default_vote_param._16va_down = 3.32;

    // 参数处理
    on_loading = true;

    //x=GetSystemMetrics(SM_CXSCREEN)
    //y=GetSystemMetrics(SM_CYSCREEN)

    // 调整尺寸
    Width = 1664;//1366;
    Height = 768;//798;

    for (int i=0;i<32;i++)
    {
        level_meter[i][0] = -49;
        level_meter[i][1] = -100;
    }

    pnlOperator->Width = REAL_INPUT_DSP_NUM * PANEL_WIDTH + imgPresetBg->Width + REAL_OUTPUT_DSP_NUM * PANEL_WIDTH;
    pnlOperator->Width = Math::Max(pnlOperator->Width, Width-16);
        HorzScrollBar->Visible = (pnlOperator->Width > Width);
    pnlOperator->Height = 650;//-(728-584);
    pnlOperator->Top = pnlHeader->Height;

    pnlMix->Width = REAL_INPUT_DSP_NUM * PANEL_WIDTH;                               

    pb_watch_list[0] = pb_watch;

    //----------------------------------------
    // 输入
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

    pnlOperator->DoubleBuffered = true;
    pnlDspDetail->DoubleBuffered = true;
    pnlHeader->DoubleBuffered = true;
    pnlMonitor->DoubleBuffered = true;

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

    file_dirty = false;

    InitGdipus();

    pnlDspDetail->BringToFront();

    mireg0 = 0;

    on_loading = false;

    // 私有变量初始化
    keep_live_count = 5;
    device_connected = false;

    // 读取配置
    TIniFile * ini_file = new TIniFile(ExtractFilePath(Application->ExeName) + "SMC.ini");
    last_device_id = ini_file->ReadString("connection", "last_id", "");
    delete ini_file;

    // 需要初始化完成后才开启SLP，否则自动连接后会立即调整界面，导致Input面板还未初始化时就做调整，这样就会出现异常。
    tmSLP->Enabled = true;
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
    String inner_mac[5] = {"10-0B-A9-2F-55-90", "00-5A-39-FF-49-28","00-E0-4C-39-17-31","74-D0-2B-95-48-02","00-E0-4C-15-1B-C0"};
    is_inner_pc = false;
    for (int i=0;i<5;i++)
    {                       
        if (local_mac_list.Pos(inner_mac[i]) > 0)
        {
            is_inner_pc = true;
            break;
        }
    }

    if (is_inner_pc)
    {
        edtCmdId->Show();
        edtDeviceType->Show();
        edtStartBuildTime->Show();

        btnUnlockExt->Show();
        btnLeaveTheFactory->Show();
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
    memo_debug->Lines->Add(GetTime()+"InputConfigMap:" + IntToStr(sizeof(InputConfigMap)));
    memo_debug->Lines->Add(GetTime()+"OutputConfigMap:" + IntToStr(sizeof(OutputConfigMap)));
    memo_debug->Lines->Add(GetTime()+"MasterMixConfigMap:" + IntToStr(sizeof(MasterMixConfigMap)));
    memo_debug->Lines->Add(GetTime()+"ConfigMap:" + IntToStr(sizeof(ConfigMap)));
    memo_debug->Lines->Add(GetTime()+"mix_mute:" + IntToStr(sizeof(config_map.master_mix.mix_mute)));
    memo_debug->Lines->Add(GetTime()+"NotStorageCmd:" + IntToStr(sizeof(NotStorageCmd)));

    // 根据数量初始化控制器
    // Panel->Tag
    // Panel->Left
    // Label->Caption

    mmLog->Text = Now();

    if (FileExists("SMC.log"))
    {
        try{
            log_file = new TFileStream("SMC.log", fmOpenWrite|fmShareDenyWrite);
            log_file->Seek(0, soFromEnd);
        }
        catch(...)
        {
            log_file = new TFileStream("SMC"+Now().FormatString("yymmdd_hhnnss")+".log", fmCreate);
        }
    }
    else
    {
        log_file = new TFileStream("SMC.log", fmCreate);
    }                       

    btnRefresh->Click();

    //tsSearch->Show();
    this->Repaint();

    pnlMix->BringToFront();

    // 设置最大最小值
    cg2_5V->MaxValue = 250+75;
    cg3_3V->MaxValue = 330+75;
    cg3_3Vd->MaxValue = 330+75;
    cg5Va->MaxValue = 500+75;
    cg5Vd->MaxValue = 500+75;
    cg8Va->MaxValue = 800+75;
    cg8Vd->MaxValue = 800+75;
    cg12Va->MaxValue = 1200+75;
    cg_12Va->MaxValue = 1200+75;
    cg16Va->MaxValue = 1600+75;
    cg_16Va->MaxValue = 1600+75;
    cg46Va->MaxValue = 4800+75;

    cg2_5V->MinValue = 250-75;
    cg3_3V->MinValue = 330-75;
    cg3_3Vd->MinValue = 330-75;
    cg5Va->MinValue = 500-75;
    cg5Vd->MinValue = 500-75;
    cg8Va->MinValue = 800-75;
    cg8Vd->MinValue = 800-75;
    cg12Va->MinValue = 1200-75;
    cg_12Va->MinValue = 1200-75;
    cg16Va->MinValue = 1600-75;
    cg_16Va->MinValue = 1600-75;
    cg46Va->MinValue = 4800-75;

    // 加载LOGO图片
    if (FileExists("logo.bmp"))
    {
        imgLogo->Picture->LoadFromFile("logo.bmp");
        // Height -- 3, 73px  -- 中线 38
        // imgLogo->Height/2  <-- 38
        imgLogo->Top = 38 - imgLogo->Height/2;
    }

    // 上位机时间
    edtStartBuildTime->Text = " \t \t";
    edtStartBuildTime->Text = edtStartBuildTime->Text + GetDateTimeFromMarco(compile_time).FormatString("yymmddhhnnss");
}
//---------------------------------------------------------------------------
void __fastcall TForm1::FormDestroy(TObject *Sender)
{
    if (log_file)
    {
        delete log_file;
        log_file = NULL;
    }
    for (int i=0;i<REAL_OUTPUT_DSP_NUM;i++)
    {
        if (MixPicture[i] != NULL)
            delete MixPicture[i];
    }
    delete panel_agent;
    delete paint_agent;

}
//---------------------------------------------------------------------------
void TForm1::SendCmd2(D1608Cmd& cmd)
{
    unsigned __int8 * p = (unsigned __int8*)&cmd;
    edtDebug->Text = "";
    String cmd_text = "";
    for (unsigned int i=sizeof(cmd.flag);i<sizeof(cmd.flag)+16+cmd.length;i++)
    {
        cmd_text = cmd_text + IntToHex(p[i], 2) + " ";
    }
    edtDebug->Text = cmd_text;

    if (on_loading || !udpControl->Active || keep_live_count >= 5)
    {
        return;
    }
    if (!device_connected && cmd.id != GetOffsetOfData(&config_map.op_code))
    {
        return;
    }

    try
    {
        udpControl->SendBuffer(dst_ip, UDP_PORT_CONTROL, &cmd, sizeof(cmd));
    }
    catch(...)
    {
        udpControl->Active = false;
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

void TForm1::SendLogBuff(int udp_port, void * buff, int size)
{
    TPackage package;
    package.udp_port = udp_port;
    memcpy(package.data, buff, size);
    package.data_size = size;

    reverse(sendcmd_list.begin(), sendcmd_list.end());
    sendcmd_list.push_back(package);
    reverse(sendcmd_list.begin(), sendcmd_list.end());

    int sendcmd_list_length = sendcmd_list.size();
    // 保存在队列中
    if (sendcmd_list_length == 1)
    {
        memo_debug->Lines->Add(GetTime()+"发出UDP信息");
        try
        {
            udpControl->SendBuffer(dst_ip, udp_port, buff, size);
        }
        catch(...)
        {
            udpControl->Active = false;
        }
        sendcmd_delay_count = 15;
    }
}
void TForm1::SendCmd(D1608Cmd& cmd)
{
    edtCmdId->Text = cmd.id;

    if (!udpControl->Active)
    {
        edtCmdId->Text = edtCmdId->Text + "*";
        return;
    }

    if (on_loading || !udpControl->Active || keep_live_count >= 5)
    {
        return;
    }
    if (!device_connected && cmd.id != GetOffsetOfData(&config_map.op_code))
    {
        return;
    }

#if 1
    // 需要发送的命令id是否在队列中
    if (sendcmd_list.size() > 1)
    {
        for (vector<TPackage>::iterator iter=sendcmd_list.begin();
            iter+1!=sendcmd_list.end();
            iter++)
        {
            if (iter->udp_port != UDP_PORT_CONTROL)
                continue;

            D1608Cmd * package_sendcmd = (D1608Cmd*)(iter+1)->data;
            if (package_sendcmd->id == cmd.id)
            {
                sendcmd_list.erase(iter);
                break;
            }
        }
    }
#endif

    TPackage package;
    package.udp_port = UDP_PORT_CONTROL;
    memcpy(package.data, &cmd, sizeof(cmd));
    package.data_size = sizeof(cmd);

    // 保存在队列中
    sendcmd_list.insert(sendcmd_list.begin(), package);

    int sendcmd_list_length = sendcmd_list.size();
    // 如果原先队列是空，那么触发一次发送消息
    if (sendcmd_list_length == 1)
    {
        TPackage package = sendcmd_list.back();
        D1608Cmd * p_cmd = (D1608Cmd*)package.data;

        memo_debug->Lines->Add(GetTime()+"发出消息:"+IntToStr(cmd.id)+"|"+IntToStr(cmd.data.data_32)+"    "+IntToStr(p_cmd->id)+"|"+IntToStr(p_cmd->data.data_32));
        SendCmd2(cmd);
        sendcmd_delay_count = 15;
    }
    else
    {
        TPackage package = sendcmd_list.back();
        D1608Cmd * p_cmd = (D1608Cmd*)package.data;

        memo_debug->Lines->Add(GetTime()+"追加到队列"+IntToStr(cmd.id)+"|"+IntToStr(cmd.data.data_32)+"    "+IntToStr(p_cmd->id)+"|"+IntToStr(p_cmd->data.data_32));
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
        }
    }

    GetLocalIpList(lbIplist->Items);


    for (int i=0;i<lvDevice->Items->Count;i++)
    {
        TListItem * find_item = lvDevice->Items->Item[i];
        if (find_item->SubItems->Strings[6] == last_device_id)
        {
            find_item->Selected = true;
            break;
        }
    }

    if (lvDevice->Selected != NULL)
    {
        if (lvDevice->Selected->SubItems->Strings[3] != "")
            lblDeviceName->Caption = lvDevice->Selected->SubItems->Strings[3];
        else
        {
            lblDeviceName->Caption = lvDevice->Selected->SubItems->Strings[4]+"-"+lvDevice->Selected->SubItems->Strings[6];
            if (lblDeviceName->Caption.Length() > 16)
                lblDeviceName->Caption = lvDevice->Selected->SubItems->Strings[4];
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::udpSLPUDPRead(TObject *Sender,
      TStream *AData, TIdSocketHandle *ABinding)
{
    T_slp_pack slp_pack = {0};
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

    String cpu_id;
    unsigned char * px = (unsigned char*)slp_pack.cpu_id;
    for (int i=0;i<12;i++)
    {
        cpu_id = cpu_id + IntToHex(px[i], 2);
    }

    TListItem * item = NULL;
    // 查找是否列表中已经存在
    for (int i=0;i<lvDevice->Items->Count;i++)
    {
        TListItem * find_item = lvDevice->Items->Item[i];
        if (find_item->SubItems->Strings[6] == mac)
        {
            // 更新属性
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
            data->count = 2;
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
        data->count = 2;
        data->data = slp_pack;
        item->Data = data;
    }

    if (!udpControl->Active)
    {
        if ((last_device_id == "") || (last_device_id == mac) || ( Now() > startup_time+10.0/3600/24 ))
        {
            // 连接第一个 或者 匹配ID 或者 5秒钟 或者 没有找到原先的设备
            //file_dirty = false;
            item->Selected = true;
            btnSelectClick(NULL);
            startup_time = startup_time + 100000;
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::lvDeviceSelectItem(TObject *Sender,
      TListItem *Item, bool Selected)
{
    if (!Selected)
    {
        return;
    }

    //AppendLog("选择："+IntToStr(Item->Index+1)+" "+Item->SubItems->Strings[0]);
}
//---------------------------------------------------------------------------
void TForm1::AppendLog(String log)
{
    mmLog->Lines->Add(log);
    if (log_file != NULL)
    {
        log_file->WriteBuffer(log.c_str(), log.Length());
        log_file->WriteBuffer("\x0d\x0a", 2);
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

    // 是否版本兼容
    if (selected->SubItems->Strings[8] != "YES")
    {
        if (Sender != NULL)
            ShowMessage("版本不兼容，请更新上位机软件");
        return;
    }

    // 断开文件
    if (file_dirty)
    {
        /* 暂时不考虑提示对话框
        int ret = Application->MessageBox("是否保存当前preset修改", "关闭确认", MB_YESNOCANCEL);
        if (ret == ID_CANCEL)
        {
            return;
        }
        else if (ret == ID_YES)
        {
            // SAVE
            SaveAllPreset->Click();
            if (file_dirty)
            {
                return;
            }
        }
        else*/
        {
            // 放弃
            preset_lib_filename = "";
            file_dirty = false;
        }
    }

    dst_ip = selected->SubItems->Strings[0];
    String broadcast_ip = selected->SubItems->Strings[1];
    device_cpuid = selected->SubItems->Strings[5];

    sendcmd_list.clear();
    // 初始化socket
    udpControl->Active = false;
    udpControl->Bindings->Clear();
    udpControl->Bindings->Add();
    udpControl->Bindings->Items[0]->IP = broadcast_ip;
    udpControl->Bindings->Items[0]->Port = 0;
    udpControl->Active = true;

    UpdateCaption();

    // 发送上位机时间
    double app_time = Now()-TDateTime(2000,1,1);
    app_time = app_time * 24 * 3600 * 10;
    D1608Cmd cmd;
    cmd.id = GetOffsetOfData(&config_map.op_code.set_time);
    cmd.data.data_64 = app_time;
    cmd.length = 8;
    udpControl->SendBuffer(dst_ip, UDP_PORT_CONTROL, &cmd, sizeof(cmd));

    // 获取下位机PRESET数据，同时这个消息作为下位机认可的消息
    TPackage package;
    package.udp_port = UDP_PORT_READ_PRESET;

    StartReadOnePackage(0xFF);
    restor_delay_count = 15;
    tmDelayBackup->Enabled = true;

    D1608PresetCmd preset_cmd(version);
    preset_cmd.preset = 0; // 读取global_config
    preset_cmd.store_page = 0;
    memcpy(package.data, &preset_cmd, sizeof(preset_cmd));
    package.data_size = sizeof(preset_cmd);
    read_one_preset_package_list.push_back(package);

    udpControl->SendBuffer(dst_ip, package.udp_port, package.data, package.data_size);

    // 发送上位机时间, 单位100ms, 从2000年1月1日开始计算
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

    last_device_id = selected->SubItems->Strings[6];
    // 记录在配置文件里
    TIniFile * ini_file = new TIniFile(ExtractFilePath(Application->ExeName) + "SMC.ini");
    ini_file->WriteString("connection", "last_id", last_device_id);
    delete ini_file;

    // 显示名称
    if (selected->SubItems->Strings[3] != "")
        lblDeviceName->Caption = selected->SubItems->Strings[3];
    else             
    {
        lblDeviceName->Caption = lvDevice->Selected->SubItems->Strings[4]+"-"+lvDevice->Selected->SubItems->Strings[6];
        if (lblDeviceName->Caption.Length() > 16)
            lblDeviceName->Caption = lvDevice->Selected->SubItems->Strings[4];
    }

    edtMAC->Text = selected->SubItems->Strings[6];

    // 从数据中获取版本信息            
    DeviceData * data = (DeviceData*)selected->Data;
    lblVersion->Caption = VersionToStr(data->data.version)+ " " +VersionToStr(version);
    lblDeviceInfo->Caption = "VERSION " + VersionToStr(data->data.version);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::tmSLPTimer(TObject *Sender)
{
    tmSLP->Enabled = false;

    if (lbIplist->Count > 0)
    {
        local_broadcast_ip = lbIplist->Items->Strings[0];
        lbIplist->Items->Delete(0);

        //AppendLog("尝试侦听地址： " + local_broadcast_ip);

        udpSLP->Active = false;
        udpSLP->Bindings->Clear();
        udpSLP->Bindings->Add();
        udpSLP->Bindings->Items[0]->IP = local_broadcast_ip;
        udpSLP->Bindings->Items[0]->Port = 0;
        udpSLP->BroadcastEnabled = true;

        try{
            udpSLP->Active = true;
            char *search_flag;
            if (is_inner_pc)
            {
                search_flag = "ver info";
            }
            else
            {
                search_flag = "rep";
            }
            udpSLP->SendBuffer("255.255.255.255", UDP_PORT_SLP, search_flag, sizeof(search_flag));
        }
        catch(...)
        {
            AppendLog("网络异常");
        }
    }
    else
    {
        btnRefresh->Click();
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
        D1608Cmd cmd;
        cmd.id = GetOffsetOfData(&config_map.input_dsp[dsp_num-1].level_a);
        cmd.data.data_16 = value;
        cmd.length = 2;
        SendCmd(cmd);

        config_map.input_dsp[dsp_num-1].level_a = value;
    }
    else
    {
        //D1608Cmd cmd;
        //cmd.id = GetOffsetOfData(&config_map.mix_dsp.level_a);
        //cmd.data.data_8 = value;
        //SendCmd(cmd);
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ToogleMute(TObject *Sender)
{
    TSpeedButton* btn = (TSpeedButton*)Sender;
    int dsp_id = btn->Tag;

    D1608Cmd cmd;
    cmd.id = GetOffsetOfData(&config_map.input_dsp[dsp_id-1].mute_switch);
    cmd.data.data_8 = btn->Down;
    cmd.length = 1;
    SendCmd(cmd);

    config_map.input_dsp[dsp_id-1].mute_switch = btn->Down;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ToogleNoise(TObject *Sender)
{
    TSpeedButton* btn = (TSpeedButton*)Sender;
    int dsp_id = btn->Tag;

    D1608Cmd cmd;
    cmd.id = GetOffsetOfData(&config_map.input_dsp[dsp_id-1].noise_switch);
    cmd.data.data_8 = btn->Down;
    cmd.length = 1;
    SendCmd(cmd);

    config_map.input_dsp[dsp_id-1].noise_switch = btn->Down;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ToogleInvert(TObject *Sender)
{
    TSpeedButton* btn = (TSpeedButton*)Sender;
    int dsp_id = btn->Tag;

    D1608Cmd cmd;
    cmd.id = GetOffsetOfData(&config_map.input_dsp[dsp_id-1].invert_switch);
    cmd.data.data_8 = btn->Down;
    cmd.length = 1;
    SendCmd(cmd);

    config_map.input_dsp[dsp_id-1].invert_switch = btn->Down;
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
#if 0
        int dsp_id = btn->Tag;
        D1608Cmd cmd = InputDefault(dsp_id, 0);
        cmd.length = 1;
        SendCmd(cmd);
#endif
    }
    else
    {
        // default 缺少取消所有 default
        last_default_btn = NULL;
#if 0
        D1608Cmd cmd = InputDefault(0, 0);
        cmd.length = 1;
        SendCmd(cmd);
#endif
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ToogleAuto(TObject *Sender)
{
#if 0
    TSpeedButton* btn = (TSpeedButton*)Sender;
    int dsp_id = btn->Tag;
    static int input_auto = 0;
    input_auto = SetBit(input_auto, dsp_id, btn->Down);
    D1608Cmd cmd = InputAuto(input_auto);
    cmd.length = 1;
    SendCmd(cmd);
#endif
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ToogleEQ(TObject *Sender)
{
    TSpeedButton* btn = (TSpeedButton*)Sender;
    int dsp_id = btn->Tag;

    D1608Cmd cmd;
    cmd.id = GetOffsetOfData(&config_map.input_dsp[dsp_id-1].eq_switch);
    cmd.data.data_8 = btn->Down;
    cmd.length = 1;
    SendCmd(cmd);

    config_map.input_dsp[dsp_id-1].eq_switch = btn->Down;
}
//---------------------------------------------------------------------------
int CalcVot1(unsigned __int16 true_data1, float rdown, float r_up)
{
    return true_data1 * (rdown+r_up) / rdown / 10;
}
int CalcVot2(unsigned __int16 true_data1, unsigned __int16 true_data2, float r2_down, float r2_up, float r1_up, float r1_down)
{
    return ((true_data2 * (r1_up+r1_down) / r1_up) - (true_data1 * (r1_down/r1_up) * (r2_down+r2_up) / r2_down))  / 10;
}
String IntOrZeroSring(int value)
{
    return String((value>0?value:0));
}
void __fastcall TForm1::udpControlUDPRead(TObject *Sender, TStream *AData,
      TIdSocketHandle *ABinding)
{
    if (ABinding->PeerPort == UDP_PORT_CONTROL)
    {
        D1608Cmd cmd;
        AData->ReadBuffer(&cmd, std::min(sizeof(cmd), AData->Size));

        //memo_debug->Lines->Add(GetTime()+"Reply：" + CmdLog(cmd));

        // id == 1 表示全局配置
        if (cmd.type == CMD_TYPE_GLOBAL)
        {
            /*if (cmd.id == offsetof(GlobalConfig, adjust_running_time))
            {
                global_config.adjust_running_time = cmd.data.data_64;
            }
            else if (cmd.id == offsetof(GlobalConfig, unlock_string))*/
            {
                // 重新获取数据
                TPackage package;
                package.udp_port = UDP_PORT_READ_PRESET;

                D1608PresetCmd preset_cmd(version);
                preset_cmd.preset = 0; // 读取global_config
                preset_cmd.store_page = 0;
                memcpy(package.data, &preset_cmd, sizeof(preset_cmd));
                package.data_size = sizeof(preset_cmd);
                read_one_preset_package_list.push_back(package);

                udpControl->SendBuffer(dst_ip, package.udp_port, package.data, package.data_size);
            }

            //ApplyConfigToUI();
        }
        else if (cmd.id == GetOffsetOfData(&config_map.op_code.noop))
        {
            ProcessWatchLevel(cmd.data.keep_alive.watch_level);
            ProcessVote(cmd.data.keep_alive.adc);
            ProcessKeepAlive(cmd.data.keep_alive.switch_preset, cmd.data.keep_alive.set_time);
        }
        else if (cmd.id == GetOffsetOfData(&config_map.op_code.switch_preset))
        {
            SetPresetId(cmd.data.data_32);
            StartReadOnePackage(0xFF);
            restor_delay_count = 15;
            tmDelayBackup->Enabled = true;
        }
        else
        {
            // 如果是当前调节的数据，需要忽略
            if (!ProcessSendCmdAck(cmd, AData, ABinding))//(last_cmd.id != cmd.id /*|| !paint_agent->IsMouseDown()*/)
            {
                memo_debug->Lines->Add(GetTime()+"Reply：" + CmdLog(cmd));

                memcpy(((char*)(&config_map))+cmd.id, (char*)&cmd.data, cmd.length);
                OnFeedbackData(cmd.id, cmd.length);
            }
        }
    }
    else if (ABinding->PeerPort == UDP_PORT_READ_LOG)
    {
        LogBuff buff;

        AData->ReadBuffer(&buff, sizeof(buff));
        if ((buff.address >= LOG_START_PAGE) && (buff.address < MAC_LIST_START_PAGE))
        {
            // 日志
            for (int i=0;i<128;i++)
            {
                if (buff.event[i].timer != 0xFFFFFFFF)
                {
                    TListItem * item = lvLog->Items->Insert(0);

                    int ms = buff.event[i].timer % 10;  buff.event[i].timer /= 10;
                    int sec = buff.event[i].timer % 60; buff.event[i].timer /= 60;
                    int min = buff.event[i].timer % 60; buff.event[i].timer /= 60;

                    item->Caption = IntToStr(buff.event[i].timer)+":"
                                  + IntToStr(min)+":"
                                  + IntToStr(sec)+"."
                                  + IntToStr(ms);

                    switch (buff.event[i].event_id)
                    {
                    case EVENT_POWER_OFF:
                        item->SubItems->Add("关闭电源");
                        item->SubItems->Add("启动次数"+IntToStr(buff.event[i].event_data));
                        break;
                    case EVENT_SYSTEM_LIMIT:
                        item->SubItems->Add("达到运行次数或时间限制");
                        item->SubItems->Add(buff.event[i].event_data);
                        break;
                    case EVENT_SAVE_PRESET:
                        item->SubItems->Add("保存Preset");
                        item->SubItems->Add("Preset编号:"+IntToStr(buff.event[i].event_data));
                        break;
                    case EVENT_POWER_SAVE_OK:
                        item->SubItems->Add("关机存盘成功");
                        item->SubItems->Add("下次启动次数"+IntToStr(buff.event[i].event_data));
                        break;
                    case EVENT_INPUT_OVERFLOW:
                        item->SubItems->Add("input通道音量溢出");
                        item->SubItems->Add("通道号"+IntToStr(buff.event[i].event_data));
                        break;
                    case EVENT_OUTPUT_OVERFLOW:
                        item->SubItems->Add("output通道音量溢出");
                        item->SubItems->Add("通道号"+IntToStr(buff.event[i].event_data));
                        break;
                    case EVENT_WRITE_FLASH_ERROR:
                        item->SubItems->Add("写flash失败");
                        item->SubItems->Add("地址:0x"+IntToHex(buff.event[i].event_data * 2048, 8));
                        break;
                    case EVENT_REBOOT:
                        item->SubItems->Add("上位机发起重启");
                        item->SubItems->Add(buff.event[i].event_data);
                        break;
                    case EVENT_SAVE_PRESET_OK:
                        item->SubItems->Add("存盘完成");
                        {
                            String page_indexs = "存盘页";
                            if (buff.event[i].event_data & 1) page_indexs = page_indexs + "1,";
                            if (buff.event[i].event_data & 2) page_indexs = page_indexs + "2,";
                            if (buff.event[i].event_data & 4) page_indexs = page_indexs + "3,";
                            if (buff.event[i].event_data & 8) page_indexs = page_indexs + "4,";
                            if (buff.event[i].event_data & 16) page_indexs = page_indexs + "5,";
                            item->SubItems->Add(page_indexs);
                        }
                        break;
                    case EVENT_CHECK_MD5_FAIL:
                        item->SubItems->Add("激活码错误");
                        item->SubItems->Add("");
                        break;
                    case EVENT_28J60_REINIT_ERROR:
                        item->SubItems->Add("ENC28J60初始化失败");
                        item->SubItems->Add("失败次数:"+IntToStr(buff.event[i].event_data));
                        break;
                    case EVENT_MAC_ADDRESS_OVERFLOW:
                        item->SubItems->Add("MAC地址日志满");
                        item->SubItems->Add("");
                        break;
                    case EVENT_NO_KEY:
                        item->SubItems->Add("设备未授权");
                        item->SubItems->Add("");
                        break;
                    case EVENT_DSP_NOT_MATCH_ERROR:
                        item->SubItems->Add("YSS920与配置不符");
                        if (buff.event[i].event_data > 0x80)
                            item->SubItems->Add("缺少:"+IntToStr(buff.event[i].event_data-0x80));
                        else
                            item->SubItems->Add("多出:"+IntToStr(buff.event[i].event_data));
                        break;
                    case EVENT_LED_NUM_ERR:
                        item->SubItems->Add("LED控制芯片错误");
                        item->SubItems->Add(buff.event[i].event_data);
                        break;
                    case EVENT_FILENAME_CHANGED:
                        item->SubItems->Add("导入/导出配置");
                        item->SubItems->Add("");
                        break;
                    case EVENT_SET_MAC_ADDRESS:
                        item->SubItems->Add("设置了MAC地址");
                        item->SubItems->Add(IntToHex(buff.event[i].event_data, 6));
                        break;
                    case EVENT_ADDA_ERROR:
                        item->SubItems->Add("AD/DA数量与配置不符");
                        item->SubItems->Add("AD:"+IntToStr(buff.event[i].event_data/32)+"/DA:"+IntToStr(buff.event[i].event_data%32));
                        break;
                    default:
                        item->SubItems->Add(buff.event[i].event_id);
                        item->SubItems->Add(buff.event[i].event_data);
                        break;
                    }

                }
            }
        }
        else
        {
            // MAC地址
            for (int i=0;i<128;i++)
            {
                if (memcmp(buff.mac[i], "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF", 8) != 0)
                {
                    TListItem * item = lvLog->Items->Add();

                    item->Caption = "";
                    item->SubItems->Add("mac");
                    String mac_string;
                    mac_string.sprintf("%0X-%0X-%0X-%0X-%0X-%0X",
                                        buff.mac[i][0], buff.mac[i][1], buff.mac[i][2],
                                        buff.mac[i][3], buff.mac[i][4], buff.mac[i][5]);
                    item->SubItems->Add(mac_string);
                }
            }
            btnGetLog->Enabled = true;
        }

        if (!ProcessLogBuffAck(buff, AData, ABinding))
        {
            memo_debug->Lines->Add(GetTime()+"Reply Buff");
        }
    }
    else if (ABinding->PeerPort == UDP_PORT_GET_DEBUG_INFO)
    {
        typedef struct
        {
            char desc[10];
            short log_level;
            long timer;
        }LogData;
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

                item->Caption = IntToStr(log_data[i].timer)+":"
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
        if (package.data_size != AData->Size)
            return;

        D1608PresetCmd * send_preset_cmd = (D1608PresetCmd*)package.data;
        if (send_preset_cmd->preset != preset_cmd.preset || send_preset_cmd->store_page != preset_cmd.store_page)
            return;

        int preset_id = preset_cmd.preset;
        if (preset_id == 0xFF)
            preset_id = cur_preset_id;

        // 保存
        if ((preset_id & 0x7F) == 0)
        {
            // global_config
            memcpy(&global_config, preset_cmd.data, sizeof(global_config));
            UpdateCaption();

            char * device_type = global_config.device_type;
            if (global_config.device_type[0] == '*')
            {
                edtDeviceType->Color = clRed;
                device_type = global_config.device_type+1;
            }
            
            // 显示版本信息
            edtDeviceType->Text = device_type;

            try
            {
                edtStartBuildTime->Text = "'"+GetDateTimeFromMarco(global_config.start_build_time).FormatString("yymmddhhnnss");
            }
            catch(...)
            {
                edtStartBuildTime->Text = global_config.start_build_time;
            }
            edtStartBuildTime->Text = edtStartBuildTime->Text+"\t'" + global_config.build_time + "\t'";
            edtStartBuildTime->Text = edtStartBuildTime->Text + GetDateTimeFromMarco(compile_time).FormatString("yymmddhhnnss");

            //edtBuildTime->Text = global_config.build_time;
            edtDeviceName->Text = global_config.d1616_name;
            //TDateTime d = edtBuildTime->Text;

            String gain_set = GetVersionConfig(device_type).gain_function;
            iMIC->Visible = (gain_set.Pos("iMIC")!=0);
            i10dBv->Visible = (gain_set.Pos("i10dBv")!=0);
            i22dBu->Visible = (gain_set.Pos("i22dBu")!=0);
            i24dBu->Visible = (gain_set.Pos("i24dBu")!=0);
            o10dBv->Visible = (gain_set.Pos("o10dBv")!=0);
            o22dBu->Visible = (gain_set.Pos("o22dBu")!=0);
            o24dBu->Visible = (gain_set.Pos("o24dBu")!=0);

            lblPresetFileName->Caption = global_config.import_filename;

            // 显示preset名称
            clbAvaliablePreset->Items->Strings[0] = global_config.preset_name[0][0] ? global_config.preset_name[0] : "PRESET1";
            clbAvaliablePreset->Items->Strings[1] = global_config.preset_name[1][0] ? global_config.preset_name[1] : "PRESET2";
            clbAvaliablePreset->Items->Strings[2] = global_config.preset_name[2][0] ? global_config.preset_name[2] : "PRESET3";
            clbAvaliablePreset->Items->Strings[3] = global_config.preset_name[3][0] ? global_config.preset_name[3] : "PRESET4";
            clbAvaliablePreset->Items->Strings[4] = global_config.preset_name[4][0] ? global_config.preset_name[4] : "PRESET5";
            clbAvaliablePreset->Items->Strings[5] = global_config.preset_name[5][0] ? global_config.preset_name[5] : "PRESET6";
            clbAvaliablePreset->Items->Strings[6] = global_config.preset_name[6][0] ? global_config.preset_name[6] : "PRESET7";
            clbAvaliablePreset->Items->Strings[7] = global_config.preset_name[7][0] ? global_config.preset_name[7] : "PRESET8";

            // 版本校验
            if (global_config.version >= offsetof(GlobalConfig, ad_da_card))
            {
                int dsp_920_num = 0;
                TPanel* iDsp[8] = {iDsp1, iDsp2, iDsp3, iDsp4, iDsp5, iDsp6, iDsp7, iDsp8};
                for (int i=0;i<8;i++) // xDO: 暂时只考虑8片
                {
                    dsp_920_num += global_config.yss920[i];
                    iDsp[i]->Caption = (global_config.yss920[i]?"920":"X");
                }

                TPanel* iAdDa[8] = {iAD_abcd, iAD_efgh, iAD_ijkl, iAD_mnop, iDA_1234, iDA_5678, iDA_9_12, iDA_13_16};
                for (int i=0;i<8;i++)
                    iAdDa[i]->Caption = global_config.ad_da_card[i];

                iLed->Caption = global_config.led_control_count;

                REAL_INPUT_DSP_NUM = GetVersionConfig(edtDeviceType->Text).input_channel_count;
                REAL_OUTPUT_DSP_NUM = GetVersionConfig(edtDeviceType->Text).output_channel_count;

                SetIOChannelNum();
            }
            cbGlobalDspName->Checked = ((global_config.is_global_name == 1) || (global_config.is_global_name == 0xFF));
            if (global_config.version >= offsetof(GlobalConfig, auto_saved))
            {
                // 读取'自动保存'配置
                cbPresetAutoSaved->Checked = ((global_config.auto_saved == 1) || (global_config.auto_saved == 0xFF));
            }
        }
        else
        {
            AppendLog(IntToHex(preset_cmd.preset, 2) + "/" + IntToStr(preset_cmd.store_page));

            ConfigMap * dest_config_map = &all_config_map[preset_id-1];

            switch(preset_cmd.store_page)
            {
            case 0:
                memcpy(&dest_config_map->input_dsp[0], preset_cmd.data, sizeof(config_map.input_dsp[0])*4);
                break;
            case 1:
                memcpy(&dest_config_map->input_dsp[4], preset_cmd.data, sizeof(config_map.input_dsp[0])*4);
                break;
            case 2:
                memcpy(&dest_config_map->input_dsp[8], preset_cmd.data, sizeof(config_map.input_dsp[0])*4);
                break;
            case 3:
                memcpy(&dest_config_map->input_dsp[12],preset_cmd.data,  sizeof(config_map.input_dsp[0])*4);
                break;
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
            case 8:
                memcpy(&dest_config_map->master_mix, preset_cmd.data,
                        sizeof(config_map.master_mix));
                break;
            }
        }

        read_one_preset_package_list.pop_back();

        if (read_one_preset_package_list.empty())
        {
            if (preset_id == cur_preset_id)
                config_map = all_config_map[cur_preset_id-1];
            tmDelayUpdateUI->Enabled = false;
            ApplyConfigToUI();
            CloseDspDetail();
            memo_debug->Lines->Add(GetTime()+"同步完成");
        }
        else
        {
            package = read_one_preset_package_list.back();
            udpControl->SendBuffer(dst_ip, package.udp_port, package.data, package.data_size);

            //pbBackup->Position = pbBackup->Max - read_one_preset_package_list.size();
            restor_delay_count = 15;
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
        if (package.data_size != AData->Size)
            return;
        if (memcmp(package.data, &preset_cmd, package.data_size) != 0)
            return;

        package_list.pop_back();

        if (package_list.empty())
        {
            pbBackup->Position = pbBackup->Max;
            ::Sleep(1000);
            pbBackup->Hide();

            // 发送一个重新加载preset的指令
            if (preset_cmd.preset & 0x80)
            {
                D1608Cmd cmd;
                cmd.id = GetOffsetOfData(&config_map.op_code.switch_preset);
                cmd.data.data_32_array[0] = smc_config.global_config.active_preset_id;
                cmd.data.data_32_array[1] = 1;
                SendCmd2(cmd);
            }
            else if (cur_preset_id == clbAvaliablePreset->ItemIndex+1)
            {
                D1608Cmd cmd;
                cmd.id = GetOffsetOfData(&config_map.op_code.switch_preset);
                cmd.data.data_32_array[0] = cur_preset_id;
                cmd.data.data_32_array[1] = 1;
                SendCmd2(cmd);
            }
        }
        else
        {
            package = package_list.back();
            udpControl->SendBuffer(dst_ip, package.udp_port, package.data, package.data_size);

            pbBackup->Position = pbBackup->Max - package_list.size();
            restor_delay_count = 15;
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
        if (package.data_size != AData->Size)
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
            ::Sleep(1000);
            pbBackup->Hide();

            // 完成，写入文件
            TFileStream * file = new TFileStream(save_device_to_file_filename, fmCreate);
            if (!file)
            {
                ShowMessage("打开文件失败");
                return;
            }

            // 转换成普通的8个preset数据
            LoadGlobalConfig(smc_config.global_config, smc_config.device_flash_dump);
            for (int i=1;i<=8;i++)
            {
                ConfigMap config_map;
                LoadPresetById(i, config_map, smc_config.device_flash_dump);
                smc_config.all_config_map[i-1] = config_map;
            }
            file->WriteBuffer(&smc_config, sizeof(smc_config));

            pbBackup->Position = pbBackup->Max;
            ::Sleep(1000);
            pbBackup->Hide();
            delete file;
        }
        else
        {
            package = package_list.back();
            udpControl->SendBuffer(dst_ip, package.udp_port, package.data, package.data_size);

            pbBackup->Position = pbBackup->Max - package_list.size();
            restor_delay_count = 15;
            tmDelayBackup->Enabled = true;
        }
    }
}
//---------------------------------------------------------------------------
void TForm1::CalcAllVote(ADC_Data & adc_data)
{
    VoteParam vote_param;
    if (global_config.vote_param._5vd_up != 0)
        vote_param = global_config.vote_param;
    else
        vote_param = default_vote_param;

    adc_data._5vd   = CalcVot1(adc_data._5vd  , vote_param._5vd_down,    vote_param._5vd_up);
    adc_data._8vdc  = CalcVot1(adc_data._8vdc , vote_param._8vdc_down,   vote_param._8vdc_up);
    adc_data._8vac  = CalcVot1(adc_data._8vac , vote_param._8vac_down,   vote_param._8vac_up);
    adc_data._8va   = CalcVot1(adc_data._8va  , vote_param._8va_down,    vote_param._8va_up);
    adc_data._46vc  = CalcVot1(adc_data._46vc , vote_param._46vc_down,   vote_param._46vc_up);
    adc_data._48va  = CalcVot1(adc_data._48va , vote_param._48va_down,   vote_param._48va_up);
    adc_data._46va  = CalcVot1(adc_data._46va , vote_param._46va_down,   vote_param._46va_up);
    adc_data._5va   = CalcVot1(adc_data. _5va , vote_param. _5va_down,   vote_param. _5va_up);
    adc_data._12va  = CalcVot1(adc_data._12va , vote_param._12va_down,   vote_param._12va_up);
    adc_data._16va  = CalcVot1(adc_data._16va , vote_param._16va_down,   vote_param._16va_up);
    adc_data._16vac = CalcVot1(adc_data._16vac, vote_param._16vac_down,  vote_param._16vac_up);
    adc_data._x16vac= CalcVot2(adc_data._16vac, adc_data._x16vac, vote_param._x16vac_down,  vote_param._x16vac_up, vote_param._16vac_up, vote_param._16vac_down);
    adc_data._x16va = CalcVot2(adc_data._16va , adc_data._x16va , vote_param._x16va_down,   vote_param._x16va_up,  vote_param._16va_up,  vote_param._16va_down);
    adc_data._x12va = CalcVot2(adc_data._12va , adc_data._x12va , vote_param._x12va_down,   vote_param._x12va_up,  vote_param._12va_up,  vote_param._12va_down);
}
void TForm1::ProcessVote(short adc[ADC_NUM])
{
    __int16 data[16];

    // 计算检测到的电压 3.3v ~ 4096
    for (int i=0; i<ADC_NUM; i++)
    {
        data[i] = adc[i] * 2500 / adc[1];
    }

    for (int i=0; i<ADC_NUM; i++)
    {
        ValueListEditor1->Cells[1][i+1] = adc[i];
    }

    int org_base = adc[1];
    ADC_Data * true_data = (ADC_Data *)data;

    ADC_Data calc_data;

    // 正电压检测：
    //      ra = 4
    //      rb = 5v 3 / 8v 10 / 12v 15 / 16v 20 / 48v 75
    // 负电压检测
    //      ra rb 同正电压
    //      -5v  rc=4  rd=15
    //      -12v rc=10 rd=15
    //      -16v rc=15 rd=20
    calc_data = *true_data;
    calc_data._2_5v  = true_data->_2_5v / 10;
    calc_data.base   = 2500 / 10;
    CalcAllVote(calc_data);

    __int16 * xcalc_data = (__int16 *)&calc_data;
    for (int i=0; i<ADC_NUM; i++)
    {
        double vot = xcalc_data[i] / 100.0f;
        ValueListEditor2->Cells[1][i+1] = FloatToStr(vot);
    }

    lblDiff->Caption = calc_data._8vdc-calc_data._8va;

    ValueListEditor2->Cells[1][18] = (int)((calc_data._8va - calc_data._8vac) / 0.27);
    ValueListEditor2->Cells[1][19] = (calc_data._48va - calc_data._46vc) / 0.5;
    ValueListEditor2->Cells[1][20] = (calc_data._16va - calc_data._16vac) / 0.5;
    ValueListEditor2->Cells[1][21] = (calc_data._x16vac - calc_data._x16va) / 0.5;

    //====================================================================
    lbl2_5V->Caption = String::FormatFloat("0.00 ", calc_data._2_5v / 100.0);
    lbl3_3V->Caption = String::FormatFloat("0.00 ", (4096.0/org_base)*2.5);
    lbl3_3Vd->Caption = String::FormatFloat("0.00 ", (calc_data._2_5v+75) / 100.0);
    lbl5Va->Caption = String::FormatFloat("0.00 ", calc_data._5va / 100.0);
    lbl5Vd->Caption = String::FormatFloat("0.00 ", calc_data._5vd / 100.0);
    lbl8Va->Caption = String::FormatFloat("0.00 ", calc_data._8va / 100.0);
    lbl8Vd->Caption = String::FormatFloat("0.00 ", calc_data._8vdc / 100.0);
    lbl12Va->Caption = String::FormatFloat("0.00 ", calc_data._12va / 100.0);
    lbl_12Va->Caption = String::FormatFloat("0.00 ", calc_data._x12va / 100.0);
    lbl16Va->Caption = String::FormatFloat("0.00 ", calc_data._16va / 100.0);
    lbl_16Va->Caption = String::FormatFloat("0.00 ", calc_data._x16va / 100.0);
    lbl46Va->Caption = String::FormatFloat("0.00 ", calc_data._46va / 100.0);

    //====================================================================
    cg2_5V->Progress = calc_data._2_5v;
    cg3_3V->Progress = (4096.0/org_base)*250;
    cg3_3Vd->Progress = (calc_data._2_5v+75);
    cg5Va->Progress = calc_data._5va;
    cg5Vd->Progress = calc_data._5vd;
    cg8Va->Progress = calc_data._8va;
    cg8Vd->Progress = calc_data._8vdc;
    cg12Va->Progress = calc_data._12va;
    cg_12Va->Progress = calc_data._x12va + 2400;
    cg16Va->Progress = calc_data._16va;
    cg_16Va->Progress = calc_data._x16va + 3200;
    cg46Va->Progress = calc_data._46va;

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
        }
    }

    //====================================================================
    lbl2_5mA->Caption = "-- ";
    lbl3_3mA->Caption = IntOrZeroSring((int)((calc_data._8va - calc_data._8vdc) / 0.27 * 0.1)) + " ";   //8Vd * 0.10
    lbl3_3mAd->Caption = IntOrZeroSring((int)((calc_data._8va - calc_data._8vdc) / 0.27 * 0.85)) + " "; //8Vd * 0.85
    lbl5mAa->Caption = IntOrZeroSring((int)((calc_data._8va - calc_data._8vac) / 0.27)) + " ";          // 8Va
    lbl5mAd->Caption = IntOrZeroSring((int)((calc_data._8va - calc_data._8vdc) / 0.27 * 0.05)) + " ";   // 8Vd * 0.05
    lbl8mAa->Caption = IntOrZeroSring((int)((calc_data._8va - calc_data._8vac) / 0.27)) + " ";
    lbl8mAd->Caption = IntOrZeroSring((int)((calc_data._8va - calc_data._8vdc) / 0.27)) + " ";
    lbl12mAa->Caption = IntOrZeroSring((calc_data._16vac - calc_data._16va) / 0.5) + " ";               // 16Va
    lbl_12mAa->Caption = IntOrZeroSring((calc_data._x16va - calc_data._x16vac) / 0.5) + " ";            // -16Va
    lbl16mAa->Caption = IntOrZeroSring((calc_data._16vac - calc_data._16va) / 0.5) + " ";
    lbl_16mAa->Caption = IntOrZeroSring((calc_data._x16va - calc_data._x16vac) / 0.5) + " ";
    lbl46mAa->Caption = IntOrZeroSring((calc_data._48va - calc_data._46vc) / 0.5) + " ";
}
//---------------------------------------------------------------------------
bool TForm1::ProcessSendCmdAck(D1608Cmd& cmd, TStream *AData, TIdSocketHandle *ABinding)
{
    if (sendcmd_list.size() == 0)
        return false;

    TPackage package = sendcmd_list.back();

    if (package.udp_port != ABinding->PeerPort)
        return false;
    if (package.data_size != AData->Size)
        return false;

    D1608Cmd* package_cmd = (D1608Cmd*)package.data;
    if (cmd.id != package_cmd->id)
        return false;
    if (cmd.length != package_cmd->length)
    {
        memo_debug->Lines->Add(GetTime()+"消息长度不匹配");
        //return;
    }
    if (cmd.type != package_cmd->type)
        return false;
    if (memcmp(&cmd.data, &package_cmd->data, package_cmd->length) == 0)
    {
        memo_debug->Lines->Add(GetTime()+"消息匹配:"+IntToStr(cmd.id)+"|"+IntToStr(cmd.data.data_32));

        sendcmd_list.pop_back();
        if (sendcmd_list.size() > 0)
        {
            package = sendcmd_list.back();
            udpControl->SendBuffer(dst_ip, package.udp_port, package.data, package.data_size);
            sendcmd_delay_count = 15;

            {
                D1608Cmd& cmd1 = *(D1608Cmd*)package.data;
                memo_debug->Lines->Add(GetTime()+IntToHex((int)&cmd1, 8)+"继续下一个消息:"+IntToStr(cmd1.id)+"|"+IntToStr(cmd1.data.data_32));
            }
        }
        return true;
    }
    return false;
}
void TForm1::ProcessKeepAlive(int preset_id, long timer)
{
    if (preset_id<1 || preset_id>8)
    {
        // 收到的数据包异常
        return;
    }

    if (preset_id != cur_preset_id)
    {
        SetPresetId(preset_id);
        StartReadOnePackage(0xFF);
        restor_delay_count = 15;
        tmDelayBackup->Enabled = true;
    }

    // 显示时钟
    long running_time = (timer - global_config.adjust_running_time) / 1000;    // 单位：秒
    int hour = running_time / 3600;
    int minute = running_time % 3600;
    minute = minute / 60;
    lblDeviceRunningTime->Caption = lblDeviceRunningTime->Caption.sprintf("%d:%02d", hour, minute);
    //lblDeviceRunningTime->Caption = IntToStr(hour) + ":" + IntToStr(minute) + "";
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
        int remain_count = global_config.reboot_count_limit - (global_config.reboot_count - global_config.adjust_reboot_count);
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

    keep_live_count = 0;
    shape_live->Show();
    shape_link->Show();
    shape_power->Show();
    lblDeviceName->Show();
    lblDeviceInfo->Show();
    device_connected = true;
}
bool TForm1::ProcessLogBuffAck(LogBuff& buff, TStream *AData, TIdSocketHandle *ABinding)
{
    TPackage package = sendcmd_list.back();
    if (package.udp_port != ABinding->PeerPort)
        return false;
    if (package.data_size != AData->Size)
        return false;

    LogBuff* package_buff = (LogBuff*)package.data;

    if (buff.address == package_buff->address)
    {
        memo_debug->Lines->Add(GetTime()+"消息匹配");
        sendcmd_list.pop_back();
        if (sendcmd_list.size() > 0)
        {
            package = sendcmd_list.back();
            udpControl->SendBuffer(dst_ip, package.udp_port, package.data, package.data_size);
            sendcmd_delay_count = 15;
        }
    }
    return true;
}
//---------------------------------------------------------------------------
void TForm1::ProcessWatchLevel(int watch_level[INPUT_DSP_NUM + OUTPUT_DSP_NUM])
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

                if (i < 16)
                {
                    UpdateWatchLevel(i, (valuex - base) * 20 + 1 + 24);
                }
                else
                {
                    UpdateWatchLevel(i,
                        (valuex - base) * 20 + 1 + 24,
                        -100);
#if 0
                    int comp_level = watch_level[i+16];
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
#endif
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
    /*D1608Cmd cmd;
    cmd.id = GetOffsetOfData(&config_map.op_code.WatchLevel);
    SendCmd2(cmd);
    cmd.id = GetOffsetOfData(&config_map.op_code.adc);
    SendCmd2(cmd);*/

    // keep alive
    if ((keep_live_count < 5) && udpControl->Active)
    {
        keep_live_count++;
    }
    else
    {
        udpControl->Active = false;
        sendcmd_list.empty();
        shape_live->Hide();
        shape_link->Hide();
        shape_power->Hide();
        lblDeviceName->Hide();
        lblDeviceInfo->Hide();
        // Level Meter归零
        for (int i=0;i<32;i++)
        {
            UpdateWatchLevel(i, -49);
        }
        device_connected = false;
    }

    D1608Cmd cmd;
    cmd.type = CMD_TYPE_PRESET;
    cmd.id = GetOffsetOfData(&config_map.op_code);
    SendCmd2(cmd);

    // 检测resize事件
    if (nees_resize)
    {
        SetIOChannelNum();
        nees_resize = false;
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ToogleOutputMute(TObject *Sender)
{
    TSpeedButton* btn = (TSpeedButton*)Sender;
    int dsp_id = btn->Tag;

    D1608Cmd cmd;
    cmd.id = GetOffsetOfData(&config_map.output_dsp[dsp_id-1].mute_switch);
    cmd.data.data_8 = btn->Down;
    cmd.length = 1;
    SendCmd(cmd);

    config_map.output_dsp[dsp_id-1].mute_switch = btn->Down;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ToogleOutputInvert(TObject *Sender)
{
    TSpeedButton* btn = (TSpeedButton*)Sender;
    int dsp_id = btn->Tag;

    D1608Cmd cmd;
    cmd.id = GetOffsetOfData(&config_map.output_dsp[dsp_id-1].invert_switch);
    cmd.data.data_8 = btn->Down;
    cmd.length = 1;
    SendCmd(cmd);

    config_map.output_dsp[dsp_id-1].invert_switch = btn->Down;
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

        pnlMix->Top = btn->Top + btn->Height + 10;//312;
        pnlMix->Show();
        pnlMix->Tag = btn->Tag;

        // 数据
        int out_dsp_num = last_out_num_btn->Tag;

        for (int i=0;i<REAL_INPUT_DSP_NUM;i++)
        {
            mix_mute_btn[i]->Down = config_map.master_mix.mix_mute[i][out_dsp_num-1];
            mix_level_trackbar[i]->Position = config_map.master_mix.mix[i][out_dsp_num-1];
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
    int dsp_id = btn->Tag;

    D1608Cmd cmd;
    cmd.id = GetOffsetOfData(&config_map.output_dsp[dsp_id-1].eq_switch);
    cmd.data.data_8 = btn->Down;
    cmd.length = 1;
    SendCmd(cmd);

    config_map.output_dsp[dsp_id-1].eq_switch = btn->Down;
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

    D1608Cmd cmd;
    cmd.id = GetOffsetOfData(&config_map.output_dsp[dsp_num-1].level_a);
    cmd.data.data_16 = value;
    cmd.length = 2;
    SendCmd(cmd);

    config_map.output_dsp[dsp_num-1].level_a = value;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::lvDeviceDblClick(TObject *Sender)
{
    TListItem * item = lvDevice->Selected;
    if (item != NULL)
    {
        dst_ip = item->SubItems->Strings[0];
        btnSelect->Click();
    }
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
            btnPhanton->Down = config_map.input_dsp[dsp_num-1].phantom_switch;
            btnPhanton->Show();

            // 调整PaintBox1的尺寸
            PaintBox1->Left = 8;
            PaintBox1->Width = 753;
            pbComp->Hide();
            
            // 隐藏COMP界面
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
            btnPhanton->Hide();

            // 调整PaintBox1的尺寸
            if (GetVersionConfig(edtDeviceType->Text).is_comp)
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

            if (GetVersionConfig(edtDeviceType->Text).is_comp)
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
    // mix_panel_state bit3
    TSpeedButton* btn = (TSpeedButton*)Sender;

    D1608Cmd cmd;
    cmd.id = GetOffsetOfData(&config_map.mix_dsp.invert_switch);
    cmd.data.data_8 = btn->Down;
    cmd.length = 1;
    SendCmd(cmd);
#endif
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btnMAXONClick(TObject *Sender)
{
#if 0
    // mix_panel_state bit1
    mix_panel_state = SetBit(mix_panel_state, 1, btnMAXON->Down);
    D1608Cmd cmd = Mix_MaxPrio_Invert_Mute(mix_panel_state);
    cmd.length = 1;
    SendCmd(cmd);
#endif
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btnLastOnClick(TObject *Sender)
{
#if 0
    TSpeedButton* btn = (TSpeedButton*)Sender;
    D1608Cmd cmd = InputDefault(0, btn->Down);
    cmd.length = 1;
    SendCmd(cmd);
#endif
}
//---------------------------------------------------------------------------

void __fastcall TForm1::btnFShiftClick(TObject *Sender)
{
#if 0
    TSpeedButton* btn = (TSpeedButton*)Sender;
    D1608Cmd cmd = FShift(btn->Down);
    cmd.length = 1;
    SendCmd(cmd);
#endif
}
//---------------------------------------------------------------------------
void __fastcall TForm1::MasterVolumeChange(TObject *Sender)
{
    TAdvTrackBar* track = (TAdvTrackBar*)Sender;
    int value = track->Position;

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

    D1608Cmd cmd;
    cmd.id = GetOffsetOfData(&config_map.master_mix.level_a);
    cmd.data.data_32 = value;
    cmd.length = 4;
    SendCmd(cmd);

    config_map.master_mix.level_a = value;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btnMixMuteClick(TObject *Sender)
{
#if 0
    TSpeedButton* btn = (TSpeedButton*)Sender;

    D1608Cmd cmd;
    cmd.id = GetOffsetOfData(&config_map.mix_dsp.mute_switch);
    cmd.data.data_8 = btn->Down;
    cmd.length = 1;
    SendCmd(cmd);
#endif
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btnMasterMuteClick(TObject *Sender)
{
    TSpeedButton* btn = (TSpeedButton*)Sender;

    D1608Cmd cmd;
    cmd.id = GetOffsetOfData(&config_map.master_mix.mute_switch);
    cmd.data.data_8 = btn->Down;
    cmd.length = 1;
    SendCmd(cmd);

    config_map.master_mix.mute_switch = btn->Down;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btnPhantonClick(TObject *Sender)
{
    TSpeedButton* btn = (TSpeedButton*)Sender;
    int dsp_id = btn->Parent->Tag;

    D1608Cmd cmd;
    cmd.id = GetOffsetOfData(&config_map.input_dsp[dsp_id-1].phantom_switch);
    cmd.data.data_8 = btn->Down;
    cmd.length = 1;
    SendCmd(cmd);

    config_map.input_dsp[dsp_id-1].phantom_switch = btn->Down;
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
    // 输入菜单
    TLabel * popup_label = (TLabel*)PopupMenu1->PopupComponent;
    popup_label->Caption = ((TMenuItem*)Sender)->Caption;
    int dsp_num = popup_label->Tag;

    D1608Cmd cmd;
    cmd.id = GetOffsetOfData(&config_map.input_dsp[dsp_num-1].gain);
    cmd.length = 1;
    if (popup_label->Caption == "MIC")
    {
        cmd.data.data_8 = INPUT_GAIN_MIC;
    }
    else if (popup_label->Caption == "10dBv")
    {
        cmd.data.data_8 = INPUT_GAIN_10dBv;
    }
    else if (popup_label->Caption == "22dBu")
    {
        cmd.data.data_8 = INPUT_GAIN_22dBu;
    }
    else if (popup_label->Caption == "24dBu")
    {
        cmd.data.data_8 = INPUT_GAIN_24dBu;
    }
    SendCmd(cmd);

    config_map.input_dsp[dsp_num-1].gain = cmd.data.data_8;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::MenuItem3Click(TObject *Sender)
{
    // 输出菜单
    TLabel * popup_label = (TLabel*)PopupMenu2->PopupComponent;
    popup_label->Caption = ((TMenuItem*)Sender)->Caption;
    int dsp_num = popup_label->Tag - 16;

    D1608Cmd cmd;
    cmd.id = GetOffsetOfData(&config_map.output_dsp[dsp_num-1].gain);
    cmd.length = 1;
    if (popup_label->Caption == "10dBv")
    {
        cmd.data.data_8 = OUTPUT_GAIN_10dBv;
    }
    else if (popup_label->Caption == "22dBu")
    {
        cmd.data.data_8 = OUTPUT_GAIN_22dBu;
    }
    else if (popup_label->Caption == "24dBu")
    {
        cmd.data.data_8 = OUTPUT_GAIN_24dBu;
    }
    SendCmd(cmd);

    config_map.output_dsp[dsp_num-1].gain = cmd.data.data_8;
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
        filter_set.GetFilter(i)->ChangFilterParameter("Parametric", preset_freq_list[i-1], 0, 4.09);
        filter_set.GetFilter(i)->name = IntToStr(i-1);
        filter_set.SetBypass(i, false);
        filter_set.RepaintPaint(i);
    }

    filter_set.GetFilter(HP_FILTER+1)->ChangFilterParameter("Low Shelf", preset_freq_list[HP_FILTER], 0, 4.09);
        filter_set.GetFilter(HP_FILTER+1)->name = IntToStr(HP_FILTER+1-1);
        filter_set.SetBypass(HP_FILTER+1, false);
        filter_set.RepaintPaint(HP_FILTER+1);
    filter_set.GetFilter(LP_FILTER-1)->ChangFilterParameter("High Shelf", preset_freq_list[LP_FILTER-1-1], 0, 4.09);
        filter_set.GetFilter(LP_FILTER-1)->name = IntToStr(LAST_FILTER-1-1);    // name按照PEQ编号
        filter_set.SetBypass(LP_FILTER-1, false);
        filter_set.RepaintPaint(LP_FILTER-1);

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
        }

        InputVolumeChange(input_level_trackbar[dsp_num-1]);
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

        OutputVolumeChange(output_level_trackbar[dsp_num-1]);
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

        MasterVolumeChange(master_panel_trackbar);
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

    if (label->Caption == edtInput->Text)
    {
        return;
    }

    label->Caption = edtInput->Text;

    D1608Cmd cmd;
    cmd.type = cbGlobalDspName->Checked;
    if (cmd.type == CMD_TYPE_GLOBAL)
    {
        cmd.id = offsetof(GlobalConfig, input_dsp_name[label->Tag]);
        strncpy(global_config.input_dsp_name[label->Tag], label->Caption.c_str(), 6);
    }
    else
    {
        cmd.id = GetOffsetOfData(&config_map.input_dsp[label->Tag].dsp_name);
        strncpy(config_map.input_dsp[label->Tag].dsp_name, label->Caption.c_str(), 6);
    }
    strncpy(cmd.data.data_string, label->Caption.c_str(), 6);
    cmd.length = 7;
    SendCmd(cmd);
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

    if (label->Caption == edtInput->Text)
    {
        return;
    }
    else
    {
        label->Caption = edtInput->Text;
    }

    D1608Cmd cmd;
    cmd.type = cbGlobalDspName->Checked;
    if (cmd.type == CMD_TYPE_GLOBAL)
    {
        cmd.id = offsetof(GlobalConfig, output_dsp_name[label->Tag]);
        strncpy(global_config.output_dsp_name[label->Tag], label->Caption.c_str(), 6);
    }
    else
    {
        cmd.id = GetOffsetOfData(&config_map.output_dsp[label->Tag].dsp_name);
        strncpy(config_map.output_dsp[label->Tag].dsp_name, label->Caption.c_str(), 6);
    }

    strncpy(cmd.data.data_string, label->Caption.c_str(), 6);
    cmd.length = 7;
    SendCmd(cmd);
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

        D1608Cmd cmd;
        cmd.id = GetOffsetOfData(&config_map.master_mix.mix[in_dsp_num-1][out_dsp_num-1]);
        cmd.data.data_16 = value;
        cmd.length = 2;
        SendCmd(cmd);

        config_map.master_mix.mix[in_dsp_num-1][out_dsp_num-1] = value;
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

        pnlmix_level_trackbarChange(mix_level_trackbar[dsp_num-1]);
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

        D1608Cmd cmd;
        cmd.id = GetOffsetOfData(&config_map.master_mix.mix_mute[in_dsp_num-1][out_dsp_num-1]);
        cmd.data.data_8 = btn->Down;
        cmd.length = 1;
        SendCmd(cmd);

        config_map.master_mix.mix_mute[in_dsp_num-1][out_dsp_num-1] = btn->Down;
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btnSavePresetToFileClick(TObject *Sender)
{
    // 保存到文件
    SaveDialog1->Filter = "preset|*.preset";
    if (SaveDialog1->Execute())
    {
        // save config_map to file
        TFileStream * file = new TFileStream(SaveDialog1->FileName, fmCreate);
        if (!file)
        {
            ShowMessage("打开文件失败");
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

        file->ReadBuffer(&smc_config.all_config_map[select_preset_id-1], sizeof(config_map));

        delete file;

        ///ApplyConfigToUI();
        ///CloseDspDetail();

        if (!udpControl->Active)
        {
            // 脱机
            all_config_map[select_preset_id-1] = smc_config.all_config_map[select_preset_id-1].ToConfigMap();
            // TODO: 是否更新当前界面？
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

            // Download To Device
            // 写入所有的preset数据
            D1608PresetCmd preset_cmd(version);
            preset_cmd.preset = select_preset_id;

            for (int i=select_preset_id-1;i<=select_preset_id-1;i++)
            for (int store_page=0;store_page<9;store_page++)
            {
                preset_cmd.store_page = store_page;
                switch(preset_cmd.store_page)
                {
                case 0:
                    memcpy(preset_cmd.data, &smc_config.all_config_map[i].input_dsp[0], sizeof(InputConfigMap)*4);
                    break;
                case 1:
                    memcpy(preset_cmd.data, &smc_config.all_config_map[i].input_dsp[4], sizeof(InputConfigMap)*4);
                    break;
                case 2:
                    memcpy(preset_cmd.data, &smc_config.all_config_map[i].input_dsp[8], sizeof(InputConfigMap)*4);
                    break;
                case 3:
                    memcpy(preset_cmd.data, &smc_config.all_config_map[i].input_dsp[12], sizeof(InputConfigMap)*4);
                    break;
                case 4:
                    memcpy(preset_cmd.data, &smc_config.all_config_map[i].output_dsp[0], sizeof(OutputConfigMap)*4);
                    break;
                case 5:
                    memcpy(preset_cmd.data, &smc_config.all_config_map[i].output_dsp[4], sizeof(OutputConfigMap)*4);
                    break;
                case 6:
                    memcpy(preset_cmd.data, &smc_config.all_config_map[i].output_dsp[8], sizeof(OutputConfigMap)*4);
                    break;
                case 7:
                    memcpy(preset_cmd.data, &smc_config.all_config_map[i].output_dsp[12], sizeof(OutputConfigMap)*4);
                    break;
                case 8:
                    memcpy(preset_cmd.data, &smc_config.all_config_map[i].master_mix,
                            sizeof(MasterMixConfigMap));
                }

                TPackage package;
                memcpy(package.data, &preset_cmd, sizeof(preset_cmd));
                package.udp_port = UDP_PORT_STORE_PRESET_PC2FLASH;
                package.data_size = sizeof(preset_cmd);

                package_list.push_back(package);
            }

            std::reverse(package_list.begin(), package_list.end());

            TPackage package = package_list.back();
            udpControl->SendBuffer(dst_ip, package.udp_port, package.data, package.data_size);

            //last_command = UDP_PORT_STORE_PRESET_PC2FLASH;
            //last_restore_package = preset_cmd;
            restor_delay_count = 15;
            tmDelayBackup->Enabled = true;

            pbBackup->Max = package_list.size();
            //pbBackup->Position = 0;
            //pbBackup->Show();
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
    master_panel_trackbar->Position = config_map.master_mix.level_a;
    btnMasterMute->Down = (config_map.master_mix.mute_switch==1);

    on_loading = false;
}
void TForm1::OnFeedbackData(unsigned int cmd_id, int length)
{
    on_loading = true;

	int ObjectIndex = -1;
	if ((cmd_id >= GetOffsetOfData(&config_map.input_dsp))
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
// 		else if (cmd_id == GetOffsetOfData((char*)&config_map.input_dsp[ObjectIndex].delay))
// 		{
// 		}
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
	}
	else if ((cmd_id >= GetOffsetOfData(&config_map.master_mix))
		&& (cmd_id < GetOffsetOfData((void*)&config_map.master_mix.mix)))
	{
		if (cmd_id == GetOffsetOfData(&config_map.master_mix.mute_switch))
		{
			btnMasterMute->Down = config_map.master_mix.mute_switch;
		}
		else if (cmd_id == GetOffsetOfData((char*)&config_map.master_mix.level_a))
		{
            master_panel_trackbar->Position = config_map.master_mix.level_a;
		}
	}
	else if ((cmd_id >= GetOffsetOfData((char*)&config_map.master_mix.mix))
		&& (cmd_id < sizeof(config_map.master_mix.mix)+GetOffsetOfData((char*)&config_map.master_mix.mix)))
	{
		int offset = (cmd_id - GetOffsetOfData((char*)&config_map.master_mix.mix))/sizeof(config_map.master_mix.mix[0][0]);
		//int channel_in = offset / OUTPUT_DSP_NUM;
		int channel_out = offset % OUTPUT_DSP_NUM;
        // 小界面
        if (pnlMix->Visible && pnlMix->Tag-1==channel_out)
        {
            for (int i=0;i<REAL_INPUT_DSP_NUM;i++)
            {
                mix_mute_btn[i]->Down = config_map.master_mix.mix_mute[i][channel_out];
                mix_level_trackbar[i]->Position = config_map.master_mix.mix[i][channel_out];
            }
        }
	}
	else if ((cmd_id >= GetOffsetOfData((char*)&config_map.master_mix.mix_mute))
		&& (cmd_id < sizeof(config_map.master_mix.mix_mute)+GetOffsetOfData((char*)&config_map.master_mix.mix_mute)))
	{
		int offset = (cmd_id - GetOffsetOfData((char*)&config_map.master_mix.mix_mute))/sizeof(config_map.master_mix.mix_mute[0][0]);
		//int channel_in = offset / OUTPUT_DSP_NUM;
		int channel_out = offset % OUTPUT_DSP_NUM;
        // 小界面
        if (pnlMix->Visible && pnlMix->Tag-1==channel_out)
        {
            for (int i=0;i<REAL_INPUT_DSP_NUM;i++)
            {
                mix_mute_btn[i]->Down = config_map.master_mix.mix_mute[i][channel_out];
                mix_level_trackbar[i]->Position = config_map.master_mix.mix[i][channel_out];
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
void __fastcall TForm1::SetFileDirty(bool dirty_flag)
{
    file_dirty = dirty_flag;
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
    if (file_dirty)
    {
        Caption = Caption + "*";
    }
}
//---------------------------------------------------------------------------


void __fastcall TForm1::FormCloseQuery(TObject *Sender, bool &CanClose)
{
    if (file_dirty)
    {
        /* 暂时不考虑提示对话框
        int ret = Application->MessageBox("是否保存当前preset修改", "关闭确认", MB_YESNOCANCEL);
        if (ret == ID_CANCEL)
        {
            CanClose = false;
            return;
        }
        else if (ret == ID_YES)
        {
            // SAVE
            SaveAllPreset->Click();
            if (file_dirty)
            {
                CanClose = false;
                return;
            }
        }
        else*/
        {
            return;
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::StoreClick(TObject *Sender)
{
    if (udpControl->Active)
    {
        udpControl->SendBuffer(dst_ip, UDP_PORT_STORE_PRESET_MEM2FLASH, "\100"/*"\xFF"*/, 1);
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::StoreAsClick(TObject *Sender)
{
    TMenuItem * menu = (TMenuItem*)Sender;
    char preset_id = menu->Tag;
    if (udpControl->Active)
    {
        udpControl->SendBuffer(dst_ip, UDP_PORT_STORE_PRESET_MEM2FLASH, &preset_id, 1);
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
    SetPresetId(menu->Tag);

    CloseDspDetail();

    if (udpControl->Active)
    {
        D1608Cmd cmd;
        cmd.id = GetOffsetOfData(&config_map.op_code.switch_preset);
        cmd.data.data_32 = menu->Tag;
        SendCmd2(cmd);

        // 延时5秒后强制从本地内存更新到界面
        tmDelayUpdateUI->Enabled = true;
    }
    else
    {
       tmDelayUpdateUITimer(NULL);
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btnSetIpClick(TObject *Sender)
{
    D1608Cmd cmd;
    cmd.type = CMD_TYPE_GLOBAL;
    cmd.id = offsetof(GlobalConfig, static_ip_address);

    if (rbStaticIpEnabled->Checked == false)
    {
        cmd.data.data_32 = 0;
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
        cmd.data.data_32 = ip;
    }
        
    cmd.length = 4;
    SendCmd2(cmd);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btnGetLogClick(TObject *Sender)
{
    // 最后一包一定是MAC地址，所以实在MAC处理的时候恢复这个Enabled
    btnGetLog->Enabled = false;
    lvLog->Clear();
    LogBuff buff;
    for (buff.address = 0x8000000+128*1024+10*5*2*1024;
        buff.address < MAC_LIST_START_PAGE+MAC_LIST_SIZE;
        buff.address += 1024)
    {
        SendLogBuff(UDP_PORT_READ_LOG, &buff, sizeof(buff));
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btnGetDebugClick(TObject *Sender)
{
    char buf[1024] = {0};
    udpControl->SendBuffer(dst_ip, UDP_PORT_GET_DEBUG_INFO, buf, 1024);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::lblPresetNameClick(TObject *Sender)
{
    String new_name = InputBox("修改名称", "", lblPresetName->Caption);
    if (new_name == lblPresetName->Caption)
        return;
    else
        lblPresetName->Caption = new_name;

    D1608Cmd cmd;
    cmd.type = CMD_TYPE_GLOBAL;
    cmd.id = offsetof(GlobalConfig, preset_name[cur_preset_id-1]);
    strncpy(cmd.data.data_string, lblPresetName->Caption.c_str(), 16);
    cmd.length = 17;
    SendCmd(cmd);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btnDeviceNameClick(TObject *Sender)
{
    strncpy(global_config.d1616_name, edtDeviceName->Text.c_str(), 16);

    UpdateCaption();

    D1608Cmd cmd;
    cmd.type = CMD_TYPE_GLOBAL;
    cmd.id = offsetof(GlobalConfig, d1616_name);
    strncpy(cmd.data.data_string, global_config.d1616_name, 16);
    cmd.length = 17;
    SendCmd2(cmd);
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

    D1608Cmd cmd;
    cmd.type = CMD_TYPE_GLOBAL;
    cmd.id = offsetof(GlobalConfig, avaliable_preset);
    for (int i=0;i<PRESET_NUM;i++)
    {
        global_config.avaliable_preset[i] =  clbAvaliablePreset->Checked[i];
        cmd.data.data_string[i] = clbAvaliablePreset->Checked[i];
    }
    cmd.length = 8;
    SendCmd2(cmd);
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


    D1608Cmd cmd;
    cmd.type = CMD_TYPE_GLOBAL;

    cmd.id = offsetof(GlobalConfig, running_timer_limit);

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
    strncpy(global_config.password, edtPassword->Text.c_str(), 16);
    strncpy(global_config.password_of_key, edtKeyPassword->Text.c_str(), 16);


    cmd.length = sizeof(global_config.running_timer_limit)
                +sizeof(global_config.reboot_count_limit)
                +sizeof(global_config.locked_string)
                +sizeof(global_config.password)
                +sizeof(global_config.password_of_key);
    memcpy(cmd.data.data_string, &global_config.running_timer_limit, cmd.length);

    SendCmd(cmd);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btnUnlockClick(TObject *Sender)
{
    D1608Cmd cmd;
    cmd.type = CMD_TYPE_GLOBAL;
    cmd.id = offsetof(GlobalConfig, unlock_string);
    strncpy(cmd.data.data_string, edtUnlockPassword->Text.c_str(), 16);
    cmd.length = 20;
    SendCmd2(cmd);
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
        roboot_count = rt;
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
    // 后台解锁    
    D1608Cmd cmd;
    cmd.type = CMD_TYPE_GLOBAL;
    cmd.id = offsetof(GlobalConfig, back_door);
    cmd.length = 68+4;
    memset(&cmd.data, 0, cmd.length);
    SendCmd(cmd);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btnLeaveTheFactoryClick(TObject *Sender)
{
    // 出厂    
    D1608Cmd cmd;
    cmd.type = CMD_TYPE_GLOBAL;
    cmd.id = offsetof(GlobalConfig, adjust_running_time);
    SendCmd(cmd);
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
    int dsp_id = btn->Tag;

    D1608Cmd cmd;
    cmd.id = GetOffsetOfData(&config_map.output_dsp[dsp_id-1].comp_switch);
    cmd.data.data_8 = btn->Down;
    cmd.length = 1;
    SendCmd(cmd);

    config_map.output_dsp[dsp_id-1].comp_switch = btn->Down;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::edtCompRatioKeyDown(TObject *Sender, WORD &Key,
      TShiftState Shift)
{
    TEdit* edt = (TEdit*)Sender;
    int dsp_id = edt->Parent->Parent->Tag;

    if (dsp_id < 100)
    {
        return;
    }
    dsp_id -= 100;

    if (Key == VK_ESCAPE)
    {
        edt->Text = Ration2String(config_map.output_dsp[dsp_id-1].ratio);
    }
    else if (Key == VK_RETURN)
    {
        try{
            if (edt->Text == "∞")
                config_map.output_dsp[dsp_id-1].ratio = 0;
            else
                config_map.output_dsp[dsp_id-1].ratio = 100.0 / edt->Text.ToDouble();//*ratio_config.scale;
            if (config_map.output_dsp[dsp_id-1].ratio > ratio_config.max_value)
                config_map.output_dsp[dsp_id-1].ratio = ratio_config.max_value;
            else if (config_map.output_dsp[dsp_id-1].ratio < ratio_config.min_value)
                config_map.output_dsp[dsp_id-1].ratio = ratio_config.min_value;
            
            D1608Cmd cmd;
            cmd.id = GetOffsetOfData(&config_map.output_dsp[dsp_id-1].ratio);
            cmd.data.data_32 = config_map.output_dsp[dsp_id-1].ratio;
            cmd.length = 4;
            SendCmd(cmd);

            filter_set.ratio = config_map.output_dsp[dsp_id-1].ratio / 100.0;
            pbComp->Invalidate();
        }
        catch(...)
        {
        }

        edt->Text = Ration2String(config_map.output_dsp[dsp_id-1].ratio);
        edt->SelectAll();
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::edtCompThresholdKeyDown(TObject *Sender, WORD &Key,
      TShiftState Shift)
{
    TEdit* edt = (TEdit*)Sender;
    int dsp_id = edt->Parent->Parent->Tag;

    if (dsp_id < 100)
    {
        return;
    }
    dsp_id -= 100;

    if (Key == VK_ESCAPE)
    {
        edt->Text = FormatFloat(config_map.output_dsp[dsp_id-1].threshold/threshold_config.scale, threshold_config.precise);
    }
    else if (Key == VK_RETURN)
    {
        try{
            config_map.output_dsp[dsp_id-1].threshold = edt->Text.ToDouble()*threshold_config.scale;

            if (config_map.output_dsp[dsp_id-1].threshold > threshold_config.max_value)
                config_map.output_dsp[dsp_id-1].threshold = threshold_config.max_value;
            else if (config_map.output_dsp[dsp_id-1].threshold < threshold_config.min_value)
                config_map.output_dsp[dsp_id-1].threshold = threshold_config.min_value;

            D1608Cmd cmd;
            cmd.id = GetOffsetOfData(&config_map.output_dsp[dsp_id-1].threshold);
            cmd.data.data_32 = config_map.output_dsp[dsp_id-1].threshold;
            cmd.length = 4;
            SendCmd(cmd);

            filter_set.threshold = config_map.output_dsp[dsp_id-1].threshold / 10.0;
            pbComp->Invalidate();
        }
        catch(...)
        {
        }

        edt->Text = FormatFloat(config_map.output_dsp[dsp_id-1].threshold/threshold_config.scale, threshold_config.precise);
        edt->SelectAll();
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::edtCompAttackTimeKeyDown(TObject *Sender,
      WORD &Key, TShiftState Shift)
{
    TEdit* edt = (TEdit*)Sender;
    int dsp_id = edt->Parent->Parent->Tag;

    if (dsp_id < 100)
    {
        return;
    }
    dsp_id -= 100;

    if (Key == VK_ESCAPE)
    {
        edt->Text = FormatFloat(config_map.output_dsp[dsp_id-1].attack_time/attack_config.scale, attack_config.precise);
    }
    else if (Key == VK_RETURN)
    {
        try{
            config_map.output_dsp[dsp_id-1].attack_time = edt->Text.ToDouble()*attack_config.scale;

            if (config_map.output_dsp[dsp_id-1].attack_time > attack_config.max_value)
                config_map.output_dsp[dsp_id-1].attack_time = attack_config.max_value;
            else if (config_map.output_dsp[dsp_id-1].attack_time < attack_config.min_value)
                config_map.output_dsp[dsp_id-1].attack_time = attack_config.min_value;
            
            D1608Cmd cmd;
            cmd.id = GetOffsetOfData(&config_map.output_dsp[dsp_id-1].attack_time);
            cmd.data.data_32 = config_map.output_dsp[dsp_id-1].attack_time;
            cmd.length = 4;
            SendCmd(cmd);
        }
        catch(...)
        {
        }

        edt->Text = FormatFloat(config_map.output_dsp[dsp_id-1].attack_time/attack_config.scale, attack_config.precise);
        edt->SelectAll();
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::edtCompReleaseTimeKeyDown(TObject *Sender,
      WORD &Key, TShiftState Shift)
{
    TEdit* edt = (TEdit*)Sender;
    int dsp_id = edt->Parent->Parent->Tag;

    if (dsp_id < 100)
    {
        return;
    }
    dsp_id -= 100;

    if (Key == VK_ESCAPE)
    {
        edt->Text = FormatFloat(config_map.output_dsp[dsp_id-1].release_time/release_config.scale, release_config.precise);
    }
    else if (Key == VK_RETURN)
    {
        try{
            config_map.output_dsp[dsp_id-1].release_time = edtCompReleaseTime->Text.ToDouble()*release_config.scale;

            if (config_map.output_dsp[dsp_id-1].release_time > release_config.max_value)
                config_map.output_dsp[dsp_id-1].release_time = release_config.max_value;
            else if (config_map.output_dsp[dsp_id-1].release_time < release_config.min_value)
                config_map.output_dsp[dsp_id-1].release_time = release_config.min_value;
            
            D1608Cmd cmd;
            cmd.id = GetOffsetOfData(&config_map.output_dsp[dsp_id-1].release_time);
            cmd.data.data_32 = config_map.output_dsp[dsp_id-1].release_time;
            cmd.length = 4;
            SendCmd(cmd);
        }
        catch(...)
        {
        }

        edt->Text = FormatFloat(config_map.output_dsp[dsp_id-1].release_time/release_config.scale, release_config.precise);
        edt->SelectAll();
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::edtCompGainKeyDown(TObject *Sender, WORD &Key,
      TShiftState Shift)
{
    TEdit* edt = (TEdit*)Sender;
    int dsp_id = edt->Parent->Parent->Tag;

    if (dsp_id < 100)
    {
        return;
    }
    dsp_id -= 100;

    if (Key == VK_ESCAPE)
    {
        edt->Text = FormatFloat(config_map.output_dsp[dsp_id-1].comp_gain/gain_config.scale, gain_config.precise);
    }
    else if (Key == VK_RETURN)
    {
        try{
            config_map.output_dsp[dsp_id-1].comp_gain = edt->Text.ToDouble()*gain_config.scale;

            if (config_map.output_dsp[dsp_id-1].comp_gain > gain_config.max_value)
                config_map.output_dsp[dsp_id-1].comp_gain = gain_config.max_value;
            else if (config_map.output_dsp[dsp_id-1].comp_gain < gain_config.min_value)
                config_map.output_dsp[dsp_id-1].comp_gain = gain_config.min_value;

            D1608Cmd cmd;
            cmd.id = GetOffsetOfData(&config_map.output_dsp[dsp_id-1].comp_gain);
            cmd.data.data_32 = config_map.output_dsp[dsp_id-1].comp_gain;
            cmd.length = 4;
            SendCmd(cmd);

            filter_set.gain = config_map.output_dsp[dsp_id-1].comp_gain / 10.0;
            pbComp->Invalidate();
        }
        catch(...)
        {
        }

        edt->Text = FormatFloat(config_map.output_dsp[dsp_id-1].comp_gain/gain_config.scale, gain_config.precise);
        edt->SelectAll();
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::cbCompAutoTimeClick(TObject *Sender)
{
    TCheckBox* check_box = (TCheckBox*)Sender;
    int dsp_id = check_box->Parent->Parent->Tag;

    config_map.output_dsp[dsp_id-101].auto_time = check_box->Checked;

    D1608Cmd cmd;
    cmd.id = GetOffsetOfData(&config_map.output_dsp[dsp_id-101].auto_time);
    cmd.data.data_8 = config_map.output_dsp[dsp_id-101].auto_time;
    cmd.length = 1;
    SendCmd(cmd);

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
        edtCompAttackTime->Text = config_map.output_dsp[dsp_id-101].attack_time/10.0;
        edtCompReleaseTime->Text = config_map.output_dsp[dsp_id-101].release_time/10.0;
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
        }
        break;
    case 5:
        /*if (is_inner_pc)
        {
            pnlSearch->Show();
            pnlSearch->BringToFront();
        }*/
        break;
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::lbl5VdClick(TObject *Sender)
{
    // 记录显示的
    TControl* control = (TControl*)Sender;

    shape_active_adc->Top = control->Top;
    shape_active_adc->Show();

    line_value = control->Tag / 10.0;
    Series1->Clear();
    lineUpLimit->Clear();
    lineDownLimit->Clear();
    Chart1->BottomAxis->SetMinMax(0, 100);

    // 纵坐标范围设定为+-1V
    Chart1->LeftAxis->SetMinMax(line_value*1.1, line_value*0.9);

    up_line_value = line_value*1.05;
    down_line_value = line_value*0.95;

    for (int i=0;i<100;i++)
    {
        lineUpLimit->AddXY(i, up_line_value, "e", clRed);
        lineDownLimit->AddXY(i, down_line_value, "b", clRed);
        Series1->AddNull("");
    }

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

    // 设置CheckBox的背景
    // bmp->Canvas->Pixels[rbDhcpEnabled->Left+1][rbDhcpEnabled->Top+1];
    rbDhcpEnabled->Color = bmp->Canvas->Brush->Color;
    rbStaticIpEnabled->Color = bmp->Canvas->Brush->Color;
    cbRunningTimer->Color = bmp->Canvas->Brush->Color;
    cbRebootCount->Color = bmp->Canvas->Brush->Color;
    cbLockedString->Color = bmp->Canvas->Brush->Color;
    cbGlobalDspName->Color = bmp->Canvas->Brush->Color;
    cbPresetAutoSaved->Color = bmp->Canvas->Brush->Color;
    cbLockUpDownMenuKey->Color = bmp->Canvas->Brush->Color;

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
            D1608Cmd cmd;
            cmd.id = GetOffsetOfData(&config_map.input_dsp[dsp_num-1].level_b);
            cmd.data.data_16 = value;
            cmd.length = 2;
            SendCmd(cmd);

            config_map.input_dsp[dsp_num-1].level_b = value;
        }
    }
    else
    {
        if (config_map.output_dsp[dsp_num-101].level_b != value)
        {
            // output channel
            D1608Cmd cmd;
            cmd.id = GetOffsetOfData(&config_map.output_dsp[dsp_num-101].level_b);
            cmd.data.data_16 = value;
            cmd.length = 2;
            SendCmd(cmd);

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
    int value = track->Position;
    //int dsp_num = track->Parent->Tag;
    dsp_delay_edit->Text = value;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::dsp_delay_editKeyDown(TObject *Sender, WORD &Key,
      TShiftState Shift)
{
    if (Key == VK_RETURN)
    {
        try{
            dsp_delay_trackbar->Position = dsp_delay_edit->Text.ToInt();
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
        HorzScrollBar->Visible = (pnlOperator->Width > Width);

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
    }

    for (int i=2;i<=OUTPUT_DSP_NUM;i++)
    {
        MoveOutputPanel(i, this);
    }
    for (int i=1;i<=OUTPUT_DSP_NUM;i++)
    {
        output_comp_btn[i-1]->Enabled = (GetVersionConfig(edtDeviceType->Text).is_comp);
    }
    for (int i=1;i<=OUTPUT_DSP_NUM;i++)
    {
        MoveWatchPanel(i+16, this, String(i), imgMasterMixBg->Left + imgMasterMixBg->Width + (i-1) * PANEL_WIDTH);
    }

    //============================
    //----------------------------------
    // 生成pnlmix背景
    pnlmix_background->Picture->Bitmap->Width = REAL_INPUT_DSP_NUM * PANEL_WIDTH;  // xDO: 原来是17个，包括automix，现在只按照输入数量
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
    ApplyConfigToUI();

    D1608Cmd cmd;
    cmd.type = CMD_TYPE_GLOBAL;
    cmd.id = offsetof(GlobalConfig, is_global_name);
    cmd.data.data_32 = cbGlobalDspName->Checked;
    cmd.length = 4;
    SendCmd2(cmd);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::cbPresetAutoSavedClick(TObject *Sender)
{
    D1608Cmd cmd;
    cmd.type = CMD_TYPE_GLOBAL;
    cmd.id = offsetof(GlobalConfig, auto_saved);
    cmd.data.data_8 = cbPresetAutoSaved->Checked;
    cmd.length = 1;
    SendCmd(cmd);
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
        D1608Cmd cmd;
        cmd.type = CMD_TYPE_PRESET;
        cmd.id = offsetof(ConfigMap, op_code.reboot);
        cmd.data.data_32 = 0;
        cmd.length = 4;
        SendCmd2(cmd);
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::cbMenuKeyFunctionChange(TObject *Sender)
{
    D1608Cmd cmd;
    cmd.type = CMD_TYPE_GLOBAL;
    cmd.id = offsetof(GlobalConfig, menu_key_function);
    cmd.data.data_32 = cbMenuKeyFunction->ItemIndex+1;
    cmd.length = 4;
    SendCmd2(cmd);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::cbUpKeyFunctionChange(TObject *Sender)
{
    D1608Cmd cmd;
    cmd.type = CMD_TYPE_GLOBAL;
    cmd.id = offsetof(GlobalConfig, up_key_function);
    cmd.data.data_32 = cbUpKeyFunction->ItemIndex+1;
    cmd.length = 4;
    SendCmd2(cmd);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::cbDownKeyFunctionChange(TObject *Sender)
{
    D1608Cmd cmd;
    cmd.type = CMD_TYPE_GLOBAL;
    cmd.id = offsetof(GlobalConfig, down_key_function);
    cmd.data.data_32 = cbDownKeyFunction->ItemIndex+1;
    cmd.length = 4;
    SendCmd2(cmd);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::cbLockUpDownMenuKeyClick(TObject *Sender)
{
    D1608Cmd cmd;
    cmd.type = CMD_TYPE_GLOBAL;
    cmd.id = offsetof(GlobalConfig, lock_updownmenu);
    cmd.data.data_8 = cbLockUpDownMenuKey->Checked;
    cmd.length = 1;
    SendCmd2(cmd);
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
            // 脱机，保存为逻辑preset，后缀smc
            // 清除缓存数据
            memset(&smc_config, 0, sizeof(smc_config));
            global_config.active_preset_id = cur_preset_id;

            smc_config.global_config = global_config;
            smc_config.all_config_map[0] = all_config_map[0];
            smc_config.all_config_map[1] = all_config_map[1];
            smc_config.all_config_map[2] = all_config_map[2];
            smc_config.all_config_map[3] = all_config_map[3];
            smc_config.all_config_map[4] = all_config_map[4];
            smc_config.all_config_map[5] = all_config_map[5];
            smc_config.all_config_map[6] = all_config_map[6];
            smc_config.all_config_map[7] = all_config_map[7];

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

            // 联机，保存为flash dump，读取完毕后转换成smc
            save_device_to_file_filename = SaveDialog1->FileName;

            // 替换文件名
            D1608Cmd cmd;
            cmd.type = CMD_TYPE_GLOBAL;
            cmd.id = offsetof(GlobalConfig, import_filename);
            cmd.data.data_string[0] = 's';
            strncpy(cmd.data.data_string+1, ExtractFileName(SaveDialog1->FileName).c_str(), 8);
            cmd.length = 10;
            SendCmd(cmd);

            memset(smc_config.device_flash_dump, 0, sizeof(smc_config.device_flash_dump));

            for (int address=PRESET_START_PAGE;address<0x8000000+256*1024;address+=1024)
            {
                // 从设备把数据同步上来
                FlashRW_Data flash_rw;
                //  每包2k数据
                flash_rw.start_address = address;
                flash_rw.end_address = 0x8000000+256*1024;

                TPackage package;
                memcpy(package.data, &flash_rw, sizeof(flash_rw));
                package.udp_port = UDP_PORT_READ_FLASH;
                package.data_size = sizeof(flash_rw);

                package_list.push_back(package);
            }

            std::reverse(package_list.begin(), package_list.end());

            TPackage package = package_list.back();
            udpControl->SendBuffer(dst_ip, package.udp_port, package.data, package.data_size);

            //last_command = UDP_PORT_READ_FLASH;
            //last_readflash_package = buff;
            restor_delay_count = 15;
            tmDelayBackup->Enabled = true;

            pbBackup->Max = package_list.size();
            pbBackup->Position = 0;
            pbBackup->Show();
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

        file->ReadBuffer(&smc_config, sizeof(smc_config));
        // 替换文件名
        smc_config.global_config.import_filename[0] = 'l';
        strncpy(smc_config.global_config.import_filename+1, ExtractFileName(OpenDialog1->FileName).c_str(), 8);

        for (int i=0;i<PRESET_NUM;i++)
        {
            all_config_map[i] = smc_config.all_config_map[i].ToConfigMap();
        }
        global_config = smc_config.global_config;

        delete file;

        if (!udpControl->Active)
        {
            // 脱机
            //global_config = smc_config.global_config;
            //memcpy(&all_config_map, &smc_config.all_config_map, sizeof(all_config_map));

            SetPresetId(global_config.active_preset_id);
            CloseDspDetail();
            tmDelayUpdateUITimer(NULL);
        }
        else
        {
            // 联机
            // Download To Device
            // 准备好所有报文
            //  global_config数据
            {
                D1608PresetCmd preset_cmd(version);
                preset_cmd.preset = 0x80;
                preset_cmd.store_page = 8;  // 使用8表示最后一页，TODO: 表达不是很清楚
                memcpy(preset_cmd.data, &smc_config.global_config, sizeof(smc_config.global_config));

                TPackage package;
                memcpy(package.data, &preset_cmd, sizeof(preset_cmd));
                package.udp_port = UDP_PORT_STORE_PRESET_PC2FLASH;
                package.data_size = sizeof(preset_cmd);

                package_list.push_back(package);
            }
            // 写入所有的preset数据
            for (int i=0;i<8;i++)
            {
                if (smc_config.global_config.avaliable_preset[i] == 1)
                {
                    D1608PresetCmd preset_cmd(version);
                    preset_cmd.preset = 0x80+i+1;
                    for (int store_page=0;store_page<9;store_page++)
                    {
                        preset_cmd.store_page = store_page;
                        switch(preset_cmd.store_page)
                        {
                        case 0:
                            memcpy(preset_cmd.data, &smc_config.all_config_map[i].input_dsp[0], sizeof(InputConfigMap)*4);
                            break;
                        case 1:
                            memcpy(preset_cmd.data, &smc_config.all_config_map[i].input_dsp[4], sizeof(InputConfigMap)*4);
                            break;
                        case 2:
                            memcpy(preset_cmd.data, &smc_config.all_config_map[i].input_dsp[8], sizeof(InputConfigMap)*4);
                            break;
                        case 3:
                            memcpy(preset_cmd.data, &smc_config.all_config_map[i].input_dsp[12], sizeof(InputConfigMap)*4);
                            break;
                        case 4:
                            memcpy(preset_cmd.data, &smc_config.all_config_map[i].output_dsp[0], sizeof(OutputConfigMap)*4);
                            break;
                        case 5:
                            memcpy(preset_cmd.data, &smc_config.all_config_map[i].output_dsp[4], sizeof(OutputConfigMap)*4);
                            break;
                        case 6:
                            memcpy(preset_cmd.data, &smc_config.all_config_map[i].output_dsp[8], sizeof(OutputConfigMap)*4);
                            break;
                        case 7:
                            memcpy(preset_cmd.data, &smc_config.all_config_map[i].output_dsp[12], sizeof(OutputConfigMap)*4);
                            break;
                        case 8:
                            memcpy(preset_cmd.data, &smc_config.all_config_map[i].master_mix,
                                    sizeof(MasterMixConfigMap));
                        }

                        TPackage package;
                        memcpy(package.data, &preset_cmd, sizeof(preset_cmd));
                        package.udp_port = UDP_PORT_STORE_PRESET_PC2FLASH;
                        package.data_size = sizeof(preset_cmd);

                        package_list.push_back(package);
                    }
                }
            }

            {
                // 追加一个0x89作为结束标志
                D1608PresetCmd preset_cmd(version);
                preset_cmd.preset = 0x89;

                TPackage package;
                memcpy(package.data, &preset_cmd, sizeof(preset_cmd));
                package.udp_port = UDP_PORT_STORE_PRESET_PC2FLASH;
                package.data_size = sizeof(preset_cmd);

                package_list.push_back(package);
            }


            std::reverse(package_list.begin(), package_list.end());

            TPackage package = package_list.back();
            udpControl->SendBuffer(dst_ip, package.udp_port, package.data, package.data_size);

            //last_command = UDP_PORT_STORE_PRESET_PC2FLASH;
            //last_restore_package = preset_cmd;
            restor_delay_count = 15;
            tmDelayBackup->Enabled = true;

            pbBackup->Max = package_list.size();
            pbBackup->Position = 0;
            pbBackup->Show();
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
        ::Sleep(1000);
        pbBackup->Hide();
    }
    else if ((restor_delay_count%5) == 1)
    {
        if(package_list.size() != 0)
        {
            TPackage package = package_list.back();
            udpControl->SendBuffer(dst_ip, package.udp_port, package.data, package.data_size);

            // 备份 恢复 流程使用的延时计时器
            memo_debug->Lines->Add(GetTime()+"retry syn");
        }
        else if (read_one_preset_package_list.size() != 0)
        {
            TPackage package = read_one_preset_package_list.back();
            udpControl->SendBuffer(dst_ip, package.udp_port, package.data, package.data_size);

            // 备份 恢复 流程使用的延时计时器
            memo_debug->Lines->Add(GetTime()+"retry syn");
        }
        else
        {
            memo_debug->Lines->Add(GetTime()+"列表空");
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
        D1608Cmd cmd;
        cmd.type = CMD_TYPE_PRESET;
        cmd.id = offsetof(ConfigMap, op_code.reboot);
        cmd.data.data_32 = 2;
        cmd.length = 4;
        SendCmd2(cmd);
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::lblDeviceNameDblClick(TObject *Sender)
{
    // 闪动屏幕
    D1608Cmd cmd;
    cmd.type = CMD_TYPE_PRESET;
    cmd.id = offsetof(ConfigMap, op_code.flash_oled);
    cmd.data.data_32 = 20;
    cmd.length = 4;
    SendCmd(cmd);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btnResetKeyFunctionClick(TObject *Sender)
{
    if (cbMenuKeyFunction->ItemIndex != 0)
    {
        cbMenuKeyFunction->ItemIndex = 0;
        cbMenuKeyFunction->OnChange(NULL);
    }

    if (cbUpKeyFunction->ItemIndex != 1)
    {
        cbUpKeyFunction->ItemIndex = 1;
        cbUpKeyFunction->OnChange(NULL);
    }

    if (cbDownKeyFunction->ItemIndex != 2)
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
        D1608Cmd cmd;
        cmd.type = CMD_TYPE_PRESET;
        cmd.id = offsetof(ConfigMap, op_code.reboot);
        cmd.data.data_32 = 1;
        cmd.length = 4;
        SendCmd2(cmd);
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
    }
    else if ((sendcmd_delay_count%5) == 1)
    {
        TPackage package = sendcmd_list.back();
        udpControl->SendBuffer(dst_ip, package.udp_port, package.data, package.data_size);

        // 备份 恢复 流程使用的延时计时器
        int addr = (int)&((D1608Cmd*)(sendcmd_list.end()-1))->data;
        memo_debug->Lines->Add(GetTime()+IntToHex(addr, 8)+"retry sendcmd");
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btnSaveLogClick(TObject *Sender)
{
    TStrings * log_strs = new TStringList();

    for (int i=0;i<lvLog->Items->Count;i++)
    {
        String line = lvLog->Items->Item[i]->Caption;
        line = line + "\t" + lvLog->Items->Item[i]->SubItems->Strings[0];
        line = line + "\t" + lvLog->Items->Item[i]->SubItems->Strings[1];
        log_strs->Add(line);
    }
    String path = ExtractFilePath(Application->ExeName);
    log_strs->SaveToFile(path+device_cpuid+".log");

    delete log_strs;
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
            D1608Cmd cmd;
            cmd.type = CMD_TYPE_GLOBAL;
            cmd.id = offsetof(GlobalConfig, mac_address);
            memcpy(cmd.data.data_string, mac_array, sizeof(mac_array));
            cmd.length = 6;
            SendCmd2(cmd);
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

            Copy1->Caption = "Copy Channel Input " + IntToStr(input_dsp_id);
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
                   && (selected_channel.channel_type == copied_channel.channel_type)
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
void __fastcall TForm1::Paste1Click(TObject *Sender)
{
    Paste1->Enabled = (selected_channel.channel_type != ctNone)
                   && (selected_channel.channel_type == copied_channel.channel_type)
                   && (selected_channel.channel_id != copied_channel.channel_id);

    // 考虑做成界面运动
    if (Paste1->Enabled)
    {
        ShowMessage("Copy channel "+IntToStr(copied_channel.channel_id)+" to channel "+IntToStr(selected_channel.channel_id));
        if (selected_channel.channel_type == ctInput)
        {
            config_map.input_dsp[selected_channel.channel_id-1] = config_map.input_dsp[copied_channel.channel_id-1];
            ApplyConfigToUI();
        }
        else if (selected_channel.channel_type == ctOutput)
        {
            config_map.output_dsp[selected_channel.channel_id-1] = config_map.output_dsp[copied_channel.channel_id-1];
            ApplyConfigToUI();
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::FormResize(TObject *Sender)
{
    nees_resize = true;
}
//---------------------------------------------------------------------------
void TForm1::StartReadOnePackage(int preset_id)
{
    memo_debug->Lines->Add(GetTime()+"切换到: "+IntToStr(preset_id));
    read_one_preset_package_list.clear();
    for (int store_page=0;store_page<9;store_page++)
    {
        D1608PresetCmd preset_cmd(version);
        preset_cmd.preset = preset_id; // 读取preset
        // 从0页读取
        preset_cmd.store_page = store_page;

        TPackage package;
        package.udp_port = UDP_PORT_READ_PRESET;
        memcpy(package.data, &preset_cmd, sizeof(preset_cmd));
        package.data_size = sizeof(preset_cmd);

        read_one_preset_package_list.push_back(package);
    }

    reverse(read_one_preset_package_list.begin(), read_one_preset_package_list.end());

    TPackage package = read_one_preset_package_list.back();
    udpControl->SendBuffer(dst_ip, package.udp_port, package.data, package.data_size);
}
//---------------------------------------------------------------------------



