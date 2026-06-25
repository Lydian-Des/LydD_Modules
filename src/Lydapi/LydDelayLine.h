#pragma once

#include "LydBase.h"
#include <atomic>

namespace LydD {
namespace Delay {
   
    template <typename T = float>
    T fractionalRead(T delay, size_t rdpt, T* Buf, size_t bSize) {
        if (delay < 0) delay += bSize;
        int oi = (int)delay;
        float frac = delay - oi; //always portion from 0 - 1
        return cubicLerp(Buf, rdpt, frac, bSize);
        //return rack::math::crossfade(Buf[rdpt], Buf[(rdpt + 1) % bSize], frac);
    }

    //real artifacty when moving, best for static delays
    template <typename T = float, size_t S = 44100>
    struct SimplePitchDelayLine {
        static const size_t D = 2048;
        std::atomic<size_t> inWrite;
        T Buffer[S * 2];
        std::atomic<size_t> outWrite;
        std::atomic<size_t> outRead;
        std::atomic<size_t> pitchHead;
        size_t RA;
        size_t RB;
        T readPhase;
        size_t readSplit = D / 2; //window between alternating readheads
        T blok[D * 2];
        T Delay;
        int timeSet;
        T pitchremain;
        T Pitch;
        int samps;
        T next;
        int ni;
        void clear() {
            std::memset(this->Buffer, 0.f, sizeof(T) * (S * 2));
            std::memset(this->blok, 0.f, sizeof(T) * (8));
            RA = 0;
            this->inWrite = 0;
            this->outWrite = 0;
            this->outRead = 0;
            this->pitchHead = 0;
            this->RA = 0;
            this->RB = 0;
            this->Pitch = 0;
            this->pitchremain = 0;
            this->readPhase = 0;
        }
        SimplePitchDelayLine() {
            this->clear();
        }
     
        bool outEmpty() const {
            bool emp = this->outRead >= this->outWrite;
            return emp;
        }

        bool full() {
            return this->inWrite - this->outRead >= S;
        }
        //Delay expects # of samples
        // Pitch is set so 0v = x1 speed, 1v = x2 speed, -1v = x0.5 speed 
        void setPars(T d, T p) {
            this->Delay = rack::math::clamp(d, 1.f, S);
            this->Pitch = VoltToFreq(p, 0.f, 1.f);
        }
        //run every sample unless u want it crushed or whatever
        void push(T in) {
            size_t i = this->inWrite % S;
            Buffer[i] = in;
            Buffer[i + S] = in;
            this->inWrite++;
        }

        //not tracking a read pointer for the inBuffer so no incrementing happens here
        const T* ReadInput(size_t ind) {
            size_t i = wraparound(this->inWrite - ind, S);
            return &this->Buffer[i];
        }

        //after writing to the output block given by this (written to with AppendBlock),
        // must call outBufIncr w/ # of samps used
        T* WriteableOutput() {
            size_t i = this->outWrite % D;
            return &this->blok[i];
        }
        void outBufIncr(size_t n) {
            size_t i = (this->outWrite) % D; //get wrapped writehead
            size_t e1 = i + n; //add number of desired samples to writehead count
            size_t e2 = (e1 < (D)) ? e1 : (D); // if samples (e1) will go past S, this will == S
            // Copy data forward
            std::memcpy(&this->blok[D + i], &this->blok[i], sizeof(T) * (e2 - i));

            if (e1 > D) {
                // Copy data backward from the doubled block to the main block
                std::memcpy(this->blok, &this->blok[D], sizeof(T) * (e1 - D));
            }
            this->outWrite += n;
        }

