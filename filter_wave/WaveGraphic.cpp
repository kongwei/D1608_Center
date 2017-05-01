//---------------------------------------------------------------------------


#pragma hdrstop

#include "WaveGraphic.h"
#include "Coefficient.h"
#include "SelectSort.h"
#include "FilterList.h"
#include "FilterPanelSet.h"

#include <algorithm>
using std::min;
using std::max;
#include <gdiplus.h>
using namespace Gdiplus;
#include <math.h>
//---------------------------------------------------------------------------

#pragma package(smart_init)

void InitGdipus()
{
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR    gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
}

static int CHART_HEIGHT_RATIO = 5;
static double CHART_WIDTH_RATIO = 0.5;

#define LEFT_MARGIN 40
#define RIGHT_MARGIN 20
#define BOTTOM_MARGIN 20
#define CHART_HEIGHT CHART_HEIGHT_RATIO * 40 + 1

#define BACKGROUND_COLOR Gdiplus::Color(255, 0x26, 0x26, 0x38)
#define MAIN_GRID_COLOR Gdiplus::Color(255, 0xb4, 0xb4, 0xb4)
#define GRID_COLOR Gdiplus::Color(255, 0x64, 0x64, 0x64)
#define WAVE_COLOR Gdiplus::Color(255, 0xaf, 0xcd, 0xff)
#define ACTIVE_WAVE_COLOR Gdiplus::Color(180, 0x8f, 0x8f, 0x8f)
#define ACTIVE_THUMB_COLOR Gdiplus::Color(255, 0x00, 0xff, 0x00)
#define THUMB_COLOR Gdiplus::Color(255, 0xff, 0xff, 0xff)

#define GDIPLUS_MODE SmoothingModeHighQuality//SmoothingModeAntiAlias

