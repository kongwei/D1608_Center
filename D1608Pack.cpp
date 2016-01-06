//---------------------------------------------------------------------------


#pragma hdrstop

#include "D1608Pack.h"
#include<mem.h>

//---------------------------------------------------------------------------

#pragma package(smart_init)

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
    result.id = 0x00010101 + dsp_num;
    result.value = volume;
    return result;
}

// 输出推子
D1608Cmd OutputVolume(int dsp_num, int volume)
{
    D1608Cmd result = GetDefaultData();
    result.id = 0x00010350 + dsp_num;
    result.value = volume;
    return result;
}

// MASTER推子
D1608Cmd MasterVolume(int volume)
{
    D1608Cmd result = GetDefaultData();
    result.id = 0x0001053C;
    result.value = volume;
    return result;
}

// 交叉输出推子
D1608Cmd IOVolume(int out_dsp_num, int in_dsp_num, int volume)
{
    D1608Cmd result = GetDefaultData();
    result.id = 0x000104B3 + out_dsp_num*17-17 + in_dsp_num;
    result.value = volume;
    return result;
}

// DO1
D1608Cmd DO1Volume(int dsp_num, int volume)
{
    D1608Cmd result = GetDefaultData();
    result.id = 0x00010548 + dsp_num;
    result.value = volume;
    return result;
}

// DO2
D1608Cmd DO2Volume(int dsp_num, int volume)
{
    D1608Cmd result = GetDefaultData();
    result.id = 0x00010550 + dsp_num;
    result.value = volume;
    return result;
}

// 输入MUTE
D1608Cmd InputMute(int mute_bits)
{
    D1608Cmd result = GetDefaultData();
    result.id = 0x000200D8;
    result.value = mute_bits;
    return result;
}

// 输出MUTE
D1608Cmd OutputMute(int mute_bits)
{
    D1608Cmd result = GetDefaultData();
    result.id = 0x000104B3;
    result.value = mute_bits;
    return result;
}

D1608Cmd MasterMute(bool is_mute)
{
    D1608Cmd result = GetDefaultData();
    result.id = 0x0001053d;
    result.value = is_mute ? 1 : 0;
    return result;
}

// MOISE
D1608Cmd InputNoise(int mute_bits)
{
    D1608Cmd result = GetDefaultData();
    result.id = 0x000400D4;
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
    return CmdBits(0x000200DA, mute_bits);
}

// Output INVERT
D1608Cmd OutputInvert(int mute_bits)
{
    return CmdBits(0x000104B3, mute_bits);
}

// MIX bit2
D1608Cmd MixInvert(bool is_invert)
{
    D1608Cmd result = GetDefaultData();
    result.id = 0x0001033E;
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

D1608Cmd InputDefault(int dsp_num)
{
    D1608Cmd result = GetDefaultData();
    result.id = 0x0001033D;
    result.value = dsp_num;
    return result;
}

D1608Cmd LastOn(bool is_laston)
{
    D1608Cmd result = GetDefaultData();
    result.id = 0x0001033D;
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
    return CmdBits(0x000200F0, mute_bits);
}

D1608Cmd InputComp(int mute_bits)
{
    return CmdBits(0x000200DC, mute_bits);
}

D1608Cmd InputEQ(int mute_bits)
{
    return CmdBits(0x000200DE, mute_bits);
}

D1608Cmd OutputEQ(int mute_bits)
{
    return CmdBits(0x00010345, mute_bits);
}

D1608Cmd OutputLimit(int mute_bits)
{
    return CmdBits(0x00010346, mute_bits);
}

D1608Cmd Mix_MaxPrio_Invert_Mute(int mix_panel_data)
{
    D1608Cmd result = GetDefaultData();
    result.id = 0x0001033E;
    result.value = mix_panel_data;
    return result;
}

D1608Cmd FShift(bool is_shift)
{
    D1608Cmd result = GetDefaultData();
    result.id = 0x0001033c;  
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
    result.id = 0x000200ee;
    result.value = mute_bits;
    return result;
}

D1608Cmd LowShelf(int mute_bits)
{
    D1608Cmd result = GetDefaultData();
    result.id = 0x000200e0;
    result.value = mute_bits;
    return result;
}

D1608Cmd HighShelf(int mute_bits)
{
    D1608Cmd result = GetDefaultData();
    result.id = 0x000200e2;
    result.value = mute_bits;
    return result;
}

D1608Cmd DspEQSwitch(int eq_id, int mute_bits)
{
    D1608Cmd result = GetDefaultData();
    result.id = 0x000200e4 + eq_id - 1;
    result.value = mute_bits;
    return result;
}

D1608Cmd DspInputVolume(int dsp_num, int volume)
{
    D1608Cmd result = GetDefaultData();
    result.id = 0x000100f1 + dsp_num;
    result.value = volume;
    return result;
}

D1608Cmd Watch()
{
    D1608Cmd result = GetCheckData();
    result.id = 0x001a073a;
    
    return result;
}

