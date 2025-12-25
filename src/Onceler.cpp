#include "plugin.hpp"
#include <vector>
#include <string>


static const int maxPolyphony = 1;

static rack::simd::float_4 Zero{ 0.f };
static rack::simd::float_4 One{ 1.f }; 

struct Oneder {
    int Cuts;
    bool Redo;
    int toFell;
    int toWait;
    rack::dsp::BooleanTrigger _Hit;
    rack::dsp::PulseGenerator _trig;
    Oneder() {
        this->Cuts = -1;
        this->Redo = false;
        this->toFell = 1;
        this->toWait = 1;
        this->_Hit.reset();
    }

    void reDont() {
        this->Cuts = -1;
    }
    void reDo() {
        this->Cuts = 0;
    }

    void isBarren() {
        this->Cuts = std::min(this->Cuts, 2520); //end of time, wait for reset.
    }
    void setStrength(float treeStrength, float rotTime) {
        this->toFell = treeStrength;
        this->toWait = rotTime;
    }
    void Chop(float axe, bool* hit) {
        bool isHit = this->_Hit.process(axe >= 1.f);
        this->Cuts += isHit;
        *hit = isHit;
        //this->toFell = treeStrength;
        //this->toWait = rotTime;
        isBarren();
    }
    void cutDown(bool* isFell, int type, float sampleTime) {
        _trig.process(sampleTime);
        bool Felled = false;
        bool tFell = true;
        switch (type) {       
        case 0: { //Hold for N
            Felled = (this->Cuts >= this->toFell) && (this->Cuts <= this->toFell + this->toWait);
            break;
        }
        case 1: { //Auto Reset
            Felled = this->Cuts >= this->toFell;
            if (this->Cuts >= this->toFell + this->toWait) this->reDo();
            break;
        }
        case 2: { //Latch
            Felled = this->Cuts >= this->toFell;
            break;
        }

        case 3: { // Trig 
            bool equal = this->Cuts == this->toFell;
            if (equal && tFell) {
                this->_trig.trigger(0.08);
                tFell = false;
            }
            tFell = this->Cuts != this->toFell;
            Felled = this->_trig.isHigh();
            break;
        }
        case 4: { // AutoTrig 
            bool equal = (this->Cuts % this->toFell == 0) && this->Cuts != 0;
            if (equal && tFell) {
                this->_trig.trigger(0.08);
                tFell = false;
            }
            tFell = this->Cuts % this->toFell != 0;
            Felled = this->_trig.isHigh();
            break;
        }
        case 5: { // Not Not
            Felled = (this->Cuts < this->toFell) || (this->Cuts > this->toFell + this->toWait);
            break;
        }
        case 6: { //Auto Not Not
            Felled = (this->Cuts < this->toFell) || (this->Cuts > this->toFell + this->toWait);
            if (this->Cuts >= this->toFell + this->toWait) this->reDo();
            break;
        }
        }
        *isFell = Felled;
    }

    
};

struct OncelerModule : Module
{
    enum ParamIds {
        ENUMS(CUTS_PARAMS, 4),
        ENUMS(HOLDT_BUTTON_PARAMS, 4),
        ENUMS(REDONT_BUTTON_PARAMS, 4),
        ENUMS(WAIT_PARAMS, 4),
        NUM_PARAMS
	};
    enum InputIds {

        ENUMS(AXE_INPUTS, 4),
        ENUMS(REDONT_INPUTS, 4),
        ENUMS(SPEAK_INPUTS, 4),
        ENUMS(CUTS_INPUTS, 4),
        ENUMS(WAIT_INPUTS, 4),
        NUM_INPUTS
	};
    enum OutputIds {

        ENUMS(CHOP_OUTPUTS, 4),
        ENUMS(TALK_OUTPUTS, 4),
        NUM_OUTPUTS
	};
	enum LightIds {
        ENUMS(HOLD_LIGHTS, 12),
        NUM_LIGHTS
    };

    BaseFunctions Functions;
    BaseButtons Buttons;
    Oneder Trees[4];

    OncelerModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        for (int i = 0; i < 4; ++i) {
            configParam(CUTS_PARAMS + i, 1.f, 16.f, 4.f, "Cuts to Fell");
            paramQuantities[CUTS_PARAMS + i]->snapEnabled = true;
            configParam(HOLDT_BUTTON_PARAMS + i, 0.f, 1.f, 0.f, "Hold Type");
            configParam(REDONT_BUTTON_PARAMS + i, 0.f, 1.f, 0.f, "Re-don't");
            configParam(WAIT_PARAMS + i, 1.f, 16.f, 1.f,"Wait");
            paramQuantities[WAIT_PARAMS + i]->snapEnabled = true;
            std::string count = std::to_string(i + 1);
            std::string axeCount = "Axe Chops - ";
                axeCount += count;
            configInput(AXE_INPUTS + i, axeCount);   

            std::string redontCount = "ReDon't - ";
                redontCount += count;
            configInput(REDONT_INPUTS + i, redontCount);

            std::string speakCount = "Speak - ";
            speakCount += count;
            configInput(SPEAK_INPUTS + i, speakCount);

            std::string cutCount = "Cuts - ";
                cutCount += count;
            configInput(CUTS_INPUTS + i, cutCount);

            std::string waitCount = "Wait - ";
                waitCount += count;
            configInput(WAIT_INPUTS + i, waitCount);

            std::string chopCount = "Tree Felled - ";
            chopCount += count;
            configOutput(CHOP_OUTPUTS + i, chopCount);

            std::string talkCount = "Talk - ";
            talkCount += count;
            configOutput(TALK_OUTPUTS + i, talkCount);
        }

    }

    int currentPolyphony = 1;
    int currentBanks = 1;
    int loopCounter = 0;
    rack::dsp::BooleanTrigger _reDontsPar[4];
    rack::dsp::BooleanTrigger _reDonts[4];
    bool isinAxes[4]{ false };
    bool isinResets[4]{ false };
    bool isinSpeaks[4]{ false };
    bool isinCuts[4]{ false };
    bool isinWaits[4]{ false };
    bool redont[4]{ false };
    bool holdTypeSet[4]{ false };
    bool update[4]{ false };
    int holdType[4]{ 0 };
    int cutPar[4]{ 1 };
    int waitPar[4]{ 1 };
    float Cutting[4]{ 0 };
    float Waiting[4]{ 0 };
    bool stumpOut[4]{ false }; //output boolean High after # of gates
    int chops[4]{ 0 };
    int maxChops[4]{ 1 };
    int waitTicks[4]{ 1 };

    void process(const ProcessArgs& args) override {

        if (loopCounter % 16 == 0) {
            checkInputs(args);  
            doLights(args);
        }
        generateOutput(args);
        loopCounter++;

        if (loopCounter % 2520 == 0) {
            loopCounter = 0;
        }
    }

    void checkInputs(const ProcessArgs& args) {

        for (int o = CHOP_OUTPUTS + 0; o != NUM_OUTPUTS; ++o) {
            outputs[o].setChannels(1);
        }

        for (int i = 0; i < 4; ++i) {
            Buttons.incrementButton(params[HOLDT_BUTTON_PARAMS + i].value, &holdTypeSet[i], 7, &holdType[i]);
            switch (holdType[i]) {
            case 0: {
                paramQuantities[HOLDT_BUTTON_PARAMS + i]->name = "Hold For _";
                break;
            }
            case 1: {
                paramQuantities[HOLDT_BUTTON_PARAMS + i]->name = "Auto Reset";
                break;
            }
            case 2: {
                paramQuantities[HOLDT_BUTTON_PARAMS + i]->name = "Latch";
                break;
            }
            case 3: {
                paramQuantities[HOLDT_BUTTON_PARAMS + i]->name = "Trigger";
                break;
            }
            case 4: {
                paramQuantities[HOLDT_BUTTON_PARAMS + i]->name = "Auto Trigger";
                break;
            }
            case 5: {
                paramQuantities[HOLDT_BUTTON_PARAMS + i]->name = "Not Not";
                break;
            }
            case 6: {
                paramQuantities[HOLDT_BUTTON_PARAMS + i]->name = "Auto Not Not";
                break;
            }
            }

            isinAxes[i] = inputs[AXE_INPUTS + i].isConnected();
            isinResets[i] = inputs[REDONT_INPUTS + i].isConnected();
            isinSpeaks[i] = inputs[SPEAK_INPUTS + i].isConnected();
            isinCuts[i] = inputs[CUTS_INPUTS + i].isConnected();
            isinWaits[i] = inputs[WAIT_INPUTS + i].isConnected();
            cutPar[i] = params[CUTS_PARAMS + i].value;
            waitPar[i] = params[WAIT_PARAMS + i].value;
        }


    }

   

    void generateOutput(const ProcessArgs& args) {
        //cascade inputs
        float Axes[4]{ 0 };
        Axes[0] = (isinAxes[0]) ? inputs[AXE_INPUTS + 0].getVoltage(0) : 0.f;
        Axes[1] = (isinAxes[1]) ? inputs[AXE_INPUTS + 1].getVoltage(0) : Axes[0];
        Axes[2] = (isinAxes[2]) ? inputs[AXE_INPUTS + 2].getVoltage(0) : Axes[1];
        Axes[3] = (isinAxes[3]) ? inputs[AXE_INPUTS + 3].getVoltage(0) : Axes[2];
        float redontPar[4]{ 0 };
        for (int i = 0; i < 4; ++i) {
            redontPar[i] = _reDontsPar[i].process(params[REDONT_BUTTON_PARAMS + i].value);
        }
        redont[0] = redontPar[0] || ((isinResets[0]) ? _reDonts[0].process(inputs[REDONT_INPUTS + 0].getVoltage(0)) : false);
        redont[1] = redontPar[1] || ((isinResets[1]) ? _reDonts[1].process(inputs[REDONT_INPUTS + 1].getVoltage(0)) : redont[0]);
        redont[2] = redontPar[2] || ((isinResets[2]) ? _reDonts[2].process(inputs[REDONT_INPUTS + 2].getVoltage(0)) : redont[1]);
        redont[3] = redontPar[3] || ((isinResets[3]) ? _reDonts[3].process(inputs[REDONT_INPUTS + 3].getVoltage(0)) : redont[2]);
            
            
        for (int i = 0; i < 4; ++i) {
            Cutting[i] = cutPar[i];
            Waiting[i] = waitPar[i];
            if (isinCuts[i]) {
                float cutIn = Functions.lerp(0.f, 15.f, 0.f, 10.f, abs(inputs[CUTS_INPUTS + i].getVoltage(0)));
                Cutting[i] = rack::math::clamp(cutPar[i] + cutIn, 1.f, 16.f);
            }
            if (isinWaits[i]) {
                float waitIn = Functions.lerp(0.f, 15.f, 0.f, 10.f, abs(inputs[WAIT_INPUTS + i].getVoltage(0)));
                Waiting[i] = rack::math::clamp(waitPar[i] + waitIn, 1.f, 16.f);
            }
            if (redont[i]) {
                Trees[i].setStrength(Cutting[i], Waiting[i]);
                if (Axes[i] > 1.f) {
                    Trees[i].reDo();
                }
                else {
                    Trees[i].reDont();
                }
            }
            
            Trees[i].Chop(Axes[i], &update[i]);
            Trees[i].cutDown(&stumpOut[i], holdType[i], args.sampleTime);
            chops[i] = Trees[i].Cuts;
            maxChops[i] = Cutting[i];
            waitTicks[i] = Waiting[i];
            if ((update[i] && !stumpOut[i]) || chops[i] == 0) {
                Trees[i].setStrength(Cutting[i], Waiting[i]);
                
            }
            float speakIn = Axes[i];
            if (isinSpeaks[i]) {
                speakIn = inputs[SPEAK_INPUTS + i].getVoltage(0);                
            }
            float talkOut = stumpOut[i];
            outputs[TALK_OUTPUTS + i].setVoltage(speakIn * talkOut, 0);
            outputs[CHOP_OUTPUTS + i].setVoltage(stumpOut[i] * 10.f, 0);
        }
      
    }

    void doLights(const ProcessArgs& args) {
        for (int h = HOLD_LIGHTS + 0; h < NUM_LIGHTS; h += 3) {
            switch (holdType[h / 3]) {
            case 0: {
                lights[h].setBrightness(0.94f);
                lights[h + 1].setBrightness(0.01f);
                lights[h + 2].setBrightness(0.3f);
                break;
            }
            case 1: {
                lights[h].setBrightness(0.88f);
                lights[h + 1].setBrightness(0.84f);
                lights[h + 2].setBrightness(0.16f);
                break;
            }
            case 2: {
                lights[h].setBrightness(0.02f);
                lights[h + 1].setBrightness(0.92f);
                lights[h + 2].setBrightness(0.08f);
                break;
            }
            case 3: {                
                lights[h].setBrightness(0.99f);
                lights[h + 1].setBrightness(0.07f);
                lights[h + 2].setBrightness(0.96f);
                break;
            }
            case 4: {
                lights[h].setBrightness(0.8f);
                lights[h + 1].setBrightness(0.15f);
                lights[h + 2].setBrightness(0.4f);
                break;
            }
            case 5: {
                lights[h].setBrightness(0.7f);
                lights[h + 1].setBrightness(0.65f);
                lights[h + 2].setBrightness(0.02f);
                break;
            }
            case 6: {
                lights[h].setBrightness(0.02f);
                lights[h + 1].setBrightness(0.17f);
                lights[h + 2].setBrightness(0.91f);
                break;
            }
            }
        }
    }

    json_t* dataToJson() override {
        json_t* rootJ = json_object();

        json_t* Hold1J = json_integer(holdType[0]);
        json_t* Hold2J = json_integer(holdType[1]);
        json_t* Hold3J = json_integer(holdType[2]);
        json_t* Hold4J = json_integer(holdType[3]);

        json_object_set_new(rootJ, "Hold1", Hold1J);
        json_object_set_new(rootJ, "Hold2", Hold2J);
        json_object_set_new(rootJ, "Hold3", Hold3J);
        json_object_set_new(rootJ, "Hold4", Hold4J);

        return rootJ;
    }

    void dataFromJson(json_t* rootJ) override {
       
        json_t* Hold1J = json_object_get(rootJ, "Hold1");
        json_t* Hold2J = json_object_get(rootJ, "Hold2");
        json_t* Hold3J = json_object_get(rootJ, "Hold3");
        json_t* Hold4J = json_object_get(rootJ, "Hold4");
        holdType[0] = json_integer_value(Hold1J);
        holdType[1] = json_integer_value(Hold2J);
        holdType[2] = json_integer_value(Hold3J);
        holdType[3] = json_integer_value(Hold4J);
        
    }

};

