#include "plugin.hpp"
#include <math.h>

#define MODULE_NAME QuantModule
#define PANEL "Quantizer_panel.svg"
#define HP 12


#define OCT 12
#define BOARD 100 //expanded keyset due to hexagonal layout (highest note is Eb8, semitone 99)

using namespace LydD;

static const int maxPolyphony = 16;

//actual semitone of one panel- if C0,C#1, D-10, Eb15, E4, F-7, F#-6, G7,G#8, A-3, A#-2, B11 
//in hex format, D, F, F#/Gb, A, and A#/Bb are from the octave below, so -12, and Eb is from oct above, so +12
const int hexdown[OCT] = { 0, 0, -12, 12, 0, -12, -12, 0, 0, -12, -12, 0 };


struct HexButton : app::SvgSwitch {
    HexButton() {
        momentary = true;
        addFrame(Svg::load(asset::plugin(pluginInstance, "res/components/HexButton32px_0.svg")));
        addFrame(Svg::load(asset::plugin(pluginInstance, "res/components/HexButton32px_1.svg")));
    }
};

struct NotePicker {
    //without double resolution, output shunts back to 0 volts in between changes. not sure why but this is how VCV does it
    int Bchosen[OCT * 2] = {};
    int Pchosen[BOARD * 2] = {};
    bool enabledSimple[OCT] = {};

    void setSEnabled(bool isSet, int index) {
        enabledSimple[index] = isSet;
    }
    void semiSimple(bool enabled[OCT]) {
        
        for (int c = 0; c < OCT * 2; ++c) {
            int nearestN = 0;
            int leastD = INT_MAX;
            //if this note is enabled, no need to loop.
            if (enabled[c / 2]) {
                Bchosen[c] = c / 2;
                continue;
            }
            //if this note isnt enabled, find the nearest enabled below
            for (int n = -OCT; n <= OCT * 2; ++n) {
                int D = abs((c + 1) / 2 - n);
                if (enabled[rack::math::eucMod(n, 12)] && D < leastD) {
                    leastD = D;
                    nearestN = n;
                }
                else if (D > leastD) break;
            }
            Bchosen[c] = nearestN;                       
        }

    }

    float simpleQuant(float volt) {
        int semiVolt = floor(volt * 24); //quarter-tone resolution of 1v/oct
        int octave = rack::math::eucDiv(semiVolt, 24); //remainder math to find if new octave
        int indFreq = semiVolt - (octave * 24); // rotate with octaves to keep between 0 - 12
        int chosenote = (Bchosen[indFreq] + octave * 12); //pick semitone at adress, add back in octave offset
        return (float)chosenote / 12.f; // reduce back to voltage

    }

    void arrangeHex(bool enabled[BOARD], bool enb[BOARD]) {
        //say ive just enabled C and D. C gets index 12 and D gets index 14. D needs to end up at index 2

        for (int r = 0; r < BOARD; ++r) {
            enb[rack::math::clamp(r + hexdown[r % OCT], 0, BOARD)] = enabled[r];
        }
    }

    void semiPage(bool enabled[BOARD]) {

        for (int c = 0; c < BOARD * 2; ++c) {
            
            int nearab = 48; //middle C semitone
            int nearbe = 48;
            int nearest = 0;
            //choose note when enabled, set indices to same note until new enabled
            //if this pitch not enabled, go up and down until enabled or edge is found, compare distances, pick smallest
            int semiab = 0;
            int semibe = 0;
            if (!enabled[c / 2]) {
                while (!enabled[modN((c / 2) + semiab, BOARD)] && (c / 2) + semiab < BOARD) {
                    semiab++;
                    
                }
                if (enabled[(c / 2) + semiab]) nearab = c / 2 + semiab;

                while (!enabled[modN((c / 2) - semibe, BOARD)] && (c / 2) - semibe >= 0) {
                    semibe++;
                    
                }
                if (enabled[(c / 2) - semibe]) nearbe = c / 2 - semibe;

                //if it goes to the end without finding enabled, force it to pick the other even if distance stepped is shorter
                if (!enabled[(c / 2) + semiab]) nearab = nearbe; //nearab = c / 2 + semiab;
                if (!enabled[(c / 2) - semibe]) nearbe = nearab;; //nearbe = c / 2 - semibe;

                //pick nearest semitone
                nearest = (semiab < semibe) ? nearab : nearbe;
            }
            else {
                //nearest semi is this one
                nearest = c / 2;
            }

            Pchosen[c] = nearest;

        }
    }

