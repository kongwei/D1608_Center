//---------------------------------------------------------------------------

#ifndef untUtilsH
#define untUtilsH

#include <Controls.hpp>
TDateTime GetDateTimeFromMarco(String marcoDateTime);
void TestGetDateFrom__DATE__();
String DateTime2Str(TDateTime dt);
String AppBuildTime2Str(String app_build_time);

void MergeLog(TStrings * append_data, TStrings * log_data);
//---------------------------------------------------------------------------
#endif
