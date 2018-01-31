//---------------------------------------------------------------------------


#pragma hdrstop

#include "Coefficient.h"
#include <algorithm>
#include "untMain.h"
//---------------------------------------------------------------------------

#pragma package(smart_init)
static int Float2FixPoint(float coeff);

const double QTable[73] ={
		0.27, 0.32, 0.34, 0.36, 0.39, 0.41, 0.43, 0.46, 0.49, 0.52, 0.55, 0.58, 0.61, 0.65, 0.69, 0.73,//[0,15]
		0.77, 0.82, 0.87, 0.92, 0.97, 1.03, 1.09, 1.15, 1.22, 1.29, 1.37, 1.45, 1.54, 1.63, 1.73, 1.83,//[16,31]
		1.94, 2.05, 2.17, 2.30, 2.44, 2.58, 2.73, 2.90, 3.07, 3.25, 3.44, 3.65, 3.86, 4.09, 4.33, 4.59,//[32,47]
		4.86, 5.15, 5.46, 5.78, 6.12, 6.48, 6.87, 7.27, 7.71, 8.16, 8.65, 9.16, 9.70, 10.3, 10.9, 11.5,//[48,63]
		12.2, 12.9, 13.7, 14.5, 15.4, 16.3, 17.3, 18.3, 19.4//[64,72]
		};
double NextLargeQ(double q)
{
    for (int i=0;i<=72;i++)
    {
        if (q < QTable[i])
        {
            return QTable[i];
        }
    }
    return QTable[72];
}
double NextSmallQ(double q)
{
    for (int i=72;i>=0;i--)
    {
        if (q > QTable[i])
        {
            return QTable[i];
        }
    }
    return QTable[0];
}
bool IsInQTable(double q)
{
    for (int i=0;i<=72;i++)
    {
        if ((q-QTable[i])<0.001 && (q-QTable[i])>-0.001)
        {
            return true;
        }
    }
    return false;
}

const double FrequencyTable[121] ={
		20.0, 21.2, 22.4, 23.7, 25.0, 26.5, 28.0, 29.7, 31.5, 33.5, 35.5, 37.5, 40.0, 42.5, 45.0, 47.4, //[0,15]
		50.0, 53.0, 56.0, 59.5, 63.0, 67.0, 71.0, 75.0, 80.0, 85.0, 90.0, 95.0, 100, 106, 112, 118, 125,//[16,32]
		132, 140, 150, 160, 170, 180, 190, //[33,39]

		200,  212,  224,  237,  250,  265,  280,  297,  315,  335,  355,  375,  400,  425,  450,  474,//[40,55]
		500,  530,  560,  595,  630,  670,  710,  750,  800,  850,  900,  950,  1000, 1060, 1120, 1180, 1260,//[56,72]
		1320, 1400, 1500, 1600, 1700, 1800, 1900,//[73,79]

		2000, 2120, 2240, 2370, 2500, 2650, 2800, 2970, 3100, 3350, 3550, 3750, 4000, 4250, 4500, 4740,//[80,95]
		5000, 5300, 5600, 5950, 6300, 6700, 7100, 7500, 8000, 8500, 9000, 9500, 10000,10600,11200,11800,12500,//[96,112]
		13200,14000,15000,16000,17000,18000,19000,//[113,119]

		20000//[120,120]
		};
double NextLargeFreq(double freq)
{
    for (int i=0;i<=120;i++)
    {
        if (freq < FrequencyTable[i]-0.01)
        {
            return FrequencyTable[i];
        }
    }
    return FrequencyTable[120];
}
double NextSmallFreq(double freq)
{
    for (int i=120;i>=0;i--)
    {
        if (freq > FrequencyTable[i]+0.01)
        {
            return FrequencyTable[i];
        }
    }
    return FrequencyTable[0];
}
bool IsInFreqTable(double freq)
{
    for (int i=0;i<=72;i++)
    {
        if ((freq-FrequencyTable[i])<0.001 && (freq-FrequencyTable[i])>-0.001)
        {
            return true;
        }
    }
    return false;
}
double ClosestFreq(double freq)
{
    double next_small_freq = NextSmallFreq(freq);
    double next_large_freq = NextLargeFreq(freq);
    if ((freq-next_small_freq) > (next_large_freq-freq))
    {
        return next_large_freq;
    }
    else
    {
        return next_small_freq;
    }
}

