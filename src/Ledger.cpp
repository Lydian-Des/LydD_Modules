#include "plugin.hpp"

#define MODULE_NAME LedgerModule
#define PANEL "Ledger_panel.svg"
#define HP 8

using namespace LydD;

static const int maxPolyphony = 1;

static rack::simd::float_4 Zero{ 0.f };
static rack::simd::float_4 One{ 1.f };

struct logicBlock {
    bool And;
    bool Xor;
    bool Nand;
    bool Nor;
    bool Not1;
    bool Not2; 
    bool Flip1;
    bool Flop1;
    bool Flip2;
    bool Flop2;
    bool Ay;
    bool Bee;
    logicBlock() {
        this->And = false;
        this->Xor = false;
        this->Nand = false;
        this->Nor = false;
        this->Not1 = false;
        this->Not2 = false;
        this->Flip1 = false;
        this->Flop1 = false;
        this->Flip2 = false;
        this->Flop2 = false;
        this->Ay = false;
        this->Bee = false;
    }

    void AnB(float A, float B) {
        this->Ay = (A >= 1);
        this->Bee = (B >= 1);
    }

    void FLIPFLOP1(float A) {
        if ((A >= 1) && this->Flip1) {
            this->Flop1 = !this->Flop1;
            this->Flip1 = false;
        }
        this->Flip1 = (A < 1);
    }
    void FLIPFLOP2(float B) {
        if ((B >= 1) && this->Flip2) {
            this->Flop2 = !this->Flop2;
            this->Flip2 = false;
        }
        this->Flip2 = (B < 1);
    }

    void process() {
        this->And = (this->Ay) && (this->Bee);
        this->Xor = (this->Ay || this->Bee) && !this->And;
        this->Nand = !this->And;
        this->Nor = !(this->Ay || this->Bee);
        this->Not1 = !(this->Ay);
        this->Not2 = !(this->Bee);
        FLIPFLOP1(this->Ay);
        FLIPFLOP2(this->Bee);
    }
    //if paired, results for this block will cascade with results from the previous block
    void processPair(logicBlock* pair) {
        if (pair) {
            this->And = (this->Ay) && (pair->getAnd());
            this->Xor = (this->Ay || pair->getXor()) && !this->And;
            this->Nand = !this->And;
            this->Nor = !(this->Ay || pair->getNor());
            this->Not1 = !(this->Ay);
            this->Not2 = !(pair->getNot2());
            FLIPFLOP1(this->Ay);
            FLIPFLOP2(pair->getFlop2());
        }
        else {
            process();
        }
    }

    
    //getters for desired outputs
    bool getAnd() {
        return this->And;
    }
    bool getXor() {
        return this->Xor;
    }
    bool getNand() {
        return this->Nand;
    }
    bool getNor() {
        return this->Nor;
    }
    bool getNot1() {
        return this->Not1;
    }
    bool getNot2() {
        return this->Not2;
    }
    bool getFlop1() {
        return this->Flop1;
    }
    bool getFlop2() {
        return this->Flop2;
    }

    void reset() {
        this->And = false;
        this->Xor = false;
        this->Nand = false;
        this->Nor = false;
        this->Not1 = false;
        this->Not2 = false;
        this->Flip1 = false;
        this->Flop1 = false;
        this->Flip2 = false;
        this->Flop2 = false;
        this->Ay = false;
        this->Bee = false;
    }
};

struct LedgerModule : Module
{
    enum ParamIds {
        NUM_PARAMS
	};
    enum InputIds {
        ENUMS(LETTERS_INPUT, 10),
        NUM_INPUTS
	};
    enum OutputIds {

