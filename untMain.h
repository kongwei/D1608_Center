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
#include <jpeg.hpp>
#include <Chart.hpp>
#include <Series.hpp>
#include <TeEngine.hpp>
#include <TeeProcs.hpp>
#include "CGAUGES.h"
#include "D1608Pack.h"
#include "untFlashReader.h"

#define UDP_PORT 65518
#define TCP_PORT 15288
#define MAX_PACKAGE_SIZE 1024

#pragma pack(1)
    struct T_iap_start_pack
    {
        char op[9];
        __int32 sfilelen;
    };
#pragma pack()

//---------------------------------------------------------------------------
class TForm1 : public TForm
{
__published:	// IDE-managed Components
    TIdUDPServer *udpSLP;
    TTimer *tmSLP;
    TIdUDPServer *udpControl;
    TTimer *tmWatch;
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
    TImage *imgWatch;
    TLabel *label_watch;
    TImageList *ImageList1bak;
    TAdvTrackBar *mix_panel_trackbar;
    TAdvTrackBar *master_panel_trackbar;
    TImage *imgMasterMixBg;
    TEdit *mix_panel_level_edit;
    TEdit *master_panel_level_edit;
    TStaticText *mix_panel_dsp_num;
    TStaticText *master_panel_dsp_num;
    TLabel *input_type;
    TPopupMenu *PopupMenu1;
    TMenuItem *iMIC;
    TMenuItem *M21;
    TMenuItem *M31;
    TMenuItem *i10dBv;
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
    TImage *imgInputTemplate;
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
    TSpeedButtonNoFrame *btnDspEq;
    TSpeedButtonNoFrame *btnDspComp;
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
    TAdvTrackBar *dsp_delay_trackbar;
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
    TMenuItem *i22dBu;
    TMenuItem *i24dBu;
    TPopupMenu *PopupMenu2;
    TMenuItem *MenuItem3;
    TMenuItem *o10dBv;
    TMenuItem *o22dBu;
    TMenuItem *o24dBu;
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
    TAdvTrackBar *dsp_gain_trackbar;
    TPaintBox *pb_watch;
    TPanel *pnlMix;
    TImage *pnlmix_background;
    TSpeedButtonNoFrame *pnlmix_mute;
    TEdit *pnlmix_level_edit;
    TAdvTrackBar *pnlmix_level_trackbar;
    TStaticText *pnlmix_dsp_num;
    TMemo *memo_debug;
    TImageList *ImageList1;
    TOpenDialog *OpenDialog1;
    TSaveDialog *SaveDialog1;
    TPopupMenu *PopupMenu3;
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
    TEdit *edtIp;
    TSpeedButton *btnSetIp;
    TLabel *lblDiff;
    TListView *lvLog;
    TButton *btnGetLog;
    TButton *btnGetDebug;
    TListView *lvDebug;
    TCheckListBox *clbAvaliablePreset;
    TBevel *Bevel2;
    TBevel *Bevel3;
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
    TLabel *lbl5Vd;
    TLabel *lbl12Va;
    TLabel *lbl16Vac;
    TLabel *lbl_16Vac;
    TLabel *lbl8Vac;
    TLabel *lbl48Vp;
    TLabel *Label48;
    TLabel *Label49;
    TLabel *lbl5mAd;
    TLabel *lbl12mAa;
    TLabel *lbl16mAa;
    TLabel *lbl_16mAa;
    TLabel *lbl8mAa;
    TLabel *lbl46mAa;
    TLabel *lbl3_3Vd;
    TLabel *lbl5Va;
    TLabel *lbl8Vdc;
    TLabel *lbl3_3V;
    TLabel *lbl_12Va;
    TLabel *lbl3_3mAd;
    TLabel *lbl5mAa;
    TLabel *lbl8mAd;
    TLabel *lbl3_3mA;
    TLabel *lbl_12mAa;
    TSpeedButton *btnDeviceName;
    TBevel *Bevel4;
    TBevel *Bevel5;
    TLabel *Label43;
    TBevel *Bevel6;
    TBevel *Bevel7;
    TPanel *pnlHeader;
    TImage *imgHeader;
        TSpeedButtonNoFrame *SpeedButtonNoFrame2;
    TSpeedButtonNoFrame *btnMonitor;
        TSpeedButtonNoFrame *SpeedButtonNoFrame4;
    TAdvTrackBar *input_panel_trackbar;
    TChart *Chart1;
    TLineSeries *Series1;
    TLineSeries *lineDownLimit;
    TLineSeries *lineUpLimit;
    TShape *shape_active_adc;
    TTimer *tmLed;
    TShape *shape_live;
    TImage *imgBody;
    TPanel *pnlOperator;
    TPanel *pnlMonitor;
    TPanel *pnlSystem;
    TPanel *pnlMist;
    TPanel *Panel3;
    TPaintBox *PaintBox2;
    TBevel *Bevel8;
    TPaintBox *PaintBox3;
    TLabel *lblDeviceName;
    TLabel *lblDeviceInfo;
    TShape *shape_link;
    TShape *shape_power;
    TBevel *Bevel9;
    TPaintBox *PaintBox4;
    TImage *imgSystemBg2;
    TImage *imgSystemPresetBg;
    TImage *imgOutputTemplate;
    TImage *imgMasterMix;
    TProgressBar *ProgressBar1;
    TImage *imgPresetBg;
    TImage *imgPreset;
    TEdit *edtPreset;
    TLabel *lblPresetName;
    TPaintBox *pbComp;
    TImage *imgDspGainBtnBg;
    TImage *imgDspDelayBtnBg;
    TEdit *dsp_gain_edit;
    TEdit *dsp_delay_edit;
    TImage *imgWatchLevelBg;
    TPanel *Panel1;
    TPanel *iDsp1;
    TPanel *iDsp2;
    TPanel *iDsp3;
    TPanel *iDsp4;
    TPanel *iDsp5;
    TPanel *iDsp6;
    TPanel *iDsp7;
    TPanel *iDsp8;
    TPanel *iAD_abcd;
    TPanel *iAD_ijkl;
    TPanel *iDA_1234;
    TPanel *iDA_9_12;
    TPanel *iAD_efgh;
    TPanel *iAD_mnop;
    TPanel *iDA_5678;
    TPanel *iDA_13_16;
    TCheckBox *cbCompAutoTime;
    TLabel *Label47;
    TListView *lvDevice;
    TLabel *Label50;
    TBevel *Bevel10;
    TEdit *edtDeviceType;
    TRadioButton *rbStaticIpEnabled;
    TRadioButton *rbDhcpEnabled;
    TCheckBox *cbGlobalDspName;
    TCheckBox *cbPresetAutoSaved;
    TLabel *Label42;
    TEdit *edtDeviceName;
    TImage *Image1;
    TSpeedButton *btnRebootDevice;
    TLabel *Label44;
    TPageControl *PageControl1;
    TTabSheet *TabSheet1;
    TTabSheet *TabSheet2;
    TTabSheet *TabSheet3;
    TTabSheet *TabSheet5;
    TLabel *lblDeviceRunningTime;
    TLabel *Label1;
    TLabel *Label20;
    TLabel *Label21;
    TComboBox *cbMenuKeyFunction;
    TComboBox *cbUpKeyFunction;
    TComboBox *cbDownKeyFunction;
    TCheckBox *cbLockUpDownMenuKey;
    TTimer *tmDelayUpdateUI;
    TButton *btnClearDebug;
    TActionList *ActionList1;
    TAction *s1;
    TAction *s2;
    TAction *s3;
    TAction *s4;
    TAction *s5;
    TAction *s6;
    TAction *s7;
    TAction *s8;
    TAction *r1;
    TAction *r2;
    TAction *r3;
    TAction *r4;
    TAction *r5;
    TAction *r6;
    TAction *r7;
    TAction *r8;
    TLabel *Label23;
    TLabel *Label25;
    TTimer *tmDelayBackup;
    TSpeedButtonNoFrame *btnSaveFlashToFile;
    TSpeedButtonNoFrame *btnLoadFileToFlash;
    TProgressBar *pbBackup;
    TPopupMenu *pmPresetSaveLoad;
    TMenuItem *SavePresetAs;
    TMenuItem *LoadPreset;
    TSpeedButton *btnResetAllConfig;
    TImage *Image3;
    TLabel *Label19;
    TLabel *Label22;
    TImage *imgSystemBg4;
    TImage *imgSystemBg5;
    TImage *imgSystemBg6;
    TImage *imgSystemBg7;
    TImage *imgSystemBg1;
    TSpeedButton *btnKeyPasswordUp;
    TSpeedButton *btnKeyPasswordDown;
    TSpeedButton *btnSetLock;
    TEdit *edtRebootCount;
    TEdit *edtRunningTimer;
    TCheckBox *cbLockedString;
    TCheckBox *cbRebootCount;
    TCheckBox *cbRunningTimer;
    TEdit *edtLockedString;
    TEdit *edtKeyPassword;
    TEdit *edtPassword;
    TLabel *Label27;
    TEdit *edtRemainTime;
    TImage *Image2;
    TLabel *Label32;
    TEdit *edtRemainRebootCount;
    TImage *Image4;
    TEdit *edtLockedString1;
    TImage *Image5;
    TLabel *Label33;
    TLabel *Label24;
    TEdit *edtUnlockPassword;
    TImage *imgSystemBg3;
    TSpeedButton *btnUnlock;
    TEdit *edtStartBuildTime;
    TSpeedButton *btnResetKeyFunction;
    TSpeedButton *btnClearAllPreset;
    TLabel *lblPresetFileName;
    TImage *Image6;
    TImage *img_input_gain_trackbar;
    TImage *img_output_gain_trackbar;
    TTimer *tmDelaySendCmd;
    TButton *btnSaveLog;
    TSpeedButton *btnUnlockExt;
    TSpeedButton *btnLeaveTheFactory;
    TEdit *edtMAC;
    TImage *Image7;
    TPanel *iLed;
    TMenuItem *N1;
    TMenuItem *Paste1;
    TImage *imgLogo;
    TLabel *lblVersion;
    TButton *btnSelect;
    TButton *btnRefresh;
    TCheckBox *cbLedTest;
    TAction *aStore;
    TIdUDPServer *udpSLP1;
    TIdUDPServer *udpSLP2;
    TLabel *lblLogCount;
    TCheckBox *cbLanDebugLed;
    TCheckBox *cbLanDebugOled;
    TEdit *edtEventId;
    TEdit *edtEventData;
    TButton *btnInsertUserLog;
    TLabel *lblDeviceRunningTime2;
    TButton *Button1;
    TButton *btnDisconnect;
    TLabel *lblCpuId;
    TLabel *lblSn;
    TLabel *lblConfigFilename;
    TButton *btnCopyVoteDataToClip;
    TMemo *mmVoteOrg;
    TSpeedButton *btnClearDataAndTime;
    TEdit *edtDeviceFullName;
    TButton *btnCopyDebugLog;
    TButton *btnCutDebugLog;
    TLabel *lblCtrlPort;
    TButton *btnDebugInfoEx;
    TMemo *memo_debug_ex;
    TPanel *Panel2;
    TLabel *lblKeepLiveCheck;
    TLabel *Label17;
    TSpeedButtonNoFrame *SpeedButtonNoFrame1;
    TSpeedButtonNoFrame *SpeedButtonNoFrame3;
    TRadioGroup *rgLedTest;
    TCheckBox *cbUsart1ReceiveAck;
    TCheckBox *cbUsart3ReceiveAck;
    TButton *btnStopDebugInfoEx;
    TButton *Button2;
    TButton *Button3;
    TEdit *edtDebufExPort;
    TCheckBox *cbLogEnabled;
    TMenuItem *Copy1;
    TTimer *tmProcessReply;
    TMemo *mmVote;
    TLabel *lblMaster;
    TPaintBox *input_panel_thumb;
    TPaintBox *output_panel_thumb;
    TEdit *edt4Tab;
    TEdit *Edit2;
    TEdit *Edit3;
    TImage *Image8;
    TEdit *edtAdminPassword;
    TSpeedButton *btnAdminPassword;
    TLabel *Label18;
    TCheckBox *cbLockParameter;
    TImage *imgMask;
    void __fastcall FormCreate(TObject *Sender);
    void __fastcall btnRefreshClick(TObject *Sender);
    void __fastcall udpSLPUDPRead(TObject *Sender, TStream *AData,
          TIdSocketHandle *ABinding);
    void __fastcall btnSelectClick(TObject *Sender);
    void __fastcall tmSLPTimer(TObject *Sender);
    void __fastcall InputVolumeChange(TObject *Sender);
    void __fastcall ToogleMute(TObject *Sender);
    void __fastcall ToogleNoise(TObject *Sender);
    void __fastcall ToogleInvert(TObject *Sender);
    void __fastcall ToogleDefault(TObject *Sender);
    void __fastcall ToogleAuto(TObject *Sender);
    void __fastcall ToogleDSPCOMP(TObject *Sender);
    void __fastcall ToogleEQ(TObject *Sender);
    void __fastcall udpControlUDPRead(TObject *Sender, TStream *AData,
          TIdSocketHandle *ABinding);
    void __fastcall tmWatchTimer(TObject *Sender);
    void __fastcall ToogleOutputMute(TObject *Sender);
    void __fastcall ToogleOutputInvert(TObject *Sender);
    void __fastcall SpeedButton119Click(TObject *Sender);
    void __fastcall ToogleDO(TObject *Sender);
    void __fastcall ToogleOutputMix(TObject *Sender);
    void __fastcall ToogleCOMP(TObject *Sender);
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
    void __fastcall dsp_gain_trackbarChange(TObject *Sender);
    void __fastcall FormMouseWheel(TObject *Sender, TShiftState Shift,
          int WheelDelta, TPoint &MousePos, bool &Handled);
    void __fastcall input_typeMouseDown(TObject *Sender,
          TMouseButton Button, TShiftState Shift, int X, int Y);
    void __fastcall i10dBvClick(TObject *Sender);
    void __fastcall i10dBvDrawItem(TObject *Sender, TCanvas *ACanvas,
          TRect &ARect, bool Selected);
    void __fastcall i10dBvMeasureItem(TObject *Sender, TCanvas *ACanvas,
          int &Width, int &Height);
    void __fastcall btnDspResetEQClick(TObject *Sender);
    void __fastcall MenuItem3Click(TObject *Sender);
    void __fastcall input_panel_level_editKeyDown(TObject *Sender,
          WORD &Key, TShiftState Shift);
    void __fastcall output_panel_level_editKeyDown(TObject *Sender,
          WORD &Key, TShiftState Shift);
    void __fastcall input_panel_level_editExit(TObject *Sender);
    void __fastcall io_panel_trackbarKeyDown(TObject *Sender, WORD &Key,
          TShiftState Shift);
    void __fastcall WatchPaint(TObject *Sender);
    void __fastcall input_panel_level_editClick(TObject *Sender);
    void __fastcall input_panel_dsp_numClick(TObject *Sender);
    void __fastcall output_panel_dsp_numClick(TObject *Sender);
    void __fastcall input_panel_trackbarEnter(TObject *Sender);
    void __fastcall input_panel_trackbarExit(TObject *Sender);
    void __fastcall master_panel_level_editKeyDown(TObject *Sender,
          WORD &Key, TShiftState Shift);
    void __fastcall pnlmix_level_trackbarChange(TObject *Sender);
    void __fastcall pnlmix_level_editKeyDown(TObject *Sender, WORD &Key,
          TShiftState Shift);
    void __fastcall pnlmix_muteClick(TObject *Sender);
    void __fastcall btnSavePresetToFileClick(TObject *Sender);
    void __fastcall btnLoadPresetFromFileClick(TObject *Sender);
    void __fastcall StoreClick(TObject *Sender);
    void __fastcall StoreAsClick(TObject *Sender);
    void __fastcall RecallClick(TObject *Sender);
    void __fastcall btnSetIpClick(TObject *Sender);
    void __fastcall btnGetLogClick(TObject *Sender);
    void __fastcall btnGetDebugClick(TObject *Sender);
    void __fastcall lblPresetNameClick(TObject *Sender);
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
    void __fastcall btnInputCancelClick(TObject *Sender);
    void __fastcall btnInputOKClick(TObject *Sender);
    void __fastcall edtInputExit(TObject *Sender);
    void __fastcall edtInputKeyDown(TObject *Sender, WORD &Key,
          TShiftState Shift);
    void __fastcall edtCompRatioKeyDown(TObject *Sender, WORD &Key,
          TShiftState Shift);
    void __fastcall btnDspEqClick(TObject *Sender);
    void __fastcall edtCompThresholdKeyDown(TObject *Sender, WORD &Key,
          TShiftState Shift);
    void __fastcall edtCompAttackTimeKeyDown(TObject *Sender, WORD &Key,
          TShiftState Shift);
    void __fastcall edtCompReleaseTimeKeyDown(TObject *Sender, WORD &Key,
          TShiftState Shift);
    void __fastcall edtCompGainKeyDown(TObject *Sender, WORD &Key,
          TShiftState Shift);
    void __fastcall lbl5VdClick(TObject *Sender);
    void __fastcall tmLedTimer(TObject *Sender);
    void __fastcall PaintBox2Paint(TObject *Sender);
    void __fastcall SpeedButtonNoFrame2MouseDown(TObject *Sender,
          TMouseButton Button, TShiftState Shift, int X, int Y);
    void __fastcall PaintBox3Paint(TObject *Sender);
    void __fastcall PaintBox4Paint(TObject *Sender);
    void __fastcall dsp_gain_editKeyDown(TObject *Sender, WORD &Key,
          TShiftState Shift);
    void __fastcall dsp_delay_trackbarChange(TObject *Sender);
    void __fastcall dsp_delay_editKeyDown(TObject *Sender, WORD &Key,
          TShiftState Shift);
    void __fastcall cbCompAutoTimeClick(TObject *Sender);
    void __fastcall rbStaticIpEnabledClick(TObject *Sender);
    void __fastcall Label51Click(TObject *Sender);
    void __fastcall Label42Click(TObject *Sender);
    void __fastcall cbGlobalDspNameClick(TObject *Sender);
    void __fastcall cbPresetAutoSavedClick(TObject *Sender);
    void __fastcall btnRebootDeviceClick(TObject *Sender);
    void __fastcall cbMenuKeyFunctionChange(TObject *Sender);
    void __fastcall cbUpKeyFunctionChange(TObject *Sender);
    void __fastcall cbDownKeyFunctionChange(TObject *Sender);
    void __fastcall cbLockUpDownMenuKeyClick(TObject *Sender);
    void __fastcall btnSaveFlashToFileClick(TObject *Sender);
    void __fastcall btnLoadFileToFlashClick(TObject *Sender);
    void __fastcall tmDelayUpdateUITimer(TObject *Sender);
    void __fastcall btnClearDebugClick(TObject *Sender);
    void __fastcall tmDelayBackupTimer(TObject *Sender);
    void __fastcall pmPresetSaveLoadPopup(TObject *Sender);
    void __fastcall clbAvaliablePresetMouseDown(TObject *Sender,
          TMouseButton Button, TShiftState Shift, int X, int Y);
    void __fastcall btnResetAllConfigClick(TObject *Sender);
    void __fastcall lblDeviceNameDblClick(TObject *Sender);
    void __fastcall btnResetKeyFunctionClick(TObject *Sender);
    void __fastcall btnClearAllPresetClick(TObject *Sender);
    void __fastcall tmDelaySendCmdTimer(TObject *Sender);
    void __fastcall btnSaveLogClick(TObject *Sender);
    void __fastcall edtMACMouseDown(TObject *Sender, TMouseButton Button,
          TShiftState Shift, int X, int Y);
    void __fastcall edtDeviceTypeExit(TObject *Sender);
    void __fastcall PopupMenu3Popup(TObject *Sender);
    void __fastcall Copy1Click(TObject *Sender);
    void __fastcall Paste1Click(TObject *Sender);
    void __fastcall FormResize(TObject *Sender);
    void __fastcall cbLedTestClick(TObject *Sender);
    void __fastcall lvDeviceCustomDrawItem(TCustomListView *Sender,
          TListItem *Item, TCustomDrawState State, bool &DefaultDraw);
    void __fastcall btnInsertUserLogClick(TObject *Sender);
    void __fastcall lvLogAdvancedCustomDrawItem(TCustomListView *Sender,
          TListItem *Item, TCustomDrawState State, TCustomDrawStage Stage,
          bool &DefaultDraw);
    void __fastcall btnDisconnectClick(TObject *Sender);
    void __fastcall vleAdcMaxDrawCell(TObject *Sender, int ACol, int ARow,
          const TRect &Rect, TGridDrawState State);
    void __fastcall vleAdcMinDrawCell(TObject *Sender, int ACol, int ARow,
          const TRect &Rect, TGridDrawState State);
    void __fastcall btnCopyVoteDataToClipClick(TObject *Sender);
    void __fastcall btnClearDataAndTimeClick(TObject *Sender);
    void __fastcall input_panel_trackbarMouseMove(TObject *Sender,
          TShiftState Shift, int X, int Y);
    void __fastcall btnCopyDebugLogClick(TObject *Sender);
    void __fastcall btnCutDebugLogClick(TObject *Sender);
    void __fastcall lblKeepLiveCheckDblClick(TObject *Sender);
    void __fastcall lvLogData(TObject *Sender, TListItem *Item);
    void __fastcall output_panel_level_editEnter(TObject *Sender);
    void __fastcall edtSelectAllAndCopy(TObject *Sender);
    void __fastcall SpeedButtonNoFrame1Click(TObject *Sender);
    void __fastcall rgLedTestClick(TObject *Sender);
    void __fastcall cbUsart1ReceiveAckClick(TObject *Sender);
    void __fastcall cbUsart3ReceiveAckClick(TObject *Sender);
    void __fastcall btnDebugInfoExClick(TObject *Sender);
    void __fastcall btnStopDebugInfoExClick(TObject *Sender);
    void __fastcall Button2Click(TObject *Sender);
    void __fastcall Button3Click(TObject *Sender);
    void __fastcall tmProcessReplyTimer(TObject *Sender);
    void __fastcall input_panel_thumbPaint(TObject *Sender);
    void __fastcall output_panel_thumbPaint(TObject *Sender);
    void __fastcall PaintBox1Click(TObject *Sender);
    void __fastcall edt4TabKeyDown(TObject *Sender, WORD &Key,
          TShiftState Shift);
    void __fastcall Edit2Enter(TObject *Sender);
    void __fastcall Edit3Enter(TObject *Sender);
    void __fastcall btnAdminPasswordClick(TObject *Sender);
    void __fastcall cbLockParameterClick(TObject *Sender);
    void __fastcall imgMaskMouseDown(TObject *Sender, TMouseButton Button,
          TShiftState Shift, int X, int Y);
private:
    HANDLE h_sem;

