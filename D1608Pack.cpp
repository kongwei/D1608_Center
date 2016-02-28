//---------------------------------------------------------------------------


#pragma hdrstop

#include "D1608Pack.h"
#include <string.h>

//---------------------------------------------------------------------------

#pragma package(smart_init)

#pragma pack(push, 1)
typedef unsigned __int8 Bits8;
typedef unsigned __int16 Bits16;
typedef unsigned __int8 Level;
typedef unsigned __int8 Delay;
typedef unsigned __int32 Filter;
typedef unsigned __int16 HLFilter;
struct ConfigMap
{
	Bits16 input_mute;
	Bits16 input_invert;
	Bits16 input_comp;
	Bits16 input_eq;
	Bits16 low_shelf;
	Bits16 high_shelf;
	Bits16 input_eq_inner[5];
	Bits16 input_phanton;
	Bits16 input_auto;
	Level input_innerlevel[16];
	Level input_level[16];
	Level mix_level;
	__int8 pad0;
	Filter input_lowshelf_data[16];
	Filter input_highshelf_data[16];
	Filter input_eq_data[16][5];
	char pad1[104];
	Bits8 FShift;
	__int8 input_default_laston;	// 33D
	Bits8 mix_conf;
	__int8 mix_sen;
	__int8 pad2;
	__int8 mix_holdtime;	// div 50
	__int8 pad21;
	__int8 mix_nom_limit;
	__int8 mix_nom_att;
	Bits8 output_eq;
	Bits8 output_limit;
	Bits8 output_highpass;
	Bits8 output_lowpass;
	Bits8 output_eq1;
	Bits8 output_eq2;
	Bits8 output_eq3;
	Bits8 output_eq4;
	Bits8 output_eq5;
	Bits8 output_eq6;
	Bits8 output_eq7;
	Bits8 output_eq8;
	Level output_level[8];
	Delay output_delay[8];
	char pad3[49];
	HLFilter output_highpass_data[8];
	HLFilter output_lowpass_data[8];
	Filter output_eq_data[8][8];	// DSP eq
	Bits8 output_invert;
	Bits8 output_mute;
	Level matrix_output_inputmix[8][17];
	Level master_level;
	Bits8 master_mute;
	char pad4[11];
	Level DO1[8];
	Level DO2[8];
	char pad5[10];
	// 0x48C
};
#define Offset(name) ((int)&(((ConfigMap *)NULL)->name)+0xD8)

#pragma pack(pop)

static char head[] = "\x53\x65\x74\x50\x61\x72\x61\x00\x00\x00\x4D\x41\x54\x31\x36\x31\x30\x43\x6F\x6E\x66\x69\x67\x44\x61\x74\x61\x00\x00\x00";

static D1608Cmd GetDefaultData()
{
    D1608Cmd result;
    memmove(result.flag, head, 30);
    result.type = 0x060a;
    result.id = 0;
    result.value = 0;
    return result;
};
static D1608Cmd GetCheckData()
{
    D1608Cmd result;
    memmove(result.flag, head, 30);
    result.type = 0x03ca;
    result.id = 0;
    result.value = 0;
    return result;
};

// 输入推子 -72 ~ +12 每刻度0.5
D1608Cmd InputVolume(int dsp_num, int volume)
{
    D1608Cmd result = GetDefaultData();
    result.id = Offset(input_level[dsp_num-1]);
    result.value = volume;
    return result;
}

// 输出推子
D1608Cmd OutputVolume(int dsp_num, int volume)
{
    D1608Cmd result = GetDefaultData();
    result.id = Offset(output_level[dsp_num-1]);//0x0350 + dsp_num;
    result.value = volume;
    return result;
}

// MASTER推子
D1608Cmd MasterVolume(int volume)
{
    D1608Cmd result = GetDefaultData();
    result.id = Offset(master_level);
    result.value = volume;
    return result;
}

// 交叉输出推子
D1608Cmd IOVolume(int out_dsp_num, int in_dsp_num, int volume)
{
    D1608Cmd result = GetDefaultData();
    result.id = Offset(matrix_output_inputmix[out_dsp_num-1][in_dsp_num-1]);
    result.value = volume;
    return result;
}

// DO1
D1608Cmd DO1Volume(int dsp_num, int volume)
{
    D1608Cmd result = GetDefaultData();
    result.id = Offset(DO1[dsp_num-1]);
    result.value = volume;
    return result;
}

// DO2
D1608Cmd DO2Volume(int dsp_num, int volume)
{
    D1608Cmd result = GetDefaultData();
    result.id = Offset(DO2[dsp_num-1]);//0x0550 + dsp_num;
    result.value = volume;
    return result;
}

// 输入MUTE
D1608Cmd InputMute(int mute_bits)
{
    D1608Cmd result = GetDefaultData();
    result.id = Offset(input_mute);//0x00D8;
    result.value = mute_bits;
    return result;
}