        //call order ReadInput -> WriteableOutput -> AppendBlock -> outBufIncr
        void AppendBlock(const T* input, T* output, size_t blksize = D) {
            std::memcpy(output, input, sizeof(T) * blksize);
        }
        T circularwrap(float min, float max, T v, size_t lim) {
                float newmax = (min < max) ? max : max + lim;
                float m = min + rack::simd::fmod((v - min), (newmax - min));
                return  (T)rack::simd::fmod(m, lim);
            
        }
        //pull repitched samples from the output Block
        T shift() {
            T t = 0;
            //E.G. x2 speed
            //at first sample pitchHead and outRead are both 0
            //RA and RB are a readsplit and half a readsplit in the past from pitchHead(and outread for now)
            // on  second sample RA and RB are same distance from pitchHead, but slightly closer to outread
            // samples go by
            // when outRead has traveled half the block, pitchHead(and RA & RB) will have gone the whole length
            //INSTEAD
            // the windows should wrap a readsplit size around outread, to follow it more directly
            // RA follows pitchHead, and should start the window with a gain of 1
            // RB starts 1/2 readsplit back, and should start with gain of 0
            //when outread has traveled the whole block, 

            //# of samples expected to increment based on pitch(x2 speed wants to increment 2 samples)
            //must include offset of previous movement for int compensation
            //if (outRead % D == 0) pitchHead = 0;
            next = this->Pitch + this->pitchremain;
            ni = static_cast<int>(next);
            int hsplit = readSplit / 2;
            RA = wraparound(pitchHead - readSplit, D) ;
           // RA = wraparound(RA, D);
            int lowrap = wraparound((outRead - readSplit), D);
            int hiwrap = wraparound((outRead + hsplit), D);
           // RA = circularwrap(lowrap, hiwrap, RA ,D);
            //phase is ratio between differences
            T ph1 = (T)RA / (T)D;// (T)((RA) % readSplit) / (T)readSplit;
           
            RB = wraparound((int)pitchHead, (int)D);
           // RB = wraparound(RB, D);
           // RB = circularwrap(lowrap, hiwrap, RB, D);
            T ph2 = (T)RB / (T)D;// (T)((RB - hsplit) % readSplit) / (T)readSplit;
           
            //read from both heads
            T t1 = fractionalRead(pitchremain, RA, this->blok, D);
            T t2 = fractionalRead(pitchremain, RB, this->blok, D);
            //make phase for each read pointer
            //since RA starts at 0, offset is needed to give it the appropriate gain
            
           
            //window each fractional read
            //may make more for me less for the CPU by creating a precalculatable Hann Array 
            HannWindow(ph1, &t1);
            HannWindow(ph2, &t2);
            //increase pitchreadhead by deduced integer amount
            pitchHead += ni;
            //remember difference
            this->pitchremain = next - ni;
            
            //this still moves at a constant speed so the buffer empties in constant time
            this->outRead += 1;
            t = t1 + t2;// +t2;
            return t;
        }

        //can call every sample, will only run when it needs to
        void process() {
                             
            // update delay time only when buffer empties
            //
            if (this->outEmpty()) {
                samps = static_cast<int>(this->Delay);
                this->timeSet = samps;
                const T* read_input = this->ReadInput(samps);
                T* write_output = this->WriteableOutput();
                //just fill the whole output Block each time
                this->AppendBlock(read_input, write_output, D);
                this->outBufIncr(D);
            }
        }
        //call this each sample for output
        T pullTap() {
            T tap = 0;
            //T window = (float)(this->outRead % this->timeSet) / (float)timeSet;
            if (!this->outEmpty()) {
                tap = this->shift();
              //  HannWindow(window, &tap);
            }
                return tap;
        }
    };


    //used to be essentially DoubleRingBuffer from VCV SDK but I required gut level changes.
    //manually indexable doubleRingBuffer with crossfading output buffers(no pitch shift on time changes)
    // freezable and reversible, also pitchable
    template <typename T = float, size_t S = 44100, size_t D = 2048>
    struct CFDelayLine {
        std::atomic<size_t> inRead;
        std::atomic<size_t> inWrite;
        std::atomic<size_t> outRead[2];
        std::atomic<size_t> outWrite[2];
        std::atomic<size_t> wblkhd;
        T inBuf[S * 2];
        T outBuf[2][D * 2];
        T wetblok[8];
        T Pitch;
        T pitchIncr[2];
        rack::dsp::Timer _crossTime;
        rack::dsp::SlewLimiter _crossFade;