struct OncelerDisplay : DigitalDisplay {
    OncelerDisplay() {
        fontPath = asset::plugin(pluginInstance, "res/fonts/DSEG7ClassicMini-Bold.ttf");
        textPos = Vec(box.size.x - 2, box.size.y - 2);
        bgText = " ";
        fontSize = 16;
    }
};



struct OnceWidget : OncelerDisplay {
    const NVGcolor BLOO = nvgRGB(0x00, 0xaa, 0xff);
    OncelerModule* module;
    int flag = 0;
    void step() override {
        fgColor = BLOO;
        int cutnum = module->maxChops[flag];
        int waitnum = module->waitTicks[flag];
        if (cutnum >= 10 || waitnum >= 10) {
            fontSize = 12;
        }
        else {
            fontSize = 16;
        }
        if (module->stumpOut[flag]) {
            fgColor = rack::color::GREEN;
        }
        else if (module->chops[flag] > cutnum) {
            fgColor = rack::color::MAGENTA;
        }
        text = rack::string::f("%d|%d", cutnum, waitnum);
    }
};


struct OncelerPanelWidget : ModuleWidget {
    OncelerPanelWidget(OncelerModule* module) {
        setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Onceler_panel.svg")));

		addChild(createWidget<ScrewSilver>(Vec(15, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 30, 0)));
		addChild(createWidget<ScrewSilver>(Vec(15, 365)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 30, 365)));


