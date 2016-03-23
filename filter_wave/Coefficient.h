//---------------------------------------------------------------------------

#ifndef CoefficientH
#define CoefficientH

#include <math.h>
#include <algorithm>
#include <math.hpp>
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
    void SetType(String type)
    {
        ChangFilterParameter(type, _freq, _gain, _q);
    }
    void ChangFilterParameter(String type, double freq, double gain, double q)
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
        else if (type == "High Butterworth 2nd")
        {
            HighPassButterworth2(freq);
            _type_id = 0;
        }
        else if (type == "High Butterworth 4nd")
        {
            HighPassButterworth4(freq);
            _type_id = 0;
        }
        else if (type == "Low Butterworth 2nd")
        {
            LowPassButterworth2(freq);
            _type_id = 0;
        }
        else if (type == "Low Butterworth 4nd")
        {
            LowPassButterworth4(freq);
            _type_id = 0;
        }
    }
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

    void HighPassButterworth2(double freq)
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
    void HighPassButterworth4(double freq)
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

        Coefficient second_filter;
        second_filter.HighPassButterworth4_second(freq);
        second_filter.AddToMiddle(middles);
    }
    void LowPassButterworth2(double freq)
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
    void LowPassButterworth4(double freq)
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
        
        Coefficient second_filter;
        second_filter.LowPassButterworth4_second(freq);
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

private:
    double a0, a1, a2;
    double b0, b1, b2;

    // 1000¸ö¼ä¸ô
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
    void PrepreMiddle()
    {
        for (int i=0;i<1001;i++)
        {
            middles[i] = FilterCoe(freq_point[i] / SAMPLE_FREQ);

            middles[i] = 20 * Log10(middles[i]);
        }
    }

    void HighPassButterworth4_second(double freq)
    {
        double wc = tan(M_PI * freq / SAMPLE_FREQ);
        double k = 2 * wc * sin(M_PI * 3 / 8);

        a0 = 1;
        a1 = (2 * wc * wc - 2) / (1 + k + wc * wc);
        a2 = (1 - k + wc * wc) / (1 + k + wc * wc);
        b0 = 1 / (1 + k + wc * wc);
        b1 = (-2) * b0;
        b2 = 1 / (1 + k + wc * wc);

        PrepreMiddle();
    }
    void LowPassButterworth4_second(double freq)
    {
        double wc = tan(M_PI * freq / SAMPLE_FREQ);
        double k = 2 * wc * sin(M_PI * 3 / 4);

        a0 = 1;
        a1 = (2 * wc * wc - 2) / (1 + k + wc * wc);
        a2 = (1 - k + wc * wc) / (1 + k + wc * wc);
        b0 = (wc * wc) / (1 + k + wc * wc);
        b1 = 2 * b0;
        b2 = (wc * wc) / (1 + k + wc * wc);

        PrepreMiddle();
    }

    String _type;
    int _type_id;
    double _freq;
    double _gain;
    double _q;
};
//---------------------------------------------------------------------------
#endif
