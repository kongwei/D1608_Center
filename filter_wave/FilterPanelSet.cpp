//---------------------------------------------------------------------------


#pragma hdrstop

#include "FilterPanelSet.h"
#include "FilterList.h"
#include "untMain.h"
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
static String GetFilterTypeSuffix(int band)
{
    switch(band)
    {
    case HP_FILTER:
        return " High";
    case LP_FILTER:
        return " Low";
    default:
        return "";
    }
}

void PanelAgent::SetPanel(int band, TPanel* panel, TEdit* edtFreq, TEdit* edtQ, TEdit* edtGain, TComboBox* cbType, TCheckBox* cbBypass)
{
    _panel[band] = panel;
    _edtFreq[band] = edtFreq;
    _edtQ[band] = edtQ;
    _edtGain[band] = edtGain;
    _cbType[band] = cbType;
    _cbBypass[band] = cbBypass;

    panel->Tag = band;

    edtFreq->Tag = 101;
    edtQ->Tag = 102;
    edtGain->Tag = 103;

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
void PanelAgent::SetPanelEnabled(int band, bool value)
{
    if (_panel[band])
    {
        _panel[band]->Enabled = value;
    }
}
void PanelAgent::SetFreqTextEnabled(int band, bool value)
{
    if (_edtFreq[band])
    {
        _edtFreq[band]->Enabled = value;
    }
}
void PanelAgent::SetQTextEnabled(int band, bool value)
{
    if (_edtQ[band])
    {
        _edtQ[band]->Enabled = value;
    }
}
void PanelAgent::SetGainTextEnabled(int band, bool value)
{
    if (_edtGain[band])
    {
        _edtGain[band]->Enabled = value;
    }
}
void PanelAgent::SetGainTextValue(int band, String value)
{
    if (_edtGain[band])
    {
        _edtGain[band]->Text = value;
    }
}

TComboBox* PanelAgent::GetType(int band)
{
    return _cbType[band];
}
TCheckBox* PanelAgent::GetByPass(int band)
{
    return _cbBypass[band];
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
void PanelAgent::LoadPreset()
{
    int dsp_id = Form1->pnlDspDetail->Tag;
    for (int band=1;band<12;band++)
    {
        if (_panel[band] != NULL)
        {
            // 写入FilterSet
            if (dsp_id == 0)
            {
            }
            else if (dsp_id < 100)
            {
                 _filter_set.GetFilter(band)->SetTypeId(config_map.input_dsp[dsp_id-1].filter[band-1].TYPE);
                 _filter_set.GetFilter(band)->SetFreq(config_map.input_dsp[dsp_id-1].filter[band-1].FREQ / 10.0);
                 _filter_set.GetFilter(band)->SetGain(config_map.input_dsp[dsp_id-1].filter[band-1].GAIN / 10.0);
                 _filter_set.GetFilter(band)->SetQ(config_map.input_dsp[dsp_id-1].filter[band-1].Q / 100.0);
                 _filter_set.SetBypass(band, config_map.input_dsp[dsp_id-1].filter[band-1].bypass);
            }
            else
            {
                 _filter_set.GetFilter(band)->SetTypeId(config_map.output_dsp[dsp_id-101].filter[band-1].TYPE);
                 _filter_set.GetFilter(band)->SetFreq(config_map.output_dsp[dsp_id-101].filter[band-1].FREQ / 10.0);
                 _filter_set.GetFilter(band)->SetGain(config_map.output_dsp[dsp_id-101].filter[band-1].GAIN / 10.0);
                 _filter_set.GetFilter(band)->SetQ(config_map.output_dsp[dsp_id-101].filter[band-1].Q / 100.0);
                 _filter_set.SetBypass(band, config_map.output_dsp[dsp_id-101].filter[band-1].bypass);
            }

            UpdateFreqQGain(band);
        }
    }

    if (dsp_id >= 100)
    {
        _filter_set.ratio = (config_map.output_dsp[dsp_id-101].ratio / 100.0);
        _filter_set.threshold = (config_map.output_dsp[dsp_id-101].threshold / 10.0);
        _filter_set.attack_time = config_map.output_dsp[dsp_id-101].attack_time / 10.0;
        _filter_set.release_time = config_map.output_dsp[dsp_id-101].release_time / 10.0;
        _filter_set.gain = (config_map.output_dsp[dsp_id-101].comp_gain / 10.0);
    }
}
void PanelAgent::UpdateUIEnabled()
{
    // 原本是  FIRST_FILTER .. LAST_FILTER-2, LP_FILTER-1, LP_FILTER
    for (int i=HP_FILTER;i<=LP_FILTER;i++)
    {
        bool band_forbidden = _filter_set.IsBandForbidden(i);
        SetPanelEnabled(i, !band_forbidden);
        SetFreqTextEnabled(i, !band_forbidden);
        SetQTextEnabled(i, !band_forbidden);
        SetGainTextEnabled(i, !band_forbidden);
    }
}
void __fastcall PanelAgent::cbTypeChange(TObject *Sender)
{
    int band = ((TControl*)Sender)->Parent->Tag;
    TComboBox * type_ccombobox = (TComboBox*)Sender;
    String type = type_ccombobox->Text;

    type = type + GetFilterTypeSuffix(band);

    _filter_set.GetFilter(band)->SetType(type);

    if (!_filter_set.GetFilter(band)->IsGainEnabled())
    {
        SetGainTextValue(band, "0");
        SetGainTextEnabled(band, false);

        _filter_set.GetFilter(band)->SetGain(0);
    }
    else
    {
        SetGainTextValue(band, _filter_set.GetFilter(band)->GetGain());
        SetGainTextEnabled(band, true);
    }

    UpdateUIEnabled();

    SaveToConfigMap(band);
    _filter_set.RepaintPaint();
}

void __fastcall PanelAgent::cbBypassClick(TObject *Sender)
{
    int band = ((TControl*)Sender)->Parent->Tag;
    TCheckBox * bypass_checkbox = (TCheckBox*)Sender;
    _filter_set.SetBypass(band, bypass_checkbox->Checked);

    UpdateUIEnabled();

    SaveToConfigMap(band);
    _filter_set.RepaintPaint(band);
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
    edtControl->OnKeyDown(edtControl, Key, TShiftState());
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
        edtFreq->SelectAll();

        _filter_set.GetFilter(band)->SetFreq(freq);
        SaveToConfigMap(band);
        _filter_set.RepaintPaint();
    }
    else if (Key == VK_ESCAPE)
    {
        int band = ((TControl*)Sender)->Parent->Tag;
        TEdit * edtFreq = (TEdit*)Sender;
        edtFreq->Text = _filter_set.GetFilter(band)->GetFreq();
    }
    else if (Key == VK_UP || Key == VK_DOWN || Key == VK_PRIOR || Key == VK_NEXT)
    {
        int band = ((TControl*)Sender)->Parent->Tag;
        TEdit * edtFreq = (TEdit*)Sender;
        double current_freq = edtFreq->Text.ToDouble();
        current_freq *= 10;
        current_freq = Floor(current_freq+0.5);
        current_freq = current_freq / 10;
        if (Key == VK_UP || Key == VK_PRIOR)
        {
            current_freq = NextLargeFreq(current_freq);
        }
        else if (Key == VK_DOWN || Key == VK_NEXT)
        {
            current_freq = NextSmallFreq(current_freq);
        }
        edtFreq->Text = current_freq;
        edtFreq->SelectAll();

        _filter_set.GetFilter(band)->SetFreq(current_freq);
        SaveToConfigMap(band);
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
        if (!_filter_set.GetFilter(band)->IsGainEnabled())
        {
            TEdit * edtGain = (TEdit*)Sender;
            edtGain->Text = 0;
            edtGain->SelectAll();
        }
        else
        {
            TEdit * edtGain = (TEdit*)Sender;
            double gain = Str2Double(edtGain->Text, _filter_set.GetFilter(band)->GetGain());

            _filter_set.GetFilter(band)->SetGain(gain);
            SaveToConfigMap(band);
            _filter_set.RepaintPaint();

            edtGain->Text = _filter_set.GetFilter(band)->GetGain();
            edtGain->SelectAll();
        }
    }
    else if (Key == VK_ESCAPE)
    {
        int band = ((TControl*)Sender)->Parent->Tag;
        TEdit * edtGain = (TEdit*)Sender;
        edtGain->Text = _filter_set.GetFilter(band)->GetGain();
    }
    else if (Key == VK_UP || Key == VK_DOWN || Key == VK_PRIOR || Key == VK_NEXT)
    {
        int band = ((TControl*)Sender)->Parent->Tag;
        TEdit * edtGain = (TEdit*)Sender;
        double gain = edtGain->Text.ToDouble();
        if (Key == VK_UP)
        {
            gain += 0.1;
        }
        else if (Key == VK_DOWN)
        {
            gain -= 0.1;
        }
        else if (Key == VK_PRIOR)
        {
            gain += 1;
            gain = int(gain);
        }
        else if (Key == VK_NEXT)
        {
            gain -= 1;
            gain = int(gain);
        }

        _filter_set.GetFilter(band)->SetGain(gain);
        SaveToConfigMap(band);
        _filter_set.RepaintPaint();

        edtGain->Text = _filter_set.GetFilter(band)->GetGain();
        edtGain->SelectAll();
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
        edtQ->SelectAll();

        _filter_set.GetFilter(band)->SetQ(q);
        SaveToConfigMap(band);
        _filter_set.RepaintPaint();
    }
    else if (Key == VK_ESCAPE)
    {
        int band = ((TControl*)Sender)->Parent->Tag;
        TEdit * edtQ = (TEdit*)Sender;
        edtQ->Text = _filter_set.GetFilter(band)->GetQ();
    }
    else if (Key == VK_UP || Key == VK_DOWN || Key == VK_PRIOR || Key == VK_NEXT)
    {
        int band = ((TControl*)Sender)->Parent->Tag;
        TEdit * edtQ = (TEdit*)Sender;
        double current_q = edtQ->Text.ToDouble();
        current_q *= 100;
        current_q = Floor(current_q+0.5);
        current_q = current_q / 100;
        if (Key == VK_UP || Key == VK_PRIOR)
        {
            current_q = NextLargeQ(current_q);
        }
        else if (Key == VK_DOWN || Key == VK_NEXT)
        {
            current_q = NextSmallQ(current_q);
        }
        edtQ->Text = current_q;
        edtQ->SelectAll();

        _filter_set.GetFilter(band)->SetQ(current_q);
        SaveToConfigMap(band);
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
    if (control->Tag == 103)
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
        edtGainKeyDown(control, key, shift);
        Handled = true;
    }
}
void PanelAgent::UpdateFreqQGain(int band)
{
    _edtFreq[band]->Text = _filter_set.GetFilter(band)->GetFreq();
    _edtQ[band]->Text = _filter_set.GetFilter(band)->GetQ();
    _edtGain[band]->Text = _filter_set.GetFilter(band)->GetGain();
    _cbBypass[band]->Checked = _filter_set.IsBypass(band);

    for (int i=0;i<_cbType[band]->Items->Count;i++)
    {
        if (_cbType[band]->Items->Strings[i]+GetFilterTypeSuffix(band) == _filter_set.GetFilter(band)->GetType())
        {
            _cbType[band]->ItemIndex = i;
            break;
        }
    }
}

void PanelAgent::SaveToConfigMap(int band)
{
    int dsp_id = Form1->pnlDspDetail->Tag;
    if (dsp_id == 0)
    {
    }
    else if (dsp_id < 100)
    {
        config_map.input_dsp[dsp_id-1].filter[band-1].TYPE = _filter_set.GetFilter(band)->GetTypeId() * 10;
        config_map.input_dsp[dsp_id-1].filter[band-1].GAIN = _filter_set.GetFilter(band)->GetGain()*10;
        config_map.input_dsp[dsp_id-1].filter[band-1].Q = _filter_set.GetFilter(band)->GetQ()*100;
        config_map.input_dsp[dsp_id-1].filter[band-1].bypass = _filter_set.IsBypass(band) ? 1 : 0;
    }
    else
    {
        config_map.output_dsp[dsp_id-101].filter[band-1].TYPE = _filter_set.GetFilter(band)->GetTypeId() * 10;
        config_map.output_dsp[dsp_id-101].filter[band-1].GAIN = _filter_set.GetFilter(band)->GetGain()*10;
        config_map.output_dsp[dsp_id-101].filter[band-1].Q = _filter_set.GetFilter(band)->GetQ()*100;
        config_map.output_dsp[dsp_id-101].filter[band-1].bypass = _filter_set.IsBypass(band) ? 1 : 0;
    }
}

