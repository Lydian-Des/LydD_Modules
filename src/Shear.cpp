#include "plugin.hpp"
#include <vector>
#include <string>


static const int maxPolyphony = 1;


struct ShearModule : Module
{
    enum ParamIds {
        CUTOFF_PARAM,
        RESONANCE_PARAM,
        FEEDBACK_PARAM,
        EVEN_ODD_PARAM,
        SKEW_PARAM,
        NUM_PARAMS
	};
    enum InputIds {

        AUDIOLEFT_INPUT,
        AUDIORIGHT_INPUT,
        CUTOFF_INPUT,
        RESONANCE_INPUT,
        FEEDBACK_INPUT,
        FEEDLEFT_RETURN_INPUT,
        FEEDRIGHT_RETURN_INPUT,
        EVEN_ODD_INPUT,
        NUM_INPUTS
	};
    enum OutputIds {

        AUDIOLEFT_OUTPUT,
        AUDIORIGHT_OUTPUT,
        FEEDLEFT_SEND_OUTPUT,
        FEEDRIGHT_SEND_OUTPUT,
        NUM_OUTPUTS
	};
	enum LightIds {
        NUM_LIGHTS
    };

    BaseFunctions Functions;
    BaseButtons Buttons;


    int currentPolyphony = 1;
    int currentBanks = 1;
    int loopCounter = 0;
    bool isExternalFeedL = false;
    bool isExternalFeedR = false;
    bool isinLeft = false;
    bool isinRight = false;

    float cutoff = 0.f;
    float resonance = 0.f;
    float EvenOdd = 0.f;
    float feedback = 0.f;
    float skew = 0.f;
    float combSumLeft = 0;
    float combSumRight = 0;
    float BandsLeft[12] = { 0.f };
    float BandsRight[12] = { 0.f };
    float visData[6] = { 0.f };

    //thank goud for these little guys
    rack::dsp::BiquadFilter _CombL[12];
    rack::dsp::BiquadFilter _CombR[12];

    ShearModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(CUTOFF_PARAM, -3.f, 3.f, 0.f, "Cutoff");
        configParam(RESONANCE_PARAM, 0.f, 0.99f, 0.f, "Resonance");
        configParam(FEEDBACK_PARAM, 0.f, 1.f, 0.f, "Feedback");
        configParam(EVEN_ODD_PARAM, -1.f, 1.f, 0.f, "Even / Odd");
        configParam(SKEW_PARAM, -1.f, 1.f, 0.f, "Skew");
        for (int i = 0; i < 12; ++i) {
            _CombL[i].reset();
            _CombR[i].reset();
        }
    }

    

    void process(const ProcessArgs& args) override {
  
        if (loopCounter % 4 == 0) {
            checkInputs(args);          
        }

        generateOutput(args);

        loopCounter++;
        if (loopCounter % 2520 == 0) {
            loopCounter = 0;
        }
    }


    void checkInputs(const ProcessArgs& args) {
        for (int o = AUDIOLEFT_OUTPUT; o != NUM_OUTPUTS; ++o) {
            outputs[o].setChannels(1);
        }

        float cutpar = params[CUTOFF_PARAM].value;
        float cutin = (inputs[CUTOFF_INPUT].isConnected()) ? rack::math::clamp(inputs[CUTOFF_INPUT].getVoltage(0), -3.f, 3.f) : 0.f;
        
        cutoff = (Functions.VoltToFreq(cutpar + cutin, 0.f, 130.8125)) / args.sampleRate;
        cutoff = rack::math::clamp(cutoff, 0.f, 0.49f / 12.f); //absolute limit for filter is 0.5, and I have 12 bands up the spectrum 
        
        float respar = params[RESONANCE_PARAM].value;
        float resin = (inputs[RESONANCE_INPUT].isConnected()) ? rack::math::clamp(abs(inputs[RESONANCE_INPUT].getVoltage(0) / 10.f), 0.f, 0.9f) : 1.f;
        resonance = respar * resin;

        float evenoddpar = params[EVEN_ODD_PARAM].value;
        float evenoddin = inputs[EVEN_ODD_INPUT].isConnected() ? rack::math::clamp(inputs[EVEN_ODD_INPUT].getVoltage(0) / 5.f, -1.f, 1.f) : 1.f;
        EvenOdd = evenoddpar * evenoddin;

        skew = params[SKEW_PARAM].value;

        float Q = (resonance * 45) + 4.f;
        _CombL[0].setParameters(_CombL[0].BANDPASS, cutoff, Q, 1.2f);
        _CombR[0].setParameters(_CombR[0].BANDPASS, cutoff, Q, 1.2f);
        
        for (int i = 1; i < 12; ++i) {
            float cutoffClamp = rack::math::clamp(cutoff * (i + 1.f + (skew / 2.f)), 0.f, 0.499f);
            _CombL[i].setParameters(_CombL[i].BANDPASS, cutoffClamp, Q, (resonance * 2.f + 0.4) * (1.f / (i + 1)));
            _CombR[i].setParameters(_CombR[i].BANDPASS, cutoffClamp, Q, (resonance * 2.f + 0.4) * (1.f / (i + 1)));
        }

        float feedbackpar = params[FEEDBACK_PARAM].value;
        float feedbackin = inputs[FEEDBACK_INPUT].isConnected() ? inputs[FEEDBACK_INPUT].getVoltage(0) : 1.f;
        feedback = (feedbackpar * feedbackin) / 2.f;

        isExternalFeedL = outputs[FEEDLEFT_SEND_OUTPUT].isConnected() && inputs[FEEDLEFT_RETURN_INPUT].isConnected();
        isExternalFeedR = outputs[FEEDRIGHT_SEND_OUTPUT].isConnected() && inputs[FEEDRIGHT_RETURN_INPUT].isConnected();
        isinLeft = inputs[AUDIOLEFT_INPUT].isConnected();
        isinRight = inputs[AUDIORIGHT_INPUT].isConnected();
    }
    //tweaked vcv fundamental vcf drive with a bit of an inverse relationship to amplitude 
    float driveClamp(float wave) {
        wave *= pow(1 / (abs(wave) + 1.f), 3) + 2.f; //force down *less gain* if wave is high amplitude
        wave *= (21.f + wave * wave) / (21.f + 3.f * wave * wave);
        wave *= 2.5f;
        wave = rack::math::clamp(wave, -(5.f), 5.f);
        return wave;
    }

    void generateOutput(const ProcessArgs& args) {
       
               
        float audinLeft = 0.f;
        float audinRight = 0.f;
        //cascade normal and add feedback in
        if (isinLeft) {
            audinLeft = inputs[AUDIOLEFT_INPUT].getVoltage(0) / 2.5f;
            audinLeft += (feedback * 0.99f * combSumLeft);
            if (isExternalFeedL) {
                float feedloopL = inputs[FEEDLEFT_RETURN_INPUT].getVoltage(0) / 2.5f;
                audinLeft += (feedback * 0.99f * feedloopL);
            }
        
        }
        if (isinRight) {
            audinRight = inputs[AUDIORIGHT_INPUT].getVoltage(0) / 2.5f;
            audinRight += (feedback * 0.99f * combSumRight);
            if (isExternalFeedR) {
                float feedloopR = inputs[FEEDRIGHT_RETURN_INPUT].getVoltage(0) / 2.5f;
                audinRight += (feedback * 0.99f * feedloopR);
            }

        }
        else {
            audinRight = audinLeft;
        }

        //process 1st band then add others to it in loop
        BandsLeft[0] = _CombL[0].process(audinLeft);
        BandsRight[0] = _CombR[0].process(audinRight);
        combSumLeft = BandsLeft[0];
        combSumRight = BandsRight[0];
        for (int i = 1; i < 12; ++i) {
            float eveness = (i % 2 == 1) ? (EvenOdd) : (-EvenOdd);
            BandsLeft[i] = _CombL[i].process(audinLeft);
            combSumLeft += BandsLeft[i] * (( eveness ) / 2.f + 0.5f);
            BandsRight[i] = _CombR[i].process(audinRight);
            combSumRight += BandsRight[i] * (( eveness ) / 2.f + 0.5f);
        }
        //clamp to +-1 (should already be close) then enlarge with resonance, as the cutting tends to reduce volume
        combSumLeft = rack::math::clamp(combSumLeft, -1.f, 1.f);
        combSumRight = rack::math::clamp(combSumRight, -1.f, 1.f);
        combSumLeft *= resonance + 1.f;
        combSumRight *= resonance + 1.f;

        for (int d = 0; d < 3; ++d) { //give Vis lights somethin to work with, cutting out some floor movement
            visData[d] = (combSumLeft <= -0.001f || combSumLeft >= 0.001f) ? _CombL[d].getFrequencyPhase(BandsLeft[d]) * 0.4 : 0.f;
            visData[d + 3] = (combSumRight <= -0.001f || combSumRight >= 0.001f) ? _CombR[d].getFrequencyPhase(BandsRight[d]) * 0.4 : 0.f;
        }

        float outLeft = driveClamp(combSumLeft);
        float outRight = driveClamp(combSumRight);
        //don't send driven signal to feedback, keeps things tamer
        outputs[FEEDLEFT_SEND_OUTPUT].setVoltage(combSumLeft * (5.f), 0);
        outputs[FEEDRIGHT_SEND_OUTPUT].setVoltage(combSumRight * (5.f), 0);
        outputs[AUDIOLEFT_OUTPUT].setVoltage(outLeft, 0);
        outputs[AUDIORIGHT_OUTPUT].setVoltage(outRight, 0);
    }

    void onReset(const ResetEvent& e) override {
        //failsafe
        combSumLeft = 0;
        combSumRight = 0;
    }
    //nothing to save so the relic sits untouched
    json_t* dataToJson() override {
        json_t* rootJ = json_object();

        //json_t* RunningJ = json_boolean(runSet);

        //json_object_set_new(rootJ, "Running", RunningJ);

        return rootJ;
    }

    void dataFromJson(json_t* rootJ) override {
       
        //json_t* RunningJ = json_object_get(rootJ, "Running");
        
    }

};
//altered RectangleLight from sdk
struct LineLight : ModuleLightWidget {
    Vec lineStart;
    Vec lineEnd;
    int flag;
    ShearModule* module;
    LineLight(ShearModule* m, Vec topLeft, Vec size, int f) {
        this->module = m;
        this->box.pos = topLeft;
        this->box.size = size;
        this->lineStart = Vec(0, size.y);
        this->lineEnd = Vec(size.x, 0);
        
        this->flag = f;
    }

