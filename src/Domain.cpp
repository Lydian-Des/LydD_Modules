#include "plugin.hpp"

#define MODULE_NAME SwitchModule
#define PANEL "Switch_panel.svg"
#define HP 10

using namespace LydD;




static const int maxPolyphony = 1;

struct SwitchModule : Module
{
    enum ParamIds {
        BIG_BUTTON_PARAM,
        BIG_MODE_BUTTON_PARAM,
        ENUMS(TO1_BUTTON_PARAMS, 4),
        ENUMS(TO2_BUTTON_PARAMS, 4),//not atually using this at all, one button is shared on both sides
        ENUMS(MODE_BUTTON_PARAMS, 4),
        NUM_PARAMS
    };
    enum InputIds {
        BIG_SWITCH_INPUT,
        ENUMS(TO1_SWITCH_INPUTS, 4),
        ENUMS(TO2_SWITCH_INPUTS, 4), 
        ENUMS(THIS_INPUTS, 4), //top(1) input     }
        ENUMS(THAT_INPUTS, 4), //bottom(2) input  } 2 in 1 out
        ENUMS(TOTHER_INPUTS, 4), // 1 input  ]1 in 2 out
        NUM_INPUTS
    };
    enum OutputIds {

        ENUMS(TOTHER_OUTPUTS, 4),
        ENUMS(THIS_OUTPUTS, 4),
        ENUMS(THAT_OUTPUTS, 4),
        NUM_OUTPUTS
    };
    enum LightIds {
        ENUMS(MASTER_LIGHT, 3),
        ENUMS(SWITCH_LIGHT, 12),
        NUM_LIGHTS
    };

    /*BaseFunctions Functions;
    BaseButtons Buttons;*/

    SwitchModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configSwitch(BIG_BUTTON_PARAM, 0.f, 1.f, 0.f, "Master Button");
        configSwitch(BIG_MODE_BUTTON_PARAM, 0.f, 1.f, 0.f, "Master Latch");
        configInput(BIG_SWITCH_INPUT, "Master Switch");

