#pragma once
#include "Lydapi/LydBase.h"
#include "rack.hpp"
#include <complex>
#include <cmath>


namespace LydD {

namespace Fractal {
    using namespace rack;
    const extern float E;

    using Brot_Pick = std::complex<float>(*)(float, std::complex<float>, std::complex<float>);
    std::complex<float> andrewkayTan(std::complex<float> x);

    std::complex<float> Mandelbrot(float EXP, std::complex<float> C, std::complex<float> Ztemp);

    std::complex<float> BurningShip(float EXP, std::complex<float> C, std::complex<float> Ztemp);

    std::complex<float> Beetle(float EXP, std::complex<float> C, std::complex<float> Ztemp);

    std::complex<float> Bird(float EXP, std::complex<float> C, std::complex<float> Ztemp);

    std::complex<float> Daisy(float EXP, std::complex<float> C, std::complex<float> Ztemp);

    std::complex<float> Unicorn(float EXP, std::complex<float> C, std::complex<float> Ztemp);

    //returns the function itself to reduce # of switches run
    struct brotPicker {

        Brot_Pick chooseFractal(int fractal) {
            switch (fractal) {
            case 0: {
                return Mandelbrot;
                break;
            }
            case 1: {
                return BurningShip;
                break;
            }
            case 2: {
                return Beetle;
                break;
            }
            case 3: {
                return Bird;
                break;
            }
            case 4: {
                return Daisy;
                break;
            }
            case 5: {
                return Unicorn;
                break;
            }
            }
            return nullptr;
        }
    };

    std::complex<float> RunSingle(Brot_Pick brot, std::complex<float> Z, std::complex<float> C, float exp);

    // 'reals and 'imags must be of size 'iters + 1
    int RunVertical(Brot_Pick brot, float* reals, float* imags, int iters, std::complex<float> Z, std::complex<float> C, float exp);
    

    //a method of generating and saving key data per iteration
    struct FractalRenderer {
    private:
        //Z captured by each pixel for for next iteration
        std::complex<float>* Zblock;
        // K 'escape value' reached
        int* Kval;
        // true if Z has escaped or died , stops incrementing K
        bool* isEscape;
        bool* isInSet;
        //# of times Z has touched a pixel, used for BuddhaBrot
        int* touchCount;
        int width;
        int height;
        int MAX_ITERS = 25;
        int totalItersReached = 0;
        void deleteData() {
            if (Zblock != nullptr) delete[] Zblock;
            if (Kval != nullptr) delete[] Kval;
            if (isEscape != nullptr) delete[] isEscape;
            if (isInSet != nullptr) delete[] isInSet;
            if (touchCount != nullptr) delete[] touchCount;
        }
        void init(int w, int h) {
            width = w;
            height = h;
            Zblock = new std::complex<float>[width * height];
            Kval = new int[width * height];
            isEscape = new bool[width * height];
            isInSet = new bool[width * height];
            touchCount = new int[width * height];
        }
    public:


        FractalRenderer() {
            init(160, 120);
            clear();
        }
        FractalRenderer(int w, int h) {
            init(w , h);
            clear();
        }
        ~FractalRenderer() {
            deleteData();
        }
       
        void clear() {
            memset(Kval, 0, (width * height) * sizeof(int));
            memset(isEscape, 0, (width * height) * sizeof(bool));
            memset(isInSet, 0, (width * height) * sizeof(bool));
            memset(touchCount, 0, (width * height) * sizeof(int));
            for (int i = 0; i < width * height; ++i) {
                Zblock[i] = std::complex<float>(0, 0);
            }
            totalItersReached = 0;
        }
        //should only do this once at init
        void setNumIters(int it) {
            this->MAX_ITERS = it;
        }

        //getters for calculated datas
        int getKsingle(int idx) {
            return this->Kval[idx];
        }
        void copyKblock(int* dst) {
            memcpy(dst, this->Kval, sizeof(int) * (width * height));
        }
        //returns smoothed escape value 
        float getKEscapeFraction(int idx) {
            //if (isEscape[idx]) {
                float Zfrac = (std::fmod(abs(Zblock[idx]), 2.f));
                return float(this->Kval[idx]) - log2(Zfrac);
            //}
            //return this->Kval[idx];
        }
        std::complex<float> getZsingle(int idx) {
            return this->Zblock[idx];
        }
        void copyZblock(std::complex<float>* dst) {
            memcpy(dst, this->Zblock, sizeof(std::complex<float>) * (width * height));
        }
        bool getEscapeSingle(int idx) {
            return this->isEscape[idx];
        }
        bool getInSetSingle(int idx) {
            return this->isInSet[idx];
        }
        void copyDone(bool* dst) {
            memcpy(dst, this->isEscape, sizeof(bool) * (width * height));
        }
        int getZtouchsingle(int idx) {
            return this->touchCount[idx];
        }
        void copyZtouches(int* dst) {
            memcpy(dst, this->touchCount, sizeof(int) * (width * height));
        }
        bool checkEscape(std::complex<float> Z) {
            return abs(Z) >= 2.f;
        }
        bool checkInSet(int idx, std::complex<float> Z, std::complex<float> Zprev) {
            if ((Kval[idx] >= MAX_ITERS) ||
                (rack::math::isNear(real(Zprev), real(Z)) && rack::math::isNear(imag(Zprev), imag(Z)))) {
                return true;
            }
            return false;
        }
        

