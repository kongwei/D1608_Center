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

    if (band == HP_FILTER)
    {
        return false;
    }

    if ((band-HP_FILTER+1) <= GetFilter(HP_FILTER)->UseIIRCount())
    {
        return !IsBypass(HP_FILTER);
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
            return !IsBypass(LP_FILTER);
        }
        break;
    case 3:
        if (band >= LAST_FILTER-2)
        {
            return !IsBypass(LP_FILTER);
        }
        break;
    case 4:
        if (band >= LAST_FILTER-3)
        {
            return !IsBypass(LP_FILTER);
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
}
void FilterSet::SendPeqCmd(int band)
{
    int dsp_num = Form1->pnlDspDetail->Tag;
    if (band == 0)
        band = select_band;

    if (band > 0)
    {
        // 下发PEQ系数
        FilterConfigMap data_filter;
        double tmp;

        data_filter.TYPE = GetFilter(band)->GetTypeId();

        tmp = GetFilterFreq(band)*10;
        data_filter.FREQ = tmp;

        tmp = GetFilterGain(band)*10;
        data_filter.GAIN = tmp;

        // TODO: 1940会变成1939？
        tmp = GetFilter(band)->GetQ()*1000; 
        data_filter.Q = ((int)tmp)/10;

        tmp = IsBypass(band) ? 1 : 0;  
        data_filter.bypass = tmp;

        if (dsp_num == 0)
        {
        }
        else if (dsp_num < 100)
        {
            config_map.input_dsp[dsp_num-1].filter[band-1] = data_filter;

            String cmd_text = D1608CMD_FLAG;
            cmd_text = cmd_text+"input<"+IntToStr(dsp_num)+">.peq<"+IntToStr(band)+">="
                +FormatFloat("0.0", GetFilterFreq(band))+"Hz,"
                +FormatFloat("0.0", GetFilter(band)->GetQ())+","
                +FormatFloat("0.0", GetFilterGain(band))+"dB,"
                +GetFilter(band)->GetType()+""
                +"]";
            Form1->SendCmd(cmd_text);
        }
        else
        {
            config_map.output_dsp[dsp_num-101].filter[band-1] = data_filter;

            String cmd_text = D1608CMD_FLAG;
            cmd_text = cmd_text+"output<"+IntToStr(dsp_num-100)+">.peq<"+IntToStr(band)+">="
                +FormatFloat("0.0", GetFilterFreq(band))+"Hz,"
                +FormatFloat("0.0", GetFilter(band)->GetQ())+","
                +FormatFloat("0.0", GetFilterGain(band))+"dB,"
                +GetFilter(band)->GetType()+""
                +"]";
            Form1->SendCmd(cmd_text);

        }
    }
}
void FilterSet::SendBypassCmd(int band)
{
    int dsp_num = Form1->pnlDspDetail->Tag;
    if (band == 0)
        band = select_band;

    if (band > 0)
    {
        // 下发PEQ系数
        FilterConfigMap data_filter;
        double tmp;

        data_filter.TYPE = GetFilter(band)->GetTypeId();

        tmp = GetFilterFreq(band)*10;
        data_filter.FREQ = tmp;

        tmp = GetFilterGain(band)*10;
        data_filter.GAIN = tmp;

        // TODO: 1940会变成1939？
        tmp = GetFilter(band)->GetQ()*1000; 
        data_filter.Q = ((int)tmp)/10;

        tmp = IsBypass(band) ? 1 : 0;  
        data_filter.bypass = tmp;

        if (dsp_num == 0)
        {
        }
        else if (dsp_num < 100)
        {
            config_map.input_dsp[dsp_num-1].filter[band-1] = data_filter;

            String cmd_text = D1608CMD_FLAG;
            cmd_text = cmd_text+"input<"+IntToStr(dsp_num)+">.peq<"+IntToStr(band)+">.bypass="+
                (IsBypass(band) ? "on" : "off")
                +"]";
            Form1->SendCmd(cmd_text);
        }
        else
        {
            config_map.output_dsp[dsp_num-101].filter[band-1] = data_filter;

            String cmd_text = D1608CMD_FLAG;
            cmd_text = cmd_text+"output<"+IntToStr(dsp_num-100)+">.peq<"+IntToStr(band)+">.bypass="+
                (IsBypass(band) ? "on" : "off")
                +"]";
            Form1->SendCmd(cmd_text);
        }
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


