//---------------------------------------------------------------------------

#ifndef untTextCmdParseH
#define untTextCmdParseH
//---------------------------------------------------------------------------

#include <vector>

std::vector<UINT> ProcessTextCommand(String command);
int DelayProcessTextCommand(String command_head, std::vector<UINT> & cmd_queue);

//---------------------------------------------------------------------------
#endif
