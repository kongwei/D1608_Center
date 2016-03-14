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
#include "AdvTrackBar.hpp"
#include <Graphics.hpp>
#include <ImgList.hpp>
#include "SpeedButtonNoFrame.h"
#include <Menus.hpp>

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
    TPanel *input_panel;
    TSpeedButtonNoFrame *input_panel_eq_btn;
    TSpeedButtonNoFrame *input_panel_comp_btn;
    TSpeedButtonNoFrame *input_panel_auto_btn;
    TSpeedButtonNoFrame *input_panel_invert_btn;
    TSpeedButtonNoFrame *input_panel_noise_btn;
    TSpeedButtonNoFrame *input_panel_mute_btn;
    TSpeedButtonNoFrame *input_panel_default_btn;
    TCheckBox *cbWatch;
    TTimer *tmWatch;
    TPanel *output_panel;
    TSpeedButtonNoFrame *output_panel_eq_btn;
    TSpeedButtonNoFrame *output_panel_limit_btn;
    TSpeedButtonNoFrame *output_panel_number_btn;
    TSpeedButtonNoFrame *output_panel_invert_btn;
    TSpeedButtonNoFrame *output_panel_mute_btn;
    TSpeedButtonNoFrame *SpeedButton119;
    TPanel *mix_panel;
    TSpeedButtonNoFrame *SpeedButton73;
    TSpeedButtonNoFrame *SpeedButton80;
    TSpeedButtonNoFrame *btnMixMute;
    TSpeedButtonNoFrame *input_panel_dsp_btn;
    TSpeedButtonNoFrame *btnMAXON;
    TSpeedButtonNoFrame *output_panel_dsp_btn;
    TSpeedButtonNoFrame *SpeedButton108;
    TSpeedButtonNoFrame *btnPRIORITY;
    TSpeedButtonNoFrame *btnMasterMute;
    TPanel *pnlDspDetail;
    TLabel *lblDSPInfo;
    TTrackBar *TrackBar27;
    TSpeedButtonNoFrame *SpeedButton84;
    TSpeedButtonNoFrame *SpeedButton98;
    TEdit *edtDebug;
    TLabel *Label28;
    TLabel *Label29;
    TLabel *Label30;
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
    TIdUDPServer *IdUDPServer1;
    TTimer *Timer1;
    TComboBox *cbPreset;
    TLabel *Label31;
    TPanel *watch_panel;
    TImage *watch_bkimage;
    TLabel *label_watch;
    TProgressBar *pb_watch;
    TImageList *ImageList1;
    TImage *input_panel_bkground;
    TAdvTrackBar *input_panel_trackbar;
    TAdvTrackBar *output_panel_trackbar;
    TImage *output_panel_bkground;
    TEdit *input_panel_level_edit;
    TEdit *output_panel_level_edit;
    TStaticText *input_panel_dsp_num;
    TStaticText *output_panel_dsp_num;
    TAdvTrackBar *AdvTrackBar1;
    TAdvTrackBar *AdvTrackBar2;
    TImage *Image1;
    TEdit *mix_panel_level_edit;
    TEdit *master_panel_level_edit;
    TStaticText *StaticText1;
    TStaticText *StaticText2;
    TPanel *Panel1;
    TImage *Image2;
    TProgressBar *ProgressBar1;
    TLabel *input_type;
    TPopupMenu *PopupMenu1;
    TMenuItem *M11;
    TMenuItem *M21;
    TMenuItem *M31;
    TMenuItem *M41;
    void __fastcall FormCreate(TObject *Sender);
    void __fastcall FormDestroy(TObject *Sender);
    void __fastcall btnRefreshClick(TObject *Sender);
    void __fastcall udpSLPUDPRead(TObject *Sender, TStream *AData,
          TIdSocketHandle *ABinding);
    void __fastcall lvDeviceSelectItem(TObject *Sender, TListItem *Item,
          bool Selected);
    void __fastcall btnSelectClick(TObject *Sender);
    void __fastcall tmSLPTimer(TObject *Sender);
    void __fastcall InputVolumeChange(TObject *Sender);
    void __fastcall ToogleMute(TObject *Sender);
    void __fastcall ToogleNoise(TObject *Sender);
    void __fastcall ToogleInvert(TObject *Sender);
    void __fastcall ToogleDefault(TObject *Sender);
    void __fastcall ToogleAuto(TObject *Sender);
    void __fastcall ToogleCOMP(TObject *Sender);
    void __fastcall ToogleEQ(TObject *Sender);
    void __fastcall udpControlUDPRead(TObject *Sender, TStream *AData,
          TIdSocketHandle *ABinding);
    void __fastcall tmWatchTimer(TObject *Sender);
    void __fastcall ToogleOutputMute(TObject *Sender);
    void __fastcall ToogleOutputInvert(TObject *Sender);
    void __fastcall SpeedButton119Click(TObject *Sender);
    void __fastcall ToogleDO(TObject *Sender);
    void __fastcall ToogleOutputMix(TObject *Sender);
    void __fastcall ToogleLimit(TObject *Sender);
    void __fastcall ToogleOutputEQ(TObject *Sender);
    void __fastcall OutputVolumeChange(TObject *Sender);
    void __fastcall lvDeviceDblClick(TObject *Sender);
    void __fastcall ToggleDSP(TObject *Sender);
    void __fastcall SpeedButton80Click(TObject *Sender);
    void __fastcall btnMAXONClick(TObject *Sender);
    void __fastcall SpeedButton108Click(TObject *Sender);
    void __fastcall SpeedButton73Click(TObject *Sender);
    void __fastcall MasterVolumeChange(TObject *Sender);
    void __fastcall btnMixMuteClick(TObject *Sender);
    void __fastcall btnMasterMuteClick(TObject *Sender);
    void __fastcall SpeedButton84Click(TObject *Sender);
    void __fastcall ToogleLowShelf(TObject *Sender);
    void __fastcall ToogleHighShelf(TObject *Sender);
    void __fastcall ToogleEQ_DSP(TObject *Sender);
    void __fastcall TrackBar27Change(TObject *Sender);
    void __fastcall FormMouseWheel(TObject *Sender, TShiftState Shift,
          int WheelDelta, TPoint &MousePos, bool &Handled);
    void __fastcall input_typeMouseDown(TObject *Sender,
          TMouseButton Button, TShiftState Shift, int X, int Y);
    void __fastcall M41Click(TObject *Sender);
    void __fastcall M41DrawItem(TObject *Sender, TCanvas *ACanvas,
          TRect &ARect, bool Selected);
    void __fastcall M41MeasureItem(TObject *Sender, TCanvas *ACanvas,
          int &Width, int &Height);

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
    TProgressBar * pb_watch_list[24];
    TEdit* input_level_edit[17];
    TEdit* output_level_edit[16];
};
//---------------------------------------------------------------------------
extern PACKAGE TForm1 *Form1;
//---------------------------------------------------------------------------
#endif