    void drawBackground(const widget::Widget::DrawArgs& args) override {
        // Derived from LightWidget::drawBackground()

        nvgBeginPath(args.vg);
        nvgRect(args.vg, 0, 0, this->box.size.x, this->box.size.y);

        // Background
        if (this->bgColor.a > 0.0) {
            nvgFillColor(args.vg, this->bgColor);
            nvgFill(args.vg);
        }

        // Border
        if (this->borderColor.a > 0.0) {
            nvgStrokeWidth(args.vg, 0.5);
            nvgStrokeColor(args.vg, this->borderColor);
            nvgStroke(args.vg);
        }
    }

    void drawLight(const widget::Widget::DrawArgs& args) override {
        // Derived from LightWidget::drawLight()
        float colflag = module->visData[flag];
        this->color.a = module->feedback / 2.f + 0.4f;
        switch (flag) {
        case 0: {
        }
        case 3: {
            this->color.r = 0.1;
            this->color.g = colflag / 3.f;
            this->color.b = (colflag * 2.f);
            break;
        }
        case 1: {
        }
        case 4: {
            this->color.r = 0.1;
            this->color.g = colflag / 2.f;
            this->color.b = (colflag * 2.f);
            break;
        }
        case 2: {
        }
        case 5: {
            this->color.r = 0.1;
            this->color.g = colflag;
            this->color.b = (colflag * 2.f);
            break;
        }
        }
        // Foreground
        if (this->color.a > 0.0) {
            nvgBeginPath(args.vg);
            nvgMoveTo(args.vg, this->lineStart.x, this->lineStart.y);
            nvgLineTo(args.vg, this->lineEnd.x, this->lineEnd.y);
            nvgStrokeColor(args.vg, this->color);
            nvgStrokeWidth(args.vg, 1.6);
            nvgStroke(args.vg);

        }
    }
};


