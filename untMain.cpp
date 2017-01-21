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
#pragma resource "*.dfm"
#pragma comment(lib, "gdiplus.lib")

#define PANEL_WIDTH 48

TForm1 *Form1;
ConfigMap all_config_map[8];
ConfigMap config_map;
GlobalConfig global_config = {0};
//TAdvGDIPPicture * x = new TAdvGDIPPicture();

static bool on_loading = false;

//static char head[] = "\x53\x65\x74\x50\x61\x72\x61\x00\x00\x00\x4D\x41\x54\x31\x36\x31\x30\x43\x6F\x6E\x66\x69\x67\x44\x61\x74\x61\x00\x00\x00";

unsigned int GetOffsetOfData(void * p_data)
{
    static char * p_config_map = (char*)&config_map;
    return (char*)p_data - p_config_map;
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
    for (int i=0;i<16;i++)
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
    CopyInputPanelButton(form->input_panel_dsp_btn, panel_id)->Caption = "DSP " + String(char('A'-1+panel_id));
    form->input_eq_btn[panel_id-1] = CopyInputPanelButton(form->input_panel_eq_btn, panel_id);
    form->input_comp_btn[panel_id-1] = CopyInputPanelButton(form->input_panel_comp_btn, panel_id);
    form->input_auto_btn[panel_id-1] = CopyInputPanelButton(form->input_panel_auto_btn, panel_id);
    form->input_default_btn[panel_id-1] = CopyInputPanelButton(form->input_panel_default_btn, panel_id);
    form->input_invert_btn[panel_id-1] = CopyInputPanelButton(form->input_panel_invert_btn, panel_id);
    form->input_noise_btn[panel_id-1] = CopyInputPanelButton(form->input_panel_noise_btn, panel_id);
    form->input_mute_btn[panel_id-1] = CopyInputPanelButton(form->input_panel_mute_btn, panel_id);
    form->input_level_edit[panel_id-1] = CopyInputPanelEdit(form->input_panel_level_edit, panel_id);
    form->input_level_trackbar[panel_id-1] = CopyInputPanelTrackbar(form->input_panel_trackbar, panel_id);
    form->input_dsp_name[panel_id-1] = CopyInputPanelLabel(form->input_panel_dsp_num, panel_id);
        form->input_dsp_name[panel_id-1]->Caption = String(char('A'-1+panel_id));
}
static void CreateOutputPanel(int panel_id, TForm1 * form)
{
    TSpeedButton * btn_output_dsp = CopyInputPanelButton(form->output_panel_dsp_btn, panel_id);
        btn_output_dsp->Caption = "DSP" + IntToStr(panel_id);
        btn_output_dsp->Tag = 100 + panel_id;
    form->output_eq_btn[panel_id-1] = CopyInputPanelButton(form->output_panel_eq_btn, panel_id);
    form->output_comp_btn[panel_id-1] = CopyInputPanelButton(form->output_panel_comp_btn, panel_id);
    form->output_invert_btn[panel_id-1] = CopyInputPanelButton(form->output_panel_invert_btn, panel_id);
    form->output_mute_btn[panel_id-1] = CopyInputPanelButton(form->output_panel_mute_btn, panel_id);

    Graphics::TBitmap * bmp = new Graphics::TBitmap();
    form->ImageList1->GetBitmap(panel_id-1, bmp);
    CopyInputPanelButton(form->output_panel_number_btn, panel_id, bmp);

    form->output_level_edit[panel_id-1] = CopyInputPanelEdit(form->output_panel_level_edit, panel_id);
    form->output_level_trackbar[panel_id-1] = CopyInputPanelTrackbar(form->output_panel_trackbar, panel_id);
    form->output_dsp_name[panel_id-1] = CopyInputPanelLabel(form->output_panel_dsp_num, panel_id);
        form->output_dsp_name[panel_id-1]->Caption = IntToStr(panel_id);
}
static void CopyWatchPanel(int panel_id, TForm1 * form, String label, int left)
{
    TPanel * watch_panel = new TPanel(form->tsOperator);
    watch_panel->SetBounds(left, form->watch_panel->Top, form->watch_panel->Width, form->watch_panel->Height);
    watch_panel->BevelOuter = form->watch_panel->BevelOuter;
    watch_panel->Parent = form->tsOperator;
    watch_panel->Color = form->watch_panel->Color;

    TImage * bk_image = new TImage(watch_panel);
    bk_image->Picture = form->watch_bkimage->Picture;
    bk_image->BoundsRect = form->watch_bkimage->BoundsRect;
    bk_image->Parent = watch_panel;

    TPaintBox * pb_level = new TPaintBox(watch_panel);
    pb_level->BoundsRect = form->pb_watch->BoundsRect;
    pb_level->OnPaint = form->pb_watch->OnPaint;
    pb_level->Parent = watch_panel;
    pb_level->Tag = form->pb_watch->Tag;
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
    on_loading = true;

    pb_watch_list[0] = pb_watch;

    // 生成input背景
    input_panel_bkground->Picture->Bitmap->Width = 16 * PANEL_WIDTH;
    for (int i=0;i<16;i++)
    {
        TRect templet_image_rect = Image3->BoundsRect;
        TRect dest_rect = TRect(i*PANEL_WIDTH,
                                templet_image_rect.Top,
                                (i+1)*PANEL_WIDTH,
                                templet_image_rect.Bottom);
        input_panel_bkground->Canvas->CopyRect(dest_rect, input_panel_bkground->Canvas, templet_image_rect);
    }
    // 生成InputPanel
    input_type_lbl[0] = input_type;
    input_eq_btn[0] = input_panel_eq_btn;
    input_comp_btn[0] = input_panel_comp_btn;
    input_auto_btn[0] = input_panel_auto_btn;
    input_default_btn[0] = input_panel_default_btn;
    input_invert_btn[0] = input_panel_invert_btn;
    input_noise_btn[0] = input_panel_noise_btn;
    input_mute_btn[0] = input_panel_mute_btn;
    input_dsp_name[0] = input_panel_dsp_num;
    for (int i=2;i<=16;i++)
    {
        CreateInputPanel(i, this);
        CopyWatchPanel(i, this, String((char)('A'-1+i))+" ("+IntToStr(i)+")", (i-1) * PANEL_WIDTH);
    }

    //------------------------------------
    output_panel_bkground->Width = OUTPUT_DSP_NUM * PANEL_WIDTH;
    output_panel_bkground->Picture->Bitmap->Width = OUTPUT_DSP_NUM * PANEL_WIDTH;
    for (int i=1;i<OUTPUT_DSP_NUM;i++)
    {
        TRect templet_image_rect = Image3->BoundsRect;
        TRect dest_rect = TRect(i*PANEL_WIDTH,
                                templet_image_rect.Top,
                                (i+1)*PANEL_WIDTH,
                                templet_image_rect.Bottom);
        output_panel_bkground->Canvas->CopyRect(dest_rect, output_panel_bkground->Canvas, templet_image_rect);
    }

    mix_panel_trackbar->Thumb->Picture = input_panel_trackbar->Thumb->Picture;
    mix_panel_trackbar->Thumb->PictureHot = input_panel_trackbar->Thumb->PictureHot;
    mix_panel_trackbar->Thumb->PictureDown = input_panel_trackbar->Thumb->PictureDown;

    master_panel_trackbar->Thumb->Picture = input_panel_trackbar->Thumb->Picture;
    master_panel_trackbar->Thumb->PictureHot = input_panel_trackbar->Thumb->PictureHot;
    master_panel_trackbar->Thumb->PictureDown = input_panel_trackbar->Thumb->PictureDown;

    output_panel_trackbar->Thumb->PictureDown = input_panel_trackbar->Thumb->PictureDown;
    output_panel_trackbar->Thumb->PictureHot = input_panel_trackbar->Thumb->PictureHot;

    output_eq_btn[0] = output_panel_eq_btn;
    output_comp_btn[0] = output_panel_comp_btn;
    output_invert_btn[0] = output_panel_invert_btn;
    output_mute_btn[0] = output_panel_mute_btn;
    output_dsp_name[0] = output_panel_dsp_num;
    for (int i=2;i<=OUTPUT_DSP_NUM;i++)
    {
        CreateOutputPanel(i, this);
    }

    //----------------------------------
    // 生成pnlmix背景
    pnlmix_background->Picture->Bitmap->Width = 17 * PANEL_WIDTH;
    for (int i=1;i<17;i++)
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
    for (int i=2;i<=17;i++)
    {
        CreatePnlMix(i, this);
    }
    mix_level_edit[0] = pnlmix_level_edit;
    mix_level_trackbar[0] = pnlmix_level_trackbar;
    SetWindowLong(pnlmix_level_edit->Handle, GWL_STYLE, GetWindowLong(pnlmix_level_edit->Handle, GWL_STYLE) | ES_RIGHT);
    pnlmix_level_trackbar->OnChange(pnlmix_level_trackbar);

    //----------------------------------
    for (int i=17;i<=17+15;i++)
    {
        CopyWatchPanel(i, this, String(1+(i-17)), mix_panel->Left + mix_panel->Width + (i-17) * PANEL_WIDTH);
    }
    input_level_edit[0] = input_panel_level_edit;
    input_level_trackbar[0] = input_panel_trackbar;
    SetWindowLong(input_panel_level_edit->Handle, GWL_STYLE, GetWindowLong(input_panel_level_edit->Handle, GWL_STYLE) | ES_RIGHT);
    input_panel_trackbar->OnChange(input_panel_trackbar);

    output_level_edit[0] = output_panel_level_edit;
    output_level_trackbar[0] = output_panel_trackbar;
    SetWindowLong(output_panel_level_edit->Handle, GWL_STYLE, GetWindowLong(output_panel_level_edit->Handle, GWL_STYLE) | ES_RIGHT);
    output_panel_trackbar->OnChange(output_panel_trackbar);

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
    paint_agent = new PaintAgent(PaintBox1, filter_set);
    filter_set.Register(paint_agent, panel_agent);

    pnlDspDetail->DoubleBuffered = true;

    // TODO: 受到滤波器数量影响
    panel_agent->SetPanel(0, panelBand0, edtFreq0, edtQ0, edtGain0, cbType0, cbBypass0);
    panel_agent->SetPanel(1, panelBand1, edtFreq1, edtQ1, edtGain1, cbType1, cbBypass1);
    panel_agent->SetPanel(2, panelBand2, edtFreq2, edtQ2, edtGain2, cbType2, cbBypass2);
    panel_agent->SetPanel(3, panelBand3, edtFreq3, edtQ3, edtGain3, cbType3, cbBypass3);
    panel_agent->SetPanel(4, panelBand4, edtFreq4, edtQ4, edtGain4, cbType4, cbBypass4);
    panel_agent->SetPanel(5, panelBand5, edtFreq5, edtQ5, edtGain5, cbType5, cbBypass5);
    panel_agent->SetPanel(6, panelBand6, edtFreq6, edtQ6, edtGain6, cbType6, cbBypass6);
    //panel_agent->SetPanel(7, panelBand7, edtFreq7, edtQ7, edtGain7, cbType7, cbBypass7);
    //panel_agent->SetPanel(8, panelBand8, edtFreq8, edtQ8, edtGain8, cbType8, cbBypass8);
    panel_agent->SetPanel(10, panelBand9, edtFreq9, edtQ9, edtGain9, cbType9, cbBypass9);
    panel_agent->SetPanel(11, panelBand10, edtFreq10, edtQ10, edtGain10, cbType10, cbBypass10);

    InitConfigMap();
    btnDspResetEQ->Click();

    SetPresetId(1);
    for (int i=0;i<8;i++)
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
void __fastcall TForm1::FormCreate(TObject *Sender)
{
    // 输出结构体大小
    memo_debug->Lines->Add("InputConfigMap:" + IntToStr(sizeof(InputConfigMap)));
    memo_debug->Lines->Add("OutputConfigMap:" + IntToStr(sizeof(OutputConfigMap)));
    memo_debug->Lines->Add("MasterConfigMap:" + IntToStr(sizeof(MasterConfigMap)));
    memo_debug->Lines->Add("ConfigMap:" + IntToStr(sizeof(ConfigMap)));
    memo_debug->Lines->Add("mix_mute:" + IntToStr(sizeof(config_map.mix_mute)));

    // 根据数量初始化控制器
    // Panel->Tag
    // Panel->Left
    // Label->Caption

    mmLog->Text = Now();
                                     
    if (FileExists("iap.log"))
    {                                                             
        log_file = new TFileStream("iap.log", fmOpenWrite);
        log_file->Seek(0, soFromEnd);
    }
    else
    {
        log_file = new TFileStream("iap.log", fmCreate);
    }                       

    btnRefresh->Click();

    //tsSearch->Show();
    this->Repaint();

    pnlMix->BringToFront();
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
#if 1
    unsigned __int8 * p = (unsigned __int8*)&cmd;
    edtDebug->Text = "";
    String cmd_text = "";
    for (unsigned int i=30;i<30+16+cmd.length;i++)
    {
        cmd_text = cmd_text + IntToHex(p[i], 2) + " ";
    }
    edtDebug->Text = cmd_text;
#endif
    if (on_loading || !udpControl->Active)
    {
    }
    else
    {
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
    String device_name = slp_pack.type;

    TListItem * item = NULL;
    // 查找是否列表中已经存在
    for (int i=0;i<lvDevice->Items->Count;i++)
    {
        item = lvDevice->Items->Item[i];
        String ip = item->SubItems->Strings[0];
        if (ip == ip_address)
        {
            // 更新属性
            item->Data = (void*)2;
            item->SubItems->Strings[1] = ABinding->IP;
            item->SubItems->Strings[2] = device_name;
            break;
        }
    }

    if (item == NULL)
    {
        item = lvDevice->Items->Add();
        item->Data = (void*)2;
        item->Caption = IntToStr(item->Index + 1);
        item->SubItems->Add(ip_address);
        item->SubItems->Add(ABinding->IP);
        item->SubItems->Add(device_name);
    }

    if (last_select_device_ip == ip_address)
    {
        item->Selected = true;
    }

    if (lvDevice->Selected == NULL)
    {
        item->Selected = true;
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

    dst_ip = Item->SubItems->Strings[0];
    AppendLog("选择："+IntToStr(Item->Index+1)+" "+dst_ip);

    flag_ip_selected = (dst_ip!="");
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
    preset_cmd.preset = 1; // 读取preset
    // 从0页读取
    preset_cmd.store_page = 0;
    udpControl->SendBuffer(dst_ip, 905, &preset_cmd, sizeof(preset_cmd));

    preset_cmd.preset = 0; // 读取global_config
    udpControl->SendBuffer(dst_ip, 905, &preset_cmd, sizeof(preset_cmd));

    tsOperator->Show();
    cbWatch->Down = true;

    keep_live_count = 0;
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
        udpSLP->Active = true;
        char search_flag[] = "rep";
        udpSLP->SendBuffer("255.255.255.255", 888, search_flag, sizeof(search_flag));
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
void __fastcall TForm1::ToogleCOMP(TObject *Sender)
{
/*    TSpeedButton* btn = (TSpeedButton*)Sender;
    int dsp_id = btn->Tag;
    static int input_comp = 0;
    input_comp = SetBit(input_comp, dsp_id, btn->Down);
    D1608Cmd cmd = InputComp(input_comp);
    cmd.length = 1;
    SendCmd(cmd);*/
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
struct ADC_Data
{
    unsigned __int16 base;
    unsigned __int16 _12va;
    unsigned __int16 _5vd ;
    unsigned __int16 _8vd ;
    unsigned __int16 _16va;
    unsigned __int16 _48va;
    unsigned __int16 _5va;
    unsigned __int16 _x12va;
    unsigned __int16 _8va ;
    unsigned __int16 _x16va;
};
int CalcVot(unsigned __int16 true_data1, float ra, float rb)
{
    return true_data1 * (ra+rb) / ra / 10;
}
int CalcVot(unsigned __int16 true_data1, unsigned __int16 true_data2, float ra, float rb, float rc, float rd)
{
    return ((true_data2 * (rc+rd) / rc) - (true_data1 * (rd/rc) * (ra+rb) / ra))  / 10;
}
void __fastcall TForm1::udpControlUDPRead(TObject *Sender, TStream *AData,
      TIdSocketHandle *ABinding)
{
    if (ABinding->PeerPort == 2305)
    {
        D1608Cmd cmd;
        AData->ReadBuffer(&cmd, std::min(sizeof(cmd), AData->Size));

        //memo_debug->Lines->Add("Reply：" + CmdLog(cmd));

        if (cmd.id == GetOffsetOfData(&config_map.op_code.WatchLevel))
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
            __int16 data[10];

            // 计算检测到的电压 3.3v ~ 4096
            for (int i=1; i<10; i++)
            {
                data[i] = cmd.data.data_16_array[i] * 2500 / cmd.data.data_16_array[0];
            }
            data[0] = 2500;

            for (int i=0; i<10; i++)
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
            calc_data.base   = 2500;
            calc_data._12va  = CalcVot(true_data->_12va, 4.75, 14.7);
            calc_data._5vd   = CalcVot(true_data-> _5vd, 4.75,  3.32);
            calc_data._8vd   = CalcVot(true_data-> _8vd, 4.75, 10);
            calc_data._16va  = CalcVot(true_data->_16va, 4.75, 20);
            calc_data._48va  = CalcVot(true_data->_48va, 4.75, 75);
            calc_data._5va   = CalcVot(true_data-> _5va, 4.75,  3);
            calc_data._x12va = CalcVot(true_data->_12va, true_data->_x12va, 4.75, 14.7, 10, 14.7);
            calc_data._8va   = CalcVot(true_data-> _8va, 4.75, 10);
            calc_data._x16va = CalcVot(true_data->_16va, true_data->_x16va, 4.75, 20, 14.7, 20);

            __int16 * xcalc_data = (__int16 *)&calc_data;
            for (int i=0; i<10; i++)
            {
                double vot = xcalc_data[i] / 100.0f;
                ValueListEditor2->Cells[1][i+1] = FloatToStr(vot);
            }

            lblDiff->Caption = calc_data._8vd-calc_data._8va;
        }
        else if (cmd.id == GetOffsetOfData(&config_map.op_code.noop))
        {
            keep_live_count = 0;
            tsOperator->Caption = "操作(连接)";
        }
        else
        {
            memo_debug->Lines->Add("Reply：" + CmdLog(cmd));

            if (cmd.id == GetOffsetOfData(&config_map.master.level_a))
            {
                master_panel_trackbar->OnChange = NULL;
                master_panel_trackbar->Position = cmd.data.data_32;
                master_panel_trackbar->OnChange = MasterVolumeChange;

                int value = cmd.data.data_32;
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

                config_map.master.level_a = value;
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
        struct
        {
            int address;
            Event event[128];
        }buff;
        AData->ReadBuffer(&buff, sizeof(buff));
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
        if (buff.address < 0x8000000+128*1024+10*5*2*1024+(10* 2*1024))
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
                memcpy(&config_map.master, preset_cmd.data,
                        sizeof(config_map.master)+sizeof(config_map.mix)+sizeof(config_map.mix_mute));
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


            if (preset_cmd.store_page < 8)
            {
                // next
                udpControl->SendBuffer(dst_ip, 905, &preset_cmd, sizeof(preset_cmd));
            }
            else
            {
                SetPresetId(preset_id);
                ApplyConfigToUI();  // 子窗体的数据在加载时更新
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
                memcpy(preset_cmd.data, &config_map.master,
                        sizeof(config_map.master)+sizeof(config_map.mix)+sizeof(config_map.mix_mute));
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
            pb_watch_list[i]->Tag = -71;
            pb_watch_list[i]->Invalidate();
        }
        else
        {
            try{
                double valuex = log10(value);
                double base = log10(0x00FFFFFF);
                pb_watch_list[i]->Tag = (valuex - base) * 20 + 1 + 24;
                pb_watch_list[i]->Invalidate();
            }
            catch(...)
            {
                pb_watch_list[i]->Tag = 0;
                pb_watch_list[i]->Invalidate();
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
            pb_watch_list[i]->Tag = -49;
            pb_watch_list[i]->Invalidate();
        }
    }

    // keep alive
    if ((keep_live_count < 10) && udpControl->Active)
    {
        keep_live_count++;
    
        D1608Cmd cmd;
        cmd.id = GetOffsetOfData(&config_map.op_code.noop);
        SendCmd(cmd);
    }
    else
    {
        // TODO: 断链
        udpControl->Active = false;
        tsOperator->Caption = "操作(断开)";
        // Level Meter归零
        for (int i=0;i<32;i++)
        {
            pb_watch_list[i]->Tag = -49;
            pb_watch_list[i]->Invalidate();
        }
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

        for (int i=0;i<17;i++)
        {
            TAdvTrackBar* trackbar = mix_level_trackbar[i];
            if (trackbar != NULL)
            {
                trackbar->Thumb->Picture = MixPicture[btn->Tag - 1];
                trackbar->Thumb->PictureDown = MixPicture[btn->Tag - 1];
                trackbar->Thumb->PictureHot = MixPicture[btn->Tag - 1];
            }
        }

        pnlMix->Left = Width - 30 - pnlMix->Width;

        pnlMix->Top = 312;
        pnlMix->Show();
        pnlMix->Tag = btn->Tag;

        // 数据
        int out_dsp_num = last_out_num_btn->Tag;

        for (int i=0;i<=16;i++)
        {
            mix_mute_btn[i]->Down = config_map.mix_mute[i][out_dsp_num-1];
            mix_level_trackbar[i]->Position = config_map.mix[i][out_dsp_num-1];
        }
    }
    else
    {
        last_out_num_btn = NULL;
        pnlMix->Hide();
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ToogleLimit(TObject *Sender)
{
/*    TSpeedButton* btn = (TSpeedButton*)Sender;
    int dsp_id = btn->Tag;
    static int output_limit = 0;
    output_limit = SetBit(output_limit, dsp_id, btn->Down);
    D1608Cmd cmd = OutputLimit(output_limit);
    cmd.length = 1;
    SendCmd(cmd);*/
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
        cbAutoRefresh->Checked = false;
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
            TrackBar27->BackGround = p_input_inner_level->BackGround;
            TrackBar27->Max = p_input_inner_level->Max;
            TrackBar27->Min = p_input_inner_level->Min;
            lblDSPInfo->Caption = "Input Channel " + IntToStr(btn->Tag) + " DSP Setup";
            pnlDspDetail->Left = 0;

            int dsp_num = btn->Tag;
            TrackBar27->Position = config_map.input_dsp[dsp_num-1].level_b;
            btnPhanton->Down = config_map.input_dsp[dsp_num-1].phantom_switch;
        }
        else
        {
            TrackBar27->BackGround = p_output_inner_level->BackGround;
            TrackBar27->Max = p_output_inner_level->Max;
            TrackBar27->Min = p_output_inner_level->Min;
            lblDSPInfo->Caption = "Output Channel " + IntToStr(btn->Tag-100) + " DSP Setup";
            pnlDspDetail->Left = Width - pnlDspDetail->Width;

            int dsp_num = btn->Tag-100;
            TrackBar27->Position = config_map.output_dsp[dsp_num-1].level_b;
        }

        pnlDspDetail->Top = 192;
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
    cmd.id = GetOffsetOfData(&config_map.master.level_a);
    cmd.data.data_32 = value;
    cmd.length = 4;
    SendCmd(cmd);

    config_map.master.level_a = value;
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
    cmd.id = GetOffsetOfData(&config_map.master.mute_switch);
    cmd.data.data_8 = btn->Down;
    cmd.length = 1;
    SendCmd(cmd);

    config_map.master.mute_switch = btn->Down;
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
void __fastcall TForm1::TrackBar27Change(TObject *Sender)
{
    TAdvTrackBar* track = (TAdvTrackBar*)Sender;
    int value = track->Position;
    int dsp_num = track->Parent->Tag;

    if (dsp_num < 100)
    {
        // input channel
        D1608Cmd cmd;
        cmd.id = GetOffsetOfData(&config_map.input_dsp[dsp_num-1].level_b);
        cmd.data.data_16 = value;
        cmd.length = 2;
        SendCmd(cmd);

        config_map.input_dsp[dsp_num-1].level_b = value;
    }
    else
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
        edt->OnKeyDown(edt, key, shift);
        Handled = true;
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
    int level = pb_watch->Tag;

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

    r.top = 24-level+1;
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
}
//---------------------------------------------------------------------------
void __fastcall TForm1::input_panel_dsp_numClick(TObject *Sender)
{
    TLabel * label = (TLabel*)Sender;
    label->Caption = InputBox("修改名称", "", label->Caption);

    D1608Cmd cmd;
    cmd.type = tbGlobalDspName->Down;
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
    TLabel * label = (TLabel*)Sender;
    label->Caption = InputBox("修改名称", "", label->Caption);

    D1608Cmd cmd;
    cmd.type = tbGlobalDspName->Down;
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
        cmd.id = GetOffsetOfData(&config_map.mix[in_dsp_num-1][out_dsp_num-1]);
        cmd.data.data_16 = value;
        cmd.length = 2;
        SendCmd(cmd);

        config_map.mix[in_dsp_num-1][out_dsp_num-1] = value;
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

    if (last_out_num_btn != NULL && mix_level_trackbar[dsp_num] != NULL)
    {
        int in_dsp_num = dsp_num;
        int out_dsp_num = last_out_num_btn->Tag;

        D1608Cmd cmd;
        cmd.id = GetOffsetOfData(&config_map.mix_mute[in_dsp_num-1][out_dsp_num-1]);
        cmd.data.data_8 = btn->Down;
        cmd.length = 1;
        SendCmd(cmd);

        config_map.mix_mute[in_dsp_num-1][out_dsp_num-1] = btn->Down;
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

        // Download To Device
        D1608PresetCmd preset_cmd;
        preset_cmd.preset = cur_preset_id;
        preset_cmd.store_page = 0;
        memcpy(preset_cmd.data, &config_map.input_dsp[0], sizeof(config_map.input_dsp[0])*4);

        udpControl->SendBuffer(dst_ip, 907, &preset_cmd, sizeof(preset_cmd));
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

    edtPresetName->Text = global_config.preset_name[cur_preset_id];

    for (int i=0;i<8;i++)
        clbAvaliablePreset->Checked[i] = global_config.avaliable_preset[i];

    // 修改界面
    for (int i=0;i<16;i++)
    {
        input_type_lbl[i]->Caption = InputGain2String(config_map.input_dsp[i].gain);

        input_eq_btn[i]->Down = config_map.input_dsp[i].eq_switch;
        input_comp_btn[i]->Down = config_map.input_dsp[i].comp_switch;
        input_auto_btn[i]->Down = config_map.input_dsp[i].auto_switch;
        //input_default_btn[i]->Down = config_map.input_dsp[i].;
        input_invert_btn[i]->Down = config_map.input_dsp[i].invert_switch;
        input_noise_btn[i]->Down = config_map.input_dsp[i].noise_switch;
        input_mute_btn[i]->Down = config_map.input_dsp[i].mute_switch;
        input_level_trackbar[i]->Position = config_map.input_dsp[i].level_a;

        const char *dsp_name;
        if (tbGlobalDspName->Down)
            dsp_name = global_config.input_dsp_name[i];
        else
            dsp_name = config_map.input_dsp[i].dsp_name;

        if (dsp_name[0] == '\0')
            input_dsp_name[i]->Caption = String(char('A'+i));
        else
            input_dsp_name[i]->Caption = dsp_name;
    }

    for (int i=0;i<16;i++)
    {
        output_type_lbl[i]->Caption = OutputGain2String(config_map.output_dsp[i].gain);

        output_eq_btn[i]->Down = config_map.output_dsp[i].eq_switch;
        output_comp_btn[i]->Down = config_map.output_dsp[i].comp_switch;
        output_invert_btn[i]->Down = config_map.output_dsp[i].invert_switch;
        output_mute_btn[i]->Down = config_map.output_dsp[i].mute_switch;
        output_level_trackbar[i]->Position = config_map.output_dsp[i].level_a;

        const char *dsp_name;
        if (tbGlobalDspName->Down)
            dsp_name = global_config.output_dsp_name[i];
        else
            dsp_name = config_map.output_dsp[i].dsp_name;

        if (dsp_name[0] == '\0')
            output_dsp_name[i]->Caption = IntToStr(i+1);
        else
            output_dsp_name[i]->Caption = dsp_name;
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
    if (global_config.d1616_name[0] == '\0')
        Caption = "D1608";
    else
        Caption = global_config.d1616_name;
    Caption = Caption + " " + preset_lib_filename;
    if (file_dirty)
    {
        Caption = Caption + "*";
    }

    if (udpControl->Active)
    {
        Caption = Caption + " - " + dst_ip;
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

    D1608Cmd cmd;
    cmd.id = GetOffsetOfData(&config_map.op_code.switch_preset);
    cmd.data.data_32 = menu->Tag;
    SendCmd(cmd);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ToolButton1Click(TObject *Sender)
{
    //
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ToolButton2Click(TObject *Sender)
{
    //
}
//---------------------------------------------------------------------------

void __fastcall TForm1::btnSetIpClick(TObject *Sender)
{
    D1608Cmd cmd;
    cmd.type = 1;
    cmd.id = offsetof(GlobalConfig, static_ip_address);
    cmd.data.data_32 = inet_addr(edtIp->Text.c_str());
    cmd.length = 4;
    SendCmd(cmd);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::tbGlobalDspNameClick(TObject *Sender)
{
    ApplyConfigToUI();
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
void __fastcall TForm1::btnModifyPresetNameClick(TObject *Sender)
{
    edtPresetName->Text = InputBox("修改名称", "", edtPresetName->Text);

    D1608Cmd cmd;
    cmd.type = 1;
    cmd.id = offsetof(GlobalConfig, preset_name[cur_preset_id]);
    strncpy(cmd.data.data_string, edtPresetName->Text.c_str(), 16);
    cmd.length = 17;
    SendCmd(cmd);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btnDeviceNameClick(TObject *Sender)
{
    String d1616name = InputBox("修改名称", "", global_config.d1616_name);
    strncpy(global_config.d1616_name, d1616name.c_str(), 16);

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
        for (int i=0;i<8;i++)
        {
            clbAvaliablePreset->Checked[i] = global_config.avaliable_preset[i];
        }
        return;
    }

    D1608Cmd cmd;
    cmd.type = 1;
    cmd.id = offsetof(GlobalConfig, avaliable_preset);
    for (int i=0;i<8;i++)
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
void __fastcall TForm1::btnDownloadPresetClick(TObject *Sender)
{
    // 下载某个preset在flash里的值
}
//---------------------------------------------------------------------------

