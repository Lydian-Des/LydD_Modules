#include "plugin.hpp"
#include <vector>
#include <string>

#define MODULE_NAME ClockModule
#define PANEL "Clock_panel.svg"

static const int maxPolyphony = 1;

static rack::simd::float_4 Zero{ 0.f };
static rack::simd::float_4 One{ 1.f };

class ClockTree {
private:
    const float _2PIto1 = 0.1591549431f;//multiply 2pi based phase by this to rescale to 0-1

    enum FreeClocks {
        THRU_QUARTER,
        SIXTEENTH,
        EIGHTH,
        DOTTED_EIGHTH,
        OFFBEAT_QUARTER,
        TRIPLET_BEATS,
        TRIPLET_MEASURE,
        MID_MEASURE,
        MEASURE,
        FOUR_MEAS,
        BEATS_MEAS,
        SIXTEEN_MEAS,
        BBQ_MEAS,
        NUM_CLOCKS
    };
    enum MeasureClocks {
        B_DOTTED_SIXTEENTH,
        B_DOTTED_EIGHTH,
        B_TRIPLET_QUARTER,
        B_DOTTED_QUARTER,
        B_HALF_NOTE,
        B_UNUSED,
        NUM_MB_CLOCKS
    };

    float fundclockFreq = 0.f;
    float fundclockPhase = 0.f;
    bool fundphaseReset = false;
    float Swing = 0.f;
    float pulseWidth[NUM_CLOCKS]; // first 6 reused in measure clocks, its global and they're all the same anyway 

    int clocksTick[NUM_CLOCKS];
    float clocksFreq[NUM_CLOCKS];
    float clocksPhase[NUM_CLOCKS];
    bool phaseSet[NUM_CLOCKS];

    float measBfreq[NUM_MB_CLOCKS];
    float measBphase[NUM_MB_CLOCKS];
    bool measBset[NUM_MB_CLOCKS];

    float refBPS = 2.f; // frequency calculation gives and takes Hz- 2Hz = 120 bpm.
    float BPMparvolt = 0.f;
    float BPMinvolt = 0.f;

    bool isextConnect = false;
    float externalclockFreq = 0.f;

    int timeSignatureBeats = 4.f;
    int timeSignatureQuaver = 4.f;



    std::vector<float> averageTick; // last 4 'time-between-pulse's to average out
    rack::dsp::BooleanTrigger clockphaseResets[NUM_CLOCKS]; // 0 is base clock
    rack::dsp::BooleanTrigger measBResets[NUM_MB_CLOCKS]; //measurebound triggers
    rack::dsp::PulseGenerator outputPulses[NUM_CLOCKS]; // 0 is base clock
    rack::dsp::PulseGenerator measBPulses[NUM_MB_CLOCKS]; //measurebound pulses
    rack::dsp::Timer externalTimer;

public:

    ClockTree() {
        externalTimer.reset();
        for (int bp = 0; bp < NUM_CLOCKS; ++bp) {
            clockphaseResets[bp].reset();
            outputPulses[bp].reset();
            if (bp < 4) {
                averageTick.push_back(0.5f); //0.5s between pulse = 2Hz = 120bpm.
            }
            pulseWidth[bp] = 0.5f;
        }
    }
    void setBPMparameter(float parameter) {
        BPMparvolt = parameter;
    }
    void setBPMinput(bool isconnect, float voltage) {
        BPMinvolt = (isconnect) ? voltage : 0.f;
    }
    void setfundFreq() {
        float internalBPM = BPMparvolt + BPMinvolt;
        if (!isextConnect) {
            clocksFreq[THRU_QUARTER] = VoltToFreq(internalBPM, 0.f, refBPS);
        }
        else {
            clocksFreq[THRU_QUARTER] = externalclockFreq;
        }
    }
    void setSwing(float swing) {
        Swing = swing;
    }
    void setPulseW(float PWM) {
        for (int i = 0; i < NUM_CLOCKS; ++i) {
            pulseWidth[i] = PWM;
        }
    }

    void TimeSignature(float beats, float quaver) {
        timeSignatureBeats = beats;
        timeSignatureQuaver = quaver;
    }

    void reset() {
        for (int i = 0; i < NUM_CLOCKS; ++i) {
            phaseSet[i] = false;
            clocksTick[i] = -1;
            clocksPhase[i] = 0.f;
            clockphaseResets[i].reset();
            outputPulses[i].reset();
        }
        for (int i = 0; i < NUM_MB_CLOCKS; ++i) {
            measBset[i] = false;
            measBphase[i] = 0.f;
            measBResets[i].reset();
            measBPulses[i].reset();
        }
    }
    void clockAdvance(int which) {
        ++clocksTick[which];
        clocksTick[which] %= 2520;  //2520 divisible by 1 - 9
    }