    TIdUDPServer * udpSLPList[3];
private:
    String dst_ip;
    String local_ip;
    String local_broadcast_ip;

    String last_select_device_ip;

    bool is_manual_disconnect;
private:
    TSpeedButton * last_default_btn;
    TSpeedButton * last_out_num_btn;
    TSpeedButton * last_dsp_btn;

    int mix_panel_state;

    int low_freq;
    int low_gain;
    int high_freq;
    int high_gain;

    int eq_freq[11];
    int eq_gain[11];
    int eq_q[11];

    // 部分刷新界面的函数
    void OnFeedbackData(unsigned int cmd_id);
public:
    int sendcmd_delay_count;
    vector<TPackage> sendcmd_list;

    void SendBuffer(AnsiString AHost, const int APort, void *ABuffer, const int AByteCount);
    void SendCmd(String cmd);
    void SendCmd2(String cmd);
    void SendDisconnect();
    bool ProcessSendTextCmdAck(String cmd_text, TStream *AData, TIdSocketHandle *ABinding);

private:
    int log_count;
    Event event_data[EVENT_POOL_SIZE];
    Event event_data_tmp[EVENT_POOL_SIZE];
    String event_syn_timer[EVENT_POOL_SIZE];

    static String InnerMacInfo(String mac_string)
    {
        if (mac_string == "00-5A-39-FF-49-28")
        {
            return "LX无线";
        }
        else if (mac_string == "00-E0-4C-39-17-31")
        {
            return "LX独立网卡";
        }
        else if (mac_string == "74-D0-2B-95-48-02")
        {
            return "LEL无线网卡";
        }
        else if (mac_string == "00-E0-4C-15-1B-C0")
        {
            return "LEL单独网卡";
        }
        else if (mac_string == "00-5A-39-FF-49-28")
        {
            return "LEL单独网卡2";
        }
        else if (mac_string == "10-0B-A9-2F-55-90")
        {
            return "KW无线网卡";
        }
        else if (mac_string == "F0-DE-F1-B0-9E-42")
        {
            return "KW有线网卡";
        }
        else if (mac_string == "50-B7-C3-6F-83-47")
        {
            return "LX900X3D有线";
        }
        else if (mac_string == "00-18-DE-C9-8B-71")
        {
            return "T61无线";
        }
        else if (mac_string == "C4-85-08-E5-20-13")
        {
            return "LX900X3D无线";
        }
        else if (mac_string == "C4-85-08-E5-20-13")
        {
            return "LX900X3D无线";
        }
        else if (mac_string == "38-2C-4A-BA-EF-54")
        {
            return "LX板载网卡";
        }

        return "";
    }