void Coefficient::PrepreMiddle()
{
    for (int i=0;i<1001;i++)
    {
        middles[i] = FilterCoe(freq_point[i] / SAMPLE_FREQ);
        if (middles[i] <= 0.00001)
        {
            middles[i] = -30;// 数据太小，用-30表示图片上的负无穷大
        }
        else
        {
            middles[i] = 20 * Log10(middles[i]);
        }
    }
}
void Coefficient::ChangFilterParameter(String type, double freq, double gain, double q)
{
    if ((type == _type)
      &&(freq == _freq)
      &&(gain == _gain)
      &&(q == _q))
    {
        return;
    }

    gain = std::max(gain, -30.0);
    gain = std::min(gain, 15.0);

    if (q == 0)
        q = 4;

    if (type == "PARAMETRIC")
    {
        Peaking(freq, gain, q);
        _type_id = 1;
    }
    else if (type == "BAND_PASS")
    {
        BandPass(freq, gain, q);
        _type_id = 2;
    }
    else if (type == "HIGH_SHELF")
    {
        gain = std::min(gain, 12.0);

        HighShelving(freq, gain, q);
        _type_id = 3;
    }
    else if (type == "LOW_SHELF")
    {
        gain = std::min(gain, 12.0);

        LowShelving(freq, gain, q);
        _type_id = 4;
    }
    else if (type == "NOTCH")
    {
        Notch(freq, gain, q);
        _type_id = 5;
    }
    else if (type == "12dB_BESSEL_HIGH")
    {
        HighBessel_12dB(freq);
        _type_id = 1202;
    }
    else if (type == "12dB_BESSEL_LOW")
    {
        LowBessel_12dB(freq);
        _type_id = 1212;
    }
    else if (type == "18dB_BESSEL_HIGH")
    {
        HighBessel_18dB(freq);
        _type_id = 1802;
    }
    else if (type == "18dB_BESSEL_LOW")
    {
        LowBessel_18dB(freq);
        _type_id = 1812;
    }
    else if (type == "24dB_BESSEL_HIGH")
    {
        HighBessel_24dB(freq);
        _type_id = 2402;
    }
    else if (type == "24dB_BESSEL_LOW")
    {
        LowBessel_24dB(freq);
        _type_id = 2412;
    }
    else if (type == "48dB_BESSEL_HIGH")
    {
        HighPassButterworth_48dB(freq);
        _type_id = 4802;
    }
    else if (type == "48dB_Bessel_LOW")
    {
        LowPassButterworth_48dB(freq);
        _type_id = 4812;
    }

    else if (type == "12dB_LINKWITZ-RILEY_HIGH")
    {
        HighLinkwitz_12dB(freq);
        _type_id = 1203;
    }
    else if (type == "12dB_LINKWITZ-RILEY_LOW")
    {
        LowLinkwitz_12dB(freq);
        _type_id = 1213;
    }
    else if (type == "24dB_LINKWITZ-RILEY_HIGH")
    {
        HighLinkwitz_24dB(freq);
        _type_id = 2403;
    }
    else if (type == "24dB_LINKWITZ-RILEY_LOW")
    {
        LowLinkwitz_24dB(freq);
        _type_id = 2413;
    }
    else if (type == "48dB_LINKWITZ-RILEY_HIGH")
    {
        HighLinkwitz_48dB(freq);
        _type_id = 4803;
    }
    else if (type == "48dB_LINKWITZ-RILEY_LOW")
    {
        LowLinkwitz_48dB(freq);
        _type_id = 4813;
    }

    else if (type == "6dB_BANSEN_HIGH")
    {
        HighBansen(freq);
        _type_id = 604;
    }
    else if (type == "6dB_BANSEN_LOW")
    {
        LowBansen(freq);
        _type_id = 614;
    }

    else if (type == "12dB_BUTTERWORTH_HIGH")
    {
        HighPassButterworth_12dB(freq);
        _type_id = 1201;
    }
    else if (type == "18dB_BUTTERWORTH_HIGH")
    {
        HighPassButterworth_18dB(freq);
        _type_id = 1801;
    }
    else if (type == "24dB_BUTTERWORTH_HIGH")
    {
        HighPassButterworth_24dB(freq);
        _type_id = 2401;
    }
    /*else if (type == "36dB_BUTTERWORTH_HIGH")
    {
        HighPassButterworth_36dB(freq);
        _type_id = 3601;
    }*/
    else if (type == "48dB_BUTTERWORTH_HIGH")
    {
        HighPassButterworth_48dB(freq);
        _type_id = 4801;
    }
    else if (type == "12dB_BUTTERWORTH_LOW")
    {
        LowPassButterworth_12dB(freq);
        _type_id = 1211;
    }
    else if (type == "18dB_BUTTERWORTH_LOW")
    {
        LowPassButterworth_18dB(freq);
        _type_id = 1811;
    }
    else if (type == "24dB_BUTTERWORTH_LOW")
    {
        LowPassButterworth_24dB(freq);
        _type_id = 2411;
    }
    /*else if (type == "36dB_BUTTERWORTH_LOW")
    {
        LowPassButterworth_36dB(freq);
        _type_id = 3611;
    }*/
    else if (type == "48dB_BUTTERWORTH_LOW")
    {
        LowPassButterworth_48dB(freq);
        _type_id = 4811;
    }
    else if (type == "PINK")
    {
        Pink(freq, gain, q);
        _type_id = 7;
    }
    else
    {
    }

    _type = type;
    _freq = freq;
    _gain = gain;
    _q = q;

    float base = 0.125;

    Form1->mmCoeff->Clear();
    a1 = -a1;
    a2 = -a2;
    Form1->mmCoeff->Lines->Add("L_0:  " + String::FormatFloat("0.#########################", b0/a0*base));
    Form1->mmCoeff->Lines->Add("L_1:  " + String::FormatFloat("0.#########################", b1/a0*base));
    Form1->mmCoeff->Lines->Add("L_2:  " + String::FormatFloat("0.#########################", b2/a0*base));
    Form1->mmCoeff->Lines->Add("R_1:  " + String::FormatFloat("0.#########################", a1/a0*base));
    Form1->mmCoeff->Lines->Add("R_2:  " + String::FormatFloat("0.#########################", a2/a0*base));


    short coeffs[10] = {0};
    coeffs[0] = Float2FixPoint(b0/a0*base) & 0xFFFF;
    coeffs[2] = Float2FixPoint(b1/a0*base) & 0xFFFF;
    coeffs[4] = Float2FixPoint(b2/a0*base) & 0xFFFF;
    coeffs[6] = Float2FixPoint(a1/a0*base) & 0xFFFF;
    coeffs[8] = Float2FixPoint(a2/a0*base) & 0xFFFF;
    coeffs[1] = Float2FixPoint(b0/a0*base) >> 16;
    coeffs[3] = Float2FixPoint(b1/a0*base) >> 16;
    coeffs[5] = Float2FixPoint(b2/a0*base) >> 16;
    coeffs[7] = Float2FixPoint(a1/a0*base) >> 16;
    coeffs[9] = Float2FixPoint(a2/a0*base) >> 16;

    double dsp2double[5];
    for (int i=0;i<10;i+=2)
    {
        String left_right;
        if (i < 5)
        {
            left_right = "L_"+IntToStr(i/2);
        }
        else
        {
            left_right = "R_"+IntToStr(i/2-2);
        }

        coeffs[i] = htons(coeffs[i]);
        coeffs[i+1] = htons(coeffs[i+1]) * 16 & 0xFFF0;
        Form1->mmCoeff->Lines->Add(left_right + ":  " + IntToHex(coeffs[i]&0xFFFF, 4)+" "+IntToHex(coeffs[i+1]&0xFFFF, 4));
        dsp2double[i/2] = coeffs[i]/32768.0 + ((unsigned short)coeffs[i+1]) / 32768.0  / 65536.0;
    }
    Form1->mmCoeff->Lines->Add("----仿真软件参数----");
    Form1->mmCoeff->Lines->Add(String::FormatFloat("0.#########################", dsp2double[0]));
    Form1->mmCoeff->Lines->Add(String::FormatFloat("0.#########################", dsp2double[1]));
    Form1->mmCoeff->Lines->Add(String::FormatFloat("0.#########################", dsp2double[2]));
    Form1->mmCoeff->Lines->Add(String::FormatFloat("0.#########################", 0.125));
    Form1->mmCoeff->Lines->Add(String::FormatFloat("0.#########################", -dsp2double[3]));
    Form1->mmCoeff->Lines->Add(String::FormatFloat("0.#########################", -dsp2double[4]));
}

