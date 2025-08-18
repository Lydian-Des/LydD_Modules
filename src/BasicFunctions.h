#pragma once
#include "rack.hpp"

extern float PI;
extern float _2PI;

struct BaseButtons {


    void incrementButton(float press, bool* set, int limit, int* value);

    void momentButton(float press, bool* set, bool* reset);

    void latchButton(float press, bool* set, bool* reset);

};

struct BaseFunctions {



    float incrementToward(float here, float there, float factor);

    rack::simd::float_4 incrementToward(rack::simd::float_4 here, rack::simd::float_4 there, float factor);


    float lerp(float newMin, float newMax, float oldMin, float oldMax, float L);

    float roundToTwelve(float num);
    float roundToTwelfth(float num);
    float roundToDec(float num, float round);


    float VoltToFreq(float voltage, float refVolt, float refFreq);

    float incrementSize(float pitch, float sampleRate);

    void incrementPhase(float pitch, float sampleRate, float* phase);
    void incrementPhase(rack::simd::float_4 pitch, float sampleRate, rack::simd::float_4* phase, float limit);

};

struct BaseMatrices {
    //simple float type for coordinates
    std::vector<float> MatrixMult(std::vector<std::vector<float>> matrix, std::vector<float> vector);

    //for multiples lines with coordinates concatinated into float_4's
    std::vector<rack::simd::float_4> MatrixMult(std::vector<rack::simd::float_4> matrix, std::vector<rack::simd::float_4> vector);

    std::vector<rack::simd::float_4> Projection(float dist);

    std::vector<rack::simd::float_4> RotationYZ(rack::simd::float_4 angle);

    std::vector<rack::simd::float_4> RotationXZ(rack::simd::float_4 angle);
    std::vector<rack::simd::float_4> RotationXY(rack::simd::float_4 angle);
    std::vector<rack::simd::float_4> RotationZW(rack::simd::float_4 angle);
    std::vector<rack::simd::float_4> RotationYW(rack::simd::float_4 angle);
    std::vector<rack::simd::float_4> RotationXW(rack::simd::float_4 angle);
};