    float pageQuant(float volt) {
        float vfix = rack::math::clamp(volt, -4.f, 4.f); //clamp to 8 octaves for array access
        float voffset = rack::math::clamp(vfix + 4.f, 0.f, 8.f); //offset and clamp again for safety
        int semiVolt = floor(voffset * 24); //quarter-tone resolution of 1v/oct -makes 0volts == middle C @ 48semis

        //no need for any wrapping, send to discrete semitone
        int chosenote = (Pchosen[semiVolt]); //pick semitone at adress
        // reduce back to volt and remove page offset (page 4 == TRANS 0 == C4 == 48semis)
        return (float)(chosenote / 12.f) - 4.f; 
    }

};

struct QuantModule : Module
{
    enum ParamIds {
        TRANS_PARAM,
        Q_MODE_BUTTON,
        ENUMS(KEY_BUTTON, OCT),
        BANK_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        POLY_INPUT,
        TRANS_INPUT,
        ENUMS(LANE_INPUT, 4),
        BANK_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {

        ENUMS(LANE_OUTPUT, 4),
        ENUMS(NEWNOTE_OUTPUT, 4),
        //TEST_OUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        ENUMS(PLAYING_LIGHT, OCT),
        MODE_LIGHT,
        NUM_LIGHTS
    };

    NotePicker* QT;
    rack::dsp::BooleanTrigger _modeSwitch;
    rack::dsp::BooleanTrigger _newNote[4];
    rack::dsp::PulseGenerator _notePulse[4];
    int polyChan = 1;
    int loopCounter = 0;
    int CurrentBank = 0;
    bool PageMode = false;
    bool Qreset = false;
    bool BasicAllowed[OCT]; //one octave normal quantize
    bool BasicBank[8][OCT];
    bool PageAllowed[BOARD]; //88 key discrete keyboard quantize + padding
    bool PageBank[8][BOARD];
    bool PolyAllowed[BOARD]; //88 key discrete keyboard quantize polyinput
    int polyNote[maxPolyphony];
    bool HexEnabled[BOARD]; //properly arranged PageAllowed
    bool isLitKey[OCT]; //what keys are currently lit for buttons
    bool buttonPress[OCT];
    bool buttonReset[OCT];
    float lastpress[OCT];
    bool pagepress[BOARD];
    bool pagereset[BOARD];
    int playingPitch[4];
    int Page = 0;
    float Transpose = 0.f;
    float Transpar = 0.f;
    bool gothere = false; //debug, ask if u got here
    float quants[4];

    void clear() {
        memset(&BasicAllowed, false, sizeof(bool) * OCT);
        memset(&PageAllowed, false, sizeof(bool) * BOARD);
        memset(&isLitKey, false, sizeof(bool) * OCT);
        memset(&buttonPress, false, sizeof(bool) * OCT);
        memset(&buttonReset, false, sizeof(bool) * OCT);
        memset(&lastpress, 0, sizeof(float) * OCT);
        memset(&pagepress, false, sizeof(bool) * BOARD);
        memset(&pagereset, false, sizeof(bool) * BOARD);
        for (int b = 0; b < 8; ++b) {
            memset(&BasicBank[b], false, sizeof(bool) * OCT);
            memset(&PageBank[b], false, sizeof(bool) * BOARD);
        }
    }

    QuantModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(TRANS_PARAM, -12.f, 12.f, 0.f, "Transpose");
        paramQuantities[TRANS_PARAM]->snapEnabled = true;
        configSwitch(Q_MODE_BUTTON, 0.f, 1.f, 0.f, "Quantize Mode");
        configParam(BANK_PARAM, 0.f, 7.f, 0.f, "Bank Select");
        paramQuantities[BANK_PARAM]->snapEnabled = true;



        for (int p = 0; p < OCT; ++p) {
            configSwitch(KEY_BUTTON + p, 0.f, 1.f, 0.f, "Note Button");
        }
        QT = new (NotePicker);

        clear();

        /*std::string which[4]{ "One", "Two", "Three", "Four" };
        for (int i = 0; i < 4; ++i) {
            std::string This = "This - ";
            This += which[i];
            configInput(THIS_INPUTS + i, This);
            std::string That = "That - ";
            That += which[i];
            configInput(THAT_INPUTS + i, That);
        }*/
       
    }
    ~QuantModule() {
        delete QT;
    }

    void onReset(const ResetEvent& e) override {
        clear();
        Module::onReset(e);
    }

