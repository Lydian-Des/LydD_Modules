#pragma once

#include "LydBase.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <complex>

namespace LydD {
    template<typename T = float>
    T wrapWindow(T value, T min, T max) {
        T range = max - min;
        return min + rack::simd::fmod(rack::simd::fmod(value - min, range) + range, range);
    }

namespace FFTStuff {
    template <typename T = float >
    void unifHann(T time, T* ret, T* window = nullptr) {
        T shape = 0.5f * (1.f - rack::simd::cos(_2_PI * time));
        *ret *= shape;
        if (window) *window = shape;
    }
    template <typename T = float >
    void curveHann(T time, T* ret, T cur, T* window = nullptr) {
        T shape = 0.5f * (1.f - pow(rack::simd::cos(_2_PI * time), cur));
        *ret *= shape;
        if (window) *window = shape;
    }

    

    //given and return Bins must be of length 'size * 2 + 1'
    //FFT bins are interlaced real and imaginary values, Real first
   /* template<typename T = float>
    using FFTFunction = void(*)(T* givenBins, T* returnBins, int size);
    template<typename T = float>
    void doNothing(T* givenBins, T* returnBins, int size) {
        for (int i = 0; i < size; ++i) {
            int ir = i * 2;
            int ii = i * 2 + 1;
            returnBins[ir] = givenBins[ir];
            returnBins[ii] = givenBins[ii];
        }
    }
    template<typename T = float>
    void windowFilter(T* givenBins, T* returnBins, int size) {
        //how far to shift filter window
        int startDex = Scramble * (R * 0.75f);
        for (int i = 0; i < R; ++i) {
            int ir = i * 2;
            int ii = i * 2 + 1;
            T window = filtHann[wraparound(startDex - i, R)];
            outbins[ir] = inbins[ir] * window;
            outbins[ii] = inbins[ii] * window;
            //distribute to conjugates
            if (i > 0 && i < R - 1) {

                outbins[I - ii] = outbins[ir];
                outbins[I - ir] = -outbins[ii];
                //binPhase[S - a] = -binPhase[a];
                //altPhase[S - a] = -altPhase[a];
            }
        }

    }
    template<typename T = float>
    void binScramble(T* givenBins, T* returnBins, int size) {
        for (int i = 0; i < size; ++i) {
            int ir = i * 2;
            int ii = i * 2 + 1;
            int scram = (rack::math::clamp(scramTo[i] * Scramble + i, 0.f, (float)S));
            int rr = scram * 2;
            int ri = scram * 2 + 1;
            returnBins[rr] = givenBins[ir];
            returnBins[ri] = givenBins[ii];
        }
    }*/


    //package for using racks FFT, retains all prerequisites
    template<typename T = float, int S = 2048>
    struct BaseFFT {
        rack::dsp::ComplexFFT _fft;

        BaseFFT() : _fft(S) {}
        //complexFFT requires input and output arrays for all to be 2x the FFT size 'S',
        // interleaved as real and imaginary parts
        ~BaseFFT() {
            
        }

        //takes array 'in' and returns fft bins to 'bins'
        void takeFFT(T* in, T* bins) {
            this->_fft.fft(in, bins);            
        }
        //scales the frequency bins with racks scaling
        void takeScaledFFT(T* in, T* bins) {
            this->_fft.fft(in, bins);
            this->_fft.scale(bins);
        }
        //takes inverse FFT of 'bins' and returns audio block to 'out'
        void invertFFT(T* bins, T* out) {
            this->_fft.ifft(bins, out);
        }
        void invertScaleFFT(T* bins, T* out) {
            this->_fft.ifft(bins, out);
            this->_fft.scale(out);
        }
    };
    //takes in FFT bins and shifts their freguency content with pitch relationships
    //H is S / number of Hops, for phase difference calculations
    template<typename T, int S = 2048, int H = 512> 
    struct BinPitcher {
        static const int R = S / 2 + 1;
        static const int I = S * 2;
        T Pitch;
        T binPhase[S];
        T binMag[R];
        T binFreq[R];
        T altPhase[S];
        T altMag[R];
        T altFreq[R];