    void phaseCProcess(float* fdst, float* fsrc, float div, float* phdst, bool* psetdst, rack::dsp::BooleanTrigger* rst) {
        *fdst = *fsrc / div;
        bool isTap = *phdst > 0.f && *phdst < 0.1f;//enough window to catch for sure
        *psetdst = rst->process(isTap);
    }
    void tapCProcess(float* fdst, float* fsrc, float div, bool isTap, bool* psetdst, rack::dsp::BooleanTrigger* rst) {
        *fdst = *fsrc / div;
        *psetdst = rst->process(isTap);
    }


    void phaseAccum(float samplerate) {

        if (!isextConnect) {
            bool internalTick = clocksPhase[THRU_QUARTER] > 0.f && clocksPhase[THRU_QUARTER] < 1.f;
            phaseSet[THRU_QUARTER] = clockphaseResets[THRU_QUARTER].process(internalTick);
            externalTimer.reset();
        }

        clocksFreq[OFFBEAT_QUARTER] = clocksFreq[THRU_QUARTER];
        clocksPhase[OFFBEAT_QUARTER] = clocksPhase[THRU_QUARTER] + (_PI + Swing);
        bool offbeattick = clocksPhase[THRU_QUARTER] > (_PI + Swing);
        phaseSet[OFFBEAT_QUARTER] = clockphaseResets[OFFBEAT_QUARTER].process(offbeattick);

        //Generated multiples and measures

        float measurelength = (timeSignatureBeats * (timeSignatureQuaver / 4.f));
        int signote = THRU_QUARTER;
        if (timeSignatureQuaver < 4) {
            signote = EIGHTH;
            measurelength = measurelength * 2.f;
        }

        phaseCProcess(&clocksFreq[MEASURE], &clocksFreq[signote], measurelength,
            &clocksPhase[MEASURE], &phaseSet[MEASURE], &clockphaseResets[MEASURE]);

        phaseCProcess(&clocksFreq[SIXTEENTH], &clocksFreq[THRU_QUARTER], 0.25f,
            &clocksPhase[SIXTEENTH], &phaseSet[SIXTEENTH], &clockphaseResets[SIXTEENTH]);

        bool eighthtick = (clocksPhase[THRU_QUARTER] > 0.f && clocksPhase[THRU_QUARTER] < 0.25f)
            || (offbeattick && clocksPhase[THRU_QUARTER] < _2_PI - 0.5);
        tapCProcess(&clocksFreq[EIGHTH], &clocksFreq[THRU_QUARTER], 0.5f,
            eighthtick, &phaseSet[EIGHTH], &clockphaseResets[EIGHTH]);

        phaseCProcess(&clocksFreq[DOTTED_EIGHTH], &clocksFreq[THRU_QUARTER], 0.75f,
            &clocksPhase[DOTTED_EIGHTH], &phaseSet[DOTTED_EIGHTH], &clockphaseResets[DOTTED_EIGHTH]);

        phaseCProcess(&clocksFreq[TRIPLET_BEATS], &clocksFreq[signote], 0.666666666667f,
            &clocksPhase[TRIPLET_BEATS], &phaseSet[TRIPLET_BEATS], &clockphaseResets[TRIPLET_BEATS]);

        phaseCProcess(&clocksFreq[TRIPLET_MEASURE], &clocksFreq[MEASURE], 0.333333333333f,
            &clocksPhase[TRIPLET_MEASURE], &phaseSet[TRIPLET_MEASURE], &clockphaseResets[TRIPLET_MEASURE]);

        //if even time signature, dont offset, but if odd offset up by one, e.g. 3-2 in 5/4 is more common than 2-3
        int clocksmid = (rack::math::isEven((int)(measurelength))) ? ((int)(measurelength / 2)) : ((int)(measurelength / 2) + 1);
        bool front = clocksTick[MID_MEASURE] % 2 == 0; //is true every even tick of this clock
        int relevantclocks = clocksTick[signote] % timeSignatureBeats; //wrap fundamental quavers around number of beats
        int midclocks = (front) ? clocksmid : timeSignatureBeats; //decide how many ticks to wait, -from the start-
        clocksFreq[MID_MEASURE] = clocksFreq[signote] / ((front) ? clocksmid : (timeSignatureBeats - clocksmid));//get frequency to follow this division
        bool midmeasuretick = relevantclocks % midclocks == 0;  //e.g. int 5/4 tick 0 waits 3 fund ticks(clocksmid), then tick 1 waits 2 more for a total of 5(timesigbeats)        
        phaseSet[MID_MEASURE] = clockphaseResets[MID_MEASURE].process(midmeasuretick);

        bool fourmeasuretick = clocksTick[MEASURE] % 4 == 0;
        tapCProcess(&clocksFreq[FOUR_MEAS], &clocksFreq[MEASURE], 4.f,
            fourmeasuretick, &phaseSet[FOUR_MEAS], &clockphaseResets[FOUR_MEAS]);

        bool beatsmeasuretick = ((clocksTick[MEASURE] % (int)timeSignatureBeats + 1) + (clocksPhase[MEASURE] / _2_PI)) >= timeSignatureBeats;
        tapCProcess(&clocksFreq[BEATS_MEAS], &clocksFreq[MEASURE], timeSignatureBeats,
            beatsmeasuretick, &phaseSet[BEATS_MEAS], &clockphaseResets[BEATS_MEAS]);

        bool sixteenmeasuretick = clocksTick[FOUR_MEAS] % 4 == 0;
        tapCProcess(&clocksFreq[SIXTEEN_MEAS], &clocksFreq[FOUR_MEAS], 4.f,
            sixteenmeasuretick, &phaseSet[SIXTEEN_MEAS], &clockphaseResets[SIXTEEN_MEAS]);

        float bbq = timeSignatureBeats * timeSignatureQuaver;
        bool bbqmeasuretick = ((clocksTick[MEASURE] % (int)bbq + 1) + (clocksPhase[MEASURE] / _2_PI) >= bbq);
        tapCProcess(&clocksFreq[BBQ_MEAS], &clocksFreq[MEASURE], bbq,
            bbqmeasuretick, &phaseSet[BBQ_MEAS], &clockphaseResets[BBQ_MEAS]);


        for (int i = 0; i < NUM_CLOCKS; ++i) {
            if (phaseSet[i]) {
                clocksPhase[i] = 0.f;
                clockAdvance(i);
            }

            incrementPhase(clocksFreq[i], samplerate, &clocksPhase[i]);
        }



        //Measure Bound synchopations
        phaseCProcess(&measBfreq[B_DOTTED_SIXTEENTH], &clocksFreq[EIGHTH], 0.75f,
            &measBphase[B_DOTTED_SIXTEENTH], &measBset[B_DOTTED_SIXTEENTH], &measBResets[B_DOTTED_SIXTEENTH]);

        phaseCProcess(&measBfreq[B_DOTTED_EIGHTH], &clocksFreq[DOTTED_EIGHTH], 1.f,
            &measBphase[B_DOTTED_EIGHTH], &measBset[B_DOTTED_EIGHTH], &measBResets[B_DOTTED_EIGHTH]);

        phaseCProcess(&measBfreq[B_TRIPLET_QUARTER], &clocksFreq[TRIPLET_BEATS], 1.f,
            &measBphase[B_TRIPLET_QUARTER], &measBset[B_TRIPLET_QUARTER], &measBResets[B_TRIPLET_QUARTER]);

        phaseCProcess(&measBfreq[B_DOTTED_QUARTER], &clocksFreq[THRU_QUARTER], (2.f * 0.75f),
            &measBphase[B_DOTTED_QUARTER], &measBset[B_DOTTED_QUARTER], &measBResets[B_DOTTED_QUARTER]);

        phaseCProcess(&measBfreq[B_HALF_NOTE], &clocksFreq[THRU_QUARTER], 1.f,
            &measBphase[B_HALF_NOTE], &measBset[B_HALF_NOTE], &measBResets[B_HALF_NOTE]);

        for (int i = 0; i < NUM_MB_CLOCKS; ++i) {
            if (measBset[i] || phaseSet[8]) {
                measBphase[i] = 0.f;
            }

            incrementPhase(measBfreq[i], samplerate, &measBphase[i]);
        }

    }


