#pragma once
#include "rack.hpp"
#include <math.h>

namespace LydD {

    //various frequency and time conversions

    float VoltToFreq(float voltage, float refVolt, float refFreq);

    rack::simd::float_4 VoltToFreq(rack::simd::float_4 voltage, rack::simd::float_4 refVolt, rack::simd::float_4 refFreq);
    
    //found via stackoverflow forum post
    template<typename T = float>
    T sinApproxNick(T theta) {
        const T b = 4.f / _PI;
        const T c = -4.f / (_PI *_PI);
        const T magick = 0.224;
        T _t = b * theta + c * theta * rack::simd::abs(theta); 
        _t = magick * (_t * rack::simd::abs(_t) - _t) + _t; 
        return _t;
    }
    template<typename T = float>
    T cosApproxNick(T theta) {
        const T tp = 1.f / (2.f * _PI);
        const T magick = 0.224;
        T _t = theta * tp;
        _t -= T(0.25) + rack::simd::floor(_t + T(0.25));
        _t *= T(16) * (rack::simd::abs(_t) - T(0.5));
        _t += magick * _t * (rack::simd::abs(_t) - T(1));
        return _t;
    }
    // approximation of tan with lower relative error
    //taken from https://andrewkay.name/blog/post/efficiently-approximating-tan-x/ (TA3)
    template <typename T = float>
    inline T tanapp(T x)
    {
        //the actual equation shown is tan(x) = (ax + lx^3) / (1 - (4 / pi^2)x^2
        //where a = 1, and l = (32 / pi^4) - (4 / pi^2);
        //calculation of l results in -0.0767733024195
        //maybe im stupid here but the equation and the code dont seem to match.
        //to match id rewrite it thus:
        static const T l = -0.0767733024195;
        static const T pi_denom = 0.4052847345694; // result of 4 / pi ^ 2
        T xsq = x * x;
        return (x + (l * (xsq * x))) / (1.f - (pi_denom * xsq));
        /*this is what i was given
        const T pisqby4 = 2.4674011002723397f;
        const T adjpisqby4 = 2.471688400562703f;
        const T adj1minus8bypisq = 0.189759681063053f;
        T xsq = x * x;
        return x * (adjpisqby4 - adj1minus8bypisq * xsq) / (pisqby4 - xsq);*/
    }

    //x and y can point to same values as rad and theta
    template<typename T = float>
    void PolartoCart(T rad, T theta, T* x, T* y) {
        T r = rad;
        T t = theta;
        *x = r * cosApproxNick(t);
        *y = r * sinApproxNick(t);
    }


    template <typename T = float>
    T SecToSample(T sec, float samplerate) {
        return sec * samplerate;
    }
    template <typename T = float>
    T MeterToSample(T meters, float samplerate) {
        float sos = 343.2; //meters per second speed of sound
        T sec = meters / sos;
        return SecToSample(sec, samplerate);
    }
    template <typename T = float>
    T MSToSample(T ms, float samplerate) {
        return (ms * 0.001f) * samplerate;
    }
    template <typename T = float>
    T MSToFreq(T ms) {
        ms = (ms == 0) ? (T)1 : ms;
        return 1000.f / ms;
    }
    template <typename T = float>
    T SampleToFreq(T samples, float samplerate) {
        T mask = 0;
        samples = rack::simd::ifelse((samples == mask), (T)1, samples);
        return samplerate / samples;
    }
    //returns fractional sample value
    template <typename T = float>
    T FreqToSampleF(T freq, float samplerate) {
        T mask = 0;
        freq = rack::simd::ifelse((freq == mask), (T)1, freq);
        return samplerate / freq;
    }

    enum class Frequency_Types {
        HZ,
        SAMPLES,
        METERS,
        SECONDS,
        MILLI_SECONDS
    };
    //all return fractional value except Type::SAMPLES, which is just hiding floor
    template<typename T = float>
    T FrequencyToSampleConvert(T f, LydD::Frequency_Types type, float sr) {

        switch (type) {
        default: {}
        case LydD::Frequency_Types::HZ: {
            return FreqToSampleF(f, sr);
        }
        case LydD::Frequency_Types::SAMPLES: {
            return rack::simd::floor(f);
        }
        case LydD::Frequency_Types::METERS: {
            return MeterToSample(f, sr);
        }
        case LydD::Frequency_Types::SECONDS: {
            return SecToSample(f, sr);
        }
        case LydD::Frequency_Types::MILLI_SECONDS: {
            return MSToSample(f, sr);
        }
        }
    }

    //get arrays aligned to 16
    template <typename T = float, size_t S = 4>
    struct alignas(16) niceBlock {
        T* dat;
        void Empty() {
            std::memset(this->dat, 0, sizeof(T) * S);
        }
        niceBlock() {
            this->dat = new T[S];
            Empty();
        }
        ~niceBlock() {
            delete[] this->dat;
        }
    };

}