    void process(const ProcessArgs& args) override {
        
        setMode();
        setBank();

        if (!PageMode) {
            if (loopCounter % 8 == 0) {
                paramQuantities[TRANS_PARAM]->name = "Transpose";
                setParamsBasic(args);
                setLightsBasic(args);
            }

            BasicQuantize(args);
        }
        else {
            std::string pg = std::to_string(Page);
            paramQuantities[TRANS_PARAM]->name = "Page: " + pg;
            
            //paramQuantities[TRANS_PARAM]->setDisplayValueString(pg);
            if (loopCounter % 8 == 0) {
                
               
                setParamsPage(args);
                setLightsPage(args);
            }

            PageQuantize(args);
        }
        ++loopCounter;
        if (loopCounter % 2520 == 0) {
            loopCounter = 0;
        }
    }
    void setMode() {
        bool changemode = _modeSwitch.process(params[Q_MODE_BUTTON].value);
        latchButton((float)changemode, &PageMode, &Qreset);
        if (changemode) {
            QT->semiSimple(BasicAllowed);
            QT->semiPage(HexEnabled);
        }
        lights[MODE_LIGHT].setBrightness(PageMode);
    }
    void setBank() {
        int lastbank = CurrentBank;
        int bankpar = params[BANK_PARAM].value;
        int bankin = inputs[BANK_INPUT].isConnected() ? floor(inputs[BANK_INPUT].getVoltage(0)) : 0;
        int bank = rack::math::clamp(bankpar + bankin, 0, 7);
        CurrentBank = bank;
        if (lastbank != CurrentBank) {
            //loading banks 'presses all the buttons'
            bankLoadBasic(CurrentBank, buttonPress);
            bankLoadPage(CurrentBank, pagepress);
        }
    }

    void bankSaveBasic(int bank, bool enabled[OCT]) {
        memcpy(&BasicBank[bank], enabled, sizeof(bool) * OCT);
    }
    
    void bankLoadBasic(int bank, bool ret[OCT]) {
        memcpy(ret, &BasicBank[bank], sizeof(bool) * OCT);
    }

    void bankSavePage(int bank, bool enabled[BOARD]) {
        memcpy(&PageBank[bank], enabled, sizeof(bool) * BOARD);
    }

    void bankLoadPage(int bank, bool ret[BOARD]) {
        memcpy(ret, &PageBank[bank], sizeof(bool) * BOARD);
    }

    void setParamsBasic(const ProcessArgs& args) {

        int polychans = inputs[POLY_INPUT].getChannels();
        //refresh poly inputs before merging with main bools
        bool polyPlay[OCT] = {false, false, false, false, false, false, false, false, false, false, false, false};
        
        bool polychange = false;
        for (int c = 0; c < polychans; ++c) {
            int tempPoly = polyNote[c];
            int polySemi = inputs[POLY_INPUT].getVoltage(c) * 12;
            int polyOct = rack::math::eucDiv(polySemi, 12);
            int polyIndex = polySemi - (polyOct * 12);
            
            polyNote[c] = polyIndex;
            if (polyNote[c] != tempPoly) polychange = true;
            polyPlay[polyIndex] = true;
        }



        bool anychange = false;
            for (int k = 0; k < OCT; ++k) {

                latchButton(params[KEY_BUTTON + k].value, &buttonPress[k], &buttonReset[k]);

                if (BasicAllowed[k] != buttonPress[k]) {
                    anychange = true;
                    BasicAllowed[k] = buttonPress[k];
                }
                if (polyPlay[k]) {
                    BasicAllowed[k] |= polyPlay[k];
                }
            }

        if (anychange || polychange) {
            bankSaveBasic(CurrentBank, BasicAllowed);

            QT->semiSimple(BasicAllowed);
        }
        Transpar = params[TRANS_PARAM].value;
        Page = 0;
    }

    void setLightsBasic(const ProcessArgs& args) {
        /*for (int l = 0; l < OCT; ++l) {
            isLitKey[l] = BasicAllowed[l];
        }*/
        memcpy(&isLitKey, &BasicAllowed, sizeof(bool) * OCT);
    }