    void externalBPMgen(float sampletime, bool connected, float inputgate) {
        isextConnect = connected;
        if (isextConnect) {
            externalTimer.process(sampletime);
            bool externalTick = inputgate >= 1.f;
            phaseSet[THRU_QUARTER] = clockphaseResets[THRU_QUARTER].process(externalTick);
            if (phaseSet[THRU_QUARTER]) {
                float timeCapture = externalTimer.getTime();
                averageTick.push_back(timeCapture);
                if (averageTick.size() > 4) {
                    averageTick.erase(averageTick.begin());
                }
                externalTimer.reset();
            }
            if ((int)averageTick.size() > 0) {
                float runavg = 0.f;
                for (int i = 0; i < (int)averageTick.size(); ++i) {
                    runavg += averageTick[i];
                }
                runavg /= (float)averageTick.size();
                externalclockFreq = 1.f / runavg; // running avg of 4 clock pulses in Hz
            }

        }
    }

    void makePulses(float sampletime) {
        for (int i = 0; i < NUM_CLOCKS; ++i) {
            outputPulses[i].process(sampletime);

            if (phaseSet[i]) {
                outputPulses[i].trigger((1.f / (clocksFreq[i])) * pulseWidth[i]);
            }
        }
        for (int i = 0; i < NUM_MB_CLOCKS; ++i) {
            measBPulses[i].process(sampletime);

            if (measBset[i]) {
                measBPulses[i].trigger((1.f / (measBfreq[i])) * pulseWidth[i]);
            }
        }
    }
    bool getPulseClock(int index) {
        return outputPulses[index].isHigh();
    }
    bool getPulseMeasB(int index) {
        return measBPulses[index].isHigh();
    }
    float getMeasurePhase(int index, float shape) {
        float phase = clocksPhase[index] * _2PIto1;
        float shapemod = lerp(1, phase, 0, 1, shape);//same curving as dobbs
        float curve = (phase * shapemod) * 10.f;
        return curve;
    }
    float getFundFreq() {
        return clocksFreq[THRU_QUARTER];
    }
    void getTimeSig(int* beats, int* quaver) {
        *beats = timeSignatureBeats;
        *quaver = timeSignatureQuaver;
    }
};

