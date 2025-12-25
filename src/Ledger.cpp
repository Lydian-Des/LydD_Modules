#include "plugin.hpp"
#include <vector>
#include <string>


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
    bool Flip;
    bool Flop;
    bool Ay;
    bool Bee;
    logicBlock() {
        this->And = false;
        this->Xor = false;
        this->Nand = false;
        this->Nor = false;
        this->Not1 = false;
        this->Not2 = false;
        this->Flip = false;
        this->Flop = false;
        this->Ay = false;
        this->Bee = false;
    }

    void AnB(float A, float B) {
        this->Ay = (A >= 1);
        this->Bee = (B >= 1);
    }
    void process() {
        this->And = (this->Ay) && (this->Bee);
        this->Xor = (this->Ay || this->Bee) && !this->And;
        this->Nand = !this->And;
        this->Nor = !(this->Ay || this->Bee);
        this->Not1 = !(this->Ay);
        this->Not2 = !(this->Bee);
    }
    void processPair(logicBlock* pair) {
        if (pair) {
            this->And = (this->Ay) && (pair->And);
            this->Xor = (this->Ay || pair->Xor) && !this->And;
            this->Nand = !this->And;
            this->Nor = !(this->Ay || pair->Nor);
            this->Not1 = !(this->Ay);
            this->Not2 = !(pair->Not2);
        }
        else {
            process();
        }
    }

    void FLIPFLOP(float A) {
        if ((A >= 1) && this->Flip) {
            this->Flop = !this->Flop;
            this->Flip = false;
        }
        this->Flip = (A < 1);
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
        ENUMS(NOTS_OUTPUT, 6),
        ENUMS(FLIPFLOPS_OUTPUT, 4),
        NUM_OUTPUTS
	};
    enum LightIds {
        ENUMS(AND_LIGHTS, 5),
        ENUMS(XOR_LIGHTS, 5),
        ENUMS(NAND_LIGHTS, 5),
        ENUMS(NOR_LIGHTS, 5),
        ENUMS(NOT_LIGHTS, 6),
        ENUMS(FLOP_LIGHTS, 4),
        NUM_LIGHTS
    };

    BaseFunctions Functions;
    BaseButtons Buttons;
    logicBlock blocks[5];

    LedgerModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

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
            lights[AND_LIGHTS + i].setBrightness(blocks[i].And);
            lights[XOR_LIGHTS + i].setBrightness(blocks[i].Xor);
            lights[NAND_LIGHTS + i].setBrightness(blocks[i].Nand);
            lights[NOR_LIGHTS + i].setBrightness(blocks[i].Nor);
        }
        for (int i = 0; i < 6; ++i) {
            if (i == 1) i = 2;
            int ind = (i >= 2) ? i - 1 : i;
            lights[NOT_LIGHTS + i].setBrightness(blocks[ind].Not1);
        }
        lights[NOT_LIGHTS + 1].setBrightness(blocks[0].Not2);
        for (int i = 0; i < 4; ++i) {
            lights[FLOP_LIGHTS + i].setBrightness(blocks[i + 1].Flop);
        }
    }

    void generateOutput(const ProcessArgs& args) {

        float Avolt = (isinA) ? inputs[LETTERS_INPUT + 0].getVoltage(0) : 0.f;
        float Bvolt = (isinB) ? inputs[LETTERS_INPUT + 1].getVoltage(0) : 0.f;

        float Volts[8];
        for (int i = 0; i < 8; ++i) {
            Volts[i] = (ins[i]) ? inputs[LETTERS_INPUT + (i + 2)].getVoltage(0) : blocks[i / 2].Flop;
        }

        blocks[0].AnB(Avolt, Bvolt);
        blocks[0].process();
        blocks[0].FLIPFLOP(Avolt);

        for (int i = 1; i < 5; ++i) {
            int evenindex = (i - 1) * 2;
            logicBlock* pairBlock = nullptr;
            if (!(ins[evenindex + 1])) pairBlock = &blocks[i - 1];
            
            blocks[i].AnB(Volts[evenindex], Volts[evenindex + 1]);
            blocks[i].processPair(pairBlock);
            blocks[i].FLIPFLOP((ins[evenindex + 1]) ? Volts[evenindex + 1] : Volts[evenindex]);
        }

        outputs[ANDS_OUTPUT + 0].setVoltage(blocks[0].And * 10.f, 0);
        outputs[XORS_OUTPUT + 0].setVoltage(blocks[0].Xor * 10.f, 0);
        outputs[NANDS_OUTPUT + 0].setVoltage(blocks[0].Nand * 10.f, 0);
        outputs[NORS_OUTPUT + 0].setVoltage(blocks[0].Nor * 10.f, 0);
        outputs[NOTS_OUTPUT + 0].setVoltage(blocks[0].Not1 * 10.f, 0);
        outputs[NOTS_OUTPUT + 1].setVoltage(blocks[0].Not2 * 10.f, 0);
        for (int i = 1; i < 5; ++i) {
            outputs[ANDS_OUTPUT + i].setVoltage(blocks[i].And * 10.f, 0);
            outputs[XORS_OUTPUT + i].setVoltage(blocks[i].Xor * 10.f, 0);
            outputs[NANDS_OUTPUT + i].setVoltage(blocks[i].Nand * 10.f, 0);
            outputs[NORS_OUTPUT + i].setVoltage(blocks[i].Nor * 10.f, 0);
            outputs[NOTS_OUTPUT + (i + 1)].setVoltage(blocks[i].Not1 * 10.f, 0);
            outputs[FLIPFLOPS_OUTPUT + (i - 1)].setVoltage(blocks[i].Flop * 10.f, 0);
        }
    }

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
    void drawLight(const DrawArgs& args) override {
        //nvgGlobalCompositeBlendFunc(args.vg, NVG_ONE_MINUS_DST_COLOR, NVG_ONE);
        if (!sw->svg) return;

        for (auto s = sw->svg->handle->shapes; s; s = s->next) {

            for (auto p = s->paths; p; p = p->next) {
                nvgBeginPath(args.vg);
                nvgFillColor(args.vg, this->color);
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

struct LedgerPanelWidget : ModuleWidget {
    LedgerPanelWidget(LedgerModule* module) {
        setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Cask_panel.svg")));

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

            addOutput(createOutput<PurplePort>(Vec(andLaneX, row1StartY + (i * laneYdist)), module, LedgerModule::ANDS_OUTPUT + i));
            addOutput(createOutput<PurplePort>(Vec(orLaneX, row1StartY + (i * laneYdist)), module, LedgerModule::XORS_OUTPUT + i));
            addOutput(createOutput<PurplePort>(Vec(andLaneX, row2StartY + (i * laneYdist)), module, LedgerModule::NANDS_OUTPUT + i));
            addOutput(createOutput<PurplePort>(Vec(orLaneX, row2StartY + (i * laneYdist)), module, LedgerModule::NORS_OUTPUT + i));
        }
        for (int i = 0; i < 6; ++i) {
            //skip the offset one           
            if (i == 1) i = 2;
            int ind = (i >= 2) ? i - 1: i;
            addChild(createLightCentered<NOTLight>(Vec(NglyphLaneX, glyphRow1Y + (ind * laneYdist)), module, LedgerModule::NOT_LIGHTS + i));

            addOutput(createOutput<PurplePort>(Vec(notLaneX, row1StartY + (ind * laneYdist)), module, LedgerModule::NOTS_OUTPUT + i));
        }
        //the offset one
        addChild(createLightCentered<NOTLight>(Vec(NglyphLaneX, glyphRow2Y), module, LedgerModule::NOT_LIGHTS + 1));

        addOutput(createOutput<PurplePort>(Vec(notLaneX, row2StartY), module, LedgerModule::NOTS_OUTPUT + 1));

        for (int i = 0; i < 4; ++i) {
            addChild(createLightCentered<FLOPLight>(Vec(NglyphLaneX, glyphRow2Y + ((i + 1) * laneYdist)), module, LedgerModule::FLOP_LIGHTS + i));

            addOutput(createOutput<PurplePort>(Vec(notLaneX, row2StartY + ((i + 1) * laneYdist)), module, LedgerModule::FLIPFLOPS_OUTPUT + i));
        }


        
      

    }

};

Model* modelLedger = createModel<LedgerModule, LedgerPanelWidget>("Ledger-Logic");