//---------------------------------------------------------------------------

#ifndef D1608PackH
#define D1608PackH
//---------------------------------------------------------------------------
#define INPUT_DSP_NUM 16
#define OUTPUT_DSP_NUM 16

#pragma pack(push, 1)
typedef struct
{
	unsigned short TYPE;
    signed short GAIN;
    unsigned int FREQ;
    unsigned short Q;
	short bypass;
}FilterConfigMap;

typedef struct
{
    unsigned char eq_switch;
    unsigned char comp_switch;
    unsigned char auto_switch;
    unsigned char invert_switch;
    unsigned char noise_switch;
    unsigned char mute_switch;
    unsigned char phantom_switch;
    unsigned char pad1;
    short level_a;
    short level_b;
    unsigned char gain;
    unsigned int delay;
    FilterConfigMap filter[9];
}InputConfigMap;
typedef struct
{
    unsigned char eq_switch;
    unsigned char comp_switch;
    unsigned char invert_switch;
    unsigned char mute_switch;
    unsigned char pad1;
    unsigned char pad2;
    unsigned char pad3;
    unsigned char pad4;
    short level_a;
    short level_b;
    unsigned char gain;
    unsigned int pad5;
    FilterConfigMap filter[9];
}OutputConfigMap;
typedef struct
{
    unsigned char mute_switch;
    int level_a;
}MasterConfigMap;

typedef struct
{
    int noop;
    //InputConfigMap mix_dsp;
	MasterConfigMap master;
	short mix[INPUT_DSP_NUM+1][OUTPUT_DSP_NUM];
	unsigned char mix_mute[INPUT_DSP_NUM+1][OUTPUT_DSP_NUM];

    InputConfigMap input_dsp[INPUT_DSP_NUM];
    OutputConfigMap output_dsp[OUTPUT_DSP_NUM];
    int WatchLevel[INPUT_DSP_NUM + OUTPUT_DSP_NUM];
	//int preset_id;
}ConfigMap;

extern ConfigMap config_map;

// /////////////////////////////////////////////
// // ConfigMap_BIT
// typedef struct
// {
//     unsigned char eq_switch;
//     unsigned char comp_switch;
//     unsigned char auto_switch;
//     unsigned char invert_switch;
//     unsigned char noise_switch;
//     unsigned char mute_switch;
//     unsigned char phantom_switch;
//     unsigned char level_a;
//     unsigned char level_b;
//     unsigned char gain;
//     unsigned int delay;
//     unsigned char filter[9];
// }InputConfigMap_BIT;
// typedef struct
// {
//     unsigned char eq_switch;
//     unsigned char comp_switch;
//     unsigned char invert_switch;
//     unsigned char mute_switch;
//     unsigned char pad1;
//     unsigned char pad2;
//     unsigned char pad3;
//     unsigned char pad4;
//     unsigned char level_a;
//     unsigned char  level_b;
//     unsigned char gain;
//     unsigned char filter[9];
// }OutputConfigMap_BIT;
// typedef struct
// {
//     unsigned char mute_switch;
//     unsigned char level_a;
// }MasterConfigMap_BIT;

// typedef struct
// {
// 	MasterConfigMap_BIT master;
// 	unsigned char mix[INPUT_DSP_NUM+1][OUTPUT_DSP_NUM];
// 	unsigned char mix_mute[INPUT_DSP_NUM+1][OUTPUT_DSP_NUM];

//     InputConfigMap_BIT input_dsp[INPUT_DSP_NUM];
//     OutputConfigMap_BIT output_dsp[OUTPUT_DSP_NUM];
//     unsigned char WatchLevel[INPUT_DSP_NUM + OUTPUT_DSP_NUM];
// }ConfigMap_BIT;

// extern ConfigMap_BIT config_map_bit;
/////////////////////////////////////////////
typedef struct __D1608Cmd
{
    char flag[30];
    int preset;
    int type;
    unsigned int id;
	unsigned int length;
    union
	{
		unsigned char data_8;
		unsigned short data_16;
		unsigned int data_32;
		FilterConfigMap data_filter;
		int value[32];
	}data;
#ifdef __cplusplus
    __D1608Cmd()
    {
        preset = 0;
        type = 0;
        id = 0;
        for (int i=0;i<32;i++)
    		data.value[i] = 0;
	    length = 0;
    }
#endif
}D1608Cmd;
typedef struct
{
	char flag[30];
	int preset; // 低位表示preset, 0x80位表示是否全部下载
	int type;
	unsigned int id;
	char data[1024];
}D1608PresetCmd;
typedef struct
{
	int opr;
	int value1;
}CIDebugCmd;

void LoadDefaultConfig(ConfigMap * p_config_map);

//#define Offset(name) ((int)&(((ConfigMap *)NULL)->name))
//#define Size(name) (sizeof(((ConfigMap *)NULL)->name))
unsigned int GerOffsetOfData(void * p_data);
typedef int BOOL;

#pragma pack(pop)
#if 0
D1608Cmd InputVolume(int dsp_num, int volume);
D1608Cmd OutputVolume(int dsp_num, int volume);
D1608Cmd MasterVolume(int volume);
D1608Cmd IOVolume(int out_dsp_num, int in_dsp_num, int volume);
D1608Cmd DO1Volume(int dsp_num, int volume);
D1608Cmd DO2Volume(int dsp_num, int volume);
D1608Cmd InputMute(int mute_bits);
D1608Cmd OutputMute(int is_mute);
D1608Cmd MasterMute(BOOL mute_bits);
D1608Cmd InputNoise(int mute_bits);
D1608Cmd InputInvert(int mute_bits);
D1608Cmd OutputInvert(int mute_bits);
D1608Cmd MixInvert(BOOL is_invert);
D1608Cmd InputComp(int mute_bits);
D1608Cmd InputEQ(int mute_bits);
D1608Cmd OutputEQ(int mute_bits);
D1608Cmd OutputLimit(int mute_bits);
D1608Cmd Phanton(int mute_bits);
D1608Cmd LowShelf(int mute_bits);
D1608Cmd HighShelf(int mute_bits);
D1608Cmd DspEQSwitch(int eq_id, int mute_bits);
D1608Cmd DspInputVolume(int dsp_num, int volume);
D1608Cmd CoefParam(int dsp_num, int gain, int freq);
D1608Cmd Watch(void);
#endif
//D1608Cmd InputDefault(int dsp_num, BOOL is_laston);
//D1608Cmd InputAuto(int mute_bits);
//D1608Cmd Mix_MaxPrio_Invert_Mute(int mix_panel_data);
//D1608Cmd FShift(BOOL is_shift);

#endif

