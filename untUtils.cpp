//---------------------------------------------------------------------------


#pragma hdrstop

#include "untUtils.h"
#include "assert.h"
//---------------------------------------------------------------------------

#pragma package(smart_init)

//--------------------------------------
// Timer String Utils
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
    if (marcoDateTime == "                  ")
        return 0;
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
    if ((double)dt == 0.0)
    {
        return "9999-000000";
    }
    else
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

//--------------------------------------
static String GetLogWithoutTime(String log)
{
    TStrings * tmpStr = new TStringList();
    tmpStr->Delimiter = '\t';
    tmpStr->DelimitedText = log;

    if (tmpStr->Count > 4)
    {
        log = tmpStr->Strings[0] + "\t" + tmpStr->Strings[1] + "\t" + tmpStr->Strings[2] + "\t" + tmpStr->Strings[3];
    }

    delete tmpStr;

    return log;
}
static int FindLogStr(TStrings * log, String str)
{
    for (int i=0;i<log->Count;i++)
    {
        String log_str = GetLogWithoutTime(log->Strings[i]);
        str = GetLogWithoutTime(str);
        if (log_str == str)
            return i;
    }

    return -1;
}
static void DeleteMac(TStrings * log_data)
{
    TStrings * tmp = new TStringList();
    for (int i=0;i<log_data->Count;i++)
    {
        if (log_data->Strings[i][1] == '\t')
        {
        }
        else
        {
            tmp->Add(log_data->Strings[i]);
        }
    }
    log_data->Clear();
    log_data->AddStrings(tmp);

    delete tmp;
}
static void DeleteLog(TStrings * log_data)
{
    TStrings * tmp = new TStringList();
    for (int i=0;i<log_data->Count;i++)
    {
        if (log_data->Strings[i][1] == '\t')
        {
            tmp->Add(log_data->Strings[i]);
        }
        else
        {
        }
    }
    log_data->Clear();
    log_data->AddStrings(tmp);

    delete tmp;
}
// Log File Utils
void MergeLogX(TStrings * append_data, TStrings * log_data)
{
    DeleteMac(log_data);
    if (append_data->Count == 0)
    {
        append_data->AddStrings(log_data);
    }
    else
    {
        // 在log_data中查找
        String first_data = append_data->Strings[append_data->Count - 1];
        int index_of_first_data = FindLogStr(log_data, first_data);

        //  校验一下，是不是从 0 ~ index_of_first_data 都在 log_data里
        for (int i=0;i<index_of_first_data;i++)
        {
            String old_log_str = log_data->Strings[i];
            if (FindLogStr(append_data, old_log_str) == -1)
            {
                // 日志文件无法合并，直接追加
                index_of_first_data = -1;
                log_data->Insert(0, "日志文件无法合并");
                break;
            }
        }

        for (int i=index_of_first_data+1;i<log_data->Count;i++)
        {
            append_data->Add(log_data->Strings[i]);
        }
    }
}
// Mac File Utils
void MergeMac(TStrings * append_data, TStrings * log_data)
{
    DeleteLog(log_data);
    if (append_data->Count == 0)
    {
        append_data->AddStrings(log_data);
    }
    else
    {
        //  校验一下，是不是从 0 ~ index_of_first_data 都在 log_data里
        for (int i=0;i<log_data->Count;i++)
        {
            String old_log_str = log_data->Strings[i];
            if (append_data->IndexOf(old_log_str) == -1)
            {
                log_data->Add(old_log_str);
            }
        }
    }
}

// 测试log合并
static TStrings * CreateList(String append_data[], int count)
{
    TStrings * ret = new TStringList;
    for (int i=0;i<count;i++)
    {
        ret->Add(append_data[i]);
    }

    return ret;
}
static bool CompareList(TStrings * strs, String data[], int count)
{
    if (strs->Count != count)
    {
        return false;
    }

    for (int i=0;i<count;i++)
    {
        if (strs->Strings[i] != data[i])
        {
            return false;
        }
    }

    return true;
}
void TEST_MergeLog_1()
{
    String append_data[] = {"5", "4", "3", "2", "1"};
    String log_data[] = {"2", "1", "3", "2", "1"};
    String result[] = {"5", "4", "3", "2", "1", "3", "2", "1"};

    TStrings * a = CreateList(append_data, 5);
    TStrings * l = CreateList(log_data, 5);

    MergeLogX(a, l);

    assert(CompareList(a, result, 8));
}

void MergeLog(TStrings * append_data, TStrings * log_data)
{
    //TEST_MergeLog_1();
    MergeLogX(append_data, log_data);
}

