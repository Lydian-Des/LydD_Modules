#include "plugin.hpp"
#include "Lydapi/LydEQ.h"
#include "Lydapi/LydDelayLine.h"

#define MODULE_NAME CanyonModule
#define PANEL "Canyon_panel.svg"
#define HP 10


static const int maxPolyphony = 1;

using namespace LydD;

constexpr static size_t INPUT_BUFFER_SPACE = 44100 * 8; //~8seconds
constexpr static size_t DELAY_BUFFER_SPACE = 4096 * 2;


struct CanyonModule : Module
{

    enum ParamIds {
        ENUMS(DELAY_PARAM, 2),
        ENUMS(DELAY_CV_PARAM, 2),
        ENUMS(STONE_PARAM, 2),
        ENUMS(STONE_CV_PARAM, 2),
        FEEDBACK_PARAM,
        PITCH_PARAM,
        SCATTER_PARAM,
        FREEZE_DELAY_BUTTON,
        REVERSE_BUTTON,
        DRY_PARAM,
        WET_PARAM,
        MODE_BUTTON,
        NUM_PARAMS
    };
    enum InputIds {

        ENUMS(DELAY_INPUT, 2),
        ENUMS(STONE_INPUT, 2),
        FEEDBACK_INPUT,
        PITCH_INPUT,
        SCATTER_INPUT,
        FREEZE_DELAY_INPUT,
        REVERSE_INPUT,
        DRY_INPUT,
        WET_INPUT,
        ENUMS(AUDIO_INPUT, 2),
        NUM_INPUTS
    };
    enum OutputIds {

        //TESTOUT,
        //TESTOUT2,
        ENUMS(AUDIO_OUTPUT, 2),
        NUM_OUTPUTS
    };
    enum LightIds {
        ENUMS(FREEZE_LIGHT, 3),
        ENUMS(REVERSE_LIGHT, 3),
        ENUMS(MODE_LIGHT, 3),
        ENUMS(LEFT_WALL_LIGHT, 3),
        ENUMS(RIGHT_WALL_LIGHT, 3),
        NUM_LIGHTS
    };


    enum States {
        DUAL_MONO,
        PING_PONG,
        CROSS_FEED,
        NUM_STATES
    };
    int State = 0;


    Delay::CFDelayLine<float, INPUT_BUFFER_SPACE, DELAY_BUFFER_SPACE> _Delay[2];
    Filter::SFRCFilter<float> _Stone[2];
    Filter::AllP1Smear<> _scatter[2];
    rack::dsp::BiquadFilter deClick[2];
    rack::dsp::Timer _lightTime[2];
    float scasmall[4] = { 5, 11, 29, 47 }; //in samples, for scatter param
    float scabig[4] = { 153, 271, 387, 401 }; 
    bool ButtonPress[3] = { 0, 0, 0 };

    int currentPolyphony = 1;
    int currentBanks = 1;
    int loopCounter = 0;

    bool isinDelay[2] = { false, false };
    bool isinStone[2] = { false, false };
    bool isinFeed = false;
    bool isinFreezeD = false;
    bool isinReverse = false;
    bool isinPitch = false;
    bool isinScat = false;
    bool isinDryMix = false;
    bool isinWetMix = false;

    bool isinAudL = false;
    bool isinAudR = false;

    bool modepress = false;
    bool frozenDelay = false;
    bool reverseDelay = false;


    float totaldelay[2] = { 100, 100 };
    float tempDelayTime = 0.f;
    float delayPar[2] = { 0.f, 0.f };
    float cvAttDTime[2] = { 0.f, 0.f };
    float stonePar[2] = { 0.f, 0.f };
    float cvAttStone[2] = { 0.f, 0.f };
    float feedbackPar = 0.f;
    float feedback = 0.f;
    int swapbuf = 0;
    float pitchPar = 0.f;
    float Scatter = 0.f;
    float dryMixPar = 0.f;
    float wetMixPar = 0.f;

    float Dry[2] = { 0, 0 };
    float lastWet[2] = { 0, 0 };
    float DelayOutput[2] = { 0, 0 };
    float lastSample[2] = { 0.f, 0.f };

    CanyonModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        std::string side[2] = { " - Left", " - Right" };
        for (int a = 0; a < 2; ++a) {
            configParam(DELAY_PARAM + a, 0.1f, 16.f, 1.f, "Echo Time" + side[a]);
            configParam(DELAY_CV_PARAM + a, -1.f, 1.f, 0.f, "Echo CV" + side[a]);
            configParam(STONE_PARAM + a, 0.f, 1.f, 0.5f, "Light/Dark" + side[a]);
            configParam(STONE_CV_PARAM + a, -1.f, 1.f, 0.f, "L/D CV" + side[a]);

            configInput(DELAY_INPUT + a, "Echo" + side[a]);
            configInput(STONE_INPUT + a, "Light/Dark" + side[a]);
            configInput(AUDIO_INPUT + a, "Audio In" + side[a]);

            configOutput(AUDIO_OUTPUT + a, "Audio Out" + side[a]);
        }

        configParam(FEEDBACK_PARAM, 0.f, 1.f, 0.2f, "Feedback");
        configParam(PITCH_PARAM, -12.f, 12.f, 0.f, "Pitch");
        configParam(SCATTER_PARAM, 0.f, 1.f, 0.f, "Scatter");
        configSwitch(FREEZE_DELAY_BUTTON, 0.f, 1.f, 0.f, "Freeze Delay");
        configSwitch(REVERSE_BUTTON, 0.f, 1.f, 0.f, "Reverse Delay");
        configSwitch(MODE_BUTTON, 0.f, 1.f, 0.f, "Delay Mode");

        configParam(DRY_PARAM, 0.f, 1.f, 0.5f, "Dry Mix");
        configParam(WET_PARAM, 0.f, 1.f, 0.5f, "Wet Mix");


