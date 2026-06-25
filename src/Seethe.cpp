#include "plugin.hpp"

#include "Lydapi/LydFFT.h"
#define MODULE_NAME SeetheModule
#define PANEL "Seethe_panel.svg"
#define HP 12


static const int maxPolyphony = 1;

using namespace LydD;

//6th order approx of e^x
//follows negative portion of sigmoid great, falls off above about +1.5
//since that uses e^(-x) it follows the inverse is true for positive powers
template<typename T = float>
T EulerToPower(T x) {
    const T coeffs[6] = { 1.f, 0.5f, 0.166666667f, 0.041666667f, 0.008333333f, 0.0013888889f };
    T order[6];
    for (int i = 0; i < 6; ++i) {
        T xpow = x;
        int k = 0;
        //ratchet up powers of x
        while (k < i) {
            xpow *= x;
            k++;
        }
        order[i] = xpow * coeffs[i];
    }
    return 1.f + order[0] + order[1] + order[2] + order[3] + order[4] + order[5];
}

template<typename T = float, int L = 1024>
struct SaturateCurve {
    enum CType {
        FULL,
        EVEN_HW, //half wave rectify
        EVEN_FW, //full wave rectify
        SIN,
        NUM_CURVES
    };

    T CurveArray[L];
    T sigmoid(T x) {
        return 1.f / (1.f + EulerToPower(-x));
    }
    //run once at init, provided as constructor
    void preComputeCurve(CType curve = FULL) {
        switch (curve) {
        default: {}
        case FULL: {
            for (int i = 0; i < L / 2; ++i) {
                //create phase from -5 to +5
                T simInput = ((float)i / (float)L) * 10.f - 5.f;
                //my sigmoid approx is only good for negative inputs, luckily its symmetric
                CurveArray[i] = sigmoid(simInput) * 2.f - 1.f;
                CurveArray[L - i - 1] = -CurveArray[i];
            }
            break;
        }
        case EVEN_HW: {
            for (int i = 0; i < L / 2; ++i) {
                //create phase from -5 to +5
                T simInput = ((float)i / (float)L) * 10.f - 5.f;
                //the half wave version of even saturatio just cuts off negative voltages. harsh
                CurveArray[i] = 0;
                CurveArray[L - i - 1] = -(sigmoid(simInput) * 2.f - 1.f);
            }
            break;
        }
        case EVEN_FW: {
            for (int i = 0; i < L / 2; ++i) {
                //create phase from -5 to +5
                T simInput = ((float)i / (float)L) * 10.f - 5.f;
                T sigm = sigmoid(simInput) * 2.f - 1.f;
                //full wave even is like bouncy ball
                CurveArray[i] = -sigm;
                CurveArray[L - i - 1] = -sigm;
            }
            break;
        }
        case SIN: {
            for (int i = 0; i < L; ++i) {
                //create phase from -5 to +5

                T simInput = ((float)i / (float)L) * 10.f - 5.f;
                T sigmee = sigmoid(simInput) * 2.f - 1.f;

                T sigm = rack::simd::sin(simInput / 5.f * _2_PI) / 2.f;
                sigm -= sigmee;
                //full wave even is like bouncy ball
                CurveArray[i] = sigm;
                //CurveArray[L - i - 1] = -sigm;
            }
            break;
        }
        }
    }
    SaturateCurve() {
        preComputeCurve();
    }
    SaturateCurve(CType curve) {
        preComputeCurve(curve);
    }

    T getCurveDirect(int frame) {
        int pt = frame % L;
        return CurveArray[pt];
    }

    //maps input in range +-5v to location on curvePar that ranges +-1
    //inputs beyond +-5v are capped
    
    T applyCurve(T in, T tension) {
        int center = L / 2;
        //0v = center, 5v = L, -5v = 0
        T inasdex = (in * (center / 5.f)) + center;
        inasdex = rack::math::clamp(inasdex, (T)0, T(L - 1));
        size_t dex = std::floor(inasdex);
        float frac = inasdex - dex;
        T curve = rack::math::crossfade(CurveArray[dex], CurveArray[dex + 1],  frac);
        curve = normalCurve(-1.f, 1.f, curve, tension);
        //scale back from curvePar to input range
        curve *= 5.f;
        return curve;
    }
};