    int mac_count;
    MacCode mac_data[256];
    void ProcessMACLog(LogBuff & buff)
    {
        // MAC地址
        for (int i=0;i<128;i++)
        {
            if (memcmp(buff.data.mac[i], "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF", 8) != 0)
            {
                memcpy(&mac_data[mac_count], buff.data.mac[i], sizeof(MacCode));
                mac_count++;
            }
        }
        /*

                TListItem * item = lvLog->Items->Add();

                item->Caption = "";
                item->SubItems->Add("mac");
                String mac_string;
                mac_string.sprintf("%02X-%02X-%02X-%02X-%02X-%02X",
                                    buff.mac[i][0], buff.mac[i][1], buff.mac[i][2],
                                    buff.mac[i][3], buff.mac[i][4], buff.mac[i][5]);
                item->SubItems->Add(mac_string);
                item->SubItems->Add(InnerMacInfo(mac_string));
                item->SubItems->Add("");        */


    }

    void SendLogBuff(int udp_port, void * buff, int size);
    bool ProcessLogBuffAck(LogBuff& buff, TStream *AData, TIdSocketHandle *ABinding);

    // 最大启动次数地址
    Event* tail_address;
    void ProcessLogData();
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

    String edtDeviceFullName_data;
    void __fastcall ApplyConfigToUI();
    void __fastcall ClearUI();

