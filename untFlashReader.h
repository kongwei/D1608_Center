//---------------------------------------------------------------------------

#ifndef untFlashReaderH
#define untFlashReaderH

#include <vector>
extern "C"{
#include "../enc28j60_iap_app/inc/D1608Pack.h"
}
//---------------------------------------------------------------------------
using namespace std;

void LoadPresetById(int preset_id, ConfigMap& tmp_config_map, unsigned char* flash_dump_data);
void LoadGlobalConfig(GlobalConfig& global_config, unsigned char* flash_dump_data);

// 多包交互统一处理
// 用一个大数组记录需要发送的报文
// 每次交互完处理下一个包
// 每包数据不超过2k字节
struct TPackage
{
    unsigned char data[2048];
    int udp_port;
    int data_size;
};

#endif
