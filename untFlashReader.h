//---------------------------------------------------------------------------

#ifndef untFlashReaderH
#define untFlashReaderH

#include <vector>
#include "D1608Pack.h"
//---------------------------------------------------------------------------
using namespace std;



// preset��,input��output���ݵĸ�ʽ��Ϣ
#define TypeFilterConfigMap 21

typedef struct
{
	unsigned short offset;
	unsigned char type_length;	// 1: ����Ԫ�س���
	unsigned char array_length;	// 0: ����Ԫ��, 1-n: ����Ԫ�ظ���
}OlaInfo;

typedef struct
{
	int length;		// �ṹ���ܳ���
	int struct_length;
	OlaInfo ola_info[100];	// ����
}OlaList;

typedef struct
{
	int length;		// �ṹ���ܳ���	15
	int struct_length;
	OlaInfo ola_info[15];	// ����
}InputOlaList;

typedef struct
{
	int length;		// �ṹ���ܳ���
	int struct_length;
	OlaInfo ola_info[18];	// ����
}OutputOlaList;

extern const InputOlaList input_dsp_ola_list;
extern const OutputOlaList output_dsp_ola_list;
void ReadIODspMem(char * dst, char * src, const OlaList * dst_ola_list, const OlaList * src_ola_list);



// ��ֹ  NotStorageCmd �仯��������ݴ��̴���
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
}NotStorageCmdX;
typedef struct
{
    InputConfigMap input_dsp[INPUT_DSP_NUM];
    OutputConfigMap output_dsp[OUTPUT_DSP_NUM];
    //NotStorageCmdX op_code;

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

void SavePresetById(int preset_id, int index, void * buf, unsigned char* flash_dump_data);
void LoadPresetById(int preset_id, ConfigMap& tmp_config_map, unsigned char* flash_dump_data);
void LoadGlobalConfig(GlobalConfig& global_config, unsigned char* flash_dump_data);
void LoadSinglePreset(ConfigMap& tmp_config_map, unsigned char* flash_dump_data);

// �������ͳһ����
// ��һ���������¼��Ҫ���͵ı���
// ÿ�ν����괦����һ����
// ÿ�����ݲ�����2k�ֽ�
enum Action {ACT_NOOP=0, ACT_RELOAD_PRESET, ACT_DISCONNECT};
struct TPackage
{
    unsigned char data[1500];
    int udp_port;
    int data_size;
    enum Action action;
};

#define EVENT_POOL_SIZE (LOG_SIZE/sizeof(Event))
Event * GetLogPtr(Event log_data[EVENT_POOL_SIZE]);

typedef char (*P_2K) [2048];
typedef struct
{
	unsigned char* preset_address[8][4];
	unsigned char* config_address;
}Using_Page;

typedef struct
{
	int count;
	unsigned int id;
	char check;
	char padt[7];
}Page_Header;
void GetUsingPage(Using_Page * using_page, unsigned char* flash_dump_data);

#endif

