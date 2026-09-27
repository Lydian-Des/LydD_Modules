#include "plugin.hpp"
#include "Lydapi/LydEnvelope.h"
#define MODULE_NAME DobbsModule
#define PANEL "Dobbs_panel.svg"
#define HP 12

using namespace LydD;

static const int maxPolyphony = 1;

struct DobbsModule : Module
{
    #include "Theme/PanelVars.h"
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
        ENUMS(MOUNT_LIGHT, 6),
        ENUMS(HILL_LIGHT, 6),
        NUM_LIGHTS
    };

    LydD::Envelope::Couplable_Envelope<float> Main[2];
    LydD::Envelope::Couplable_Envelope<float> Couple[2];



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
        Main[0].set_child(&Couple[0]);
        Main[1].set_child(&Couple[1]);

        #include "Theme/setDefaultInit.h"
    }

    int currentPolyphony = 1;
    int currentBanks = 1;
    int loopCounter = 0;
    float Gates[2] = { 0.f, 0.f };
    float Velocity[2] = { 0.f, 0.f };
    float Main_Env[2] = { 0.f, 0.f };
    float Couple_Env[2] = { 0.f, 0.f };
    bool ASR_set[2] = { false, false };
    bool ASR_reset[2] = { false, false };
    bool spdMain_set[2] = { false, false };
    bool spdMain_reset[2] = { false, false };
    bool spdCoup_set[2] = { false, false };
    bool spdCoup_reset[2] = {false, false};
    bool isTriggered[2] = { false, false };
    bool isinGate[2] = { false, false };
    bool isinAttackM[2] = { false, false };
    bool isinAttackC[2] = { false, false };
    bool isinReleaseM[2] = { false, false };
    bool isinReleaseC[2] = { false, false };
    bool isinShape[2] = { false, false };
    bool isinOffset[2] = { false, false };
    bool auto_velocity_mode = false;

    void process(const ProcessArgs& args) override {

        if (loopCounter % 4 == 0) {
            setParams(args);         
        }
        generateOutput(args);

        if (loopCounter % 8 == 0) {
            doLights(args);
        }
        
        ++loopCounter;
        if (loopCounter % 2520 == 0) {
            loopCounter = 0;
        }
    }

    void setParams(const ProcessArgs& args) {

        for (int i = 0; i < 2; ++i) {
            latchButton(params[MODE_BUTTON_PARAM + i].value, &ASR_set[i], &ASR_reset[i]);
            latchButton(params[SPEED_FMAIN_BUTTON_PARAM + i].value, &spdMain_set[i], &spdMain_reset[i]);
            latchButton(params[SPEED_FCOMP_BUTTON_PARAM + i].value, &spdCoup_set[i], &spdCoup_reset[i]);
            isinGate[i] = inputs[GATE_INPUT + i].isConnected();
            isinAttackM[i] = inputs[AMAIN_INPUT + i].isConnected();
            isinAttackC[i] = inputs[ACOMP_INPUT + i].isConnected();
            isinReleaseM[i] = inputs[RMAIN_INPUT + i].isConnected();
            isinReleaseC[i] = inputs[RCOMP_INPUT + i].isConnected();
            isinShape[i] = inputs[SHAPE_INPUT + i].isConnected();
            isinOffset[i] = inputs[DELAY_INPUT + i].isConnected();

            Main[i].set_asr_mode(ASR_set[i]);
            Couple[i].set_asr_mode(ASR_set[i]);
        }
        
    }

    void generateOutput(const ProcessArgs& args) {
        for (int i = 0; i < 2; ++i) {
            

            float attackM = params[AMAIN_PARAM + i].value;
            attackM += (isinAttackM[i]) ? (inputs[AMAIN_INPUT + i].getVoltage(0) / 5.f) : 0.f;
            attackM = rack::math::clamp(attackM, 0.0001f, 1.f);
            float releaseM = params[RMAIN_PARAM + i].value; 
            releaseM += (isinReleaseM[i]) ? (inputs[RMAIN_INPUT + i].getVoltage(0) / 5.f) : 0.f;
            releaseM = rack::math::clamp(releaseM, 0.0001f, 1.f);
            float attackC = params[ACOMP_PARAM + i].value;
            attackC += (isinAttackC[i]) ? (inputs[ACOMP_INPUT + i].getVoltage(0) / 5.f) : 0.f;
            attackC = rack::math::clamp(attackC, 0.0001f, 1.f);
            float releaseC = params[RCOMP_PARAM + i].value;
            releaseC += (isinReleaseC[i]) ? (inputs[RCOMP_INPUT + i].getVoltage(0) / 5.f) : 0.f;
            releaseC = rack::math::clamp(releaseC, 0.0001f, 1.f);

            float fastM = spdMain_set[i] ? 0.5f : 12.f;
            float fastC = spdCoup_set[i] ? 0.5f : 12.f;
            attackM = (attackM * attackM) * fastM;
            releaseM = (releaseM * releaseM) * fastM;
            attackC = (attackC * attackC) * fastC;
            releaseC = (releaseC * releaseC) * fastC;


            Main[i].setAttackRelease(attackM, releaseM);
            Couple[i].setAttackRelease(attackC, releaseC);

            float delay = params[DELAY_PARAM + i].value;
            delay += (isinOffset[i]) ? abs(inputs[DELAY_INPUT + i].getVoltage(0) / 5.f) : 0.f;
            delay = rack::math::clamp(delay, 0.001f, 1.f);

            Couple[i].set_delay_from_parent(delay);

            float gateprev = Gates[i];
            Gates[i] = (isinGate[i]) ? inputs[GATE_INPUT + i].getVoltage(0) : 0.f;
            bool high = Gates[i] > 0.5f;
            Main[i].trigger(high, args.sampleTime);
            Main[i].process(args.sampleTime);

            float shape = params[SHAPE_PARAM + i].value;
            shape *= (isinShape[i]) ? (inputs[SHAPE_INPUT + i].getVoltage(0) / 5.f) : 1.f;
            shape = rack::math::clamp(shape, -0.99f, 0.99f);

            Main_Env[i] = Main[i].getEnvelope();
            Main_Env[i] = normalCurve(-1.f, 1.f, Main_Env[i], shape);
            Couple_Env[i] = Couple[i].getEnvelope();
            Couple_Env[i] = normalCurve(-1.f, 1.f, Couple_Env[i], shape);
            if (Main[i].is_attacking()) {
                //if gates have some slew, wait til they reach their maximum to stop assigning velocity
                if (gateprev < Gates[i]) {
                    Velocity[i] = Gates[i];
                }
            }
            //once sustaining, allow gate voltage to directly affect volume
            else if (Main[i].is_sustaining()) {
                Velocity[i] = Gates[i];
            }
            float peak = auto_velocity_mode ? Velocity[i] : 10.f;
            outputs[ENVMAIN_OUTPUT + i].setVoltage(Main_Env[i] * peak, 0);
            outputs[ENVCOMP_OUTPUT + i].setVoltage(Couple_Env[i] * peak, 0);

            float EOCM = Main[i].is_EOC() * 10.f;
            float EOCC = Couple[i].is_EOC() * 10.f;
            outputs[EOCMAIN_OUTPUT + i].setVoltage(EOCM, 0);
            outputs[EOCCOMP_OUTPUT + i].setVoltage(EOCC, 0);
        }
    }

    void doLights(const ProcessArgs& args) {
        float Hue;
        switch (currPanel) {
        default: {

        }
        case 0: {
            Hue = 136.f;
            break;
        }
        case 1: {
            Hue = 201.f;
            break;
        }
        case 2: {
            Hue = 325;
            break;
        }
        case 3: {
            Hue = 26;
            break;
        }
        case 4 : {
            Hue = 256;
            break;
        }
        case 5: {
            Hue = 60;
            break;
        }
        }
        for (int l = 0; l < 2; ++l) {
            int i = l * 3;
            float enVal = Main_Env[l];
            float coVal = Couple_Env[l];

            //float hu = ((this->currPanel / 5.f) + (enVal / 5.f)) * 360.f;
            float r1, g1, b1, r2, g2, b2;
            Components::HSLtoRGB(Hue + enVal, 0.98, enVal * 0.56f, &r1, &g1, &b1);
            Components::HSLtoRGB(Hue + coVal, 0.98, coVal * 0.56f, &r2, &g2, &b2);
            lights[MOUNT_LIGHT + i + 0].setBrightness(enVal * r1 / 255.f);
            lights[MOUNT_LIGHT + i + 1].setBrightness(enVal * g1 / 255.f);
            lights[MOUNT_LIGHT + i + 2].setBrightness(enVal * b1 / 255.f);
            lights[HILL_LIGHT + i + 0].setBrightness(coVal * r2 / 255.f);
            lights[HILL_LIGHT + i + 1].setBrightness(coVal * g2 / 255.f);
            lights[HILL_LIGHT + i + 2].setBrightness(coVal * b2 / 255.f);

        }
        lights[ASR_LEFT_LIGHT].setBrightness(ASR_set[0]);
        lights[ASR_RIGHT_LIGHT].setBrightness(ASR_set[1]);
        lights[SPEEDMAIN_LEFT_LIGHT].setBrightness(spdMain_set[0]);
        lights[SPEEDMAIN_RIGHT_LIGHT].setBrightness(spdMain_set[1]);
        lights[SPEEDCOMP_LEFT_LIGHT].setBrightness(spdCoup_set[0]);
        lights[SPEEDCOMP_RIGHT_LIGHT].setBrightness(spdCoup_set[1]);
    }

    json_t* dataToJson() override {
        json_t* rootJ = json_object();

        json_t* panelJ = json_integer(currPanel);
        json_object_set_new(rootJ, "Panel", panelJ);

        json_t* ASR1J = json_boolean(ASR_set[0]);
        json_t* ASR2J = json_boolean(ASR_set[1]);
        json_t* SPDMAIN1J = json_boolean(spdMain_set[0]);
        json_t* SPDMAIN2J = json_boolean(spdMain_set[1]);
        json_t* SPDCOMP1J = json_boolean(spdCoup_set[0]);
        json_t* SPDCOMP2J = json_boolean(spdCoup_set[1]);
        json_t* VelocityJ = json_boolean(auto_velocity_mode);

        json_object_set_new(rootJ, "ASR1", ASR1J);
        json_object_set_new(rootJ, "ASR2", ASR2J);
        json_object_set_new(rootJ, "SPEEDMAIN1", SPDMAIN1J);
        json_object_set_new(rootJ, "SPEEDMAIN2", SPDMAIN2J);
        json_object_set_new(rootJ, "SPEEDCOMP1", SPDCOMP1J);
        json_object_set_new(rootJ, "SPEEDCOMP2", SPDCOMP2J);
        json_object_set_new(rootJ, "Velocity", VelocityJ);

        return rootJ;
    }

    void dataFromJson(json_t* rootJ) override {
       
        json_t* panelJ = json_object_get(rootJ, "Panel");
        if (panelJ) currPanel = json_integer_value(panelJ);

        json_t* ASR1J = json_object_get(rootJ, "ASR1");
        json_t* ASR2J = json_object_get(rootJ, "ASR2");
        json_t* SPDMAIN1J = json_object_get(rootJ, "SPEEDMAIN1");
        json_t* SPDMAIN2J = json_object_get(rootJ, "SPEEDMAIN2");
        json_t* SPDCOMP1J = json_object_get(rootJ, "SPEEDCOMP1");
        json_t* SPDCOMP2J = json_object_get(rootJ, "SPEEDCOMP2");
        json_t* VelocityJ = json_object_get(rootJ, "Velocity");
        ASR_set[0] = json_boolean_value(ASR1J);
        ASR_set[1] = json_boolean_value(ASR2J);
        spdMain_set[0] = json_boolean_value(SPDMAIN1J);
        spdMain_set[1] = json_boolean_value(SPDMAIN2J);
        spdCoup_set[0] = json_boolean_value(SPDCOMP1J);
        spdCoup_set[1] = json_boolean_value(SPDCOMP2J);
        auto_velocity_mode = json_boolean_value(VelocityJ);
    }

};