        float distY = 84;

        float axeX = 8.5;
        float treeX = 120;
        float row1Y = 36;
        float row2Y = 61;
        
        float reButY = 87;
        float olrButX = 102;
        float olrButY = 86;

        float speakX = 38;
        float speakY = 74;
        
        float CWinX = 65;
        float CutsY = 61;
        float WaitY = 86;
        
        float digitX = 42;
        float digitY = 31;
        float lightX = 127;
        float lightY = 97;


        for (int i = 0; i < 12; i += 3) {
            addChild(createLightCentered<SmallLight<RedGreenBlueLight>>(Vec(lightX, lightY + (distY * (i / 3))), module, OncelerModule::HOLD_LIGHTS + i));

        }

        
        for (int i = 0; i < 4; ++i) {
            addInput(createInput<PurplePort>(Vec(axeX, row1Y + (i * distY)), module, OncelerModule::AXE_INPUTS + i));
            addInput(createInput<PurplePort>(Vec(axeX, row2Y + (i * distY)), module, OncelerModule::REDONT_INPUTS + i));
            addInput(createInput<PurplePort>(Vec(speakX, speakY + (i * distY)), module, OncelerModule::SPEAK_INPUTS + i));
            addInput(createInput<PurplePort>(Vec(CWinX, CutsY + (i * distY)), module, OncelerModule::CUTS_INPUTS + i));
            addInput(createInput<PurplePort>(Vec(CWinX, WaitY + (i * distY)), module, OncelerModule::WAIT_INPUTS + i));
        }
        for(int i = 0; i < 4; ++i) {
            addParam(createParam<RoundSmallBlackKnob>(Vec(90.5, 34 + (i * distY)), module, OncelerModule::CUTS_PARAMS + i));
            addParam(createParam<Trimpot>(Vec(94, 65 + (i * distY)), module, OncelerModule::WAIT_PARAMS + i));
            addParam(createParam<VCVButton>(Vec(12, reButY + (i * distY)), module, OncelerModule::REDONT_BUTTON_PARAMS + i));
            addParam(createParam<VCVButton>(Vec(olrButX, olrButY + (i * distY)), module, OncelerModule::HOLDT_BUTTON_PARAMS + i));
            addOutput(createOutput<PurplePort>(Vec(treeX, row1Y + (i * distY)), module, OncelerModule::CHOP_OUTPUTS + i));
            addOutput(createOutput<PurplePort>(Vec(treeX, row2Y + (i * distY)), module, OncelerModule::TALK_OUTPUTS + i));
        }


      