// 输出MUTE
D1608Cmd OutputMute(int mute_bits)
{
    D1608Cmd result = GetDefaultData();
    result.id = Offset(output_mute);//0x04B3;
    result.value = mute_bits;
    return result;
}

D1608Cmd MasterMute(bool is_mute)
{
    D1608Cmd result = GetDefaultData();
    result.id = Offset(master_mute);
    result.value = is_mute ? 1 : 0;
    return result;
}

// MOISE
D1608Cmd InputNoise(int mute_bits)
{
	// TODO:
    D1608Cmd result = GetDefaultData();
    result.id = 0x00D4;
    result.value = mute_bits;
    return result;
}

// Input bits
static D1608Cmd CmdBits(int cmd_id, int mute_bits)
{
    D1608Cmd result = GetDefaultData();
    result.id = cmd_id;
    result.value = mute_bits;
    return result;
}

// Input INVERT
D1608Cmd InputInvert(int mute_bits)
{
    return CmdBits(Offset(input_invert), mute_bits);
}

// Output INVERT
D1608Cmd OutputInvert(int mute_bits)
{
    return CmdBits(Offset(output_invert), mute_bits);
}

// TODO: InputDefault,LastOn 合并
D1608Cmd InputDefault(int dsp_num)
{
    D1608Cmd result = GetDefaultData();
    result.id = Offset(input_default_laston);//0x033D;
    result.value = dsp_num;
    return result;
}
D1608Cmd LastOn(bool is_laston)
{
    D1608Cmd result = GetDefaultData();
    result.id = Offset(input_default_laston);//0x033D;
    if (is_laston)
    {
        result.value = 0x00000080;
    }
    else
    {
        result.value = 0;
    }
    return result;
}

D1608Cmd InputAuto(int mute_bits)
{
    return CmdBits(Offset(input_auto), mute_bits);
}

D1608Cmd InputComp(int mute_bits)
{
    return CmdBits(Offset(input_comp), mute_bits);
}

D1608Cmd InputEQ(int mute_bits)
{
    return CmdBits(Offset(input_eq), mute_bits);
}

D1608Cmd OutputEQ(int mute_bits)
{
    return CmdBits(Offset(output_eq), mute_bits);
}

D1608Cmd OutputLimit(int mute_bits)
{
    return CmdBits(Offset(output_limit), mute_bits);
}

// TODO: 合并
D1608Cmd Mix_MaxPrio_Invert_Mute(int mix_panel_data)
{
    D1608Cmd result = GetDefaultData();
    result.id = Offset(mix_conf);
    result.value = mix_panel_data;
    return result;
}
// MIX bit2
D1608Cmd MixInvert(bool is_invert)
{
    D1608Cmd result = GetDefaultData();
    result.id = Offset(mix_conf);//0x033E;
    if (is_invert)
    {
        result.value = 0x00000005;
    }
    else
    {
        result.value = 0x00000001;
    }
    return result;
}

D1608Cmd FShift(bool is_shift)
{
    D1608Cmd result = GetDefaultData();
    result.id = Offset(FShift);//0x033c;  
    if (is_shift)
    {
        result.value = 0x80;
    }
    else
    {
        result.value = 0;
    }
    return result;
}

// PHANTON
D1608Cmd Phanton(int mute_bits)
{
    D1608Cmd result = GetDefaultData();
    result.id = Offset(input_phanton);
    result.value = mute_bits;
    return result;
}

D1608Cmd LowShelf(int mute_bits)
{
    D1608Cmd result = GetDefaultData();
    result.id = Offset(low_shelf);//0x00e0;
    result.value = mute_bits;
    return result;
}

D1608Cmd HighShelf(int mute_bits)
{
    D1608Cmd result = GetDefaultData();
    result.id = Offset(high_shelf);//0x00e2;
    result.value = mute_bits;
    return result;
}

D1608Cmd DspEQSwitch(int eq_id, int mute_bits)
{
    D1608Cmd result = GetDefaultData();
    result.id = Offset(input_eq_inner[eq_id - 1]);//0x00e4 + eq_id - 1;
    result.value = mute_bits;
    return result;
}

D1608Cmd DspInputVolume(int dsp_num, int volume)
{
    D1608Cmd result = GetDefaultData();
    result.id = Offset(input_innerlevel[dsp_num-1]);//0x00f1 + dsp_num;
    result.value = volume;
    return result;
}

D1608Cmd CoefParam(int dsp_num, int gain, int freq)
{
    D1608Cmd result = GetCheckData();
    result.type = dsp_num;
    result.id = gain;
    result.value = freq;
    return result;
}

D1608Cmd Watch()
{
    D1608Cmd result = GetCheckData();
    result.id = 0x001a073a;
    
    return result;
}