static int Float2FixPoint(float coeff)
{
	int result;
    int temp;
    float f_temp;

    temp = (int)(coeff * 0x8000);
    if ( (coeff<0) && (coeff>-1.0) )
	{ 
		temp = temp -1;
	}
    f_temp = (float)temp / (float)0x8000;
    
	result = ( temp>>8 ) & 0xff;
    temp = temp & 0xff;
	temp = temp << 8;
	result = result + temp;
    
    f_temp = coeff - f_temp;
    temp = (int)(f_temp * 0x8000000);
    result = result + (((temp>>8) & 0xff)<<16);
    result = result + ((temp & 0xff)<<24);

	return result;
}

String Coefficient::GetTypeName(int type_id)
{
    switch(type_id)
    {
    case 1:
        return "PARAMETRIC";
    case 2:
        return "BAND_PASS";
    case 3:
        return "HIGH_SHELF";
    case 4:
        return "LOW_SHELF";
    case 5:
        return "NOTCH";
    case 1202:
        return "12dB_BESSEL_HIGH";
    case 1212:
        return "12dB_BESSEL_LOW";
    case 1802:
        return "18dB_BESSEL_HIGH";
    case 1812:
        return "18dB_BESSEL_LOW";
    case 2402:
        return "24dB_BESSEL_HIGH";
    case 2412:
        return "24dB_BESSEL_LOW";
    case 4802:
        return "48dB_BESSEL_HIGH";
    case 4812:
        return "48dB_BESSEL_LOW";
    case 1203:
        return "12dB_LINKWITZ-RILEY_HIGH";
    case 1213:
        return "12dB_LINKWITZ-RILEY_LOW";
    case 2403:
        return "24dB_LINKWITZ-RILEY_HIGH";
    case 2413:
        return "24dB_LINKWITZ-RILEY_LOW";
    case 4803:
        return "48dB_LINKWITZ-RILEY_HIGH";
    case 4813:
        return "48dB_LINKWITZ-RILEY_LOW";
    case 604:
        return "6dB_BANSEN_HIGH";
    case 614:
        return "6dB_BANSEN_LOW";
    case 1201:
        return "12dB_BUTTERWORTH_HIGH";
    case 1801:
        return "18dB_BUTTERWORTH_HIGH";
    case 2401:
        return "24dB_BUTTERWORTH_HIGH";
    case 4801:
        return "48dB_BUTTERWORTH_HIGH";
    case 1211:
        return "12dB_BUTTERWORTH_LOW";
    case 1811:
        return "18dB_BUTTERWORTH_LOW";
    case 2411:
        return "24dB_BUTTERWORTH_LOW";
    case 4811:
        return "48dB_BUTTERWORTH_LOW";
    case 7:
        return "PINK";
    }

    return "PARAMETRIC";
}

