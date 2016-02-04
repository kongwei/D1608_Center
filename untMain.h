//---------------------------------------------------------------------------

#ifndef untMainH
#define untMainH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <Buttons.hpp>
#include <ComCtrls.hpp>
#include <Dialogs.hpp>
#include <ExtCtrls.hpp>
#include <IdBaseComponent.hpp>
#include <IdComponent.hpp>
#include <IdIPWatch.hpp>
#include <IdTCPClient.hpp>
#include <IdTCPConnection.hpp>
#include <IdUDPBase.hpp>
#include <IdUDPServer.hpp>
#include "CSPIN.h"
#include <ActnList.hpp>
#include "D1608Pack.h"

#include "WaveGraphic.h"
#include "FilterPanelSet.h"
#include "FilterList.h"

#define UDP_PORT 65518
#define TCP_PORT 15288
#define MAX_PACKAGE_SIZE 1024

#pragma pack(1)
    struct T_iap_data_pack
    {
        char flag[32];
        __int32 cmd;
        __int8 data[MAX_PACKAGE_SIZE];
    };
    struct T_iap_start_pack
    {
        char op[9];
        __int32 sfilelen;
    };
    struct T_slp_pack
    {
        __int8 ip[4];
        __int8 flag1[10];
        __int8 flag2[26];
        __int8 flag3[64];
        __int8 type[16];
        __int8 ser[16];
        __int8 ver[16];
        __int8 fwdate[20];
        __int8 mac[6];
        __int8 mask[4];
        __int8 gateway[4];
        __int16 port;
    };
#pragma pack()

