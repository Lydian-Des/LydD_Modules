#pragma once
#include "rack.hpp"
#include "LydBase.h"
#include <atomic>
namespace LydD {
namespace Filter {
	
	//eq parameters for various materials (using EQ4Band)
	extern float WoodF[4];
	extern float WoodQ[4];
	extern float WoodG[4];
	extern float SteelF[4];
	extern float SteelQ[4];
	extern float SteelG[4];
	extern float BoardF[4];
	extern float BoardQ[4];
	extern float BoardG[4];
	
	//chamberlin implementation i found somewhere
	template <typename T = float>
	struct Chamberlin {
		T Freq;
		T Q;
		T Buf[3]; 
		T pole1;
		T pole1prev;
		T pole2;
		T pole2prev;
		T gain;
		T LP;
		T BP;
		T NP;
		T HP; 
		Chamberlin() {
			pole1 = 0;
			pole2 = 0;
		}
		// f = frequncy in Hz, q = resonance/must be more than 0.5, st = sampleTime
		void setParameters(T f, T q, T g, float st) {
			this->Freq = 2.f * sin((_PI * f) * st);
			this->Q = 1.f / q;
			this->gain = g;
		}
		void process(T in) {
			HP = in - pole2 - (pole1 * Q); //highpass is first tap
			BP = HP * Freq + pole1;			
			LP = pole1 * Freq + pole2;		
			NP = HP + LP;

			pole1 = BP;
			pole2 = LP;
		}
		T lowpass() {
			return LP * gain;
		}
		T bandpass() {
			return BP * gain;
		}
		T notch() {
			return NP * gain;
		}
		T highpass() {
			return HP * gain;
		}
	};

	//simple cruddy eq from 4 filters
	template <typename T = float>
	struct EQ4Band {


		int types[4] = { 2, 6, 6 , 3 }; //low, band, band hi in biquad type
		Chamberlin<T> Bands[4];
		rack::dsp::TBiquadFilter<T> BBands[4];
		using type = rack::dsp::BiquadFilter::Type;

		//FOR CHAMBERLIN
		/*void setBands(T* bandFreq, T* bandQ, T* bandG, float st) {
			for (int b = 0; b < 4; ++b) {
				Bands[b].setParameters(bandFreq[b] * 0.5f, bandQ[b], bandG[b], st);
			}
		}

		T process(T in) {
			for (int b = 0; b < 4; ++b) {
				Bands[b].process(in);
			}
			T blend = (Bands[0].lowpass() + Bands[1].notch()
						+ Bands[2].notch() + Bands[3].bandpass()) * 0.45;
			return blend;
		}*/
		//FOR BIQUAD
		void setBands(T* bandFreq, T* bandQ, T* bandG, float st) {

			BBands[0].setParameters(type::LOWPASS, bandFreq[0] * st, bandQ[0], bandG[0]);
			BBands[1].setParameters(type::BANDPASS,  bandFreq[1] * st, bandQ[1], bandG[1]);
			BBands[2].setParameters(type::NOTCH,  bandFreq[2] * st, bandQ[2], bandG[2]);
			BBands[3].setParameters(type::HIGHPASS,  bandFreq[3] * st, bandQ[3], bandG[3]);
			
		}

		T process(T in) {
			T blend = 0;
			for (int b = 0; b < 4; ++b) {
				blend += BBands[b].process(in);
			}
			blend *= 0.45f;
			return blend;
		}
	};

}
}