    void BasicQuantize(const ProcessArgs& args) {
        Transpose = (Transpar / 12.f) + (inputs[TRANS_INPUT].isConnected() ? inputs[TRANS_INPUT].getVoltage(0) : 0.f);
        for (int i = 0; i < 4; ++i) {
            _notePulse[i].process(args.sampleTime);
            float quantprev = quants[i];
            float pitchIn = inputs[LANE_INPUT + i].getVoltage(0) + Transpose;
            quants[i] = QT->simpleQuant(pitchIn);
            outputs[LANE_OUTPUT + i].setVoltage(quants[i], 0);

            bool newnote = _newNote[i].process(quantprev != quants[i]);
            if (newnote) _notePulse[i].trigger(0.008f);
            bool notetrig = _notePulse[i].isHigh();
            outputs[NEWNOTE_OUTPUT + i].setVoltage(notetrig * 10.f, 0);

            playingPitch[i] = modN((int)(quants[i] * 12), 12);
        }
       
    }

    void setParamsPage(const ProcessArgs& args) {
        //only set current page at a time. influences all pages
        int polychans = inputs[POLY_INPUT].getChannels();
        //refresh poly inputs before merging with main bools
        bool polyPlay[BOARD] = {};
        bool polychange = false;
        memset(&polyPlay, false, sizeof(bool) * BOARD);
        for (int c = 0; c < polychans; ++c) {
            int tempPoly = polyNote[c];
            //offset voltage by 4 pages/octaves to center in array
            int polySemi = (inputs[POLY_INPUT].getVoltage(c) + 4) * 12;            
            polySemi = rack::math::clamp(polySemi, 0, BOARD);
            //take direct note, nice and simple
            polyNote[c] = polySemi;
            if (polyNote[c] != tempPoly) polychange = true;
            polyPlay[polySemi] = true;
        }
        Page = (params[TRANS_PARAM].value / 4) + 4; //access pages 1 - 6 so overflow can happen on 0 and 7
 
        bool anychange = false;

        for (int k = 0; k < OCT; ++k) {
            int hexdex = k + (Page * 12) ;
            latchButton(params[KEY_BUTTON + k].value, &pagepress[hexdex], &pagereset[hexdex]);
            if (PageAllowed[hexdex] != pagepress[hexdex]) {
                anychange = true;
                PageAllowed[hexdex] = pagepress[hexdex];
            }

        }
        //rearrange array to place semitones in Hex layout
        
        QT->arrangeHex(PageAllowed, HexEnabled);
        memcpy(&PolyAllowed, &polyPlay, sizeof(bool) * BOARD);
        if (polychange) {
            for (int m = 0; m < BOARD; ++m) {
                HexEnabled[m] |= polyPlay[m];
            }
        }
        if (anychange || polychange) {
            bankSavePage(CurrentBank, PageAllowed);

            QT->semiPage(HexEnabled);
        }

    }

    void setLightsPage(const ProcessArgs& args) {
        for (int l = 0; l < OCT; ++l) {
            isLitKey[l] = PageAllowed[l + (Page * 12)] || PolyAllowed[l + (Page * 12) + hexdown[l]];//undo rearrange to display on the 'one octave' panel
        }
        //memcpy(&isLitKey, &PageAllowed[Page * 12], sizeof(bool) * OCT);
    }

    void PageQuantize(const ProcessArgs& args) {
       

        Transpose = (inputs[TRANS_INPUT].isConnected() ? inputs[TRANS_INPUT].getVoltage(0) : 0.f);
        for (int i = 0; i < 4; ++i) {
            _notePulse[i].process(args.sampleTime);
            float quantprev = quants[i];
            float pitchIn = inputs[LANE_INPUT + i].getVoltage(0) + Transpose;
            pitchIn = rack::math::clamp(pitchIn, -3.f, 3.f);
            quants[i] = QT->pageQuant(pitchIn);
            outputs[LANE_OUTPUT + i].setVoltage(quants[i], 0);

            bool newnote = _newNote[i].process(quantprev != quants[i]);
            if (newnote) _notePulse[i].trigger(0.008f);
            bool notetrig = _notePulse[i].isHigh();
            outputs[NEWNOTE_OUTPUT + i].setVoltage(notetrig * 10.f, 0); 
            
            playingPitch[i] = modN((int)(quants[i] * 12), 12);
        }

    }

