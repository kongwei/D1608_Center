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
#include <ActnList.hpp>
#include <Graphics.hpp>
#include <ImgList.hpp>
#include <Menus.hpp>
#include <Graphics.hpp>
#include <ImgList.hpp>
#include <Menus.hpp>
#include "SpeedButtonNoFrame.h"
#include "WaveGraphic.h"
#include "FilterPanelSet.h"
#include "FilterList.h"
#include "AdvTrackBar.hpp"
#include "CSPIN.h"
#include <ToolWin.hpp>
#include <Grids.hpp>
#include <ValEdit.hpp>
#include <CheckLst.hpp>
extern "C"{
#include "../enc28j60_iap_app/inc/D1608Pack.h"
}

#define UDP_PORT 65518
#define TCP_PORT 15288
#define MAX_PACKAGE_SIZE 1024

#define FIRST_FILTER 1
#define LAST_FILTER 8
#define HP_FILTER 1
#define LP_FILTER 11

#pragma pack(1)
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
    TTimer *tmWatch;
    TPanel *mix_panel;
    TSpeedButtonNoFrame *btnFShift;
    TSpeedButtonNoFrame *btnMixInvert;
    TSpeedButtonNoFrame *btnMixMute;
    TSpeedButtonNoFrame *btnMAXON;
    TSpeedButtonNoFrame *btnLastOn;
    TSpeedButtonNoFrame *btnPRIORITY;
    TSpeedButtonNoFrame *btnMasterMute;
    TPanel *pnlDspDetail;
    TLabel *lblDSPInfo;
    TEdit *edtDebug;
    TLabel *Label31;
    TPanel *watch_panel;
    TImage *watch_bkimage;
    TLabel *label_watch;
    TImageList *ImageList1bak;
    TAdvTrackBar *mix_panel_trackbar;
    TAdvTrackBar *master_panel_trackbar;
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
    TSpeedButtonNoFrame *input_panel_noise_btn;
    TSpeedButtonNoFrame *input_panel_mute_btn;
    TEdit *input_panel_level_edit;
    TSpeedButtonNoFrame *input_panel_invert_btn;
    TSpeedButtonNoFrame *input_panel_eq_btn;
    TSpeedButtonNoFrame *input_panel_dsp_btn;
    TSpeedButtonNoFrame *input_panel_default_btn;
    TSpeedButtonNoFrame *input_panel_comp_btn;
    TImage *input_panel_bkground;
    TSpeedButtonNoFrame *input_panel_auto_btn;
    TStaticText *input_panel_dsp_num;
    TImage *Image3;
    TAdvTrackBar *output_panel_trackbar;
    TImage *output_panel_bkground;
    TSpeedButtonNoFrame *output_panel_mute_btn;
    TSpeedButtonNoFrame *output_panel_invert_btn;
    TSpeedButtonNoFrame *SpeedButton119;
    TSpeedButtonNoFrame *output_panel_number_btn;
    TSpeedButtonNoFrame *output_panel_comp_btn;
    TSpeedButtonNoFrame *output_panel_eq_btn;
    TSpeedButtonNoFrame *output_panel_dsp_btn;
    TEdit *output_panel_level_edit;
    TStaticText *output_panel_dsp_num;
    TLabel *Label30;
    TLabel *Label29;
    TLabel *Label28;
    TLabel *Label2;
    TSpeedButtonNoFrame *btnPhanton;
    TSpeedButtonNoFrame *SpeedButtonNoFrame1;
    TSpeedButtonNoFrame *btnDSPCOMP;
    TLabel *Label3;
    TPaintBox *PaintBox1;
    TPanel *pnlComp;
    TPanel *panelBand9;
    TLabel *Label4;
    TComboBox *cbType9;
    TEdit *edtFreq9;
    TEdit *edtGain9;
    TEdit *edtQ9;
    TCheckBox *cbBypass9;
    TAdvTrackBar *AdvTrackBar3;
    TPanel *panelBand1;
    TLabel *Label11;
    TLabel *Label12;
    TEdit *edtFreq1;
    TComboBox *cbType1;
    TPanel *panelBand2;
    TLabel *Label5;
    TComboBox *cbType2;
    TEdit *edtFreq2;
    TEdit *edtGain2;
    TEdit *edtQ2;
    TCheckBox *cbBypass2;
    TPanel *panelBand3;
    TLabel *Label6;
    TComboBox *cbType3;
    TEdit *edtFreq3;
    TEdit *edtGain3;
    TEdit *edtQ3;
    TCheckBox *cbBypass3;
    TPanel *panelBand4;
    TLabel *Label7;
    TComboBox *cbType4;
    TEdit *edtFreq4;
    TEdit *edtGain4;
    TEdit *edtQ4;
    TCheckBox *cbBypass4;
    TPanel *panelBand5;
    TLabel *Label8;
    TComboBox *cbType5;
    TEdit *edtFreq5;
    TEdit *edtGain5;
    TEdit *edtQ5;
    TCheckBox *cbBypass5;
    TPanel *panelBand6;
    TLabel *Label9;
    TComboBox *cbType6;
    TEdit *edtFreq6;
    TEdit *edtGain6;
    TEdit *edtQ6;
    TCheckBox *cbBypass6;
    TPanel *panelBand7;
    TLabel *Label10;
    TComboBox *cbType7;
    TEdit *edtFreq7;
    TEdit *edtGain7;
    TEdit *edtQ7;
    TCheckBox *cbBypass7;
    TPanel *panelBand8;
    TLabel *Label15;
    TComboBox *cbType8;
    TEdit *edtFreq8;
    TEdit *edtGain8;
    TEdit *edtQ8;
    TCheckBox *cbBypass8;
    TPanel *panelBand0;
    TLabel *Label16;
    TComboBox *cbType0;
    TEdit *edtFreq0;
    TEdit *edtGain0;
    TEdit *edtQ0;
    TCheckBox *cbBypass0;
    TSpeedButton *btnDspResetEQ;
    TMenuItem *N22dBu1;
    TMenuItem *N24dBu1;
    TPopupMenu *PopupMenu2;
    TMenuItem *MenuItem3;
    TMenuItem *MenuItem4;
    TMenuItem *MenuItem5;
    TMenuItem *MenuItem6;
    TCheckBox *cbBypass1;
    TEdit *edtQ1;
    TEdit *edtGain1;
    TPanel *panelBand10;
    TLabel *Label13;
    TEdit *edtFreq10;
    TCheckBox *cbBypass10;
    TLabel *Label14;
    TComboBox *cbType10;
    TEdit *edtQ10;
    TEdit *edtGain10;
    TMemo *mmCoeff;
    TAdvTrackBar *TrackBar27;
    TAdvTrackBar *p_output_inner_level;
    TAdvTrackBar *p_input_inner_level;
    TPaintBox *pb_watch;
    TAdvTrackBar *input_panel_trackbar;
    TPanel *pnlMix;
    TImage *pnlmix_background;
    TSpeedButtonNoFrame *pnlmix_mute;
    TEdit *pnlmix_level_edit;
    TAdvTrackBar *pnlmix_level_trackbar;
    TStaticText *pnlmix_dsp_num;
    TTabSheet *TabSheet1;
    TMemo *memo_debug;
    TImageList *ImageList1;
    TButton *btnRecall;
    TOpenDialog *OpenDialog1;
    TSaveDialog *SaveDialog1;
    TPopupMenu *PopupMenu3;
    TMenuItem *SavePreset;
    TMenuItem *LoadPreset;
    TMenuItem *N1;
    TMenuItem *SaveAllPreset;
    TMenuItem *LoadAllPreset;
    TMenuItem *SaveAllPresetAs;
    TMenuItem *N2;
    TMenuItem *Store;
    TMenuItem *StoreAs;
    TMenuItem *N11;
    TMenuItem *N21;
    TMenuItem *N31;
    TMenuItem *N41;
    TMenuItem *N51;
    TMenuItem *N61;
    TMenuItem *N71;
    TMenuItem *N81;
    TMenuItem *Recall1;
    TMenuItem *N12;
    TMenuItem *N22;
    TMenuItem *N32;
    TMenuItem *N42;
    TMenuItem *N52;
    TMenuItem *N62;
    TMenuItem *N72;
    TMenuItem *N82;
    TEdit *edtPreset;
    TToolBar *ToolBar1;
    TToolButton *ToolButton1;
    TToolButton *ToolButton2;
    TToolButton *cbWatch;
    TToolButton *divbase;
    TToolButton *ToolButton5;
    TEdit *edtIp;
    TButton *btnSetIp;
    TValueListEditor *ValueListEditor1;
    TValueListEditor *ValueListEditor2;
    TLabel *Label17;
    TLabel *Label18;
    TLabel *lblDiff;
    TToolButton *tbGlobalDspName;
    TListView *lvLog;
    TButton *btnGetLog;
    TButton *btnGetDebug;
    TListView *lvDebug;
    TEdit *edtPresetName;
    TSpeedButton *btnModifyPresetName;
    TToolButton *btnDeviceName;
    TCheckListBox *clbAvaliablePreset;
    TButton *btnSetLock;
    TLabel *Label22;
    TEdit *edtPassword;
    TEdit *edtKeyPassword;
    TEdit *edtLockedString;
    TLabel *Label24;
    TEdit *edtUnlockPassword;
    TButton *btnUnlock;
    TCheckBox *cbRunningTimer;
    TCheckBox *cbRebootCount;
    TCheckBox *cbLockedString;
    TLabel *Label19;
    TEdit *edtRunningTimer;
    TEdit *edtRebootCount;
    TSpeedButton *btnKeyPasswordUp;
    TSpeedButton *btnKeyPasswordDown;
    TBevel *Bevel2;
    TBevel *Bevel3;
    TButton *btnUnlockExt;
    TButton *btnLeaveTheFactory;
    TButton *btnDownloadPreset;
    TTabSheet *TabSheet2;
    TPanel *Panel2;
    TTrackBar *tbRatio;
    TTrackBar *tbThreshold;
    TTrackBar *tbRelease;
    TTrackBar *tbAttack;
    TLabel *Label20;
    TLabel *Label21;
    TLabel *Label23;
    TLabel *Label25;
    TLabel *Label27;
    TLabel *Label32;
    TLabel *Label33;
    TButton *btnSetComp;
    TEdit *edtThreshold;
    TEdit *edtAttack;
    TEdit *edtRelease;
    TEdit *edtRatio;
    TEdit *edtInput;
    TLabel *Label26;
    TLabel *Label34;
    TLabel *Label35;
    TLabel *Label36;
    TLabel *Label37;
    TEdit *edtCompRatio;
    TEdit *edtCompThreshold;
    TEdit *edtCompAttackTime;
    TEdit *edtCompReleaseTime;
    TEdit *edtCompGain;
    TLabel *Label38;
    TLabel *Label39;
    TLabel *Label40;
    TLabel *Label41;
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
    void __fastcall btnMixInvertClick(TObject *Sender);
    void __fastcall btnMAXONClick(TObject *Sender);
    void __fastcall btnLastOnClick(TObject *Sender);
    void __fastcall btnFShiftClick(TObject *Sender);
    void __fastcall MasterVolumeChange(TObject *Sender);
    void __fastcall btnMixMuteClick(TObject *Sender);
    void __fastcall btnMasterMuteClick(TObject *Sender);
    void __fastcall btnPhantonClick(TObject *Sender);
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
    void __fastcall btnDspResetEQClick(TObject *Sender);
    void __fastcall MenuItem3Click(TObject *Sender);
    void __fastcall input_panel_level_editKeyDown(TObject *Sender,
          WORD &Key, TShiftState Shift);
    void __fastcall output_panel_level_editKeyDown(TObject *Sender,
          WORD &Key, TShiftState Shift);
    void __fastcall input_panel_level_editExit(TObject *Sender);
    void __fastcall output_panel_level_editExit(TObject *Sender);
    void __fastcall io_panel_trackbarKeyDown(TObject *Sender, WORD &Key,
          TShiftState Shift);
    void __fastcall WatchPaint(TObject *Sender);
    void __fastcall input_panel_level_editClick(TObject *Sender);
    void __fastcall input_panel_dsp_numClick(TObject *Sender);
    void __fastcall output_panel_dsp_numClick(TObject *Sender);
    void __fastcall input_panel_trackbarEnter(TObject *Sender);
    void __fastcall input_panel_trackbarExit(TObject *Sender);
    void __fastcall master_panel_level_editExit(TObject *Sender);
    void __fastcall master_panel_level_editKeyDown(TObject *Sender,
          WORD &Key, TShiftState Shift);
    void __fastcall FormClick(TObject *Sender);
    void __fastcall FormMouseDown(TObject *Sender, TMouseButton Button,
          TShiftState Shift, int X, int Y);
    void __fastcall pnlmix_level_trackbarChange(TObject *Sender);
    void __fastcall pnlmix_level_editExit(TObject *Sender);
    void __fastcall pnlmix_level_editKeyDown(TObject *Sender, WORD &Key,
          TShiftState Shift);
    void __fastcall pnlmix_muteClick(TObject *Sender);
    void __fastcall btnSavePresetToFileClick(TObject *Sender);
    void __fastcall btnLoadPresetFromFileClick(TObject *Sender);
    void __fastcall SaveAllPresetClick(TObject *Sender);
    void __fastcall LoadAllPresetClick(TObject *Sender);
    void __fastcall FormCloseQuery(TObject *Sender, bool &CanClose);
    void __fastcall SaveAllPresetAsClick(TObject *Sender);
    void __fastcall StoreClick(TObject *Sender);
    void __fastcall StoreAsClick(TObject *Sender);
    void __fastcall RecallClick(TObject *Sender);
    void __fastcall ToolButton1Click(TObject *Sender);
    void __fastcall ToolButton2Click(TObject *Sender);
    void __fastcall btnSetIpClick(TObject *Sender);
    void __fastcall tbGlobalDspNameClick(TObject *Sender);
    void __fastcall btnGetLogClick(TObject *Sender);
    void __fastcall btnGetDebugClick(TObject *Sender);
    void __fastcall btnModifyPresetNameClick(TObject *Sender);
    void __fastcall btnDeviceNameClick(TObject *Sender);
    void __fastcall clbAvaliablePresetClickCheck(TObject *Sender);
    void __fastcall btnSetLockClick(TObject *Sender);
    void __fastcall btnUnlockClick(TObject *Sender);
    void __fastcall cbRunningTimerClick(TObject *Sender);
    void __fastcall cbRebootCountClick(TObject *Sender);
    void __fastcall cbLockedStringClick(TObject *Sender);
    void __fastcall edtRunningTimerExit(TObject *Sender);
    void __fastcall edtRebootCountExit(TObject *Sender);
    void __fastcall edtRebootCountKeyDown(TObject *Sender, WORD &Key,
          TShiftState Shift);
    void __fastcall edtPasswordKeyPress(TObject *Sender, char &Key);
    void __fastcall edtKeyPasswordKeyDown(TObject *Sender, WORD &Key,
          TShiftState Shift);
    void __fastcall btnKeyPasswordUpClick(TObject *Sender);
    void __fastcall btnKeyPasswordDownClick(TObject *Sender);
    void __fastcall edtKeyPasswordKeyPress(TObject *Sender, char &Key);
    void __fastcall btnUnlockExtClick(TObject *Sender);
    void __fastcall btnLeaveTheFactoryClick(TObject *Sender);
    void __fastcall btnDownloadPresetClick(TObject *Sender);
    void __fastcall tbRatioChange(TObject *Sender);
    void __fastcall tbThresholdChange(TObject *Sender);
    void __fastcall tbReleaseChange(TObject *Sender);
    void __fastcall tbAttackChange(TObject *Sender);
    void __fastcall btnSetCompClick(TObject *Sender);
    void __fastcall edtRatioKeyDown(TObject *Sender, WORD &Key,
          TShiftState Shift);
    void __fastcall btnInputCancelClick(TObject *Sender);
    void __fastcall btnInputOKClick(TObject *Sender);
    void __fastcall edtInputExit(TObject *Sender);
    void __fastcall edtInputKeyDown(TObject *Sender, WORD &Key,
          TShiftState Shift);
    void __fastcall edtCompRatioKeyDown(TObject *Sender, WORD &Key,
          TShiftState Shift);
    void __fastcall SpeedButtonNoFrame1Click(TObject *Sender);
    void __fastcall edtCompThresholdKeyDown(TObject *Sender, WORD &Key,
          TShiftState Shift);
    void __fastcall edtCompAttackTimeKeyDown(TObject *Sender, WORD &Key,
          TShiftState Shift);
    void __fastcall edtCompReleaseTimeKeyDown(TObject *Sender, WORD &Key,
          TShiftState Shift);
    void __fastcall edtCompGainKeyDown(TObject *Sender, WORD &Key,
          TShiftState Shift);
    void __fastcall edtCompRatioExit(TObject *Sender);
    void __fastcall edtCompRatioClick(TObject *Sender);

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

    void MsgWatchHandle(const D1608Cmd& cmd);

    int low_freq;
    int low_gain;
    int high_freq;
    int high_gain;

    int eq_freq[11];
    int eq_gain[11];
    int eq_q[11];
