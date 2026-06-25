#include "LydBase.h"


namespace LydD {

   
    float VoltToFreq(float voltage, float refVolt, float refFreq) {
        // Calculate the number of octaves
        //float refFreq = 2.05;
        float toNote = (voltage - refVolt);
        //toNote += log2(refFreq - refVolt);
        // Calculate the frequency based on octaves and the reference frequency
        float toFreq = refFreq * pow(2, toNote);
        return toFreq;
    }
    rack::simd::float_4 VoltToFreq(rack::simd::float_4 voltage, rack::simd::float_4 refVolt, rack::simd::float_4 refFreq) {
        rack::simd::float_4 toNote = (voltage.v - refVolt.v);
        rack::simd::float_4 toFreq = refFreq.v * rack::simd::pow(2.f, toNote.v);
        return toFreq;
    }

}

