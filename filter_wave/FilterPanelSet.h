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
    :_filter_set(filter_set){}

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
private:
    TPanel* _panel[9];
    TEdit* _edtFreq[9];
    TEdit* _edtQ[9];
    TEdit* _edtGain[9];
    TComboBox* _cbType[9];
    TCheckBox* _cbBypass[9];


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
};

#endif