        bool bufferSwitch = false;
        bool shrink = false;
        bool Reverse = false;
        bool Frozen = false;
        bool revtmp = false;
        bool fretmp = false;
        bool StateChange = false;

        T fractionaldelay;
        size_t delayinSamples = D * 1.5;        
        size_t shrinksize;
        size_t lastsize = 0; 
        T fadetime;
 

        void clear() {
            std::memset(this->inBuf, 0.f, sizeof(T) * (S * 2));
            this->inRead = 0;
            this->inWrite = 0;
            this->wblkhd = 0;
            this->Pitch = 1;
            for (int b = 0; b < 2; ++b) {
                std::memset(this->outBuf[b], 0.f, sizeof(T) * (D * 2));
                this->outRead[b] = 0;
                this->outWrite[b] = 0; 
                this->pitchIncr[b] = 0;
            }
            _crossTime.reset();
        }


        CFDelayLine() {
            this->clear();
        }
        //most of these little ones are straight from DoubleRingBuffer, and not even used
        bool inEmpty() const {
            return this->inRead >= this->inWrite;
        }
        bool outEmpty(int which) const {
            bool emp = this->outRead[which] >= this->outWrite[which];
            return emp;
        }
        bool inFull() const {
            return this->inWrite - this->inRead >= S;
        }
        bool outFull() const {
            return this->outWrite[0] - this->outRead[0] >= D;
        }
        size_t inSize() const {
            return this->inWrite - this->inRead;
        }
        size_t outSize() const {
            return this->outWrite[0] - this->outRead[0];
        }
        size_t inCapacity() const {
            return S - this->inSize();
        }
        size_t outCapacity() const {
            return D - this->outSize();
        }
        void forceEmpty(int which) {
            //set writehead = readhead to force buffer to write at current point in time and overwrite the unused data
            this->outWrite[which].store(this->outRead[which].load());
        }

        void detectState(bool change, bool* tmp) {
            //if recent change  snap readhead to writehead (before, it should alredy be pretty close)
            //if (change == true) *tmp = true;
            if (*tmp != change) {
                this->inRead.store(this->inWrite.load());
                //this will hold true thru one sample the moment 'change' changes
                this->StateChange = true;
                *tmp = change;
                return;
            }
            this->StateChange = false;
            return;
        }

        //put a sample into the end of input and advance input Write head every frame
        //run every sample
        void pushInput(T t, bool frozen = false, bool reverse = false) {
            //flip the same boolean whichever state changes
            detectState(frozen, &this->Frozen);
            detectState(reverse, &this->Reverse);

            size_t i = this->inWrite % S;
            if (!this->Frozen) {
                this->inBuf[i] = t;
                this->inBuf[i + S] = t;    
                this->inWrite++;
                //if (!reverse) this->inRead++;//keep readhead at now for delaytime 
        
            }


        }
        
        //returns pointer to data from index (ind) behind current index w/ optional return of index value to given pointer ni
       // to be copied onto output buffer 
        const T* ReadDataFromInput(size_t ind, size_t* ni = nullptr) {
            size_t i;
            if (this->Frozen || this->Reverse) i = wraparound(int(this->inRead - ind), int(S));
            else i = wraparound(int(this->inWrite - ind), int(S)); //when not frozen or reversing, we use the writehead
            if (ni) *ni = i;
            return &this->inBuf[i];
        }
        //use this with n of samples used (to keep buffer unFrozen - must be timed/sized right for good sound)
        void inReadIncr(size_t n = 1) {
            //this reverse will just reverse the entire input buffer
            if (!this->Reverse) {
                this->inRead += n;
            }
            else {              
                this->inRead = wraparound(int(this->inRead - n), int(S));
            }
        }
        //n = #Of samps to incr inBuffer-> 1/2 incr to outbuffer[x]. 
        // cs = crawl size->loop size of inBuffer. 
        // pt = dist from frozen inWrite -> delaytime
        void inReadIncrFroze(size_t n, size_t cs, size_t pt) {
            //looping inRead directly helps here
            this->inRead = this->inRead % (S * 2);
            //if this is being called, inWrite isnt moving
//calculate wraps if requested indexes loop the ring
            size_t sf = this->inWrite % S;
            //start of frozen slice
            size_t ef = wraparound(int(sf - pt), int(S));
            //end of frozen slice --- this may read fron the doubled section of the buffer
            size_t cf = ef + cs;
            //size_t ps = this->inRead + n; //projected next read point - unneeded atm
            if (!this->Reverse) {
                this->inRead += n;
                //if outside desired location skip around til inside
                if (this->inRead > cf) this->inRead = ef;
                if (this->inRead < ef) this->inRead = cf;
            }
            else {
                this->inRead -= n;
                if (this->inRead <= ef) this->inRead = cf;
                if (this->inRead >= cf) this->inRead = ef;
                //this->inRead = wraparound(this->inRead, S);
            }
        }