        for (int i = 0; i < 4; ++i) {
            configSwitch(TO1_BUTTON_PARAMS + i, 0.f, 1.f, 0.f, "Switch");
            configSwitch(MODE_BUTTON_PARAMS + i, 0.f, 1.f, 0.f, "Latch");

            configInput(TO1_SWITCH_INPUTS + i, "Switch 2->1 - " + std::to_string(i + 1));
            configInput(TO2_SWITCH_INPUTS + i, "Switch 1->2 - " + std::to_string(i + 1));

            configInput(THIS_INPUTS + i, "A - " + std::to_string(i + 1));
            configInput(THAT_INPUTS + i, "B - " + std::to_string(i + 1));
            configInput(TOTHER_INPUTS + i, "C - " + std::to_string(i + 1));
            configOutput(TOTHER_OUTPUTS + i, "2->1 - C - " + std::to_string(i + 1));
            configOutput(THIS_OUTPUTS + i, "1->2 - A - " + std::to_string(i + 1));
            configOutput(THAT_OUTPUTS + i, "1->2 - B - " + std::to_string(i + 1));


        }

    }

    int currentPolyphony = 1;
    int currentBanks = 1;
    int loopCounter = 0;
   
    int masterMode = 0; // 0 for Hold While High, 1 for Toggle
    bool masterModeSet = false;
    bool masterModeReset = false;
    bool masterSwitchPar = false;
    bool masterSwitchIn = false;
    bool masterSwitchVal = false;
    bool isinMaster = false;
    bool masterSet = false;
    bool masterReset = false;
    bool masterLatchSet = false;
    bool masterLatchReset = false;


    int mode[4]{ 0, 0, 0, 0 };
    bool modeSet[4]{ 0, 0, 0, 0 };      //mode switches[buttons]
    bool modeReset[4]{ 0, 0, 0, 0 };
    float masterSwitchLight = 0.f;

    bool isinThisLeft[4]{ 0, 0, 0, 0 };
    bool isinThatLeft[4]{ 0, 0, 0, 0 };     //bools for inputs
    bool isinTotherRight[4]{ 0, 0, 0, 0 };
    bool isinSwitchLeft[4]{ 0, 0, 0, 0 };
    bool isinSwitchRight[4]{ 0, 0, 0, 0 };

    float ThisIn[4]{ 0, 0, 0, 0 };
    float ThatIn[4]{ 0, 0, 0, 0 };  // this or that go to t'other
    float TotherOut[4]{ 0, 0, 0, 0 };
    bool switchParLeft[4]{ 0, 0, 0, 0 };   
    bool switchInLeft[4]{ 0, 0, 0, 0 };   
    bool switchValLeft[4]{ 0, 0, 0, 0 };
    bool latchResetLeft[4]{ 0, 0, 0, 0 };
    float sLL[4]{ 0, 0, 0, 0 };

    float TotherIn[4]{ 0, 0, 0, 0 };
    float ThisOut[4]{ 0, 0, 0, 0 };  // t'other goes to this or that
    float ThatOut[4]{ 0, 0, 0, 0 };
    bool switchParRight[4]{ 0, 0, 0, 0 };
    bool switchInRight[4]{ 0, 0, 0, 0 };
    bool switchValRight[4]{ 0, 0, 0, 0 };
    bool latchResetRight[4]{ 0, 0, 0, 0 };
    float sLR[4]{ 0, 0, 0, 0 };


    void process(const ProcessArgs& args) override {
   
        if (loopCounter % 8 == 0) {
            setParams(args);    
            setLights(args);
        }
        generateOutput(args);

        ++loopCounter;
        if (loopCounter % 2520 == 0) {
            loopCounter = 0;
        }
    }

    void setParams(const ProcessArgs& args) {
        //Buttons.momentButton(params[BIG_BUTTON_PARAM].value, &masterSet, &masterReset);
        latchButton(params[BIG_MODE_BUTTON_PARAM].value, &masterModeSet, &masterModeReset);
        masterMode = masterModeSet;
        isinMaster = inputs[BIG_SWITCH_INPUT].isConnected();
        masterSwitchPar = params[BIG_BUTTON_PARAM].value >= 1.f;
        for (int m = 0; m < 4; ++m) {
            latchButton(params[MODE_BUTTON_PARAMS + m].value, &modeSet[m], &modeReset[m]);
            mode[m] = modeSet[m];

            isinThisLeft[m] = inputs[THIS_INPUTS + m].isConnected();
            isinThatLeft[m] = inputs[THAT_INPUTS + m].isConnected();
            isinSwitchLeft[m] = inputs[TO1_SWITCH_INPUTS + m].isConnected();
            
            isinTotherRight[m] = inputs[TOTHER_INPUTS + m].isConnected();
            isinSwitchRight[m] = inputs[TO2_SWITCH_INPUTS + m].isConnected();

            switchParLeft[m] = params[TO1_BUTTON_PARAMS + m].value >= 1.f;
            switchParRight[m] = params[TO2_BUTTON_PARAMS + m].value >= 1.f;
        }


    }

    void setLights(const ProcessArgs& args) {
        masterSwitchLight = static_cast<float>(masterSwitchVal);
        for (int s = 0; s < 4; ++s) {
            sLL[s] = static_cast<float>(switchValLeft[s]);
            sLR[s] = static_cast<float>(switchValRight[s]);
        }
        lights[MASTER_LIGHT + 0].setBrightness(0.13f * masterSwitchLight);
        lights[MASTER_LIGHT + 1].setBrightness(0.75f * masterSwitchLight);
        lights[MASTER_LIGHT + 2].setBrightness(0.58f * masterSwitchLight);
        for (int l = 0; l < 4; ++l) {
            lights[SWITCH_LIGHT + (l * 3) + 0].setBrightness(0.64 * sLL[l]);
            lights[SWITCH_LIGHT + (l * 3) + 1].setBrightness(0.47 * sLL[l] + 0.49 * sLR[l]);
            lights[SWITCH_LIGHT + (l * 3) + 2].setBrightness(0.84 * sLR[l]);
        }

    }

    void generateOutput(const ProcessArgs& args) {
        bool master = isinMaster ? (inputs[BIG_SWITCH_INPUT].getVoltage(0) > 0.5f) : masterSwitchPar;
        if (masterMode == 0) {
            masterSwitchVal = master;
        }
        else if (masterMode == 1) {
            latchButton((float)master, &masterLatchSet, &masterLatchReset);
            masterSwitchVal = masterLatchSet;
        }
        //run thru each switch
        for (int s = 0; s < 4; ++s) {
            float thisInput = isinThisLeft[s] ? inputs[THIS_INPUTS + s].getVoltage(0) : 0.f;
            float thatInput = isinThatLeft[s] ? inputs[THAT_INPUTS + s].getVoltage(0) : 0.f;
            float TotherOutput = 0.f;
            bool switchLeft = (isinSwitchLeft[s]) ? (inputs[TO1_SWITCH_INPUTS + s].getVoltage(0)) > 0.5f : switchParLeft[s];

            
            bool switchRight = (isinSwitchRight[s]) ? (inputs[TO2_SWITCH_INPUTS + s].getVoltage(0)) > 0.5f : switchLeft;

            if (masterSwitchVal >= 1.f) {
                switchLeft = !switchLeft;
                switchRight = !switchRight;
            }
            if (mode[s] == 0) {
                switchValLeft[s] = switchLeft;
                switchValRight[s] = switchRight;
            }
            else if (mode[s] == 1) {
                latchButton((float)switchLeft, &switchValLeft[s], &latchResetLeft[s]);
                latchButton((float)switchRight, &switchValRight[s] , &latchResetRight[s]);
            }
            TotherOutput = (switchValLeft[s]) ? thatInput : thisInput;
            outputs[TOTHER_OUTPUTS + s].setVoltage(TotherOutput, 0);
           

            float totherInput = isinTotherRight[s] ? inputs[TOTHER_INPUTS + s].getVoltage(0) : TotherOutput;
            float ThisOutput = 0.f;
            float ThatOutput = 0.f;

            ThisOutput = (!switchValRight[s]) ? totherInput : 0.f;
            ThatOutput = (switchValRight[s]) ? totherInput : 0.f;
            outputs[THIS_OUTPUTS + s].setVoltage(ThisOutput, 0);
            outputs[THAT_OUTPUTS + s].setVoltage(ThatOutput, 0);


        }

    }

    json_t* dataToJson() override {
        json_t* rootJ = json_object();

        json_t* mastermodeJ = json_boolean(masterMode);
        json_object_set_new(rootJ, "mastermode", mastermodeJ);

        json_t* modearrayJ = json_array();
        for (int ind : mode) {
            json_t* set = json_boolean(mode[ind]);
            json_array_append_new(modearrayJ, set);
        }
        json_object_set_new(rootJ, "modearray", modearrayJ);

        return rootJ;
    }

    void dataFromJson(json_t* rootJ) override {
       
        json_t* mastermodeJ = json_object_get(rootJ, "mastermode");
        masterMode = json_boolean_value(mastermodeJ);
        masterModeSet = masterMode;
        json_t* modearrayJ = json_object_get(rootJ, "modearray");
        if (modearrayJ) {
            size_t i = 0;
            json_t* get;
            json_array_foreach(modearrayJ, i, get) {
                mode[i] = json_boolean_value(get);
                modeSet[i] = mode[i];
            }

        }
    }

};

