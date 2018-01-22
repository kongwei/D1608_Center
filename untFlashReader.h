//---------------------------------------------------------------------------

#ifndef untFlashReaderH
#define untFlashReaderH

#include <vector>
extern "C"{
#include "../enc28j60_iap_app/inc/D1608Pack.h"
}
//---------------------------------------------------------------------------
using namespace std;

//00000000000000000000
// 防止  NotStorageCmd 变化引起的数据存盘错误
typedef struct
{
    int noop;
    short adc[16];
    int hardware_mute;
    int WatchLevel[INPUT_DSP_NUM + OUTPUT_DSP_NUM];
    int switch_preset;
    struct
    {
        int ratio;
        int threshold;
        int attack;
        int release;
    } comp;
	unsigned char pad_yss920[16];
	unsigned char pad_ad_da_card[8];
	int reboot;
	int flash_oled;
	//int version_list;
}NotStorageCmdX;
typedef struct
{
    MasterMixConfigMap master_mix;

    InputConfigMap input_dsp[INPUT_DSP_NUM];
    OutputConfigMap output_dsp[OUTPUT_DSP_NUM];
    NotStorageCmdX op_code;

    void operator = (ConfigMap & config_map)
    {
        memcpy(this, &config_map, sizeof(config_map));
    }
    ConfigMap ToConfigMap()
    {
        ConfigMap result;
        memcpy(&config_map, this, sizeof(config_map));
        return config_map;
    }
}ConfigMapX;
//000000000000000000000

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

#define EVENT_POOL_SIZE (LOG_SIZE/sizeof(Event))
Event * GetLogPtr(Event log_data[EVENT_POOL_SIZE]);

#endif
