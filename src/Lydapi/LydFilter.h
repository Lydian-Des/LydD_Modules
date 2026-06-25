#pragma once
#include <atomic>

namespace LydD {

namespace Filter {

	
		

	//single ringbuffer allpass w/ a 'lowest' cutoff of S samples before it loops back on itself in time
	template <typename T = float, size_t S = 2048>
	struct AllPass1st {
		std::atomic<size_t> ind;
		T Buf[S];
		T blok[8];
		std::atomic<size_t> blkhd;
		T Foffset;
		T Gcoeff;
		void reset() {
			std::memset(Buf, 0.f, sizeof(T) * S);
			ind = 0;
			blkhd = 0;
		}
		AllPass1st() {
			this->reset();
		}
		void setCoeffs(T freq, T gain, float samplerate) {
			this->Gcoeff = gain;
			this->Foffset = FreqToSampleF(freq, samplerate);
		}
		void setCoeffsSample(T samples, T gain) {
			this->Gcoeff = gain;
			this->Foffset = samples;
		}
		//any nested allpass must have its Coeffs set independant
		inline T process(T in, AllPass1st* nest = nullptr) {
			size_t idx = wraparound((this->ind % S) - (int)this->Foffset, S);
			T del = this->Buf[idx];  
			this->blok[blkhd % 8] = del;
			T inM = in - (del * this->Gcoeff);
			T out = /*cubicLerp(this->blok, (blkhd - 2) % 8, this->Foffset, 8)*/ del + (inM * this->Gcoeff); //lerp 4 output from 2 in the past to smooth time changes
			
			this->Buf[this->ind % S] = inM;
			if (nest) this->Buf[this->ind % S] = nest->process(inM);
			++this->ind;
			++this->blkhd;
			return out;
		}
		
	};

	template <typename T = float, size_t S = 44100, int N = 4>
	struct AllP1Smear {
		AllPass1st<T, S> AllP[N];
		void reset() {
			for (int i = 0; i < N; ++i) {
				AllP[i].reset();
			}
		}
		AllP1Smear() {
			this->reset();
		}
		//fund = setof initial delays, mod between 0 - 1, trgt = set of spread delays
		void setSmear(T* fund, T mod, T* trgt, float sr) {
			for (int b = 0; b < N; ++b) {
				AllP[b].setCoeffs(fund[b] + (mod * mod * trgt[b]), 0.35f, sr);
			}
		}

		T SmearSerial(T in) {
			T ser = in;
			for (int a = 0; a < N; a += 1) {
				ser = AllP[a].process(ser);
			}
			return ser;
		}
	};


	template <typename T = float>
	struct AllPass2nd {
		/*
		wh = current sample out of 3 in buffer
		X[n]  = dry input buffer
		Y[n] = final output
		d = break frequency coefficient
		c = bandwidth coefficient
		*/
		int wh = 0;
		//T V[3] = { 0, 0, 0 };
		T X[3] = { 0, 0, 0 };
		T Y[3] = { 0, 0, 0 };
		T d = 0.2;
		T c = 0.2;
		T a[3] = { 0, 0, 0 };
		T b[3] = { 0, 0, 0 };


		void reset() {
			//std::memset(V, (T)0, sizeof(T) * 3);
			std::memset(this->X, (T)0, sizeof(T) * 3);
			std::memset(this->Y, (T)0, sizeof(T) * 3);
			std::memset(this->a, (T)0, sizeof(T) * 3);
			std::memset(this->b, (T)0, sizeof(T) * 3);
			wh = 0;

		}

		AllPass2nd() {
			this->reset();
		}

		//equations found at TheWolfSound
		//cutoff and band  given in Hz
		void setCoeffs(T cutoff, T band, T samplerate) {
			T tang = tanapp((_PI * band) / samplerate);
			this->c = (tang - 1.f) / (tang + 1.f);
			this->d = -cos((_PI * cutoff) / samplerate);
			this->a[0] = -this->c;
			this->a[1] = this->d * (1.f - this->c);
			this->a[2] = 1.f;
			this->b[0] = 0;
			this->b[1] = this->a[1];
			this->b[2] = this->a[0];
		}
		