struct ClockModule : Module
{
    enum ParamIds {
        GLOBAL_BPM_PARAM,
        SIG_BEATS_PARAM,
        SIG_QUAVER_PARAM,
        SWING_PARAM,
        CURVE_PARAM,
        PULSEWIDTH_PARAM,
        RESET_BUTTON_PARAM,
        RUN_BUTTON_PARAM,
        NUM_PARAMS
	};
    enum InputIds {
        BPM_CV_INPUT,
        EXT_GATE_INPUT,
        RESET_INPUT,
        RUN_INPUT,
        PULSEWIDTH_INPUT,
        NUM_INPUTS
	};
    enum OutputIds {
        RUN_TRIG_OUTPUT,
        RESET_TRIG_OUTPUT,
        THRU_OUTPUT,
        SIXTEENTH_OUTPUT,
        EIGHTH_OUTPUT,
        DOTTED_EIGHTH_OUTPUT,
        OFFBEAT_OUTPUT,
        TRIPLET_BEATS_OUTPUT,
        TRIPLET_MEASURE_OUTPUT,
        MID_MEASURE_OUTPUT,
        MEASURE_OUTPUT,
        FOUR_MEAS_OUTPUT,
        BEATS_MEAS_OUTPUT,
        SIXTEEN_MEAS_OUTPUT,
        BBQ_MEAS_OUTPUT,
        MID_MEASURE_PHASE_OUTPUT,
        MEASURE_PHASE_OUTPUT,
        FOUR_MEASURE_PHASE_OUTPUT,
        BEATS_MEASURE_PHASE_OUTPUT,
        SIXTEEN_MEASURE_PHASE_OUTPUT,
        BBQ_MEASURE_PHASE_OUTPUT,
        MEASB_DOTSIXTEENTH_OUTPUT,
        MEASB_DOTEIGHTH_OUTPUT,
        MEASB_HALFNOTE_OUTPUT,
        MEASB_TRIPLETQUART_OUTPUT,
        MEASB_DOTTEDQUART_OUTPUT,
        NUM_OUTPUTS
	};
	enum LightIds {
        RUN_LIGHT,
        ENUMS(PHASE_LIGHTS, 6),
        NUM_LIGHTS
    };

    BaseFunctions Functions;
    BaseButtons Buttons;
    BaseMatrices Matrix;
    ClockTree* clocks = new(ClockTree);

    #include "Theme/PanelVars.h"

    ClockModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(GLOBAL_BPM_PARAM, -4.f, 1.f, 0.f, "BPM");
        configParam(SIG_BEATS_PARAM, 2.f, 16.f, 4.f, "Beats per measure");
        paramQuantities[SIG_BEATS_PARAM]->snapEnabled = true;
        configParam(SIG_QUAVER_PARAM, 2.f, 16.f, 4.f, "Quaver type");
        paramQuantities[SIG_QUAVER_PARAM]->snapEnabled = true;
        configParam(SWING_PARAM, -1.f, 1.f, 0.f, "Swing"); 
        configParam(CURVE_PARAM, -0.99f, 0.99f, 0.f, "Curve");
        configParam(PULSEWIDTH_PARAM, 0.01f, 0.99f, 0.5f, "Pulse Width");
        configParam(RESET_BUTTON_PARAM, 0.f, 1.f, 0.f, "Reset");
        configParam(RUN_BUTTON_PARAM, 0.f, 1.f, 0.f, "Run");
        clocks->reset();