        //runs a single iteration across a whole grid of locations
// C isnt needed here because it is the pixel coordinate in fractal space
// zoom X and Y determine viewing window X(minimum x, maximum x)
//Kval, isEscape, and touchCount defined as 1D array of size width * height
        void RunHorizontal(Brot_Pick brot, std::complex<float> Z, float exp, Vec zoomX, Vec zoomY, bool julia = false) {
            //run a single iteration on each pixel, saving what #K, if escaped(done), 
            //and how many times Z has hit a given pixel 
            for (int h = 0; h < height; ++h) {
                for (int w = 0; w < width; ++w) {
                    int idx = h * width + w;

                    if (isEscape[idx] || isInSet[idx]) {
                        continue;
                    }
                    else {
                        float Xscale = lerp(zoomX.x, zoomX.y, 0.f, (float)width, (float)w);
                        float Yscale = lerp(zoomY.x, zoomY.y, 0.f, (float)height, (float)h);
                        std::complex<float>C(Xscale, Yscale);

                        if (Kval[idx] == 0) {
                            //plant initial Z value on first iteration
                            //julia is also set on first iter but C also needs update for every iteration
                            Zblock[idx] = Z;
                            if (julia) {
                                std::complex<float>Zswap = Z;
                                Zblock[idx] = C;
                                C = Zswap;
                            }
                        }
                        else {
                            if (julia) C = Z;
                        }

                        std::complex<float> zlast = Zblock[idx];
                        Zblock[idx] = RunSingle(brot, zlast, C, exp);

                        //check if escaped or in set and make done if so
                        isEscape[idx] = checkEscape(Zblock[idx]);
                        isInSet[idx] = checkInSet(idx, Zblock[idx], zlast);
                        if(!isEscape[idx] && !isInSet[idx]) {
                            //save current K
                            Kval[idx] += 1;
                            if (Kval[idx] < MAX_ITERS && Kval[idx] > 2 && idx % 2 == 0) {
                                touchZsingle(Zblock[idx], zoomX, zoomY);
                            }
                        }
                    }

                }

            }

            totalItersReached += 1;
            totalItersReached %= MAX_ITERS + 1;
        }
        //touch whatever pixel Z landed on
        void touchZsingle(std::complex<float>Z, Vec zoomX, Vec zoomY) {
            float zx = lerp(0.f, (float)width, zoomX.x, zoomX.y, real(Z));
            int ZpixX = rack::math::clamp(int(zx + 0.5f), 0, width - 1);
            float zy = lerp(0.f, (float)height, zoomY.x, zoomY.y, imag(Z));
            int ZpixY = rack::math::clamp(int(zy + 0.5f), 0, height - 1);
            //remove outermost edges, only touch for points outside the set
            int dix = ZpixY * width + ZpixX;
            ++this->touchCount[dix];
        }
        //the below is actuallly unnecessary
        // 
        //take Z location pointed to by idx, scale, and add to touchCount
        //same zooms as sent to runhorizontal
        void touchZpixels(Brot_Pick brot, int loc, std::complex<float> Z, float exp, Vec zoomX, Vec zoomY, bool julia = false) {
            //just run up to 6 iters
            int ix = loc % width;
            int iy = loc / width;
            float Xscale = lerp(zoomX.x, zoomX.y, 0.f, (float)width, (float)ix);
            float Yscale = lerp(zoomY.x, zoomY.y, 0.f, (float)height, (float)iy);
            std::complex<float>C(Xscale, Yscale);
            std::complex<float>ZP = Z;
            if (julia) {
                std::complex<float>Zswap = ZP;
                ZP = C;
                C = Zswap;
            }
            for (int i = 0; i < 6; ++i) {
                std::complex<float> ZL = ZP;
                ZP = RunSingle(brot, ZL, C, exp);
                if (checkEscape(ZP)) {
                    return;
                }
                //else {
                touchZsingle(ZP, zoomX, zoomY);
                //}

            }
        }
        //once all done, check for boundary ring and do above
        //this way whats inside the set never gets added, which is essential for buddhabrot
        void checkRing(Brot_Pick brot, std::complex<float> Z, float exp, Vec zoomX, Vec zoomY, bool julia = false) {
            for (int b = 0; b < width * height; ++b) {
                //if it is part of the boundary[finished] this iteration
                if (isEscape[b] && Kval[b] == totalItersReached - 1) {
                    touchZpixels(brot, b, Z, exp,  zoomX, zoomY, julia);
                }
            }
        }

    };

}
}

