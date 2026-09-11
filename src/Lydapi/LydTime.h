#pragma once
#include "rack.hpp"
#include <math.h>
#include <vector>


namespace LydD {

namespace Time {
	template<typename T = float>
	struct ImpulseTimer {
		rack::dsp::TTimer<T> _time;
		rack::dsp::BooleanTrigger _trig;
		T clockTime;
		bool clockTick;
		void reset() {
			_time.reset();
			_trig.reset();
			clockTime = 0.f;
			clockTick = false;
		}
		ImpulseTimer() {
			reset();
		}
		bool getTick() {
			return this->clockTick;
		}
		virtual T process(T clk, float st) {
			_time.process(st);
			bool istick = clk >= 1.f;
			clockTick = _trig.process(istick);
			if (clockTick) {
				clockTime = _time.getTime();
				_time.reset();
			}
			return clockTime;
		}
	};

	template<typename T = float, int W = 2>
	struct AverageTimer : ImpulseTimer<T> {
		T storedTime[W];
		T lastTime = 0.5; //default 120 bpm
		int index;
		//T average;
		void reset() {
			//average = 0;
			index = 0;
			for (int i = 0; i < W; ++i) {
				storedTime[i] = lastTime;
			}
		}
		AverageTimer() {
			reset();
		}

		void store(T clk, T st) {
			T timeget = ImpulseTimer<T>::process(clk, st);
			lastTime = timeget;
			bool tick = ImpulseTimer<T>::getTick();
			if (tick) {
				storedTime[index] = timeget;
				index = (index + 1) % W;
				
			}
		}
		T average() {
			T avg = 0.f;
			for (int i = 0; i < W; ++i) {
				int idx = (i + index) % W;
				avg += storedTime[idx];
			}
			avg /= T(W);
			return avg;
		}
	};
}
}