struct SeetheModule : Module
{

    enum ParamIds {
        ENUMS(DRIVE_PARAM, 3),
        ENUMS(MORPH_PARAM, 3),
        ENUMS(CURVE_PARAM, 3),
        ENUMS(DRY_WET_PARAM, 3),
        NUM_PARAMS
    };
    enum InputIds {
        AUDIO_INPUT,
        ENUMS(BAND_INPUT, 3),
        ENUMS(DRIVE_INPUT, 3),
        ENUMS(MORPH_INPUT, 3),
        ENUMS(CURVE_INPUT, 3),
        ENUMS(DRY_WET_INPUT, 3),
        NUM_INPUTS
    };
    enum OutputIds {
        AUDIO_OUTPUT,
        ENUMS(BAND_OUTPUT, 3),
        TEST1_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {

        NUM_LIGHTS
    };

    int loopCounter = 0;

    SaturateCurve<float, 1024> _sigma[3];
    rack::dsp::TBiquadFilter<float> _bands[3];
    rack::dsp::TRCFilter<float> _DCRem;
    rack::dsp::TRCFilter<float> _BandDCRem[3];

    float drivePar[3] = { 0, 0, 0 };
    float morphPar[3] = { 0, 0, 0 };
    float curvePar[3] = { 0, 0, 0 };
    float mixPar[3] = { 0, 0, 0 };

    float bandFreq[3] = { 0, 0, 0 };
    float bandZeroFreq[3] = { 450.f, 850.f, 1500.f };
    bool isinGlobal;
    bool isBandin[3];
    bool isCVin[12]; //cv inputs in 3s- drive, morph, curve, mix

    SeetheModule() {

        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        for (int i = 0; i < 3; ++i) {
            configParam(DRIVE_PARAM + i, 0.1f, 10.f, 1.f, "Drive");
            configParam(MORPH_PARAM + i, 0.f, 2.f, 0.f, "Morph");
            configParam(CURVE_PARAM + i, -1.f, 1.f, 0.f, "Curve");
            configParam(DRY_WET_PARAM + i, 0.f, 1.f, 0.f, "Dry/Wet");
        }

        _sigma[0].preComputeCurve(_sigma[0].FULL);
        _sigma[1].preComputeCurve(_sigma[1].EVEN_FW);
        _sigma[2].preComputeCurve(_sigma[2].SIN);

        _bands[0].setParameters(_bands[0].LOWPASS, 0.0181f, 4.f, 1.f);
        _bands[1].setParameters(_bands[1].BANDPASS, 0.034f, 2.f, 1.f);
        _bands[2].setParameters(_bands[2].HIGHPASS, 0.0476f, 4.f, 1.f);
    }
    



    void process(const ProcessArgs& args) override {
        if (loopCounter % 8 == 0) {
            checkInputs(args);
            setParams(args);
        }
        processSaturate(args);
        loopCounter++;
        if (loopCounter % 1024 == 0) {
            loopCounter = 0;
        }
    }
    void checkInputs(const ProcessArgs& args) {
        isinGlobal = inputs[AUDIO_INPUT].isConnected();
        for (int i = 0; i < 3; ++i) {
            isBandin[i] = inputs[BAND_INPUT + i].isConnected();
            isCVin[i] = inputs[DRIVE_INPUT + i].isConnected();
            isCVin[i + 3] = inputs[MORPH_INPUT + i].isConnected();
            isCVin[i + 6] = inputs[CURVE_INPUT + i].isConnected();
            isCVin[i + 9] = inputs[DRY_WET_INPUT + i].isConnected();
        }
    
    }

