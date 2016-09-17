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
    TPanel* GetPanel(int band);
    TEdit* GetFreqText(int band);
    TEdit* GetQText(int band);
    TEdit* GetGainText(int band);
    TComboBox* GetType(int band);
    TCheckBox* GetByPass(int band);

    void __fastcall OnMouseWheel(TObject *Sender, TShiftState Shift,
      int WheelDelta, TPoint &MousePos, bool &Handled);
    void UpdateFreqQGain(int band);

    void SetPanelUnselect(int band);
    void SetPanelSelect(int band);

    void LoadPreset();
private:
    TPanel* _panel[11];
    TEdit* _edtFreq[11];
    TEdit* _edtQ[11];
    TEdit* _edtGain[11];
    TComboBox* _cbType[11];
    TCheckBox* _cbBypass[11];


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
};

#endif