    json_t* dataToJson() override {
        json_t* rootJ = json_object();

        json_t* modeJ = json_boolean(PageMode);
        json_object_set_new(rootJ, "Qmode",modeJ);

        json_t* basicarrayJ = json_array();
        for (int ind = 0; ind < OCT; ++ind) {
            json_t* setb = json_boolean(BasicAllowed[ind]);
            json_array_append_new(basicarrayJ, setb);
        }
        json_object_set_new(rootJ, "basicarray", basicarrayJ);

        json_t* pagearrayJ = json_array();
        for (int pind = 0; pind < BOARD; ++pind) {
            json_t* setp = json_boolean(PageAllowed[pind]);
            json_array_append_new(pagearrayJ, setp);
        }
        json_object_set_new(rootJ, "pagearray", pagearrayJ);

        json_t* currBankJ = json_integer(CurrentBank);
        json_object_set_new(rootJ, "currentBank", currBankJ);


        json_t* BanksBasicJ = json_array();
        for (int i = 0; i < 8; i++) {
            json_t* innerBankJ = json_array();
            for (int j = 0; j < OCT; j++) {
                json_array_append_new(innerBankJ, json_boolean(BasicBank[i][j]));
            }
            json_array_append_new(BanksBasicJ, innerBankJ);
        }
        json_object_set_new(rootJ, "BasicSavedBanks", BanksBasicJ);

        json_t* BanksPageJ = json_array();
        for (int i = 0; i < 8; i++) {
            json_t* innerPageJ = json_array();
            for (int j = 0; j < BOARD; j++) {
                json_array_append_new(innerPageJ, json_boolean(PageBank[i][j]));
            }
            json_array_append_new(BanksPageJ, innerPageJ);
        }
        json_object_set_new(rootJ, "PageSavedBanks", BanksPageJ);


        return rootJ;
    }

    void dataFromJson(json_t* rootJ) override {
        json_t* modeJ = json_object_get(rootJ, "Qmode");
        PageMode = json_boolean_value(modeJ);

        json_t* currBankJ = json_object_get(rootJ, "currentBank");
        CurrentBank = json_integer_value(currBankJ);
        json_t* BanksBasicJ = json_object_get(rootJ, "BasicSavedBanks");
        if (BanksBasicJ) {
            for (size_t i = 0; i < json_array_size(BanksBasicJ); i++) {
                json_t* innerBankJ = json_array_get(BanksBasicJ, i);
                for (size_t j = 0; j < json_array_size(innerBankJ); j++) {
                    BasicBank[i][j] = json_boolean_value(json_array_get(innerBankJ, j));
                }
            }
            bankLoadBasic(CurrentBank, buttonPress);
            memcpy(BasicAllowed, buttonPress, sizeof(bool) * OCT);
            QT->semiSimple(BasicAllowed);
        }

        json_t* BanksPageJ = json_object_get(rootJ, "PageSavedBanks");
        if (BanksPageJ) {
            for (size_t i = 0; i < json_array_size(BanksPageJ); i++) {
                json_t* innerPageJ = json_array_get(BanksPageJ, i);
                for (size_t j = 0; j < json_array_size(innerPageJ); j++) {
                    PageBank[i][j] = json_boolean_value(json_array_get(innerPageJ, j));
                }
            }
            bankLoadPage(CurrentBank, pagepress);
            memcpy(PageAllowed, pagepress, sizeof(bool) * BOARD);
            QT->arrangeHex(PageAllowed, HexEnabled);
            QT->semiPage(HexEnabled);
        }
    }

};


struct HexLight : SvgWidget {
    QuantModule* module;
    int Note;
    int type = 0;
    
    HexLight() {
    }
    void Svg(std::string path) {
        this->setSvg(Svg::load(asset::plugin(pluginInstance, path)));
    }
    void drawLayer(const DrawArgs& args, int layer) override {
        if (module && this->svg && layer == 1) {
            float Active = module->isLitKey[Note];
            float mode = module->PageMode;
            int playingNote = -1; //note not playing on any lane
            int numplaying = 0; //how many lanes playing this note
            for (int l = 0; l < 4; ++l) {
                if (module->playingPitch[l] == Note) {
                    
                    numplaying++;
                    playingNote += (l + 1) * numplaying;
                }
            }
            nvgGlobalCompositeBlendFunc(args.vg, NVG_ONE_MINUS_DST_COLOR, NVG_ONE);
            float hue = (mode == 0) ? 0.65 : 0.75 - (module->Page / 14.f);
            hue = playingNote != -1 ? playingNote / 8.f : hue;
                for (auto s = svg->handle->shapes; s; s = s->next) {
                    nvgFillColor(args.vg, nvgHSL(hue, 0.8, Active / 2.f));
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
                    }
                }
        }
        Widget::drawLayer(args, layer);
    }

   
};

using namespace LydD::Components;
struct QuantPanelWidget : ModuleWidget {

    //include struct for logo here so it has modules name
    #include "Theme/LogoLight.h"