        configInput(FEEDBACK_INPUT, "Feedback");
        configInput(PITCH_INPUT, "Pitch");
        configInput(SCATTER_INPUT, "Scatter");
        configInput(FREEZE_DELAY_INPUT, "Freeze Gate");
        configInput(REVERSE_INPUT, "Reverse Gate");
        configInput(DRY_INPUT, "Dry Mix");
        configInput(WET_INPUT, "Wet Mix");


    }



    void process(const ProcessArgs& args) override {
        _lightTime[0].process(args.sampleTime);
        _lightTime[1].process(args.sampleTime);
        if (loopCounter % 8 == 0) {
            checkInputs(args);
            setParams(args);
        }
        if (loopCounter % 16 == 0) {
            
        }

        getDry();
        if (loopCounter % 2 == 0) {
            setFeed();       
            processDelay(args);
            setLights(args);
        }
        setOutputs(args);
        loopCounter++;
        if (loopCounter % INPUT_BUFFER_SPACE == 0) {
            loopCounter = 0;
        }
    }

    void getDry() {
        //copy mono to stereo if no        
        Dry[0] = isinAudL ? inputs[AUDIO_INPUT + 0].getVoltage(0) : 0.f;
        Dry[1] = isinAudR ? inputs[AUDIO_INPUT + 1].getVoltage(0) : Dry[0];
        
    }
    void setFeed() {
        

        switch (State) {
        default: {
            //fall through to default to dual mono
        }
        case DUAL_MONO: {
            //both lines get respective Drys
            for (int a = 0; a < 2; ++a) {
                float Feed = (Dry[a] + lastWet[a]);

                    _Delay[a].pushInput(Feed, frozenDelay, reverseDelay);
            
            }
            break;
        }
        case PING_PONG: {
            //each get the others output

            float FeedL = 0.f; 
            float FeedR = 0.f; 
            
            //if stereo, both get Dry, if mono, only give Dry to left
            if (isinAudR) {
                FeedL = (Dry[0] + lastWet[1]);
                FeedR = (Dry[1] + lastWet[0]);
            }
            else {
                FeedL = (Dry[0] + lastWet[1]);
                FeedR = (lastWet[0]);
            }
            

                _Delay[0].pushInput(FeedL, frozenDelay, reverseDelay);
                _Delay[1].pushInput(FeedR, frozenDelay, reverseDelay);


            break;
        }
        case CROSS_FEED: {
            //both get Dry and their own output, + a percentage of others output
            float FeedL = (Dry[0] + (lastWet[0] * 0.57) + (lastWet[1] * 0.43f));
            float FeedR = (Dry[1] + (lastWet[1]*0.57) + (lastWet[0] * 0.43f));

                _Delay[0].pushInput(FeedL, frozenDelay, reverseDelay);
                _Delay[1].pushInput(FeedR, frozenDelay, reverseDelay);

            break;
        }
        }
    }
    // modes: dual mono, pingpong, cross-feed
    void processDelay(const ProcessArgs& args) {
        
        float fcrawl = feedback * args.sampleRate; //crawl inupt for frozen, up to ~1 second size
        float Pitch = pitchPar + ((isinPitch) ? inputs[PITCH_INPUT].getVoltage(0) : 0.f);
        Pitch = rack::math::clamp(Pitch, -2.f, 2.f);
        for (int a = 0; a < 2; ++a) {
            

            float delayIn = (isinDelay[a] ? (inputs[DELAY_INPUT + a].getVoltage(0) * cvAttDTime[a]) / 5.f : 0.f);
            //float delayFreq = VoltToFreq(delayPar[a] + delayIn, 0.f, 4.f);
            float delayTime = rack::math::clamp(delayPar[a] + delayIn, 0.01f, 1.f);//(1.f / delayFreq);
            totaldelay[a] = rack::math::clamp(delayTime * INPUT_BUFFER_SPACE, 1.f, (float)INPUT_BUFFER_SPACE);

            _Delay[a].blockProcess(totaldelay[a], args.sampleTime, Pitch, fcrawl);
            float Wet = _Delay[a].CrossfadeOutput(args.sampleTime);
            Wet = _Stone[a].process(Wet, stonePar[a]);

            float wetscat = _scatter[a].SmearSerial(Wet);
            Wet = rack::math::crossfade(wetscat, Wet, Scatter);
       
            float Output = deClick[a].process(Wet);
            lastWet[a] = Wet * feedback;//filter and gain in feedbac loop. mixed by delay type at input
            lastSample[a] = DelayOutput[a];
            DelayOutput[a] = (Output);
        }
       // outputs[TESTOUT].setVoltage(_Delay[0].outRead[0] % DELAY_BUFFER_SPACE / (float)DELAY_BUFFER_SPACE, 0);
       // outputs[TESTOUT2].setVoltage(_Delay[0].inRead % INPUT_BUFFER_SPACE / (float)INPUT_BUFFER_SPACE, 0);

    }


    void setOutputs(const ProcessArgs& args) {
        float dryMixIn = isinDryMix ? inputs[DRY_INPUT].getVoltage(0) / 5.f : 0.f;
        float dryMix = rack::math::clamp((dryMixPar + dryMixIn), 0.f, 1.f);
        float wetMixIn = isinWetMix ? inputs[WET_INPUT].getVoltage(0) / 5.f : 0.f;
        float wetMix = rack::math::clamp((wetMixPar + wetMixIn), 0.f, 1.f);

        for (int a = 0; a < 2; ++a) {
            float upsamp = rack::math::crossfade(lastSample[a], DelayOutput[a], (loopCounter % 2) * 0.5f);
            float mixOut = (Dry[a] * dryMix) + (upsamp * wetMix);
            outputs[AUDIO_OUTPUT + a].setVoltage(mixOut, 0);
        }
    }


    void checkInputs(const ProcessArgs& args) {
        for (int a = 0; a < 2; ++a) {
            isinDelay[a] = inputs[DELAY_INPUT + a].isConnected();
            isinStone[a] = inputs[STONE_INPUT + a].isConnected();
        }
        isinFeed = inputs[FEEDBACK_INPUT].isConnected();
        isinFreezeD = inputs[FREEZE_DELAY_INPUT].isConnected();      
        isinReverse = inputs[REVERSE_INPUT].isConnected();
        isinPitch = inputs[PITCH_INPUT].isConnected();
        isinDryMix = inputs[DRY_INPUT].isConnected();
        isinWetMix = inputs[WET_INPUT].isConnected();
        isinScat = inputs[SCATTER_INPUT].isConnected();
        isinAudL = inputs[AUDIO_INPUT + 0].isConnected();
        isinAudR = inputs[AUDIO_INPUT + 1].isConnected();
       

        
    }
    
    void setParams(const ProcessArgs& args) {

        incrementButton(params[MODE_BUTTON].value, &modepress, 3, &State);
        std::string modename = "Mode - ";
        switch (State) {
        default: {}
        case DUAL_MONO: {
            std::string mode = modename + std::string("Dual-Mono");
            paramQuantities[MODE_BUTTON]->name = mode;
            break;
        }
        case PING_PONG: {
            std::string mode = modename + std::string("Ping-Pong");
            paramQuantities[MODE_BUTTON]->name = mode;
            break;
        }
        case CROSS_FEED: {
            std::string mode = modename + std::string("Cross-Feed");
            paramQuantities[MODE_BUTTON]->name = mode;
            break;
        }
        }

        for (int a = 0; a < 2; ++a) {
            delayPar[a] = params[DELAY_PARAM + a].value / 16.f;
            cvAttDTime[a] = params[DELAY_CV_PARAM + a].value;
            cvAttStone[a] = params[STONE_CV_PARAM + a].value;
        }
        feedbackPar = params[FEEDBACK_PARAM].value * 1.1f;
        feedback = feedbackPar * (isinFeed ? (inputs[FEEDBACK_INPUT].getVoltage(0) / 5.f) : 1.f);
        feedback = rack::math::clamp(feedback, 0.f, 1.1f);

        dryMixPar = params[DRY_PARAM].value;
        wetMixPar = params[WET_PARAM].value;
        pitchPar = params[PITCH_PARAM].value / 12.f;
        float scatPar = params[SCATTER_PARAM].value;
        Scatter = scatPar * (isinScat ? inputs[SCATTER_INPUT].getVoltage(0) / 5.f: 1.f);
        Scatter = 1.f - rack::math::clamp(Scatter, 0.f, 1.f);

        float stone[2];
        for (int s = 0; s < 2; ++s) {
            stonePar[s] = params[STONE_PARAM + s].value;
            float stonein = isinStone[s] ? inputs[STONE_INPUT + s].getVoltage(0) / 5.f : 0.f;
            stonePar[s] = stonePar[s] + (cvAttStone[s] * stonein);
            stone[s] = VoltToFreq(stonePar[s] * 2.f, 0.f, 300.f);
            stonePar[s] = rack::math::clamp(stonePar[s], 0.f, 1.f);
        }
          
        for (int a = 0; a < 2; ++a) {
            _scatter[a].setSmear(scasmall, Scatter, scabig, args.sampleRate * 0.5f);
            _Stone[a].setCut(stone[a], args.sampleRate * 0.5f);
        }

        float pickfreezeD = isinFreezeD ? inputs[FREEZE_DELAY_INPUT].getVoltage(0) : params[FREEZE_DELAY_BUTTON].value;
        float pickreverse = isinReverse ? inputs[REVERSE_INPUT].getVoltage(0) : params[REVERSE_BUTTON].value;
        frozenDelay = pickfreezeD >= 1.f;
        reverseDelay = pickreverse >= 1.f;

        for (int a = 0; a < 2; ++a) {
            deClick[a].setParameters(deClick[a].Type::LOWPASS, 0.22676f, 10.f, 1.f);
        }
    }

    void setLights(const ProcessArgs& args) {
        ButtonPress[0] = frozenDelay;
        ButtonPress[1] = reverseDelay;
        ButtonPress[2] = modepress;
        float lr, lb, lg;
        float rr, rb, rg;

        float butcol[3] = { 0.12, 0.37, 0.91 };
        for (int b = 0; b < 3; ++b) {
            lights[FREEZE_LIGHT + b].setBrightness(butcol[b] * ButtonPress[0]);
            lights[REVERSE_LIGHT + b].setBrightness(butcol[b] * ButtonPress[1]);
            lights[MODE_LIGHT + b].setBrightness((butcol[b] * (State == b)));
        }
        float huel = (totaldelay[0] / INPUT_BUFFER_SPACE) * 360.f;
        float huer = (totaldelay[1] / INPUT_BUFFER_SPACE) * 360.f;
        Components::HSLtoRGB(huel, 0.9, 0.4, &lr, &lb, &lg);
        Components::HSLtoRGB(huer, 0.9, 0.4, &rr, &rb, &rg);
        float Ltime = 1.f - ((_lightTime[0].getTime() * args.sampleRate * 0.5f) / totaldelay[0]);
        if (Ltime < 0.f) _lightTime[0].reset();
        Ltime = rack::math::clamp(Ltime - 0.2f, 0.f, 1.f);
        float Rtime = 1.f - ((_lightTime[1].getTime() * args.sampleRate * 0.5f) / totaldelay[1]);
        if (Rtime < 0.f) _lightTime[1].reset();
        Rtime = rack::math::clamp(Rtime - 0.2f, 0.f, 1.f);

        lights[LEFT_WALL_LIGHT + 0].setBrightness(Ltime * (lr / 255.f));
        lights[LEFT_WALL_LIGHT + 1].setBrightness(Ltime * (lg / 255.f));
        lights[LEFT_WALL_LIGHT + 2].setBrightness(Ltime * (lb / 255.f));
        lights[RIGHT_WALL_LIGHT + 0].setBrightness(Rtime * (rr / 255.f));
        lights[RIGHT_WALL_LIGHT + 1].setBrightness(Rtime * (rg / 255.f));
        lights[RIGHT_WALL_LIGHT + 2].setBrightness(Rtime * (rb / 255.f));
    }

    

    void onReset(const ResetEvent& e) override {
        for (int a = 0; a < 2; ++a) {
            _Delay[a].clear();
            _lightTime[a].reset();
        }
    }


    json_t* dataToJson() override {
        json_t* rootJ = json_object();
        json_t* stateJ = json_integer(State);
        json_object_set_new(rootJ, "state", stateJ);
        return rootJ;
    }

    void dataFromJson(json_t* rootJ) override {
        json_t* stateJ = json_object_get(rootJ, "state");
        if(stateJ) State = json_integer_value(stateJ);
    }

};

