#include "plugin.hpp"

#define MODULE_NAME ReflectModule
#define PANEL "Reflect_panel.svg"
#define HP 8


using namespace LydD;

static const int maxPolyphony = 1;


struct ReflectModule : Module
{

    float reflectRecurse(float lowlim, float uplim, float Val, int* loops) {
        //if val above uplim, newval = uplim - (val - uplim) invert if val below lowlim
        bool ishi = Val > uplim;
        bool islow = Val < lowlim;
        float newval = (ishi) ? uplim - (Val - uplim) : (islow) ? lowlim - (Val - lowlim) : Val;
        //check if newval still isnt in range ( usually pops out the other side). do again til it is.
        if ((newval < uplim && newval > lowlim) || *loops >= 10) {
            return newval;

        }
        else {
            *loops += 1;
            return reflectRecurse(lowlim, uplim, newval, loops);
        }
    }

    float wrapRecurse(float lowlim, float uplim, float Val, int* loops) {
        bool ishi = Val > uplim;
        bool islow = Val < lowlim;
        float range = uplim - lowlim;
        float newval = (ishi) ? Val - range : (islow) ? Val + range : Val;
        if ((newval < uplim && newval > lowlim) || *loops >= 10) {
            return newval;

        }
        else {
            *loops += 1;
            return wrapRecurse(lowlim, uplim, newval, loops);
        }
    }

    //float Curve(float lowlim, float uplim, float Val, float Curve) {
    //    //create normalized value, curve the value, rescale 
    //    float normval = lerp(0.f, 1.f, lowlim, uplim, Val);
    //    float curval = lerp(1.f, normval, 0, 1, Curve);
    //    float curvenorm = normval * curval;
    //    return lerp(lowlim, uplim, 0.f, 1.f, curvenorm);
    //}

    enum ParamIds {
        ENUMS(WINDOW_SIZE_PARAM, 2),
        ENUMS(WINDOW_OFFSET_PARAM, 2),
        ENUMS(CURVE_PARAM, 2),
        ENUMS(MIRROR_BUTTON, 2),
        NUM_PARAMS
    };
    enum InputIds {

        ENUMS(SIGNAL_INPUT, 2),
        ENUMS(WSIZE_INPUT, 2),
        ENUMS(WOFFSET_INPUT, 2),
        ENUMS(CURVE_INPUT, 2),
        NUM_INPUTS
    };
    enum OutputIds {

        ENUMS(MAIN_OUTPUT, 2),
        ENUMS(INVERT_OUTPUT, 2),
        NUM_OUTPUTS
    };
    enum LightIds {
        ENUMS(MIRROR_LIGHT, 2),
        ENUMS(LOOP_LIGHT, 2),
        NUM_LIGHTS
    };



    int currentPolyphony = 1;
    int currentBanks = 1;
    int loopCounter = 0;
    int wrapCount[2] = {0, 0};
    int wrapinv[2] = { 0, 0 };
    bool MirMode[2] = { false, false };
    bool mireset[2] = { false, false };
    float visData[4] = { 0.f }; // capture nearness to each window
    bool isinSig[2] = { false, false };
    bool isinWsize[2] = { false, false };
    bool isinWoffs[2] = { false, false };
    bool isinCurve[2] = { false, false };


    
    
    ReflectModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        for (int p = 0; p < 2; ++p) {
            configParam(WINDOW_SIZE_PARAM + p, 0.5f, 10.f, 10.f, "Window Size");
            configParam(WINDOW_OFFSET_PARAM + p, -5.f, 5.f, 0.f, "Shift");
            configParam(CURVE_PARAM + p, -1.f, 1.f, 0.f, "Curve");
            configSwitch(MIRROR_BUTTON + p, 0.f, 1.f, 0.f, "Mirror Type");
        }

    }

    

    void process(const ProcessArgs& args) override {
  
        if (loopCounter % 4 == 0) {
            checkInputs(args);          
        }

        generateOutput(args);

        loopCounter++;
        loopCounter %= 2520;
    }


    void checkInputs(const ProcessArgs& args) {
        for (int m = 0; m < 2; ++m) {
            latchButton(params[MIRROR_BUTTON + m].value, &MirMode[m], &mireset[m]);
            lights[MIRROR_LIGHT + m].setBrightness(!MirMode[m]);
            lights[LOOP_LIGHT + m].setBrightness(MirMode[m]);
            isinSig[m] = inputs[SIGNAL_INPUT + m].isConnected();
            isinWsize[m] = inputs[WSIZE_INPUT + m].isConnected();
            isinWoffs[m] = inputs[WOFFSET_INPUT + m].isConnected();
            isinCurve[m] = inputs[CURVE_INPUT + m].isConnected();
        }
        
    }

    void generateOutput(const ProcessArgs& args) {

        float cascIn[2];
        cascIn[0] = isinSig[0] ? inputs[SIGNAL_INPUT + 0].getVoltage(0) : 0.f;
        cascIn[1] = isinSig[1] ? inputs[SIGNAL_INPUT + 1].getVoltage(0) : cascIn[0];

        for (int m = 0; m < 2; ++m) {
            float signal = cascIn[m];
            float signalinv = -signal;

            float sizepar = params[WINDOW_SIZE_PARAM + m].value;
            float sizein = isinWsize[m] ? abs(inputs[WSIZE_INPUT + m].getVoltage(0)) : 0.f;
            float size = rack::math::clamp(sizepar + sizein, 0.5f, 10.f);

            float offsetpar = params[WINDOW_OFFSET_PARAM + m].value;
            float offsetin = isinWoffs[m] ? inputs[WOFFSET_INPUT + m].getVoltage(0) : 0.f;
            float offset = rack::math::clamp(offsetpar + offsetin, -5.f, 5.f);

            float curvepar = params[CURVE_PARAM + m].value;
            float curvein = isinCurve[m] ? (inputs[CURVE_INPUT + m].getVoltage(0) / 5.f) : 0.f;
            float curve = rack::math::clamp(curvepar + curvein, -0.99f, 0.99f);

            float mirror1 = offset - (size * 0.5f);
            float mirror2 = offset + (size * 0.5f);
            float reflected = 0.f;
            float reflectedinv = 0.f;
            wrapCount[m] = 0;
            wrapinv[m] = 0;
            switch (MirMode[m]) {
            case 0: {
                reflected = reflectRecurse(mirror1, mirror2, signal, &wrapCount[m]);
                reflectedinv = reflectRecurse(mirror1, mirror2, signalinv, &wrapinv[m]);
                break;
            }
            case 1: {
                reflected = wrapRecurse(mirror1, mirror2, signal, &wrapCount[m]);
                reflectedinv = wrapRecurse(mirror1, mirror2, signalinv, &wrapinv[m]);
                break;
            }
            }
            reflected = normalCurve(mirror1, mirror2, reflected, curve);
            reflectedinv = normalCurve(mirror1, mirror2, reflectedinv, curve);
            outputs[MAIN_OUTPUT + m].setVoltage(reflected, 0);
            outputs[INVERT_OUTPUT + m].setVoltage(reflectedinv, 0);
            visData[m * 2] = lerp(0.f, 1.f, 0.f, abs(mirror1 - mirror2), abs(reflected - mirror1));
            visData[(m * 2) + 1] = lerp(0.f, 1.f, 0.f, abs(mirror1 - mirror2), abs(reflected - mirror2));;
        }
    }

    /*void onReset(const ResetEvent& e) override {

    }*/

    json_t* dataToJson() override {
        json_t* rootJ = json_object();

        json_t* mode1J = json_boolean(MirMode[0]);
        json_t* mode2J = json_boolean(MirMode[1]);
        json_object_set_new(rootJ, "mode1", mode1J);
        json_object_set_new(rootJ, "mode2", mode2J);

        return rootJ;
    }

    void dataFromJson(json_t* rootJ) override {
       
        json_t* mode1J = json_object_get(rootJ, "mode1");
        if (mode1J) MirMode[0] = json_boolean_value(mode1J);
        json_t* mode2J = json_object_get(rootJ, "mode2");
        if (mode2J) MirMode[1] = json_boolean_value(mode2J);
        
    }

};
//altered RectangleLight from sdk
struct BarLight : ModuleLightWidget {
    Vec lineStart;
    Vec lineEnd;
    int which;
    int side;
    ReflectModule* module;
    BarLight(ReflectModule* m, Vec topLeft, Vec size, int w, int s) {
        this->module = m;
        this->box.pos = topLeft;
        this->box.size = size;
        this->lineStart = Vec(0, size.y);
        this->lineEnd = Vec(size.x, 0);
        this->which = w;
        this->side = s;
    }

