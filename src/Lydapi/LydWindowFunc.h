#pragma once

#include "LydBase.h"
#include <array>
#include <algorithm>

namespace LydD {
namespace Windower {

    //in & out must be same count, window must be at least count
    template <typename T = float>
    void applyWindowtoBlock(T* in, T* out, float* window, size_t count) {
        int i = 0;
        for (T a : out) {
            a = in[i] * window[i];
            ++i;
        }
    }

    enum class Window_Types {
        HANN,
        CURVED_HANN,
        SIGMOID,
        NUM_WINDOWS
    };
    //turns array index into phase from 0-1 
    template<typename T = float>
    T index_to_phase(int idx, int length) {
        return T(idx) / T(length - 1);
    }
    template<typename T = float>
    T Hann_Window(T time, T* window = nullptr) {
        T shape = 0.5f * (1.f - rack::simd::cos(_2_PI * time));
        if (window) *window *= shape;
        return shape;
    }
    template<typename T = float>
    T Curved_Hann_Window(T time, T cur, T* window = nullptr) {
        T ret = Hann_Window(time);
        T shape = normalCurve(0.f, 1.f, ret, cur);
        if (window) *window *= shape;
        return shape;
    }
    //sigmoid goes between -1 & 1 from -5 - +5 input
    //must run that and then run back in one window
    //euler approx doesnt like negatives so extra wrapping must also happen
    //and then reflection for the upper halves
    //smoosh and reflect from -5 <-> 0 and flip where it should be positive
    template<typename T = float>
    T Sigmoid_Window(T x, T cur, T* window = nullptr) {
        float xprep = x * 20.f - 5.f;
        bool ishi = xprep > 5.f;
        float xflip = ishi ? 5.f - (xprep - 5.f) : xprep;
        bool ispos = xflip > 0.f;
        float xfit = ispos ? -xflip : xflip;
        float sig = 1.f / (1.f + EulerToPower(-xfit));
        float sigfit = ispos ? 0.5f + (0.5f - sig) : sig;
        float shape = normalCurve(-1.f, 1.f, sigfit, cur);
        if (window) *window *= shape;
        return shape;
    }

    //storage and generator for window functions to multiply in (window is range 0 - 1)
    //will expand to more window types
    template<typename T = float, size_t S = 2048>
    struct WindowArray {
        static const int SI = S;
        std::array<float, S> Window;
        std::array<T, S> Output;
        void clearOutput() {
            for (int i = 0; i < int(S); ++i) {
                Output[i] = T(0.f);
            }
        }
        void clearWindow() {
            std::memset(&Window[0], 0, sizeof(float) * S);
        }
        void clear() {
            clearWindow();
            clearOutput();
        }

        void generateWindow(Window_Types wind, float curve = 0.f) {
            switch (wind) {
            default: {}
            case Window_Types::NUM_WINDOWS: {}
            case Window_Types::HANN: {
                for (int h = 0; h < SI; ++h) {
                    float ph = index_to_phase(h, SI);
                    this->Window[h] = Hann_Window(ph);
                }
                break;
            }
            case Window_Types::CURVED_HANN: {
                for (int h = 0; h < SI; ++h) {
                    float ph = index_to_phase(h, SI);
                    this->Window[h] = Curved_Hann_Window(ph, curve);
                }
                break;
            }
            case Window_Types::SIGMOID: {
                for (int h = 0; h < SI; ++h) {
                    float ph = index_to_phase(h, SI);
                    this->Window[h] = Sigmoid_Window(ph, curve);
                }
                break;
            }
            }

        }

        WindowArray() {
            clear();
        }



        float getWindowInd(int ind) {
            return this->Window[ind];
        }
        float* getWindow() {
            return &this->Window[0];
        }

        T* windowBlock(T* in) {
            applyWindowtoBlock(in, &this->Output[0], &this->Window[0], S);
            return &this->Output[0];
        }
    };

}
}