        configInput(BPM_CV_INPUT, "BPM CV");
        configInput(EXT_GATE_INPUT, "External Gate");
        configInput(RESET_INPUT, "Reset");
        configInput(RUN_INPUT, "Run");
        configInput(PULSEWIDTH_INPUT, "PulseWidth");
        configOutput(RUN_TRIG_OUTPUT, "Run Trigger");
        configOutput(RESET_TRIG_OUTPUT, "Reset Trigger");
        configOutput(THRU_OUTPUT, "Thru / Quarter Note");
        configOutput(SIXTEENTH_OUTPUT, "Sixteenth Note");
        configOutput(EIGHTH_OUTPUT, "Eighth Note");
        configOutput(DOTTED_EIGHTH_OUTPUT, "Dotted Eighth Note");
        configOutput(OFFBEAT_OUTPUT, "Offbeat Quarter Note");
        configOutput(TRIPLET_BEATS_OUTPUT, "Triplet Quaver");
        configOutput(TRIPLET_MEASURE_OUTPUT, "Triplet Measure");
        configOutput(MID_MEASURE_OUTPUT, "Mid-Measure");
        configOutput(MEASURE_OUTPUT, "Measure");
        configOutput(FOUR_MEAS_OUTPUT, "Four Measures");
        configOutput(BEATS_MEAS_OUTPUT, "Beats Measures");
        configOutput(SIXTEEN_MEAS_OUTPUT, "Sixteen Measures");
        configOutput(BBQ_MEAS_OUTPUT, "Beats by Quaver Measures");
        configOutput(MID_MEASURE_PHASE_OUTPUT, "Mid Measure Phase");
        configOutput(MEASURE_PHASE_OUTPUT, "Measure Phase");
        configOutput(FOUR_MEASURE_PHASE_OUTPUT, "Four Measure Phase");
        configOutput(BEATS_MEASURE_PHASE_OUTPUT, "Beats Measure Phase");
        configOutput(SIXTEEN_MEASURE_PHASE_OUTPUT, "Sixteen Measure Phase");
        configOutput(BBQ_MEASURE_PHASE_OUTPUT, "Beats by Quaver Measure Phase");
        configOutput(MEASB_DOTSIXTEENTH_OUTPUT, "Dotted Sixteenth Note (Measure-Bound)");
        configOutput(MEASB_DOTEIGHTH_OUTPUT, "Dotted Eighth Note (Measure - Bound)");
        configOutput(MEASB_HALFNOTE_OUTPUT, "Half Note (Measure - Bound)");
        configOutput(MEASB_TRIPLETQUART_OUTPUT, "Triplet Quaver (Measure - Bound)");
        configOutput(MEASB_DOTTEDQUART_OUTPUT, "Dotted Quarter Note (Measure - Bound)");
    }

    int currentPolyphony = 1;
    int currentBanks = 1;
    int loopCounter = 0;
    bool isCVin = false;
    bool isExtConnect = false;
    float timesigbeats = 4.f;
    float timesigquaver = 4.f;
    float phaseShape = 0.f;
    bool resetSet = false;
    bool resetReset = false;
    bool runSet = false;
    bool runReset = false;
    bool resetconnect = false;
    bool runconnect = false;
    bool isPWConnect = false;
    float phaseOut[6] = { 0.f };

    rack::dsp::BooleanTrigger _Run;
    rack::dsp::BooleanTrigger _Stop;
    rack::dsp::PulseGenerator _runPulse;
    rack::dsp::BooleanTrigger _Reset;
    rack::dsp::PulseGenerator _resetPulse;

    void process(const ProcessArgs& args) override {          
        if (loopCounter % 16 == 0) {
            setParams(args);
            doLights(args);
        }
        generateOutput(args);

        loopCounter++;
        if (loopCounter % 2520 == 0) {
            loopCounter = 0;
        }
    }

    void setParams(const ProcessArgs& args) {
        for (int o = RUN_TRIG_OUTPUT; o != NUM_OUTPUTS; ++o) {
            outputs[o].setChannels(1);
        }

        timesigbeats = params[SIG_BEATS_PARAM].value;
        timesigquaver = params[SIG_QUAVER_PARAM].value;
        clocks->setBPMparameter(params[GLOBAL_BPM_PARAM].value);
        clocks->TimeSignature(timesigbeats, timesigquaver);
        clocks->setSwing(params[SWING_PARAM].value);
        clocks->setPulseW(params[PULSEWIDTH_PARAM].value);
        phaseShape = params[CURVE_PARAM].value;
        isCVin = inputs[BPM_CV_INPUT].isConnected();
        isExtConnect = inputs[EXT_GATE_INPUT].isConnected();
        resetconnect = inputs[RESET_INPUT].isConnected();
        runconnect = inputs[RUN_INPUT].isConnected();
        isPWConnect = inputs[PULSEWIDTH_INPUT].isConnected();
        
        

        Buttons.latchButton(params[RUN_BUTTON_PARAM].value, &runSet, &runReset);
        if (runconnect) {
            Buttons.latchButton(inputs[RUN_INPUT].getVoltage(0) + 0.1f, &runSet, &runReset);

        }

    }

    void doLights(const ProcessArgs& args) {
        lights[RUN_LIGHT].setBrightness(runSet);
        for (int l = 0; l < 6; ++l) {
            lights[PHASE_LIGHTS + l].setBrightness(phaseOut[l] / 10.f);
    }
    
    }

    void generateOutput(const ProcessArgs& args) {
        if (resetconnect) {
            Buttons.momentButton(inputs[RESET_INPUT].getVoltage(0), &resetSet, &resetReset);
        }
        else {
            Buttons.momentButton(params[RESET_BUTTON_PARAM].value, &resetSet, &resetReset);
        }
        if (isPWConnect) {
            clocks->setPulseW(rack::math::clamp(params[PULSEWIDTH_PARAM].value + ((inputs[PULSEWIDTH_INPUT].getVoltage(0) / 5.f) ), 0.01, 0.99));
        }

        float BPMCVin = inputs[BPM_CV_INPUT].getVoltage(0);
        float ExtCVin = inputs[EXT_GATE_INPUT].getVoltage(0);
        clocks->setBPMinput(isCVin, BPMCVin);
        clocks->externalBPMgen(args.sampleTime, isExtConnect, ExtCVin);
        clocks->setfundFreq(); 

        _runPulse.process(args.sampleTime);
        _resetPulse.process(args.sampleTime);
        
        bool isRun = _Run.process(runSet);
        bool isStop = _Stop.process(!runSet);
        if (isRun || isStop) {
            _runPulse.trigger(0.08);
        }

        float clocksOut[13] = { 0.f };
        float measBout[6] = { 0.f };

        if (runSet) {
            clocks->makePulses(args.sampleTime);
            clocks->phaseAccum(args.sampleRate);

            for (int i = 0; i < 13; ++i) {
                clocksOut[i] = clocks->getPulseClock(i) * 10.f;
            }
            for (int i = 0; i < 6; ++i) {
                measBout[i] = clocks->getPulseMeasB(i) * 10.f;
                phaseOut[i] = clocks->getMeasurePhase(i + 7, phaseShape);
            }
        }
        else {
            //clocks->reset();
            for (int i = 0; i < 13; ++i) {
                clocksOut[i] = 0.f;
            }
            for (int i = 0; i < 6; ++i) {
                measBout[i] = 0.f;
            }
            resetSet = true;
        }

        bool isReset = _Reset.process(resetSet);
        if (isReset) {
            clocks->reset();
            _resetPulse.trigger(0.08);
        }
        float runtrig = (_runPulse.isHigh()) * 10.f;
        float resetrig = (_resetPulse.isHigh()) * 10.f;
        outputs[RUN_TRIG_OUTPUT].setVoltage(runtrig, 0);
        outputs[RESET_TRIG_OUTPUT].setVoltage(resetrig, 0);

        outputs[THRU_OUTPUT].setVoltage(clocksOut[0], 0);
        outputs[SIXTEENTH_OUTPUT].setVoltage(clocksOut[1], 0);
        outputs[EIGHTH_OUTPUT].setVoltage(clocksOut[2], 0);
        outputs[DOTTED_EIGHTH_OUTPUT].setVoltage(clocksOut[3], 0);
        outputs[OFFBEAT_OUTPUT].setVoltage(clocksOut[4], 0);
        outputs[TRIPLET_BEATS_OUTPUT].setVoltage(clocksOut[5], 0);
        outputs[TRIPLET_MEASURE_OUTPUT].setVoltage(clocksOut[6], 0);
        outputs[MID_MEASURE_OUTPUT].setVoltage(clocksOut[7], 0);
        outputs[MEASURE_OUTPUT].setVoltage(clocksOut[8], 0);
        outputs[FOUR_MEAS_OUTPUT].setVoltage(clocksOut[9], 0);
        outputs[BEATS_MEAS_OUTPUT].setVoltage(clocksOut[10], 0);
        outputs[SIXTEEN_MEAS_OUTPUT].setVoltage(clocksOut[11], 0);
        outputs[BBQ_MEAS_OUTPUT].setVoltage(clocksOut[12], 0);

        
        outputs[MEASB_DOTSIXTEENTH_OUTPUT].setVoltage(measBout[0], 0);
        outputs[MEASB_DOTEIGHTH_OUTPUT].setVoltage(measBout[1], 0);
        outputs[MEASB_TRIPLETQUART_OUTPUT].setVoltage(measBout[2], 0);
        outputs[MEASB_DOTTEDQUART_OUTPUT].setVoltage(measBout[3], 0);
        outputs[MEASB_HALFNOTE_OUTPUT].setVoltage(measBout[4], 0);

        outputs[MID_MEASURE_PHASE_OUTPUT].setVoltage(phaseOut[0], 0);
        outputs[MEASURE_PHASE_OUTPUT].setVoltage(phaseOut[1], 0);
        outputs[FOUR_MEASURE_PHASE_OUTPUT].setVoltage(phaseOut[2], 0);
        outputs[BEATS_MEASURE_PHASE_OUTPUT].setVoltage(phaseOut[3], 0);
        outputs[SIXTEEN_MEASURE_PHASE_OUTPUT].setVoltage(phaseOut[4], 0);
        outputs[BBQ_MEASURE_PHASE_OUTPUT].setVoltage(phaseOut[5], 0);

    }

    json_t* dataToJson() override {
        json_t* rootJ = json_object();
        json_t* panelJ = json_integer(currPanel);
        json_t* RunningJ = json_boolean(runSet);
        json_object_set_new(rootJ, "Panel", panelJ);
        json_object_set_new(rootJ, "Running", RunningJ);

        return rootJ;
    }

    void dataFromJson(json_t* rootJ) override {
        json_t* panelJ = json_object_get(rootJ, "Panel");
        if (panelJ) currPanel = json_integer_value(panelJ);
        json_t* RunningJ = json_object_get(rootJ, "Running");
        if (RunningJ) {
            runSet = json_boolean_value(RunningJ);

            clocks->reset();
        }
    }

};




