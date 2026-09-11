#pragma once

#include "LydBase.h"
#include "LydWindowFunc.h"
#include <atomic>
#include <array>
#include <algorithm>
using namespace LydD::Windower;
namespace LydD {
namespace Buffers {
    //interpolates between 2 samples from rdpt by frac of a Buf given its total count bSize
    //even if frac is just the whole fractional index rdpt was derived from
    template <typename T = float>
    T fractionalRead(float frac, size_t rdpt, T* Buf, size_t bSize) {
        if (frac < 0) frac += bSize;
        int oi = (int)frac;
        float fr = frac - oi; //always portion from 0 - 1
        //return cubicLerp(Buf, rdpt, frac, bSize);
        return LydD::base_crossfade(Buf[rdpt], Buf[(rdpt + 1) % bSize], fr);
    }


    template<typename T = int>
    T get_wrapped_distance(T wh, T rh, T wrap) {
        T wrpd = rack::simd::fmod((wh - rh + wrap), wrap);
        //T wrd = rh > wh ? rh - wrap : rh;
        //T d = abs(wh - wrd);
        return wrpd;
    }
    //use tolerance to decide how much ahead is allowed before we decide its wrapped behind 
    template<typename T = int>
    T get_signed_distance(T wh, T rh, T wrap, T tolerance) {
        //if rh is only a little bit above wh, we assume it has moved past and not wrapped behind
        bool isahead = (rh > wh) && (rh - wh < tolerance);
        int sgn = isahead ? -1 : 1;
        T wrd = ((rh > wh) && !isahead) ? rh - wrap : rh;
        //if wrapped, positive distance, if ahead, negative distance(only a couple)
        T d = sgn * abs(wh - wrd);
        return d;
    }

    //multichannel audio frame with overloads, simd safe
    //OVERLOADS ASSUME FRAMES OF SAME 'N' (and T but thats slightly less critical)
    template<typename T = float, int N = 2>
    struct FrameStereo {
        std::array<T, N> channel;
        FrameStereo() {
            std::array<T, N> tmp;
            tmp.fill(0);
            channel = tmp;
        }
    
        FrameStereo(T lr) {
            std::array<T, N> tmp;
            tmp.fill(lr);
            channel = tmp;
        }
        FrameStereo(T l, T r){
            channel[0] = l;
            channel[1] = r;
        }
        FrameStereo(T a[N]) {
            for (int i = 0; i < N; ++i) {
                channel[i] = a[i];
            }
        }
        FrameStereo(std::array<T, N> a)  {
            channel = a;
        }
        //index access
        T& operator[](size_t i) {
            return channel[i];
        }
        const T& operator[](size_t i) const {
            return channel[i];
        }
        //set, with scalar
        FrameStereo& operator=(const FrameStereo& f) = default;
        FrameStereo& operator=(T f) {
            for (int i = 0; i < N; ++i) {
                channel[i] = f;
            }
            return *this;
        }

        //these dont check values, only if is the same instance
        friend bool operator!=(const FrameStereo& f, const FrameStereo& b) {
            return { &f != &b };
        }
        friend bool operator==(const FrameStereo& f, const FrameStereo& b) {
            return { &f == &b };
        }
        //other frames
        FrameStereo& operator+=(const FrameStereo& f) {
            for (int i = 0; i < N; ++i) {
                channel[i] += f.channel[i];
            }
            return *this;
        }
        FrameStereo& operator-=(const FrameStereo& f) {
            for (int i = 0; i < N; ++i) {
                channel[i] -= f.channel[i];
            }
            return *this;
        }
        FrameStereo& operator*=(const FrameStereo& f) {
            for (int i = 0; i < N; ++i) {
                channel[i] *= f.channel[i];
            }
            return *this;
        }
        FrameStereo& operator/=(const FrameStereo& f) {
            for (int i = 0; i < N; ++i) {
                channel[i] /= f.channel[i];
            }
            return *this;
        }
        //scalar values
        FrameStereo& operator+=(const T& f) {
            for (int i = 0; i < N; ++i) {
                channel[i] += f;
            }
            return *this;
        }
        FrameStereo& operator-=(const T& f) {
            for (int i = 0; i < N; ++i) {
                channel[i] -= f;
            }
            return *this;
        }
        FrameStereo& operator*=(const T& f) {
            for (int i = 0; i < N; ++i) {
                channel[i] *= f;
            }
            return *this;
        }
        FrameStereo& operator/=(const T& f) {
            for (int i = 0; i < N; ++i) {
                channel[i] /= f;
            }
            return *this;
        }