static void DrawWave(double point[1001], Gdiplus::Graphics &gdiplus_g);
static void DrawFillWave(double point[1001], Gdiplus::Graphics &gdiplus_g);
static void DrawThumb(PaintAgent* paint_agent, Gdiplus::Graphics &gdiplus_g);
//---------------------------------------------------------------------------
static double Canvas2Freq(double x)
{
    x -= LEFT_MARGIN;
    x /= CHART_WIDTH_RATIO;
    x = pow(FREQ_INTERVAL_RATIO, x);
    // 保留1位小数
    x *= 20;
    x = Floor(10 * x);
    return x / 10;
}
static double Canvas2Gain(double y)
{
    y /= CHART_HEIGHT_RATIO;
    y = 21 - y;
    // 保留1位小数
    y = Floor(10 * y);
    return y / 10;
}
static double Freq2Canvas(double freq)
{
    try
    {
        return log(freq / 20) / log(FREQ_INTERVAL_RATIO) * CHART_WIDTH_RATIO + LEFT_MARGIN;
    }
    catch(...)
    {
        return 0;
    }
}
static double Gain2Canvas(double gain)
{
    return CHART_HEIGHT_RATIO * (21 - gain);
}
#define THUMB_SIZE 5
static RectF FreqGainRectF(double freq, double gain)
{
    double x = Freq2Canvas(freq);
    double y = Gain2Canvas(gain);

    return RectF(x-THUMB_SIZE, y-THUMB_SIZE, THUMB_SIZE*2, THUMB_SIZE*2);
}

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
//---------------------------------------------------------------------------
PaintAgent::PaintAgent(TPaintBox* paint_box, TPaintBox* paint_box_comp, FilterSet& filter_set)
:_filter_set(filter_set)
{
    paint_control = paint_box;
    paint_control->OnMouseDown = OnMouseDown;
    paint_control->OnMouseUp = OnMouseUp;
    paint_control->OnMouseMove = OnMouseMove;
    paint_control->OnPaint = OnPaint;

    paint_control_comp = paint_box_comp;
    paint_control_comp->OnMouseDown = OnCompMouseDown;
    paint_control_comp->OnMouseUp = OnCompMouseUp;
    paint_control_comp->OnMouseMove = OnCompMouseMove;
    paint_control_comp->OnPaint = OnCompPaint;

    is_mouse_down = false;
}
//---------------------------------------------------------------------------
void __fastcall PaintAgent::OnMouseDown(TObject *Sender,
    TMouseButton Button, TShiftState Shift, int X, int Y)
{
    extern void SelectNullControl();
    SelectNullControl();
    
    for (int i=0;i<12;i++)
    {
        int band = GetSortElem(i);
        if (_filter_set.IsBandForbidden(band))
            continue;

        double freq = _filter_set.GetFilter(band)->GetFreq();
        double gain = _filter_set.GetFilter(band)->GetGain();
        RectF select_rect = FreqGainRectF(freq, gain);

        if (select_rect.Contains(X,Y))
        {
            Push2First(band);
            _filter_set.SetActiveBand(band);

            is_mouse_down = true;
            paint_control->Invalidate();
            break;
        }
    }
}
void __fastcall PaintAgent::OnMouseUp(TObject *Sender, TMouseButton Button,
      TShiftState Shift, int X, int Y)
{
    is_mouse_down = false;
}
void __fastcall PaintAgent::OnMouseMove(TObject *Sender, TShiftState Shift,
      int X, int Y)
{                 
    double freq = Canvas2Freq(X);
    double gain = Canvas2Gain(Y);

    if (!_filter_set.IsBandForbidden(_filter_set.GetActiveBand()) && is_mouse_down)
    {
        String type = _filter_set.GetFilter(_filter_set.GetActiveBand())->GetType();
        if (!_filter_set.GetFilter(_filter_set.GetActiveBand())->IsGainEnabled())
        {
            gain = 0;
        }

        if (Shift.Contains(ssShift))
        {
            freq = ClosestFreq(freq);
        }
        else
        {
            freq = max(freq, 20.0);
            freq = min(freq, 20000.0);
        }

        _filter_set.GetFilter(_filter_set.GetActiveBand())->SetFreqGain(freq, gain);
        _filter_set.RepaintPaint(_filter_set.GetActiveBand());

        paint_control->Invalidate();
    }
}
void __fastcall PaintAgent::OnPaint(TObject * Sender)
{
    // 计算比例
    CHART_HEIGHT_RATIO = (paint_control->Height - BOTTOM_MARGIN) / 40;
    CHART_WIDTH_RATIO = (paint_control->Width - LEFT_MARGIN - RIGHT_MARGIN) / 1000.0;

    TCanvas * canvas = paint_control->Canvas;
    Gdiplus::Graphics gdiplus_g(canvas->Handle);

    // 清除图像
    gdiplus_g.Clear(BACKGROUND_COLOR);

    // 绘制网格
    int left = Freq2Canvas(20);
    int right = Freq2Canvas(MAX_FREQ);
    int top = Gain2Canvas(18);
    int bottom = Gain2Canvas(-18);

    Gdiplus::Font font(L"Arial", 8);
    SolidBrush brush(MAIN_GRID_COLOR);
    PointF p;


    Pen pen(GRID_COLOR, 1);
    Pen bold_pen(MAIN_GRID_COLOR, 2);
    int y;
    y = Gain2Canvas(18);  gdiplus_g.DrawLine(&bold_pen, left-8, y, right, y);
    p = PointF(left-LEFT_MARGIN, y-6); gdiplus_g.DrawString(L" 18dB", 5, &font, p, &brush);
    y = Gain2Canvas(15);  gdiplus_g.DrawLine(&pen,      left, y, right, y);
    y = Gain2Canvas(12);  gdiplus_g.DrawLine(&pen,      left-8, y, right, y);
    p = PointF(left-LEFT_MARGIN, y-6); gdiplus_g.DrawString(L" 12dB", 5, &font, p, &brush);
    y = Gain2Canvas( 9);  gdiplus_g.DrawLine(&pen,      left, y, right, y);
    y = Gain2Canvas( 6);  gdiplus_g.DrawLine(&pen,      left-8, y, right, y);
    p = PointF(left-LEFT_MARGIN, y-6); gdiplus_g.DrawString(L"  6dB", 5, &font, p, &brush);
    y = Gain2Canvas( 3);  gdiplus_g.DrawLine(&pen,      left, y, right, y);
    y = Gain2Canvas( 0);  gdiplus_g.DrawLine(&pen,      left-8, y, right, y);
    p = PointF(left-LEFT_MARGIN, y-6); gdiplus_g.DrawString(L"  0dB", 5, &font, p, &brush);
    y = Gain2Canvas(- 3);  gdiplus_g.DrawLine(&pen,     left, y, right, y);
    y = Gain2Canvas(- 6); gdiplus_g.DrawLine(&pen,      left-8, y, right, y);
    p = PointF(left-LEFT_MARGIN, y-6); gdiplus_g.DrawString(L" -6dB", 5, &font, p, &brush);
    y = Gain2Canvas(- 9);  gdiplus_g.DrawLine(&pen,     left, y, right, y);
    y = Gain2Canvas(-12); gdiplus_g.DrawLine(&pen,      left-8, y, right, y);
    p = PointF(left-LEFT_MARGIN, y-6); gdiplus_g.DrawString(L"-12dB", 5, &font, p, &brush);
    y = Gain2Canvas(-15);  gdiplus_g.DrawLine(&pen,     left, y, right, y);
    y = Gain2Canvas(-18); gdiplus_g.DrawLine(&bold_pen, left-8, y, right, y);
    p = PointF(left-LEFT_MARGIN, y-6); gdiplus_g.DrawString(L"-18dB", 5, &font, p, &brush);

    int x;
    x = Freq2Canvas(20); gdiplus_g.DrawLine(&bold_pen, x, top, x, bottom+5);
    p = PointF(x-15, bottom+10); gdiplus_g.DrawString(L" 20Hz", 5, &font, p, &brush);
    x = Freq2Canvas(30); gdiplus_g.DrawLine(&pen, x, top, x, bottom);
    x = Freq2Canvas(40); gdiplus_g.DrawLine(&pen, x, top, x, bottom);
    x = Freq2Canvas(50); gdiplus_g.DrawLine(&pen, x, top, x, bottom+5);
    p = PointF(x-15, bottom+10); gdiplus_g.DrawString(L" 50Hz", 5, &font, p, &brush);
    x = Freq2Canvas(60); gdiplus_g.DrawLine(&pen, x, top, x, bottom);
    x = Freq2Canvas(70); gdiplus_g.DrawLine(&pen, x, top, x, bottom);
    x = Freq2Canvas(80); gdiplus_g.DrawLine(&pen, x, top, x, bottom);
    x = Freq2Canvas(90); gdiplus_g.DrawLine(&pen, x, top, x, bottom);
    x = Freq2Canvas(100); gdiplus_g.DrawLine(&bold_pen, x, top, x, bottom+5);
    p = PointF(x-15, bottom+10); gdiplus_g.DrawString(L"100Hz", 5, &font, p, &brush);
    x = Freq2Canvas(200); gdiplus_g.DrawLine(&pen, x, top, x, bottom+5);
    p = PointF(x-15, bottom+10); gdiplus_g.DrawString(L"200Hz", 5, &font, p, &brush);
    x = Freq2Canvas(300); gdiplus_g.DrawLine(&pen, x, top, x, bottom);
    x = Freq2Canvas(400); gdiplus_g.DrawLine(&pen, x, top, x, bottom);
    x = Freq2Canvas(500); gdiplus_g.DrawLine(&pen, x, top, x, bottom+5);
    p = PointF(x-15, bottom+10); gdiplus_g.DrawString(L"500Hz", 5, &font, p, &brush);
    x = Freq2Canvas(600); gdiplus_g.DrawLine(&pen, x, top, x, bottom);
    x = Freq2Canvas(700); gdiplus_g.DrawLine(&pen, x, top, x, bottom);
    x = Freq2Canvas(800); gdiplus_g.DrawLine(&pen, x, top, x, bottom);
    x = Freq2Canvas(900); gdiplus_g.DrawLine(&pen, x, top, x, bottom);
    x = Freq2Canvas(1000); gdiplus_g.DrawLine(&bold_pen, x, top, x, bottom+5);
    p = PointF(x-15, bottom+10); gdiplus_g.DrawString(L" 1kHz", 5, &font, p, &brush);
    x = Freq2Canvas(2000); gdiplus_g.DrawLine(&pen, x, top, x, bottom+5);
    p = PointF(x-15, bottom+10); gdiplus_g.DrawString(L" 2kHz", 5, &font, p, &brush);
    x = Freq2Canvas(3000); gdiplus_g.DrawLine(&pen, x, top, x, bottom);
    x = Freq2Canvas(4000); gdiplus_g.DrawLine(&pen, x, top, x, bottom);
    x = Freq2Canvas(5000); gdiplus_g.DrawLine(&pen, x, top, x, bottom+5);
    p = PointF(x-15, bottom+10); gdiplus_g.DrawString(L" 5kHz", 5, &font, p, &brush);
    x = Freq2Canvas(6000); gdiplus_g.DrawLine(&pen, x, top, x, bottom);
    x = Freq2Canvas(7000); gdiplus_g.DrawLine(&pen, x, top, x, bottom);
    x = Freq2Canvas(8000); gdiplus_g.DrawLine(&pen, x, top, x, bottom);
    x = Freq2Canvas(9000); gdiplus_g.DrawLine(&pen, x, top, x, bottom);
    x = Freq2Canvas(10000); gdiplus_g.DrawLine(&bold_pen, x, top, x, bottom+5);
    p = PointF(x-15, bottom+10); gdiplus_g.DrawString(L"10kHz", 5, &font, p, &brush);
    x = Freq2Canvas(20000); gdiplus_g.DrawLine(&bold_pen, x, top, x, bottom+5);
    p = PointF(x-15, bottom+10); gdiplus_g.DrawString(L"20kHz", 5, &font, p, &brush);
    x = Freq2Canvas(24000); gdiplus_g.DrawLine(&bold_pen, x, top, x, bottom);

    double point[1001];
    // 绘制选中filter图像
    if (!_filter_set.IsBandForbidden(_filter_set.GetActiveBand()) && !_filter_set.IsBypass(_filter_set.GetActiveBand()))
    {
        Coefficient::InitPointData(point);
        _filter_set.GetFilter(_filter_set.GetActiveBand())->AddToMiddle(point);

        DrawFillWave(point, gdiplus_g);
    }

    // 绘制所有filter图像
    Coefficient::InitPointData(point);
    for (int i=1;i<12;i++)
    {
        if (!_filter_set.IsBypass(i) && !_filter_set.IsBandForbidden(i))
        {
            _filter_set.GetFilter(i)->AddToMiddle(point);
        }
    }

    DrawThumb(this, gdiplus_g);
    DrawWave(point, gdiplus_g);

    y = Gain2Canvas(-18); gdiplus_g.DrawLine(&bold_pen, left-8, y, right, y);
}
//---------------------------------------------------------------------------
void __fastcall PaintAgent::OnMouseWheel(TObject *Sender, TShiftState Shift,
      int WheelDelta, TPoint &MousePos, bool &Handled)
{
    if (is_mouse_down && _filter_set.GetActiveBand()!=0)
    {
        double current_q = _filter_set.GetFilter(_filter_set.GetActiveBand())->GetQ();
        current_q *= 100;
        current_q = Floor(current_q+0.5);
        current_q = current_q / 100;
        if (WheelDelta >0)
        {
            current_q = NextLargeQ(current_q);
        }
        else if (WheelDelta < 0)
        {
            current_q = NextSmallQ(current_q);
        }
        _filter_set.GetFilter(_filter_set.GetActiveBand())->SetQ(current_q);
        _filter_set.RepaintPaint(_filter_set.GetActiveBand());

        paint_control->Invalidate();
        Handled = true;
        return;
    }
}
void DrawWave(double point[1001], Gdiplus::Graphics &gdiplus_g)
{
    Gdiplus::PointF point_gp[1001];
    for (int i=0;i<1001;i++)
    {
        point_gp[i].X = i*CHART_WIDTH_RATIO + LEFT_MARGIN;
        point_gp[i].Y = Gain2Canvas(max(point[i], -18.0));
    }

    Pen pen(WAVE_COLOR, 2);
    GraphicsPath path;
    path.AddLines(point_gp, 1001);

    gdiplus_g.SetSmoothingMode(GDIPLUS_MODE);
    gdiplus_g.DrawPath(&pen, &path);
}
//---------------------------------------------------------------------------
void DrawFillWave(double point[1001], Gdiplus::Graphics &gdiplus_g)
{
    Gdiplus::PointF point_gp[1002];
    point_gp[0].X = 0 + LEFT_MARGIN;
    point_gp[0].Y = Gain2Canvas(0);
    for (int i=0;i<1001;i++)
    {
        point_gp[i+1].X = i*CHART_WIDTH_RATIO + LEFT_MARGIN;
        point_gp[i+1].Y = Gain2Canvas(max(point[i], -18.0));
    }
    point_gp[1001].X = 1000*CHART_WIDTH_RATIO + LEFT_MARGIN;
    point_gp[1001].Y = Gain2Canvas(0);

    SolidBrush brush(ACTIVE_WAVE_COLOR);
    GraphicsPath path;
    path.AddLines(point_gp, 1002);

    gdiplus_g.SetSmoothingMode(GDIPLUS_MODE);
    gdiplus_g.FillPath(&brush, &path);
}
//---------------------------------------------------------------------------
void DrawThumb(PaintAgent* paint_agent, Gdiplus::Graphics &gdiplus_g)
{
    for (int i=11;i>=0;i--)
    {
        int band = GetSortElem(i);
        if (paint_agent->_filter_set.IsBandForbidden(band))
            continue;

        double freq = paint_agent->_filter_set.GetFilter(band)->GetFreq();
        double gain = paint_agent->_filter_set.GetFilter(band)->GetGain();

        Gdiplus::Color color;
        if (band == paint_agent->_filter_set.GetActiveBand())
        {
            color = ACTIVE_THUMB_COLOR;
        }
        else
        {
            color = THUMB_COLOR;
        }
        RectF point_rect = FreqGainRectF(freq, gain);
        SolidBrush brush(color);
        gdiplus_g.FillEllipse(&brush, point_rect);

        int x = Freq2Canvas(freq);
        int y = Gain2Canvas(gain);

        if (gain >= 0)
        {
            PointF p = PointF(x-3, y-20);
            Gdiplus::Font f(L"Arial", 8);
            WideString name = paint_agent->_filter_set.GetFilter(band)->name;
            gdiplus_g.DrawString(name, 1, &f, p, &brush);
        }
        else
        {
            PointF p = PointF(x-3, y+10);
            Gdiplus::Font f(L"Arial", 8);
            WideString name = paint_agent->_filter_set.GetFilter(band)->name;
            gdiplus_g.DrawString(name, 1, &f, p, &brush);
        }
    }
}
//---------------------------------------------------------------------------
static double CHART_RATIO;
static float Comp2CanvasY(double y)
{
    y = -y * CHART_RATIO + Gain2Canvas(18);
    // 保留1位小数
    y = Floor(10 * y);
    return y / 10;
}
static float Comp2CanvasX(double x)
{
    x = (x+48) * CHART_RATIO + LEFT_MARGIN;
    // 保留1位小数
    x = Floor(10 * x);
    return x / 10;
}
static float CanvasY2Gain(double y)
{
    y = (-y + Gain2Canvas(18)) / CHART_RATIO;
    return y;
}
static float CanvasX2Threshold(double x)
{
    x = (x - LEFT_MARGIN) / CHART_RATIO - 48;
    return x;
}

