//---------------------------------------------------------------------------

#ifndef D1608PackH
#define D1608PackH
//---------------------------------------------------------------------------
#define INPUT_DSP_NUM 16
#define OUTPUT_DSP_NUM 16

#pragma pack(push, 1)
#if 0
typedef struct
{
    char flag[12];
    unsigned __int32 dsp_type;  // 0: system, 1: input, 2: output
    unsigned __int32 dsp_id;
    unsigned __int32 func_id;
    unsigned __int32 value;
}MatrixCmd;
#endif
typedef struct
{
	unsigned int TYPE;
    int GAIN;
    unsigned int FREQ;
    unsigned int Q;
	int bypass;
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
    int level_a;
    int level_b;
    int gain;
    unsigned int delay;
    FilterConfigMap filter_pad1;
    FilterConfigMap filter_pad2;
    FilterConfigMap filter[11];
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
    int level_a;
    int level_b;
    int gain;
    unsigned int pad5;
    FilterConfigMap high_pass;
    FilterConfigMap low_pass;
    FilterConfigMap filter[11];
}OutputConfigMap;

typedef struct
{
    InputConfigMap input_dsp[INPUT_DSP_NUM];
    OutputConfigMap output_dsp[OUTPUT_DSP_NUM];
}ConfigMap;

extern ConfigMap config_map;

typedef struct
{
    char flag[30];
    int preset;
    int type;
    unsigned int id;
    int value;
    int value2;
    int value3;
    int value4;
    int value5;
}D1608Cmd;

void LoadDefaultConfig(ConfigMap * p_config_map);

//#define Offset(name) ((int)&(((ConfigMap *)NULL)->name))
//#define Size(name) (sizeof(((ConfigMap *)NULL)->name))
int GerOffsetOfData(void * p_data);
typedef int BOOL;

#pragma pack(pop)
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
D1608Cmd InputDefault(int dsp_num, BOOL is_laston);
D1608Cmd InputAuto(int mute_bits);
D1608Cmd InputComp(int mute_bits);
D1608Cmd InputEQ(int mute_bits);
D1608Cmd OutputEQ(int mute_bits);
D1608Cmd OutputLimit(int mute_bits);
D1608Cmd FShift(BOOL is_shift);
D1608Cmd Mix_MaxPrio_Invert_Mute(int mix_panel_data);
D1608Cmd Phanton(int mute_bits);
D1608Cmd LowShelf(int mute_bits);
D1608Cmd HighShelf(int mute_bits);
D1608Cmd DspEQSwitch(int eq_id, int mute_bits);
D1608Cmd DspInputVolume(int dsp_num, int volume);

D1608Cmd CoefParam(int dsp_num, int gain, int freq);

D1608Cmd Watch(void);

#endif