        if (module) {

            OnceWidget* O1Widget = createWidget<OnceWidget>((Vec(digitX, digitY)));
            O1Widget->box.size = (Vec(36, 26));
            O1Widget->textPos = (Vec(35, 22));
            O1Widget->flag = 0;
            O1Widget->module = module;
            addChild(O1Widget);

            OnceWidget* O2Widget = createWidget<OnceWidget>((Vec(digitX, digitY + (distY))));
            O2Widget->box.size = (Vec(36, 26));
            O2Widget->textPos = (Vec(35, 22));
            O2Widget->flag = 1;
            O2Widget->module = module;
            addChild(O2Widget);

            OnceWidget* O3Widget = createWidget<OnceWidget>((Vec(digitX, digitY + (distY * 2))));
            O3Widget->box.size = (Vec(36, 26));
            O3Widget->textPos = (Vec(35, 22));
            O3Widget->flag = 2;
            O3Widget->module = module;
            addChild(O3Widget);

            OnceWidget* O4Widget = createWidget<OnceWidget>((Vec(digitX, digitY + (distY * 3))));
            O4Widget->box.size = (Vec(36, 26));
            O4Widget->textPos = (Vec(35, 22));
            O4Widget->flag = 3;
            O4Widget->module = module;
            addChild(O4Widget);

        }
      

    }

};

Model* modelOnceler = createModel<OncelerModule, OncelerPanelWidget>("The-Onceler");