//template <typename TBase = RedGreenBlueLight>
struct CanyonLLight : Components::TColorSVGLight {
    CanyonLLight() {
        this->setSvg(Svg::load(asset::plugin(pluginInstance, "res/CanyonLights/CanWallL.svg")));
    }
};
//using CanyonLLight = TCanyonLLight<>;

//template <typename TBase = RedGreenBlueLight>
struct CanyonRLight : Components::TColorSVGLight {
    CanyonRLight() {
        this->setSvg(Svg::load(asset::plugin(pluginInstance, "res/CanyonLights/CanWallR.svg")));
    }
};
//using CanyonRLight = TCanyonRLight<>;

struct CanButtonLight : Components::TColorSVGLight {
    CanButtonLight() {
        this->setSvg(Svg::load(asset::plugin(pluginInstance, "res/LilButtonLight.svg")));
    }
};

struct modeLight : SvgWidget {
    CanyonModule* module;
    int which = 0;
    float isHigh = 0.f;
    float modecolor = 0.f;
    modeLight() {
    }
    void Svg(std::string path) {
        this->setSvg(Svg::load(asset::plugin(pluginInstance, path)));
    }
    void drawLayer(const DrawArgs& args, int layer) override {
        if (module && svg && layer == 1) {
            isHigh = module->ButtonPress[which];
            modecolor = module->State * 0.24f;
            nvgGlobalCompositeBlendFunc(args.vg, NVG_ONE_MINUS_DST_COLOR, NVG_ONE);

            for (auto s = svg->handle->shapes; s; s = s->next) {
                nvgFillColor(args.vg, nvgHSL(0.5f + modecolor, 0.8, isHigh / 2.f));

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

struct FreezeLight : SvgWidget {
    CanyonModule* module;
    int which = 0;
    float isHigh = 0.f;
    FreezeLight() {
    }
    void Svg(std::string path) {
        this->setSvg(Svg::load(asset::plugin(pluginInstance, path)));
    }
    void drawLayer(const DrawArgs& args, int layer) override {
        if (module && svg && layer == 1) {
            isHigh = module->ButtonPress[which];
            nvgGlobalCompositeBlendFunc(args.vg, NVG_ONE_MINUS_DST_COLOR, NVG_ONE);

            for (auto s = svg->handle->shapes; s; s = s->next) {
                nvgFillColor(args.vg, nvgHSL(0.5f, 0.8, isHigh / 2.f));
                
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
struct CanyonPanelWidget : ModuleWidget {

    //include struct for logo here so it has modules name
    #include "Theme/LogoLight.h"

    CanyonPanelWidget(CanyonModule* module) {
        setModule(module);
       // LydD::Components::setPlugin(pluginInstance);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Canyon_panel.svg")));

		addChild(createWidget<ScrewSilver>(Vec(15, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 30, 0)));
		addChild(createWidget<ScrewSilver>(Vec(15, 365)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 30, 365)));

      


        addParam(createParam<RoundLargeBlackKnob>(Vec(10, 50.5), module, CanyonModule::DELAY_PARAM + 0));
        addParam(createParam<RoundLargeBlackKnob>(Vec(92.5, 50.5), module, CanyonModule::DELAY_PARAM + 1));
        addParam(createParam<Trimpot>(Vec(52.5, 93), module, CanyonModule::DELAY_CV_PARAM + 0));
        addParam(createParam<Trimpot>(Vec(79 ,93), module, CanyonModule::DELAY_CV_PARAM + 1));
        addParam(createParam<RoundSmallBlackKnob>(Vec(119, 233), module, CanyonModule::FEEDBACK_PARAM));
        addParam(createParam<RoundBlackKnob>(Vec(27, 115.5), module, CanyonModule::STONE_PARAM + 0));
        addParam(createParam<RoundBlackKnob>(Vec(91.5, 115.5), module, CanyonModule::STONE_PARAM + 1));
        addParam(createParam<Trimpot>(Vec(3.5, 107), module, CanyonModule::STONE_CV_PARAM + 0));
        addParam(createParam<Trimpot>(Vec(128, 107), module, CanyonModule::STONE_CV_PARAM + 1));
        addParam(createParam<LilButton>(Vec(120, 162), module, CanyonModule::FREEZE_DELAY_BUTTON));
        addParam(createParam<RoundSmallBlackKnob>(Vec(11, 169), module, CanyonModule::PITCH_PARAM));
        addParam(createParam<RoundSmallBlackKnob>(Vec(63.5, 195.5), module, CanyonModule::SCATTER_PARAM));

        addParam(createParam<LilButton>(Vec(120, 202), module, CanyonModule::REVERSE_BUTTON));
        addParam(createParam<LilButton>(Vec(64.5, 153.5), module, CanyonModule::MODE_BUTTON));
        
        addParam(createParam<RoundSmallBlackKnob>(Vec(8.5, 226.5), module, CanyonModule::DRY_PARAM));
        addParam(createParam<RoundSmallBlackKnob>(Vec(41.5, 226.5), module, CanyonModule::WET_PARAM));

        addInput(createInput<PurplePort>(Vec(5, 338), module, CanyonModule::AUDIO_INPUT + 0));
        addInput(createInput<PurplePort>(Vec(33, 338), module, CanyonModule::AUDIO_INPUT + 1));
        addInput(createInput<PurplePort>(Vec(5, 272.5), module, CanyonModule::DELAY_INPUT + 0));
        addInput(createInput<PurplePort>(Vec(33, 272.5), module, CanyonModule::DELAY_INPUT + 1));
        addInput(createInput<PurplePort>(Vec(5, 305), module, CanyonModule::STONE_INPUT + 0));
        addInput(createInput<PurplePort>(Vec(33, 305), module, CanyonModule::STONE_INPUT + 1));
        addInput(createInput<PurplePort>(Vec(90.5, 272.5), module, CanyonModule::PITCH_INPUT));
        addInput(createInput<PurplePort>(Vec(61.5, 272.5), module, CanyonModule::SCATTER_INPUT));
        addInput(createInput<PurplePort>(Vec(90.5, 305), module, CanyonModule::FEEDBACK_INPUT));
        addInput(createInput<PurplePort>(Vec(118.5, 272.5), module, CanyonModule::FREEZE_DELAY_INPUT));
        addInput(createInput<PurplePort>(Vec(118.5, 305), module, CanyonModule::REVERSE_INPUT));
        addInput(createInput<PurplePort>(Vec(61.5, 305), module, CanyonModule::DRY_INPUT));
        addInput(createInput<PurplePort>(Vec(61.5, 337), module, CanyonModule::WET_INPUT));



        addOutput(createOutput<PurplePort>(Vec(90, 338), module, CanyonModule::AUDIO_OUTPUT + 0));
        addOutput(createOutput<PurplePort>(Vec(118.5, 338), module, CanyonModule::AUDIO_OUTPUT + 1));
       // addOutput(createOutput<PurplePort>(Vec(0, 88), module, CanyonModule::TESTOUT));
       // addOutput(createOutput<PurplePort>(Vec(0, 120), module, CanyonModule::TESTOUT2));
        if (module) {
            addChild(createLight<CanButtonLight>(Vec(120, 162), module, CanyonModule::FREEZE_LIGHT + 0));
            addChild(createLight<CanButtonLight>(Vec(120, 202), module, CanyonModule::REVERSE_LIGHT + 0));
            addChild(createLight<CanButtonLight>(Vec(64.5, 153.5), module, CanyonModule::MODE_LIGHT + 0));

            addChild(createLight<CanyonLLight>(Vec(0, 138.5), module, CanyonModule::LEFT_WALL_LIGHT));
            addChild(createLight<CanyonRLight>(Vec(96.5, 138.5), module, CanyonModule::RIGHT_WALL_LIGHT));

            //must be called 'logoPos'for all modules 
            Vec logoPos = Vec(((15.f * HP) / 2.f) - 12.5, 363.f);
            CanyonModule* module = dynamic_cast<CanyonModule*>(this->module);
            assert(module);
            #include "Theme/LogoChild.h"
            
        }

    }

};

Model* modelCanyon = createModel<CanyonModule, CanyonPanelWidget>("Canyon-Echo");