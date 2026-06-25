#include "LydMatrix.h"
using float_4 = rack::simd::float_4;
namespace LydD {

namespace Matrix {

    std::vector<float> MatrixMult(std::vector<std::vector<float>> matrix, std::vector<float> vector)
    {             
        int rows1 = matrix.size();
        int cols1 = matrix[0].size();
        std::vector<float> rslt(rows1, 0.0);
        for (int r = 0; r < rows1; r++) {
            for (int c = 0; c < cols1; c++) {
                rslt[r] += matrix[r][c] * vector[c];
            }
        }
        return rslt;
    }
    
    std::vector<float_4> MatrixMult(std::vector<float_4> matrix, std::vector<float_4> vector)
    {
        int rows1 = matrix.size();
        int cols1 = vector.size();

        std::vector<float_4> rslt(rows1, float_4{ 0.0 });
        for (int r = 0; r < rows1; r++) {
            for (int c = 0; c < cols1; c++) {
                rslt[r].v += matrix[c].v * vector[r][c];
            }
        }
        return rslt;
    }

    std::vector<float_4> Projection(float dist) {
        float_4 projX = float_4{ dist, 0, 0, 0 };
        float_4 projY = float_4{ 0, dist, 0, 0 };
        float_4 projZ = float_4{ 0, 0, dist, 0 };
        float_4 projW = float_4{ 0, 0, 0, dist };
        return std::vector<rack::simd::float_4> {projX, projY, projZ, projW};
    }

    std::vector<float_4> RotationYZ(rack::simd::float_4 angle) {
        float_4 XrotateYZ = float_4{ 1, 0, 0, 0 };
        float_4 YrotateYZ = float_4{ 0, cos(angle)[0], -sin(angle)[0], 0 };
        float_4 ZrotateYZ = float_4{ 0, sin(angle)[0], cos(angle)[0], 0 };
        float_4 WrotateYZ = float_4{ 0, 0, 0, 1 };
        return std::vector<float_4> { XrotateYZ,
            YrotateYZ,
            ZrotateYZ,
            WrotateYZ };
    }

    std::vector<float_4> RotationXZ(float_4 angle) {
        float_4 XrotateXZ = float_4{ cos(angle)[0], 0, -sin(angle)[0], 0 };
        float_4 YrotateXZ = float_4{ 0, 1, 0, 0 };
        float_4 ZrotateXZ = float_4{ sin(angle)[0], 0, cos(angle)[0], 0 };
        float_4 WrotateXZ = float_4{ 0, 0, 0, 1 };
        return std::vector<float_4>{ XrotateXZ,
            YrotateXZ,
            ZrotateXZ,
            WrotateXZ };
    }
    std::vector<rack::simd::float_4> RotationXY(rack::simd::float_4 angle) {
        float_4 XrotateXY = float_4{ cos(angle)[0], -sin(angle)[0], 0, 0 };
        float_4 YrotateXY = float_4{ sin(angle)[0], cos(angle)[0], 0, 0 };
        float_4 ZrotateXY = float_4{ 0, 0, 1, 0 };
        float_4 WrotateXY = float_4{ 0, 0, 0, 1 };
        return std::vector<float_4>{ XrotateXY,
            YrotateXY,
            ZrotateXY,
            WrotateXY };
    }
    std::vector<float_4> RotationZW(float_4 angle) {
        float_4 XrotateZW = float_4{ 1, 0, 0, 0 };
        float_4 YrotateZW = float_4{ 0, 1, 0, 0 };
        float_4 ZrotateZW = float_4{ 0, 0, cos(angle)[0], -sin(angle)[0] };
        float_4 WrotateZW = float_4{ 0, 0, sin(angle)[0], cos(angle)[0] };
        return std::vector<float_4>{ XrotateZW,
            YrotateZW,
            ZrotateZW,
            WrotateZW };
    }
    std::vector<float_4> RotationYW(float_4 angle) {
        float_4 XrotateYW = float_4{ 1, 0, 0, 0 };
        float_4 YrotateYW = float_4{ 0, cos(angle)[0], 0, -sin(angle)[0] };
        float_4 ZrotateYW = float_4{ 0, 0, 1, 0 };
        float_4 WrotateYW = float_4{ 0, sin(angle)[0], 0, cos(angle)[0] };
        return std::vector<float_4>{ XrotateYW,
            YrotateYW,
            ZrotateYW,
            WrotateYW };
    }
    std::vector<float_4> RotationXW(float_4 angle) {
        float_4 XrotateXW = float_4{ cos(angle)[0], 0, 0, -sin(angle)[0] };
        float_4 YrotateXW = float_4{ 0, 1, 0, 0 };
        float_4 ZrotateXW = float_4{ 0, 0, 1, 0 };
        float_4 WrotateXW = float_4{ sin(angle)[0], 0, 0, cos(angle)[0] };
        return std::vector<float_4>{ XrotateXW,
            YrotateXW,
            ZrotateXW,
            WrotateXW };
    }


}
}

