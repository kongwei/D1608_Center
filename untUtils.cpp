//---------------------------------------------------------------------------


#pragma hdrstop

#include "untUtils.h"
#include "assert.h"
//---------------------------------------------------------------------------

#pragma package(smart_init)

TDate GetDateFrom__DATE__(String __DATA__STR)
{
    String day_str = __DATA__STR.SubString(5, 2);
    String month_str = __DATA__STR.SubString(1, 3);
    String year_str = __DATA__STR.SubString(8, 4);

    const String template_month_str = "JanFebMarAprMayJunJulAugSepOctNovDec";

    int month = template_month_str.Pos(month_str);
    if (month == 0)
        return TDate(0);
    else
    {
        month = month - 1;
        month = month / 3;
        month = month + 1;
    }

    return  TDate(year_str.ToInt(), month, day_str.ToInt());
}
TDateTime GetDateTimeFromMarco(String marcoDateTime)
{
    try
    {
        TDate date = GetDateFrom__DATE__(marcoDateTime);

        String time_str = marcoDateTime.SubString(13, 8);
        TTime time = TTime(time_str);

        return  date+time;
    }
    catch(...)
    {
        return 0;
    }
}
void TestGetDateFrom__DATE__()
{
    assert(GetDateFrom__DATE__("Jan  6 2017 22:34:01") == TDate(2017, 1 , 6));
    assert(GetDateFrom__DATE__("Feb  6 2017 22:34:01") == TDate(2017, 2 , 6));
    assert(GetDateFrom__DATE__("Mar  6 2017 22:34:01") == TDate(2017, 3 , 6));
    assert(GetDateFrom__DATE__("Apr  6 2017 22:34:01") == TDate(2017, 4 , 6));
    assert(GetDateFrom__DATE__("May  6 2017 22:34:01") == TDate(2017, 5 , 6));
    assert(GetDateFrom__DATE__("Jun  6 2017 22:34:01") == TDate(2017, 6 , 6));
    assert(GetDateFrom__DATE__("Jul  6 2017 22:34:01") == TDate(2017, 7 , 6));
    assert(GetDateFrom__DATE__("Aug  6 2017 22:34:01") == TDate(2017, 8 , 6));
    assert(GetDateFrom__DATE__("Sep  6 2017 22:34:01") == TDate(2017, 9 , 6));
    assert(GetDateFrom__DATE__("Oct  6 2017 22:34:01") == TDate(2017, 10, 6));
    assert(GetDateFrom__DATE__("Nov  6 2017 22:34:01") == TDate(2017, 11, 6));
    assert(GetDateFrom__DATE__("Dec  6 2017 22:34:01") == TDate(2017, 12, 6));
}
String DateTime2Str(TDateTime dt)
{
    unsigned short y,m,d;
    dt.DecodeDate(&y, &m, &d);

    if (y>2010 && y<2020)
        y = y - 2010;
    else
        y = y % 100;

    String result;
    result.sprintf("%02d", d);
    result = IntToStr(y)+IntToHex(m, 1)+result + dt.FormatString("-hhnnss");

    return result;
}
String AppBuildTime2Str(String app_build_time)
{
    // 前4个字符的处理
    if (app_build_time.Length() != 12)
    {
        return app_build_time;
    }

    // year
    String str_y = app_build_time.SubString(2,1);

    // month
    String mm = app_build_time.SubString(3,2);
    // mm to m
    int i_m = mm.ToInt();
    String str_m = IntToHex(i_m, 1);

    // day
    String str_d = app_build_time.SubString(5, 2);

    String result = str_y+str_m+str_d+"-"+app_build_time.SubString(7, 8);
    return result;
}

