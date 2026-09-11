#pragma once

#include "LydBase.h"
#include "LydBuffers.h"
#include "LydWindowFunc.h"
#include <atomic>
#include <array>
#include <algorithm>
using namespace LydD::Buffers;
using namespace LydD::Windower;
namespace LydD {
namespace Delay {
    

    
    //buffer types set at compile time
    template <typename T = float, size_t S = 44100, size_t D = 2048, int N_CHAN = 2, 
                            typename IN_TYPE = IndexDoubleRing<FrameStereo<T, N_CHAN>, S>, 
                            typename OUT_TYPE = IndexDoubleRing<FrameStereo<T, N_CHAN>, D> >
    struct CF_DelayLine {
        //ints for easy iterating
        static const int SI = S;
        static const int DI = D;
        static const int SS = S * 2;
        static const int DD = D * 2;
        const int dd = D / 2;

        //buffers
        IN_TYPE inBuf;
        OUT_TYPE outBuf[2];
        //window -- applied to output buffers
        WindowArray<FrameStereo<T>, D> _blockWindow;

        T fractionaldelay[N_CHAN];
        size_t delayinSamples[N_CHAN];
        size_t freezeLoop;

        //update time slower than even the buffer switches, smoother
        std::atomic<bool> buffer_switch;
        rack::dsp::BooleanTrigger _switchTick;
        const int max_ticks = 8;
        int tick_count = 0;
        bool updateTime = false;


        T Wet[N_CHAN];

        void clear() {
            for (int b = 0; b < N_CHAN; ++b) {
                Wet[b] = 0;
            }

            this->inBuf.clear();
            this->outBuf[0].clear();
            this->outBuf[1].clear();
            this->buffer_switch.store(false);
            this->_blockWindow.clearOutput();
        }
        /*void buf_delete() {
            if (this->inBuf != nullptr) delete this->inBuf;
            if (this->outBuf[1] != nullptr) delete this->outBuf[1];
            if (this->outBuf[0] != nullptr) delete this->outBuf[0];
        }*/
        CF_DelayLine() {
            //buf_delete();
            //this->inBuf = std::make_unique<IN_TYPE>();
            //this->outBuf[0] = std::make_unique<OUT_TYPE>();
            //this->outBuf[1] = std::make_unique<OUT_TYPE>();
            this->_blockWindow.generateWindow(Window_Types::HANN);
            this->clear();
        }
        ~CF_DelayLine() {
            //buf_delete();
        }
        //******Wrappers for the buffers internal functions*******//
        //returns pointer to data from index (ind) behind current index w/ optional return of index value to given pointer ni
        // to be copied onto output buffer 
        virtual FrameStereo<T, N_CHAN>* read_input_data(size_t ind, size_t* ni = nullptr) {
            return this->inBuf.ReadBlockFromIndex(ind, ni);

        }
        //use this with n of samples used (to keep buffer unFrozen - must be timed/sized right for good sound)
        virtual void read_incr_input(size_t n = 1) {
            this->inBuf.ReadBlockIncr(n);
        }

        //returns pointer to the currently writable buffer to copy data onto at Writehead -- DST/consumer/output
        virtual FrameStereo<T, N_CHAN>* write_output_data() {
            return this->outBuf[buffer_switch].WriteToBlock();

        }

        //must call this after write_output_data has been used and before any other functions of this buffer if you want things to work
        virtual void write_incr_output(size_t n) {
            this->outBuf[buffer_switch].WriteBlockIncr(n);

        }