struct SwitchLight : Components::TColorSVGLight {
    SwitchLight() {
        this->setSvg(Svg::load(asset::plugin(pluginInstance, "res/LilButtonLight.svg")));
    }
};
struct BigLight : Components::TColorSVGLight {
    BigLight() {
        this->setSvg(Svg::load(asset::plugin(pluginInstance, "res/BigButtonLight.svg")));
    }
};

using namespace LydD::Components;
struct SwitchPanelWidget : ModuleWidget {

    //include struct for logo here so it has modules name
    #include "Theme/LogoLight.h"

    SwitchPanelWidget(SwitchModule* module) {
        setModule(module);
        LydD::Components::setPlugin(pluginInstance);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Switch_panel.svg")));

		addChild(createWidget<ScrewSilver>(Vec(15, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 30, 0)));
		addChild(createWidget<ScrewSilver>(Vec(15, 365)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 30, 365)));

        addParam(createParam<BigButton>(Vec(50, 50), module, SwitchModule::BIG_BUTTON_PARAM));
        addParam(createParam<VCVButton>(Vec(116, 66.5), module, SwitchModule::BIG_MODE_BUTTON_PARAM));
        addInput(createInput<PurplePort>(Vec(14.5, 64.5), module, SwitchModule::BIG_SWITCH_INPUT));

