//---------------------------------------------------------------------------


#pragma hdrstop

#include "FilterPanelSet.h"
#include "FilterList.h"
#include <algorithm>
using std::min;
using std::max;
//---------------------------------------------------------------------------

#pragma package(smart_init)
static double Str2Freq(String str, double old_freq)
{
    try{
        return str.ToDouble();
    }catch(...){
        return old_freq;
    }
}
static double Str2Double(String str, double old_freq)
{
    try{
        return str.ToDouble();
    }catch(...){
        return old_freq;
    }
}

void PanelAgent::SetPanel(int band, TPanel* panel, TEdit* edtFreq, TEdit* edtQ, TEdit* edtGain, TComboBox* cbType, TCheckBox* cbBypass)
{
    _panel[band] = panel;
    _edtFreq[band] = edtFreq;
    _edtQ[band] = edtQ;
    _edtGain[band] = edtGain;

    edtFreq->Tag = 101;
    edtQ->Tag = 102;

    cbType->OnChange = cbTypeChange;
    cbBypass->OnClick = cbBypassClick;

    _edtFreq[band]->OnClick = edtSelectAllOnClick;
    _edtFreq[band]->OnExit = edtSelectAllOnExit;
    _edtQ[band]->OnClick = edtSelectAllOnClick;
    _edtQ[band]->OnExit = edtSelectAllOnExit;
    _edtGain[band]->OnClick = edtSelectAllOnClick;
    _edtGain[band]->OnExit = edtSelectAllOnExit;

    _edtFreq[band]->OnEnter = OnPanelEnter;
    _edtQ[band]->OnEnter = OnPanelEnter;
    _edtGain[band]->OnEnter = OnPanelEnter;
    cbType->OnEnter = OnPanelEnter;
    cbBypass->OnEnter = OnPanelEnter;

    _edtFreq[band]->OnKeyDown = edtFreqKeyDown;
    _edtQ[band]->OnKeyDown = edtQKeyDown;
    _edtGain[band]->OnKeyDown = edtGainKeyDown;
}
TPanel* PanelAgent::GetPanel(int band)
{
    return _panel[band];
}
TEdit* PanelAgent::GetFreqText(int band)
{
    return _edtFreq[band];
}
TEdit* PanelAgent::GetQText(int band)
{
    return _edtQ[band];
}
TEdit* PanelAgent::GetGainText(int band)
{
    return _edtGain[band];
}

void PanelAgent::SetPanelUnselect(int band)
{
    _panel[band]->BevelInner = bvNone;
    _panel[band]->BevelOuter = bvNone;
}
void PanelAgent::SetPanelSelect(int band)
{
    _panel[band]->BevelInner = bvRaised;
    _panel[band]->BevelOuter = bvLowered;
}

void __fastcall PanelAgent::cbTypeChange(TObject *Sender)
{
    int band = ((TControl*)Sender)->Parent->Tag;
    TComboBox * type_ccombobox = (TComboBox*)Sender;
    _filter_set.GetFilter(band)->SetType(type_ccombobox->Text);

    String type = type_ccombobox->Text;
    if ((type == "BandPass")
      ||(type == "Notch")
      ||(type == "High Butterworth 2nd")
      ||(type == "High Butterworth 4nd")
      ||(type == "Low Butterworth 2nd")
      ||(type == "Low Butterworth 4nd"))
    {
        GetGainText(band)->Text = "0";
        GetGainText(band)->Enabled = false;

        _filter_set.GetFilter(band)->SetGain(0);
    }
    else
    {
        GetGainText(band)->Enabled = true;
    }

    if (band == 1)
    {
        // band 2 ʧЧ
        GetPanel(2)->Enabled = (type != "High Butterworth 4nd");
        GetFreqText(2)->Enabled = (type != "High Butterworth 4nd");
        GetQText(2)->Enabled = (type != "High Butterworth 4nd");
        GetGainText(2)->Enabled = (type != "High Butterworth 4nd");
    }
    if (band == 8)
    {
        // band 7 ʧЧ
        GetPanel(7)->Enabled = (type != "Low Butterworth 4nd");
        GetFreqText(7)->Enabled = (type != "Low Butterworth 4nd");
        GetQText(7)->Enabled = (type != "Low Butterworth 4nd");
        GetGainText(7)->Enabled = (type != "Low Butterworth 4nd");
    }

    _filter_set.GetFilter(band)->SetType(type_ccombobox->Text);
    _filter_set.RepaintPaint();
}

void __fastcall PanelAgent::cbBypassClick(TObject *Sender)
{
    int band = ((TControl*)Sender)->Parent->Tag;
    TCheckBox * bypass_checkbox = (TCheckBox*)Sender;
    _filter_set.SetBypass(band, bypass_checkbox->Checked);

    _filter_set.RepaintPaint();
}

void __fastcall PanelAgent::edtSelectAllOnClick(TObject *Sender)
{
    TEdit * try_cast2Edit = dynamic_cast<TEdit*>(Sender);
    if (try_cast2Edit != NULL)
    {
        try_cast2Edit->SelectAll();
        try_cast2Edit->OnClick = NULL;
    }
}
void __fastcall PanelAgent::edtSelectAllOnExit(TObject *Sender)
{
    TEdit * edtControl = (TEdit*)Sender;
    WORD Key = VK_RETURN;
    TShiftState Shift;
    edtControl->OnKeyDown(edtControl, Key, Shift);

    edtControl->OnClick = edtSelectAllOnClick;
}
void __fastcall PanelAgent::OnPanelEnter(TObject *Sender)
{
    int band = ((TControl*)Sender)->Parent->Tag;
    
    if (_filter_set.GetActiveBand() != band)
    {
        _filter_set.SetActiveBand(band);
        _filter_set.RepaintPaint();
    }
}