    void setParams(const ProcessArgs& args) {
        _DCRem.setCutoffFreq(15.2f / args.sampleRate);
        for (int p = 0; p < 3; ++p) {
            _BandDCRem[p].setCutoffFreq(15.2f / args.sampleRate);

            drivePar[p] = params[DRIVE_PARAM + p].value;
            morphPar[p] = params[MORPH_PARAM + p].value;
            curvePar[p] = params[CURVE_PARAM + p].value;
            mixPar[p] = params[DRY_WET_PARAM + p].value;

        }

        _bands[0].setParameters(_bands[0].LOWPASS, bandFreq[0], 4.f, 1.2f);
        _bands[1].setParameters(_bands[1].BANDPASS, bandFreq[1], 2.f, 1.f);
        _bands[2].setParameters(_bands[2].HIGHPASS, bandFreq[2], 2.f, 1.f);

    }

    void processSaturate(const ProcessArgs& args) {
        float dry = isinGlobal ? inputs[AUDIO_INPUT].getVoltage(0) : 0.f;
        //DCRem.process(dry);
        //dry = DCRem.highpass();
        float dryband[3];
        float drive[3];
        float curve[3];
        float mix[3];
        int morphidx[3];
        float morph[3];
        for (int m = 0; m < 3; ++m) {
            //each band input goes only to its band
            dryband[m] = isBandin[m] ? inputs[BAND_INPUT + m].getVoltage(0) : 0.f;
           

            drive[m] = isCVin[m] ? inputs[DRIVE_INPUT + m].getVoltage(0) / 2.f : 0.f;
            drive[m] = rack::math::clamp(drivePar[m] + drive[m], 0.1f, 10.f);

            morph[m] = isCVin[m + 3] ? inputs[MORPH_INPUT + m].getVoltage(0) / 5.f : 0.f;
            morph[m] = rack::math::clamp(morphPar[m] + morph[m], 0.f, 2.f);
            morphidx[m] = std::floor(morph[m]);
            morph[m] = morph[m] - morphidx[m];

            curve[m] = isCVin[m + 6] ? inputs[CURVE_INPUT + m].getVoltage(0) / 5.f : 0.f;
            curve[m] = rack::math::clamp(curvePar[m] + curve[m], -1.f, 1.f);
            bandFreq[m] = VoltToFreq(curve[m] * 2.f, 0.f, bandZeroFreq[m]);
            bandFreq[m] /= args.sampleRate;

            mix[m] = isCVin[m + 9] ? inputs[DRY_WET_INPUT + m].getVoltage(0) / 5.f : 0.f;
            mix[m] = rack::math::clamp(mixPar[m] + mix[m], 0.f, 1.f);


        }
        //saturate each band
        float saturated[3];
        for (int s = 0; s < 3; ++s) {
            float satur = _bands[s].process(dry + dryband[s]);
            satur *= drive[s];

            float mapped[3];
            for (int i = 0; i < 3; ++i) {
                mapped[i] = _sigma[i].applyCurve(satur, curve[s]);
            }
            saturated[s] = rack::math::crossfade(mapped[morphidx[s]], mapped[(morphidx[s] + 1) % 3], morph[s]);
            //this process can in even curves produce an unwanted DC offset. get rid of it
            _BandDCRem[s].process(saturated[s]);
            saturated[s] = _BandDCRem[s].highpass();
            saturated[s] = rack::math::crossfade(dry + dryband[s], saturated[s], mix[s]);
            outputs[BAND_OUTPUT + s].setVoltage(saturated[s], 0);
        }
        float satgroup = (saturated[0] + saturated[1] + saturated[2]) / 3.f;
        outputs[AUDIO_OUTPUT].setVoltage(satgroup, 0);

        //take a look at the actual curvePar produced over time
        float testcurves[3];
        for (int i = 0; i < 3; ++i) {
            testcurves[i] = _sigma[i].getCurveDirect(loopCounter);
            testcurves[i] = normalCurve(-1.f, 1.f, testcurves[i], curve[0]);
        }
        float tcur = rack::math::crossfade(testcurves[morphidx[0]], testcurves[(morphidx[0] + 1) % 3], morph[0]);
        outputs[TEST1_OUTPUT].setVoltage(tcur, 0);
      
    }


