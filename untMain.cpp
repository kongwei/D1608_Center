//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "untMain.h"
#include <winsock2.h>
#include <algorithm>
#include <stddef.h>
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

TForm1 *Form1;
ConfigMap all_config_map[PRESET_NUM];
ConfigMap config_map;
GlobalConfig global_config = {0};
int REAL_INPUT_DSP_NUM = 16;
int REAL_OUTPUT_DSP_NUM = 16;

static bool on_loading = false;
static String last_device_id;

static Word enter_key = VK_RETURN;

//static char head[] = "\x53\x65\x74\x50\x61\x72\x61\x00\x00\x00\x4D\x41\x54\x31\x36\x31\x30\x43\x6F\x6E\x66\x69\x67\x44\x61\x74\x61\x00\x00\x00";
static BLENDFUNCTION blend = {AC_SRC_OVER, 0, 200, 0};

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
        //WSACleanup();
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
    //bk_image->Picture = form->imgWatch->Picture;
    bk_image->Picture->Bitmap = new Graphics::TBitmap();
    bk_image->Picture->Bitmap->Height = bk_image->Height;
    bk_image->Picture->Bitmap->Width = bk_image->Width;
    bk_image->Canvas->Draw(-watch_panel->Left, -watch_panel->Top, form->imgBody->Picture->Graphic);

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
    // 参数处理
    on_loading = true;

    //x=GetSystemMetrics(SM_CXSCREEN)
    //y=GetSystemMetrics(SM_CYSCREEN)

    // 调整尺寸
    Width = 1800;//1366;
    Height = 780;//798;

    for (int i=0;i<32;i++)
    {
        level_meter[i][0] = -49;
        level_meter[i][1] = -100;
    }

    pnlOperator->Width = REAL_INPUT_DSP_NUM * PANEL_WIDTH + imgPresetBg->Width + REAL_OUTPUT_DSP_NUM * PANEL_WIDTH;
    pnlOperator->Width = Math::Max(pnlOperator->Width, Width-16);
        HorzScrollBar->Visible = (pnlOperator->Width > Width);
    pnlOperator->Height = 798;//-(728-584);
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

    InitConfigMap();
    btnDspResetEQ->Click();

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
    keep_live_count = 0;
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

