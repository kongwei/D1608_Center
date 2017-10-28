//---------------------------------------------------------------------------

#ifndef untUtilsH
#define untUtilsH

#include <Controls.hpp>
TDateTime GetDateTimeFromMarco(String marcoDateTime);
void TestGetDateFrom__DATE__();
String DateTime2Str(TDateTime dt);
String AppBuildTime2Str(String app_build_time);

void MergeLog(TStrings * current_log_data, TStrings * log_file_data);
void MergeMac(TStrings * current_log_data, TStrings * log_file_data);

String GetCpuIdString(unsigned int cpu_id[3]);
//---------------------------------------------------------------------------
#endif
