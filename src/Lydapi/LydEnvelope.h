#pragma once
#include "rack.hpp"
#include "LydBase.h"
#include <atomic>
namespace LydD {
namespace Envelope {
	
    //AR/ASR envelope able to trigger another with delay
    //no float_4 compat yet
	template<typename T = float>
	struct Couplable_Envelope {
    private:
        rack::dsp::BooleanTrigger _trigger;
        rack::dsp::BooleanTrigger _EOC;
        rack::dsp::PulseGenerator _EOCPulse;
        //total time between triggers
        rack::dsp::TTimer<T> _epoch;
        //doesnt run during sustain to keep release happy
        rack::dsp::TTimer<T> _interval;

        T attack_time;
        T release_time;
        T total_time;
        T attack_phase;
        T release_phase;
        T offset_from_parent;
        T time_since_trigger;
        T Env_elope;

        bool is_triggered;
        bool asr_mode;
        bool Attacking;
        bool Sustaining;
        bool Releasing;
        bool EOC;
        //true upon init, false once triggered, true again after envelope is over
        bool is_finished;
        Couplable_Envelope* child = nullptr;

        bool grab_EOC(float sampletime) {
            bool eoc_trig = this->_EOC.process(this->Env_elope <= 0.001f);
            this->_EOCPulse.process(sampletime);
            if (eoc_trig) {
                this->_EOCPulse.trigger(0.08f);
            }
            return this->_EOCPulse.isHigh();
        }

        void attack(T st) {
            if (!this->Attacking) {
                return;
            }
            else {
                //fix releasephase to 0
                this->release_phase = 0.f;
                //cover for retrigger after attack was over
                if (this->Releasing) {
                    //shoudlnt matter do it anyawy
                    this->Releasing = false;
                    this->_interval.reset();
                    T comp = this->Env_elope * this->attack_time;
                    //adjust time directly
                    this->_interval.time += (comp);
                }
                T atta = this->_interval.process(st);
                this->attack_phase = atta / this->attack_time;
                if (this->attack_phase >= 1.f) {
                    this->Releasing = true;
                    this->Attacking = false;
                }
                return;
            }

            return;

        }
        void release(T st) {
            if (this->is_finished) {
                this->attack_phase = 0.f;
                this->release_phase = 0.f;
                return;
            }
            if (this->is_sustaining()) {
                this->attack_phase = 0.f;
                this->release_phase = 1.f;
                return;
            }
            else if (!this->is_sustaining() && this->Releasing) {
                T rele = this->_interval.process(st);
                this->attack_phase = 0.f;
                this->release_phase = (this->total_time - rele) / this->release_time;
                if (this->release_phase <= 0.f) {
                    this->release_phase = 0.f;
                    this->Releasing = false;
                    this->is_finished = true;
                    _interval.reset();
                }
                return;
            }

            return;

        }
    public:

        //give this one a child(params and delay set separately)
        //do only once on init probly
        void clear() {
            attack_time = 0.5f;
            release_time = 0.5f;
            total_time = 1.f;
            attack_phase = 0.f;
            release_phase = 0.f;
            Env_elope = 0.f;
            offset_from_parent = 0.f;
            time_since_trigger = 0.f;
            is_finished = true;
            _epoch.reset();
            _interval.reset();
            _EOCPulse.reset();         
        }
        Couplable_Envelope() {
            clear();
            child = nullptr;
        }
        ~Couplable_Envelope() {
            if (child) child = nullptr;
        }

        void set_child(Couplable_Envelope* c) {
            this->child = c;
        }
        //a & r as actual desired time in seconds
        void setAttackRelease(T a, T r) {
            this->attack_time = a;
            this->release_time = r;
            this->total_time = a + r;
        }
        //different kind of set, take total time and proportion of attack length 0-1
        void setProportionalTime(T time, T pro) {
            this->total_time = time;
            this->attack_time = time * pro;
            this->release_time = time - this->attack_time;
        }
        void set_asr_mode(bool s) {
            this->asr_mode = s;
        }
        //amount of time in seconds to wait after parent to trigger
        //wont trigger if parent triggers again before time is up
        void set_delay_from_parent(T delay) {
            offset_from_parent = delay;
        }

        //snd gate(t) as bool > whatever level threshold
        //if triggered, will stay triggered until next call of this function
        //only call highest parent trigger
        void trigger(bool t, T st) {
            this->is_triggered = this->_trigger.process(t);
            if (this->is_triggered) {
                this->_epoch.reset();
                this->Attacking = true;
                this->is_finished = false;
            }
            this->time_since_trigger = this->_epoch.process(st);
            this->Sustaining = t;
            if (this->child) {
                bool ct = this->time_since_trigger >= this->child->offset_from_parent;
                this->child->trigger(ct, t, st);
            }
        }
        //trig for when trigger and gate are separate
        //only children really use this
        void trigger(bool t, bool g, T st) {

            this->is_triggered = this->_trigger.process(t);
            if (this->is_triggered) {
                this->_epoch.reset();
                this->Attacking = true;
                this->is_finished = false;
            }
            this->time_since_trigger = this->_epoch.process(st);
            this->Sustaining = g;
            if (this->child) {
                bool ct = this->time_since_trigger >= this->child->offset_from_parent;
                this->child->trigger(ct, g, st);
            }
        }

