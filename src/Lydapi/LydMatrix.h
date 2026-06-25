#pragma once
#include "rack.hpp"
#include <math.h>


namespace LydD {

namespace Matrix {
    //basic matrix math for dimaensional rotation and projection
    //for arrays and square matrix, all must have as many members as order
    template <typename T = float>
    void MatrixMult(T* matrix, T* vector, T* rslt, int order) {                   //r1 = 3, c1 = 1, r2 = 3, c2 = 3... r1 == c2\/
        int rows1 = order;
        int cols1 = order;
        //make sure rslt is empty before adding in
        std::memset(rslt, T(0), sizeof(T) * order);
        for (int r = 0; r < rows1; r++) {
            for (int c = 0; c < cols1; c++) {
                rslt[r] += matrix[r * cols1 + c] * vector[c];
            }
        }
    }
    //simple float type for coordinates
    std::vector<float> MatrixMult(std::vector<std::vector<float>> matrix, std::vector<float> vector);
    //for multiples lines with coordinates concatinated into float_4's {X, Y, Z, W}
    std::vector<rack::simd::float_4> MatrixMult(std::vector<rack::simd::float_4> matrix, std::vector<rack::simd::float_4> vector);
    //Mult by these to perform what they say
    //could/should template to use with normal stuff as well
    std::vector<rack::simd::float_4> Projection(float dist);
    std::vector<rack::simd::float_4> RotationYZ(rack::simd::float_4 angle);
    std::vector<rack::simd::float_4> RotationXZ(rack::simd::float_4 angle);
    std::vector<rack::simd::float_4> RotationXY(rack::simd::float_4 angle);
    std::vector<rack::simd::float_4> RotationZW(rack::simd::float_4 angle);
    std::vector<rack::simd::float_4> RotationYW(rack::simd::float_4 angle);
    std::vector<rack::simd::float_4> RotationXW(rack::simd::float_4 angle);


}
}