        ENUMS(ANDS_OUTPUT, 5),
        ENUMS(XORS_OUTPUT, 5),
        ENUMS(NANDS_OUTPUT, 5),
        ENUMS(NORS_OUTPUT, 5),
        ENUMS(NOTS_OUTPUT, 5),
        ENUMS(FLIPFLOPS_OUTPUT, 5),
        NUM_OUTPUTS
	};
    enum LightIds {
        ENUMS(AND_LIGHTS, 5),
        ENUMS(XOR_LIGHTS, 5),
        ENUMS(NAND_LIGHTS, 5),
        ENUMS(NOR_LIGHTS, 5),
        ENUMS(NOT_LIGHTS, 5),
        ENUMS(FLOP_LIGHTS, 5),
        NUM_LIGHTS
    };

    logicBlock blocks[5];

    #include "Theme/PanelVars.h"

    LedgerModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
   
        std::string letters[10] = { "A - ", "B - ", "C - ", "D - ", "E - ", "F - ", "G - ", "H - ", "I - ", "J - " };
        for (int i = 0; i < 10; ++i) {
            std::string num = std::to_string((i / 2) + 1);
            configInput(LETTERS_INPUT + i, letters[i] + num);
        }
        for (int j = 0; j < 5; ++j) {
            std::string num = std::to_string(j + 1);
            configOutput(ANDS_OUTPUT + j, "And - " + num);
            configOutput(XORS_OUTPUT + j, "Xor - " + num);
            configOutput(NANDS_OUTPUT + j, "Nand - " + num);
            configOutput(NORS_OUTPUT + j, "Nor - " + num);
            configOutput(NOTS_OUTPUT + j, "Not - " + num);
            configOutput(FLIPFLOPS_OUTPUT + j, "FlipFlop - " + num);

        }
        #include "Theme/setDefaultInit.h"
    }

    int currentPolyphony = 1;
    int currentBanks = 1;
    int loopCounter = 0;
  
    bool isinA = false;
    bool isinB = false;
    bool isinC = false;
    bool isinD = false;
    bool isinE = false;
    bool isinF = false;
    bool isinG = false;
    bool isinH = false;
    bool isinI = false;
    bool isinJ = false;
    bool ins[8]{ false };

    void process(const ProcessArgs& args) override {

      
    
        if (loopCounter % 32 == 0) {
            checkInputs(args);         
        }
        if (loopCounter % 16 == 0) {
            Lights(args);
        }
        generateOutput(args);
        //reset will just reset phase, so sync shoud do the same?

        if (loopCounter % 4 == 0) {

        }
        
        loopCounter++;
        if (loopCounter % 2520 == 0) {
            loopCounter = 0;
        }
    }

    void checkInputs(const ProcessArgs& args) {
        for (int o = ANDS_OUTPUT + 0; o != NUM_OUTPUTS; ++o) {
            outputs[o].setChannels(1);
        }

        isinA = inputs[LETTERS_INPUT + 0].isConnected();
        isinB = inputs[LETTERS_INPUT + 1].isConnected();
        for (int i = 0; i < 8; ++i) {
            ins[i] = inputs[LETTERS_INPUT + (i + 2)].isConnected();
        }

    }
    void Lights(const ProcessArgs& args) {

        for (int i = 0; i < 5; ++i) {
            lights[AND_LIGHTS + i].setBrightness(blocks[i].getAnd());
            lights[XOR_LIGHTS + i].setBrightness(blocks[i].getXor());
            lights[NAND_LIGHTS + i].setBrightness(blocks[i].getNand());
            lights[NOR_LIGHTS + i].setBrightness(blocks[i].getNor());
            lights[NOT_LIGHTS + i].setBrightness(blocks[i].getNot1());
            lights[FLOP_LIGHTS + i].setBrightness(blocks[i].getFlop2());
        }
    }

    void generateOutput(const ProcessArgs& args) {

        float Avolt = (isinA) ? inputs[LETTERS_INPUT + 0].getVoltage(0) : 0.f;
        float Bvolt = (isinB) ? inputs[LETTERS_INPUT + 1].getVoltage(0) : 0.f;

        blocks[0].AnB(Avolt, Bvolt);
        blocks[0].process();

        float Volts[8];
        for (int i = 0; i < 4; ++i) {
            //start at letters 2 & 3
            int ia = i * 2;
            int ib = ia + 1;
            //if top(even) is no input set Volt to NOT of block before
            //if bottom(odd) is no input set Volt to FLOP of block before
            Volts[ia] = (ins[ia]) ? inputs[LETTERS_INPUT + (ia + 2)].getVoltage(0) : blocks[i].getFlop1();
            Volts[ib] = (ins[ib]) ? inputs[LETTERS_INPUT + (ib + 2)].getVoltage(0) : blocks[i].getFlop2();
        }
        //run nested process on blocks, only nesting if at least one input if disconnected
        for (int i = 1; i < 5; ++i) {
            int evenindex = (i - 1) * 2;
            logicBlock* pairBlock = nullptr;
            if (!(ins[evenindex + 1])) pairBlock = &blocks[i - 1];
            
            blocks[i].AnB(Volts[evenindex], Volts[evenindex + 1]);
            blocks[i].processPair(pairBlock);
        }

        outputs[ANDS_OUTPUT + 0].setVoltage(blocks[0].getAnd() * 10.f, 0);
        outputs[XORS_OUTPUT + 0].setVoltage(blocks[0].getXor() * 10.f, 0);
        outputs[NANDS_OUTPUT + 0].setVoltage(blocks[0].getNand() * 10.f, 0);
        outputs[NORS_OUTPUT + 0].setVoltage(blocks[0].getNor() * 10.f, 0);
        outputs[NOTS_OUTPUT + 0].setVoltage(blocks[0].getNot1() * 10.f, 0);
        outputs[FLIPFLOPS_OUTPUT + 0].setVoltage(blocks[0].getFlop2() * 10.f, 0);
        for (int i = 1; i < 5; ++i) {
            outputs[ANDS_OUTPUT + i].setVoltage(blocks[i].getAnd() * 10.f, 0);
            outputs[XORS_OUTPUT + i].setVoltage(blocks[i].getXor() * 10.f, 0);
            outputs[NANDS_OUTPUT + i].setVoltage(blocks[i].getNand() * 10.f, 0);
            outputs[NORS_OUTPUT + i].setVoltage(blocks[i].getNor() * 10.f, 0);
            outputs[NOTS_OUTPUT + i].setVoltage(blocks[i].getNot1() * 10.f, 0);
            outputs[FLIPFLOPS_OUTPUT + i].setVoltage(blocks[i].getFlop2() * 10.f, 0);
        }
    }

    void onReset(const ResetEvent& e) override {

        loopCounter = 0;
        for (int i = 1; i < 5; ++i) {
            blocks[i].reset();
        }
        Module::onReset(e);
    }

    json_t* dataToJson() override {
        json_t* rootJ = json_object();
        json_t* panelJ = json_integer(currPanel);
        json_object_set_new(rootJ, "Panel", panelJ);
        return rootJ;
    }

    void dataFromJson(json_t* rootJ) override {
        json_t* panelJ = json_object_get(rootJ, "Panel");
        if (panelJ) currPanel = json_integer_value(panelJ);
        
    }

};