        void process(T st) {
            attack(st);
            release(st);
            this->Env_elope = this->attack_phase + this->release_phase;
            this->EOC = this->grab_EOC(st);
            if (this->child) this->child->process(st);
        }

        T getEnvelope() {
            return this->Env_elope;
        }

        bool trigger_moment() {
            return this->is_triggered;
        }
        bool is_sustaining() {
            return this->asr_mode && this->Sustaining && !this->Attacking;
        }
        bool is_attacking() {
            return this->Attacking;
        }
        bool is_releasing() {
            return this->Releasing;
        }
        bool is_EOC() {
            return this->EOC;
        }
	};


    class Naive_Companion_Envelope {
    private:
        rack::dsp::BooleanTrigger _trigger;
        rack::dsp::BooleanTrigger _EOC;
        rack::dsp::PulseGenerator _EOCPulse;
        rack::dsp::Timer _totalTime;
    public:
        float Atime;
        float Rtime;
        float Aphase;
        float Rphase;
        float samplePhase;
        float TotalTime;
        float Shape;
        bool Attacking;
        bool Sustain;
        bool Sustaining;
        bool EOC;

        Naive_Companion_Envelope() {
            Atime = 0.1f;
            Rtime = 0.2f;
            Aphase = 0.f;
            Rphase = 0.f;
            samplePhase = 0.f;
            Attacking = false;
            Sustain = false;
            Sustaining = false;
            EOC = false;
            _trigger.reset();
            _totalTime.reset();
        }

        void setAttackRelease(bool timesize, float a, float r, bool sus) {
            float multiplier = (timesize) ? 0.5 : 12.f;
            this->Atime = a * a * multiplier;
            this->Rtime = r * r * multiplier;
            this->Sustain = sus;
        }
        void setShape(float shape) {
            this->Shape = shape;
        }
        //build in retrigger smoothing
        void Trigger(bool gate, float sampletime, bool* istrig = nullptr) {
            bool triggered = this->_trigger.process(gate);
            this->Sustaining = (this->Sustain) ? gate : false;
            //capture total time between triggers
            TotalTime = _totalTime.process(sampletime);
            if (triggered) {
                //retrigger
                if (this->samplePhase != 0.f) {
                    this->samplePhase = (this->Attacking) ? this->Aphase * this->Atime : (-this->Rphase + 1.f) * this->Atime;
                }
                this->Attacking = true;
                _totalTime.reset();
            }
            //leave it to comsumer to set back to false
            if (istrig) *istrig |= triggered;
            return;
        }
        void triggerCompanion(Naive_Companion_Envelope* companion, float delaytime, float sampletime) {
            float comphase = this->TotalTime;
            //float totalphase = (this->Attacking) ? this->Aphase : this->Rphase + 1.f;
            float trigcompanion = (comphase >= delaytime) || this->Sustaining ? 1.f : 0.f;
            companion->Trigger(trigcompanion, sampletime);
        }
        void AttackPhase(float* Value, float sampletime) {
            if (this->Attacking) {
                float shapemod = lerp(1.f, *Value, 0.f, 1.f, this->Shape);

                this->Aphase = this->samplePhase / this->Atime;
                *Value = this->Aphase * shapemod;
                if (*Value >= 1.f) {
                    *Value = 1.f;
                    this->Attacking = false;
                    this->Aphase = 0.f;
                    this->samplePhase = 0.f;
                    return;
                }
                this->samplePhase += sampletime;
            }
            return;
        }
        void ReleasePhase(float* Value, float sampletime) {
            this->EOC = _EOC.process(*Value <= 0.01f);
            if (this->Sustaining) return;
            if (!this->Attacking && *Value > 0.f) {
                float shapemod = lerp(1.f, *Value, 0.f, 1.f, this->Shape);

                this->Rphase = this->samplePhase / this->Rtime;
                *Value = (-this->Rphase + 1.f) * shapemod;

                if (*Value <= 0.f) {
                    *Value = 0.f;
                    this->Rphase = 0.f;
                    this->samplePhase = 0.f;
                    return;
                }
                this->samplePhase += sampletime;
            }
            return;
        }

        bool isEOC(float sampletime) {
            _EOCPulse.process(sampletime);
            if (this->EOC) {
                _EOCPulse.trigger(0.08f);
            }
            return _EOCPulse.isHigh();
        }
    };
}
}