        //to pull individual channel lanes from frame blocks pulled from different times
        //each result frame is each successive channel from each succesive block
        // e.g. 2 channel frame gets 2 framegroups, at one index
        //    output channel 0 pulls from framegroup[0] channel[0]
        //    output channel 1 pulls from framegroup[1] channel[1]
        virtual void collate_frames(FrameStereo<T, N_CHAN>** framegroup, FrameStereo<T, N_CHAN>* output, int blksize) {
            std::array<T, N_CHAN> collate[blksize];
            //for each channel, run the pointed array pulling out each lane at a time
            //i think this is better cache addressing nested loop than the other way around
            for (int c = 0; c < N_CHAN; ++c) {
                for (int b = 0; b < blksize; ++b) {
                    collate[b][c] = framegroup[c][b][c];
                }
            }
            //apply Windowing at this stage
            for (int d = 0; d < blksize; ++d) {
                output[d] = FrameStereo<T, N_CHAN>(collate[d])* this->_blockWindow.getWindowInd(d);
            }
        }
        //checks if half a buffer has passed, and switches writing to the other. 
        // windowing in collate makes this process silent
        virtual void switch_buffers() {
            float rload = this->outBuf[buffer_switch]._Rhead.load() % D;
            bool sw = rload >= dd;
            bool tick = this->_switchTick.process(sw);
            if (tick) {
                this->buffer_switch.store(!this->buffer_switch);
                this->outBuf[buffer_switch].force_empty();
                if (this->tick_count == 0) {
                    for (int d = 0; d < N_CHAN; ++d) {
                        this->delayinSamples[d] = this->fractionaldelay[d];
                    }
                }
                this->tick_count = (this->tick_count + 1) % max_ticks;
            }
        }
        //pass a block from the input buffer into current output Buffer
        virtual void pass_block() {
            FrameStereo<T, N_CHAN>* read_input[N_CHAN];
            for (int i = 0; i < N_CHAN; ++i) {
                read_input[i] = this->read_input_data(this->delayinSamples[i]);
            }
            FrameStereo<T, N_CHAN>* write_output = this->write_output_data();
            this->collate_frames(read_input, write_output, DI);

            this->write_incr_output(D);

            this->read_incr_input(this->dd);


        }
        //shift one frame from a given output buffer(meant to be overlap-added)
        virtual FrameStereo<T, N_CHAN> shift_output(int which) {
            //reading from 0 delay here
            FrameStereo<T, N_CHAN> t = this->outBuf[which].ReadFromIndexF(0);
            return t;

        }

        //*****MAIN FUNCTIONS FOR EXTERNAL USE*****//

        //delaytime given in fractional samples
        //samples must be array of N_CHAN size
        virtual void setDelay(T* samples) {
            for (int n = 0; n < N_CHAN; ++n) {
                this->fractionaldelay[n] = rack::math::clamp(samples[n], float(DI), float(SI));
            }
        }

        //t is array of one sample per channel, size N_CHAN
        virtual void PushInput(T* t) {
            FrameStereo<T, N_CHAN> frame = FrameStereo<T, N_CHAN>(t);
            inBuf.Write(frame);
        }

        //call after pushInput
        virtual void BlockProcess() {
            this->switch_buffers();

            if (this->outBuf[this->buffer_switch].is_empty()) {
                this->pass_block();
            }
        }
        //call after blockProcess
        //returns pointer to internal Wet array of N_CHAN size
        virtual T* CrossfadeOutput() {
            FrameStereo<T, N_CHAN> fade;
            //overlap add output buffers
            for (int w = 0; w < 2; ++w) {
                if (!this->outBuf[this->buffer_switch].is_empty()) {
                    fade += this->shift_output(w);
                }
            }
            for (int i = 0; i < N_CHAN; ++i) {
                this->Wet[i] = fade[i];
            }

            return this->Wet;
        }
    };