		inline T process(T in) {
			this->X[wh] = in;
			this->Y[wh] = (this->a[0] * this->X[this->wh]) + (this->a[1] * this->X[wraparound(this->wh - 1, 3)]) + (this->a[2] * this->X[wraparound(this->wh - 2, 3)])
				- (this->b[1] * this->Y[wraparound(this->wh - 1, 3)]) - (this->b[2] * this->Y[wraparound(this->wh - 2, 3)]);
			/*T Y = 0;

			this->V[this->wh] = a[2] * in - ((a[1]) * this->V[wraparound(this->wh - 1, 3)])
							+ (-a[0] * this->V[wraparound(this->wh - 2, 3)]);
			Y = (b[2] * this->V[this->wh]) + ((b[1]) * this->V[wraparound(this->wh - 1, 3)]) + this->V[wraparound(this->wh - 2, 3)];
			*/
			auto hh = this->wh;
			++this->wh;
			this->wh %= 3;
			return this->Y[hh];
		}
		
	};

	template <typename T = float>
	struct SFRCFilter {
		rack::dsp::TRCFilter<T> _low;
		rack::dsp::TRCFilter<T> _high;

		void reset() {
			_low.reset();
			_high.reset();
		}
		SFRCFilter() {
			this->reset();
		}
		void setCut(T f, float sr) {
			_low.setCutoffFreq(f / sr);
			_high.setCutoffFreq((f + 100.f) / sr);
		}
		T process(T in, T p) {
			_low.process(in);
			_high.process(in);
			T out = rack::simd::crossfade(_low.lowpass(), _high.highpass(), p);
			return out;
		}
	};

	//WARNING:: S must be a power of 2
	//for one-in/one-out style delays
	//double ring format allows you to pull blocks if you wish as long as they are less than S in size
	//without worrying about wrapping
	template<typename T = float, size_t S = 2048>
	struct BaseDoubleRing {
		const uint32_t M = S - 1;
		std::atomic<size_t> wh;
		std::atomic<size_t> rh;
		T* Buf;
		virtual void reset() {
			std::memset(this->Buf, 0, sizeof(T) * S * 2);
			this->wh.store(0);
			this->rh.store(0);
		}
		BaseDoubleRing() {
			Buf = new T[S * 2];
			this->reset();
		}
		virtual ~BaseDoubleRing() {
			delete[] Buf;
		}
		virtual void push(T in) {
			size_t ci = wh & M;
			this->Buf[ci] = in;
			this->Buf[(ci)+S] = in;
			this->wh.store(this->wh + 1);
		}
		inline virtual T pull() {
			size_t ri = this->rh & M;
			return this->Buf[ri];
			this->rh.store(this->rh + 1);
		}
	};

	template <typename T = float, size_t S = 2048>
	struct DelayCombBase: BaseDoubleRing<T, S> {
		using BA = BaseDoubleRing<T, S>;
		std::atomic<size_t> blkhd;
		T Dry;
		T* blok;
		T Freq;
		T FeedGain;
		int Inverting;
		void reset() override {
			this->blkhd = 0;
			std::memset(this->blok, 0, sizeof(T) * 8);
			this->Freq = 1;
			this->FeedGain = 0.1;
			this->Inverting = false;
			BA::reset();
		}
		DelayCombBase() {
			blok = new T[8];
			this->reset();
		}
		~DelayCombBase() {
			delete[] blok;
		}
		void setFreq(T f, LydD::Frequency_Types type, float sr) {
			this->Freq = FrequencyToSampleConvert(f, type, sr);
		}
		void setGain(T g) {
			this->FeedGain = g;
		}
		//inverting is a multiplier for branchless processing(this can/should be set on init)
		void inverting(bool v) {
			this->Inverting = v ? -1 : 1;
		}
		void push(T in) override {
			Dry = in;
			BA::push(in);
		}
		//pulling is left to subclasses, use 'blok' as a smoothing window if you like
	};

	template <typename T = float, size_t S = 2048>
	struct DelayCombPuller : DelayCombBase<T, S> {
		using DC = DelayCombBase<T, S>;
		using BA = BaseDoubleRing<T, S>;
		inline T pull() override {
			T fridx = BA::rh - DC::Freq;
			size_t idx = wraparound(int(fridx), int(S));
			DC::blok[DC::blkhd % 8] = BA::Buf[idx];
			T out = cubicLerp(DC::blok, (DC::blkhd - 2) % 8, fridx, 8);
			BA::rh.store(BA::rh + 1);
			DC::blkhd.store((DC::blkhd + 1) & 7);
			return out;
		}
	};
	