struct ShearPanelWidget : ModuleWidget {
    ShearPanelWidget(ShearModule* module) {
        setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Shear_panel.svg")));

		addChild(createWidget<ScrewSilver>(Vec(15, 0)));
		//addChild(createWidget<ScrewSilver>(Vec(box.size.x - 30, 0)));
		//addChild(createWidget<ScrewSilver>(Vec(15, 365)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 30, 365)));

        addParam(createParam<RoundBigBlackKnob>(Vec(37, 33), module, ShearModule::CUTOFF_PARAM));
        addParam(createParam<RoundBlackKnob>(Vec(7, 91), module, ShearModule::RESONANCE_PARAM));
        addParam(createParam<RoundBlackKnob>(Vec(53, 109), module, ShearModule::EVEN_ODD_PARAM));
        addParam(createParam<RoundBlackKnob>(Vec(7, 214), module, ShearModule::FEEDBACK_PARAM));
        addParam(createParam<Trimpot>(Vec(67, 82), module, ShearModule::SKEW_PARAM));

        addInput(createInput<PurplePort>(Vec(7, 310), module, ShearModule::AUDIOLEFT_INPUT));
        addInput(createInput<PurplePort>(Vec(7, 340), module, ShearModule::AUDIORIGHT_INPUT));
        addInput(createInput<PurplePort>(Vec(7, 48), module, ShearModule::CUTOFF_INPUT));
        addInput(createInput<PurplePort>(Vec(7, 128), module, ShearModule::RESONANCE_INPUT));
        addInput(createInput<PurplePort>(Vec(60, 153), module, ShearModule::EVEN_ODD_INPUT));
        addInput(createInput<PurplePort>(Vec(7, 255), module, ShearModule::FEEDBACK_INPUT));
        addInput(createInput<PurplePort>(Vec(60, 198), module, ShearModule::FEEDLEFT_RETURN_INPUT));
        addInput(createInput<PurplePort>(Vec(60, 224), module, ShearModule::FEEDRIGHT_RETURN_INPUT));

        addOutput(createOutput<PurplePort>(Vec(60, 254), module, ShearModule::FEEDLEFT_SEND_OUTPUT));
        addOutput(createOutput<PurplePort>(Vec(60, 280), module, ShearModule::FEEDRIGHT_SEND_OUTPUT));
        addOutput(createOutput<PurplePort>(Vec(60, 310), module, ShearModule::AUDIOLEFT_OUTPUT));
        addOutput(createOutput<PurplePort>(Vec(60, 340), module, ShearModule::AUDIORIGHT_OUTPUT));

        if (module) {
            LineLight* line1L = new LineLight(module, Vec(3.352, 149.791), Vec(26.131, 43.358), 0);
            addChild(line1L);
            LineLight* line2L = new LineLight(module, Vec(3.501, 151.013), Vec(20.871, 34.248), 1);
            addChild(line2L);
            LineLight* line3L = new LineLight(module, Vec(3.568, 153.144), Vec(15.624, 25.160), 2);
            addChild(line3L);

            LineLight* line1R = new LineLight(module, Vec(26.333, 152.268), Vec(23.068, 38.053), 3);
            addChild(line1R);
            LineLight* line2R = new LineLight(module, Vec(34.023, 156.876), Vec(18.57, 30.262), 4);
            addChild(line2R);
            LineLight* line3R = new LineLight(module, Vec(43.127, 164.835), Vec(12.189, 19.210), 5);
            addChild(line3R);
        }
    }

};

Model* modelShear = createModel<ShearModule, ShearPanelWidget>("Shear-Comb");