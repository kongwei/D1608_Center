//---------------------------------------------------------------------------

#ifndef CoefficientH
#define CoefficientH

#include <math.h>
#include <algorithm>
#include <math.hpp>
extern "C"{
#include "../enc28j60_iap_app/inc/D1608Pack.h"
}
//---------------------------------------------------------------------------

#define SAMPLE_FREQ 48000
#define MAX_FREQ 24000.0
#define FREQ_INTERVAL_RATIO 1.0071152709380548682037835125915

double NextLargeQ(double q);
double NextSmallQ(double q);
bool IsInQTable(double q);

double NextLargeFreq(double freq);
double NextSmallFreq(double freq);
bool IsInFreqTable(double freq);
double ClosestFreq(double freq);

class Coefficient
{
public:
    Coefficient()
    {
        InitFreqPoint();
        EqualizerOff();

        ChangFilterParameter("Parametric", 1000, 0, 4);
    }
    WideString name;
    double GetFreq() const
    {
        return _freq;
    }
    double GetGain() const
    {
        return _gain;
    }
    double GetQ() const
    {
        return _q;
    }
    String GetType() const
    {
        return _type;
    }
    int GetTypeId() const
    {
        return _type_id;
    }
    void SetFreqGain(double freq, double gain)
    {
        ChangFilterParameter(_type, freq, gain, _q);
    }
    void SetFreq(double freq)
    {
        ChangFilterParameter(_type, freq, _gain, _q);
    }
    void SetGain(double gain)
    {
        ChangFilterParameter(_type, _freq, gain, _q);
    }
    void SetQ(double q)
    {
        ChangFilterParameter(_type, _freq, _gain, q);
    }
    void SetTypeId(int type_id)
    {
        String type_name = GetTypeName(type_id);
        SetType(type_name);
    }
    bool IsGainEnabled()
    {
        if ((_type == "Band Pass")
          ||(_type == "Notch")
          ||(_type == "6dB Bansen High")
          ||(_type == "6dB Bansen Low")
          ||(_type == "12dB Bessel High")
          ||(_type == "12dB Bessel Low")
          ||(_type == "18dB Bessel High")
          ||(_type == "18dB Bessel Low")
          ||(_type == "24dB Bessel High")
          ||(_type == "24dB Bessel Low")
          ||(_type == "48dB Bessel High")
          ||(_type == "48dB Bessel Low")
          ||(_type == "12dB Linkwitz-Riley High")
          ||(_type == "12dB Linkwitz-Riley Low")
          ||(_type == "24dB Linkwitz-Riley High")
          ||(_type == "24dB Linkwitz-Riley Low")
          ||(_type == "48dB Linkwitz-Riley High")
          ||(_type == "48dB Linkwitz-Riley Low")
          ||(_type == "12dB Butterworth High")
          ||(_type == "18dB Butterworth High")
          ||(_type == "24dB Butterworth High")
    //      ||(_type == "36dB Butterworth High")
          ||(_type == "48dB Butterworth High")
          ||(_type == "12dB Butterworth Low")
          ||(_type == "18dB Butterworth Low")
          ||(_type == "24dB Butterworth Low")
    //      ||(_type == "36dB Butterworth Low")
          ||(_type == "48dB Butterworth Low"))
        {
            return false;
        }
        else
        {
            return true;
        }
    }
    int UseIIRCount()
    {
        if (_type == "18dB Bessel High")
        {
            return 2;
        }
        else if (_type == "18dB Bessel Low")
        {
            return 2;
        }
        else if (_type == "24dB Bessel High")
        {
            return 2;
        }
        else if (_type == "24dB Bessel Low")
        {
            return 2;
        }
        else if (_type == "48dB Bessel High")
        {
            return 4;
        }
        else if (_type == "48dB Bessel Low")
        {
            return 4;
        }

        else if (_type == "24dB Linkwitz-Riley High")
        {
            return 2;
        }
        else if (_type == "24dB Linkwitz-Riley Low")
        {
            return 2;
        }
        else if (_type == "48dB Linkwitz-Riley High")
        {
            return 4;
        }
        else if (_type == "48dB Linkwitz-Riley Low")
        {
            return 4;
        }

        else if (_type == "18dB Butterworth Low")
        {
            return 2;
        }
        else if (_type == "18dB Butterworth High")
        {
            return 2;
        }
        else if (_type == "24dB Butterworth High")
        {
            return 2;
        }
        else if (_type == "24dB Butterworth Low")
        {
            return 2;
        }
        else if (_type == "36dB Butterworth High")
        {
            return 3;
        }
        else if (_type == "36dB Butterworth Low")
        {
            return 3;
        }
        else if (_type == "48dB Butterworth High")
        {
            return 4;
        }
        else if (_type == "48dB Butterworth Low")
        {
            return 4;
        }

        else
        {
            return 1;
        }
    }
    void SetType(String type)
    {
        ChangFilterParameter(type, _freq, _gain, _q);
    }
    void ChangFilterParameter(String type, double freq, double gain, double q);
    
