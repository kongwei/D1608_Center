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
    void RepaintPaint(int band=0);
private:
    // TODO: 第一个元素没有使用到
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