	using U = rack::simd::float_4;
	template<size_t S>
	struct DelayCombPuller<U, S> : DelayCombBase<U, S> {
		using DC = DelayCombBase<U, S>;
		using BA = BaseDoubleRing<U, S>;
		inline U pull() override {
			size_t ci = BA::rh & BA::M;
			U _idx = wraparound(U{ ci - DC::Freq }, U{ S });// rack::simd::floor(wraparound((ci)-this->Freq, (T)S));
			size_t idx0 = static_cast<size_t>(_idx[0]); //grab each index from indpendent delay times
			size_t idx1 = static_cast<size_t>(_idx[1]);
			size_t idx2 = static_cast<size_t>(_idx[2]);
			size_t idx3 = static_cast<size_t>(_idx[3]);
			U lns = U{ BA::Buf[idx0][0], BA::Buf[idx1][1], BA::Buf[idx2][2], BA::Buf[idx3][3] };
			DC::blok[DC::blkhd % 8] = lns;
			U out = cubicLerp(DC::blok, (DC::blkhd - 2) % 8, DC::Freq, 8);
			BA::rh.store(BA::rh + 1);
			DC::blkhd.store((DC::blkhd + 1) & 7);
			return out;
		}
	};

	template <typename T = float, size_t S = 2048>
	struct DelayComb : DelayCombPuller<T, S> {
		using DP = DelayCombPuller<T, S>;
		using DC = DelayCombBase<T, S>;
		using BA = BaseDoubleRing<T, S>;
		DelayComb() {}
		~DelayComb() {}

		void setPars(T freq, T FB, float sr, LydD::Frequency_Types freqtype = LydD::Frequency_Types::SAMPLES, bool neg = false) {
			DC::setGain(FB);
			DC::setFreq(freq, freqtype, sr);
			DC::inverting(neg);
		}
		inline T process(T in) {
			T wet = DP::pull();
			T pus = DC::Inverting * (in + (wet * DC::FeedGain));
			DC::push(pus);
			return wet;
		}
	};


	template <typename T = float, size_t S = 2048>
	struct FFPuller : DelayCombBase<T, S>{
		using DC = DelayCombBase<T, S>;
		using BA = BaseDoubleRing<T, S>;
		FFPuller() {}
		~FFPuller() {}
		inline T pull() override {
			T fridx = BA::rh - DC::Freq;
			size_t idx = wraparound(int(fridx), int(S));
			DC::blok[DC::blkhd] = BA::Buf[idx];
			T delsm = cubicLerp(DC::blok, (DC::blkhd), fridx, 8);
			T out = (DC::Dry) + (DC::Inverting * (delsm * DC::FeedGain));
			BA::rh.store(BA::rh + 1);
			DC::blkhd.store(DC::blkhd + 1) % 8;
			return out;
		}
	};
	//if T for FFComb is float_4 use this process instead
	//S MUST BE POWER OF 2//
	using U = rack::simd::float_4;
	template<size_t S>
	struct FFPuller<U, S> : DelayCombBase<U,S> {
		using DC = DelayCombBase<U, S>;
		using BA = BaseDoubleRing<U, S>;
		FFPuller() {}
		~FFPuller() {}
		inline U pull() override {
			size_t ci = BA::rh & BA::M;
			U _idx = wraparound(U{ ci - DC::Freq }, U{ S });// rack::simd::floor(wraparound((ci)-this->Freq, (T)S));
			size_t idx0 = static_cast<size_t>(_idx[0]); //grab each index from indpendent delay times
			size_t idx1 = static_cast<size_t>(_idx[1]);
			size_t idx2 = static_cast<size_t>(_idx[2]);
			size_t idx3 = static_cast<size_t>(_idx[3]);

			U lns = U{ BA::Buf[idx0][0], BA::Buf[idx1][1], BA::Buf[idx2][2], BA::Buf[idx3][3] };
			DC::blok[DC::blkhd] = lns;
			U delsm = lns;// cubicLerp(DC::blok, (DC::blkhd), 0.5, 8);
			U out = (DC::Dry) + (DC::Inverting * (delsm * DC::FeedGain));
			BA::rh.store(BA::rh + 1);
			DC::blkhd.store((DC::blkhd + 1) & 7);
			return out;
		}
	};

	template <typename T = float, size_t S = 2048>
	struct FFComb : FFPuller<T, S> {
		using FP = FFPuller<T, S>;
		using DC = DelayCombBase<U, S>;
		using BA = BaseDoubleRing<U, S>;
		FFComb() {}
		~FFComb() {}
		void setPars(T freq, T FF, float sr, LydD::Frequency_Types freqtype = 0, bool neg = false) {
			DC::inverting(neg);
			DC::setFreq(freq, freqtype, sr);
			DC::setGain(FF);			
		}
		inline T process(T in) {
			DC::push(in);
			T out = FP::pull();
			return out;				
		}
		
	};
}
}