    void EqualizerOff()
    {
        a0 = 1;
        a1 = 0;
        a2 = 0;
        b0 = 1;
        b1 = 0;
        b2 = 0;

        PrepreMiddle();
    }
    void Peaking(double freq, double gain, double q)
    {
        double wc = 2 * M_PI * freq / SAMPLE_FREQ;
        double A = Power(10.0, gain / 40);
        double sin_wc = sin(wc);
        double cos_wc = cos(wc);
        double alpha = sin_wc / (2 * q);

        a0 = 1 + alpha / A;
        a1 = -2 * cos_wc;
        a2 = 1 - alpha / A;
        b0 = 1 + alpha * A;
        b1 = -2 * cos_wc;
        b2 = 1 - alpha * A;

        PrepreMiddle();
    }
    void BandPass(double freq, double gain, double q)
    {
        double wc = 2 * M_PI * freq / SAMPLE_FREQ;
        double A = Power(10.0, gain / 40);
        double sin_wc = sin(wc);
        double cos_wc = cos(wc);
        double alpha = sin_wc / (2 * q);

        a0 = 1 + alpha;
        a1 = -2 * cos_wc;
        a2 = 1 - alpha;
        b0 = alpha;
        b1 = 0;
        b2 = -alpha;

        PrepreMiddle();
    }
    void HighShelving(double freq, double gain, double q)
    {
        double wc = 2 * M_PI * freq / SAMPLE_FREQ;
        double A = Power(10.0, gain / 40);
        double sin_wc = sin(wc);
        double cos_wc = cos(wc);
        double alpha = sin_wc / (2 * q);
        double beta = sqrt(2 * A);

        a0 = (A + 1) - (A - 1) * cos_wc + beta * sin_wc;
        a1 = 2 * ((A - 1) - (A + 1) * cos_wc);
        a2 = (A + 1) - (A - 1) * cos_wc - beta * sin_wc;
        b0 = A * ((A + 1) + (A - 1) * cos_wc + beta * sin_wc);
        b1 = -2 * A * ((A - 1) + (A + 1) * cos_wc);
        b2 = A * ((A + 1) + (A - 1) * cos_wc - beta * sin_wc);

        PrepreMiddle();
    }
    void LowShelving(double freq, double gain, double q)
    {
        double wc = 2 * M_PI * freq / SAMPLE_FREQ;
        double A = Power(10.0, gain / 40);
        double sin_wc = sin(wc);
        double cos_wc = cos(wc);
        double alpha = sin_wc / (2 * q);
        double beta = sqrt(2 * A);

        a0 = (A + 1) + (A - 1) * cos_wc + beta * sin_wc;
        a1 = -2 * ((A - 1) + (A + 1) * cos_wc);
        a2 = (A + 1) + (A - 1) * cos_wc - beta * sin_wc;
        b0 = A * ((A + 1) - (A - 1) * cos_wc + beta * sin_wc);
        b1 = 2 * A * ((A - 1) - (A + 1) * cos_wc);
        b2 = A * ((A + 1) - (A - 1) * cos_wc - beta * sin_wc);

        PrepreMiddle();
    }
    void Notch(double freq, double gain, double q)
    {
        double wc = 2 * M_PI * freq / SAMPLE_FREQ;
        double sin_wc = sin(wc);
        double cos_wc = cos(wc);
        double alpha = sin_wc / (2 * q);

        a0 = 1 + alpha;
        a1 = -2 * cos_wc;
        a2 = 1 - alpha;
        b0 = 1;
        b1 = -2 * cos_wc;
        b2 = 1;

        PrepreMiddle();
    }
    void Pink(double freq, double gain, double q)
    {
        //
        double alpha = 1.0;
        a0 = 1;
        a1 = (1-1-alpha/2.0)*a0/1;
        a2 = (2-1-alpha/2.0)*a1/2;
        b0 = 1;
        b1 = 0;
        b2 = 0;

        b0 = 1;
        b1 = 0.5;
        b2 = -0.125;
        a0 = 1;
        a1 = -0.5;
        a2 = -0.125;
/**/
        PrepreMiddle();

        /*Coefficient second_filter;
        second_filter.LowPassButterworth4_second(freq);
        second_filter.AddToMiddle(middles);*/
    }