        BinPitcher() {
            std::memset(binPhase, 0, sizeof(T) * S);
            std::memset(binMag, 0, sizeof(T) * R);
            std::memset(binFreq, 0, sizeof(T) * R);
            std::memset(altPhase, 0, sizeof(T) * S);
            std::memset(altMag, 0, sizeof(T) * R);
            std::memset(altFreq, 0, sizeof(T) * R);
        }
        void setPitch(T p) {
            this->Pitch = p;
        }
        void process(T* inbins, T* outbins) {
            //remove potential unwanted data that might not otherwise be replaced
            for (int i = 0; i < R; ++i) {
                altMag[i] = altFreq[i] = 0;
            }
            //only run half the thing cuz complex conjugates
            for (int a = 0; a < R; ++a) {
                // real -- magnitude
                int a1 = a * 2 + 0;
                // imaginary -- phase
                int a2 = a * 2 + 1;
                //analysis section
                //'a' is a bin at this frequency
                float cfreq = _2_PI * a / (float)S;
                std::complex<T> inComplex(inbins[a1], inbins[a2]);
                binMag[a] = abs(inComplex); //analysis magnitude
                float binPha = std::arg(inComplex); //analysis phase
                float phdif = binPha - binPhase[a];//dif in phase from last hop
                float phremain = wrapWindow(phdif - (cfreq * (H)), -_PI, _PI);//remainder is phase dif - center bin freq
                binFreq[a] = (phremain * S / (float)(H) / _2_PI) + a; //analysis frequency as fractional bin     
                binPhase[a] = binPha; //save last hops phase    
                int newbin = rack::simd::floor(Pitch * a + 0.5f); //k` in bela
                //newbin = (rack::math::isEven(newbin)) ? newbin : newbin - 1;
                newbin = rack::math::clamp(newbin, 0, S);
                if (newbin <= R - 1) {
                    //synthesized magnitude of new bin is magnitude of analysis original bin
                    //bins have the potential to be moved to the same newbin, so should be added in.
                    altMag[newbin] += binMag[a];
                    altFreq[newbin] = binFreq[a] * this->Pitch;
                }
            }
            for (int o = 0; o < R; ++o) {
                //output section
                float newfreq = _2_PI * o / (float)S;
                // real -- magnitude
                int o1 = o * 2 + 0;
                // imaginary -- phase
                int o2 = o * 2 + 1;
                T sfreq = altFreq[o] - o; //new specific freq -- old fractional bin number * pitch ratio - this index
                T spdif = sfreq * _2_PI * (H) / (float)S;
                spdif += newfreq * (float)H; //synthesis phase remainder
                T spwrap = altPhase[o] + spdif;
                T synPhase = wrapWindow(spwrap, -_PI, _PI); //synthesis phase for bin 'newbin'
                std::complex<T> outComplex = std::polar(altMag[o], altPhase[o]);
                outbins[o1] = real(outComplex);
                outbins[o2] = imag(outComplex);
                altPhase[o] = synPhase;
                //distribute to conjugates
                if (o > 0 && o < R - 1) {
                    outbins[I - o2] = outbins[o1];
                    outbins[I - o1] = -outbins[o2];
                }
            }
        }
    };
   
    //FFT sizes(S) must be power of 2 && multiple of 16
    template<typename T = float, int S = 2048>
    struct FFTPitchShift : BaseFFT<T, S> {

        std::thread pitchWorker;
        std::mutex workTex;
        std::condition_variable workCV;
        std::atomic<bool> FFTReady{ false };
        std::atomic<bool> Quitting{ false };


        //S is FFT size, I is doubled for compex values(interlaced), H is hop size
        static const int R = S / 2 + 1;
        static const int I = S * 2;
        static const int HN = 4;
        static const int H = S / HN;

        BinPitcher<T, S, H> _pitchShift;

        std::atomic<size_t> inWH[HN];
        std::atomic<size_t> outRH[HN];
        T HANN[S];
        T filtHann[R];
        bool firstTimeIn[HN];
        std::atomic<bool> outReady[HN];
        int currentWindow = 0; //current window being FFT'd
        niceBlock<T, I>* inWindow;
        niceBlock<T, I> FFFreqs;
        niceBlock<T, I> alteredFreqs;
        niceBlock<T, I>* outWindow;
       
        //phase, magnitude, and frequency calculated for analysis and synthesis each hop
        
        int scramTo[S];

        T Scramble;
        T finalOuts[HN];

        FFTPitchShift()  {
            this->inWindow = new niceBlock<T, I>[HN];
            this->outWindow = new niceBlock<T, I>[HN];
            for (int i = 0; i < HN; ++i) {

                this->inWindow[i].Empty();
                this->outWindow[i].Empty();
                this->inWH[i] = 0;
                this->outRH[i] = 0;
                this->firstTimeIn[i] = true;
            }

            FFFreqs.Empty();
            alteredFreqs.Empty();



            for (int h = 0; h < S; ++h) {
                this->HANN[h] = (T)1;
                T ph = (T)h / (T)(S - 1);
                unifHann(ph, &HANN[h]);
                int rnd = rack::random::uniform() * (H / 4);
                scramTo[h] = rnd;
            }
            for (int f = 0; f < R / 4; ++f) {
                filtHann[f] = (T)1;
                T hanph = (float)f / (float)(R / 4 - 1);
                curveHann(hanph, &filtHann[f], 4.f);
            }
            for (int f = R / 4; f < R; ++f) {
                filtHann[f] =(T)0;
            }
            pitchWorker = std::thread(&FFTPitchShift::processFFT, this);
        }
        ~FFTPitchShift() {
            {
                std::lock_guard<std::mutex> lock(workTex);
                Quitting.store(true);
            }
            workCV.notify_one();
            if (pitchWorker.joinable()) pitchWorker.join();
            
            delete[] this->inWindow;
            delete[] this->outWindow;
        }
        void setPitch(T pitch) {
            this->_pitchShift.setPitch(VoltToFreq(pitch, 0.f, 1.f));
            this->Scramble = (pitch + 1.f) * 0.25f;
        }
        bool inFull(int w) {
            return this->inWH[w] % S >= S - 1;
        }
        bool inHalf(int w) {
            return this->inWH[w] % S == S / 2;
        }
        bool inEmpty(int w) {
            return this->inWH[w] % S == 0;
        }
        bool outFull(int w) {
            return this->outRH[w] % S >= S - 1;
        }
        bool outEmpty(int w) {
            return this->outRH[w] % S == 0;
        }