struct LeftDobbsLight1 : Components::TColorSVGLight {
    LeftDobbsLight1() {
        this->setSvg(Svg::load(asset::plugin(pluginInstance, "res/DobbsLights/MountDobbsLeft.svg")));
    }
};
struct LeftDobbsLight2 : Components::TColorSVGLight {
    LeftDobbsLight2() {
        this->setSvg(Svg::load(asset::plugin(pluginInstance, "res/DobbsLights/MountDobbsLeft2.svg")));
    }
};
struct RightDobbsLight1 : Components::TColorSVGLight {
    RightDobbsLight1() {
        this->setSvg(Svg::load(asset::plugin(pluginInstance, "res/DobbsLights/MountDobbsRight.svg")));
    }
};
struct RightDobbsLight2 : Components::TColorSVGLight {
    RightDobbsLight2() {
        this->setSvg(Svg::load(asset::plugin(pluginInstance, "res/DobbsLights/MountDobbsRight2.svg")));
    }
};

using namespace LydD::Components;
struct DobbsPanelWidget : ModuleWidget {


    #include "Theme/LogoLight.h"
    std::string panel;

    DobbsPanelWidget(DobbsModule* module) {
        setModule(module);
        panel = PANEL;
        //set panel on init
        #include "Theme/initChoosePanel.h"

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

        //mountain shaped lights to follow envelopes
        addChild(createLight<LeftDobbsLight1>(Vec(0, 33), module, DobbsModule::MOUNT_LIGHT + 0));
        addChild(createLight<LeftDobbsLight2>(Vec(65, 36.5), module, DobbsModule::HILL_LIGHT + 0));
        addChild(createLight<RightDobbsLight1>(Vec(116, 25), module, DobbsModule::MOUNT_LIGHT + 3));
        addChild(createLight<RightDobbsLight2>(Vec(55.5, 28), module, DobbsModule::HILL_LIGHT + 3));

        if (module) {

            //must be called 'logoPos'for all modules
            Vec logoPos = Vec(((15.f * HP) / 2.f) - 12.5, 363.f);
            DobbsModule* module = dynamic_cast<DobbsModule*>(this->module);
            assert(module);
            #include "Theme/LogoChild.h" 
        }
    }
    
    //give struct to menu containing panel options
    #include "Theme/PanelList.h" 

    void appendContextMenu(Menu* menu) override {
        DobbsModule* module = dynamic_cast<DobbsModule*>(this->module);
        assert(module);

        menu->addChild(new MenuSeparator());

        //wow checkbox lambdas, how unique
        menu->addChild(createCheckMenuItem("Auto-Velocity", "Gate Voltage determines peak height",
            [=]() {return module->auto_velocity_mode != false; },
            [=]() {module->auto_velocity_mode ^= true; }
        ));

        menu->addChild(new MenuSeparator());

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

Model* modelDobbs = createModel<DobbsModule, DobbsPanelWidget>("Dobbs-Env");