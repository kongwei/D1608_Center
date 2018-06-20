//---------------------------------------------------------------------------

#ifndef WaveGraphicH
#define WaveGraphicH
//---------------------------------------------------------------------------
#include <ExtCtrls.hpp>
#include "FilterPanelSet.h"

void InitGdipus();

class PaintAgent : public TObject
{
public:
    PaintAgent(TPaintBox* paint_box, TPaintBox* paint_box_comp, class FilterSet& filter_set);

    void __fastcall OnMouseWheel(TObject *Sender, TShiftState Shift,
      int WheelDelta, TPoint &MousePos, bool &Handled);
    void Repaint()
    {
        paint_control->Repaint();
    }

    bool IsMouseDown()
    {
        return is_mouse_down || is_comp_mouse_down;
    }

    FilterSet& _filter_set;
private:
    TPaintBox* paint_control;
    void __fastcall OnMouseDown(TObject *Sender, TMouseButton Button, TShiftState Shift, int X, int Y);
    void __fastcall OnMouseUp(TObject *Sender, TMouseButton Button, TShiftState Shift, int X, int Y);
    void __fastcall OnMouseMove(TObject *Sender, TShiftState Shift, int X, int Y);
    void __fastcall OnPaint(TObject * Sender);
    bool is_mouse_down;

    TPaintBox* paint_control_comp;
    void __fastcall OnCompMouseDown(TObject *Sender, TMouseButton Button, TShiftState Shift, int X, int Y);
    void __fastcall OnCompMouseUp(TObject *Sender, TMouseButton Button, TShiftState Shift, int X, int Y);
    void __fastcall OnCompMouseMove(TObject *Sender, TShiftState Shift, int X, int Y);
    void __fastcall OnCompPaint(TObject * Sender);
    bool is_comp_mouse_down;
    bool is_comp_gain_selected;
    bool is_comp_threshold_selected;
    bool is_comp_ratio_selected;
public:
    void PaintThumbnail(TPaintBox *, FilterSet & filter_set);
};

#endif