    String preset_lib_filename;
    void __fastcall SetPresetLibFilename(String filename);
    void __fastcall SetFileDirty(bool dirty_flag);

    void __fastcall UpdateCaption();
    void __fastcall UpdateBuildTime();
    void __fastcall UpdateDeviceType()
    {
        char * device_type;
        if (device_setting.device_type[0] == '*')
        {
            edtDeviceType->Color = clRed;
            edtDeviceType->Text = device_setting.device_type+1;
        }
        else
        {
            edtDeviceType->Color = clWindow;
            edtDeviceType->Text = device_setting.device_type;
        }
    }
public:		// User declarations
    __fastcall TForm1(TComponent* Owner);
    __fastcall ~TForm1();

    TPanel * watch_panel_inner[17+16];

    TSpeedButton* input_dsp_btn[17];
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
    TPaintBox * input_thumb[17];

    TSpeedButton* output_dsp_btn[16];
    TLabel* output_type_lbl[16];
    TSpeedButton* output_eq_btn[16];
    TSpeedButton* output_comp_btn[16];
    TSpeedButton* output_number_btn[16];
    TSpeedButton* output_invert_btn[16];
    TSpeedButton* output_mute_btn[16];
    TEdit* output_level_edit[16];
    TAdvTrackBar* output_level_trackbar[16];
    TStaticText* output_dsp_name[16];
    TPaintBox * output_thumb[16];