        void autoWindow(T* in, T* out) {
            for (int i = 0; i < S; ++i) {
                int ni = i * 2;
                int mi = i * 2 + 1;
                out[ni] = in[ni] * this->HANN[i];
                out[mi] = in[mi] * this->HANN[i];
            }
        }
        T getWritePhase(int w) {
            return this->inWH[w] % S / (T)S;
        }
        T getInWindow(int w) {
            return this->HANN[this->inWH[w] % S];
        }
        T getOutWindow(int w) {
            return this->HANN[this->outRH[w] % S];
        }
        void push(T in) {
            {
                std::lock_guard<std::mutex> lock(workTex);
                for (int q = 0; q < HN; ++q) {
                    //use writeheads in background as counters to tell when to start for the first time
                    //first sample, none are ready to write, so each ticks. 
                    // then the first window buffer will be greater than 0(H * 0), no longer first time, and write a sample
                    if (((int)this->inWH[q] < H * q) && this->firstTimeIn[q]) {
                        this->inWH[q] += 1;
                    }
                    //when its time to start, reset writehead to begin actually writing, make sure its not first time anymore
                    else if (((int)this->inWH[q] >= H * q) && this->firstTimeIn[q]) {
                        this->inWH[q] = 0; //reset counter to 'start' actual time
                        this->firstTimeIn[q] = false;
                    }
                    //at equilibrium, every window buffer will be pushing the same sample into different read positions
                    //overlapping at 1/HN * S -- one hop size
                    if (!this->firstTimeIn[q]) {
                        size_t whn = this->inWH[q] % S;
                        size_t whr = whn * 2;
                        size_t whi = whn * 2 + 1;
                        this->inWindow[q].dat[whr] = in * this->HANN[whn];
                        this->inWindow[q].dat[whi] = 0;
                        this->inWH[q] += 1;
                    }
                }
            }
            if (this->inFull(this->currentWindow)) {
                this->FFTReady.store(true);
                workCV.notify_one();
            }
        }

       

        

        void processFFT() {
            while (true) {
                std::unique_lock<std::mutex> lock(workTex);
                workCV.wait(lock, [this] {
                    return this->FFTReady.load() || this->Quitting.load();
                    });
                //make sure to close the thread when the program terminates
                if (this->Quitting) return;
                //if (this->inFull(this->currentWindow)) {
                    this->takeScaledFFT(this->inWindow[this->currentWindow].dat, this->FFFreqs.dat);

                    //remove potentially unwanted info from altered bins
                    for (int l = 0; l < (int)S; ++l) {
                        this->alteredFreqs.dat[l * 2] = 0;
                        this->alteredFreqs.dat[l * 2 + 1] = 0;
                    }
                    //}
                    //if(inHalf(f)) {
                    this->_pitchShift.process(this->FFFreqs.dat, this->alteredFreqs.dat);
                    //doNothing(this->FFFreqs.dat, this->alteredFreqs.dat);
                    //binScramble(this->FFFreqs.dat, this->alteredFreqs.dat);
                // windowFilter(this->alteredFreqs.dat, this->alteredFreqs.dat);


                    this->invertFFT(this->alteredFreqs.dat, this->outWindow[this->currentWindow].dat);
                    autoWindow(this->outWindow[this->currentWindow].dat, this->outWindow[this->currentWindow].dat);
                    //this is set true for the first time for the first outblock only after the first fft is done
                    this->outReady[this->currentWindow] = true;
                    this->currentWindow = (this->currentWindow + 1) % this->HN;
                    this->FFTReady.store(false);
                //}
                lock.unlock();
            }
        }

        T addArray(T* arr, int size) {
            T val = 0;
            for (int i = 0; i < size; ++i) {
                val += arr[i];
            }
            return val;
        }
        T pull() {

            for (int q = 0; q < HN; ++q) {
                //if any buffer has been FFT'd, reset its read pointer
                if (this->outReady[q]) {
                    this->outRH[q] = 0;
                    this->outReady[q] = false;
                }
                //grab sample from each hop block
                if (!this->outFull(q)) {
                    size_t rh = this->outRH[q] % S;
                    this->finalOuts[q] = this->outWindow[q].dat[rh * 2] * this->HANN[rh];
                   
                }
                this->outRH[q] += 1;
            }
            //add hop blocks together to output
            return (addArray(this->finalOuts, HN)) * (5.f / HN);
        }
    };



}
}