    void HighLinkwitz_12dB(double freq)
    {
		float wc = 2 * M_PI * freq;
		float k = wc / tan(wc/2/SAMPLE_FREQ);

        b2 = k*k/(wc*wc+k*k+2*k*wc);
        b1 = (-2.0) * b2;
        b0 = b2;
        a2 = (wc*wc+k*k-2*k*wc)/(wc*wc+k*k+2*k*wc);
        a1 = (2*wc*wc-2*k*k)/(wc*wc+k*k+2*k*wc);
        a0 = 1;

        PrepreMiddle();
    }
    void LowLinkwitz_12dB(double freq)
    {
		float wc = 2 * M_PI * freq;
		float k = wc / tan(wc/2/SAMPLE_FREQ);

        b2 = wc*wc/(wc*wc+k*k+2*k*wc);
        b1 = 2 * b2;
        b0 = b2;
        a2 = (wc*wc+k*k-2*k*wc)/(wc*wc+k*k+2*k*wc);
        a1 = (2*wc*wc-2*k*k)/(wc*wc+k*k+2*k*wc);
        a0 = 1;

        PrepreMiddle();
    }
    void HighLinkwitz_24dB(double freq)
    {
        EqualizerOff();

        Coefficient second_filter;
        second_filter.HighPassButterworth_12dB(freq);
        second_filter.AddToMiddle(middles);
        second_filter.AddToMiddle(middles);
    }
    void LowLinkwitz_24dB(double freq)
    {
        EqualizerOff();

        Coefficient second_filter;
        second_filter.LowPassButterworth_12dB(freq);
        second_filter.AddToMiddle(middles);
        second_filter.AddToMiddle(middles);
    }
    void HighLinkwitz_48dB(double freq)
    {
        EqualizerOff();

        Coefficient second_filter;
        second_filter.HighPassButterworth_24dB(freq);
        second_filter.AddToMiddle(middles);
        second_filter.AddToMiddle(middles);
    }
    void LowLinkwitz_48dB(double freq)
    {
        EqualizerOff();

        Coefficient second_filter;
        second_filter.LowPassButterworth_24dB(freq);
        second_filter.AddToMiddle(middles);
        second_filter.AddToMiddle(middles);
    }

    void HighBansen(double freq)
    {
        double wc = tan(M_PI * freq / SAMPLE_FREQ);

		b2 = 0;
		b1 = (-1)/(wc+1);
		b0 = (-1)*b1;
		a2 = 0;
		a1 = (wc-1)/(wc+1);
        a0 = 1;

        PrepreMiddle();
    }
    void LowBansen(double freq)
    {
        double wc = tan(M_PI * freq / SAMPLE_FREQ);

        b2 = 0;
        b1 = wc/(wc+1);
        b0 = b1;
        a2 = 0;
        a1 = (wc-1)/(wc+1);
        a0 = 1;

        PrepreMiddle();
    }