    TSpeedButton* mix_mute_btn[17];
    TEdit* mix_level_edit[17];
    TAdvTrackBar* mix_level_trackbar[17];

private:
    // reply缓冲区
    struct ReplyMsgList{
        int count;
        ReplyMsg reply[REPLY_TEXT_MSG_SIZE];
    };
    vector<ReplyMsgList> reply_msg_buf;
    unsigned int pre_received_msg_id;
    void CachePackageMessageFeedback(char*);
    void ProcessPackageMessageFeedback(ReplyMsg text_syn_msg[REPLY_TEXT_MSG_SIZE], int reply_msg_count, std::vector<UINT> & cmd_id_list);

    void CalcAllVote(ADC_Data_Ex & adc_data);
    void ProcessKeepAlive(int preset_id, bool need_reload, unsigned __int64 timer);
    void ProcessVote(ADC_Data_Ex adc_ex, ADC_Data_Ex adc_ex_max, ADC_Data_Ex adc_ex_min);
    void ProcessWatchLevel(int watch_level[INPUT_DSP_NUM + OUTPUT_DSP_NUM], int watch_level_comp[OUTPUT_DSP_NUM]);
    ADC_Data_Ex adc_ex_max;
    ADC_Data_Ex adc_ex_min;
private:
    void UpdateWatchLevel(int i, int value, int comp_value=-100)
    {
        level_meter[i][0] = value;
        level_meter[i][1] = comp_value;
        if (pb_watch_list[i] != NULL)
        {
            pb_watch_list[i]->Invalidate();
        }
    }
    int level_meter[32][2];
public:
    TPaintBox * pb_watch_list[32];

private:
    unsigned __int64 running_timer;
    int roboot_count;