void __fastcall PanelAgent::edtFreqKeyDown(TObject *Sender, WORD &Key,
      TShiftState Shift)
{
    if (Key == VK_RETURN)
    {
        int band = ((TControl*)Sender)->Parent->Tag;
        TEdit * edtFreq = (TEdit*)Sender;
        double freq = Str2Freq(edtFreq->Text, _filter_set.GetFilter(band)->GetFreq());

        freq = max(freq, 20.0);
        freq = min(freq, 20000.0);

        edtFreq->Text = freq;

        _filter_set.GetFilter(band)->SetFreq(freq);
        _filter_set.RepaintPaint();
    }
    else if (Key == VK_UP || Key == VK_DOWN)
    {
        int band = ((TControl*)Sender)->Parent->Tag;
        TEdit * edtFreq = (TEdit*)Sender;
        double current_freq = edtFreq->Text.ToDouble();
        current_freq *= 10;
        current_freq = Floor(current_freq+0.5);
        current_freq = current_freq / 10;
        if (Key == VK_UP)
        {
            current_freq = NextLargeFreq(current_freq);
        }
        else if (Key == VK_DOWN)
        {
            current_freq = NextSmallFreq(current_freq);
        }
        edtFreq->Text = current_freq;

        _filter_set.GetFilter(band)->SetFreq(current_freq);
        _filter_set.RepaintPaint();
    }
}
//---------------------------------------------------------------------------
void __fastcall PanelAgent::edtGainKeyDown(TObject *Sender, WORD &Key,
      TShiftState Shift)
{
    if (Key == VK_RETURN)
    {
        int band = ((TControl*)Sender)->Parent->Tag;

        String type = _filter_set.GetFilter(band)->GetType();
        if ((type == "BandPass")
          ||(type == "Notch")
          ||(type == "High Butterworth 2nd")
          ||(type == "High Butterworth 4nd")
          ||(type == "Low Butterworth 2nd")
          ||(type == "Low Butterworth 4nd"))
        {
            TEdit * edtGain = (TEdit*)Sender;
            edtGain->Text = 0;
        }
        else
        {
            TEdit * edtGain = (TEdit*)Sender;
            double gain = Str2Double(edtGain->Text, _filter_set.GetFilter(band)->GetGain());
            gain = max(gain, -30.0);
            gain = min(gain, 15.0);

            edtGain->Text = gain;

            _filter_set.GetFilter(band)->SetGain(gain);
            _filter_set.RepaintPaint();
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall PanelAgent::edtQKeyDown(TObject *Sender, WORD &Key,
      TShiftState Shift)
{
    if (Key == VK_RETURN)
    {
        int band = ((TControl*)Sender)->Parent->Tag;
        TEdit * edtQ = (TEdit*)Sender;
        double q = Str2Double(edtQ->Text, _filter_set.GetFilter(band)->GetQ());
        if (!IsInQTable(q))
             q = NextSmallQ(q);
                               
        edtQ->Text = q;

        _filter_set.GetFilter(band)->SetQ(q);
        _filter_set.RepaintPaint();
    }
    else if (Key == VK_UP || Key == VK_DOWN)
    {
        int band = ((TControl*)Sender)->Parent->Tag;
        TEdit * edtQ = (TEdit*)Sender;
        double current_q = edtQ->Text.ToDouble();
        current_q *= 100;
        current_q = Floor(current_q+0.5);
        current_q = current_q / 100;
        if (Key == VK_UP)
        {
            current_q = NextLargeQ(current_q);
        }
        else if (Key == VK_DOWN)
        {
            current_q = NextSmallQ(current_q);
        }
        edtQ->Text = current_q;

        _filter_set.GetFilter(band)->SetQ(current_q);
        _filter_set.RepaintPaint();
    }
}
//---------------------------------------------------------------------------
void __fastcall PanelAgent::OnMouseWheel(TObject *Sender, TShiftState Shift,
      int WheelDelta, TPoint &MousePos, bool &Handled)
{
    TControl* control = dynamic_cast<TControl*>(Sender);
    TComboBox * combobox = dynamic_cast<TComboBox*>(Sender);
    if (combobox != NULL /*&& !combobox->DroppedDown*/)
    {
        Handled = true;
    }

    if (control->Tag == 101)
    {
        TShiftState shift;
        WORD key;
        if (WheelDelta >0)
        {
            key = VK_UP;
        }
        else if (WheelDelta < 0)
        {
            key = VK_DOWN;
        }
        edtFreqKeyDown(control, key, shift);
        Handled = true;
    }
    if (control->Tag == 102)
    {
        TShiftState shift;
        WORD key;
        if (WheelDelta >0)
        {
            key = VK_UP;
        }
        else if (WheelDelta < 0)
        {
            key = VK_DOWN;
        }
        edtQKeyDown(control, key, shift);
        Handled = true;
    }
}
void PanelAgent::UpdateFreqQGain(int band)
{
    _edtFreq[band]->Text = _filter_set.GetFilter(band)->GetFreq();
    _edtQ[band]->Text = _filter_set.GetFilter(band)->GetQ();
    _edtGain[band]->Text = _filter_set.GetFilter(band)->GetGain();
}