    json_t* dataToJson() override {
        json_t* rootJ = json_object();

        return rootJ;
    }

    void dataFromJson(json_t* rootJ) override {

    }

};


using namespace LydD::Components;
struct SeethePanelWidget : ModuleWidget {

    //include struct for logo here so it has modules name
    #include "Theme/LogoLight.h"

    SeethePanelWidget(SeetheModule* module) {
        setModule(module);
        LydD::Components::setPlugin(pluginInstance);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Seethe_panel.svg")));

		addChild(createWidget<ScrewSilver>(Vec(15, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 30, 0)));
		addChild(createWidget<ScrewSilver>(Vec(15, 365)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 30, 365)));

        float pY[4] = { 41.252f, 97.926f, 142.23f, 182.415f };
        float dX[3] = { 9.077f, 72.f, 133.396f };
        float mX[3] = { 13.773f, 75.826f,  137.222f };
        float cdwX[3] = { 16.61f, 77.779f, 138.954f };
        for (int p = 0; p < 3; ++p) {
            addParam(createParam<RoundLargeBlackKnob>(Vec(dX[p], pY[0]), module, SeetheModule::DRIVE_PARAM + p));
            addParam(createParam<RoundBlackKnob>(Vec(mX[p], pY[1]), module, SeetheModule::MORPH_PARAM + p));
            addParam(createParam<RoundSmallBlackKnob>(Vec(cdwX[p], pY[2]), module, SeetheModule::CURVE_PARAM + p));
            addParam(createParam<RoundSmallBlackKnob>(Vec(cdwX[p], pY[3]), module, SeetheModule::DRY_WET_PARAM + p));
        }

        float cvY[4] = {217.67f, 233.886f, 255.308f, 271.523f};
        float cvPX1[3] = {5.943f, 63.125f, 122.329f};
        float cvPX2[3] = {33.094f, 92.727f, 151.931f};
        float bandX[3] = {42.121f, 75.013f, 107.906};
        float ioY[2] = { 304.766 , 337.813 };

        for (int i = 0; i < 3; ++i) {
            addInput(createInput<PurplePort>(Vec(cvPX1[i], cvY[0]), module, SeetheModule::DRIVE_INPUT + i));
            addInput(createInput<PurplePort>(Vec(cvPX2[i], cvY[1]), module, SeetheModule::MORPH_INPUT + i));
            addInput(createInput<PurplePort>(Vec(cvPX1[i], cvY[2]), module, SeetheModule::CURVE_INPUT + i));
            addInput(createInput<PurplePort>(Vec(cvPX2[i], cvY[3]), module, SeetheModule::DRY_WET_INPUT + i));

        }

        addInput(createInput<PurplePort>(Vec(10.133, ioY[0]), module, SeetheModule::AUDIO_INPUT));
        addOutput(createOutput<PurplePort>(Vec(144.830, ioY[1]), module, SeetheModule::AUDIO_OUTPUT));

        for (int o = 0; o < 3; ++o) {
            addInput(createInput<PurplePort>(Vec(bandX[o], ioY[0]), module, SeetheModule::BAND_INPUT + o));
            addOutput(createOutput<PurplePort>(Vec(bandX[o], ioY[1]), module, SeetheModule::BAND_OUTPUT + o));
        }

        addOutput(createOutput<PurplePort>(Vec(130, 338), module, SeetheModule::TEST1_OUTPUT));

        if (module) {
            

            //must be called 'logoPos'for all modules 
            Vec logoPos = Vec(((15.f * HP) / 2.f) - 12.5f, 363.f);
            SeetheModule* module = dynamic_cast<SeetheModule*>(this->module);
            assert(module);
            #include "Theme/LogoChild.h"
            
        }

    }

};

Model* modelSeethe = createModel<SeetheModule, SeethePanelWidget>("Seethe-Distort");