    void drawHalo(const widget::Widget::DrawArgs& args) override {
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
        if (module) {
            float colflag = module->wrapCount[which];
            /*this->color.a = 55;

            this->color.r = module->visData[which * 2 + side];
            this->color.g = colflag / 23.f;
            this->color.b = 1 - 1.f / (colflag);*/
            this->color = nvgHSL(0.43 + colflag / 8.f, 0.7, 0.5 - module->visData[(which * 2) + side]);

        }
        // Foreground
            nvgBeginPath(args.vg);
            nvgRect(args.vg, 0, 0, this->box.size.x, this->box.size.y);
            nvgFillColor(args.vg, this->color);
            nvgFill(args.vg);
            /*nvgMoveTo(args.vg, this->lineStart.x, this->lineStart.y);
            nvgLineTo(args.vg, this->lineEnd.x, this->lineEnd.y);
            nvgStrokeColor(args.vg, this->color);
            nvgStrokeWidth(args.vg, 1.9);
            nvgStroke(args.vg);*/
    }
};

using namespace LydD::Components;
struct ReflectPanelWidget : ModuleWidget {

    //include struct for logo here so it has modules name
    #include "Theme/LogoLight.h"

    ReflectPanelWidget(ReflectModule* module) {
        setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Reflect_panel.svg")));

		addChild(createWidget<ScrewSilver>(Vec(15, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 30, 0)));
		addChild(createWidget<ScrewSilver>(Vec(15, 365)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 30, 365)));

        float bdy = 153.f;
        for (int b = 0; b < 2; ++b) {
            addChild(createLight<MediumLight<BlueLight>>(Vec(39.5, 162.1 + (bdy * b)), module, ReflectModule::MIRROR_LIGHT + b));
            addChild(createLight<MediumLight<BlueLight>>(Vec(72.5, 162.1 + (bdy * b)), module, ReflectModule::LOOP_LIGHT + b));


            addParam(createParam<RoundLargeBlackKnob>(Vec(12.7, 70.6 + (bdy * b)), module, ReflectModule::WINDOW_SIZE_PARAM + b));
            addParam(createParam<RoundLargeBlackKnob>(Vec(69.5, 70.6 + (bdy * b)), module, ReflectModule::WINDOW_OFFSET_PARAM + b));
            addParam(createParam<RoundSmallBlackKnob>(Vec(71, 122.9 + (bdy * b)), module, ReflectModule::CURVE_PARAM + b));

            addParam(createParam<VCVButton>(Vec(50.6, 156.6 + (bdy * b)), module, ReflectModule::MIRROR_BUTTON + b));

            addInput(createInput<PurplePort>(Vec(7, 154.1 + (bdy * b)), module, ReflectModule::WSIZE_INPUT + b));
            addInput(createInput<PurplePort>(Vec(89.3, 154.1 + (bdy * b)), module, ReflectModule::WOFFSET_INPUT + b));
            addInput(createInput<PurplePort>(Vec(25.9, 120.5 + (bdy * b)), module, ReflectModule::CURVE_INPUT + b));
            addInput(createInput<PurplePort>(Vec(48.2, 184.9 + (bdy * b)), module, ReflectModule::SIGNAL_INPUT + b));

            addOutput(createOutput<PurplePort>(Vec(7.0, 184.9 + (bdy * b)), module, ReflectModule::MAIN_OUTPUT + b));
            addOutput(createOutput<PurplePort>(Vec(89.3, 184.9 + (bdy * b)), module, ReflectModule::INVERT_OUTPUT + b));
        }

        if (module) {
            BarLight* line1L = new BarLight(module, Vec(11, 107), Vec(4, 44), 0, 0);
            addChild(line1L);
            BarLight* line2L = new BarLight(module, Vec(103.5 + 4, 107), Vec(-4, 44), 0, 1);
            addChild(line2L);
            BarLight* line3L = new BarLight(module, Vec(11, 259.5), Vec(4, 44), 1, 0);
            addChild(line3L);
            BarLight* line4L = new BarLight(module, Vec(103.5 + 4, 259.5), Vec(-4, 44), 1, 1);
            addChild(line4L);

            //must be called 'logoPos'for all modules 
            Vec logoPos = Vec(((15.f * HP) / 2.f) - 12.5, 363.f);
            ReflectModule* module = dynamic_cast<ReflectModule*>(this->module);
            assert(module);
            #include "Theme/LogoChild.h"

        }
    }

};

Model* modelReflect = createModel<ReflectModule, ReflectPanelWidget>("Reflector");