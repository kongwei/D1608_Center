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
#pragma link "AdvTrackBar"
#pragma link "SpeedButtonNoFrame"
#pragma resource "*.dfm"
#pragma comment(lib, "gdiplus.lib")

#define PANEL_WIDTH 48

TForm1 *Form1;
ConfigMap config_map;

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
static TSpeedButton * CopyInputPanelButton(TSpeedButton * src_btn, int dsp_id, Graphics::TBitmap* bmp=NULL)
{
    TSpeedButton * dsp_btn = new TSpeedButtonNoFrame(src_btn->Parent);
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
static TAdvTrackBar * CopyInputPanelTrackbar(TAdvTrackBar * src_trackbar, int dsp_id)
{
    TAdvTrackBar * trackbar = new TAdvTrackBar(src_trackbar->Parent);
    trackbar->Parent = src_trackbar->Parent;

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
    return label;
}
static void CreateInputPanel(int panel_id, TForm1 * form)
{
    CopyInputPanelButton(form->input_panel_dsp_btn, panel_id)->Caption = "DSP " + String(char('A'-1+panel_id));
    CopyInputPanelButton(form->input_panel_eq_btn, panel_id);
    CopyInputPanelButton(form->input_panel_comp_btn, panel_id);
    CopyInputPanelButton(form->input_panel_auto_btn, panel_id);
    CopyInputPanelButton(form->input_panel_default_btn, panel_id);
    CopyInputPanelButton(form->input_panel_invert_btn, panel_id);
    CopyInputPanelButton(form->input_panel_noise_btn, panel_id);
    CopyInputPanelButton(form->input_panel_mute_btn, panel_id);
    form->input_level_edit[panel_id-1] = CopyInputPanelEdit(form->input_panel_level_edit, panel_id);
    CopyInputPanelTrackbar(form->input_panel_trackbar, panel_id);
    CopyInputPanelLabel(form->input_panel_dsp_num, panel_id)->Caption = String(char('A'-1+panel_id));
}
static void CreateOutputPanel(int panel_id, TForm1 * form)
{
    TSpeedButton * btn_output_dsp = CopyInputPanelButton(form->output_panel_dsp_btn, panel_id);
        btn_output_dsp->Caption = "DSP" + IntToStr(panel_id);
        btn_output_dsp->Tag = 100 + panel_id;
    CopyInputPanelButton(form->output_panel_eq_btn, panel_id);
    CopyInputPanelButton(form->output_panel_limit_btn, panel_id);

    Graphics::TBitmap * bmp = new Graphics::TBitmap();
    form->ImageList1->GetBitmap(panel_id-1, bmp);
    CopyInputPanelButton(form->output_panel_number_btn, panel_id, bmp);

    CopyInputPanelButton(form->output_panel_invert_btn, panel_id);
    CopyInputPanelButton(form->output_panel_mute_btn, panel_id);
    form->output_level_edit[panel_id-1] = CopyInputPanelEdit(form->output_panel_level_edit, panel_id);
    CopyInputPanelTrackbar(form->output_panel_trackbar, panel_id);
    CopyInputPanelLabel(form->output_panel_dsp_num, panel_id)->Caption = IntToStr(panel_id);
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
    input_type->Tag = panel_id;
}
__fastcall TForm1::TForm1(TComponent* Owner)
    : TForm(Owner)
{
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
    for (int i=2;i<=16;i++)
    {
        CreateInputPanel(i, this);
        CopyWatchPanel(i, this, 'A'-1+i, (i-1) * PANEL_WIDTH);
    }

    //------------------------------------
    output_panel_bkground->Width = 8 * PANEL_WIDTH;
    output_panel_bkground->Picture->Bitmap->Width = 8 * PANEL_WIDTH;
    for (int i=1;i<8;i++)
    {
        TRect templet_image_rect = Image3->BoundsRect;
        TRect dest_rect = TRect(i*PANEL_WIDTH,
                                templet_image_rect.Top,
                                (i+1)*PANEL_WIDTH,
                                templet_image_rect.Bottom);
        output_panel_bkground->Canvas->CopyRect(dest_rect, output_panel_bkground->Canvas, templet_image_rect);
    }
    for (int i=2;i<=8;i++)
    {
        CreateOutputPanel(i, this);
    }

    //----------------------------------
    for (int i=17;i<=17+7;i++)
    {
        CopyWatchPanel(i, this, '1'+(i-17), mix_panel->Left + mix_panel->Width + (i-17) * PANEL_WIDTH);
    }
    input_level_edit[0] = input_panel_level_edit;
    SetWindowLong(input_panel_level_edit->Handle, GWL_STYLE, GetWindowLong(input_panel_level_edit->Handle, GWL_STYLE) | ES_RIGHT);
    input_panel_trackbar->OnChange(input_panel_trackbar);

    output_level_edit[0] = output_panel_level_edit;
    SetWindowLong(output_panel_level_edit->Handle, GWL_STYLE, GetWindowLong(output_panel_level_edit->Handle, GWL_STYLE) | ES_RIGHT);
    output_panel_trackbar->OnChange(output_panel_trackbar);

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

    pnlDspDetail->DoubleBuffered = true;

    panel_agent->SetPanel(0, panelBand0, edtFreq0, edtQ0, edtGain0, cbType0, cbBypass0);
    panel_agent->SetPanel(1, panelBand1, edtFreq1, edtQ1, edtGain1, cbType1, cbBypass1);
    panel_agent->SetPanel(2, panelBand2, edtFreq2, edtQ2, edtGain2, cbType2, cbBypass2);
    panel_agent->SetPanel(3, panelBand3, edtFreq3, edtQ3, edtGain3, cbType3, cbBypass3);
    panel_agent->SetPanel(4, panelBand4, edtFreq4, edtQ4, edtGain4, cbType4, cbBypass4);
    panel_agent->SetPanel(5, panelBand5, edtFreq5, edtQ5, edtGain5, cbType5, cbBypass5);
    panel_agent->SetPanel(6, panelBand6, edtFreq6, edtQ6, edtGain6, cbType6, cbBypass6);
    panel_agent->SetPanel(7, panelBand7, edtFreq7, edtQ7, edtGain7, cbType7, cbBypass7);
    panel_agent->SetPanel(8, panelBand8, edtFreq8, edtQ8, edtGain8, cbType8, cbBypass8);

    btnDspResetEQ->Click();

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
    this->Repaint();
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
    int value = track->Position;
    int dsp_num = track->Tag;

    if (input_level_edit[dsp_num-1] != NULL)
    {
        if (value == -720)
        {
            input_level_edit[dsp_num-1]->Text = " -- ";
        }
        else
        {
            String x;
            input_level_edit[dsp_num-1]->Text = x.sprintf("%1.1f", value/10.0);
        }
    }

    if (last_out_num_btn == NULL)
    {
        D1608Cmd cmd;
        cmd.id = GerOffsetOfData(&config_map.input_dsp[dsp_num-1].level_a);
        cmd.value = value;
        SendCmd(cmd);
    }
    else
    {
        // TODO: MIX Level
        int out_dsp_num = last_out_num_btn->Tag;
        D1608Cmd cmd = IOVolume(out_dsp_num, dsp_num, value);
        SendCmd(cmd);
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ToogleMute(TObject *Sender)
{
    TSpeedButton* btn = (TSpeedButton*)Sender;
    int dsp_id = btn->Tag;

    D1608Cmd cmd;
    cmd.id = GerOffsetOfData(&config_map.input_dsp[dsp_id-1].mute_switch);
    cmd.value = btn->Down;
    SendCmd(cmd);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ToogleNoise(TObject *Sender)
{
    TSpeedButton* btn = (TSpeedButton*)Sender;
    int dsp_id = btn->Tag;
    static int input_noise = 0;
    input_noise = SetBit(input_noise, dsp_id, btn->Down);
    D1608Cmd cmd = InputNoise(input_noise);
    SendCmd(cmd);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ToogleInvert(TObject *Sender)
{
    TSpeedButton* btn = (TSpeedButton*)Sender;
    int dsp_id = btn->Tag;

    D1608Cmd cmd;
    cmd.id = GerOffsetOfData(&config_map.input_dsp[dsp_id-1].invert_switch);
    cmd.value = btn->Down;
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
        int dsp_id = btn->Tag;
        D1608Cmd cmd = InputDefault(dsp_id, 0);
        SendCmd(cmd);
    }
    else
    {
        // default 缺少取消所有 default
        last_default_btn = NULL;
        D1608Cmd cmd = InputDefault(0, 0);
        SendCmd(cmd);
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ToogleAuto(TObject *Sender)
{
    TSpeedButton* btn = (TSpeedButton*)Sender;
    int dsp_id = btn->Tag;
    static int input_auto = 0;
    input_auto = SetBit(input_auto, dsp_id, btn->Down);
    D1608Cmd cmd = InputAuto(input_auto);
    SendCmd(cmd);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ToogleCOMP(TObject *Sender)
{
/*    TSpeedButton* btn = (TSpeedButton*)Sender;
    int dsp_id = btn->Tag;
    static int input_comp = 0;
    input_comp = SetBit(input_comp, dsp_id, btn->Down);
    D1608Cmd cmd = InputComp(input_comp);
    SendCmd(cmd);*/
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ToogleEQ(TObject *Sender)
{
    TSpeedButton* btn = (TSpeedButton*)Sender;
    int dsp_id = btn->Tag;

    D1608Cmd cmd;
    cmd.id = GerOffsetOfData(&config_map.input_dsp[dsp_id-1].eq_switch);
    cmd.value = btn->Down;
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
    int dsp_id = btn->Tag;

    D1608Cmd cmd;
    cmd.id = GerOffsetOfData(&config_map.output_dsp[dsp_id-1].mute_switch);
    cmd.value = btn->Down;
    SendCmd(cmd);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ToogleOutputInvert(TObject *Sender)
{
    TSpeedButton* btn = (TSpeedButton*)Sender;
    int dsp_id = btn->Tag;

    D1608Cmd cmd;
    cmd.id = GerOffsetOfData(&config_map.output_dsp[dsp_id-1].invert_switch);
    cmd.value = btn->Down;
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
/*    TSpeedButton* btn = (TSpeedButton*)Sender;
    int dsp_id = btn->Tag;
    static int output_limit = 0;
    output_limit = SetBit(output_limit, dsp_id, btn->Down);
    D1608Cmd cmd = OutputLimit(output_limit);
    SendCmd(cmd);*/
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ToogleOutputEQ(TObject *Sender)
{
    TSpeedButton* btn = (TSpeedButton*)Sender;
    int dsp_id = btn->Tag;

    D1608Cmd cmd;
    cmd.id = GerOffsetOfData(&config_map.output_dsp[dsp_id-1].eq_switch);
    cmd.value = btn->Down;
    SendCmd(cmd);
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
            output_level_edit[dsp_num-1]->Text = " -- ";
        }
        else
        {
            String x;
            output_level_edit[dsp_num-1]->Text = x.sprintf("%1.1f", value/10.0);
        }
    }

    D1608Cmd cmd;
    cmd.id = GerOffsetOfData(&config_map.output_dsp[dsp_num-1].level_a);
    cmd.value = value;
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

        if (btn->Tag < 100)
        {
            lblDSPInfo->Caption = "Input Channel " + IntToStr(btn->Tag) + " DSP Setup";
            pnlDspDetail->Left = 0;
        }
        else
        {
            lblDSPInfo->Caption = "Output Channel " + IntToStr(btn->Tag-100) + " DSP Setup";
            pnlDspDetail->Left = Width - pnlDspDetail->Width;
        }

        pnlDspDetail->Top = 192;
        pnlDspDetail->Show();
        pnlDspDetail->Tag = btn->Tag;

        // TODO: 加载面板参数和不同类型
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
void __fastcall TForm1::btnLastOnClick(TObject *Sender)
{
    TSpeedButton* btn = (TSpeedButton*)Sender;
    D1608Cmd cmd = InputDefault(0, btn->Down);
    SendCmd(cmd);
}
//---------------------------------------------------------------------------

void __fastcall TForm1::btnFShiftClick(TObject *Sender)
{
    TSpeedButton* btn = (TSpeedButton*)Sender;
    D1608Cmd cmd = FShift(btn->Down);
    SendCmd(cmd);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::MasterVolumeChange(TObject *Sender)
{
    TAdvTrackBar* track = (TAdvTrackBar*)Sender;
    int value = track->Position;
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
void __fastcall TForm1::btnPhantonClick(TObject *Sender)
{
    TSpeedButton* btn = (TSpeedButton*)Sender;
    int dsp_id = btn->Parent->Tag;

    D1608Cmd cmd;
    cmd.id = GerOffsetOfData(&config_map.input_dsp[dsp_id-1].phantom_switch);
    cmd.value = btn->Down;
    SendCmd(cmd);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ToogleLowShelf(TObject *Sender)
{
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
    TSpeedButton* btn = (TSpeedButton*)Sender;
    int dsp_id = btn->Parent->Tag;
    int eq_id = btn->Tag;
    static int eq_switch[5] = {0,0,0,0,0};
    eq_switch[eq_id-1] = SetBit(eq_switch[eq_id-1], dsp_id, btn->Down);
    D1608Cmd cmd = DspEQSwitch(eq_id, eq_switch[eq_id-1]);
    SendCmd(cmd);
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
        cmd.value = value;
        SendCmd(cmd);
    }
    else
    {
        // output channel
        D1608Cmd cmd;
        cmd.id = GerOffsetOfData(&config_map.output_dsp[dsp_num-101].level_b);
        cmd.value = value;
        SendCmd(cmd);
    }
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
    if (popup_label->Caption == "MIC")
    {
        cmd.value = 0;
    }
    else if (popup_label->Caption == "MIC(0)")
    {
        cmd.value = 1;
    }
    else if (popup_label->Caption == "400mv")
    {
        cmd.value = 5;
    }
    else if (popup_label->Caption == "10dBv")
    {
        cmd.value = 6;
    }
    else if (popup_label->Caption == "22dBu")
    {
        cmd.value = 3;
    }
    else if (popup_label->Caption == "24dBu")
    {
        cmd.value = 7;
    }
    SendCmd(cmd);
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
    if (popup_label->Caption == "200mv")
    {
        cmd.value = 7;
    }
    else if (popup_label->Caption == "10dBv")
    {
        cmd.value = 3;
    }
    else if (popup_label->Caption == "22dBu")
    {
        cmd.value = 1;
    }
    else if (popup_label->Caption == "24dBu")
    {
        cmd.value = 0;
    }
    SendCmd(cmd);
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
    filter_set.GetFilter(1)->ChangFilterParameter("Low Shelf", 50, 0, 4.09);
    filter_set.GetFilter(2)->ChangFilterParameter("Parametric", 100, 0, 4.09);
    filter_set.GetFilter(3)->ChangFilterParameter("Parametric", 200, 0, 4.09);
    filter_set.GetFilter(4)->ChangFilterParameter("Parametric", 500, 0, 4.09);
    filter_set.GetFilter(5)->ChangFilterParameter("Parametric", 1000, 0, 4.09);
    filter_set.GetFilter(6)->ChangFilterParameter("Parametric", 2000, 0, 4.09);
    filter_set.GetFilter(7)->ChangFilterParameter("Parametric", 5000, 0, 4.09);
    filter_set.GetFilter(8)->ChangFilterParameter("High Shelf", 10000, 0, 4.09);

    for (int i=1;i<=8;i++)
    {
        filter_set.GetFilter(i)->name = IntToStr(i);
        filter_set.SetBypass(i, false);
        filter_set.RepaintPaint(i);
    }
}
//---------------------------------------------------------------------------