NVGcolor getNVGColor(unsigned int c) {
    return nvgRGBA(
        (c ) & 0xFF,
        (c >> 8) & 0xFF,
        (c >> 16) & 0xFF,
        (c >> 24) & 0xFF );

}

struct LightGlyph : SvgLight {
    LightGlyph() {

    }
    void drawHalo(const DrawArgs& args) override {
        NVGcolor colour = this->color;
        //change light color based on current modules current panel style
        ModuleWidget* parent = dynamic_cast<ModuleWidget*>(this->getParent());
        if (parent) {
            LedgerModule* pmod = dynamic_cast<LedgerModule*>(parent->module);
            if (pmod) {
                if (pmod->currPanel == 5) {
                    colour = rack::color::RED;
                    colour.a *= this->color.a;
                }
                if (pmod->currPanel == 4) {
                    colour = rack::color::BLUE;
                    colour.a *= this->color.a;
                }
            }
        }
        this->color = colour;
        LightWidget::drawHalo(args);
    }
    void drawLight(const DrawArgs& args) override {
        //nvgGlobalCompositeBlendFunc(args.vg, NVG_ONE_MINUS_DST_COLOR, NVG_ONE);
        if (!sw->svg) return;
        NVGcolor colour = this->color;
        ModuleWidget* parent = dynamic_cast<ModuleWidget*>(this->getParent());
        if (parent) {
            LedgerModule* pmod = dynamic_cast<LedgerModule*>(parent->module);
            if (pmod) {
                if (pmod->currPanel == 5) {
                    colour = rack::color::RED;
                    colour.a = this->color.a;
                }
                if (pmod->currPanel == 4) {
                    colour = rack::color::BLUE;
                    colour.a = this->color.a;
                }
            }
        }
        for (auto s = sw->svg->handle->shapes; s; s = s->next) {

            for (auto p = s->paths; p; p = p->next) {
                nvgBeginPath(args.vg);
                nvgFillColor(args.vg, colour);
                nvgMoveTo(args.vg, p->pts[0], p->pts[1]);
                for (auto i = 0; i < p->npts - 1; i += 3) {
                    float* path = &p->pts[i * 2];
                    nvgBezierTo(args.vg, path[2], path[3], path[4], path[5], path[6], path[7]);
                }
                if (p->closed)
                    nvgLineTo(args.vg, p->pts[0], p->pts[1]);
                if (s->fill.type)
                    nvgFill(args.vg);
            }
        }
        // LightWidget::drawLight(args);
    }
};