    void HighBessel_12dB(double freq)
    {
        double wc = tan(M_PI * freq / SAMPLE_FREQ)*1.36165412871613;

        float pk = 3.0000000000;
        float qk = 3.0000000000;
        float f_temp = wc*wc + pk*wc + qk;

        b2 = (3)/f_temp;
        b1 = (-2)*b2;
        b0 = b2;
        a2 = (wc*wc-pk*wc+qk)/f_temp;
        a1 = (2*wc*wc-2*qk)/f_temp;
        a0 = 1;

        PrepreMiddle();
    }
    void LowBessel_12dB(double freq)
    {
        double wc = tan(M_PI * freq / SAMPLE_FREQ) / 1.36165412871613;

        float pk = 3.0000000000;
        float qk = 3.0000000000;
        float f_temp = 1 + pk*wc + qk*wc*wc;

        b2 = (3*wc*wc)/f_temp;
        b1 = 2 * b2;
        b0 = b2;
        a2 = (1-pk*wc+qk*wc*wc)/f_temp;
        a1 = (2*qk*wc*wc-2)/f_temp;
        a0 = 1;

        PrepreMiddle();
    }
    void HighBessel_18dB(double freq)
    {
        Coefficient second_filter;
        double wc = tan(M_PI * freq / SAMPLE_FREQ)*1.75567236868121;
        {
        float pk = 3.6778146454;
        float qk = 6.4594326935;
        float f_temp = wc*wc + pk*wc + qk;

        b2 = (1)/f_temp;
        b1 = (-2)*b2;
        b0 = b2;
        a2 = (wc*wc-pk*wc+qk)/f_temp;
        a1 = (2*wc*wc-2*qk)/f_temp;
        a0 = 1;
        PrepreMiddle();
        AddToMiddle(second_filter.middles);
        }

        b2 = 0;
        b1 = (15)/(-2.3221853546-wc);
        b0 = (-1)*b1;
        a2 = 0;
        a1 = (wc-2.3221853546)/(wc+2.3221853546);
        a0 = 1;
        PrepreMiddle();
        second_filter.AddToMiddle(middles);
    }
    void LowBessel_18dB(double freq)
    {
        Coefficient second_filter;
        double wc = tan(M_PI * freq / SAMPLE_FREQ) / 1.75567236868121;
        {
        float pk = 3.6778146454;
        float qk = 6.4594326935;
        float f_temp = 1 + pk*wc + qk*wc*wc;

        b2 = (wc*wc)/f_temp;
        b1 = 2 * b2;
        b0 = b2;
        a2 = (1-pk*wc+qk*wc*wc)/f_temp;
        a1 = (2*qk*wc*wc-2)/f_temp;
        a0 = 1;
        PrepreMiddle();
        AddToMiddle(second_filter.middles);
        }

        b2  = 0;
        b1 = 15*wc/(1+wc*2.3221853546);
        b0 = b1;
        a2 = 0;
        a1 = (-1+wc*2.3221853546)/(1+wc*2.3221853546);
        a0 = 1;
        PrepreMiddle();
        second_filter.AddToMiddle(middles);
    }
    void HighBessel_24dB(double freq)
    {
        Coefficient second_filter;
        double wc = tan(M_PI * freq / SAMPLE_FREQ)*2.11391767490422;
        {
        float pk = 4.2075787944;
        float qk = 11.4878004771;
        float f_temp = wc*wc + pk*wc + qk;

        b2 = (1)/f_temp;
        b1 = (-2)*b2;
        b0 = b2;
        a2 = (wc*wc-pk*wc+qk)/f_temp;
        a1 = (2*wc*wc-2*qk)/f_temp;
        a0 = 1;
        PrepreMiddle();
        AddToMiddle(second_filter.middles);
        }

        float pk = 5.7924212056;
        float qk = 9.1401308900;
        float f_temp = wc*wc + pk*wc + qk;

        b2 = (105)/f_temp;
        b1 = (-2)*b2;
        b0 = b2;
        a2 = (wc*wc-pk*wc+qk)/f_temp;
        a1 = (2*wc*wc-2*qk)/f_temp;
        a0 = 1;
        PrepreMiddle();
        second_filter.AddToMiddle(middles);
    }