void InitConfigMap(ConfigMap& tmp_config_map)
{
	int preset_freq[11] = {20, 50, 100, 200, 500, 1000, 2000, 5000, 7500, 10000, 20000};

	memset(&tmp_config_map, 0,sizeof(tmp_config_map));

	for (int i=0;i<INPUT_DSP_NUM;i++)
	{
        tmp_config_map.input_dsp[i].eq_switch = 1;
        tmp_config_map.input_dsp[i].gain = 7;

		for (int j=HP_FILTER;j<=LP_FILTER;j++)
		{
			tmp_config_map.input_dsp[i].filter[j-1].GAIN = 0;
			tmp_config_map.input_dsp[i].filter[j-1].bypass = 0;
			tmp_config_map.input_dsp[i].filter[j-1].FREQ = preset_freq[j-1] * 10;
			tmp_config_map.input_dsp[i].filter[j-1].Q = 409;
		}

		tmp_config_map.input_dsp[i].filter[HP_FILTER-1].TYPE = 1201;	// HP
			tmp_config_map.input_dsp[i].filter[HP_FILTER-1].bypass = 1;
		tmp_config_map.input_dsp[i].filter[HP_FILTER].TYPE = 4;	// Low Shelf
		for (int j=FIRST_FILTER+2;j<=LAST_FILTER-2;j++)
        {
            tmp_config_map.input_dsp[i].filter[j-1].TYPE = 1;	// PEQ
        }
		tmp_config_map.input_dsp[i].filter[LP_FILTER-2].TYPE = 3;	// High Shelf
		tmp_config_map.input_dsp[i].filter[LP_FILTER-1].TYPE = 1211;	// LP
			tmp_config_map.input_dsp[i].filter[LP_FILTER-1].bypass = 1;
	}

	for (int i=0;i<OUTPUT_DSP_NUM;i++)
	{
        tmp_config_map.output_dsp[i].eq_switch = 1;
        tmp_config_map.output_dsp[i].gain = 7;

		for (int j=HP_FILTER;j<=LP_FILTER;j++)
		{
			tmp_config_map.output_dsp[i].filter[j-1].GAIN = 0;
			tmp_config_map.output_dsp[i].filter[j-1].bypass = 0;
			tmp_config_map.output_dsp[i].filter[j-1].FREQ = preset_freq[j-1] * 10;
			tmp_config_map.output_dsp[i].filter[j-1].Q = 409;
		}

		tmp_config_map.output_dsp[i].filter[HP_FILTER-1].TYPE = 1201;	// HP
			tmp_config_map.output_dsp[i].filter[HP_FILTER-1].bypass = 1;
		tmp_config_map.output_dsp[i].filter[HP_FILTER].TYPE = 4;	// Low Shelf
		for (int j=FIRST_FILTER+2;j<=LAST_FILTER-2;j++)
        {
            tmp_config_map.output_dsp[i].filter[j-1].TYPE = 1;	// PEQ
        }
		tmp_config_map.output_dsp[i].filter[LP_FILTER-2].TYPE = 3;	// High Shelf
		tmp_config_map.output_dsp[i].filter[LP_FILTER-1].TYPE = 1211;	// LP
			tmp_config_map.output_dsp[i].filter[LP_FILTER-1].bypass = 1;

        // 压缩参数
        tmp_config_map.output_dsp[i].ratio = 100;
        tmp_config_map.output_dsp[i].threshold = -100;
        tmp_config_map.output_dsp[i].attack_time = 640;
        tmp_config_map.output_dsp[i].release_time = 10000;
        tmp_config_map.output_dsp[i].comp_gain = 0;
        tmp_config_map.output_dsp[i].auto_time = 1;
	}

    // mix_mute
    memset(&tmp_config_map.master_mix.mix_mute, 1, sizeof(tmp_config_map.master_mix.mix_mute));
    for (int i=0;i<INPUT_DSP_NUM/*或者OUTPUT_DSP_NUM*/;i++)
    {
        tmp_config_map.master_mix.mix_mute[i][i] = 0;
    }
}

