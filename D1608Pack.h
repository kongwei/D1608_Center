//---------------------------------------------------------------------------

#ifndef D1608PackH
#define D1608PackH
//---------------------------------------------------------------------------
#pragma pack(push, 1)
struct D1608Cmd
{
    //char flag[30];
    unsigned __int32 type;
    unsigned __int32 id;
    unsigned __int32 value;
};
struct D1608Cmd2
{
    unsigned int dsp;	// dsp id
    unsigned int channel_id;	// filter_id
    unsigned int filter_id;	// filter_id
    unsigned int type;	// type
    unsigned int gain;	// (gain+100) * 2
    unsigned int freq;	// freq
    unsigned int q;		// q*100
};

#pragma pack(pop)

D1608Cmd InputVolume(int dsp_num, int volume);
D1608Cmd OutputVolume(int dsp_num, int volume);
D1608Cmd MasterVolume(int volume);
D1608Cmd IOVolume(int out_dsp_num, int in_dsp_num, int volume);
D1608Cmd DO1Volume(int dsp_num, int volume);
D1608Cmd DO2Volume(int dsp_num, int volume);
D1608Cmd InputMute(int mute_bits);
D1608Cmd OutputMute(int is_mute);
D1608Cmd MasterMute(bool mute_bits);
D1608Cmd InputNoise(int mute_bits);
D1608Cmd InputInvert(int mute_bits);
D1608Cmd OutputInvert(int mute_bits);
D1608Cmd InputDefault(int dsp_num);
D1608Cmd LastOn(bool is_laston);
D1608Cmd InputAuto(int mute_bits);
D1608Cmd InputComp(int mute_bits);
D1608Cmd InputEQ(int mute_bits);
D1608Cmd OutputEQ(int mute_bits);
D1608Cmd OutputLimit(int mute_bits);
D1608Cmd FShift(bool is_shift);
D1608Cmd Mix_MaxPrio_Invert_Mute(int mix_panel_data);
D1608Cmd Phanton(int mute_bits);
D1608Cmd LowShelf(int mute_bits);
D1608Cmd HighShelf(int mute_bits);
D1608Cmd DspEQSwitch(int eq_id, int mute_bits);
D1608Cmd DspInputVolume(int dsp_num, int volume);

D1608Cmd CoefParam(int dsp_num, int gain, int freq);

D1608Cmd Watch();

D1608Cmd2 IOVolume2(int out_dsp_num, int in_dsp_num, int volume);
D1608Cmd2 InputVolume2(int dsp_num, int volume);

#endif