void __fastcall PaintAgent::OnCompPaint(TObject * Sender)
{
    //CHART_RATIO = (paint_control_comp->Width - RIGHT_MARGIN - RIGHT_MARGIN) / 72.0;
    // BAD: 依赖PEQ图
    CHART_RATIO = (Gain2Canvas(-18) - Gain2Canvas(18)) / 48;

    TCanvas * canvas = paint_control_comp->Canvas;
    Gdiplus::Graphics gdiplus_g(canvas->Handle);
    gdiplus_g.SetSmoothingMode(GDIPLUS_MODE);

    // 清除图像
    gdiplus_g.Clear(BACKGROUND_COLOR);

    // 绘制网格
    int left = Comp2CanvasX(-48);
    int right = Comp2CanvasX(0);
    int top = Comp2CanvasY(0);
    int bottom = Comp2CanvasY(-48);

    Gdiplus::Font font(L"Arial", 8);
    SolidBrush brush(MAIN_GRID_COLOR);
    PointF p;

    Pen pen(GRID_COLOR, 1);
    Pen bold_pen(MAIN_GRID_COLOR, 2);

    int y;
    y = Comp2CanvasY(0);  gdiplus_g.DrawLine(&bold_pen, left-8, y, right, y);
    p = PointF(left-LEFT_MARGIN, y-6); gdiplus_g.DrawString(L"  0dB", 5, &font, p, &brush);
    y = Comp2CanvasY(-6);  gdiplus_g.DrawLine(&pen, left-8, y, right, y);
    y = Comp2CanvasY(-12);  gdiplus_g.DrawLine(&pen, left-8, y, right, y);
    y = Comp2CanvasY(-18);  gdiplus_g.DrawLine(&pen, left-8, y, right, y);
    y = Comp2CanvasY(-24);  gdiplus_g.DrawLine(&bold_pen, left-8, y, right, y);
    p = PointF(left-LEFT_MARGIN, y-6); gdiplus_g.DrawString(L"-24dB", 5, &font, p, &brush);
    y = Comp2CanvasY(-30);  gdiplus_g.DrawLine(&pen, left-8, y, right, y);
    y = Comp2CanvasY(-36);  gdiplus_g.DrawLine(&pen, left-8, y, right, y);
    y = Comp2CanvasY(-42);  gdiplus_g.DrawLine(&pen, left-8, y, right, y);
    y = Comp2CanvasY(-48);  gdiplus_g.DrawLine(&bold_pen, left-8, y, right, y);
    p = PointF(left-LEFT_MARGIN, y-6); gdiplus_g.DrawString(L"-48dB", 5, &font, p, &brush);
    /*y = Comp2CanvasY(-54);  gdiplus_g.DrawLine(&pen, left-8, y, right, y);
    y = Comp2CanvasY(-60);  gdiplus_g.DrawLine(&pen, left-8, y, right, y);
    y = Comp2CanvasY(-66);  gdiplus_g.DrawLine(&pen, left-8, y, right, y);
    y = Comp2CanvasY(-72);  gdiplus_g.DrawLine(&bold_pen, left-8, y, right, y);
    p = PointF(left-LEFT_MARGIN, y-6); gdiplus_g.DrawString(L"-72dB", 5, &font, p, &brush);*/

    int x;
    x = Comp2CanvasX(0); gdiplus_g.DrawLine(&bold_pen, x, top, x, bottom+5);
    p = PointF(x-15, bottom+10); gdiplus_g.DrawString(L"  0dB", 5, &font, p, &brush);
    x = Comp2CanvasX(-6); gdiplus_g.DrawLine(&pen, x, top, x, bottom);
    x = Comp2CanvasX(-12); gdiplus_g.DrawLine(&pen, x, top, x, bottom);
    x = Comp2CanvasX(-18); gdiplus_g.DrawLine(&pen, x, top, x, bottom+5);
    x = Comp2CanvasX(-24); gdiplus_g.DrawLine(&bold_pen, x, top, x, bottom+5);
    p = PointF(x-15, bottom+10); gdiplus_g.DrawString(L"-24dB", 5, &font, p, &brush);
    x = Comp2CanvasX(-30); gdiplus_g.DrawLine(&pen, x, top, x, bottom);
    x = Comp2CanvasX(-36); gdiplus_g.DrawLine(&pen, x, top, x, bottom);
    x = Comp2CanvasX(-42); gdiplus_g.DrawLine(&pen, x, top, x, bottom+5);
    x = Comp2CanvasX(-48); gdiplus_g.DrawLine(&bold_pen, x, top, x, bottom+5);
    p = PointF(x-15, bottom+10); gdiplus_g.DrawString(L"-48dB", 5, &font, p, &brush);
    /*x = Comp2CanvasX(-54); gdiplus_g.DrawLine(&pen, x, top, x, bottom);
    x = Comp2CanvasX(-60); gdiplus_g.DrawLine(&pen, x, top, x, bottom);
    x = Comp2CanvasX(-66); gdiplus_g.DrawLine(&pen, x, top, x, bottom+5);
    x = Comp2CanvasX(-72); gdiplus_g.DrawLine(&bold_pen, x, top, x, bottom+5);
    p = PointF(x-15, bottom+10); gdiplus_g.DrawString(L"-72dB", 5, &font, p, &brush);*/

    // 标准线
    gdiplus_g.DrawLine(&pen, Comp2CanvasX(-48), Comp2CanvasY(-48), Comp2CanvasX(0), Comp2CanvasY(0));

    
    // 根据comp参数进行绘制
    double ratio = _filter_set.ratio;
    // 从 -100 ~ Threshold (dB) 绘制直线
    double threshold = _filter_set.threshold;
    // 整体增益提高
    double gain = _filter_set.gain;

    // 计算3个点
    Gdiplus::Point point_org(-48, -48+gain);
    Gdiplus::Point point_threshold(threshold, threshold+gain);
    Gdiplus::Point point_end(0, threshold+(0-threshold)*ratio+gain);

    point_org.X = Comp2CanvasX(point_org.X);
    point_org.Y = Comp2CanvasY(point_org.Y);
    point_threshold.X = Comp2CanvasX(point_threshold.X);
    point_threshold.Y = Comp2CanvasY(point_threshold.Y);
    point_end.X = Comp2CanvasX(point_end.X);
    point_end.Y = Comp2CanvasY(point_end.Y);

    Gdiplus::Pen wave_pen(WAVE_COLOR, 2);
    gdiplus_g.DrawLine(&wave_pen, point_org, point_threshold);
    gdiplus_g.DrawLine(&wave_pen, point_threshold, point_end);


    
    brush.SetColor(is_comp_ratio_selected?ACTIVE_THUMB_COLOR:THUMB_COLOR);
    gdiplus_g.FillEllipse(&brush, RectF(point_end.X-THUMB_SIZE, point_end.Y-THUMB_SIZE, THUMB_SIZE*2, THUMB_SIZE*2));

    brush.SetColor(is_comp_threshold_selected?ACTIVE_THUMB_COLOR:THUMB_COLOR);
    gdiplus_g.FillEllipse(&brush, RectF(point_threshold.X-THUMB_SIZE, point_threshold.Y-THUMB_SIZE, THUMB_SIZE*2, THUMB_SIZE*2));

    brush.SetColor(is_comp_gain_selected?ACTIVE_THUMB_COLOR:THUMB_COLOR);
    gdiplus_g.FillEllipse(&brush, RectF(point_org.X-THUMB_SIZE, point_org.Y-THUMB_SIZE, THUMB_SIZE*2, THUMB_SIZE*2));
}
//---------------------------------------------------------------------------
void __fastcall PaintAgent::OnCompMouseDown(TObject *Sender,
    TMouseButton Button, TShiftState Shift, int X, int Y)
{
    // 根据comp参数进行绘制
    double ratio = _filter_set.ratio;
    // 从 -100 ~ Threshold (dB) 绘制直线
    double threshold = _filter_set.threshold;
    // 整体增益提高
    double gain = _filter_set.gain;

    // 计算3个点
    Gdiplus::Point point_org(-48, -48+gain);
    Gdiplus::Point point_threshold(threshold, threshold+gain);
    Gdiplus::Point point_end(0, threshold+(0-threshold)*ratio+gain);

    point_org.X = Comp2CanvasX(point_org.X);
    point_org.Y = Comp2CanvasY(point_org.Y);
    point_threshold.X = Comp2CanvasX(point_threshold.X);
    point_threshold.Y = Comp2CanvasY(point_threshold.Y);
    point_end.X = Comp2CanvasX(point_end.X);
    point_end.Y = Comp2CanvasY(point_end.Y);

    RectF point_org_rect = RectF(point_org.X-THUMB_SIZE, point_org.Y-THUMB_SIZE, THUMB_SIZE*2, THUMB_SIZE*2);
    RectF point_threshold_rect = RectF(point_threshold.X-THUMB_SIZE, point_threshold.Y-THUMB_SIZE, THUMB_SIZE*2, THUMB_SIZE*2);
    RectF point_end_rect = RectF(point_end.X-THUMB_SIZE, point_end.Y-THUMB_SIZE, THUMB_SIZE*2, THUMB_SIZE*2);

    is_comp_gain_selected = false;
    is_comp_threshold_selected = false;
    is_comp_ratio_selected = false;
    if (point_org_rect.Contains(X,Y))
    {
        is_comp_mouse_down = true;
        is_comp_gain_selected = true;
        paint_control_comp->Invalidate();
    }
    else if (point_threshold_rect.Contains(X,Y))        // threshold 优先，避免在0dB时没法选择该点
    {
        is_comp_mouse_down = true;
        is_comp_threshold_selected = true;
        paint_control_comp->Invalidate();
    }
    else if (point_end_rect.Contains(X,Y))
    {
        is_comp_mouse_down = true;
        is_comp_ratio_selected = true;
        paint_control_comp->Invalidate();
    }
}
void __fastcall PaintAgent::OnCompMouseUp(TObject *Sender, TMouseButton Button,
      TShiftState Shift, int X, int Y)
{
    is_comp_mouse_down = false;
    is_comp_gain_selected = false;
    is_comp_threshold_selected = false;
    is_comp_ratio_selected = false;
}
void __fastcall PaintAgent::OnCompMouseMove(TObject *Sender, TShiftState Shift,
      int X, int Y)
{                 
    if (is_comp_gain_selected)
    {
        // 计算gain的最大值。收到另外两个系数影响 (threshold, ratio)
        // ratio不能超出范围

        // 安装 Y 计算 Gain
        _filter_set.gain = CanvasY2Gain(Y) + 48;
        _filter_set.gain = max(_filter_set.gain, 0.0);
        _filter_set.gain = min(_filter_set.gain, -_filter_set.threshold*(1-_filter_set.ratio));//24.0);
        _filter_set.UpdateCompGain();
        //paint_control_comp->Invalidate();
    }
    else if (is_comp_threshold_selected)
    {
        // 计算gain的最大值。收到另外两个系数影响 (threshold, ratio)
        // ratio不能超出范围

        // 安装 Y 计算 Gain
        _filter_set.threshold = CanvasX2Threshold(X);
        _filter_set.threshold = max(_filter_set.threshold, -32.0);
        if (_filter_set.ratio == 1)
        {
            _filter_set.threshold = min(_filter_set.threshold, 0.0);
        }
        else
        {
            _filter_set.threshold = min(_filter_set.threshold, -_filter_set.gain / (1-_filter_set.ratio));//0.0);
        }
        _filter_set.UpdateCompThreshold();
        //paint_control_comp->Invalidate();
    }
    else if (is_comp_ratio_selected)
    {
        // 计算gain的最大值。收到另外两个系数影响 (threshold, ratio)
        // ratio不能超出范围

        // 安装 Y 计算 Gain
        if (_filter_set.threshold == 0)
        {
            _filter_set.ratio = 1;
        }
        else
        {
            _filter_set.ratio = 1 - (CanvasY2Gain(Y) - _filter_set.gain) / _filter_set.threshold;
            _filter_set.ratio = max(_filter_set.ratio, 0.0);
            _filter_set.ratio = min(_filter_set.ratio, 1 + _filter_set.gain / _filter_set.threshold);
            _filter_set.UpdateCompRatio();
        }
        //paint_control_comp->Invalidate();
    }
}

