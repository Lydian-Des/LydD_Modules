#pragma once
#include "rack.hpp"
#include <math.h>


namespace LydD {
    //why doesnt C++ have a frgn sgn function
    template <typename T>
    int sgn(T val) {
        return ((T)0 < val) - (val < (T)0);
    }
//mod and ceil from repelzen for working with negatives
    int modN(int k, int n);
    int ceilN(float x);

//negative wrapping modulo for most types
    template<typename T = float>
    T wraparound(T val, T max) {
        T mod = { 0 };
        mod = rack::simd::ifelse(val < mod, (-val / max) + 1, 0);
        return rack::simd::fmod(val + max * mod, max);
    }
    template<>
    int wraparound<int>(int val, int max);
    template<>
    size_t wraparound<size_t>(size_t val, size_t max);

    //min must always be less than max
    template<typename T = float>
    T wrapFree(T val, T min, T max) {
        T r = max - min;
        T l = rack::simd::fmod(val - min, r);
        return min + rack::simd::fmod(l + r, r);
    }

//really just rescale etc but i maked it myself before i started this so im keepin it
    template <typename T = float>
    T lerp(T newMin, T newMax, T oldMin, T oldMax, T L) {
        return (((L - oldMin) * (newMax - newMin)) / (oldMax - oldMin)) + newMin;
    }
//exponential version
    template <typename T = float>
    T exponlerp(T newmin, T newmax, T oldmin, T oldmax, T pos) {
        return newmin * rack::simd::pow((newmax / newmin), ((pos - oldmin) / (oldmax - oldmin)));
    }



//naive normalizing curving function. uses 'Curve' to shift 
// from 0= no change, 1= exponential, and -1= nearly logarithmic, more like inverted exponential
    template<typename T = float>
    T normalCurve(T lowlim, T uplim, T Val, T Curve) {
        //create normalized value, curvePar the value, rescale 
        T normval = lerp((T)-1, (T)1, lowlim, uplim, Val);
        T curval = lerp((T)1, sgn(normval) * normval, (T)0, (T)1, Curve);
        T curvenorm = normval * curval;
        //rescale out of normalized
        return lerp(lowlim, uplim, (T)-1, (T)1, curvenorm);
    }

//cubic lerp between samples 1 and 2 by looking at 0 and 3
// (Buf must be array of bsize, rdpt itself must be < bsize) 
// treats 'Buf' as circular buffer- you want rdpt to be a point at least 2 samples in 'the past'
    template <typename T = float>
    T cubicLerp(T* Buf, size_t rdpt, float f, size_t bsize) {
        T fr = abs(f - (int)f); //make sure fr is between 0 - 1
        T frsq = fr * fr;
        T x0 = Buf[wraparound(rdpt - 1, bsize)];
        T x1 = Buf[rdpt];
        T x2 = Buf[wraparound(rdpt + 1, bsize)];
        T x3 = Buf[wraparound(rdpt + 2, bsize)];
        T a = -0.5f * x0 + 1.5f * x1 - 1.5f * x2 + 0.5f * x3;
        T b = x0 - 2.5f * x1 + 2.f * x2 - 0.5f * x3;
        T c = -0.5f * x0 + 0.5f * x2;
        T d = x1;
        return (a * fr * frsq) + (b * frsq) + (c * fr) + d;
    }
//a sqrt approx i found, forget where
    template <typename T = float>
    T loosesqrt(T x) {
        float g = x / 2.f;
        for (int n = 0; n < 4; ++n) {
            float gtemp = g;
            g = (gtemp + (x / gtemp)) / 2.f;
        }
        return g;
    }

//Hann and trangular windows
    //NOTE::: this Hann is actually raised to the 4th
    void HannWindow(float time, float* val, float* window = nullptr);
    void TriWindow(float time, float* val, float* window = nullptr);

//incrementers
    float incrementToward(float here, float there, float factor);
    //this one's for a 4d coordinate.
    rack::simd::float_4 incrementToward(rack::simd::float_4 here, rack::simd::float_4 there, float factor);
    // gives dt of phase for given pitch in 2PI ratio
    float incrementSize(float pitch, float sampleRate);
    //this phase incrementer allows for different limits, defaulting to 2PI. note smaller limit is 'faster' etc.
    template <typename T = float>
    void incrementPhase(T pitch, float sampleRate, T* phase, T limit = (T)_2_PI) {
        T incremPhase = limit * pitch / sampleRate;
        *phase += incremPhase;
        if (*phase >= limit) {
            *phase -= limit;
        }
    }
    template<>
    void incrementPhase<rack::simd::float_4>(rack::simd::float_4 pitch, float sampleRate, rack::simd::float_4* phase, rack::simd::float_4 limit);


//basic Buttons
//  NOTE::: types can be the same
    //increments value wrapped by limit each time pressed( press > 1)
    template<typename T = float, typename B = bool, typename I = int>
    void incrementButton(T press, B* set, I limit, I* value) {
        I Zmask = 0;
        I mask = 1;
        I val = *value;
        //while the button isnt being pressed, set is 'false'
        B preset = *set;
        *set = rack::simd::ifelse(press < mask, Zmask, mask);
        //at the moment the button is pressed, preset != set, specifically preset==0 < set==1
        val = rack::simd::ifelse(preset < *set, val + mask, val);
        val = rack::simd::fmod(val, limit);
        *value = val;
    }
    //returns set == true (1) on the frame the button is pressed( press > 1)  
    template<typename T = float, typename B= bool>
    void momentButton(T press, B* set, B* reset) {
        B Zmask = 0;
        B mask = 1;
        B preset = *reset;
        *reset = rack::simd::ifelse(press < mask, Zmask, mask);
        //at the moment the button is pressed, preset != reset, specifically preset==0 < reset==1
        *set = rack::simd::ifelse(preset < *reset, mask, Zmask);
    }
    //returns set != set(latching behavior) each time pressed(press > 1)
    template<typename T = float, typename B = bool>
    void latchButton(T press, B* set, B* reset) {
        B Zmask = 0;
        B mask = 1;
        B preset = *reset;
        *reset = rack::simd::ifelse(press < mask, Zmask, mask);
        //tap wil trigger once each time press rises
        B tap = rack::simd::ifelse(preset < *reset, mask, Zmask);
        //if tap ==1 , if set ==0 -> set = 1 ,elseif set == 1 ->set = 0
        B flip = rack::simd::fmod(*set + 1, (T)2);
        *set = rack::simd::ifelse(tap == mask, flip, *set);
    }

}
