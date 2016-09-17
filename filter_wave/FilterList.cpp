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

    if (band == FIRST_FILTER)
    {
        return false;
    }

    if ((band-FIRST_FILTER+1) <= GetFilter(FIRST_FILTER)->UseIIRCount())
    {
        return true;
    }

    if (band == LAST_FILTER)
    {
        return false;
    }

    if ((LAST_FILTER-band+1) <= GetFilter(LAST_FILTER)->UseIIRCount())
    {
        return true;
    }

    return false;
}

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
    //char in_out = Form1->lblDSPInfo->Caption[1];
    if (band == 0)
        band = select_band;

    if (band > 0)
    {
        // TODO: 下发系数 需要优化
        D1608Cmd cmd;
        double tmp;

        cmd.value[0] = GetFilter(band)->GetTypeId();   // TYPE

        tmp = GetFilterFreq(band)*10;
        cmd.value[1] = tmp;           // FREQ

        tmp = GetFilterGain(band)*10;
        cmd.value[2] = tmp;           // GAIN

        tmp = GetFilter(band)->GetQ()*100;       // Q
        cmd.value[3] = tmp;

        tmp = IsBypass(band) ? 1 : 0;   // Bypass
        cmd.value[4] = tmp;

        if (dsp_id < 100)
        {
            cmd.id = GerOffsetOfData(&config_map.input_dsp[dsp_id-1].filter[band-1]);
            config_map.input_dsp[dsp_id-1].filter[band-1].TYPE = cmd.value[0];
            config_map.input_dsp[dsp_id-1].filter[band-1].FREQ = cmd.value[1];
            config_map.input_dsp[dsp_id-1].filter[band-1].GAIN = cmd.value[2];
            config_map.input_dsp[dsp_id-1].filter[band-1].Q = cmd.value[3];
            config_map.input_dsp[dsp_id-1].filter[band-1].bypass = cmd.value[4];
        }
        else
        {
            cmd.id = GerOffsetOfData(&config_map.output_dsp[dsp_id-101].filter[band-1]);
            config_map.output_dsp[dsp_id-101].filter[band-1].TYPE = cmd.value[0];
            config_map.output_dsp[dsp_id-101].filter[band-1].FREQ = cmd.value[1];
            config_map.output_dsp[dsp_id-101].filter[band-1].GAIN = cmd.value[2];
            config_map.output_dsp[dsp_id-101].filter[band-1].Q = cmd.value[3];
            config_map.output_dsp[dsp_id-101].filter[band-1].bypass = cmd.value[4];
        }

        Form1->SendCmd(cmd);
    }
}

