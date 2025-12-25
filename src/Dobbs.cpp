#include "plugin.hpp"
#include <vector>
#include <string>


static const int maxPolyphony = 1;

class Envelope {
private:
    BaseFunctions Functions;
    rack::dsp::BooleanTrigger _trigger;
    rack::dsp::BooleanTrigger _EOC;
    rack::dsp::PulseGenerator _EOCPulse;
public:
    float Atime;
    float Rtime;
    float Aphase;
    float Rphase;
    float samplePhase;
    float Shape;
    bool Attacking;
    bool Sustain;
    bool Sustaining;
    bool EOC;
    
    Envelope() {
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
    void Trigger(float trig, bool sustain) {
        bool triggered = this->_trigger.process(trig);
        this->Sustaining = (this->Sustain) ? sustain : false;

        if (triggered) {
            //retrigger
            if (this->samplePhase != 0.f) {
                this->samplePhase = (this->Attacking) ? this->Aphase * this->Atime : (-this->Rphase + 1.f) * this->Atime;
            }    
            this->Attacking = true;
        }

        return;
    }
    void triggerCompanion(Envelope* companion, float delaytime) {
        float totalphase = (this->Attacking) ? this->Aphase : this->Rphase + 1.f;
        float trigcompanion = (totalphase >= delaytime * 1.99f) ? 1.f : 0.f;
        companion->Trigger(trigcompanion, this->Sustaining);
    }
    void AttackPhase(float* Value, float sampletime) {
        if (this->Attacking) {
            float shapemod = Functions.lerp(1, *Value, 0, 1, this->Shape);
            
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
        if (!this->Attacking && *Value > 0.f && !this->Sustaining) {
            float shapemod = Functions.lerp(1, *Value, 0, 1, this->Shape);
            
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

struct DobbsModule : Module
{
    enum ParamIds {
        ENUMS(AMAIN_PARAM, 2),
        ENUMS(ACOMP_PARAM, 2),
        ENUMS(RMAIN_PARAM, 2),
        ENUMS(RCOMP_PARAM, 2),
        ENUMS(SHAPE_PARAM, 2),
        ENUMS(DELAY_PARAM, 2),
        ENUMS(MODE_BUTTON_PARAM, 2),
        ENUMS(SPEED_FMAIN_BUTTON_PARAM, 2),
        ENUMS(SPEED_FCOMP_BUTTON_PARAM, 2),
        NUM_PARAMS
    };
    enum InputIds {
        ENUMS(AMAIN_INPUT, 2),
        ENUMS(ACOMP_INPUT, 2),
        ENUMS(RMAIN_INPUT, 2),
        ENUMS(RCOMP_INPUT, 2),
        ENUMS(SHAPE_INPUT, 2),
        ENUMS(DELAY_INPUT, 2),
        ENUMS(GATE_INPUT, 2),
        NUM_INPUTS
    };
    enum OutputIds {

        ENUMS(ENVMAIN_OUTPUT, 2),
        ENUMS(ENVCOMP_OUTPUT, 2),
        ENUMS(EOCMAIN_OUTPUT, 2),
        ENUMS(EOCCOMP_OUTPUT, 2),

        NUM_OUTPUTS
    };
    enum LightIds {
        ASR_LEFT_LIGHT,
        ASR_RIGHT_LIGHT,
        SPEEDMAIN_LEFT_LIGHT,
        SPEEDMAIN_RIGHT_LIGHT,
        SPEEDCOMP_LEFT_LIGHT,
        SPEEDCOMP_RIGHT_LIGHT,
        NUM_LIGHTS
    };

    BaseFunctions Functions;
    BaseButtons Buttons;
    BaseMatrices Matrix;
    Envelope ENVmain[2];
    Envelope ENVcomp[2];

    DobbsModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        for (int i = 0; i < 2; ++i) {
            configParam(AMAIN_PARAM + i, 0.001f, 1.f, 0.1f, "Attack 1");
            configParam(ACOMP_PARAM + i, 0.001f, 1.f, 0.2f, "Attack 2");
            configParam(RMAIN_PARAM + i, 0.001f, 1.f, 0.1f, "Release 1");
            configParam(RCOMP_PARAM + i, 0.001f, 1.f, 0.2f, "Release 2");
            configParam(SHAPE_PARAM + i, -0.99f, 0.99f, 0.f, "Shape");
            configParam(DELAY_PARAM + i, 0.f, 1.f, 0.f, "Delay");
            configParam(MODE_BUTTON_PARAM + i, 0.f, 1.f, 0.f, "AR/ASR");
            configParam(SPEED_FMAIN_BUTTON_PARAM + i, 0.f, 1.f, 0.f, "TimeScale 1");
            configParam(SPEED_FCOMP_BUTTON_PARAM + i, 0.f, 1.f, 0.f, "TimeScale 2");
       
            std::string left = "Left";
            std::string right = "Right";
            std::string side = (i == 0) ? left : right;

            std::string Amain = "Attack 1 - ";
            Amain += side;
            configInput(AMAIN_INPUT + i, Amain);

            std::string Acomp = "Attack 2 - ";
            Acomp += side;
            configInput(ACOMP_INPUT + i, Acomp);

            std::string Rmain = "Release 1 - ";
            Rmain += side;
            configInput(RMAIN_INPUT + i, Rmain);

            std::string Rcomp = "Release 2 - ";
            Rcomp += side;
            configInput(RCOMP_INPUT + i, Rcomp);

            std::string shape = "Shape - ";
            shape += side;
            configInput(SHAPE_INPUT + i, shape);

            std::string delay = "Delay 2 - ";
            delay += side;
            configInput(DELAY_INPUT + i, delay);

            std::string gate = "Trig/Gate - ";
            gate += side;
            configInput(GATE_INPUT + i, gate);

            std::string env1 = "Envelope 1 - ";
            env1 += side;
            configOutput(ENVMAIN_OUTPUT + i, env1);
            
            std::string env2 = "Envelope 2 - ";
            env2 += side;
            configOutput(ENVCOMP_OUTPUT + i, env2);

            std::string eoc1 = "End of Cycle 1 - ";
            eoc1 += side;
            configOutput(EOCMAIN_OUTPUT + i, eoc1);

            std::string eoc2 = "End of Cycle 2 - ";
            eoc2 += side;
            configOutput(EOCCOMP_OUTPUT + i, eoc2);
        }

    }

    int currentPolyphony = 1;
    int currentBanks = 1;
    int loopCounter = 0;
    float Gates[2] = { 0.f, 0.f };
    float EnvelopeMain[2] = { 0.f, 0.f };
    float EnvelopeCompanion[2] = { 0.f, 0.f };
    bool ASRset[2] = { false, false };
    bool ASRunset[2] = { false, false };
    bool spdFsetM[2] = { false, false };
    bool spdFresetM[2] = { false, false };
    bool spdFsetC[2] = { false, false };
    bool spdFresetC[2] = {false, false};
   
    void process(const ProcessArgs& args) override {

      
    
        if (loopCounter % 8 == 0) {

            setParams(args);
          
        }
        generateOutput(args);

        if (loopCounter % 4 == 0) {

        }
        
        ++loopCounter;
        if (loopCounter % 2520 == 0) {
            loopCounter = 0;
        }
    }

    void setParams(const ProcessArgs& args) {
        for (int o = ENVMAIN_OUTPUT + 0; o != NUM_OUTPUTS; ++o) {
            outputs[o].setChannels(1);
        }


        for (int i = 0; i < 2; ++i) {
            Buttons.latchButton(params[MODE_BUTTON_PARAM + i].value, &ASRset[i], &ASRunset[i]);
            Buttons.latchButton(params[SPEED_FMAIN_BUTTON_PARAM + i].value, &spdFsetM[i], &spdFresetM[i]);
            Buttons.latchButton(params[SPEED_FCOMP_BUTTON_PARAM + i].value, &spdFsetC[i], &spdFresetC[i]);
            Gates[i] = (inputs[GATE_INPUT + i].isConnected()) ? inputs[GATE_INPUT + i].getVoltage(0) : 0.f;
        }
        lights[ASR_LEFT_LIGHT].setBrightness(ASRset[0]);
        lights[ASR_RIGHT_LIGHT].setBrightness(ASRset[1]);
        lights[SPEEDMAIN_LEFT_LIGHT].setBrightness(spdFsetM[0]);
        lights[SPEEDMAIN_RIGHT_LIGHT].setBrightness(spdFsetM[1]);
        lights[SPEEDCOMP_LEFT_LIGHT].setBrightness(spdFsetC[0]);
        lights[SPEEDCOMP_RIGHT_LIGHT].setBrightness(spdFsetC[1]);
    }

    void generateOutput(const ProcessArgs& args) {
        for (int i = 0; i < 2; ++i) {
            float attacktime1 = rack::math::clamp(params[AMAIN_PARAM + i].value + ((inputs[AMAIN_INPUT + i].isConnected()) ? (inputs[AMAIN_INPUT + i].getVoltage(0) / 5.f) : 0.f), 0.0001f, 1.f);
            float releasetime1 = rack::math::clamp(params[RMAIN_PARAM + i].value + ((inputs[RMAIN_INPUT + i].isConnected()) ? (inputs[RMAIN_INPUT + i].getVoltage(0) / 5.f) : 0.f), 0.0001f, 1.f);
            float attacktime2 = rack::math::clamp(params[ACOMP_PARAM + i].value + ((inputs[ACOMP_INPUT + i].isConnected()) ? (inputs[ACOMP_INPUT + i].getVoltage(0) / 5.f) : 0.f), 0.0001f, 1.f);
            float releasetime2 = rack::math::clamp(params[RCOMP_PARAM + i].value + ((inputs[RCOMP_INPUT + i].isConnected()) ? (inputs[RCOMP_INPUT + i].getVoltage(0) / 5.f) : 0.f), 0.0001f, 1.f);
            float shape = rack::math::clamp(params[SHAPE_PARAM + i].value * ((inputs[SHAPE_INPUT + i].isConnected()) ? (inputs[SHAPE_INPUT + i].getVoltage(0) / 5.f) : 1.f), -1.f, 1.f);
            float delay = rack::math::clamp(params[DELAY_PARAM + i].value + abs((inputs[DELAY_INPUT + i].isConnected()) ? (inputs[DELAY_INPUT + i].getVoltage(0) / 5.f) : 0.f), 0.001f, 1.f);
            ENVmain[i].setAttackRelease(spdFsetM[i], attacktime1, releasetime1, ASRset[i]);
            ENVcomp[i].setAttackRelease(spdFsetC[i], attacktime2, releasetime2, ASRset[i]);
            ENVmain[i].setShape(shape);
            ENVcomp[i].setShape(shape);
            ENVmain[i].Trigger(Gates[i], Gates[i] > 0.5);
            ENVmain[i].triggerCompanion(&ENVcomp[i], delay);
            ENVmain[i].AttackPhase(&EnvelopeMain[i], args.sampleTime);
            ENVmain[i].ReleasePhase(&EnvelopeMain[i], args.sampleTime);
            ENVcomp[i].AttackPhase(&EnvelopeCompanion[i], args.sampleTime);
            ENVcomp[i].ReleasePhase(&EnvelopeCompanion[i], args.sampleTime);
            float EOCmain = (ENVmain[i].isEOC(args.sampleTime)) * 10.f;
            float EOCcomp = (ENVcomp[i].isEOC(args.sampleTime)) * 10.f;
            outputs[ENVMAIN_OUTPUT + i].setVoltage(EnvelopeMain[i] * 10.f, 0);
            outputs[EOCMAIN_OUTPUT + i].setVoltage(EOCmain, 0);
            outputs[ENVCOMP_OUTPUT + i].setVoltage(EnvelopeCompanion[i] * 10.f, 0);
            outputs[EOCCOMP_OUTPUT + i].setVoltage(EOCcomp, 0);

        }
    }

    json_t* dataToJson() override {
        json_t* rootJ = json_object();

        json_t* ASR1J = json_boolean(ASRset[0]);
        json_t* ASR2J = json_boolean(ASRset[1]);
        json_t* SPDMAIN1J = json_boolean(spdFsetM[0]);
        json_t* SPDMAIN2J = json_boolean(spdFsetM[1]);
        json_t* SPDCOMP1J = json_boolean(spdFsetC[0]);
        json_t* SPDCOMP2J = json_boolean(spdFsetC[1]);

        json_object_set_new(rootJ, "ASR1", ASR1J);
        json_object_set_new(rootJ, "ASR2", ASR2J);
        json_object_set_new(rootJ, "SPEEDMAIN1", SPDMAIN1J);
        json_object_set_new(rootJ, "SPEEDMAIN2", SPDMAIN2J);
        json_object_set_new(rootJ, "SPEEDCOMP1", SPDCOMP1J);
        json_object_set_new(rootJ, "SPEEDCOMP2", SPDCOMP2J);

        return rootJ;
    }

    void dataFromJson(json_t* rootJ) override {
       
        json_t* ASR1J = json_object_get(rootJ, "ASR1");
        json_t* ASR2J = json_object_get(rootJ, "ASR2");
        json_t* SPDMAIN1J = json_object_get(rootJ, "SPEEDMAIN1");
        json_t* SPDMAIN2J = json_object_get(rootJ, "SPEEDMAIN2");
        json_t* SPDCOMP1J = json_object_get(rootJ, "SPEEDCOMP1");
        json_t* SPDCOMP2J = json_object_get(rootJ, "SPEEDCOMP2");
        ASRset[0] = json_boolean_value(ASR1J);
        ASRset[1] = json_boolean_value(ASR2J);
        spdFsetM[0] = json_boolean_value(SPDMAIN1J);
        spdFsetM[1] = json_boolean_value(SPDMAIN2J);
        spdFsetC[0] = json_boolean_value(SPDCOMP1J);
        spdFsetC[1] = json_boolean_value(SPDCOMP2J);
    }

};


struct EnvLEDMain : SvgWidget {
    DobbsModule* module;
    int side;
    EnvLEDMain() {
        //this->setSvg(Svg::load(asset::plugin(pluginInstance, "res/MountDobbsLeft.svg")));
    }
    void Svg(std::string path) {
        this->setSvg(Svg::load(asset::plugin(pluginInstance, path)));
    }
    void drawLayer(const DrawArgs& args, int layer) override {

        nvgGlobalCompositeBlendFunc(args.vg, NVG_ONE_MINUS_DST_COLOR, NVG_ONE);
        if (!svg) return;
        if (module && layer == 1) {
            for (auto s = svg->handle->shapes; s; s = s->next) {
                nvgStrokeWidth(args.vg, (s->strokeWidth));
                float enVal = (module->EnvelopeMain[side] );
                nvgFillColor(args.vg, nvgHSL(0.3f + (enVal / 5.f), 0.8, enVal * 0.6f));
                for (auto p = s->paths; p; p = p->next) {
                    nvgBeginPath(args.vg);
                    nvgMoveTo(args.vg, p->pts[0], p->pts[1]);
                    for (auto i = 0; i < p->npts - 1; i += 3) {
                        float* path = &p->pts[i * 2];
                        nvgBezierTo(args.vg, path[2], path[3], path[4], path[5], path[6], path[7]);
                    }
                    if (p->closed)
                        nvgLineTo(args.vg, p->pts[0], p->pts[1]);
                    if (s->fill.type)
                        nvgFill(args.vg);
                    if (s->stroke.type)
                        nvgStroke(args.vg);
                }
            }
        }
        Widget::drawLayer(args, layer);
    }
};
//identical, could remove if added flag like side for which envelope to assess
struct EnvLEDComp : SvgWidget {
    DobbsModule* module;
    int side;
    EnvLEDComp() {
        //this->setSvg(Svg::load(asset::plugin(pluginInstance, "res/MountDobbsLeft2.svg")));
    }
    void Svg(std::string path) {
        this->setSvg(Svg::load(asset::plugin(pluginInstance, path)));
    }
    void drawLayer(const DrawArgs& args, int layer) override {

        nvgGlobalCompositeBlendFunc(args.vg, NVG_ONE_MINUS_DST_COLOR, NVG_ONE);
        if (!svg) return;
        if (module && layer == 1) {
            for (auto s = svg->handle->shapes; s; s = s->next) {
                nvgStrokeWidth(args.vg, (s->strokeWidth));
                float enVal = (module->EnvelopeCompanion[side]);
                nvgFillColor(args.vg, nvgHSL(0.3f + (enVal * 0.2f), 0.5f, enVal * 0.6f) /*nvgRGBAf(0.f, 1.f, 0.2f, enVal)*/);
                for (auto p = s->paths; p; p = p->next) {
                    nvgBeginPath(args.vg);
                    nvgMoveTo(args.vg, p->pts[0], p->pts[1]);
                    for (auto i = 0; i < p->npts - 1; i += 3) {
                        float* path = &p->pts[i * 2];
                        nvgBezierTo(args.vg, path[2], path[3], path[4], path[5], path[6], path[7]);
                    }
                    if (p->closed)
                        nvgLineTo(args.vg, p->pts[0], p->pts[1]);
                    if (s->fill.type)
                        nvgFill(args.vg);
                    if (s->stroke.type)
                        nvgStroke(args.vg);
                }
            }
        }
        Widget::drawLayer(args, layer);
    }
};


struct DobbsPanelWidget : ModuleWidget {
    DobbsPanelWidget(DobbsModule* module) {
        setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Dobbs_panel.svg")));

		addChild(createWidget<ScrewSilver>(Vec(15, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 30, 0)));
		addChild(createWidget<ScrewSilver>(Vec(15, 365)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 30, 365)));

        addChild(createLightCentered<SmallLight<BlueLight>>(Vec(62, 212), module, DobbsModule::ASR_LEFT_LIGHT));
        addChild(createLightCentered<SmallLight<BlueLight>>(Vec(120, 212), module, DobbsModule::ASR_RIGHT_LIGHT));
        addChild(createLightCentered<SmallLight<BlueLight>>(Vec(29, 110), module, DobbsModule::SPEEDMAIN_LEFT_LIGHT));
        addChild(createLightCentered<SmallLight<BlueLight>>(Vec(152, 110), module, DobbsModule::SPEEDMAIN_RIGHT_LIGHT));
        addChild(createLightCentered<SmallLight<BlueLight>>(Vec(62, 145), module, DobbsModule::SPEEDCOMP_LEFT_LIGHT));
        addChild(createLightCentered<SmallLight<BlueLight>>(Vec(120, 145), module, DobbsModule::SPEEDCOMP_RIGHT_LIGHT));
            
        //left side..
        addParam(createParam<RoundLargeBlackKnob>(Vec(10, 65), module, DobbsModule::AMAIN_PARAM + 0));
        addParam(createParam<RoundBlackKnob>(Vec(50, 105), module, DobbsModule::ACOMP_PARAM + 0));
        addParam(createParam<RoundLargeBlackKnob>(Vec(10, 135), module, DobbsModule::RMAIN_PARAM + 0));
        addParam(createParam<RoundBlackKnob>(Vec(50, 175), module, DobbsModule::RCOMP_PARAM + 0));
        addParam(createParam<Trimpot>(Vec(14, 190), module, DobbsModule::SHAPE_PARAM + 0));
        addParam(createParam<RoundSmallBlackKnob>(Vec(58, 60), module, DobbsModule::DELAY_PARAM + 0));
        addParam(createParam<VCVButton>(Vec(65, 212), module, DobbsModule::MODE_BUTTON_PARAM + 0));
        addParam(createParam<VCVButton>(Vec(8, 111), module, DobbsModule::SPEED_FMAIN_BUTTON_PARAM + 0));
        addParam(createParam<VCVButton>(Vec(65, 148), module, DobbsModule::SPEED_FCOMP_BUTTON_PARAM + 0));
        //right side..
        addParam(createParam<RoundLargeBlackKnob>(Vec(133, 65), module, DobbsModule::AMAIN_PARAM + 1));
        addParam(createParam<RoundBlackKnob>(Vec(101.5, 105), module, DobbsModule::ACOMP_PARAM + 1));
        addParam(createParam<RoundLargeBlackKnob>(Vec(134, 135), module, DobbsModule::RMAIN_PARAM + 1));
        addParam(createParam<RoundBlackKnob>(Vec(101.5, 175), module, DobbsModule::RCOMP_PARAM + 1));
        addParam(createParam<Trimpot>(Vec(150, 190), module, DobbsModule::SHAPE_PARAM + 1));
        addParam(createParam<RoundSmallBlackKnob>(Vec(99, 60), module, DobbsModule::DELAY_PARAM + 1));
        addParam(createParam<VCVButton>(Vec(98, 212), module, DobbsModule::MODE_BUTTON_PARAM + 1));
        addParam(createParam<VCVButton>(Vec(155, 111), module, DobbsModule::SPEED_FMAIN_BUTTON_PARAM + 1));
        addParam(createParam<VCVButton>(Vec(98, 148), module, DobbsModule::SPEED_FCOMP_BUTTON_PARAM + 1));


        addInput(createInput<PurplePort>(Vec(10, 248), module, DobbsModule::AMAIN_INPUT + 0));
        addInput(createInput<PurplePort>(Vec(58, 248), module, DobbsModule::ACOMP_INPUT + 0));
        addInput(createInput<PurplePort>(Vec(10, 282), module, DobbsModule::RMAIN_INPUT + 0));
        addInput(createInput<PurplePort>(Vec(58, 282), module, DobbsModule::RCOMP_INPUT + 0));
        addInput(createInput<PurplePort>(Vec(10, 216), module, DobbsModule::SHAPE_INPUT + 0));
        addInput(createInput<PurplePort>(Vec(34, 265), module, DobbsModule::DELAY_INPUT + 0));
        addInput(createInput<PurplePort>(Vec(34, 232), module, DobbsModule::GATE_INPUT + 0));

        addInput(createInput<PurplePort>(Vec(147, 248), module, DobbsModule::AMAIN_INPUT + 1));
        addInput(createInput<PurplePort>(Vec(99, 248), module, DobbsModule::ACOMP_INPUT + 1));
        addInput(createInput<PurplePort>(Vec(147, 282), module, DobbsModule::RMAIN_INPUT + 1));
        addInput(createInput<PurplePort>(Vec(99, 282), module, DobbsModule::RCOMP_INPUT + 1));
        addInput(createInput<PurplePort>(Vec(147, 216), module, DobbsModule::SHAPE_INPUT + 1));
        addInput(createInput<PurplePort>(Vec(123, 265), module, DobbsModule::DELAY_INPUT + 1));
        addInput(createInput<PurplePort>(Vec(123, 232), module, DobbsModule::GATE_INPUT + 1));


        addOutput(createOutput<PurplePort>(Vec(10, 315), module, DobbsModule::ENVMAIN_OUTPUT + 0));
        addOutput(createOutput<PurplePort>(Vec(58, 315), module, DobbsModule::ENVCOMP_OUTPUT + 0));
        addOutput(createOutput<PurplePort>(Vec(21, 340), module, DobbsModule::EOCMAIN_OUTPUT + 0));
        addOutput(createOutput<PurplePort>(Vec(46, 340), module, DobbsModule::EOCCOMP_OUTPUT + 0));

        addOutput(createOutput<PurplePort>(Vec(147, 315), module, DobbsModule::ENVMAIN_OUTPUT + 1));
        addOutput(createOutput<PurplePort>(Vec(99, 315), module, DobbsModule::ENVCOMP_OUTPUT + 1));
        addOutput(createOutput<PurplePort>(Vec(136, 340), module, DobbsModule::EOCMAIN_OUTPUT + 1));
        addOutput(createOutput<PurplePort>(Vec(111, 340), module, DobbsModule::EOCCOMP_OUTPUT + 1));

        if (module) {
            EnvLEDMain* Main1 = createWidget<EnvLEDMain>(Vec(0, 33));
            Main1->Svg("res/DobbsLights/MountDobbsLeft.svg");
            Main1->side = 0;
            Main1->module = module;
            addChild(Main1);

            EnvLEDComp* Comp1 = createWidget<EnvLEDComp>(Vec(65, 36.5));
            Comp1->Svg("res/DobbsLights/MountDobbsLeft2.svg");
            Comp1->side = 0;
            Comp1->module = module;
            addChild(Comp1);

            EnvLEDMain* Main2 = createWidget<EnvLEDMain>(Vec(116, 25));
            Main2->Svg("res/DobbsLights/MountDobbsRight.svg");
            Main2->side = 1;
            Main2->module = module;
            addChild(Main2);

            EnvLEDComp* Comp2 = createWidget<EnvLEDComp>(Vec(55.5, 28));
            Comp2->Svg("res/DobbsLights/MountDobbsRight2.svg");
            Comp2->side = 1;
            Comp2->module = module;
            addChild(Comp2);

        }
    }
    
};

Model* modelDobbs = createModel<DobbsModule, DobbsPanelWidget>("Dobbs-Env");