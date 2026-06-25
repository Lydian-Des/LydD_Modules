#include "LydBase.h"
#include <math.h>



namespace LydD {

    int modN(int k, int n) {
        return ((k %= n) < 0) ? k + n : k;
    }
    int ceilN(float x) {
        return (x < 0) ? (int)floor(x) : (int)ceil(x);
    }

    template<>
    int wraparound(int val, int max) {
        int mod = 0;
        if (max == 0) return mod;
        if (val < 0) {
            mod = (-val / max) + 1;
        }
        return (val + max * mod) % max;
    }
    //i think this one would break if 'val' jumped backward enough to wrap beyond INT_MAX - max
    template<>
    size_t wraparound(size_t val, size_t max) {
        size_t mod = 0;
        if (max == 0) return mod;
        //size_t's dont go negative, they wrap to int_max
        if (val > INT_MAX - max) {
            size_t dif = INT_MAX - val;
            val = max - dif;
            mod = (-val / max) + 1;
        }
        return (val + max * mod) % max;
    }

    void HannWindow(float time, float* val, float* window) {
        float wind = 1.f - (1.f * pow(cos(_PI * time), 4.f));
        if (window) *window = wind;
        *val *= wind;
        return;
    }

    void TriWindow(float time, float* val, float* window) {
        float wind = (time < 0.5) ? time * 2.f : (-(time * 2.f) + 2.f);
        if (window) *window = wind;
        *val *= wind;
        return;
    }

    float incrementToward(float here, float there, float factor) {
        return (there - here) / factor;
    }
    rack::simd::float_4 incrementToward(rack::simd::float_4 here, rack::simd::float_4 there, float factor) {
        return (there.v - here.v) / factor;
    }


    float incrementSize(float pitch, float sampleRate) {
        return _2_PI * pitch / sampleRate;
    }
    //float_4 specialization for phase incrementing
    template<>
    void incrementPhase<rack::simd::float_4>(rack::simd::float_4 pitch, float sampleRate, rack::simd::float_4* phase, rack::simd::float_4 limit) {
        rack::simd::float_4 incremPhase;
        rack::simd::float_4 circle = limit;
        rack::simd::float_4 sgnmask = pitch.v < 0.f;
        incremPhase = _2_PI * pitch / sampleRate;
        rack::simd::float_4 compMaskneg = *phase >= -circle;

        rack::simd::float_4 compMask = *phase <= circle;

        *phase += rack::simd::ifelse(sgnmask,
            rack::simd::ifelse((compMaskneg), incremPhase, (circle)),
            rack::simd::ifelse((compMask), incremPhase, -(circle)));
    }
}