    void LowBessel_24dB(double freq)
    {
        Coefficient second_filter;
        double wc = tan(M_PI * freq / SAMPLE_FREQ) / 2.11391767490422;

        {
        float pk = 4.2075787944;
        float qk = 11.4878004771;
        float f_temp = 1 + pk*wc + qk*wc*wc;

        b2 = (wc*wc)/f_temp;
        b1 = 2 * b2;
        b0 = b2;
        a2 = (1-pk*wc+qk*wc*wc)/f_temp;
        a1 = (2*qk*wc*wc-2)/f_temp;
        a0 = 1;
        PrepreMiddle();
        AddToMiddle(second_filter.middles);
        }

        float pk = 5.7924212056;
        float qk = 9.1401308900;
        float f_temp = 1 + pk*wc + qk*wc*wc;

        b2  = (105*wc*wc)/f_temp;
        b1 = 2 * b2;
        b0 = b2;
        a2 = (1-pk*wc+qk*wc*wc)/f_temp;
        a1 = (2*qk*wc*wc-2)/f_temp;
        a0 = 1;
        PrepreMiddle();
        second_filter.AddToMiddle(middles);
    }

    void HighPassButterworth_12dB(double freq)
    {
        double wc = tan(M_PI * freq / SAMPLE_FREQ);
        double k = 2 * wc * sin(M_PI / 4);

        a0 = 1;
        a1 = (2 * wc * wc - 2) / (1 + k + wc * wc);
        a2 = (1 - k + wc * wc) / (1 + k + wc * wc);
        b0 = 1 / (1 + k + wc * wc);
        b1 = (-2) * b0;
        b2 = 1 / (1 + k + wc * wc);

        PrepreMiddle();
    }
    void HighPassButterworth_18dB(double freq)
    {
        Coefficient second_filter;

        double wc = tan(M_PI * freq / SAMPLE_FREQ);
        double k;

        k = 2 * wc * sin(M_PI / 6);
        a0 = 1;
        a1 = (2*wc*wc-2)/(1+k+wc*wc);
        a2 = (1-k+wc*wc)/(1+k+wc*wc);
        b0 = 1/(1+k+wc*wc);
        b1 = (-2)*b0;
        b2 = 1/(1+k+wc*wc);
        PrepreMiddle();
        AddToMiddle(second_filter.middles);

        k = 2 * wc * sin(M_PI * 3 / 8);
        a0 = 1;
        a1 = (wc-1)/(wc+1);
        a2 = 0;
        b0 = 1/(wc+1);
        b1 = (-1)/(wc+1);
        b2 = 0;
        PrepreMiddle();
        second_filter.AddToMiddle(middles);
    }

    void HighPassButterworth_24dB(double freq)
    {
        Coefficient second_filter;
        {
        double wc = tan(M_PI * freq / SAMPLE_FREQ);
        double k = 2 * wc * sin(M_PI / 8);

        a0 = 1;
        a1 = (2 * wc * wc - 2) / (1 + k + wc * wc);
        a2 = (1 - k + wc * wc) / (1 + k + wc * wc);
        b0 = 1 / (1 + k + wc * wc);
        b1 = (-2) * b0;
        b2 = 1 / (1 + k + wc * wc);
        PrepreMiddle();
        AddToMiddle(second_filter.middles);
        }

        double wc = tan(M_PI * freq / SAMPLE_FREQ);
        double k = 2 * wc * sin(M_PI * 3 / 8);
        a0 = 1;
        a1 = (2 * wc * wc - 2) / (1 + k + wc * wc);
        a2 = (1 - k + wc * wc) / (1 + k + wc * wc);
        b0 = 1 / (1 + k + wc * wc);
        b1 = (-2) * b0;
        b2 = 1 / (1 + k + wc * wc);
        PrepreMiddle();
        second_filter.AddToMiddle(middles);
    }

