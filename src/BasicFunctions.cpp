#include "BasicFunctions.h"



float PI = 3.1415926535897932;
float _2PI = 2.0 * 3.1415926535897932;

    void BaseButtons::incrementButton(float press, bool* set, int limit, int* value) {
        int val = *value;
        if (press == 0.0f) {
            *set = true;
        }
        if (press == 1.0f && *set) {
            val++;
            *set = false;
        }
        val %= limit;
        *value = val;

    }

    void BaseButtons::momentButton(float press, bool* set, bool* reset) {
        if (press <= 0.0f) {
            *reset = true;
        }
        if (press >= 1.0f && *reset) {
            *set = true;
            *reset = false;
        }
        else {
            *set = false;
        }
    }

    void BaseButtons::latchButton(float press, bool* set, bool* reset) {
        if (press == 0.0f) {
            *reset = true;
        }
        if (press == 1.0f && *reset) {
            *set = !*set;
            *reset = false;
        }
    }


    float BaseFunctions::incrementToward(float here, float there, float factor) {
        return (there - here) / factor;
    }

    rack::simd::float_4 BaseFunctions::incrementToward(rack::simd::float_4 here, rack::simd::float_4 there, float factor) {
        return (there.v - here.v) / factor;
    }


    float BaseFunctions::lerp(float newMin, float newMax, float oldMin, float oldMax, float L) {
        // (L - oldMin)* (newMax - newMin) / (oldMax - oldMin) + newMin;
        return (((L - oldMin) * (newMax - newMin)) / (oldMax - oldMin)) + newMin;
    }

    float BaseFunctions::roundToTwelve(float num) {
        return (int)(num / 12) * 12;
    }
    float BaseFunctions::roundToTwelfth(float num) {
        return (int)(num * 20) / 20.0;
    }
    float BaseFunctions::roundToDec(float num, float round) {
        return (int)(num * round) / round;
    }


    float BaseFunctions::VoltToFreq(float voltage, float refVolt, float refFreq) {
        // Calculate the number of octaves
        //float refFreq = 2.05;

        float toNote = (voltage - refVolt);
        //toNote += log2(refFreq - refVolt);
        // Calculate the frequency based on octaves and the reference frequency
        float toFreq = refFreq * pow(2, toNote);

        return toFreq;
    }

    float BaseFunctions::incrementSize(float pitch, float sampleRate) {
        return _2PI * pitch / sampleRate;

    }

    void BaseFunctions::incrementPhase(float pitch, float sampleRate, float* phase) {
        float incremPhase = _2PI * pitch / sampleRate;
        *phase += incremPhase;
        if (*phase >= 2.f * _2PI) {
            *phase -= 2.f * _2PI;
        }
    }
    void BaseFunctions::incrementPhase(rack::simd::float_4 pitch, float sampleRate, rack::simd::float_4* phase, float limit) {
        rack::simd::float_4 incremPhase;
        rack::simd::float_4 circle(limit);

        incremPhase = _2PI * pitch / sampleRate;
        rack::simd::float_4 compMask = *phase <= circle;
        *phase += rack::simd::ifelse((compMask), incremPhase, -(circle));
    }



    std::vector<float> BaseMatrices::MatrixMult(std::vector<std::vector<float>> matrix, std::vector<float> vector)
    {                   //r1 = 3, c1 = 1, r2 = 3, c2 = 3... r1 == c2\/
        int rows1 = matrix.size();
        int cols1 = matrix[0].size();
        //vector must have as many members as matrix has rows

        std::vector<float> rslt(rows1, 0.0);
        for (int r = 0; r < rows1; r++) {
            for (int c = 0; c < cols1; c++) {

                rslt[r] += matrix[r][c] * vector[c];

            }
        }
        return rslt;
    }

    std::vector<rack::simd::float_4> BaseMatrices::MatrixMult(std::vector<rack::simd::float_4> matrix, std::vector<rack::simd::float_4> vector)
    {
        int rows1 = matrix.size();
        int cols1 = vector.size();

        std::vector<rack::simd::float_4> rslt(rows1, rack::simd::float_4{ 0.0 });
        for (int r = 0; r < rows1; r++) {
            for (int c = 0; c < cols1; c++) {

                rslt[r].v += matrix[c].v * vector[r][c];

            }
        }
        return rslt;
    }

    std::vector<rack::simd::float_4> BaseMatrices::Projection(float dist) {
        rack::simd::float_4 projX = rack::simd::float_4{ dist, 0, 0, 0 };
        rack::simd::float_4 projY = rack::simd::float_4{ 0, dist, 0, 0 };
        rack::simd::float_4 projZ = rack::simd::float_4{ 0, 0, dist, 0 };
        rack::simd::float_4 projW = rack::simd::float_4{ 0, 0, 0, dist };
        return std::vector<rack::simd::float_4> {projX, projY, projZ, projW};
    }

    std::vector<rack::simd::float_4> BaseMatrices::RotationYZ(rack::simd::float_4 angle) {
        rack::simd::float_4 XrotateYZ = rack::simd::float_4{ 1, 0, 0, 0 };
        rack::simd::float_4 YrotateYZ = rack::simd::float_4{ 0, cos(angle)[0], -sin(angle)[0], 0 };
        rack::simd::float_4 ZrotateYZ = rack::simd::float_4{ 0, sin(angle)[0], cos(angle)[0], 0 };
        rack::simd::float_4 WrotateYZ = rack::simd::float_4{ 0, 0, 0, 1 };
        return std::vector<rack::simd::float_4> { XrotateYZ,
            YrotateYZ,
            ZrotateYZ,
            WrotateYZ };
    }

    std::vector<rack::simd::float_4> BaseMatrices::RotationXZ(rack::simd::float_4 angle) {
        rack::simd::float_4 XrotateXZ = rack::simd::float_4{ cos(angle)[0], 0, -sin(angle)[0], 0 };
        rack::simd::float_4 YrotateXZ = rack::simd::float_4{ 0, 1, 0, 0 };
        rack::simd::float_4 ZrotateXZ = rack::simd::float_4{ sin(angle)[0], 0, cos(angle)[0], 0 };
        rack::simd::float_4 WrotateXZ = rack::simd::float_4{ 0, 0, 0, 1 };
        return std::vector<rack::simd::float_4>{ XrotateXZ,
            YrotateXZ,
            ZrotateXZ,
            WrotateXZ };
    }
    std::vector<rack::simd::float_4> BaseMatrices::RotationXY(rack::simd::float_4 angle) {
        rack::simd::float_4 XrotateXY = rack::simd::float_4{ cos(angle)[0], -sin(angle)[0], 0, 0 };
        rack::simd::float_4 YrotateXY = rack::simd::float_4{ sin(angle)[0], cos(angle)[0], 0, 0 };
        rack::simd::float_4 ZrotateXY = rack::simd::float_4{ 0, 0, 1, 0 };
        rack::simd::float_4 WrotateXY = rack::simd::float_4{ 0, 0, 0, 1 };
        return std::vector<rack::simd::float_4>{ XrotateXY,
            YrotateXY,
            ZrotateXY,
            WrotateXY };
    }
    std::vector<rack::simd::float_4> BaseMatrices::RotationZW(rack::simd::float_4 angle) {
        rack::simd::float_4 XrotateZW = rack::simd::float_4{ 1, 0, 0, 0 };
        rack::simd::float_4 YrotateZW = rack::simd::float_4{ 0, 1, 0, 0 };
        rack::simd::float_4 ZrotateZW = rack::simd::float_4{ 0, 0, cos(angle)[0], -sin(angle)[0] };
        rack::simd::float_4 WrotateZW = rack::simd::float_4{ 0, 0, sin(angle)[0], cos(angle)[0] };
        return std::vector<rack::simd::float_4>{ XrotateZW,
            YrotateZW,
            ZrotateZW,
            WrotateZW };
    }
    std::vector<rack::simd::float_4> BaseMatrices::RotationYW(rack::simd::float_4 angle) {
        rack::simd::float_4 XrotateYW = rack::simd::float_4{ 1, 0, 0, 0 };
        rack::simd::float_4 YrotateYW = rack::simd::float_4{ 0, cos(angle)[0], 0, -sin(angle)[0] };
        rack::simd::float_4 ZrotateYW = rack::simd::float_4{ 0, 0, 1, 0 };
        rack::simd::float_4 WrotateYW = rack::simd::float_4{ 0, sin(angle)[0], 0, cos(angle)[0] };
        return std::vector<rack::simd::float_4>{ XrotateYW,
            YrotateYW,
            ZrotateYW,
            WrotateYW };
    }
    std::vector<rack::simd::float_4> BaseMatrices::RotationXW(rack::simd::float_4 angle) {
        rack::simd::float_4 XrotateXW = rack::simd::float_4{ cos(angle)[0], 0, 0, -sin(angle)[0] };
        rack::simd::float_4 YrotateXW = rack::simd::float_4{ 0, 1, 0, 0 };
        rack::simd::float_4 ZrotateXW = rack::simd::float_4{ 0, 0, 1, 0 };
        rack::simd::float_4 WrotateXW = rack::simd::float_4{ sin(angle)[0], 0, 0, cos(angle)[0] };
        return std::vector<rack::simd::float_4>{ XrotateXW,
            YrotateXW,
            ZrotateXW,
            WrotateXW };
    }





