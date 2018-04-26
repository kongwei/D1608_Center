//---------------------------------------------------------------------------

#ifndef FilterListH
#define FilterListH
//---------------------------------------------------------------------------
#include "Coefficient.h"
#include "WaveGraphic.h"
#include "FilterPanelSet.h"
#include <Classes.hpp>

class FilterSet
{
public:
    FilterSet()
    {
        select_band = 0;
        for (int i=0;i<12;i++)
            bypass[i] = true;
    }
    Coefficient* GetFilter(int band);

    int GetActiveBand();
    void SetActiveBand(int band);

    bool IsBandForbidden(int band);

    bool IsBypass(int band);
    void SetBypass(int band, bool isBypass);

    void Register(PaintAgent* paint_agent, PanelAgent* panel_agent);
    void RepaintPaint(int band);
    void SendPeqCmd(int band=0);
    void SendBypassCmd(int band=0);

    // 输出部分的Comp参数
    double attack_time;
    double release_time;
    double ratio;
    double threshold;
    double gain;

    void UpdateCompRatio();
    void UpdateCompThreshold();
    void UpdateCompGain();
private:
    
    // BAD: 浪费了第一个元素
    bool bypass[12];
    Coefficient filter[12];
    int select_band;
    PaintAgent* paint_agent_ref;
    PanelAgent* panel_agent_ref;

    //void OnParameterChange(int band);
    double GetFilterFreq(int band);
    double GetFilterGain(int band);
    String GetFilterType(int band);
};

#endif