    void HighPassButterworth_48dB(double freq)
    {
        Coefficient second_filter;

        double wc = tan(M_PI * freq / SAMPLE_FREQ);
        double k;

        k = 2 * wc * sin(1 * M_PI / 16);
        a0 = 1;
        a1 = (2 * wc * wc - 2) / (1 + k + wc * wc);
        a2 = (1 - k + wc * wc) / (1 + k + wc * wc);
        b0 = 1 / (1 + k + wc * wc);
        b1 = (-2) * b0;
        b2 = 1 / (1 + k + wc * wc);
        PrepreMiddle();
        AddToMiddle(second_filter.middles);

        k = 2 * wc * sin(3 * M_PI / 16);
        a0 = 1;
        a1 = (2 * wc * wc - 2) / (1 + k + wc * wc);
        a2 = (1 - k + wc * wc) / (1 + k + wc * wc);
        b0 = 1 / (1 + k + wc * wc);
        b1 = (-2) * b0;
        b2 = 1 / (1 + k + wc * wc);
        PrepreMiddle();
        AddToMiddle(second_filter.middles);

        k = 2 * wc * sin(5 * M_PI / 16);
        a0 = 1;
        a1 = (2 * wc * wc - 2) / (1 + k + wc * wc);
        a2 = (1 - k + wc * wc) / (1 + k + wc * wc);
        b0 = 1 / (1 + k + wc * wc);
        b1 = (-2) * b0;
        b2 = 1 / (1 + k + wc * wc);
        PrepreMiddle();
        AddToMiddle(second_filter.middles);

        k = 2 * wc * sin(7 * M_PI / 16);
        a0 = 1;
        a1 = (2 * wc * wc - 2) / (1 + k + wc * wc);
        a2 = (1 - k + wc * wc) / (1 + k + wc * wc);
        b0 = 1 / (1 + k + wc * wc);
        b1 = (-2) * b0;
        b2 = 1 / (1 + k + wc * wc);
        PrepreMiddle();
        second_filter.AddToMiddle(middles);
    }
    void LowPassButterworth_12dB(double freq)
    {
        double wc = tan(M_PI * freq / SAMPLE_FREQ);
        double k = 2 * wc * sin(M_PI / 4);

        a0 = 1;
        a1 = (2 * wc * wc - 2) / (1 + k + wc * wc);
        a2 = (1 - k + wc * wc) / (1 + k + wc * wc);
        b0 = (wc * wc) / (1 + k + wc * wc);
        b1 = 2 * b0;
        b2 = (wc * wc) / (1 + k + wc * wc);

        PrepreMiddle();
    }
    void LowPassButterworth_18dB(double freq)
    {
        Coefficient second_filter;

        double wc = tan(M_PI * freq / SAMPLE_FREQ);
        double k;

        k = 2 * wc * sin(M_PI / 6);
        a0 = 1;
        a1 = (2*wc*wc-2)/(1+k+wc*wc);
        a2 = (1-k+wc*wc)/(1+k+wc*wc);
        b0 = (wc*wc)/(1+k+wc*wc);
        b1 = 2 * (wc*wc)/(1+k+wc*wc);
        b2 = (wc*wc)/(1+k+wc*wc);
        PrepreMiddle();
        AddToMiddle(second_filter.middles);

        k = 2 * wc * sin(M_PI * 3 / 4);
        a0 = 1;
        a1 = (wc-1)/(wc+1);
        a2 = 0;
        b0 = wc/(wc+1);
        b1 = wc/(wc+1);
        b2 = 0;
        PrepreMiddle();
        second_filter.AddToMiddle(middles);
    }
    void LowPassButterworth_24dB(double freq)
    {
        Coefficient second_filter;
        {
        double wc = tan(M_PI * freq / SAMPLE_FREQ);
        double k = 2 * wc * sin(M_PI / 8);

        a0 = 1;
        a1 = (2 * wc * wc - 2) / (1 + k + wc * wc);
        a2 = (1 - k + wc * wc) / (1 + k + wc * wc);
        b0 = (wc * wc) / (1 + k + wc * wc);
        b1 = 2 * b0;
        b2 = (wc * wc) / (1 + k + wc * wc);
        PrepreMiddle();
        AddToMiddle(second_filter.middles);
        }

        double wc = tan(M_PI * freq / SAMPLE_FREQ);
        double k = 2 * wc * sin(M_PI * 3 / 8);

        a0 = 1;
        a1 = (2 * wc * wc - 2) / (1 + k + wc * wc);
        a2 = (1 - k + wc * wc) / (1 + k + wc * wc);
        b0 = (wc * wc) / (1 + k + wc * wc);
        b1 = 2 * b0;
        b2 = (wc * wc) / (1 + k + wc * wc);
        PrepreMiddle();
        second_filter.AddToMiddle(middles);
    }
    void LowPassButterworth_48dB(double freq)
    {
        Coefficient second_filter;

        double wc = tan(M_PI * freq / SAMPLE_FREQ);
        double k;

        k = 2 * wc * sin(1 * M_PI / 16);
        a0 = 1;
        a1 = (2 * wc * wc - 2) / (1 + k + wc * wc);
        a2 = (1 - k + wc * wc) / (1 + k + wc * wc);
        b0 = (wc * wc) / (1 + k + wc * wc);
        b1 = 2 * b0;
        b2 = (wc * wc) / (1 + k + wc * wc);
        PrepreMiddle();
        AddToMiddle(second_filter.middles);

        k = 2 * wc * sin(3 * M_PI / 16);
        a0 = 1;
        a1 = (2 * wc * wc - 2) / (1 + k + wc * wc);
        a2 = (1 - k + wc * wc) / (1 + k + wc * wc);
        b0 = (wc * wc) / (1 + k + wc * wc);
        b1 = 2 * b0;
        b2 = (wc * wc) / (1 + k + wc * wc);
        PrepreMiddle();
        AddToMiddle(second_filter.middles);

        k = 2 * wc * sin(5 * M_PI / 16);
        a0 = 1;
        a1 = (2 * wc * wc - 2) / (1 + k + wc * wc);
        a2 = (1 - k + wc * wc) / (1 + k + wc * wc);
        b0 = (wc * wc) / (1 + k + wc * wc);
        b1 = 2 * b0;
        b2 = (wc * wc) / (1 + k + wc * wc);
        PrepreMiddle();
        AddToMiddle(second_filter.middles);

        k = 2 * wc * sin(7 * M_PI / 16);
        a0 = 1;
        a1 = (2 * wc * wc - 2) / (1 + k + wc * wc);
        a2 = (1 - k + wc * wc) / (1 + k + wc * wc);
        b0 = (wc * wc) / (1 + k + wc * wc);
        b1 = 2 * b0;
        b2 = (wc * wc) / (1 + k + wc * wc);
        PrepreMiddle();
        second_filter.AddToMiddle(middles);
    }