    // 0-3 正常； 4 初始化； >=5 失联
    int keep_live_count;
    bool device_connected;

    void ShowInputPanel(TControl * Sender, TNotifyEvent event, String default_text);
    TControl * inputObject;
    TNotifyEvent input_event;
    void __fastcall after_input_panel_dsp_numClick(TObject *Sender);
    void __fastcall after_output_panel_dsp_numClick(TObject *Sender);

    // 电压检测标准值
    TLabel* active_adc;
    float up_line_value;
    float down_line_value;

    void SetIOChannelNum();

    void CloseDspDetail();
private:
    WNDPROC old_pnlSystem_proc;
    static LRESULT new_pnlSystem_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
    HWND hIpEdit;

    String local_mac_list;
    bool is_inner_pc;
private:
    // 备份设备到文件流程专用
    String save_device_to_file_filename;
    String save_single_preset_to_file_filename;

    int restor_delay_count;
    vector<TPackage> package_list;

    vector<TPackage> read_one_preset_package_list;
    void StartReadCurrentPreset(bool lock_flag);
    void StartReadPreset(int preset_id);

    void ResetLHFilter()
    {
        int preset_freq_list[11] = {20, 50, 100, 200, 500, 1000, 2000, 5000, 7500, 10000, 20000};
        filter_set.GetFilter(HP_FILTER)->ChangFilterParameter("12dB_BUTTERWORTH_HIGH", preset_freq_list[FIRST_FILTER-1], 0, 4.09);
            filter_set.SetBypass(HP_FILTER, true);
            filter_set.GetFilter(HP_FILTER)->name = "H";
            filter_set.RepaintPaint(HP_FILTER);
            filter_set.SendPeqCmd(HP_FILTER);
            filter_set.SendBypassCmd(HP_FILTER);

        filter_set.GetFilter(LP_FILTER)->ChangFilterParameter("12dB_BUTTERWORTH_LOW", preset_freq_list[LP_FILTER-1], 0, 4.09);
            filter_set.SetBypass(LP_FILTER, true);
            filter_set.GetFilter(LP_FILTER)->name = "L";
            filter_set.RepaintPaint(LP_FILTER);
            filter_set.SendPeqCmd(LP_FILTER);
            filter_set.SendBypassCmd(LP_FILTER);
    }

    void ShowLockConfigArea();
    void HideLockConfigArea();
private:
    String admin_password;
    bool admin_password_ok;
    void UpdateParameterEnabled()
    {
        bool value = (global_config.lock_parameter==0 || admin_password_ok);
        //pnlOperator->Enabled = value;
        if (value)
            imgMask->SendToBack();
        else
            imgMask->BringToFront();
    }
private:
    bool need_resize;

    VersionFunction GetVersionConfig();
private:
    int broken_count;
    int send_keeplive_count;
    int recv_keeplive_count;
    int slp_count;
    TTime last_keeplive_time;

    void CloseControlLink(String reason);
private:
    void AdjustOutputCompParam(int dsp_num);
public:
    void AppendLog(String str)
    {
        if (cbLogEnabled->Checked)
        {
            memo_debug->Lines->Add(str);
        }
    }
};
//---------------------------------------------------------------------------
extern PACKAGE TForm1 *Form1;
//---------------------------------------------------------------------------
#endif