        //other frames
        friend FrameStereo operator+(FrameStereo a, const FrameStereo& f) {
            FrameStereo<T, N> pls;
            for (int i = 0; i < N; ++i) {
                pls[i] = a[i] + f[i];
            }
            return pls;
        }
        friend FrameStereo operator-(FrameStereo a, const FrameStereo& f) {
            FrameStereo<T, N> mins;
            for (int i = 0; i < N; ++i) {
                mins[i] = a[i] - f[i];
            }
            return mins;
        }
        friend FrameStereo operator*(FrameStereo a, const FrameStereo& f) {
            FrameStereo<T, N> muls;
            for (int i = 0; i < N; ++i) {
                muls[i] = a[i] * f[i];
            }
            return muls;
        }
        friend FrameStereo operator/(FrameStereo a, const FrameStereo& f) {
            FrameStereo<T, N> divs;
            for (int i = 0; i < N; ++i) {
                divs[i] = a[i] / f[i];
            }
            return divs;
        }

        //scalar values
        friend FrameStereo operator+(const T f, FrameStereo& s) {
            FrameStereo<T, N> pls;
            for (int i = 0; i < N; ++i) {
                pls[i] = s[i] + f;
            }
            return pls;
        }
        friend FrameStereo operator-(const T f, FrameStereo& s) {
            FrameStereo<T, N> mins;
            for (int i = 0; i < N; ++i) {
                mins[i] = s[i] - f;
            }
            return mins;
        }
        friend FrameStereo operator*(const T f, FrameStereo& s) {
            FrameStereo<T, N> muls;
            for (int i = 0; i < N; ++i) {
                muls[i] = s[i] * f;
            }
            return muls;
        }
        friend FrameStereo operator/(const T f, FrameStereo& s) {
            FrameStereo<T, N> divs;
            for (int i = 0; i < N; ++i) {
                divs[i] = s[i] / f;
            }
            return divs;
        }
        //polarity
        FrameStereo operator+() const {
            return *this;
        }
        FrameStereo operator-() const {
            std::array<T, N> neg;
            for (int i = 0; i < N; ++i) {
                neg[i] = -this->channel[i];
            }

            return FrameStereo(neg);
        }
        //WARNING:::: L and R must half size of THIS
        void interleave_LR(FrameStereo<T, N / 2> L, FrameStereo<T, N / 2> R) {
            for (int i = 0; i < N; i += 2) {
                this->channel[i] = L.channel[i / 2];
                this->channel[i + 1] = R.channel[i / 2];
            }
        }


    };

    
    template <typename T = float, size_t S = 2048>
    struct IndexDoubleRing {
        static const int SI = S;
        static const int SS = S * 2;
        std::array<T, SS> Buffer;
        std::atomic<size_t> _Whead;
        std::atomic<size_t> _Rhead;

        void clear() {
            _Whead.store(0);
            _Rhead.store(0);
            //no memset here in case non-trivial type
            for (int i = 0; i < SS; ++i) {
                this->Buffer[i] = 0.f;
            }
        }

        IndexDoubleRing() {
            this->clear();
        }
        virtual ~IndexDoubleRing() {}
        //mostly for debugging
        size_t getWrite() {
            return (this->_Whead.load()) % S;
        }
        size_t getRead() {
            return (this->_Rhead.load()) % S;
        }
        //forces read head to write head
        void force_empty() {
            this->_Rhead.store(this->_Whead.load());
        }
        void force_inside() {
            this->_Whead.store(this->_Whead.load() % S);
        }
        //most of these little ones are straight from DoubleRingBuffer, and not even used
        virtual bool is_empty() {
            return this->_Rhead >= this->_Whead;
        }

        bool is_full() {
            return this->_Whead - this->_Rhead >= S;
        }

        size_t read_gap() {
            return this->_Whead - this->_Rhead;
        }