        float leftsideX = 10;       
        float rightsideX = 87;
        float Xdist = 30;
        float YY = 114;
        float YYdist = 31;
        float Yspacing = 62;
        float switchX = 63.5;
        float butX = 65.5;
        float switchY = 130;
        float butY = 161.5;

        for (int p = 0; p < 4; ++p) {
            addParam(createParam<LilButton>(Vec(switchX, switchY + (Yspacing * p)), module, SwitchModule::TO1_BUTTON_PARAMS + p));
            addParam(createParam<VCVButton>(Vec(butX, butY + (Yspacing * p)), module, SwitchModule::MODE_BUTTON_PARAMS + p));

            addInput(createInput<PurplePort>(Vec(leftsideX, YY + (Yspacing * p)), module, SwitchModule::THIS_INPUTS + p));
            addInput(createInput<PurplePort>(Vec(leftsideX, YY + YYdist + (Yspacing * p)), module, SwitchModule::THAT_INPUTS + p));
            addInput(createInput<PurplePort>(Vec(leftsideX + Xdist, YY + YYdist + (Yspacing * p)), module, SwitchModule::TO1_SWITCH_INPUTS + p));
            addOutput(createOutput<PurplePort>(Vec(leftsideX + Xdist, YY + (Yspacing * p)), module, SwitchModule::TOTHER_OUTPUTS + p));

            addInput(createInput<PurplePort>(Vec(rightsideX, YY + (Yspacing * p)), module, SwitchModule::TOTHER_INPUTS + p));
            addInput(createInput<PurplePort>(Vec(rightsideX, YY + YYdist + (Yspacing * p)), module, SwitchModule::TO2_SWITCH_INPUTS + p));
            addOutput(createOutput<PurplePort>(Vec(rightsideX + Xdist, YY + (Yspacing * p)), module, SwitchModule::THIS_OUTPUTS + p));
            addOutput(createOutput<PurplePort>(Vec(rightsideX + Xdist, YY + YYdist + (Yspacing * p)), module, SwitchModule::THAT_OUTPUTS + p));
        }

        

        if (module) {
            addChild(createLight<BigLight>(Vec(50, 50), module, SwitchModule::MASTER_LIGHT + 0));
            addChild(createLight<SwitchLight>(Vec(switchX, switchY), module, SwitchModule::SWITCH_LIGHT + 0));
            addChild(createLight<SwitchLight>(Vec(switchX, switchY + Yspacing), module, SwitchModule::SWITCH_LIGHT + 3));
            addChild(createLight<SwitchLight>(Vec(switchX, switchY + Yspacing * 2), module, SwitchModule::SWITCH_LIGHT + 6));
            addChild(createLight<SwitchLight>(Vec(switchX, switchY + Yspacing * 3), module, SwitchModule::SWITCH_LIGHT + 9));
            //must be called 'logoPos'for all modules 
            Vec logoPos = Vec(((15.f * HP) / 3.5f) - 12.5, 363.f);
            SwitchModule* module = dynamic_cast<SwitchModule*>(this->module);
            assert(module);
            #include "Theme/LogoChild.h"
        }
    }
    
};

Model* modelSwitch = createModel<SwitchModule, SwitchPanelWidget>("Master-Switch");