    //used to be essentially DoubleRingBuffer from VCV SDK but I required gut level changes.
    //manually indexable doubleRingBuffer with crossfading output buffers(no pitch shift on time changes)
    // freezable and reversible, also pitchable
    //inherently stereo delay, N_CHAN should be at least 2 
    // --uses FrameStereo internally, probably dont make this do Frames of Frames, but simd safe(i Think)
    // S determines size of input buffer, capping Maximum Delay Time in samples
    // D determines size of output buffers, capping  Minimum Delay Time in samples
    // 'S' AND 'D' SHOULD BE A POWER OF 2 (MUST BE IF USING SIMD), 'S' MUST BE AT LEAST TWICE 'D'
    //---- general call order ---- 
    // pushInput -> read_input_data & write_output_data -> collate_frames -> write_incr_output -> read_incr_input -> shift_output
    // but you have to do all the chacking yourself, so instead ::
    // PushInput -> BlockProcess -> CrossfadeOutput
    template <typename T = float, size_t S = 44100, size_t D = 2048, int N_CHAN = 2>
    struct Tricked_Out_CFDelayLine : CF_DelayLine<T, S, D, N_CHAN, 
                        ReverseFreezeDoubleRing<FrameStereo<T, N_CHAN>, S>,
                        Runaway_ReversePitchDoubleRing<FrameStereo<T, N_CHAN>, D>> {
        
        //pitch can only be global, indexes output buffers
        const int pitch_gap = S / 8;
        float Pitch;
        float pitchIncr[2];
        //states and checker
        bool StateChange = false;
        //for debug
        bool emptycatch = false;
        size_t freezeLoop;

        rack::dsp::BooleanTrigger _pitchTick;

       
        


        Tricked_Out_CFDelayLine() {
        }
        //delete handled by base class
        ~Tricked_Out_CFDelayLine() {
        }

        //flip the same boolean whichever state changes  
        void detect_state(bool change, bool curr) {
            //if recent change  snap readhead to writehead (before, it should alredy be pretty close)
            if (curr != change) {
                //thisll help it last even longer 
                // - whenever a state change occurs both head get wrapped back inside the buffer
                this->inBuf.force_inside();
                this->inBuf.force_empty();
                //this will hold true thru one sample the moment 'change' changes
                this->StateChange = true;
                //*curr = change;
                return;
            }
            this->StateChange = false;
            return;
        }

        //if pitch becomes zero and stays there, this can tell you when to put the readhead back
        bool pitch_at_zero() {
            bool iszero = rack::math::isNear(this->outBuf[0].Pitch, 1.f);
            bool stable = this->outBuf[0].is_stable_pitch();
            bool moment = _pitchTick.process(iszero && stable);
            return moment;
        }


        //checks if half a buffer has passed, and switches writing to the other. windowing makes thies process silent
        void switch_buffers() override {
            float rload = this->outBuf[this->buffer_switch]._Rhead.load() % D;
            bool sw = !this->outBuf[this->buffer_switch].Reverse ? rload >= this->dd
                : rload < this->dd;
            bool tick = this->_switchTick.process(sw);
            if (tick || StateChange) {
                this->buffer_switch.store(!this->buffer_switch);
                this->outBuf[this->buffer_switch].force_empty();

                if (this->tick_count == 0) {
                    for (int d = 0; d < N_CHAN; ++d) {
                        this->delayinSamples[d] = this->fractionaldelay[d];
                    }
                    //This is My Fix for Runaway ReadHead at unequal pitch
                    // reverse and freeze loop on their own so dont do it then
                    if (!this->inBuf.Reverse && !this->inBuf.Freeze) {
                        //check how much pitch has shifted read head
                        int wh = this->inBuf._Whead.load() % this->inBuf.SI;
                        int rh = this->inBuf._Rhead.load() % this->inBuf.SI;
                        int dist = get_wrapped_distance(wh, rh, this->inBuf.SI);
                        if (dist >= pitch_gap || this->pitch_at_zero()) {
                            this->inBuf.force_empty();
                        }
                    }
                }
                this->tick_count = (this->tick_count + 1) % this->max_ticks;
            }
        }


        //*****MAIN PROVIDED CALLS*****

        //delaytime given in fractional samples
        //samples must be array of N_CHAN size
        /*void setDelay(T* samples) in base class*/

        //playback speed as v/oct
        void setPitch(T voct) {
            this->outBuf[0].setPitch(voct);
            this->outBuf[1].setPitch(voct);
        }
        //loopgap could be same as loopsize below, or not
        void setReverse(bool rev, float loopgap) {
            detect_state(rev, this->inBuf.Reverse);
            this->inBuf.setReverse(rev, loopgap);
            //pitched output reverses just run whole buffer
            this->outBuf[0].setReverse(rev);
            this->outBuf[1].setReverse(rev);
            
        }
        //u can give me a fractional loopsize but i dont care about it
        void setFreeze(bool froze, T loopsize) {
            detect_state(froze, this->inBuf.Freeze);
            this->inBuf.setFreeze(froze, loopsize);

        }

        //t is array of one sample per channel, size N_CHAN
        /*void PushInput(T* t) in base class*/
        
        //call after pushInput
        void BlockProcess() override {
            switch_buffers();         
            if (this->outBuf[this->buffer_switch].is_empty()) {
                this->pass_block();              
            }
        }

        //call after blockProcess
        //returns pointer to internal array of N_CHAN size
        /*T* CrossfadeOutput() in base class*/
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
                size_t i = wraparound(int(this->wh.load()), int(dist[t]), int(S), true);
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


    //real artifacty when moving, best for static delays
    //Mostly Junk TM
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
            size_t i = wraparound(this->inWrite.load(), ind, S, true);
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
            RA = wraparound(pitchHead.load(), readSplit, D, true);
            // RA = wraparound(RA, D);
            int lowrap = wraparound(outRead.load(), readSplit, D, true);
            int hiwrap = wraparound(outRead.load(), size_t(hsplit), D);
            // RA = circularwrap(lowrap, hiwrap, RA ,D);
             //phase is ratio between differences
            T ph1 = (T)RA / (T)D;// (T)((RA) % readSplit) / (T)readSplit;

            RB = pitchHead % D;// wraparound((int)pitchHead, (int)D);
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



}
}