        size_t capacity() {
            return S - this->read_gap();
        }
        //********************  usual use cases  ******************//
        //either Write once per sample and read by blocks(input end)
        //or Read once per sample and write by blocks(output end)
        /*********************************************************/
        //push one sample into buffer
        //auto incr's writehead
        virtual void Write(T in) {
            size_t inw = this->_Whead.load();
            size_t i = inw % S;
            this->Buffer[i] = in;
            this->Buffer[i + S] = in;
            this->_Whead.store((inw + 1));
        }
        //this pair for blocks instead of usinig Write()
        //returns pointer to start of current writeable section
        //MUST ADD LESS THAN S SAMPLES
        virtual T* WriteToBlock() {
            size_t i = this->_Whead.load() % S;
            return &this->Buffer[i];
        }
        //call this after data is copied to return of WriteToBlock with n of samples added
        //MUST BE LESS THAN S samples
        virtual void WriteBlockIncr(size_t n = 1) {
            size_t outw = this->_Whead.load();
            size_t i = outw % S; //get wrapped writehead
            size_t e1 = i + n; //add number of desired samples to writehead count
            size_t e2 = (e1 < S) ? e1 : S; // if samples (e1) will go past S, this will == S
            // Copy data forward
            std::memcpy(&this->Buffer[S + i], &this->Buffer[i], sizeof(T) * (e2 - i));
            if (e1 > S) {
                // Copy data backward from the doubled block to the main block
                std::memcpy(&this->Buffer[0], &this->Buffer[S], sizeof(T) * (e1 - S));
            }
            this->_Whead.store((outw + n));
        }

        //shift one sample from fractional index(auto incr's read head)
        virtual T ReadFromIndexF(float ind) {
            //havent got my wraparound to natively work with size_t's(underflow of course)
            size_t inr = this->_Rhead.load();
            size_t i = wraparound(int(inr), int(ind), SI, true);
            T out = fractionalRead(ind, i, &this->Buffer[0], S);
            this->_Rhead.store((inr + 1));
            return out;
        }

        //this pair for blocks instead
        //returns pointer to data from index (ind) behind current index w/ optional return of index value to given pointer ni
        // to be copied onto another buffer 
        virtual T* ReadBlockFromIndex(size_t ind, size_t* ni = nullptr) {
            size_t i = wraparound(int(this->_Rhead.load()), int(ind), SI, true);
            if (ni) *ni = i;
            return &this->Buffer[i];
        }
        //use this with n of samples used AFTER they are copied etc.
        virtual void ReadBlockIncr(size_t n) {
            size_t inr = this->_Rhead.load();
            this->_Rhead.store((inr + n));
        }

    };
    //********
    //a freezable buffer should be input side only (outputs should keep moving)
    // so only need overrides for those operations
    // a pitching buffer should be on the output(input always comes in in constant time)
    // so only the shifting needs to change
    // a reversible buffer could be input or output so will need more
    // especially when combining with freezable or pitchable
    //********

    template <typename T = float, size_t S = 2048>
    struct FreezeDoubleRing : IndexDoubleRing<T, S> {
        //there is surely another way to use base class members
        using IDR = IndexDoubleRing<T, S>;
        bool Freeze = false;
        size_t Loop =1024;


        void setFreeze(bool f, size_t l) {
            this->Freeze = f;
            this->Loop = l;
        }

        void Write(T in) override {
            if (!this->Freeze) {
                IDR::Write(in);
            }
        }

        void ReadBlockIncr(size_t n) override {

            if (!this->Freeze) {
                IDR::ReadBlockIncr(n);
            }
            
            else {
                int inr = this->_Rhead.load();
                //calculate wraps if requested indexes loop the ring
                int wf = this->_Whead.load() % S;
                //start of frozen slice
                int sf = wraparound((wf), int(Loop), this->SI, true);
                //end of frozen slice --- this may read fron the doubled section of the buffer
                int ef = sf + Loop;
                int dist = get_signed_distance(ef, inr % this->SS, this->SS, 2048);

                //wf close to 0, sf is close to S ,ef is > S
                //inr close to 0 is less than sf so become ef, 
                int nxt = inr + n;
                //int nxtwrp = nxt % (this->SS);
                if (dist < 0) nxt = sf;
                if (dist > int(Loop)) nxt = ef;
                //if outside desired location skip to inside
                //if (nxtwrp > ef) nxt = sf;// (ef);
                //if (nxtwrp < sf) nxt = ef;// (cf);
                this->_Rhead.store(nxt);
            }
        }
    };

    template <typename T = float, size_t S = 2048>
    struct Runaway_PitchDoubleRing : IndexDoubleRing<T, S> {
        float Pitch = 1.f;
        float pitch_incr = 0.f;
        float last_pitch = 1.f;
    