    void AddToMiddle(double a_middles[1001])
    {
        for (int i=0;i<1001;i++)
        {
            a_middles[i] += middles[i];
        }
    }

    double FilterCoe(double w)
    {
        double sinwt = sin(2 * M_PI * w);
        double coswt = cos(2 * M_PI * w);
        double sin2wt = sin(2 * 2 * M_PI * w);
        double cos2wt = cos(2 * 2 * M_PI * w);

        double result =
             (pow(b0 + b1 * coswt + b2 * cos2wt, 2)
            + pow(     b1 * sinwt + b2 * sin2wt, 2))
            /
             (pow(a0 + a1 * coswt + a2 * cos2wt, 2)
            + pow(     a1 * sinwt + a2 * sin2wt, 2));

        result = sqrt(result);

        return result;
    }

    static void InitPointData(double middles[1001])
    {
        for (int i=0;i<1001;i++)
        {
            middles[i] = 0.0;
        }
    }

    double GetMaxGain()
    {
        // High Shelf 和 Low Shelf最高Gain不超过12
        if (_type_id == 3 || _type_id == 4)
        {
            return 12;
        }
        else
        {
            return 15;
        }
    }

private:
    double a0, a1, a2;
    double b0, b1, b2;

    // 1000个间隔
    double freq_point[1001];
    void InitFreqPoint()
    {
        double freqtemp = 20;
        for (int i=0; i<1000; i++)
        {
            freq_point[i] = freqtemp;
            freqtemp = freqtemp * FREQ_INTERVAL_RATIO;
            freqtemp = std::min(freqtemp, MAX_FREQ);
        }
        freq_point[1000] = MAX_FREQ;
    }

    double middles[1001];
    // 用二阶滤波器方式生成图像
    void PrepreMiddle();

    String _type;
    int _type_id;
    double _freq;
    double _gain;
    double _q;

    static String GetTypeName(int type_id);
};

void InitConfigMap(ConfigMap&);
//---------------------------------------------------------------------------
#endif
