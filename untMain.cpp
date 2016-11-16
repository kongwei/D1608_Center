//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "untMain.h"
#include <winsock2.h>
#include <algorithm>
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
TAdvGDIPPicture * x = new TAdvGDIPPicture();

static bool on_loading = false;

//static char head[] = "\x53\x65\x74\x50\x61\x72\x61\x00\x00\x00\x4D\x41\x54\x31\x36\x31\x30\x43\x6F\x6E\x66\x69\x67\x44\x61\x74\x61\x00\x00\x00";

unsigned int GerOffsetOfData(void * p_data)
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
    result = result + IntToStr(cmd.data.value[0]) + ",";
    result = result + IntToStr(cmd.data.value[1]) + ",";
    result = result + IntToStr(cmd.data.value[2]) + ",";
    result = result + IntToStr(cmd.data.value[3]) + ",";
    result = result + IntToStr(cmd.data.value[4]);

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
    CopyInputPanelLabel(form->input_panel_dsp_num, panel_id)->Caption = String(char('A'-1+panel_id));
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
    CopyInputPanelLabel(form->output_panel_dsp_num, panel_id)->Caption = IntToStr(panel_id);
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
    input_eq_btn[0] = input_panel_eq_btn;
    input_comp_btn[0] = input_panel_comp_btn;
    input_auto_btn[0] = input_panel_auto_btn;
    input_default_btn[0] = input_panel_default_btn;
    input_invert_btn[0] = input_panel_invert_btn;
    input_noise_btn[0] = input_panel_noise_btn;
    input_mute_btn[0] = input_panel_mute_btn;
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
    panel_agent->SetPanel(7, panelBand7, edtFreq7, edtQ7, edtGain7, cbType7, cbBypass7);
    panel_agent->SetPanel(8, panelBand8, edtFreq8, edtQ8, edtGain8, cbType8, cbBypass8);
    //panel_agent->SetPanel(10, panelBand9, edtFreq9, edtQ9, edtGain9, cbType9, cbBypass9);
    panel_agent->SetPanel(9, panelBand10, edtFreq10, edtQ10, edtGain10, cbType10, cbBypass10);

    btnDspResetEQ->Click();

    SetPresetId(0);
    InitConfigMap();
    for (int i=0;i<8;i++)
    {
        all_config_map[i] = config_map;
    }

    file_dirty = false;

    InitGdipus();

    pnlDspDetail->BringToFront();

    mireg0 = 0;

    on_loading = false;
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
    cmd.preset = cbPreset->ItemIndex + 1;
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
        //memo_debug->Lines->Add("Send：" + CmdLog(cmd));
        udpControl->SendBuffer(dst_ip, 2305, &cmd, sizeof(cmd));
    }

    if (!on_loading)
    {
        if (cmd.id != GerOffsetOfData(&config_map.WatchLevel))
        {
            SetFileDirty(true);
        }
    }
}
//---------------------------------------------------------------------------
void TForm1::SendCICmd(CIDebugCmd& cmd)
{
    udpControl->SendBuffer(dst_ip, 1000, &cmd, sizeof(cmd));
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

    D1608PresetCmd cmd;
    cmd.preset = 0x80;
    cmd.type = 0;

    udpControl->SendBuffer(dst_ip, 905, &cmd, sizeof(cmd));

    tsOperator->Show();
    cbWatch->Down = true;
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
        cmd.id = GerOffsetOfData(&config_map.input_dsp[dsp_num-1].level_a);
        cmd.data.data_16 = value;
        cmd.length = 2;
        SendCmd(cmd);

        config_map.input_dsp[dsp_num-1].level_a = value;
    }
    else
    {
        //D1608Cmd cmd;
        //cmd.id = GerOffsetOfData(&config_map.mix_dsp.level_a);
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
    cmd.id = GerOffsetOfData(&config_map.input_dsp[dsp_id-1].mute_switch);
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
    cmd.id = GerOffsetOfData(&config_map.input_dsp[dsp_id-1].noise_switch);
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
    cmd.id = GerOffsetOfData(&config_map.input_dsp[dsp_id-1].invert_switch);
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
    cmd.id = GerOffsetOfData(&config_map.input_dsp[dsp_id-1].eq_switch);
    cmd.data.data_8 = btn->Down;
    cmd.length = 1;
    SendCmd(cmd);

    config_map.input_dsp[dsp_id-1].eq_switch = btn->Down;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::udpControlUDPRead(TObject *Sender, TStream *AData,
      TIdSocketHandle *ABinding)
{
    if (ABinding->PeerPort == 2305)
    {
        D1608Cmd cmd;
        AData->ReadBuffer(&cmd, std::min(sizeof(cmd), AData->Size));

        if (cmd.id == GerOffsetOfData(&config_map.WatchLevel))
        {
            // 音量输出
            MsgWatchHandle(cmd);
        }
    }
    else if (ABinding->PeerPort == 2306)
    {
        D1608Cmd cmd;
        AData->ReadBuffer(&cmd, std::min(sizeof(cmd), AData->Size));
        if (cmd.id != GerOffsetOfData(&config_map.WatchLevel))
        {
            memo_debug->Lines->Add("Reply：" + CmdLog(cmd));
        }
    }
    else if (ABinding->PeerPort == 904)
    {
    /*
        D1608PresetCmd cmd;
        cmd.preset = cbPresetId->ItemIndex;
        cmd.type = 0;

        // 如果没有打开文件，则需要从下位机同步
        if (preset_lib_filename == "")
        {
            udpControl->SendBuffer(dst_ip, 905, &cmd, sizeof(cmd));
        }
    */
    }
    else if (ABinding->PeerPort == 905)
    {
        D1608PresetCmd cmd;
        AData->ReadBuffer(&cmd, std::min(sizeof(cmd), AData->Size));

        switch(cmd.type)
        {
        case 0:
            memcpy(&config_map.input_dsp[0], cmd.data, sizeof(config_map.input_dsp[0])*4);
            break;
        case 1:
            memcpy(&config_map.input_dsp[4], cmd.data, sizeof(config_map.input_dsp[0])*4);
            break;
        case 2:
            memcpy(&config_map.input_dsp[8], cmd.data, sizeof(config_map.input_dsp[0])*4);
            break;
        case 3:
            memcpy(&config_map.input_dsp[12],cmd.data,  sizeof(config_map.input_dsp[0])*4);
            break;
        case 4:
            memcpy(&config_map.output_dsp[0],cmd.data,  sizeof(config_map.output_dsp[0])*4);
            break;
        case 5:
            memcpy(&config_map.output_dsp[4],cmd.data,  sizeof(config_map.output_dsp[0])*4);
            break;
        case 6:
            memcpy(&config_map.output_dsp[8],cmd.data,  sizeof(config_map.output_dsp[0])*4);
            break;
        case 7:
            memcpy(&config_map.output_dsp[12], cmd.data, sizeof(config_map.output_dsp[0])*4);
            break;
        case 8:
            memcpy(&config_map.master, cmd.data,
                    sizeof(config_map.master)+sizeof(config_map.mix)+sizeof(config_map.mix_mute));
            break;
        }

        bool download_all_preset = cmd.preset & 0x80;
        int preset_id = cmd.preset & 0x7F;

        if (download_all_preset && (preset_id<8) && cmd.type == 8)
        {
            cmd.preset++;
            cmd.type = 0;

            // 切换 config_map
            all_config_map[preset_id] = config_map;
        }
        else
        {
            cmd.type++;
        }


        if (cmd.type < 8)
        {
            // next
            udpControl->SendBuffer(dst_ip, 905, &cmd, sizeof(cmd));
        }
        else
        {
            on_loading = true;
            SetPresetId(preset_id);

            ApplyConfigToUI();  // 子窗体的数据在加载时更新
        }
    }
    else if (ABinding->PeerPort == 907)
    {
        D1608PresetCmd cmd;
        AData->ReadBuffer(&cmd, std::min(sizeof(cmd), AData->Size));

        bool download_all_preset = cmd.preset & 0x80;
        int preset_id = cmd.preset & 0x7F;

        if (download_all_preset && (preset_id<8) && cmd.type == 8)
        {
            cmd.preset++;
            cmd.type = 0;

            // 切换 config_map
            config_map = all_config_map[preset_id];
        }
        else
        {
            cmd.type++;
        }

        switch(cmd.type)
        {
        case 0:
            memcpy(cmd.data, &config_map.input_dsp[0], sizeof(config_map.input_dsp[0])*4);
            break;
        case 1:
            memcpy(cmd.data, &config_map.input_dsp[4], sizeof(config_map.input_dsp[0])*4);
            break;
        case 2:
            memcpy(cmd.data, &config_map.input_dsp[8], sizeof(config_map.input_dsp[0])*4);
            break;
        case 3:
            memcpy(cmd.data, &config_map.input_dsp[12], sizeof(config_map.input_dsp[0])*4);
            break;
        case 4:
            memcpy(cmd.data, &config_map.output_dsp[0], sizeof(config_map.output_dsp[0])*4);
            break;
        case 5:
            memcpy(cmd.data, &config_map.output_dsp[4], sizeof(config_map.output_dsp[0])*4);
            break;
        case 6:
            memcpy(cmd.data, &config_map.output_dsp[8], sizeof(config_map.output_dsp[0])*4);
            break;
        case 7:
            memcpy(cmd.data, &config_map.output_dsp[12], sizeof(config_map.output_dsp[0])*4);
            break;
        case 8:
            memcpy(cmd.data, &config_map.master,
                    sizeof(config_map.master)+sizeof(config_map.mix)+sizeof(config_map.mix_mute));
        default:
            break;
        }
        if (cmd.type <= 8)
        {
            udpControl->SendBuffer(dst_ip, 907, &cmd, sizeof(cmd));
        }
    }
}
//---------------------------------------------------------------------------
void TForm1::MsgWatchHandle(const D1608Cmd& cmd)
{
    for (int i=0;i<32;i++)
    {
        int value = cmd.data.value[i];
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
        cmd.id = GerOffsetOfData(&config_map.WatchLevel);
        SendCmd(cmd);
    }
    else
    {
        D1608Cmd cmd;
        cmd.id = GerOffsetOfData(&config_map.noop);
        if (udpControl->Active)
        {
            udpControl->SendBuffer(dst_ip, 2305, &cmd, sizeof(cmd));
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ToogleOutputMute(TObject *Sender)
{
    TSpeedButton* btn = (TSpeedButton*)Sender;
    int dsp_id = btn->Tag;

    D1608Cmd cmd;
    cmd.id = GerOffsetOfData(&config_map.output_dsp[dsp_id-1].mute_switch);
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
    cmd.id = GerOffsetOfData(&config_map.output_dsp[dsp_id-1].invert_switch);
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

        for (int i=0;i<16;i++)
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
    cmd.id = GerOffsetOfData(&config_map.output_dsp[dsp_id-1].eq_switch);
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
    cmd.id = GerOffsetOfData(&config_map.output_dsp[dsp_num-1].level_a);
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
    cmd.id = GerOffsetOfData(&config_map.mix_dsp.invert_switch);
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
    cmd.id = GerOffsetOfData(&config_map.master.level_a);
    cmd.data.data_16 = value;
    cmd.length = 2;
    SendCmd(cmd);

    config_map.master.level_a = value;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btnMixMuteClick(TObject *Sender)
{
#if 0
    TSpeedButton* btn = (TSpeedButton*)Sender;

    D1608Cmd cmd;
    cmd.id = GerOffsetOfData(&config_map.mix_dsp.mute_switch);
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
    cmd.id = GerOffsetOfData(&config_map.master.mute_switch);
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
    cmd.id = GerOffsetOfData(&config_map.input_dsp[dsp_id-1].phantom_switch);
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
        cmd.id = GerOffsetOfData(&config_map.input_dsp[dsp_num-1].level_b);
        cmd.data.data_16 = value;
        cmd.length = 2;
        SendCmd(cmd);

        config_map.input_dsp[dsp_num-1].level_b = value;
    }
    else
    {
        // output channel
        D1608Cmd cmd;
        cmd.id = GerOffsetOfData(&config_map.output_dsp[dsp_num-101].level_b);
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
    cmd.id = GerOffsetOfData(&config_map.input_dsp[dsp_num-1].gain);
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
    cmd.id = GerOffsetOfData(&config_map.output_dsp[dsp_num-1].gain);
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
    int preset_freq_list[10] = {20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000};
    filter_set.GetFilter(FIRST_FILTER+1)->ChangFilterParameter("Low Shelf", 50, 0, 4.09);
    for (int i=FIRST_FILTER+2; i<=LAST_FILTER-2; i++)
    {
        filter_set.GetFilter(i)->ChangFilterParameter("Parametric", preset_freq_list[i], 0, 4.09);
    }
    filter_set.GetFilter(LAST_FILTER-1)->ChangFilterParameter("High Shelf", 10000, 0, 4.09);

    for (int i=FIRST_FILTER+1;i<=LAST_FILTER-1;i++)
    {
        filter_set.GetFilter(i)->name = IntToStr(i-1);
        filter_set.SetBypass(i, false);
        filter_set.RepaintPaint(i);
    }

    filter_set.GetFilter(FIRST_FILTER)->ChangFilterParameter("12dB Butterworth High", 20, 0, 4.09);
    filter_set.SetBypass(FIRST_FILTER, true);
    filter_set.GetFilter(FIRST_FILTER)->name = "H";
    filter_set.RepaintPaint(FIRST_FILTER);

    filter_set.GetFilter(LAST_FILTER)->ChangFilterParameter("12dB Butterworth Low", 20000, 0, 4.09);
    filter_set.SetBypass(LAST_FILTER, true);
    filter_set.GetFilter(LAST_FILTER)->name = "L";
    filter_set.RepaintPaint(LAST_FILTER);
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
    // TODO: 存到下位机, 限制长度
}
//---------------------------------------------------------------------------
void __fastcall TForm1::output_panel_dsp_numClick(TObject *Sender)
{
    TLabel * label = (TLabel*)Sender;
    label->Caption = InputBox("修改名称", "", label->Caption);
    // TODO: 存到下位机, 限制长度
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
        cmd.id = GerOffsetOfData(&config_map.mix[in_dsp_num-1][out_dsp_num-1]);
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
        cmd.id = GerOffsetOfData(&config_map.mix_mute[in_dsp_num-1][out_dsp_num-1]);
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
        D1608PresetCmd cmd;
        cmd.preset = cur_preset_id;
        cmd.type = 0;
        memcpy(cmd.data, &config_map.input_dsp[0], sizeof(config_map.input_dsp[0])*4);

        udpControl->SendBuffer(dst_ip, 907, &cmd, sizeof(cmd));
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btnStoreClick(TObject *Sender)
{
    udpControl->SendBuffer(dst_ip, 906, "", 1);
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
        SetPresetId(0);

        ApplyConfigToUI();

        if (udpControl->Active)
        {
            // Download To Device
            D1608PresetCmd cmd;
            cmd.preset = 0x80;
            cmd.type = 0;
            memcpy(cmd.data, &config_map.input_dsp[0], sizeof(config_map.input_dsp[0])*4);

            udpControl->SendBuffer(dst_ip, 907, &cmd, sizeof(cmd));
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ApplyConfigToUI()
{
    on_loading = true;

    // 修改界面
    for (int i=0;i<16;i++)
    {
        input_eq_btn[i]->Down = config_map.input_dsp[i].eq_switch;
        input_comp_btn[i]->Down = config_map.input_dsp[i].comp_switch;
        input_auto_btn[i]->Down = config_map.input_dsp[i].auto_switch;
        //input_default_btn[i]->Down = config_map.input_dsp[i].;
        input_invert_btn[i]->Down = config_map.input_dsp[i].invert_switch;
        input_noise_btn[i]->Down = config_map.input_dsp[i].noise_switch;
        input_mute_btn[i]->Down = config_map.input_dsp[i].mute_switch;
        input_level_trackbar[i]->Position = config_map.input_dsp[i].level_a;
    }

    for (int i=0;i<16;i++)
    {
        output_eq_btn[i]->Down = config_map.output_dsp[i].eq_switch;
        output_comp_btn[i]->Down = config_map.output_dsp[i].comp_switch;
        output_invert_btn[i]->Down = config_map.output_dsp[i].invert_switch;
        output_mute_btn[i]->Down = config_map.output_dsp[i].mute_switch;
        output_level_trackbar[i]->Position = config_map.output_dsp[i].level_a;
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
    Caption = "D1608 ";
    Caption = Caption + preset_lib_filename;
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

    if (udpControl->Active)
    {
        D1608Cmd cmd;
        cmd.preset = menu->Tag;
        cmd.type = 0;

        udpControl->SendBuffer(dst_ip, 904, &cmd, sizeof(cmd));
    }
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