struct ANDLight : LightGlyph {
    ANDLight() {
        this->setSvg(Svg::load(asset::plugin(pluginInstance, "res/Logic_Glyphs/AND-G.svg")));
        auto s = this->sw->svg->handle->shapes;
        this->addBaseColor(getNVGColor(s->fill.color));
    }
   
};
struct XORLight : LightGlyph {
    XORLight() {
        this->setSvg(Svg::load(asset::plugin(pluginInstance, "res/Logic_Glyphs/XOR-G.svg")));
        auto s = this->sw->svg->handle->shapes;
        this->addBaseColor(getNVGColor(s->fill.color));
    }

};
struct NANDLight : LightGlyph {
    NANDLight() {
        this->setSvg(Svg::load(asset::plugin(pluginInstance, "res/Logic_Glyphs/NAND-G.svg")));
        auto s = this->sw->svg->handle->shapes;
        this->addBaseColor(getNVGColor(s->fill.color));
    }
    
};
struct NORLight : LightGlyph {
    NORLight() {
        this->setSvg(Svg::load(asset::plugin(pluginInstance, "res/Logic_Glyphs/NOR-G.svg")));
        auto s = this->sw->svg->handle->shapes;
        this->addBaseColor(getNVGColor(s->fill.color));
    }

};

struct NOTLight : LightGlyph {
    NOTLight() {
        this->setSvg(Svg::load(asset::plugin(pluginInstance, "res/Logic_Glyphs/NOT-G.svg")));
        auto s = this->sw->svg->handle->shapes;
        this->addBaseColor(getNVGColor(s->fill.color));
    }

};

struct FLOPLight : LightGlyph {
    FLOPLight() {
        this->setSvg(Svg::load(asset::plugin(pluginInstance, "res/Logic_Glyphs/FLOP-G.svg")));
        auto s = this->sw->svg->handle->shapes;
        this->addBaseColor(getNVGColor(s->fill.color));
    }

};

using namespace LydD::Components;
struct LedgerPanelWidget : ModuleWidget {

    #include "Theme/LogoLight.h"
    
    std::string panel;

