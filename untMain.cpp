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
#pragma resource "*.dfm"
#pragma comment(lib, "gdiplus.lib")

#define PANEL_WIDTH 48

TForm1 *Form1;
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
//---------------------------------------------------------------------------
static TSpeedButton * CopyInputPanelButton(TSpeedButton * src_btn, TPanel * input_panel, Graphics::TBitmap* bmp=NULL)
{
    TSpeedButton * dsp_btn = new TSpeedButtonNoFrame(input_panel);
    dsp_btn->Caption = src_btn->Caption;
    dsp_btn->BoundsRect = src_btn->BoundsRect;
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
    dsp_btn->Parent = input_panel;

    return dsp_btn;
}
static TImage * CopyInputPanelBkground(TImage * src_img, TPanel * input_panel)
{
    TImage * bkground = new TImage(input_panel);
    bkground->Picture->Bitmap = src_img->Picture->Bitmap;
    bkground->BoundsRect = src_img->BoundsRect;
    bkground->Parent = input_panel;
    bkground->SendToBack();
    return bkground;
}
static TAdvTrackBar * CopyInputPanelTrackbar(TAdvTrackBar * src_trackbar, TPanel * input_panel)
{
    TAdvTrackBar * trackbar = new TAdvTrackBar(input_panel);
    trackbar->Parent = input_panel;

    trackbar->OnChange = src_trackbar->OnChange;

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

    trackbar->BoundsRect = src_trackbar->BoundsRect;

    trackbar->OnChange(trackbar);

    return trackbar;
}
static TEdit * CopyInputPanelEdit(TEdit * src_edit, TPanel * input_panel)
{
    TEdit * edit = new TEdit(input_panel);
    edit->BoundsRect = src_edit->BoundsRect;
    edit->BorderStyle = src_edit->BorderStyle;
    edit->Color = src_edit->Color;
    edit->Font = src_edit->Font;
    edit->Parent = input_panel;
    SetWindowLong(edit->Handle, GWL_STYLE, GetWindowLong(edit->Handle, GWL_STYLE) | ES_RIGHT);
    return edit;
}
static TStaticText * CopyInputPanelLabel(TStaticText * src_label, TPanel * input_panel)
{
    TStaticText * label = new TStaticText(input_panel);
    label->BoundsRect = src_label->BoundsRect;
    label->Color = src_label->Color;
    label->Font = src_label->Font;
    label->Parent = input_panel;
    return label;
}
static void CreateInputPanel(int panel_id, TForm1 * form)
{
    TPanel * input_panel = new TPanel(form->tsOperator);
    input_panel->SetBounds(form->input_panel->Left+(panel_id-1) * PANEL_WIDTH, form->input_panel->Top, form->input_panel->Width, form->input_panel->Height);
    input_panel->Parent = form->tsOperator;
    input_panel->Color = form->input_panel->Color;
    input_panel->BevelOuter = bvNone;
    input_panel->Tag = panel_id;
    CopyInputPanelButton(form->input_panel_dsp_btn, input_panel)->Caption = "DSP " + String(char('A'-1+panel_id));
    CopyInputPanelButton(form->input_panel_eq_btn, input_panel);
    CopyInputPanelButton(form->input_panel_comp_btn, input_panel);
    CopyInputPanelButton(form->input_panel_auto_btn, input_panel);
    CopyInputPanelButton(form->input_panel_default_btn, input_panel);
    CopyInputPanelButton(form->input_panel_invert_btn, input_panel);
    CopyInputPanelButton(form->input_panel_noise_btn, input_panel);
    CopyInputPanelButton(form->input_panel_mute_btn, input_panel);
    form->input_level_edit[panel_id-1] = CopyInputPanelEdit(form->input_panel_level_edit, input_panel);
    CopyInputPanelTrackbar(form->input_panel_trackbar, input_panel);
    CopyInputPanelLabel(form->input_panel_dsp_num, input_panel)->Caption = String(char('A'-1+panel_id));
    CopyInputPanelBkground(form->input_panel_bkground, input_panel);
}
static void CreateOutputPanel(int panel_id, TForm1 * form)
{
    TPanel * output_panel = new TPanel(form->tsOperator);
    output_panel->SetBounds(form->output_panel->Left+(panel_id-1) * PANEL_WIDTH, form->output_panel->Top, form->output_panel->Width, form->output_panel->Height);
    output_panel->Parent = form->tsOperator;
    output_panel->Color = form->output_panel->Color;
    output_panel->BevelOuter = bvNone;
    output_panel->Tag = panel_id;

    CopyInputPanelButton(form->output_panel_dsp_btn, output_panel)->Caption = "DSP" + IntToStr(panel_id);
    CopyInputPanelButton(form->output_panel_eq_btn, output_panel);
    CopyInputPanelButton(form->output_panel_limit_btn, output_panel);

    Graphics::TBitmap * bmp = new Graphics::TBitmap();
    form->ImageList1->GetBitmap(panel_id-1, bmp);
    CopyInputPanelButton(form->output_panel_number_btn, output_panel, bmp);

    CopyInputPanelButton(form->output_panel_invert_btn, output_panel);
    CopyInputPanelButton(form->output_panel_mute_btn, output_panel);
    form->output_level_edit[panel_id-1] = CopyInputPanelEdit(form->output_panel_level_edit, output_panel);
    CopyInputPanelTrackbar(form->output_panel_trackbar, output_panel);
    CopyInputPanelLabel(form->output_panel_dsp_num, output_panel)->Caption = IntToStr(panel_id);
    CopyInputPanelBkground(form->output_panel_bkground, output_panel);
}
static void CopyWatchPanel(int panel_id, TForm1 * form, char label, int left)
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

    TProgressBar * pb_level = new TProgressBar(watch_panel);
    pb_level->BoundsRect = form->pb_watch->BoundsRect;
    pb_level->Max = form->pb_watch->Max;
    pb_level->Min = form->pb_watch->Min;
    pb_level->Position = form->pb_watch->Position;
    pb_level->Orientation = form->pb_watch->Orientation;
    pb_level->Parent = watch_panel;
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
}
__fastcall TForm1::TForm1(TComponent* Owner)
    : TForm(Owner)
{
    pb_watch_list[0] = pb_watch;
    // 生成InputPanel
    for (int i=2;i<=16;i++)
    {
        CreateInputPanel(i, this);
        CopyWatchPanel(i, this, 'A'-1+i, (i-1) * PANEL_WIDTH);
    }
    for (int i=2;i<=8;i++)
    {
        CreateOutputPanel(i, this);
    }
    for (int i=17;i<=17+7;i++)
    {
        CopyWatchPanel(i, this, '1'+(i-17), mix_panel->Left + mix_panel->Width + (i-17) * PANEL_WIDTH);
    }
    input_level_edit[0] = input_panel_level_edit;
    SetWindowLong(input_panel_level_edit->Handle, GWL_STYLE, GetWindowLong(input_panel_level_edit->Handle, GWL_STYLE) | ES_RIGHT);

    output_level_edit[0] = output_panel_level_edit;
    SetWindowLong(output_panel_level_edit->Handle, GWL_STYLE, GetWindowLong(output_panel_level_edit->Handle, GWL_STYLE) | ES_RIGHT);

    input_level_edit[16] = mix_panel_level_edit;
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

    for (int i=0;i<9;i++)
    {
        filter_set.SetBypass(i, false);
        filter_set.GetFilter(i)->ChangFilterParameter("Peaking", 1000, 0, 4.09);
        filter_set.GetFilter(i)->name = IntToStr(i-1);
    }
    filter_set.GetFilter(1)->ChangFilterParameter("LowShelving", 1000, 0, 4.09);
    filter_set.GetFilter(7)->ChangFilterParameter("HighShelving", 1000, 0, 4.09);

    filter_set.GetFilter(1)->name = "L";
    filter_set.GetFilter(7)->name = "H";

    pnlDspDetail->DoubleBuffered = true;

    panel_agent->SetPanel(0, panelBand0, edtFreq0, edtQ0, edtGain0, cbType0, cbBypass0);
    panel_agent->SetPanel(1, panelBandL, edtFreqL, edtQL, edtGainL, cbTypeL, cbBypassL);
    panel_agent->SetPanel(2, panelBand1, edtFreq1, edtQ1, edtGain1, cbType1, cbBypass1);
    panel_agent->SetPanel(3, panelBand2, edtFreq2, edtQ2, edtGain2, cbType2, cbBypass2);
    panel_agent->SetPanel(4, panelBand3, edtFreq3, edtQ3, edtGain3, cbType3, cbBypass3);
    panel_agent->SetPanel(5, panelBand4, edtFreq4, edtQ4, edtGain4, cbType4, cbBypass4);
    panel_agent->SetPanel(6, panelBand5, edtFreq5, edtQ5, edtGain5, cbType5, cbBypass5);
    panel_agent->SetPanel(7, panelBandH, edtFreqH, edtQH, edtGainH, cbTypeH, cbBypassH);

    InitGdipus();

    pnlDspDetail->BringToFront();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::FormCreate(TObject *Sender)
{
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
    edtDebug->Text = "";
    cmd.preset = cbPreset->ItemIndex + 1;
    unsigned __int8 * p = (unsigned __int8*)&cmd;
    for (int i=30;i<sizeof(cmd);i++)
    {
        edtDebug->Text = edtDebug->Text + IntToHex(p[i], 2) + " ";
    }
    udpControl->SendBuffer(dst_ip, 2305, &cmd, sizeof(cmd));
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

    String broadcast_ip = selected->SubItems->Strings[1];
    // 初始化socket
    udpControl->Active = false;
    udpControl->Bindings->Clear();
    udpControl->Bindings->Add();
    udpControl->Bindings->Items[0]->IP = broadcast_ip;
    udpControl->Bindings->Items[0]->Port = 0;
    udpControl->Active = true;

    if (dst_ip == "")
    {
        return;
    }
    else
    {
    }
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
    int value = track->Max - track->Position;
    int dsp_num = track->Parent->Tag;

    if (input_level_edit[dsp_num-1] != NULL)
    {
        input_level_edit[dsp_num-1]->Text = value;
    }

    if (last_out_num_btn == NULL)
    {
        D1608Cmd cmd = InputVolume(dsp_num, value);
        SendCmd(cmd);
    }
    else
    {
        int out_dsp_num = last_out_num_btn->Parent->Tag;
        D1608Cmd cmd = IOVolume(out_dsp_num, dsp_num, value);
        SendCmd(cmd);
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ToogleMute(TObject *Sender)
{
    TSpeedButton* btn = (TSpeedButton*)Sender;
    int dsp_id = btn->Parent->Tag;
    static int input_mute = 0;
    input_mute = SetBit(input_mute, dsp_id, btn->Down);
    D1608Cmd cmd = InputMute(input_mute);
    SendCmd(cmd);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ToogleNoise(TObject *Sender)
{
    TSpeedButton* btn = (TSpeedButton*)Sender;
    int dsp_id = btn->Parent->Tag;
    static int input_noise = 0;
    input_noise = SetBit(input_noise, dsp_id, btn->Down);
    D1608Cmd cmd = InputNoise(input_noise);
    SendCmd(cmd);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ToogleInvert(TObject *Sender)
{
    TSpeedButton* btn = (TSpeedButton*)Sender;
    int dsp_id = btn->Parent->Tag;
    static int input_invert = 0;
    input_invert = SetBit(input_invert, dsp_id, btn->Down);
    D1608Cmd cmd = InputInvert(input_invert);
    SendCmd(cmd);
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
        int dsp_id = btn->Parent->Tag;
        D1608Cmd cmd = InputDefault(dsp_id);
        SendCmd(cmd);
    }
    else
    {
        // default 缺少取消所有 default
        last_default_btn = NULL;
        D1608Cmd cmd = InputDefault(0);
        SendCmd(cmd);
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ToogleAuto(TObject *Sender)
{
    TSpeedButton* btn = (TSpeedButton*)Sender;
    int dsp_id = btn->Parent->Tag;
    static int input_auto = 0;
    input_auto = SetBit(input_auto, dsp_id, btn->Down);
    D1608Cmd cmd = InputAuto(input_auto);
    SendCmd(cmd);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ToogleCOMP(TObject *Sender)
{
    TSpeedButton* btn = (TSpeedButton*)Sender;
    int dsp_id = btn->Parent->Tag;
    static int input_comp = 0;
    input_comp = SetBit(input_comp, dsp_id, btn->Down);
    D1608Cmd cmd = InputComp(input_comp);
    SendCmd(cmd);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ToogleEQ(TObject *Sender)
{
    TSpeedButton* btn = (TSpeedButton*)Sender;
    int dsp_id = btn->Parent->Tag;
    static int input_eq = 0;
    input_eq = SetBit(input_eq, dsp_id, btn->Down);
    D1608Cmd cmd = InputEQ(input_eq);
    SendCmd(cmd);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::udpControlUDPRead(TObject *Sender, TStream *AData,
      TIdSocketHandle *ABinding)
{
    T_iap_data_pack data_pack = {0};
    AData->ReadBuffer(&data_pack, std::min(sizeof(data_pack), AData->Size));

    switch(data_pack.cmd)
    {
    case 0x001a073a:
        // 音量输出
        MsgWatchHandle(data_pack);
        break;
    }
}
//---------------------------------------------------------------------------
void TForm1::MsgWatchHandle(const T_iap_data_pack & data_pack)
{
    struct WatchData
    {
        __int8 input_value[16];
        __int8 output_value[8];
    };
    WatchData * watch_data = (WatchData*)data_pack.data;

    for (int i=0;i<16;i++)
    {
        pb_watch_list[i]->Position = watch_data->input_value[i];
    }

    for (int i=0;i<8;i++)
    {
        pb_watch_list[i+16]->Position = watch_data->output_value[i];
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::tmWatchTimer(TObject *Sender)
{
    if (cbWatch->Checked)
    {
        D1608Cmd cmd = Watch();
        SendCmd(cmd);
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ToogleOutputMute(TObject *Sender)
{
    TSpeedButton* btn = (TSpeedButton*)Sender;
    int dsp_id = btn->Parent->Tag;
    static int output_mute = 0;
    output_mute = SetBit(output_mute, dsp_id, btn->Down);
    D1608Cmd cmd = OutputMute(output_mute);
    SendCmd(cmd);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ToogleOutputInvert(TObject *Sender)
{
    TSpeedButton* btn = (TSpeedButton*)Sender;
    int dsp_id = btn->Parent->Tag;
    static int output_invert = 0;
    output_invert = SetBit(output_invert, dsp_id, btn->Down);
    D1608Cmd cmd = OutputInvert(output_invert);
    SendCmd(cmd);
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
    // 1 ~ 8
    if (last_out_num_btn)
    {
        last_out_num_btn->Down = false;
    }

    TSpeedButton* btn = (TSpeedButton*)Sender;
    if (btn->Down)
    {
        last_out_num_btn = btn;
    }
    else
    {
        last_out_num_btn = NULL;
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ToogleLimit(TObject *Sender)
{
    TSpeedButton* btn = (TSpeedButton*)Sender;
    int dsp_id = btn->Parent->Tag;
    static int output_limit = 0;
    output_limit = SetBit(output_limit, dsp_id, btn->Down);
    D1608Cmd cmd = OutputLimit(output_limit);
    SendCmd(cmd);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ToogleOutputEQ(TObject *Sender)
{
    TSpeedButton* btn = (TSpeedButton*)Sender;
    int dsp_id = btn->Parent->Tag;
    static int output_eq = 0;
    output_eq = SetBit(output_eq, dsp_id, btn->Down);
    D1608Cmd cmd = OutputEQ(output_eq);
    SendCmd(cmd);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::OutputVolumeChange(TObject *Sender)
{
    TAdvTrackBar* track = (TAdvTrackBar*)Sender;
    int value = track->Max - track->Position;
    int dsp_num = track->Parent->Tag;

    if (output_level_edit[dsp_num-1] != NULL)
    {
        output_level_edit[dsp_num-1]->Text = value;
    }

    D1608Cmd cmd = OutputVolume(dsp_num, value);
    SendCmd(cmd);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::lvDeviceDblClick(TObject *Sender)
{
    TListItem * item = lvDevice->Selected;
    if (item != NULL)
    {
        cbAutoRefresh->Checked = false;
        btnSelect->Click();
        tsOperator->Show();
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ToggleDSP(TObject *Sender)
{
    // DSP button
    if (last_dsp_btn)
    {
        last_dsp_btn->Down = false;
    }

    TSpeedButton* btn = (TSpeedButton*)Sender;
    if (btn->Down)
    {
        last_dsp_btn = btn;
        pnlDspDetail->Left = 0;
        pnlDspDetail->Top = 192;
        pnlDspDetail->Show();
        pnlDspDetail->Tag = btn->Parent->Tag;
        
        if (btn->Tag == 1)
        {
            lblDSPInfo->Caption = "Input Channel " + IntToStr(btn->Parent->Tag) + " DSP Setup";
        }
        else
        {
            lblDSPInfo->Caption = "Output Channel " + IntToStr(btn->Parent->Tag) + " DSP Setup";
        }

        // TODO: 加载面板参数和不同类型
    }
    else
    {
        last_dsp_btn = NULL;
        pnlDspDetail->Hide();
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::SpeedButton80Click(TObject *Sender)
{
    // mix_panel_state bit3
    TSpeedButton* btn = (TSpeedButton*)Sender;
    mix_panel_state = SetBit(mix_panel_state, 3, btn->Down);
    D1608Cmd cmd = Mix_MaxPrio_Invert_Mute(mix_panel_state);
    udpControl->SendBuffer(dst_ip, 2305, &cmd, sizeof(cmd)-1);
    SendCmd(cmd);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btnMAXONClick(TObject *Sender)
{
    // mix_panel_state bit1
    mix_panel_state = SetBit(mix_panel_state, 1, btnMAXON->Down);
    D1608Cmd cmd = Mix_MaxPrio_Invert_Mute(mix_panel_state);
    SendCmd(cmd);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::SpeedButton108Click(TObject *Sender)
{
    TSpeedButton* btn = (TSpeedButton*)Sender;
    D1608Cmd cmd = LastOn(btn->Down);
    SendCmd(cmd);
}
//---------------------------------------------------------------------------

void __fastcall TForm1::SpeedButton73Click(TObject *Sender)
{
    TSpeedButton* btn = (TSpeedButton*)Sender;
    D1608Cmd cmd = FShift(btn->Down);
    SendCmd(cmd);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::MasterVolumeChange(TObject *Sender)
{
    TAdvTrackBar* track = (TAdvTrackBar*)Sender;
    int value = track->Max - track->Position;
    master_panel_level_edit->Text = value;
    D1608Cmd cmd = MasterVolume(value);
    SendCmd(cmd);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btnMixMuteClick(TObject *Sender)
{
    // mix_panel_state bit4
    TSpeedButton* btn = (TSpeedButton*)Sender;
    mix_panel_state = SetBit(mix_panel_state, 4, btn->Down);
    D1608Cmd cmd = Mix_MaxPrio_Invert_Mute(mix_panel_state);
    SendCmd(cmd);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::btnMasterMuteClick(TObject *Sender)
{
    TSpeedButton* btn = (TSpeedButton*)Sender;
    D1608Cmd cmd = MasterMute(btn->Down);
    SendCmd(cmd);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::SpeedButton84Click(TObject *Sender)
{
    TSpeedButton* btn = (TSpeedButton*)Sender;
    int dsp_id = btn->Parent->Tag;
    static int phanton = 0;
    phanton = SetBit(phanton, dsp_id, btn->Down);
    D1608Cmd cmd = Phanton(phanton);
    SendCmd(cmd);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ToogleLowShelf(TObject *Sender)
{
    // TODO: 发送命令
    TCheckBox* btn = (TCheckBox*)Sender;
    int dsp_id = btn->Parent->Parent->Tag;
    static int low = 0;
    low = SetBit(low, dsp_id, btn->Checked);
    D1608Cmd cmd = LowShelf(low);
    SendCmd(cmd);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ToogleHighShelf(TObject *Sender)
{
    // TODO: 发送命令
    TCheckBox* btn = (TCheckBox*)Sender;
    int dsp_id = btn->Parent->Tag;
    static int high = 0;
    high = SetBit(high, dsp_id, btn->Checked);
    D1608Cmd cmd = HighShelf(high);
    SendCmd(cmd);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ToogleEQ_DSP(TObject *Sender)
{
    // TODO: 发送命令
    TCheckBox* btn = (TCheckBox*)Sender;
    int dsp_id = btn->Parent->Tag;
    int eq_id = btn->Tag;
    static int eq_switch[5] = {0,0,0,0,0};
    eq_switch[eq_id-1] = SetBit(eq_switch[eq_id-1], dsp_id, btn->Checked);
    D1608Cmd cmd = DspEQSwitch(eq_id, eq_switch[eq_id-1]);
    SendCmd(cmd);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::TrackBar27Change(TObject *Sender)
{
    TTrackBar* track = (TTrackBar*)Sender;
    int value = track->Max - track->Position;
    int dsp_num = track->Parent->Tag;

    D1608Cmd cmd = DspInputVolume(dsp_num, value);
    SendCmd(cmd);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::FormMouseWheel(TObject *Sender, TShiftState Shift,
      int WheelDelta, TPoint &MousePos, bool &Handled)
{
    paint_agent->OnMouseWheel(Sender, Shift, WheelDelta, MousePos, Handled);
    if (!Handled)
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
        PopupMenu1->PopupComponent = input_type;
        PopupMenu1->Popup(p.x, p.y);
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::M41Click(TObject *Sender)
{
    TLabel * popup_label = (TLabel*)PopupMenu1->PopupComponent;
    popup_label->Caption = ((TMenuItem*)Sender)->Caption;
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