        //returns pointer to this buffer to copy data onto at Writehead -- DST/consumer/output
        T* WriteDataToOutput() {
            size_t i = this->outWrite[this->bufferSwitch] % D;
            return &this->outBuf[this->bufferSwitch][i];
        }

        //must call this after WriteData and before any other functions of this buffer if you want things to work
        void outWriteIncr(size_t n) {
            size_t i = (this->outWrite[this->bufferSwitch] ) % D; //get wrapped writehead
            size_t e1 = i + n; //add number of desired samples to writehead count
            size_t e2 = (e1 < (D)) ? e1 : (D); // if samples (e1) will go past S, this will == S
            // Copy data forward
            std::memcpy(&this->outBuf[this->bufferSwitch][D + i], &this->outBuf[this->bufferSwitch][i], sizeof(T) * (e2 - i));

            if (e1 > D) {
                // Copy data backward from the doubled block to the main block
                std::memcpy(this->outBuf[this->bufferSwitch], &this->outBuf[this->bufferSwitch][D], sizeof(T) * (e1 - D));
            }
            this->outWrite[this->bufferSwitch] += n;
        }
        //call order ReadDataFromInput -> WriteDataToOutput -> AppendBlock -> outWriteIncr -> inReadIncr
        void AppendBlock(const T* input, T* output, size_t blksize = D) {
            std::memcpy(output, input, sizeof(T) * blksize);
        }

        //read an element every frame(if you can) and index outs ReadHead
        //made overrideable for grainmaker
        virtual T shiftOutput(int which) {
            T t = 0;
            size_t i = this->outRead[which] % D;
            T next = this->Pitch + this->pitchIncr[which];
            t = fractionalRead(pitchIncr[which], i, this->outBuf[which], D);
            int ni = static_cast<int>(next);
            this->pitchIncr[which] = next - ni;

            if (!this->Reverse) {
                this->outRead[which] += ni;
            }
            else {
                this->outRead[which] = wraparound(int(this->outRead[which] - ni), int(D));
            }
            return t;
        }

