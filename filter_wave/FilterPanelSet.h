//---------------------------------------------------------------------------

#ifndef FilterPanelSetH
#define FilterPanelSetH

#include <StdCtrls.hpp>
#include <ExtCtrls.hpp>
#include "FilterPanelSet.h"    
//---------------------------------------------------------------------------
void Switch2Panel(int band);

class PanelAgent : public TObject
{
public:
    PanelAgent(class FilterSet& filter_set)
    :_filter_set(filter_set)
    {
        for (int i=0;i<11;i++)
        {
            _panel[i] = NULL;
            _edtFreq[i] = NULL;
            _edtQ[i] = NULL;
            _edtGain[i] = NULL;
            _cbType[i] = NULL;
            _cbBypass[i] = NULL;
        }
    }

    void SetPanel(int band, TPanel* panel, TEdit* edtFreq, TEdit* edtQ, TEdit* edtGain, TComboBox* cbType, TCheckBox* cbBypass);
    void SetPanelEnabled(int band, bool value);
    void SetFreqTextEnabled(int band, bool value);
    void SetQTextEnabled(int band, bool value);
    void SetGainTextEnabled(int band, bool value);
    void SetGainTextValue(int band, String value);
    TComboBox* GetType(int band);
    TCheckBox* GetByPass(int band);

    void __fastcall OnMouseWheel(TObject *Sender, TShiftState Shift,
      int WheelDelta, TPoint &MousePos, bool &Handled);
    void UpdateFreqQGain(int band);

    void SetPanelUnselect(int band);
    void SetPanelSelect(int band);

    void LoadPreset();
private:
    // BAD:浪费了第一个元素
    TPanel* _panel[12];
    TEdit* _edtFreq[12];
    TEdit* _edtQ[12];
    TEdit* _edtGain[12];
    TComboBox* _cbType[12];
    TCheckBox* _cbBypass[12];

    void __fastcall cbTypeChange(TObject *Sender);
    void __fastcall cbBypassClick(TObject *Sender);

    void __fastcall edtSelectAllOnClick(TObject *Sender);
    void __fastcall edtSelectAllOnExit(TObject *Sender);

    void __fastcall OnPanelEnter(TObject *Sender);

    void __fastcall edtFreqKeyDown(TObject *Sender, WORD &Key,
          TShiftState Shift);
    void __fastcall edtGainKeyDown(TObject *Sender, WORD &Key,
          TShiftState Shift);
    void __fastcall edtQKeyDown(TObject *Sender, WORD &Key,
          TShiftState Shift);

    FilterSet& _filter_set;

    void SaveToConfigMap(int band);

    // 根据滤波器类型和占用资源情况，设置界面可用(Enabled)属性
    void UpdateUIEnabled();
};

#endif
