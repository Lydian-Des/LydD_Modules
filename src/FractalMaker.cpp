#include "FractalMaker.h"

namespace LydD {

namespace Fractal {

	const float E = 2.7182818284590;

    using Brot_Pick = std::complex<float>(*)(float, std::complex<float>, std::complex<float>);
    std::complex<float> andrewkayTan(std::complex<float> x) {
        const float pisqby4 = 2.4674011002723397f;
        const float oneminus8bypisq = 0.1894305308612978f;
        std::complex<float> xsq = x * x;
        return x * (pisqby4 - oneminus8bypisq * xsq) / (pisqby4 - xsq);
    }

    std::complex<float> Mandelbrot(float EXP, std::complex<float> C, std::complex<float> Ztemp) {
        return pow(Ztemp, EXP) + C;
    }

    std::complex<float> BurningShip(float EXP, std::complex<float> C, std::complex<float> Ztemp) {
        return pow(std::complex<float>(abs(real(Ztemp)), abs(imag(Ztemp))), EXP) + C;
    }

    std::complex<float> Beetle(float EXP, std::complex<float> C, std::complex<float> Ztemp) {
        float realZ = sinApproxNick(real(Ztemp));
        float imagZ = sinApproxNick(imag(Ztemp));
        //rack::simd::float_4 realZ(real(Ztemp));
       // rack::simd::float_4 imagZ(imag(Ztemp));
       // rack::simd::float_4 Zrnew = sin(realZ);
       // rack::simd::float_4 Zinew = sin(imagZ);
        return pow(std::complex<float>(realZ, imagZ), EXP) + C;
    }

    std::complex<float> Bird(float EXP, std::complex<float> C, std::complex<float> Ztemp) {
        rack::simd::float_4 realZ(real(Ztemp));
        rack::simd::float_4 Zrnew = atan(realZ);
        return pow(std::complex<float>(Zrnew[0], abs(imag(Ztemp))), EXP) + C;
    }

    std::complex<float> Daisy(float EXP, std::complex<float> C, std::complex<float> Ztemp) {
        return andrewkayTan(pow(C, EXP) * pow(Ztemp, E)) + (C - Ztemp);
    }

    std::complex<float> Unicorn(float EXP, std::complex<float> C, std::complex<float> Ztemp) {
        return andrewkayTan(pow(Ztemp, EXP) + Ztemp) + (C - Ztemp);
    }

    //brotpicker struct

    //runs single iteration on single location
    std::complex<float> RunSingle(Brot_Pick brot, std::complex<float> Z, std::complex<float> C, float exp) {
        std::complex<float>pastVal = Z;
        return brot(exp, C, pastVal);
    }

    // runs a full set of iterations on a single location
    // 'reals and 'imags must be of size 'iters + 1
    int RunVertical(Brot_Pick brot, float* reals, float* imags, int iters, std::complex<float> Z, std::complex<float> C, float exp) {
        int K = 0;
        //update local copy of Z
        std::complex<float> ZZp = Z;
        for (int i = 0; i <= iters; ++i) {
            std::complex<float> ZZ = RunSingle(brot, ZZp, C, exp);
            if (abs(ZZ) <= 2.f) {
                reals[i] = (real(ZZ));
                imags[i] = (imag(ZZ));
                ++K;
            }
            //if sequence escapes or falls to a single point, stop doing the math early
            else if (abs(ZZ) > 2.f || (rack::math::isNear(real(ZZ), real(ZZp), 0.0416) && rack::math::isNear(imag(ZZ), imag(ZZp), 0.0416))) {
                //if it doesnt get to do any iterations, just copy in the C value
                if (i == 0) {
                    for (int p = 0; p <= iters; ++p) {
                        reals[p] = real(C);
                        imags[p] = imag(C);
                    }
                }
                else {
                    //repeat instead of calculating, way lighter
                    int q = 0;
                    for (int p = i; p <= iters; ++p) {
                        int rptind = q % i;
                        reals[p] = reals[rptind];
                        imags[p] = imags[rptind];
                        ++q;
                    }
                }
                break;

            }
            ZZp = ZZ;
        }
        return K;
    }
    
}
}