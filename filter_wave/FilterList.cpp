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
    if (filter[band].name == NULL)
        return true;
    if (GetFilterFreq(band)<=0.00001)
        return true;

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

    if (band == LP_FILTER)
    {
        return false;
    }

    switch (GetFilter(LP_FILTER)->UseIIRCount())
    {
    case 1:
        break;
    case 2:
        if (band >= LP_FILTER-1)
        {
            return true;
        }
        break;
    case 3:
        if (band >= LAST_FILTER-2)
        {
            return true;
        }
        break;
    case 4:
        if (band >= LAST_FILTER-3)
        {
            return true;
        }
        break;
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
        // 下发PEQ系数
        D1608Cmd cmd;
        double tmp;

        cmd.data.data_filter.TYPE = GetFilter(band)->GetTypeId();

        tmp = GetFilterFreq(band)*10;
        cmd.data.data_filter.FREQ = tmp;

        tmp = GetFilterGain(band)*10;
        cmd.data.data_filter.GAIN = tmp;

        tmp = GetFilter(band)->GetQ()*100; 
        cmd.data.data_filter.Q = tmp;

        tmp = IsBypass(band) ? 1 : 0;  
        cmd.data.data_filter.bypass = tmp;

        cmd.length = sizeof(cmd.data.data_filter);

        if (dsp_id == 0)
        {
        }
        else if (dsp_id < 100)
        {
            config_map.input_dsp[dsp_id-1].filter[band-1] = cmd.data.data_filter;
            cmd.id = GetOffsetOfData(&config_map.input_dsp[dsp_id-1].filter[band-1]);
        }
        else
        {
            config_map.output_dsp[dsp_id-101].filter[band-1] = cmd.data.data_filter;
            cmd.id = GetOffsetOfData(&config_map.output_dsp[dsp_id-101].filter[band-1]);
        }

        Form1->SendCmd(cmd);
    }
}

void FilterSet::UpdateCompRatio()
{
    WORD key = VK_RETURN;
    if (ratio == 0)
        Form1->edtCompRatio->Text = "∞";
    else
        Form1->edtCompRatio->Text = 1.0 / ratio;
    Form1->edtCompRatioKeyDown(Form1->edtCompRatio, key, TShiftState());
}
void FilterSet::UpdateCompThreshold()
{
    WORD key = VK_RETURN;
    Form1->edtCompThreshold->Text = threshold;
    Form1->edtCompThresholdKeyDown(Form1->edtCompThreshold, key, TShiftState());
}
void FilterSet::UpdateCompGain()
{
    WORD key = VK_RETURN;
    Form1->edtCompGain->Text = gain;
    Form1->edtCompGainKeyDown(Form1->edtCompGain, key, TShiftState());
}