struct ClockDisplay : DigitalDisplay {
    ClockDisplay() {
        fontPath = asset::plugin(pluginInstance, "res/fonts/DSEG7ClassicMini-Bold.ttf");
        textPos = Vec(box.size.x - 2, box.size.y - 2);
        bgText = " ";
        fontSize = 16;
    }
    const NVGcolor BLOO = nvgRGB(0x00, 0xaa, 0xff);
};



struct ClockWidget : ClockDisplay {
    
    ClockModule* module;

    void step() override {
        int tempo = 120;
        if (module) {
            switch (module->currPanel) {
            case 0: {
                fgColor = rack::color::CYAN;
                break;
            }
            case 1: {
                fgColor = BLOO;
                break;
            }
            case 2: {
                fgColor = rack::color::MAGENTA;
                break;
            }
            case 3: {
                fgColor = rack::color::RED;
                break;
            }
            case 4: {
                fgColor = rack::color::YELLOW;
                break;
            }
            case 5: {
                fgColor = rack::color::YELLOW;
                break;
            }
            }

            tempo = module->clocks->getFundFreq() * 60;
        }
        text = rack::string::f("%d", tempo);
    }
};
struct TimeSignatureWidget : ClockDisplay {

    ClockModule* module;

    void step() override {
        int beats = 4;
        int quaver = 4;
        if (module) {
            module->clocks->getTimeSig(&beats, &quaver);
            switch (module->currPanel) {
            case 0: {
                fgColor = rack::color::CYAN;
                break;
            }
            case 1: {
                fgColor = BLOO;
                break;
            }
            case 2: {
                fgColor = rack::color::MAGENTA;
                break;
            }
            case 3: {
                fgColor = rack::color::RED;
                break;
            }
            case 4: {
                fgColor = rack::color::YELLOW;
                break;
            }
            case 5: {
                fgColor = rack::color::YELLOW;
                break;
            }
            }
        }
        int quavset = (int)((1.f / (float)quaver) * 16.f);
        text = rack::string::f("%d | %d", beats, quavset);
    }
};