    QuantPanelWidget(QuantModule* module) {
        setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Quantizer_panel.svg")));

		addChild(createWidget<ScrewSilver>(Vec(15, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 30, 0)));
		addChild(createWidget<ScrewSilver>(Vec(15, 365)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 30, 365)));

        addParam(createParam<RoundBlackKnob>(Vec(22.5, 205.35), module, QuantModule::TRANS_PARAM));
        addParam(createParam<VCVButton>(Vec(137.309, 40.287), module, QuantModule::Q_MODE_BUTTON));
        addParam(createParam<RoundSmallBlackKnob>(Vec(143, 132.5), module, QuantModule::BANK_PARAM));

        addChild(createLightCentered<SmallLight<BlueLight>>(Vec(164, 49), module, QuantModule::MODE_LIGHT));
        std::vector<Vec> notePos = {
            (Vec(14.398, 99.621)),  //C
            (Vec(71.660, 99.621)), //C#
            (Vec(43.363, 150.107)), //D
            (Vec(71.660, 30.614)), //D#
            (Vec(43.363, 81.871)),  //E
            (Vec(14.398, 133.739)), //F
            (Vec(71.660, 133.739)), //F#
            (Vec(14.398, 65.503)), //G
            (Vec(71.660, 65.503)), //G#
            (Vec(43.363, 115.989)), //A
            (Vec(99.958, 115.989)),  //A#
            (Vec(43.363, 47.753)), //B
        };
        for (int b = 0; b < OCT; ++b) {
            addParam(createParam<HexButton>(notePos[b], module, QuantModule::KEY_BUTTON + b));
        }

       
       
        float lsX = 10.f;
        float dX = (147.5f - 10.f) / 3.f; // 1/3 of total desired span to give 4 total locations
        float lsY1 = 283.f;
        float lsY2 = 311.7f;
        float lsY3 = 340.5f;
        addInput(createInput<PurplePort>(Vec(lsX, 243), module, QuantModule::TRANS_INPUT));
        addInput(createInput<PurplePort>(Vec(145.5, 204), module, QuantModule::POLY_INPUT));
        addInput(createInput<PurplePort>(Vec(145.5, 169.28), module, QuantModule::BANK_INPUT));

        for (int p = 0; p < 4; ++p) {

            addInput(createInput<PurplePort>(Vec(lsX + (dX * p), lsY1), module, QuantModule::LANE_INPUT + p));
            addOutput(createOutput<PurplePort>(Vec(lsX + (dX * p), lsY2), module, QuantModule::NEWNOTE_OUTPUT + p));
            addOutput(createOutput<PurplePort>(Vec(lsX + (dX * p), lsY3), module, QuantModule::LANE_OUTPUT + p));
        }
        //addOutput(createOutput<PurplePort>(Vec(lsX , 25), module, QuantModule::TEST_OUT));


        if (module) {
            HexLight* quantlight1 = createWidget<HexLight>(notePos[0]);
            quantlight1->Note = 0;
            quantlight1->Svg("res/components/HexLight32px.svg");
            quantlight1->module = module;
            addChild(quantlight1);

            for (int note = 1; note < OCT; ++note) {
                HexLight* quantlight = createWidget<HexLight>(notePos[note]);
                quantlight->Note = note;
                quantlight->Svg("res/components/HexLight32px.svg");
                quantlight->module = module;
                addChild(quantlight);
            }

            //must be called 'logoPos'for all modules 
            Vec logoPos = Vec(((15.f * HP) / 2.f) - 12.5, 363.f);
            QuantModule* module = dynamic_cast<QuantModule*>(this->module);
            assert(module);
            #include "Theme/LogoChild.h"
        }
       
    }
    //void onDragStart(const event::DragStart& e) override {
   //    if (e.button == GLFW_MOUSE_BUTTON_LEFT) {
   //        module->QT.enabledSimple[Note] ^= true;
   //       // module->QT.setSimpleEnabled();
   //        module->QT.semiSimple();
   //    }
   //    SvgWidget::onDragStart(e);
   //}

   //void onDragEnter(const event::DragEnter& e) override {
   //    if (e.button == GLFW_MOUSE_BUTTON_LEFT) {
   //        HexLight* origin = dynamic_cast<HexLight*>(e.origin);
   //        if (origin) {
   //            module->QT.enabledSimple[Note] = module->QT.enabledSimple[origin->Note];;
   //            module->QT.semiSimple();
   //        }
   //    }
   //    SvgWidget::onDragEnter(e);
   //}
};

Model* modelQuant = createModel<QuantModule, QuantPanelWidget>("Hex-Quantizer");