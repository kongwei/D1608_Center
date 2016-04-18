//---------------------------------------------------------------------------


#pragma hdrstop

#include "FilterList.h"
#include "untMain.h"
#include <StdCtrls.hpp>
//---------------------------------------------------------------------------

#pragma package(smart_init)

int FilterSet::GetActiveBand()
{
    return select_band;
}
void FilterSet::SetActiveBand(int band)
{
    panel_agent_ref->SetPanelUnselect(select_band);
    panel_agent_ref->SetPanelSelect(band);

    select_band = band;
}

bool FilterSet::IsBandForbidden(int band)
{
    if (band == 0)
    {
        return true;
    }
    if (band == 2 && GetFilter(1)->GetType() == "High Butterworth 4nd")
    {
        return true;
    }
    if (band == 7 && GetFilter(8)->GetType() == "Low Butterworth 4nd")
    {
        return true;
    }
    return false;
}

/*void FilterSet::OnParameterChange(int band)
{
    double freq = edtFreq[band]->Text.ToDouble();
    double gain = edtGain[band]->Text.ToDouble();
    double q = edtQ[band]->Text.ToDouble();
    String type = GetFilter(band)->GetType();
    GetFilter(band)->ChangFilterParameter(type, freq, gain, q);
}*/

Coefficient* FilterSet::GetFilter(int band)
{
    return filter+band;
}

double FilterSet::GetFilterFreq(int band)
{
    return GetFilter(band)->GetFreq();
}
double FilterSet::GetFilterGain(int band)
{
    return GetFilter(band)->GetGain();
}
String FilterSet::GetFilterType(int band)
{
    return GetFilter(band)->GetType();
}
bool FilterSet::IsBypass(int band)
{
    return bypass[band];
}
void FilterSet::SetBypass(int band, bool isBypass)
{
    bypass[band] = isBypass;
}
void FilterSet::Register(PaintAgent* paint_agent, PanelAgent* panel_agent)
{
    paint_agent_ref = paint_agent;
    panel_agent_ref = panel_agent;
}

void FilterSet::RepaintPaint(int band)
{
    if (paint_agent_ref != NULL)
    {
        paint_agent_ref->Repaint();
    }
    if (panel_agent_ref != NULL)
    {
        panel_agent_ref->UpdateFreqQGain(band);
    }

    int dsp_id = Form1->pnlDspDetail->Tag;
    char in_out = Form1->lblDSPInfo->Caption[1];
    if (band == 0)
        band = select_band;

    if (band > 0)
    {
        // TODO: 下发系数
        D1608Cmd cmd;
        double tmp;

        if (dsp_id < 100)
        {
            cmd.id = GerOffsetOfData(&config_map.input_dsp[dsp_id-1].filter[band-1]);
        }
        else
        {
            cmd.id = GerOffsetOfData(&config_map.output_dsp[dsp_id-101].filter[band-1]);
        }
        cmd.value = GetFilter(band)->GetTypeId();   // type

        tmp = GetFilterFreq(band)*10;
        cmd.value2 = tmp;           // FREQ

        tmp = GetFilterGain(band)*10;
        cmd.value3 = tmp;           // GAIN

        tmp = GetFilter(band)->GetQ()*100;       // Q
        cmd.value4 = tmp;

        Form1->SendCmd(cmd);
    }
}