struct ClockPanelWidget : ModuleWidget {

    #include "Theme/LogoLight.h"
    std::string panel;

    ClockPanelWidget(ClockModule* module) {

        setModule(module);

        panel = PANEL;
        //set panel on init
        #include "Theme/initChoosePanel.h"

		addChild(createWidget<ScrewSilver>(Vec(15, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 30, 0)));
		addChild(createWidget<ScrewSilver>(Vec(15, 365)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 30, 365)));

        addChild(createLightCentered<SmallLight<BlueLight>>(Vec(49, 93), module, ClockModule::RUN_LIGHT));
        addChild(createLightCentered<TinyLight<BlueLight>>(Vec(104.7, 306.5), module, ClockModule::PHASE_LIGHTS + 0));
        addChild(createLightCentered<TinyLight<BlueLight>>(Vec(133.7, 306.5), module, ClockModule::PHASE_LIGHTS + 1));
        addChild(createLightCentered<TinyLight<BlueLight>>(Vec(110.3, 332.3), module, ClockModule::PHASE_LIGHTS + 2));
        addChild(createLightCentered<TinyLight<BlueLight>>(Vec(139.3, 332.3), module, ClockModule::PHASE_LIGHTS + 3));
        addChild(createLightCentered<TinyLight<BlueLight>>(Vec(117.6, 358.4), module, ClockModule::PHASE_LIGHTS + 4));
        addChild(createLightCentered<TinyLight<BlueLight>>(Vec(145.6, 358.4), module, ClockModule::PHASE_LIGHTS + 5));

        addParam(createParam<RoundLargeBlackKnob>(Vec(57.5, 133.5), module, ClockModule::GLOBAL_BPM_PARAM));
        addParam(createParam<RoundBlackKnob>(Vec(14,140), module, ClockModule::SIG_BEATS_PARAM));
        addParam(createParam<RoundBlackKnob>(Vec(110, 140), module, ClockModule::SIG_QUAVER_PARAM));
        addParam(createParam<Trimpot>(Vec(66.5, 263), module, ClockModule::SWING_PARAM));
        addParam(createParam<Trimpot>(Vec(66.5, 325), module, ClockModule::CURVE_PARAM));
        addParam(createParam<RoundBlackKnob>(Vec(86, 176), module, ClockModule::PULSEWIDTH_PARAM));
        addParam(createParam<VCVButton>(Vec(121, 105), module, ClockModule::RESET_BUTTON_PARAM));
        addParam(createParam<VCVButton>(Vec(58, 76), module, ClockModule::RUN_BUTTON_PARAM));

        addInput(createInput<PurplePort>(Vec(10, 64), module, ClockModule::BPM_CV_INPUT));
        addInput(createInput<PurplePort>(Vec(10, 95), module, ClockModule::EXT_GATE_INPUT));
        addInput(createInput<PurplePort>(Vec(88, 75), module, ClockModule::RESET_INPUT));
        addInput(createInput<PurplePort>(Vec(56, 100), module, ClockModule::RUN_INPUT));
        addInput(createInput<PurplePort>(Vec(116, 184), module, ClockModule::PULSEWIDTH_INPUT));

