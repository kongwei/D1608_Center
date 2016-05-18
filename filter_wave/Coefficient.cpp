//---------------------------------------------------------------------------


#pragma hdrstop

#include "Coefficient.h"

//---------------------------------------------------------------------------

#pragma package(smart_init)

const double QTable[73] ={
		0.31, 0.32, 0.34, 0.36, 0.39, 0.41, 0.43, 0.46, 0.49, 0.52, 0.55, 0.58, 0.61, 0.65, 0.69, 0.73,//[0,15]
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
            middles[i] = middles[i-1];
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

    _type = type;
    _freq = freq;
    _gain = gain;
    _q = q;

    if (type == "Parametric")
    {
        Peaking(freq, gain, q);
        _type_id = 1;
    }
    else if (type == "Band Pass")
    {
        BandPass(freq, gain, q);
        _type_id = 2;
    }
    else if (type == "High Shelf")
    {
        HighShelving(freq, gain, q);
        _type_id = 3;
    }
    else if (type == "Low Shelf")
    {
        LowShelving(freq, gain, q);
        _type_id = 4;
    }
    else if (type == "Notch")
    {
        Notch(freq, gain, q);
        _type_id = 5;
    }
    else if (type == "12dB Bessel High")
    {
        HighBessel_12dB(freq);
        _type_id = 1202;
    }
    else if (type == "12dB Bessel Low")
    {
        LowBessel_12dB(freq);
        _type_id = 1212;
    }
    else if (type == "18dB Bessel High")
    {
        HighBessel_18dB(freq);
        _type_id = 1802;
    }
    else if (type == "18dB Bessel Low")
    {
        LowBessel_18dB(freq);
        _type_id = 1812;
    }
    else if (type == "24dB Bessel High")
    {
        HighBessel_24dB(freq);
        _type_id = 2402;
    }
    else if (type == "24dB Bessel Low")
    {
        LowBessel_24dB(freq);
        _type_id = 2412;
    }

    else if (type == "12dB Linkwitz-Riley High")
    {
        HighLinkwitz_12dB(freq);
        _type_id = 1203;
    }
    else if (type == "12dB Linkwitz-Riley Low")
    {
        LowLinkwitz_12dB(freq);
        _type_id = 1213;
    }
    else if (type == "24dB Linkwitz-Riley High")
    {
        HighLinkwitz_24dB(freq);
        _type_id = 2403;
    }
    else if (type == "24dB Linkwitz-Riley Low")
    {
        LowLinkwitz_24dB(freq);
        _type_id = 2413;
    }
    else if (type == "48dB Linkwitz-Riley High")
    {
        HighLinkwitz_48dB(freq);
        _type_id = 4803;
    }
    else if (type == "48dB Linkwitz-Riley Low")
    {
        LowLinkwitz_48dB(freq);
        _type_id = 4813;
    }

    else if (type == "6dB Bansen High")
    {
        HighBansen(freq);
        _type_id = 604;
    }
    else if (type == "6dB Bansen Low")
    {
        LowBansen(freq);
        _type_id = 614;
    }

    else if (type == "12dB Butterworth High")
    {
        HighPassButterworth_12dB(freq);
        _type_id = 1201;
    }
    else if (type == "18dB Butterworth High")
    {
        HighPassButterworth_18dB(freq);
        _type_id = 1801;
    }
    else if (type == "24dB Butterworth High")
    {
        HighPassButterworth_24dB(freq);
        _type_id = 2401;
    }
    /*else if (type == "36dB Butterworth High")
    {
        HighPassButterworth_36dB(freq);
        _type_id = 3601;
    }*/
    else if (type == "48dB Butterworth High")
    {
        HighPassButterworth_48dB(freq);
        _type_id = 4801;
    }
    else if (type == "12dB Butterworth Low")
    {
        LowPassButterworth_12dB(freq);
        _type_id = 1211;
    }
    else if (type == "18dB Butterworth Low")
    {
        LowPassButterworth_18dB(freq);
        _type_id = 1811;
    }
    else if (type == "24dB Butterworth Low")
    {
        LowPassButterworth_24dB(freq);
        _type_id = 2411;
    }
    /*else if (type == "36dB Butterworth Low")
    {
        LowPassButterworth_36dB(freq);
        _type_id = 3611;
    }*/
    else if (type == "48dB Butterworth Low")
    {
        LowPassButterworth_48dB(freq);
        _type_id = 4811;
    }
    else if (type == "Pink")
    {
        Pink(freq, gain, q);
        _type_id = 7;
    }
}