    LedgerPanelWidget(LedgerModule* module) {
        setModule(module);
		
        panel = PANEL;
        //set panel on init
        #include "Theme/initChoosePanel.h"

		addChild(createWidget<ScrewSilver>(Vec(15, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 30, 0)));
		addChild(createWidget<ScrewSilver>(Vec(15, 365)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 30, 365)));

        float inLaneX = 10;
        float andLaneX = 38;
        float orLaneX = 62.5;
        float notLaneX = 87;
        float row1StartY = 32;
        float row2StartY = 65;
        float laneYdist = 68;

        float AglyphLaneX = 44.8 + 4;
        float OglyphLaneX = 69.3 + 4;
        float NglyphLaneX = 94.2 + 4;
        float glyphRow1Y = 55.7 + 4;
        float glyphRow2Y = 88.9 + 4;


        for (int i = 0; i < 10; i += 2) {
            addInput(createInput<PurplePort>(Vec(inLaneX, row1StartY + (laneYdist * (i / 2))), module, LedgerModule::LETTERS_INPUT + i));
            addInput(createInput<PurplePort>(Vec(inLaneX, row2StartY + (laneYdist * (i / 2))), module, LedgerModule::LETTERS_INPUT + i + 1));
        }
        for (int i = 0; i < 5; ++i) {
            addChild(createLightCentered<ANDLight>(Vec(AglyphLaneX, glyphRow1Y + (i * laneYdist)), module, LedgerModule::AND_LIGHTS + i));
            addChild(createLightCentered<XORLight>(Vec(OglyphLaneX, glyphRow1Y + (i * laneYdist)), module, LedgerModule::XOR_LIGHTS + i));
            addChild(createLightCentered<NANDLight>(Vec(AglyphLaneX, glyphRow2Y + (i * laneYdist)), module, LedgerModule::NAND_LIGHTS + i));
            addChild(createLightCentered<NORLight>(Vec(OglyphLaneX, glyphRow2Y + (i * laneYdist)), module, LedgerModule::NOR_LIGHTS + i));
            addChild(createLightCentered<NOTLight>(Vec(NglyphLaneX, glyphRow1Y + (i * laneYdist)), module, LedgerModule::NOT_LIGHTS + i));
            addChild(createLightCentered<FLOPLight>(Vec(NglyphLaneX, glyphRow2Y + (i * laneYdist)), module, LedgerModule::FLOP_LIGHTS + i));

            addOutput(createOutput<PurplePort>(Vec(andLaneX, row1StartY + (i * laneYdist)), module, LedgerModule::ANDS_OUTPUT + i));
            addOutput(createOutput<PurplePort>(Vec(orLaneX, row1StartY + (i * laneYdist)), module, LedgerModule::XORS_OUTPUT + i));
            addOutput(createOutput<PurplePort>(Vec(andLaneX, row2StartY + (i * laneYdist)), module, LedgerModule::NANDS_OUTPUT + i));
            addOutput(createOutput<PurplePort>(Vec(orLaneX, row2StartY + (i * laneYdist)), module, LedgerModule::NORS_OUTPUT + i));
            addOutput(createOutput<PurplePort>(Vec(notLaneX, row1StartY + (i * laneYdist)), module, LedgerModule::NOTS_OUTPUT + i));
            addOutput(createOutput<PurplePort>(Vec(notLaneX, row2StartY + (i * laneYdist)), module, LedgerModule::FLIPFLOPS_OUTPUT + i));
        }

        if (module) {
            //must be called 'logoPos'for all modules
            Vec logoPos = Vec(((15.f * HP) / 2.f) - 12.5, 363.f);
            LedgerModule* module = dynamic_cast<LedgerModule*>(this->module);
            assert(module);
            #include "Theme/LogoChild.h" 
        }
    }

    //give struct to menu containing panel options
    #include "Theme/PanelList.h" 

    void appendContextMenu(Menu* menu) override {
        LedgerModule* module = dynamic_cast<LedgerModule*>(this->module);
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

Model* modelLedger = createModel<LedgerModule, LedgerPanelWidget>("Ledger-Logic");