        addOutput(createOutput<PurplePort>(Vec(88, 100), module, ClockModule::RUN_TRIG_OUTPUT));
        addOutput(createOutput<PurplePort>(Vec(118, 75), module, ClockModule::RESET_TRIG_OUTPUT));
        addOutput(createOutput<PurplePort>(Vec(10, 184), module, ClockModule::THRU_OUTPUT));

        addOutput(createOutput<PurplePort>(Vec(21.792, 209), module, ClockModule::SIXTEENTH_OUTPUT));
        addOutput(createOutput<PurplePort>(Vec(49.979, 209), module, ClockModule::EIGHTH_OUTPUT));
        addOutput(createOutput<PurplePort>(Vec(13.803, 234.791), module, ClockModule::DOTTED_EIGHTH_OUTPUT));
        addOutput(createOutput<PurplePort>(Vec(41.989, 234.791), module, ClockModule::OFFBEAT_OUTPUT));
        addOutput(createOutput<PurplePort>(Vec(5.814, 260.582), module, ClockModule::TRIPLET_BEATS_OUTPUT));
        addOutput(createOutput<PurplePort>(Vec(34, 260.582), module, ClockModule::TRIPLET_MEASURE_OUTPUT));
        addOutput(createOutput<PurplePort>(Vec(78, 209), module, ClockModule::MID_MEASURE_OUTPUT));
        addOutput(createOutput<PurplePort>(Vec(106.186, 209), module, ClockModule::MEASURE_OUTPUT));
        addOutput(createOutput<PurplePort>(Vec(85.989, 234.791), module, ClockModule::FOUR_MEAS_OUTPUT));
        addOutput(createOutput<PurplePort>(Vec(114.176, 234.791), module, ClockModule::BEATS_MEAS_OUTPUT));
        addOutput(createOutput<PurplePort>(Vec(93.979, 260.582), module, ClockModule::SIXTEEN_MEAS_OUTPUT));
        addOutput(createOutput<PurplePort>(Vec(122.165, 260.582), module, ClockModule::BBQ_MEAS_OUTPUT));

        addOutput(createOutput<PurplePort>(Vec(20.791, 286.945), module, ClockModule::MEASB_DOTSIXTEENTH_OUTPUT));
        addOutput(createOutput<PurplePort>(Vec(48.977, 286.945), module, ClockModule::MEASB_DOTEIGHTH_OUTPUT));
        addOutput(createOutput<PurplePort>(Vec(13.803, 313.025), module, ClockModule::MEASB_TRIPLETQUART_OUTPUT));
        addOutput(createOutput<PurplePort>(Vec(41.989, 313.025), module, ClockModule::MEASB_DOTTEDQUART_OUTPUT));
        addOutput(createOutput<PurplePort>(Vec(6.815, 339.105), module, ClockModule::MEASB_HALFNOTE_OUTPUT));


        addOutput(createOutput<PurplePort>(Vec(79.001, 286.945), module, ClockModule::MID_MEASURE_PHASE_OUTPUT));
        addOutput(createOutput<PurplePort>(Vec(107.187, 286.945), module, ClockModule::MEASURE_PHASE_OUTPUT));
        addOutput(createOutput<PurplePort>(Vec(85.989, 313.025), module, ClockModule::FOUR_MEASURE_PHASE_OUTPUT));
        addOutput(createOutput<PurplePort>(Vec(114.175, 313.025), module, ClockModule::BEATS_MEASURE_PHASE_OUTPUT));
        addOutput(createOutput<PurplePort>(Vec(92.977, 339.105), module, ClockModule::SIXTEEN_MEASURE_PHASE_OUTPUT));
        addOutput(createOutput<PurplePort>(Vec(121.164, 339.105), module, ClockModule::BBQ_MEASURE_PHASE_OUTPUT));


        
        if (module) {
            
            ClockWidget* ClkWidget = createWidget<ClockWidget>((Vec(20, 30)));
            ClkWidget->box.size = (Vec(50, 27));
            ClkWidget->textPos = (Vec(47, 25));
            ClkWidget->module = module;
            addChild(ClkWidget);

            TimeSignatureWidget* TSWidget = createWidget<TimeSignatureWidget>((Vec(80, 30)));
            TSWidget->box.size = (Vec(50, 27));
            TSWidget->textPos = (Vec(49, 25));
            TSWidget->module = module;
            addChild(TSWidget);

            //half of hp(in px) - half logo width, near bottom. 
            Vec logoPos = Vec(((15.f * 10.f) / 2.f) - 22.5, 363.f);
            ClockModule* module = dynamic_cast<ClockModule*>(this->module);
            assert(module);
            #include "Theme/LogoChild.h"
        }

    }

    //give struct to menu containing panel options
    #include "Theme/PanelList.h" 

    void appendContextMenu(Menu* menu) override {
        ClockModule* module = dynamic_cast<ClockModule*>(this->module);
        assert(module);

        #include "Theme/CreatePanelMenu.h"
    }

    void step() override {
        if (module) {
            //change panel 
            #include "Theme/UpdatePanel.h"
        }
        Widget::step();
    }

};

Model* modelClock = createModel<ClockModule, ClockPanelWidget>("Clock-Long");