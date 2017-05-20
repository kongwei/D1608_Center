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

// �������ͳһ����
// ��һ���������¼��Ҫ���͵ı���
// ÿ�ν����괦����һ����
// ÿ�����ݲ�����2k�ֽ�
struct TPackage
{
    unsigned char data[2048];
    int udp_port;
    int data_size;
};

#endif