public:
    void SendCmd(D1608Cmd& cmd);
//----------------------------------
private:
    PanelAgent* panel_agent;
    PaintAgent* paint_agent;
    FilterSet filter_set;

    int cur_preset_id;
    void __fastcall SetPresetId(int id)
    {
        cur_preset_id = id;
        edtPreset->Text = IntToStr(cur_preset_id);
    }
    int mireg0;

    void __fastcall ApplyConfigToUI();

    String preset_lib_filename;
    void __fastcall SetPresetLibFilename(String filename);
    bool file_dirty;
    void __fastcall SetFileDirty(bool dirty_flag);

    void __fastcall UpdateCaption();

public:		// User declarations
    __fastcall TForm1(TComponent* Owner);
    TPaintBox * pb_watch_list[32];

    TLabel* input_type_lbl[17];
    TSpeedButton* input_eq_btn[17];
    TSpeedButton* input_comp_btn[17];
    TSpeedButton* input_auto_btn[17];
    TSpeedButton* input_default_btn[17];
    TSpeedButton* input_invert_btn[17];
    TSpeedButton* input_noise_btn[17];
    TSpeedButton* input_mute_btn[17];
    TEdit* input_level_edit[17];
    TAdvTrackBar* input_level_trackbar[17];
    TStaticText* input_dsp_name[17];

    TLabel* output_type_lbl[16];
    TSpeedButton* output_eq_btn[16];
    TSpeedButton* output_comp_btn[16];
    TSpeedButton* output_invert_btn[16];
    TSpeedButton* output_mute_btn[16];
    TEdit* output_level_edit[16];
    TAdvTrackBar* output_level_trackbar[16];
    TStaticText* output_dsp_name[17];

    TSpeedButton* mix_mute_btn[17];
    TEdit* mix_level_edit[17];
    TAdvTrackBar* mix_level_trackbar[17];

private:
    long running_timer;
    int roboot_count;

    int keep_live_count;

    void ShowInputPanel(TControl * Sender, TNotifyEvent event, String default_text);
    TControl * inputObject;
    TNotifyEvent input_event;
    void __fastcall after_input_panel_dsp_numClick(TObject *Sender);
    void __fastcall after_output_panel_dsp_numClick(TObject *Sender);
};
//---------------------------------------------------------------------------
extern PACKAGE TForm1 *Form1;
//---------------------------------------------------------------------------
#endif