void __fastcall TForm1::FormCreate(TObject *Sender)
{
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
    memo_debug->Lines->Add("InputConfigMap:" + IntToStr(sizeof(InputConfigMap)));
    memo_debug->Lines->Add("OutputConfigMap:" + IntToStr(sizeof(OutputConfigMap)));
    memo_debug->Lines->Add("MasterMixConfigMap:" + IntToStr(sizeof(MasterMixConfigMap)));
    memo_debug->Lines->Add("ConfigMap:" + IntToStr(sizeof(ConfigMap)));
    memo_debug->Lines->Add("mix_mute:" + IntToStr(sizeof(config_map.master_mix.mix_mute)));

    // 根据数量初始化控制器
    // Panel->Tag
    // Panel->Left
    // Label->Caption

    mmLog->Text = Now();
                                     
    if (FileExists("iap.log"))
    {
        try{
            log_file = new TFileStream("iap.log", fmOpenWrite);
            log_file->Seek(0, soFromEnd);
        }
        catch(...)
        {
            log_file = new TFileStream("iap"+Now().FormatString("yymmdd_hhnnss")+".log", fmCreate);
        }
    }
    else
    {
        log_file = new TFileStream("iap.log", fmCreate);
    }                       

    btnRefresh->Click();

    //tsSearch->Show();
    this->Repaint();

    pnlMix->BringToFront();

    // 设置最大最小值
    cg2_5V->MaxValue = 250+150;
    //cg3_3V->Progress = "-- ";
    cg3_3Vd->MaxValue = 330+150;
    cg5Va->MaxValue = 500+150;
    cg5Vd->MaxValue = 500+150;
    cg8Va->MaxValue = 800+150;
    cg8Vd->MaxValue = 800+150;
    cg12Va->MaxValue = 1200+150;
    cg_12Va->MaxValue = 1200+150;
    cg16Va->MaxValue = 1600+150;
    cg_16Va->MaxValue = 1600+150;
    cg48Va->MaxValue = 4800+150;

    cg2_5V->MinValue = 250-150;
    //cg3_3V->Progress = "-- ";
    cg3_3Vd->MinValue = 330-150;
    cg5Va->MinValue = 500-150;
    cg5Vd->MinValue = 500-150;
    cg8Va->MinValue = 800-150;
    cg8Vd->MinValue = 800-150;
    cg12Va->MinValue = 1200-150;
    cg_12Va->MinValue = 1200-150;
    cg16Va->MinValue = 1600-150;
    cg_16Va->MinValue = 1600-150;
    cg48Va->MinValue = 4800-150;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::FormDestroy(TObject *Sender)
{
    if (log_file)
    {
        delete log_file;
        log_file = NULL;
    }
}
//---------------------------------------------------------------------------
void TForm1::SendCmd(D1608Cmd& cmd)
{
    unsigned __int8 * p = (unsigned __int8*)&cmd;
    edtDebug->Text = "";
    String cmd_text = "";
    for (unsigned int i=sizeof(cmd.flag);i<sizeof(cmd.flag)+16+cmd.length;i++)
    {
        cmd_text = cmd_text + IntToHex(p[i], 2) + " ";
    }
    edtDebug->Text = cmd_text;

    if (on_loading || !udpControl->Active)
    {
    }
    else
    {
        if (cmd.type == 0)
            last_cmd = cmd;
        udpControl->SendBuffer(dst_ip, 2305, &cmd, sizeof(cmd));
    }

    if (!on_loading)
    {
        if (cmd.id < GetOffsetOfData(&config_map.op_code))
        {
            SetFileDirty(true);
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btnRefreshClick(TObject *Sender)
{
    AppendLog("刷新设备");

    // 删除上次已经未扫描到的项目
    for (int i=lvDevice->Items->Count-1;i>=0;i--)
    {
        TListItem * item = lvDevice->Items->Item[i];
        int count = (int)item->Data;
        count--;
        item->Data = (void*)count;
        if (count == 0)
        {
            lvDevice->Items->Delete(i);
        }
    }

    GetLocalIpList(lbIplist->Items);

    if (lvDevice->Selected == NULL)
    {
        last_select_device_ip = "";
    }
    else
    {
        last_select_device_ip = lvDevice->Selected->SubItems->Strings[0];
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
        display_device_name = slp_pack.hardware_name;
        display_device_name = display_device_name + "-" + slp_pack.id;
    }

    TListItem * item = NULL;
    // 查找是否列表中已经存在
    for (int i=0;i<lvDevice->Items->Count;i++)
    {
        TListItem * find_item = lvDevice->Items->Item[i];
        if (find_item->SubItems->Strings[2] == slp_pack.id)
        {
            // 更新属性
            find_item->Caption = display_device_name.UpperCase();
            find_item->Data = (void*)2;
            find_item->SubItems->Strings[0] = ip_address;
            find_item->SubItems->Strings[1] = ABinding->IP;
            //find_item->SubItems->Strings[2] = device_id;
            find_item->SubItems->Strings[3] = slp_pack.name;
            find_item->SubItems->Strings[4] = slp_pack.hardware_name;
            item = find_item;
            break;
        }
    }

    if (item == NULL)
    {
        item = lvDevice->Items->Add();
        item->Data = (void*)2;
        item->Caption = display_device_name.UpperCase();
        item->SubItems->Add(ip_address);
        item->SubItems->Add(ABinding->IP);
        item->SubItems->Add(slp_pack.id);
        item->SubItems->Add(slp_pack.name);
        item->SubItems->Add(slp_pack.hardware_name);
    }

    if (slp_pack.id[0] == '\x0')
    {
    }
    else
    {
        if ((last_device_id == "" || last_device_id == slp_pack.id) && !udpControl->Active)
        {
            // 连接第一个
            file_dirty = false;
            dst_ip = item->SubItems->Strings[0];
            btnSelect->Click();

            last_device_id = slp_pack.id;
        }

        if (last_device_id == slp_pack.id)
        {
            item->Selected = true;
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

    AppendLog("选择："+IntToStr(Item->Index+1)+" "+Item->SubItems->Strings[0]);
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

    // 断开文件
    if (file_dirty)
    {
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
        else
        {
            // 放弃
            preset_lib_filename = "";
            file_dirty = false;
        }
    }

    // TODO: 测试是否能够持续刷新，且不影响流程
    //cbAutoRefresh->Checked = false;

    String broadcast_ip = selected->SubItems->Strings[1];
    // 初始化socket
    udpControl->Active = false;
    udpControl->Bindings->Clear();
    udpControl->Bindings->Add();
    udpControl->Bindings->Items[0]->IP = broadcast_ip;
    udpControl->Bindings->Items[0]->Port = 0;
    udpControl->Active = true;

    UpdateCaption();

    D1608PresetCmd preset_cmd;
    preset_cmd.preset = 1; // 从0页读取preset
    preset_cmd.store_page = 0;
    udpControl->SendBuffer(dst_ip, 905, &preset_cmd, sizeof(preset_cmd));

    preset_cmd.preset = 0; // 0表示读取global_config
    udpControl->SendBuffer(dst_ip, 905, &preset_cmd, sizeof(preset_cmd));

    pnlOperator->Show();
    pnlDspDetail->Hide();
    pnlMix->Hide();
    cbWatch->Down = true;

    keep_live_count = 0;

    last_device_id = selected->SubItems->Strings[2];
    // 显示名称
    if (selected->SubItems->Strings[3] != "")
        lblDeviceName->Caption = selected->SubItems->Strings[3];
    else
        lblDeviceName->Caption = selected->SubItems->Strings[4]+"-"+selected->SubItems->Strings[2];
}
//---------------------------------------------------------------------------
void __fastcall TForm1::tmSLPTimer(TObject *Sender)
{
    tmSLP->Enabled = false;

    if (lbIplist->Count > 0)
    {
        local_broadcast_ip = lbIplist->Items->Strings[0];
        lbIplist->Items->Delete(0);

        AppendLog("尝试侦听地址： " + local_broadcast_ip);

        udpSLP->Active = false;
        udpSLP->Bindings->Clear();
        udpSLP->Bindings->Add();
        udpSLP->Bindings->Items[0]->IP = local_broadcast_ip;
        udpSLP->Bindings->Items[0]->Port = 0;
        udpSLP->BroadcastEnabled = true;

        try{
            udpSLP->Active = true;
            char search_flag[] = "rep";
            udpSLP->SendBuffer("255.255.255.255", 888, search_flag, sizeof(search_flag));
        }
        catch(...)
        {
            AppendLog("网络异常");
        }
    }
    else
    {
        // 自动刷新开关
        if (cbAutoRefresh->Checked)
        {
            static int count = spInterval->Value;
            count--;
            if (count <= 0)
            {
                count = spInterval->Value;
                // 重启刷新
                btnRefresh->Click();
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
int CalcVot1(unsigned __int16 true_data1, float ra, float rb)
{
    return true_data1 * (ra+rb) / ra / 10;
}
int CalcVot2(unsigned __int16 true_data1, unsigned __int16 true_data2, float ra, float rb, float rc, float rd)
{
    return ((true_data2 * (rc+rd) / rc) - (true_data1 * (rd/rc) * (ra+rb) / ra))  / 10;
}
String IntToAbsSring(int value)
{
    return String(abs(value));
}
void __fastcall TForm1::udpControlUDPRead(TObject *Sender, TStream *AData,
      TIdSocketHandle *ABinding)
{
    if (ABinding->PeerPort == 2305)
    {
        D1608Cmd cmd;
        AData->ReadBuffer(&cmd, std::min(sizeof(cmd), AData->Size));

        //memo_debug->Lines->Add("Reply：" + CmdLog(cmd));

        // id == 1 表示全局配置
        if (cmd.type == 1)
        {
            if (cmd.id == offsetof(GlobalConfig, d1616_name))
            {
            }
            else if (cmd.id == GetOffsetOfData(&config_map.op_code.noop))
            {
                int preset_id = cmd.data.data_32;
                if (preset_id != cur_preset_id)
                {
                    D1608PresetCmd preset_cmd;
                    preset_cmd.preset = preset_id; // 读取preset
                    // 从0页读取
                    preset_cmd.store_page = 0;
                    udpControl->SendBuffer(dst_ip, 905, &preset_cmd, sizeof(preset_cmd));
                }

                keep_live_count = 0;
                //tsOperator->Caption = "操作(连接)";
                shape_live->Show();
                shape_link->Show();
                shape_power->Show();
            }
        }
        // id == 0 表示preset配置
        else if (cmd.id == GetOffsetOfData(&config_map.op_code.WatchLevel))
        {
            // 音量输出
            MsgWatchHandle(cmd);
        }
        else if (cmd.id == GetOffsetOfData(&config_map.op_code.switch_preset))
        {
            D1608PresetCmd preset_cmd;
            preset_cmd.preset = cur_preset_id;
            // 从0页读取
            preset_cmd.store_page = 0;

            // 如果没有打开文件，则需要从下位机同步
            //if (preset_lib_filename == "")
            {
                udpControl->SendBuffer(dst_ip, 905, &preset_cmd, sizeof(preset_cmd));
            }
        }
        else if (cmd.id == GetOffsetOfData(&config_map.op_code.adc))
        {
            __int16 data[16];

            // 计算检测到的电压 3.3v ~ 4096
            for (int i=0; i<ADC_NUM; i++)
            {
                data[i] = cmd.data.data_16_array[i] * 2500 / cmd.data.data_16_array[1];
            }

            for (int i=0; i<ADC_NUM; i++)
            {
                ValueListEditor1->Cells[1][i+1] = cmd.data.data_16_array[i];
            }

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
            calc_data._2_5v  = true_data->_2_5v / 10;
            calc_data.base   = 2500 / 10;
            calc_data._5vd   = CalcVot1(true_data->_5vd, 6.81,  6.81);                             
            calc_data._8vdc  = CalcVot1(true_data->_8vdc, 4.75, 10);                                
            calc_data._8vac  = CalcVot1(true_data->_8vac, 4.75, 10);                                
            calc_data._8va   = CalcVot1(true_data->_8va, 4.75, 10);                                
            calc_data._x16vac= CalcVot2(true_data->_16vac, true_data->_x16vac, 3.32, 20, 14.7, 20);   
            calc_data._x16va = CalcVot2(true_data->_16va, true_data->_x16va, 3.32, 20, 14.7, 20);   
            calc_data._46vc  = CalcVot1(true_data->_46vc, 4.75, 75); 
            calc_data._48va  = CalcVot1(true_data->_48va, 4.75, 75);                                
            calc_data._46va  = CalcVot1(true_data->_46va, 4.75, 75);                                
            calc_data._5va   = CalcVot1(true_data-> _5va, 6.81, 6.81);                                
            calc_data._x12va = CalcVot2(true_data->_12va, true_data->_x12va, 4.75, 14.7, 10, 14.7); 
            calc_data._12va  = CalcVot1(true_data->_12va, 4.75, 14.7);                              
            calc_data._16va  = CalcVot1(true_data->_16va, 3.32, 20);                                
            calc_data._16vac = CalcVot1(true_data->_16vac, 3.32, 20);                                

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

            // 输出界面
            calc_data._5vd   = CalcVot1(true_data->_5vd, 6.81,  6.81);                             
            calc_data._8vdc  = CalcVot1(true_data->_8vdc, 4.75, 10);                                
            calc_data._8vac  = CalcVot1(true_data->_8vac, 4.75, 10);                                
            calc_data._8va   = CalcVot1(true_data->_8va, 4.75, 10);                                
            calc_data._x16vac= CalcVot2(true_data->_16vac, true_data->_x16vac, 3.32, 20, 14.7, 20);   
            calc_data._x16va = CalcVot2(true_data->_16va, true_data->_x16va, 3.32, 20, 14.7, 20);   
            calc_data._46vc  = CalcVot1(true_data->_46vc, 4.75, 75); 
            calc_data._48va  = CalcVot1(true_data->_48va, 4.75, 75);                                
            calc_data._46va  = CalcVot1(true_data->_46va, 4.75, 75);                                
            calc_data._5va   = CalcVot1(true_data-> _5va, 6.81, 6.81);
            calc_data._x12va = CalcVot2(true_data->_12va, true_data->_x12va, 4.75, 14.7, 10, 14.7); 
            calc_data._12va  = CalcVot1(true_data->_12va, 4.75, 14.7);                              
            calc_data._16va  = CalcVot1(true_data->_16va, 3.32, 20);                                
            calc_data._16vac = CalcVot1(true_data->_16vac, 3.32, 20);                                

            //====================================================================
            lbl2_5V->Caption = String::FormatFloat("0.00 ", calc_data._2_5v / 100.0);
            lbl3_3V->Caption = "-- ";
            lbl3_3Vd->Caption = String::FormatFloat("0.00 ", (calc_data._2_5v+75) / 100.0);
            lbl5Va->Caption = String::FormatFloat("0.00 ", calc_data._5va / 100.0);
            lbl5Vd->Caption = String::FormatFloat("0.00 ", calc_data._5vd / 100.0);
            lbl8Va->Caption = String::FormatFloat("0.00 ", calc_data._8va / 100.0);
            lbl8Vd->Caption = String::FormatFloat("0.00 ", calc_data._8vdc / 100.0);
            lbl12Va->Caption = String::FormatFloat("0.00 ", calc_data._12va / 100.0);
            lbl_12Va->Caption = String::FormatFloat("0.00 ", calc_data._x12va / 100.0);
            lbl16Va->Caption = String::FormatFloat("0.00 ", calc_data._16va / 100.0);
            lbl_16Va->Caption = String::FormatFloat("0.00 ", calc_data._x16va / 100.0);
            lbl48Va->Caption = String::FormatFloat("0.00 ", calc_data._48va / 100.0);

            //====================================================================
            cg2_5V->Progress = calc_data._2_5v;
            //cg3_3V->Progress = "-- ";
            cg3_3Vd->Progress = (calc_data._2_5v+75);
            cg5Va->Progress = calc_data._5va;
            cg5Vd->Progress = calc_data._5vd;
            cg8Va->Progress = calc_data._8va;
            cg8Vd->Progress = calc_data._8vdc;
            cg12Va->Progress = calc_data._12va;
            cg_12Va->Progress = calc_data._x12va + 2400;
            cg16Va->Progress = calc_data._16va;
            cg_16Va->Progress = calc_data._x16va + 3200;
            cg48Va->Progress = calc_data._48va;

            // 补充到曲线图
            if (active_adc != NULL)
            {
                try {
                    double value = active_adc->Caption.ToDouble();

                    Series1->Add(value, "", clLime);

                    //lineUpLimit->Add(line_value*1.5, "e", clRed);
                    //lineDownLimit->Add(line_value*0.5, "b", clRed);
                    lineUpLimit->Add(line_value+1.5, "e", clRed);
                    lineDownLimit->Add(line_value-1.5, "b", clRed);

                    Chart1->BottomAxis->Scroll(1, false);
                }
                catch(...)
                {
                }
            }

            //====================================================================
            lbl2_5mA->Caption = "-- ";
            lbl3_3mA->Caption = IntToAbsSring((int)((calc_data._8va - calc_data._8vdc) / 0.27 * 0.1)) + " ";   //8Vd * 0.10
            lbl3_3mAd->Caption = IntToAbsSring((int)((calc_data._8va - calc_data._8vdc) / 0.27 * 0.85)) + " "; //8Vd * 0.85
            lbl5mAa->Caption = IntToAbsSring((int)((calc_data._8va - calc_data._8vac) / 0.27)) + " ";          // 8Va
            lbl5mAd->Caption = IntToAbsSring((int)((calc_data._8va - calc_data._8vdc) / 0.27 * 0.05)) + " ";   // 8Vd * 0.05
            lbl8mAa->Caption = IntToAbsSring((int)((calc_data._8va - calc_data._8vac) / 0.27)) + " ";
            lbl8mAd->Caption = IntToAbsSring((int)((calc_data._8va - calc_data._8vdc) / 0.27)) + " ";
            lbl12mAa->Caption = IntToAbsSring((calc_data._16vac - calc_data._16va) / 0.5) + " ";               // 16Va
            lbl_12mAa->Caption = IntToAbsSring((calc_data._x16va - calc_data._x16vac) / 0.5) + " ";            // -16Va
            lbl16mAa->Caption = IntToAbsSring((calc_data._16vac - calc_data._16va) / 0.5) + " ";
            lbl_16mAa->Caption = IntToAbsSring((calc_data._x16va - calc_data._x16vac) / 0.5) + " ";
            lbl48mAa->Caption = IntToAbsSring((calc_data._48va - calc_data._46vc) / 0.5) + " ";
        }
        else
        {
            // 如果是当前调节的数据，需要忽略
            if (last_cmd.id != cmd.id || !paint_agent->IsMouseDown())
            {
                memo_debug->Lines->Add("Reply：" + CmdLog(cmd));

                memcpy(((char*)(&config_map))+cmd.id, (char*)&cmd.data, cmd.length);
                OnFeedbackData(cmd.id, cmd.length);
            }
        }
    }
    else if (ABinding->PeerPort == 903)
    {
        typedef struct
        {
            int timer;
            short event_id;
            short event_data;
        }Event;
        typedef unsigned char MacCode[8];
        struct
        {
            int address;
            union{
                Event event[128];
                MacCode mac[128];
            };
        }buff;

        AData->ReadBuffer(&buff, sizeof(buff));
        if ((buff.address >= LOG_START_PAGE) && (buff.address < MAC_LIST_START_PAGE))
        {
            // 日志
            for (int i=0;i<128;i++)
            {
                if (buff.event[i].timer != 0xFFFFFFFF)
                {
                    TListItem * item = lvLog->Items->Add();

                    int ms = buff.event[i].timer % 10;  buff.event[i].timer /= 10;
                    int sec = buff.event[i].timer % 60; buff.event[i].timer /= 60;
                    int min = buff.event[i].timer % 60; buff.event[i].timer /= 60;

                    item->Caption = IntToStr(buff.event[i].timer)+":"
                                  + IntToStr(min)+":"
                                  + IntToStr(sec)+"."
                                  + IntToStr(ms);
                    item->SubItems->Add(buff.event[i].event_id);
                    item->SubItems->Add(buff.event[i].event_data);
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
        }

        if (buff.address+1024 < MAC_LIST_START_PAGE+MAC_LIST_SIZE)
        {
            buff.address += 1024;
            udpControl->SendBuffer(dst_ip, 903, &buff, sizeof(buff));
        }
    }
    else if (ABinding->PeerPort == 904)
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
                TListItem * item = lvDebug->Items->Add();

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
    else if (ABinding->PeerPort == 905)
    {
        D1608PresetCmd preset_cmd;
        AData->ReadBuffer(&preset_cmd, std::min(sizeof(preset_cmd), AData->Size));
        if (preset_cmd.preset == 0)
        {
            // global_config
            memcpy(&global_config, preset_cmd.data, sizeof(global_config));
            UpdateCaption();
            // 显示版本信息
            edtDeviceType->Text = global_config.device_type;
            edtBuildTime->Text = global_config.build_time;
            edtDeviceName->Text = global_config.d1616_name;
            //TDateTime d = edtBuildTime->Text;

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

                REAL_INPUT_DSP_NUM = 0;
                for (int i=0;i<4;i++)
                    REAL_INPUT_DSP_NUM += global_config.ad_da_card[i];
                if (REAL_INPUT_DSP_NUM == 0)
                    REAL_INPUT_DSP_NUM = 16;

                REAL_OUTPUT_DSP_NUM = 0;
                for (int i=4;i<8;i++)
                    REAL_OUTPUT_DSP_NUM += global_config.ad_da_card[i];
                if (REAL_OUTPUT_DSP_NUM == 0)
                    REAL_OUTPUT_DSP_NUM = 16;

                SetIOChannelNum();
            }
            if (global_config.version >= offsetof(GlobalConfig, auto_saved))
            {
                // 读取'自动保存'配置
                cbPresetAutoSaved->Checked = ((global_config.auto_saved == 1) || (global_config.auto_saved == 0xFF));
            }
        }
        else
        {
            switch(preset_cmd.store_page)
            {
            case 0:
                memcpy(&config_map.input_dsp[0], preset_cmd.data, sizeof(config_map.input_dsp[0])*4);
                break;
            case 1:
                memcpy(&config_map.input_dsp[4], preset_cmd.data, sizeof(config_map.input_dsp[0])*4);
                break;
            case 2:
                memcpy(&config_map.input_dsp[8], preset_cmd.data, sizeof(config_map.input_dsp[0])*4);
                break;
            case 3:
                memcpy(&config_map.input_dsp[12],preset_cmd.data,  sizeof(config_map.input_dsp[0])*4);
                break;
            case 4:
                memcpy(&config_map.output_dsp[0],preset_cmd.data,  sizeof(config_map.output_dsp[0])*4);
                break;
            case 5:
                memcpy(&config_map.output_dsp[4],preset_cmd.data,  sizeof(config_map.output_dsp[0])*4);
                break;
            case 6:
                memcpy(&config_map.output_dsp[8],preset_cmd.data,  sizeof(config_map.output_dsp[0])*4);
                break;
            case 7:
                memcpy(&config_map.output_dsp[12], preset_cmd.data, sizeof(config_map.output_dsp[0])*4);
                break;
            case 8:
                memcpy(&config_map.master_mix, preset_cmd.data,
                        sizeof(config_map.master_mix));
                break;
            }

            bool download_all_preset = preset_cmd.preset & 0x80;
            int preset_id = preset_cmd.preset & 0x7F;

            // 读取下一个preset
            if (download_all_preset && (preset_id<8) && preset_cmd.store_page == 8)
            {
                preset_cmd.preset++;
                preset_cmd.store_page = 0;

                // 切换 config_map
                all_config_map[preset_id] = config_map;
            }
            else
            {
                preset_cmd.store_page++;
            }


            if (preset_cmd.store_page < 9)
            {
                // next
                udpControl->SendBuffer(dst_ip, 905, &preset_cmd, sizeof(preset_cmd));
            }
            else
            {
                SetPresetId(preset_id);
                ApplyConfigToUI();  // 子窗体的数据在加载时更新
                CloseDspDetail();
            }
        }
    }
    else if (ABinding->PeerPort == 907)
    {
        D1608PresetCmd preset_cmd;
        AData->ReadBuffer(&preset_cmd, std::min(sizeof(preset_cmd), AData->Size));

        {
            bool download_all_preset = preset_cmd.preset & 0x80;
            int preset_id = preset_cmd.preset & 0x7F;

            if (download_all_preset && (preset_id<8) && preset_cmd.store_page == 8)
            {
                preset_cmd.preset++;
                preset_cmd.store_page = 0;

                // 切换 config_map
                config_map = all_config_map[preset_id];
            }
            else
            {
                preset_cmd.store_page++;
            }

            switch(preset_cmd.store_page)
            {
            case 0:
                memcpy(preset_cmd.data, &config_map.input_dsp[0], sizeof(config_map.input_dsp[0])*4);
                break;
            case 1:
                memcpy(preset_cmd.data, &config_map.input_dsp[4], sizeof(config_map.input_dsp[0])*4);
                break;
            case 2:
                memcpy(preset_cmd.data, &config_map.input_dsp[8], sizeof(config_map.input_dsp[0])*4);
                break;
            case 3:
                memcpy(preset_cmd.data, &config_map.input_dsp[12], sizeof(config_map.input_dsp[0])*4);
                break;
            case 4:
                memcpy(preset_cmd.data, &config_map.output_dsp[0], sizeof(config_map.output_dsp[0])*4);
                break;
            case 5:
                memcpy(preset_cmd.data, &config_map.output_dsp[4], sizeof(config_map.output_dsp[0])*4);
                break;
            case 6:
                memcpy(preset_cmd.data, &config_map.output_dsp[8], sizeof(config_map.output_dsp[0])*4);
                break;
            case 7:
                memcpy(preset_cmd.data, &config_map.output_dsp[12], sizeof(config_map.output_dsp[0])*4);
                break;
            case 8:
                memcpy(preset_cmd.data, &config_map.master_mix,
                        sizeof(config_map.master_mix));
            default:
                break;
            }
            if (preset_cmd.store_page <= 8)
            {
                udpControl->SendBuffer(dst_ip, 907, &preset_cmd, sizeof(preset_cmd));
            }
        }
    }
}
//---------------------------------------------------------------------------
void TForm1::MsgWatchHandle(const D1608Cmd& cmd)
{
    for (int i=0;i<32;i++)
    {
        int value = cmd.data.data_32_array[i];
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
                    int comp_level = cmd.data.data_32_array[i+16];
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
    if (cbWatch->Down)
    {
        D1608Cmd cmd;
        cmd.id = GetOffsetOfData(&config_map.op_code.WatchLevel);
        SendCmd(cmd);

        cmd.id = GetOffsetOfData(&config_map.op_code.adc);
        SendCmd(cmd);
    }
    else
    {
        for (int i=0;i<32;i++)
        {
            UpdateWatchLevel(i, -49);
        }
    }

    // keep alive
    if ((keep_live_count < 5) && udpControl->Active)
    {
        keep_live_count++;
    
        D1608Cmd cmd;
        cmd.type = 1;
        cmd.id = GetOffsetOfData(&config_map.op_code.noop);
        SendCmd(cmd);
    }
    else
    {
        udpControl->Active = false;
        //tsOperator->Caption = "操作(断开)";
        shape_live->Hide();
        shape_link->Hide();
        shape_power->Hide();
        // Level Meter归零
        for (int i=0;i<32;i++)
        {
            UpdateWatchLevel(i, -49);
        }

        // 重新启动自动刷新
        cbAutoRefresh->Checked = true;
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
            dsp_gain_trackbar->BackGround = p_input_inner_level->BackGround;
            dsp_gain_trackbar->Max = p_input_inner_level->Max;
            dsp_gain_trackbar->Min = p_input_inner_level->Min;
            lblDSPInfo->Caption = "Input Channel " + IntToStr(btn->Tag) + " DSP Setup";
            pnlDspDetail->Left = input_panel_bkground->Left;

            int dsp_num = btn->Tag;
            dsp_gain_trackbar->Position = config_map.input_dsp[dsp_num-1].level_b;
            btnPhanton->Down = config_map.input_dsp[dsp_num-1].phantom_switch;
            btnPhanton->Show();

            // 调整PaintBox1的尺寸
            PaintBox1->Left = 8;
            PaintBox1->Width = 761;
            pbComp->Hide();

            // 隐藏COMP界面
            pnlComp->Hide();
            btnDspComp->Hide();

            btnDspEq->Down = config_map.input_dsp[dsp_num-1].eq_switch;
        }
        else
        {
            dsp_gain_trackbar->BackGround = p_output_inner_level->BackGround;
            dsp_gain_trackbar->Max = p_output_inner_level->Max;
            dsp_gain_trackbar->Min = p_output_inner_level->Min;
            lblDSPInfo->Caption = "Output Channel " + IntToStr(btn->Tag-100) + " DSP Setup";
            pnlDspDetail->Left = output_panel_bkground->Left + output_panel_bkground->Width - pnlDspDetail->Width;

            int dsp_num = btn->Tag-100;
            dsp_gain_trackbar->Position = config_map.output_dsp[dsp_num-1].level_b;
            btnPhanton->Hide();

            // 调整PaintBox1的尺寸
            PaintBox1->Left = 248;
            PaintBox1->Width = 521;
            pbComp->Show();

            // COMP
            btnDspComp->Down = config_map.output_dsp[dsp_num-1].comp_switch;
            edtCompRatio->Text = Ration2String(config_map.output_dsp[dsp_num-1].ratio);
            edtCompThreshold->Text = config_map.output_dsp[dsp_num-1].threshold/10.0;
            edtCompAttackTime->Text = config_map.output_dsp[dsp_num-1].attack_time/10.0;
            edtCompReleaseTime->Text = config_map.output_dsp[dsp_num-1].release_time/10.0;
            edtCompGain->Text = config_map.output_dsp[dsp_num-1].comp_gain/10.0;
            cbCompAutoTime->Checked = config_map.output_dsp[dsp_num-1].auto_time;
                edtCompReleaseTime->Enabled = !cbCompAutoTime->Checked;
                edtCompAttackTime->Enabled = !cbCompAutoTime->Checked;

            pnlComp->Show();
            btnDspComp->Show();

            btnDspEq->Down = config_map.output_dsp[dsp_num-1].eq_switch;
        }

        dsp_gain_trackbar->OnChange(dsp_gain_trackbar);
        dsp_delay_trackbar->OnChange(dsp_delay_trackbar);

        pnlDspDetail->Top = 168;
        pnlDspDetail->Show();

        panel_agent->LoadPreset();
        PaintBox1->Refresh();
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
void __fastcall TForm1::M41Click(TObject *Sender)
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
        cmd.data.data_8 = 0;
    }
    else if (popup_label->Caption == "MIC(0)")
    {
        cmd.data.data_8 = 1;
    }
    else if (popup_label->Caption == "400mv")
    {
        cmd.data.data_8 = 5;
    }
    else if (popup_label->Caption == "10dBv")
    {
        cmd.data.data_8 = 6;
    }
    else if (popup_label->Caption == "22dBu")
    {
        cmd.data.data_8 = 3;
    }
    else if (popup_label->Caption == "24dBu")
    {
        cmd.data.data_8 = 7;
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
    if (popup_label->Caption == "200mv")
    {
        cmd.data.data_8 = 7;
    }
    else if (popup_label->Caption == "10dBv")
    {
        cmd.data.data_8 = 3;
    }
    else if (popup_label->Caption == "22dBu")
    {
        cmd.data.data_8 = 1;
    }
    else if (popup_label->Caption == "24dBu")
    {
        cmd.data.data_8 = 0;
    }
    SendCmd(cmd);

    config_map.output_dsp[dsp_num-1].gain = cmd.data.data_8;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::M41DrawItem(TObject *Sender, TCanvas *ACanvas,
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
void __fastcall TForm1::M41MeasureItem(TObject *Sender, TCanvas *ACanvas,
      int &Width, int &Height)
{
    Width = Label1->Width-19;
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

    filter_set.GetFilter(HP_FILTER)->ChangFilterParameter("12dB Butterworth High", preset_freq_list[FIRST_FILTER-1], 0, 4.09);
        filter_set.SetBypass(HP_FILTER, true);
        filter_set.GetFilter(HP_FILTER)->name = "H";
        filter_set.RepaintPaint(HP_FILTER);

    filter_set.GetFilter(LP_FILTER)->ChangFilterParameter("12dB Butterworth Low", preset_freq_list[LP_FILTER-1], 0, 4.09);
        filter_set.SetBypass(LP_FILTER, true);
        filter_set.GetFilter(LP_FILTER)->name = "L";
        filter_set.RepaintPaint(LP_FILTER);

    // 压缩参数
    edtCompRatio->Text = 1;
    edtCompRatio->OnKeyDown(edtCompRatio, enter_key, TShiftState());
    edtCompThreshold->Text = 0;
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
    }
    else if (Key == VK_ESCAPE)
    {
        edt->OnExit(edt);
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
    }
    else if (Key == VK_ESCAPE)
    {
        edt->OnExit(edt);
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
    }
    else if (Key == VK_ESCAPE)
    {
        edt->OnExit(edt);
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
    int dsp_num = edt->Tag;
    if (input_level_trackbar[dsp_num-1] != NULL)
    {
        InputVolumeChange(input_level_trackbar[dsp_num-1]);
        edt->OnClick = input_panel_level_editClick; 
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::output_panel_level_editExit(TObject *Sender)
{
    TEdit * edt = (TEdit*)Sender;
    int dsp_num = edt->Tag;
    if (output_level_trackbar[dsp_num-1] != NULL)
    {
        OutputVolumeChange(output_level_trackbar[dsp_num-1]);
        edt->OnClick = input_panel_level_editClick;
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::master_panel_level_editExit(TObject *Sender)
{
    TEdit * edt = (TEdit*)Sender;

    MasterVolumeChange(master_panel_trackbar);
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

    if (level > 0)
    {
        r.bottom = 24-0+1;
        pb_watch->Canvas->Brush->Color = clYellow;
        pb_watch->Canvas->FillRect(r);
    }

    if (level > 12)
    {
        r.bottom = 24-12+1;
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

        if (comp_level > 0)
        {
            r.bottom = 24-0+1;
            pb_watch->Canvas->Brush->Color = clYellow;
            pb_watch->Canvas->FillRect(r);
        }

        if (comp_level > 12)
        {
            r.bottom = 24-12+1;
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
    else
    {
        label->Caption = edtInput->Text;
    }
    
    D1608Cmd cmd;
    cmd.type = cbGlobalDspName->Checked;
    if (cmd.type)
    {
        cmd.id = offsetof(GlobalConfig, input_dsp_name[label->Tag]);
    }
    else
    {
        cmd.id = GetOffsetOfData(&config_map.input_dsp[label->Tag].dsp_name);
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
    if (cmd.type)
    {
        cmd.id = offsetof(GlobalConfig, output_dsp_name[label->Tag]);
    }
    else
    {
        cmd.id = GetOffsetOfData(&config_map.output_dsp[label->Tag].dsp_name);
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
void __fastcall TForm1::pnlmix_level_editExit(TObject *Sender)
{
    TEdit * edt = (TEdit*)Sender;
    int dsp_num = edt->Tag;
    if (mix_level_trackbar[dsp_num-1] != NULL)
    {
        pnlmix_level_trackbarChange(mix_level_trackbar[dsp_num-1]);
        edt->OnClick = input_panel_level_editClick; 
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
    }
    else if (Key == VK_ESCAPE)
    {
        edt->OnExit(edt);
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

        file->ReadBuffer(&config_map, sizeof(config_map));

        delete file;

        ApplyConfigToUI();
        CloseDspDetail();

        // Download To Device
        D1608PresetCmd preset_cmd;
        preset_cmd.preset = cur_preset_id;
        preset_cmd.store_page = 0;
        memcpy(preset_cmd.data, &config_map.input_dsp[0], sizeof(config_map.input_dsp[0])*4);

        if (udpControl->Active)
        {
            udpControl->SendBuffer(dst_ip, 907, &preset_cmd, sizeof(preset_cmd));
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::SaveAllPresetAsClick(TObject *Sender)
{
    SaveDialog1->Filter = "presetlib|*.presetlib";
    if (SaveDialog1->Execute())
    {
        preset_lib_filename = SaveDialog1->FileName;
        SaveAllPreset->Click();
    }
    else
    {
        return;
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::SaveAllPresetClick(TObject *Sender)
{
    if (preset_lib_filename == "")
    {
        SaveAllPresetAs->Click();
    }

    if (preset_lib_filename != "")
    {
        // save config_map to file
        TFileStream * file = new TFileStream(preset_lib_filename, fmCreate);
        if (!file)
        {
            ShowMessage("打开文件失败");
            return;
        }

        all_config_map[cur_preset_id] = config_map;
        SetPresetLibFilename(SaveDialog1->FileName);
        SetFileDirty(false);

        file->WriteBuffer(&all_config_map, sizeof(all_config_map));

        delete file;
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::LoadAllPresetClick(TObject *Sender)
{
    OpenDialog1->Filter = "presetlib|*.presetlib";
    if (OpenDialog1->Execute())
    {
        if (udpControl->Active)
        {
            // 选项： 同步到下位机 / 断开下位机
            int syn_select = Application->MessageBox("请选择数据同步方式", "加载选项", MB_YESNOCANCEL);
            if (syn_select == IDYES)
            {
                // 同步
            }
            else if (syn_select == IDNO)
            {
                udpControl->Active = false;
            }
            else
            {
                // 取消
                return;
            }
        }

        // save config_map to file
        TFileStream * file = new TFileStream(OpenDialog1->FileName, fmOpenRead);
        if (!file)
        {
            ShowMessage("打开文件失败");
            return;
        }

        SetPresetLibFilename(OpenDialog1->FileName);
        SetFileDirty(false);

        file->ReadBuffer(&all_config_map, sizeof(all_config_map));

        delete file;

        config_map = all_config_map[0];
        SetPresetId(1);

        ApplyConfigToUI();
        CloseDspDetail();

        if (udpControl->Active)
        {
            // Download To Device
            D1608PresetCmd preset_cmd;
            preset_cmd.preset = 0x80;
            preset_cmd.store_page = 0;
            memcpy(preset_cmd.data, &config_map.input_dsp[0], sizeof(config_map.input_dsp[0])*4);

            udpControl->SendBuffer(dst_ip, 907, &preset_cmd, sizeof(preset_cmd));
        }
    }
}
//---------------------------------------------------------------------------
static String InputGain2String(int gain)
{
    switch (gain)
    {
    case 0:
        return "MIC";
    case 1:
        return "MIC(0)";
    case 5:
        return "400mv";
    case 6:
        return "10dBv";
    case 3:
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
        return "200mv";
    case 3:
        return "10dBv";
    case 1:
        return "22dBu";
    case 0:
        return "24dBu";
    }

    return "ERROR";
}
void __fastcall TForm1::ApplyConfigToUI()
{
    on_loading = true;

    lblPresetName->Caption = global_config.preset_name[cur_preset_id];

    for (int i=0;i<PRESET_NUM;i++)
        clbAvaliablePreset->Checked[i] = global_config.avaliable_preset[i];

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
        else
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
        udpControl->SendBuffer(dst_ip, 906, "\xFF", 1);
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::StoreAsClick(TObject *Sender)
{
    if (udpControl->Active)
    {
        TMenuItem * menu = (TMenuItem*)Sender;
        char preset_id[2] = "\xff";
        preset_id[0] = menu->Tag;
        udpControl->SendBuffer(dst_ip, 906, preset_id, 1);
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::RecallClick(TObject *Sender)
{
    TMenuItem * menu = (TMenuItem*)Sender;
    all_config_map[cur_preset_id] = config_map;
    SetPresetId(menu->Tag);
    config_map = all_config_map[cur_preset_id];

    ApplyConfigToUI();
    CloseDspDetail();

    D1608Cmd cmd;
    cmd.id = GetOffsetOfData(&config_map.op_code.switch_preset);
    cmd.data.data_32 = menu->Tag;
    SendCmd(cmd);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btnSetIpClick(TObject *Sender)
{
    D1608Cmd cmd;
    cmd.type = 1;
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
    SendCmd(cmd);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btnGetLogClick(TObject *Sender)
{
    lvLog->Clear();
    int log_address = 0x8000000+128*1024+10*5*2*1024;
    udpControl->SendBuffer(dst_ip, 903, &log_address, 4+1024);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btnGetDebugClick(TObject *Sender)
{
    char buf[1024] = {0};
    udpControl->SendBuffer(dst_ip, 904, buf, 1024);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::lblPresetNameClick(TObject *Sender)
{
    lblPresetName->Caption = InputBox("修改名称", "", lblPresetName->Caption);

    D1608Cmd cmd;
    cmd.type = 1;
    cmd.id = offsetof(GlobalConfig, preset_name[cur_preset_id]);
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
    cmd.type = 1;
    cmd.id = offsetof(GlobalConfig, d1616_name);
    strncpy(cmd.data.data_string, global_config.d1616_name, 16);
    cmd.length = 17;
    SendCmd(cmd);
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
    cmd.type = 1;
    cmd.id = offsetof(GlobalConfig, avaliable_preset);
    for (int i=0;i<PRESET_NUM;i++)
    {
        global_config.avaliable_preset[i] =  clbAvaliablePreset->Checked[i];
        cmd.data.data_string[i] = clbAvaliablePreset->Checked[i];
    }
    cmd.length = 8;
    SendCmd(cmd);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btnSetLockClick(TObject *Sender)
{
    D1608Cmd cmd;
    cmd.type = 1;

    cmd.id = offsetof(GlobalConfig, running_timer_limit);
    cmd.data.data_32 = edtRunningTimer->Enabled ? running_timer : 0;
    cmd.length = 4;
    SendCmd(cmd);

    cmd.id = offsetof(GlobalConfig, reboot_count_limit);
    cmd.data.data_32 = edtRebootCount->Enabled ? roboot_count : 0;
    cmd.length = 4;
    SendCmd(cmd);

    cmd.id = offsetof(GlobalConfig, password);
    strncpy(cmd.data.data_string, edtPassword->Text.c_str(), 16);
    cmd.length = 20;
    SendCmd(cmd);

    cmd.id = offsetof(GlobalConfig, password_of_key);
    strncpy(cmd.data.data_string, edtKeyPassword->Text.c_str(), 16);
    cmd.length = 20;
    SendCmd(cmd);

    cmd.id = offsetof(GlobalConfig, locked_string);
    if (edtLockedString->Enabled)
    {
        strncpy(cmd.data.data_string, edtLockedString->Text.c_str(), 16);
    }
    cmd.length = 20;
    SendCmd(cmd);
}
//---------------------------------------------------------------------------

void __fastcall TForm1::btnUnlockClick(TObject *Sender)
{
    D1608Cmd cmd;
    cmd.type = 1;
    cmd.id = offsetof(GlobalConfig, unlock_string);
    strncpy(cmd.data.data_string, edtUnlockPassword->Text.c_str(), 16);
    cmd.length = 20;
    SendCmd(cmd);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::edtKeyPasswordKeyDown(TObject *Sender, WORD &Key,
      TShiftState Shift)
{
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
    cmd.type = 1;
    cmd.id = offsetof(GlobalConfig, running_timer_limit);
    cmd.length = 68;
    SendCmd(cmd);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btnLeaveTheFactoryClick(TObject *Sender)
{
    // 出厂    
    D1608Cmd cmd;
    cmd.type = 1;
    cmd.id = offsetof(GlobalConfig, adjust_running_time);
    SendCmd(cmd);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::tbRatioChange(TObject *Sender)
{
    edtRatio->Text = tbRatio->Position/100.0;
    btnSetComp->Click();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::tbThresholdChange(TObject *Sender)
{
    edtThreshold->Text = tbThreshold->Position / 10.0;
    btnSetComp->Click();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::tbAttackChange(TObject *Sender)
{
    edtAttack->Text = tbAttack->Position / 10.0;
    btnSetComp->Click();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::tbReleaseChange(TObject *Sender)
{
    edtRelease->Text = tbRelease->Position / 10.0;
    btnSetComp->Click();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btnSetCompClick(TObject *Sender)
{
    D1608Cmd cmd;
    cmd.id = GetOffsetOfData(&config_map.op_code.comp);
    cmd.data.data_32_array[0] = edtRatio->Text.ToDouble()*100;
    cmd.data.data_32_array[1] = edtThreshold->Text.ToDouble()*10.0;
    cmd.data.data_32_array[2] = edtAttack->Text.ToDouble()*10;
    cmd.data.data_32_array[3] = edtRelease->Text.ToDouble()*10;
    cmd.length = 16;
    SendCmd(cmd);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::edtRatioKeyDown(TObject *Sender, WORD &Key,
      TShiftState Shift)
{
    if (Key == VK_RETURN)
    {
        btnSetComp->Click();
    }    
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

    edtCompReleaseTime->Enabled = !cbCompAutoTime->Checked;
    edtCompAttackTime->Enabled = !cbCompAutoTime->Checked;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::edtCompRatioExit(TObject *Sender)
{
    TEdit * edt = (TEdit*)Sender;
    WORD Key = VK_RETURN;
    edt->OnKeyDown(Sender, Key, TShiftState());
    edt->OnClick = edtCompRatioClick; 
}
//---------------------------------------------------------------------------
void __fastcall TForm1::edtCompRatioClick(TObject *Sender)
{
    TEdit * edt = (TEdit*)Sender;
    edt->SelectAll();
    edt->OnClick = NULL;
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
        pnlComp1->Show();
        pnlComp1->BringToFront();
        break;
    case 4:
        pnlMist->Show();
        pnlMist->BringToFront();
        break;
    case 5:
        pnlSearch->Show();
        pnlSearch->BringToFront();
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

    // 纵坐标范围设定为+-3V
    Chart1->LeftAxis->SetMinMax(line_value-3, line_value+3);


    for (int i=0;i<100;i++)
    {
        //lineUpLimit->AddXY(i, line_value*1.5, "e", clRed);
        //lineDownLimit->AddXY(i,   line_value*0.5, "b", clRed);
        lineUpLimit->AddXY(i,   line_value+1.5, "e", clRed);
        lineDownLimit->AddXY(i, line_value-1.5, "b", clRed);
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
void __fastcall TForm1::dsp_gain_editExit(TObject *Sender)
{
    dsp_gain_trackbar->OnChange(dsp_gain_trackbar);
    dsp_gain_edit->OnClick = input_panel_level_editClick;
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
    }
    else if (Key == VK_ESCAPE)
    {
        dsp_gain_trackbar->OnChange(dsp_gain_trackbar);
        dsp_gain_edit->OnClick = input_panel_level_editClick;
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
void __fastcall TForm1::dsp_delay_editExit(TObject *Sender)
{
    dsp_delay_trackbar->OnChange(dsp_delay_trackbar);
    dsp_delay_edit->OnClick = input_panel_level_editClick;
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
    }
    else if (Key == VK_ESCAPE)
    {
        dsp_delay_trackbar->OnChange(dsp_delay_trackbar);
        dsp_delay_edit->OnClick = input_panel_level_editClick;
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
void __fastcall TForm1::CSpinEdit1Change(TObject *Sender)
{
    REAL_INPUT_DSP_NUM = CSpinEdit1->Value;
    SetIOChannelNum();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::CSpinEdit2Change(TObject *Sender)
{
    REAL_OUTPUT_DSP_NUM = CSpinEdit2->Value;
    SetIOChannelNum();
}
//---------------------------------------------------------------------------
void TForm1::CloseDspDetail()
{
    pnlDspDetail->Hide();
    pnlMix->Hide();
    if (last_out_num_btn != NULL)
    {
        last_out_num_btn->Down =false;
        last_out_num_btn == NULL;
    }

    if (last_dsp_btn != NULL)
    {
        last_dsp_btn->Down =false;
        last_dsp_btn == NULL;
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
}
//---------------------------------------------------------------------------
void __fastcall TForm1::cbPresetAutoSavedClick(TObject *Sender)
{
    D1608Cmd cmd;
    cmd.type = 1;
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
        cmd.type = 0;
        cmd.id = offsetof(ConfigMap, op_code.reboot);
        cmd.data.data_32 = 1;
        cmd.length = 4;
        SendCmd(cmd);
    }
}
//---------------------------------------------------------------------------

