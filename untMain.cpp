//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "untMain.h"
#include <winsock2.h>
#include <algorithm>
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma link "CSPIN"
#pragma resource "*.dfm"
#pragma comment(lib, "gdiplus.lib")
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
__fastcall TForm1::TForm1(TComponent* Owner)
    : TForm(Owner)
{
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
    panel_agent = new PanelAgent(filter_set);
    paint_agent = new PaintAgent(PaintBox1, filter_set);
    filter_set.Register(paint_agent, panel_agent);

    for (int i=0;i<9;i++)
    {
        filter_set.SetBypass(i, false);
        filter_set.GetFilter(i)->ChangFilterParameter("Peaking", 1000, 0, 4.09);
    }
    filter_set.GetFilter(1)->ChangFilterParameter("LowShelving", 1000, 0, 4.09);
    filter_set.GetFilter(7)->ChangFilterParameter("HighShelving", 1000, 0, 4.09);

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
}
//---------------------------------------------------------------------------
void __fastcall TForm1::FormCreate(TObject *Sender)
{
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

    tsSearch->Show();
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
        udpSLP->Active = true;
        char search_flag[] = "\x53\x65\x74\x50\x61\x72\x61\x00\x00\x00\x4d\x41\x54\x31\x36\x31\x30\x43\x6f\x6e\x66\x69\x67\x44\x61\x74\x61\x00\x00\x00\xca\x03\x00\x00\xd8\x00\x00\x00";
        udpSLP->SendBuffer("255.255.255.255", 2305, search_flag, sizeof(search_flag));
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
void __fastcall TForm1::TrackBar1Change(TObject *Sender)
{
    TTrackBar* track = (TTrackBar*)Sender;
    int value = track->Max - track->Position;
    int dsp_num = track->Parent->Tag;

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
void __fastcall TForm1::SpeedButton7Click(TObject *Sender)
{
    TSpeedButton* btn = (TSpeedButton*)Sender;
    int dsp_id = btn->Parent->Tag;
    static int input_mute = 0;
    input_mute = SetBit(input_mute, dsp_id, btn->Down);
    D1608Cmd cmd = InputMute(input_mute);
    SendCmd(cmd);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::SpeedButton6Click(TObject *Sender)
{
    TSpeedButton* btn = (TSpeedButton*)Sender;
    int dsp_id = btn->Parent->Tag;
    static int input_noise = 0;
    input_noise = SetBit(input_noise, dsp_id, btn->Down);
    D1608Cmd cmd = InputNoise(input_noise);
    SendCmd(cmd);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::SpeedButton5Click(TObject *Sender)
{
    TSpeedButton* btn = (TSpeedButton*)Sender;
    int dsp_id = btn->Parent->Tag;
    static int input_invert = 0;
    input_invert = SetBit(input_invert, dsp_id, btn->Down);
    D1608Cmd cmd = InputInvert(input_invert);
    SendCmd(cmd);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::SpeedButton4Click(TObject *Sender)
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
void __fastcall TForm1::SpeedButton3Click(TObject *Sender)
{
    TSpeedButton* btn = (TSpeedButton*)Sender;
    int dsp_id = btn->Parent->Tag;
    static int input_auto = 0;
    input_auto = SetBit(input_auto, dsp_id, btn->Down);
    D1608Cmd cmd = InputAuto(input_auto);
    SendCmd(cmd);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::SpeedButton2Click(TObject *Sender)
{
    TSpeedButton* btn = (TSpeedButton*)Sender;
    int dsp_id = btn->Parent->Tag;
    static int input_comp = 0;
    input_comp = SetBit(input_comp, dsp_id, btn->Down);
    D1608Cmd cmd = InputComp(input_comp);
    SendCmd(cmd);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::SpeedButton1Click(TObject *Sender)
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

    edtPB1->Text = ProgressBar1->Position;
    ProgressBar1 ->Position = watch_data->input_value[0 ];
    ProgressBar2 ->Position = watch_data->input_value[1 ];
    ProgressBar3 ->Position = watch_data->input_value[2 ];
    ProgressBar4 ->Position = watch_data->input_value[3 ];
    ProgressBar5 ->Position = watch_data->input_value[4 ];
    ProgressBar6 ->Position = watch_data->input_value[5 ];
    ProgressBar7 ->Position = watch_data->input_value[6 ];
    ProgressBar8 ->Position = watch_data->input_value[7 ];
    ProgressBar9 ->Position = watch_data->input_value[8 ];
    ProgressBar10->Position = watch_data->input_value[9 ];
    ProgressBar11->Position = watch_data->input_value[10];
    ProgressBar12->Position = watch_data->input_value[11];
    ProgressBar13->Position = watch_data->input_value[12];
    ProgressBar14->Position = watch_data->input_value[13];
    ProgressBar15->Position = watch_data->input_value[14];
    ProgressBar16->Position = watch_data->input_value[15];

    edtPB17->Text = ProgressBar17->Position;
    ProgressBar17->Position = watch_data->output_value[0];
    ProgressBar18->Position = watch_data->output_value[1];
    ProgressBar19->Position = watch_data->output_value[2];
    ProgressBar20->Position = watch_data->output_value[3];
    ProgressBar21->Position = watch_data->output_value[4];
    ProgressBar22->Position = watch_data->output_value[5];
    ProgressBar23->Position = watch_data->output_value[6];
    ProgressBar24->Position = watch_data->output_value[7];
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
void __fastcall TForm1::SpeedButton118Click(TObject *Sender)
{
    TSpeedButton* btn = (TSpeedButton*)Sender;
    int dsp_id = btn->Parent->Tag;
    static int output_mute = 0;
    output_mute = SetBit(output_mute, dsp_id, btn->Down);
    D1608Cmd cmd = OutputMute(output_mute);
    SendCmd(cmd);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::SpeedButton116Click(TObject *Sender)
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
void __fastcall TForm1::SpeedButton126Click(TObject *Sender)
{
    // DO2
}
//---------------------------------------------------------------------------
void __fastcall TForm1::SpeedButton115Click(TObject *Sender)
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
void __fastcall TForm1::SpeedButton114Click(TObject *Sender)
{
    TSpeedButton* btn = (TSpeedButton*)Sender;
    int dsp_id = btn->Parent->Tag;
    static int output_limit = 0;
    output_limit = SetBit(output_limit, dsp_id, btn->Down);
    D1608Cmd cmd = OutputLimit(output_limit);
    SendCmd(cmd);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::SpeedButton113Click(TObject *Sender)
{
    TSpeedButton* btn = (TSpeedButton*)Sender;
    int dsp_id = btn->Parent->Tag;
    static int output_eq = 0;
    output_eq = SetBit(output_eq, dsp_id, btn->Down);
    D1608Cmd cmd = OutputEQ(output_eq);
    SendCmd(cmd);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::TrackBar17Change(TObject *Sender)
{
    TTrackBar* track = (TTrackBar*)Sender;
    int value = track->Max - track->Position;
    int dsp_num = track->Parent->Tag;
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
void __fastcall TForm1::SpeedButton77Click(TObject *Sender)
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

void __fastcall TForm1::TrackBar26Change(TObject *Sender)
{
    TTrackBar* track = (TTrackBar*)Sender;
    int value = track->Max - track->Position;
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
void __fastcall TForm1::SpeedButton87Click(TObject *Sender)
{
#if 0
    // TODO: 发送命令
    TCheckBox* btn = (TCheckBox*)Sender;
    int dsp_id = btn->Parent->Parent->Tag;
    static int low = 0;
    low = SetBit(low, dsp_id, btn->Down);
    D1608Cmd cmd = LowShelf(low);
    SendCmd(cmd);
#endif
}
//---------------------------------------------------------------------------
void __fastcall TForm1::SpeedButton178Click(TObject *Sender)
{
#if 0
    // TODO: 发送命令
    TSpeedButton* btn = (TSpeedButton*)Sender;
    int dsp_id = btn->Parent->Tag;
    static int high = 0;
    high = SetBit(high, dsp_id, btn->Down);
    D1608Cmd cmd = HighShelf(high);
    SendCmd(cmd);
#endif
}
//---------------------------------------------------------------------------
void __fastcall TForm1::SpeedButton105Click(TObject *Sender)
{
#if 0
    // TODO: 发送命令
    TSpeedButton* btn = (TSpeedButton*)Sender;
    int dsp_id = btn->Parent->Tag;
    int eq_id = btn->Tag;
    static int eq_switch[5] = {0,0,0,0,0};
    eq_switch[eq_id-1] = SetBit(eq_switch[eq_id-1], dsp_id, btn->Down);
    D1608Cmd cmd = DspEQSwitch(eq_id, eq_switch[eq_id-1]);
    SendCmd(cmd);
#endif
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