        //delaytime given in samples but as T for fractional component
        //call after pushInput
        //this wraps up much of the above neatly for you
        void blockProcess(T delaytime, float sampletime, T pitch = 0, T crawlF = 0) {
            this->_crossTime.process(sampletime);
            this->shrink = this->delayinSamples < D;
            this->shrinksize = this->shrink ? this->delayinSamples : D;
            //change delay time on intervals and switch which delay to give it to
            //get delaytime - write to silent buffer - crossfade from audible buffer - switch - repeat
            //separates delay threads by half D (or smaller if delay size shorter than D)
         
            //samplephase should equal when half of shrinksize samples have passed
            size_t samplephase = this->_crossTime.getTime() / (sampletime); //convert time to samples
            this->fractionaldelay = rack::math::clamp(delaytime, 10.f, S);
            //if enough time has elapsed 
            // update delay time and give it to silent buffer, initiating crossfade
            if (samplephase >= (this->shrinksize * 0.5) || StateChange) {
                this->Pitch = VoltToFreq(pitch, 0.f, 1.f); //0v = 1x speed, 1v = 2x speed, -1v = 0.5x speed

                this->delayinSamples = (int)this->fractionaldelay;
                this->bufferSwitch = !this->bufferSwitch; //flip from 0 - 1 and back
                forceEmpty(this->bufferSwitch);
                this->lastsize = this->shrinksize;
                _crossTime.reset();
                float risefall = (samplephase / 12.f); //slew time for crossfading
                _crossFade.setRiseFall(risefall, risefall);

            }
            //somehow delay times smaller than D (shrinksize != D) result in pop at moment of buffer switch,
            // short times result in one buffer being held empty while the other reads instead of writing every timing cycle
            if (this->outEmpty(this->bufferSwitch)) {
                //these internally read from bufferSwitch buffer
                const T* read_input = this->ReadDataFromInput(this->delayinSamples);
                T* write_output = this->WriteDataToOutput();
                this->AppendBlock(read_input, write_output, this->shrinksize);
                this->outWriteIncr(this->shrinksize);
                
                if (!this->Frozen) {
                    this->inReadIncr(this->shrinksize / 2); //increment by half, as 2 delay threads separated by half
                }
                else {
                    this->inReadIncrFroze(this->shrinksize / 2, (size_t)crawlF, this->delayinSamples);
                }
            }
        }
        //run every sample -- call after blockProcess();
        T CrossfadeOutput(float sampletime) {

            T wet = 0;
            T fade[2] = { 0, 0 };          
            fadetime = this->_crossFade.process(sampletime, this->bufferSwitch);
            //process both buffers 
            for (int w = 0; w < 2; ++w) {
                if (!this->outEmpty(w)) {
                    fade[w] = this->shiftOutput(w);
                }
            }
            //fade between buffers at bufferSwitch -> into tiny buffer for cubic lerping. 
            // this does mean output is always at minimum 2 samples delay 
            // this is stil not enough to fully remove clicking
            wetblok[wblkhd % 8] = rack::math::crossfade(fade[0], fade[1], fadetime);
            wet = cubicLerp(wetblok, (wblkhd - 2) % 8, fadetime, 8);
            ++wblkhd;
            return wet;
        }

        //mostly for debugging
        size_t getCurrentEnd() {
            return this->inWrite % S;
        }
        size_t getCurrentStart() {
            return this->inRead % S;
        }

    };

    template <typename T = float, size_t S = 44100, size_t TAPS = 4>
    struct SimpleTapLine {
        T Buffer[S];
        T taps[TAPS];
        std::atomic<size_t> wh;
        SimpleTapLine() {
            wh = 0;
            std::memset(Buffer, (T)0, sizeof(T) * S);
            std::memset(taps, (T)0, sizeof(T) * TAPS);
        }
        void Push(T in) {
            this->Buffer[wh % S] = in;
            ++wh;
        }
        //places taps in lanes(ret) with independant delays(dist) - ret and dist must be at least TAPS size
        void PullTaps(T* dist, T* ret) {

            for (int t = 0; t < TAPS; ++t) {
                size_t i = wraparound((this->wh - (int)dist[t]), S);
                this->taps[t] = fractionalRead(dist[t], i, this->Buffer, S);
                ret[t] = this->taps[t]; 
            }
        }
        //dist must be at least TAPS size
        T SumTaps(T* dist) {
            T tap[TAPS];
            T ret = 0;
            PullTaps(dist, tap);
            for (int t = 0; t < TAPS; ++t) {
                ret += tap[t];
            }
            return ret;
        }

        void clear() {
            std::memset(Buffer, (T)0, sizeof(T) * S);
            std::memset(taps, (T)0, sizeof(T) * TAPS);
            wh = 0;
        }
    };
    //fixed sample delay, D is delay time in samples
    template<typename T = float, size_t D = 100>
    struct FixedDelayLine {
        T Buf[D];
        std::atomic<size_t> wh;
        FixedDelayLine() {
            std::memset(Buf, 0, sizeof(T) * D);
            wh = 0;
        }
        T process(T in) {
            //this simply makes output follow all the way behind input via wrapping
            size_t del = (wh + (D - 1)) % D;
            Buf[del] = in;
            T out = Buf[wh % D];
            ++wh;
            return out;
        }
    };

}
}