        void setPitch(float voct) {
            this->last_pitch = this->Pitch;
            this->Pitch = VoltToFreq(voct, 0.f, 1.f); //0v = 1x speed, 1v = 2x speed, -1v = 0.5x speed;
        }

        bool is_stable_pitch() {
            return rack::math::isNear(this->last_pitch, this->Pitch);
        }

        int pitch_exchange() {
            float next = this->Pitch + this->pitch_incr;
            int ni = static_cast<int>(next);
            this->pitch_incr = next - ni;
            return ni;
        }
        void read_update(int now, int step) {
            int nextp = wraparound(now, step, this->SI);
            this->_Rhead.store(nextp);
        }
        T ReadFromIndexF(float ind) override {
            size_t inr = this->_Rhead.load();
            size_t i = wraparound(int(inr), int(ind), this->SI, true);
            T out = fractionalRead(ind, i, &this->Buffer[0], S);
            int ni = pitch_exchange();
            read_update(inr, ni);
            return out;
        }
    };

    template <typename T = float, size_t S = 128>
    struct Windowed_Pitch_Buffer{
        static const int SI = S;
        static const int read_offset = SI / 2;
        std::array<T, S> pitchBuffer;
        std::atomic<size_t> _Whead;
        std::atomic<size_t> _Phead1;
        std::atomic<size_t> _Phead2;
        WindowArray<T, S> _pWindow;

        float Pitch = 1.f;
        float pitch_incr = 0.f;
        int dist1;
        int dist2;

        void clear() {
            _Whead.store(0);
            _Phead1.store(0);
            _Phead2.store(read_offset);
            //_pWindow.clear();
            for (int i = 0; i < SI; ++i) {
                this->pitchBuffer[i] = 0.f;
            }
        }
        Windowed_Pitch_Buffer() {
            clear();
            _pWindow.generateWindow(Window_Types::HANN, -0.2f);
        }

        void setPitch(float voct) {
            this->Pitch = VoltToFreq(voct, 0.f, 1.f); //0v = 1x speed, 1v = 2x speed, -1v = 0.5x speed;
        }

        //always write at constant time
        void Write(T in) {
            size_t r = this->_Whead.load();
            size_t constime = r % S;
            pitchBuffer[constime] = in;
            _Whead.store(constime + 1);
        }
        void incr_pitch_heads() {
            float next = this->Pitch + this->pitch_incr;
            int ni = static_cast<int>(next);
            this->pitch_incr = next - ni;
            int nextp = (_Phead1.load() + ni) % S;
            int nextp2 = (nextp + read_offset) % S;
            _Phead1.store(nextp);
            _Phead2.store(nextp2);
        }
        //create window as distance from write head rather than from ring edges
        T window_from_buffer() {
            size_t p1 = _Phead1.load();
            size_t p2 = _Phead2.load();
            size_t t1 = _Whead.load();
            //just simple distance away from _Whead
            dist1 =  p1 > t1 ? S - p1 + t1 : t1 - p1;
            dist2 =  p2 > t1 ? S - p2 + t1 : t1 - p2;
            //distance index can just address window directly

            T out1 = fractionalRead(pitch_incr, p1, &pitchBuffer[0], S);
            out1 *= _pWindow.getWindowInd(dist1);
            T out2 = fractionalRead(pitch_incr, p2, &pitchBuffer[0], S);
            out2 *= _pWindow.getWindowInd(dist2);

            return out1 + out2;
        }
        void get_window_at_current(float* d1, float* d2) {
            *d1 =  _pWindow.getWindowInd(dist1);
            *d2 =  _pWindow.getWindowInd(dist2);
        }
        T Read() {
            T out = window_from_buffer();
            incr_pitch_heads();
            return out;
        }
    };


    //sounds kinda crap, expensive
    template <typename T = float, size_t S = 2048>
    struct PitchDoubleRing : IndexDoubleRing<T, S> {
        using IDR = IndexDoubleRing<T, S>;
        static const int P = S;
        Windowed_Pitch_Buffer<T, P> _Pitcher;


        void clear() {
            //_Pitcher.clear();
            IDR::clear();
        }
        PitchDoubleRing() {
            this->clear();
        }
        ~PitchDoubleRing() {
        }


        void setPitch(float voct) {
            _Pitcher.setPitch(voct);
        }

           
        T ReadFromIndexF(float ind) override {
            size_t rh = this->_Rhead.load();
            size_t wrap = wraparound(int(rh), int(ind), this->SI, true);
            _Pitcher.Write(this->Buffer[wrap]);
            T out = _Pitcher.Read();
            this->_Rhead.store(rh + 1);
        
            return out;
        }
    };