//---------------------------------------------------------------------------
class TForm1 : public TForm
{
__published:	// IDE-managed Components
    TPageControl *PageControl1;
    TTabSheet *tsSearch;
    TTabSheet *tsOperator;
    TBevel *Bevel1;
    TLabel *Label1;
    TMemo *mmLog;
    TButton *btnRefresh;
    TListView *lvDevice;
    TListBox *lbIplist;
    TCheckBox *cbAutoRefresh;
    TCSpinEdit *spInterval;
    TButton *btnSelect;
    TIdUDPServer *udpSLP;
    TTimer *tmSLP;
    TIdUDPServer *udpControl;
    TProgressBar *ProgressBar1;
    TProgressBar *ProgressBar2;
    TProgressBar *ProgressBar3;
    TProgressBar *ProgressBar4;
    TProgressBar *ProgressBar5;
    TProgressBar *ProgressBar6;
    TProgressBar *ProgressBar7;
    TProgressBar *ProgressBar8;
    TProgressBar *ProgressBar9;
    TProgressBar *ProgressBar10;
    TProgressBar *ProgressBar11;
    TProgressBar *ProgressBar12;
    TProgressBar *ProgressBar13;
    TProgressBar *ProgressBar14;
    TProgressBar *ProgressBar15;
    TProgressBar *ProgressBar16;
    TProgressBar *ProgressBar17;
    TProgressBar *ProgressBar18;
    TProgressBar *ProgressBar19;
    TProgressBar *ProgressBar20;
    TProgressBar *ProgressBar21;
    TProgressBar *ProgressBar22;
    TProgressBar *ProgressBar23;
    TProgressBar *ProgressBar24;
    TPanel *Panel1;
    TSpeedButton *SpeedButton8;
    TSpeedButton *SpeedButton9;
    TSpeedButton *SpeedButton10;
    TSpeedButton *SpeedButton12;
    TSpeedButton *SpeedButton13;
    TSpeedButton *SpeedButton14;
    TLabel *Label3;
    TTrackBar *TrackBar2;
    TSpeedButton *SpeedButton11;
    TPanel *Panel2;
    TSpeedButton *SpeedButton1;
    TSpeedButton *SpeedButton2;
    TSpeedButton *SpeedButton3;
    TSpeedButton *SpeedButton4;
    TSpeedButton *SpeedButton5;
    TSpeedButton *SpeedButton6;
    TLabel *Label2;
    TSpeedButton *SpeedButton7;
    TTrackBar *TrackBar1;
    TPanel *Panel3;
    TSpeedButton *SpeedButton15;
    TSpeedButton *SpeedButton16;
    TSpeedButton *SpeedButton17;
    TSpeedButton *SpeedButton18;
    TSpeedButton *SpeedButton19;
    TSpeedButton *SpeedButton20;
    TLabel *Label4;
    TSpeedButton *SpeedButton21;
    TTrackBar *TrackBar3;
    TPanel *Panel4;
    TSpeedButton *SpeedButton22;
    TSpeedButton *SpeedButton23;
    TSpeedButton *SpeedButton24;
    TSpeedButton *SpeedButton25;
    TSpeedButton *SpeedButton26;
    TSpeedButton *SpeedButton27;
    TLabel *Label5;
    TSpeedButton *SpeedButton28;
    TTrackBar *TrackBar4;
    TPanel *Panel5;
    TSpeedButton *SpeedButton29;
    TSpeedButton *SpeedButton30;
    TSpeedButton *SpeedButton31;
    TSpeedButton *SpeedButton32;
    TSpeedButton *SpeedButton33;
    TSpeedButton *SpeedButton34;
    TLabel *Label6;
    TSpeedButton *SpeedButton35;
    TTrackBar *TrackBar5;
    TPanel *Panel6;
    TSpeedButton *SpeedButton36;
    TSpeedButton *SpeedButton37;
    TSpeedButton *SpeedButton38;
    TSpeedButton *SpeedButton39;
    TSpeedButton *SpeedButton40;
    TSpeedButton *SpeedButton41;
    TLabel *Label7;
    TSpeedButton *SpeedButton42;
    TTrackBar *TrackBar6;
    TCheckBox *cbWatch;
    TTimer *tmWatch;
    TPanel *Panel7;
    TSpeedButton *SpeedButton43;
    TSpeedButton *SpeedButton44;
    TSpeedButton *SpeedButton45;
    TSpeedButton *SpeedButton46;
    TSpeedButton *SpeedButton47;
    TSpeedButton *SpeedButton48;
    TLabel *Label8;
    TSpeedButton *SpeedButton49;
    TTrackBar *TrackBar7;
    TPanel *Panel8;
    TSpeedButton *SpeedButton50;
    TSpeedButton *SpeedButton51;
    TSpeedButton *SpeedButton52;
    TSpeedButton *SpeedButton53;
    TSpeedButton *SpeedButton54;
    TSpeedButton *SpeedButton55;
    TLabel *Label9;
    TSpeedButton *SpeedButton56;
    TTrackBar *TrackBar8;
    TPanel *Panel9;
    TSpeedButton *SpeedButton57;
    TSpeedButton *SpeedButton58;
    TSpeedButton *SpeedButton59;
    TSpeedButton *SpeedButton60;
    TSpeedButton *SpeedButton61;
    TSpeedButton *SpeedButton62;
    TLabel *Label10;
    TSpeedButton *SpeedButton63;
    TTrackBar *TrackBar9;
    TPanel *Panel10;
    TSpeedButton *SpeedButton64;
    TSpeedButton *SpeedButton65;
    TSpeedButton *SpeedButton66;
    TSpeedButton *SpeedButton67;
    TSpeedButton *SpeedButton68;
    TSpeedButton *SpeedButton69;
    TLabel *Label11;
    TSpeedButton *SpeedButton70;
    TTrackBar *TrackBar10;
    TPanel *Panel11;
    TSpeedButton *SpeedButton71;
    TSpeedButton *SpeedButton72;
    TSpeedButton *SpeedButton74;
    TSpeedButton *SpeedButton75;
    TSpeedButton *SpeedButton76;
    TLabel *Label12;
    TTrackBar *TrackBar11;
    TPanel *Panel12;
    TSpeedButton *SpeedButton78;
    TSpeedButton *SpeedButton79;
    TSpeedButton *SpeedButton81;
    TSpeedButton *SpeedButton82;
    TSpeedButton *SpeedButton83;
    TLabel *Label13;
    TTrackBar *TrackBar12;
    TPanel *Panel13;
    TSpeedButton *SpeedButton85;
    TSpeedButton *SpeedButton86;
    TSpeedButton *SpeedButton88;
    TSpeedButton *SpeedButton89;
    TSpeedButton *SpeedButton90;
    TLabel *Label14;
    TTrackBar *TrackBar13;
    TPanel *Panel14;
    TSpeedButton *SpeedButton92;
    TSpeedButton *SpeedButton93;
    TSpeedButton *SpeedButton95;
    TSpeedButton *SpeedButton96;
    TSpeedButton *SpeedButton97;
    TLabel *Label15;
    TTrackBar *TrackBar14;
    TPanel *Panel15;
    TSpeedButton *SpeedButton99;
    TSpeedButton *SpeedButton100;
    TSpeedButton *SpeedButton102;
    TSpeedButton *SpeedButton103;
    TSpeedButton *SpeedButton104;
    TLabel *Label16;
    TTrackBar *TrackBar15;
    TPanel *Panel16;
    TSpeedButton *SpeedButton106;
    TSpeedButton *SpeedButton107;
    TSpeedButton *SpeedButton109;
    TSpeedButton *SpeedButton110;
    TSpeedButton *SpeedButton111;
    TLabel *Label17;
    TTrackBar *TrackBar16;
    TPanel *Panel17;
    TSpeedButton *SpeedButton113;
    TSpeedButton *SpeedButton114;
    TSpeedButton *SpeedButton115;
    TSpeedButton *SpeedButton116;
    TSpeedButton *SpeedButton118;
    TLabel *Label18;
    TSpeedButton *SpeedButton119;
    TTrackBar *TrackBar17;
    TPanel *Panel18;
    TSpeedButton *SpeedButton120;
    TSpeedButton *SpeedButton121;
    TSpeedButton *SpeedButton122;
    TSpeedButton *SpeedButton123;
    TSpeedButton *SpeedButton125;
    TLabel *Label19;
    TSpeedButton *SpeedButton126;
    TTrackBar *TrackBar18;
    TPanel *Panel19;
    TSpeedButton *SpeedButton127;
    TSpeedButton *SpeedButton128;
    TSpeedButton *SpeedButton129;
    TSpeedButton *SpeedButton130;
    TSpeedButton *SpeedButton132;
    TLabel *Label20;
    TTrackBar *TrackBar19;
    TPanel *Panel20;
    TSpeedButton *SpeedButton134;
    TSpeedButton *SpeedButton135;
    TSpeedButton *SpeedButton136;
    TSpeedButton *SpeedButton137;
    TSpeedButton *SpeedButton139;
    TLabel *Label21;
    TTrackBar *TrackBar20;
    TPanel *Panel21;
    TSpeedButton *SpeedButton141;
    TSpeedButton *SpeedButton142;
    TSpeedButton *SpeedButton143;
    TSpeedButton *SpeedButton144;
    TSpeedButton *SpeedButton146;
    TLabel *Label22;
    TTrackBar *TrackBar21;
    TPanel *Panel22;
    TSpeedButton *SpeedButton148;
    TSpeedButton *SpeedButton149;
    TSpeedButton *SpeedButton150;
    TSpeedButton *SpeedButton151;
    TSpeedButton *SpeedButton153;
    TLabel *Label23;
    TTrackBar *TrackBar22;
    TPanel *Panel23;
    TSpeedButton *SpeedButton155;
    TSpeedButton *SpeedButton156;
    TSpeedButton *SpeedButton157;
    TSpeedButton *SpeedButton158;
    TSpeedButton *SpeedButton160;
    TLabel *Label24;
    TTrackBar *TrackBar23;
    TPanel *Panel24;
    TSpeedButton *SpeedButton162;
    TSpeedButton *SpeedButton163;
    TSpeedButton *SpeedButton164;
    TSpeedButton *SpeedButton165;
    TSpeedButton *SpeedButton167;
    TLabel *Label25;
    TTrackBar *TrackBar24;
    TPanel *Panel25;
    TSpeedButton *SpeedButton73;
    TSpeedButton *SpeedButton80;
    TSpeedButton *btnMixMute;
    TLabel *Label26;
    TTrackBar *TrackBar25;
    TSpeedButton *SpeedButton77;
    TSpeedButton *SpeedButton91;
    TSpeedButton *SpeedButton94;
    TSpeedButton *SpeedButton101;
    TSpeedButton *SpeedButton112;
    TSpeedButton *SpeedButton117;
    TSpeedButton *SpeedButton124;
    TSpeedButton *SpeedButton131;
    TSpeedButton *SpeedButton133;
    TSpeedButton *SpeedButton138;
    TSpeedButton *SpeedButton140;
    TSpeedButton *SpeedButton145;
    TSpeedButton *SpeedButton147;
    TSpeedButton *SpeedButton152;
    TSpeedButton *SpeedButton154;
    TSpeedButton *SpeedButton159;
    TSpeedButton *btnMAXON;
    TSpeedButton *SpeedButton168;
    TSpeedButton *SpeedButton169;
    TSpeedButton *SpeedButton170;
    TSpeedButton *SpeedButton171;
    TSpeedButton *SpeedButton172;
    TSpeedButton *SpeedButton173;
    TSpeedButton *SpeedButton174;
    TSpeedButton *SpeedButton175;
    TSpeedButton *SpeedButton108;
    TSpeedButton *btnPRIORITY;
    TLabel *Label27;
    TSpeedButton *btnMasterMute;
    TTrackBar *TrackBar26;
    TPanel *pnlDspDetail;
    TLabel *lblDSPInfo;
    TTrackBar *TrackBar27;
    TSpeedButton *SpeedButton84;
    TSpeedButton *SpeedButton98;
    TEdit *edtDebug;
    TLabel *Label28;
    TLabel *Label29;
    TLabel *Label30;
    TEdit *edtPB1;
    TEdit *edtPB17;
    TPaintBox *PaintBox1;
    TPanel *panelBandL;
    TEdit *edtFreqL;
    TEdit *edtGainL;
    TEdit *edtQL;
    TComboBox *cbTypeL;
    TCheckBox *cbBypassL;
    TPanel *panelBand1;
    TComboBox *cbType1;
    TEdit *edtFreq1;
    TEdit *edtGain1;
    TEdit *edtQ1;
    TCheckBox *cbBypass1;
    TPanel *panelBand2;
    TComboBox *cbType2;
    TEdit *edtFreq2;
    TEdit *edtGain2;
    TEdit *edtQ2;
    TCheckBox *cbBypass2;
    TPanel *panelBand3;
    TComboBox *cbType3;
    TEdit *edtFreq3;
    TEdit *edtGain3;
    TEdit *edtQ3;
    TCheckBox *cbBypass3;
    TPanel *panelBand4;
    TComboBox *cbType4;
    TEdit *edtFreq4;
    TEdit *edtGain4;
    TEdit *edtQ4;
    TCheckBox *cbBypass4;
    TPanel *panelBand5;
    TComboBox *cbType5;
    TEdit *edtFreq5;
    TEdit *edtGain5;
    TEdit *edtQ5;
    TCheckBox *cbBypass5;
    TPanel *panelBandH;
    TComboBox *cbTypeH;
    TEdit *edtFreqH;
    TEdit *edtGainH;
    TEdit *edtQH;
    TCheckBox *cbBypassH;
    TPanel *panelBand0;
    TComboBox *cbType0;
    TEdit *edtFreq0;
    TEdit *edtGain0;
    TEdit *edtQ0;
    TCheckBox *cbBypass0;
    TButton *Button1;
    void __fastcall FormCreate(TObject *Sender);
    void __fastcall FormDestroy(TObject *Sender);
    void __fastcall btnRefreshClick(TObject *Sender);
    void __fastcall udpSLPUDPRead(TObject *Sender, TStream *AData,
          TIdSocketHandle *ABinding);
    void __fastcall lvDeviceSelectItem(TObject *Sender, TListItem *Item,
          bool Selected);
    void __fastcall btnSelectClick(TObject *Sender);
    void __fastcall tmSLPTimer(TObject *Sender);
    void __fastcall TrackBar1Change(TObject *Sender);
    void __fastcall SpeedButton7Click(TObject *Sender);
    void __fastcall SpeedButton6Click(TObject *Sender);
    void __fastcall SpeedButton5Click(TObject *Sender);
    void __fastcall SpeedButton4Click(TObject *Sender);
    void __fastcall SpeedButton3Click(TObject *Sender);
    void __fastcall SpeedButton2Click(TObject *Sender);
    void __fastcall SpeedButton1Click(TObject *Sender);
    void __fastcall udpControlUDPRead(TObject *Sender, TStream *AData,
          TIdSocketHandle *ABinding);
    void __fastcall tmWatchTimer(TObject *Sender);
    void __fastcall SpeedButton118Click(TObject *Sender);
    void __fastcall SpeedButton116Click(TObject *Sender);
    void __fastcall SpeedButton119Click(TObject *Sender);
    void __fastcall SpeedButton126Click(TObject *Sender);
    void __fastcall SpeedButton115Click(TObject *Sender);
    void __fastcall SpeedButton114Click(TObject *Sender);
    void __fastcall SpeedButton113Click(TObject *Sender);
    void __fastcall TrackBar17Change(TObject *Sender);
    void __fastcall lvDeviceDblClick(TObject *Sender);
    void __fastcall SpeedButton77Click(TObject *Sender);
    void __fastcall SpeedButton80Click(TObject *Sender);
    void __fastcall btnMAXONClick(TObject *Sender);
    void __fastcall SpeedButton108Click(TObject *Sender);
    void __fastcall SpeedButton73Click(TObject *Sender);
    void __fastcall TrackBar26Change(TObject *Sender);
    void __fastcall btnMixMuteClick(TObject *Sender);
    void __fastcall btnMasterMuteClick(TObject *Sender);
    void __fastcall SpeedButton84Click(TObject *Sender);
    void __fastcall SpeedButton87Click(TObject *Sender);
    void __fastcall SpeedButton178Click(TObject *Sender);
    void __fastcall SpeedButton105Click(TObject *Sender);
    void __fastcall TrackBar27Change(TObject *Sender);
    void __fastcall FormMouseWheel(TObject *Sender, TShiftState Shift,
          int WheelDelta, TPoint &MousePos, bool &Handled);
    void __fastcall Button1Click(TObject *Sender);

private:
    // 已经选择设备
    bool flag_ip_selected;
    // 已经加载版本文件
    bool flag_file_open;

    String dst_ip;
    String local_ip;
    String local_broadcast_ip;

    String last_select_device_ip;

    TFileStream * log_file;
    void AppendLog(String);
private:
    TSpeedButton * last_default_btn;
    TSpeedButton * last_out_num_btn;
    TSpeedButton * last_dsp_btn;

    int mix_panel_state;

    void MsgWatchHandle(const T_iap_data_pack &);

    int low_freq;
    int low_gain;
    int high_freq;
    int high_gain;

    int eq_freq[8];
    int eq_gain[8];
    int eq_q[8];

    void SendCmd(D1608Cmd& cmd);
//----------------------------------
private:
    PanelAgent* panel_agent;
    PaintAgent* paint_agent;
    FilterSet filter_set;
public:		// User declarations
    __fastcall TForm1(TComponent* Owner);
    void SendCmd2(D1608Cmd2& cmd);
};
//---------------------------------------------------------------------------
extern PACKAGE TForm1 *Form1;
//---------------------------------------------------------------------------
#endif