    template <typename T = float, size_t S = 2048>
    struct ReverseFreezeDoubleRing : FreezeDoubleRing<T, S> {
        using FDR = FreezeDoubleRing<T, S>;
        bool Reverse = false;
        int max_gap = S / 4;
       /* bool is_empty() override {
            return Reverse ? this->_Rhead <= this->_Whead : this->_Rhead >= this->_Whead;
        }*/
        void setReverse(bool rev, float gap) {
            this->Reverse = rev;
            max_gap = int(gap);
        }
        void reverse_incr(size_t n) {
            size_t inr = this->_Rhead.load();
            if (!this->Freeze) {
                int rev = wraparound(int(inr), int(n), this->SS, true);
                
                //check distance
                int wh = (this->_Whead.load() % this->SS);
                int dist = get_wrapped_distance(wh, rev, this->SI);
                if (dist > max_gap * 2) rev = wh;
                this->_Rhead.store(rev);
            }
            else {
                //calculate wraps if requested indexes loop the ring
                size_t wf = this->_Whead.load();
                //start of frozen slice
                size_t sf = wraparound(int(wf), int(this->Loop), this->SS, true);
                //end of frozen slice --- this may read fron the doubled section of the buffer
                size_t ef = sf + this->Loop;
                size_t nxt = wraparound(int(inr), int(n), this->SS, true);
                size_t nxtwrp = nxt % (this->SS);
                if (nxtwrp <= sf) nxt = ef;
                if (nxtwrp >= ef) nxt = sf;
                this->_Rhead.store(nxt);
            }
        }
        void ReadBlockIncr(size_t n) override {
            if (!Reverse) {
                FDR::ReadBlockIncr(n);
            }
            else {
                reverse_incr(n);
            }
        }
        T reverse_read(float ind) {
            size_t outr = this->_Rhead.load();
            size_t i = wraparound(int(outr), int(ind), this->SI, true);
            T out = fractionalRead(ind, i, &this->Buffer[0], S);
            size_t rev = wraparound(int(outr), 1, this->SS, true);
            this->_Rhead.store(rev);

            return out;
        }
        T ReadFromIndexF(float ind) override {
            if (!Reverse) {
                return FDR::ReadFromIndexF(ind);
            }
            else {
                return reverse_read(ind);
            }
        }
    };

    template <typename T = float, size_t S = 2048>
    struct ReversePitchDoubleRing : PitchDoubleRing<T, S> {
        using PDR = PitchDoubleRing<T, S>;
        bool Reverse = false;
       /* bool is_empty() override {
            return Reverse ? this->_Rhead <= this->_Whead : this->_Rhead >= this->_Whead;
        }*/
        void setReverse(bool rev) {
            this->Reverse = rev;
        }
        T reverse_read(float ind) {
            size_t rh = this->_Rhead.load();
            this->_Pitcher.Write(this->Buffer[(rh - int(ind)) % S]);
            T out = this->_Pitcher.Read();
            size_t wrap = wraparound(int(rh), 1, this->SI, true);
            this->_Rhead.store(wrap);

            return out;

        }
        T ReadFromIndexF(float ind) override {
            if (!Reverse) {
                return PDR::ReadFromIndexF(ind);
            }
            else {
                return reverse_read(ind);
            }
        }
    };
    template <typename T = float, size_t S = 2048>
    struct Runaway_ReversePitchDoubleRing : Runaway_PitchDoubleRing<T, S> {
        using R_PDR = Runaway_PitchDoubleRing<T, S>;
        bool Reverse = false;
        /*bool is_empty() override {
            return Reverse ? this->_Rhead <= this->_Whead : this->_Rhead >= this->_Whead;
        }*/
        void setReverse(bool rev) {
            this->Reverse = rev;
        }

        T reverse_read(float ind) {
            size_t inr = this->_Rhead.load();
            size_t i = wraparound(int(inr), int(ind), this->SI, true);
            T out = fractionalRead(ind, i, &this->Buffer[0], S);
            int ni = this->pitch_exchange();
            this->read_update(inr, -ni);
            return out;

        }
        T ReadFromIndexF(float ind) override {
            if (!Reverse) {
                return R_PDR::ReadFromIndexF(ind);
            }
            else {
                return reverse_read(ind);
            }
        }
    };
}
}

