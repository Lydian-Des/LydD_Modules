#include "plugin.hpp"
#include <cmath>
#include "Lydapi/LydEQ.h"
#include "Lydapi/LydDelayLine.h"
#include "Lydapi/LydTime.h"

#define MODULE_NAME CanyonModule
#define PANEL "Canyon_panel.svg"
#define HP 10


static const int maxPolyphony = 1;

using namespace LydD;
using namespace LydD::Delay;

constexpr static size_t EIGHT_SECONDS = 352800;
constexpr static size_t INPUT_BUFFER_SPACE = 524288; //closest power of 2 > 8 seconds
constexpr static size_t DELAY_BUFFER_SPACE = 1024;
constexpr static int NUM_CHAN = 10;

const std::string timeString[17] = { "1/5", "1/4", "1/3", "1/2", "1", " 1 1/4",
                               "1 1/2", "1 3/4", "2", "2 1/2", "3", "3 1/2", "4", "5", "6", "7", "8"};

const float clocked_fractions[17] = { 1.f / 5.f, 1.f / 4.f, 1.f / 3.f, 1.f / 2.f, 1.f, 1.25f, 1.5f, 1.75f,
                   2.f, 2.5f, 3.f, 3.5f, 4.f, 5.f, 6.f, 7.f, 8.f };

//knob display subclass for clocked mode (doesnt fix typing though)
struct DivDisplay : rack::engine::ParamQuantity {

    bool clocked = false;
    void isClocked(bool clk) {
        this->clocked = clk;
    }
    //0--1 -> 0--2 -> 0--8, 0.5 -> 1 stays 1
    float cubepar(float par) {
        float b = par * 2.f;
        return b * b * b;
    }
    //0--1 -> 0--16, 0.5 -> 0.25 -> 4(index as float)
    //if par = 0., index = 7 (.84) and result is 1.75
    float clockpar(float par) {
        float b = par * par;
        b *= 16.f;
        return rack::math::clamp(b, 0.f, 16.f);
    }

    float uncube_entered(float val) {
        if (val == 0.f) return 0.f;
        float unc = std::cbrt(val);
        unc /= 2.f;
        return unc;
    }
    //wont ever quite reach the 17th index on purpose
    //if enter 1.75, must end up with ~0.7
    float find_nearest_index(float val) {
        for (int i = 0; i < 16; ++i) {
            if (val >= clocked_fractions[i] && val < clocked_fractions[i + 1]) return i;
        }
        if (val < clocked_fractions[0]) return 0;
        if (val > clocked_fractions[15]) return 16;

        return 1.f;
    }
    //1 -> index 4, 2-> index 8, etc
    float clock_fraction(float val) {
        float indv = find_nearest_index(val);
        float fr = std::sqrt(indv / 16.f) + 0.01f;//make sure it ticks over into the proper section
        return fr;
    }

    void setDisplayValue(float displayValue) override {
        if (module) {
            float newval = uncube_entered(displayValue);
            if (clocked) {
                newval = clock_fraction(displayValue);
            }
            ParamQuantity::setValue(newval);
        }
    }

    std::string getDisplayValueString() override {
        //CanyonModule* modl = dynamic_cast<CanyonModule*>(this->module);

        if (module) {
            auto val = getValue();
            if (clocked) {
                int valdex = clockpar(val);
                return timeString[valdex];
            }
            else {
                return std::to_string(cubepar(val));
            }
        }
        return ParamQuantity::getDisplayValueString();
    }
};


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
        ENUMS(CLOCK_INPUT, 2),
        NUM_INPUTS
    };
    enum OutputIds {

        
        ENUMS(AUDIO_OUTPUT, 2),
        //enable if need debug
        /*TESTOUT,
        TESTOUT2,*/
        NUM_OUTPUTS
    };
    enum LightIds {
        ENUMS(FREEZE_LIGHT, 3),
        ENUMS(REVERSE_LIGHT, 3),
        ENUMS(MODE_LIGHT, 3),
        ENUMS(LEFT_WALL_LIGHT, 3),
        ENUMS(RIGHT_WALL_LIGHT, 3),
        ENUMS(SUN_LIGHT, 3),
        ENUMS(MOON_LIGHT, 3),
        NUM_LIGHTS
    };


    enum States {
        DUAL_MONO,
        PING_PONG,
        CROSS_FEED,
        NUM_STATES
    };
    int State = 0;


    Tricked_Out_CFDelayLine<float, INPUT_BUFFER_SPACE, DELAY_BUFFER_SPACE, NUM_CHAN> _Delay;
    Filter::SFRCFilter< float> _Stone[2];
    Time::AverageTimer<float, 2> _extClock[2];
    rack::dsp::BiquadFilter _deClick[2];
    rack::dsp::Timer _lightTime[2];
    float scatmult[8] = { 0.85f, 0.39f, 0.67f, 0.17f, 0.93f, 0.47f, 0.79f, 0.26f };
    Filter::SFRCFilter<float> _PitchSmooth[2];

    bool ButtonPress[3] = { 0, 0, 0 };

    int currentPolyphony = 1;
    int currentBanks = 1;
    std::atomic<size_t> loopCounter;

    bool isinDelay[2] = { false, false };
    bool isinStone[2] = { false, false };
    bool isinClock[2] = { false, false };
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


    float totaldelay[NUM_CHAN] = { 100, 100 };
    float delayPar[2] = { 0.f, 0.f };
    float stonePar[2] = { 0.f, 0.f };
    float pitchPar = 0.f;
    float extClockTime[2] = { 0.f, 0.f };
    float Feedback = 0.f;
    float Scatter[(NUM_CHAN - 2) / 2] = {0.f};
    float dryMixPar = 0.f;
    float wetMixPar = 0.f;

    float Dry[NUM_CHAN];
    FrameStereo<float, NUM_CHAN> lastWet;
    float DelayOutput[NUM_CHAN] = { 0, 0 };
    float lastSample[NUM_CHAN] = { 0.f, 0.f };
    float subsamplerate;

    CanyonModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        std::string side[2] = { " - Left", " - Right" };
        for (int a = 0; a < 2; ++a) {
            configParam<DivDisplay>(DELAY_PARAM + a, 0.00625f, 1.f, 0.5f, "Echo Time" + side[a]);
            configParam(DELAY_CV_PARAM + a, -1.f, 1.f, 0.f, "Echo CV" + side[a]);
            configParam(STONE_PARAM + a, 0.f, 1.f, 0.5f, "Light/Dark" + side[a]);
            configParam(STONE_CV_PARAM + a, -1.f, 1.f, 0.f, "L/D CV" + side[a]);

            configInput(DELAY_INPUT + a, "Echo" + side[a]);
            configInput(STONE_INPUT + a, "Light/Dark" + side[a]);
            configInput(AUDIO_INPUT + a, "Audio" + side[a]);
            configInput(CLOCK_INPUT + a, "Clock" + side[a]);

            configOutput(AUDIO_OUTPUT + a, "Audio" + side[a]);
        }

        configParam(FEEDBACK_PARAM, 0.f, 1.f, 0.2f, "Feedback");
        configParam(PITCH_PARAM, -12.f, 12.f, 0.f, "Pitch");
        configParam(SCATTER_PARAM, 0.f, 4.f, 0.f, "Scatter");
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

        loopCounter.store(0);
    }


    void process(const ProcessArgs& args) override {
        _lightTime[0].process(args.sampleTime);
        _lightTime[1].process(args.sampleTime);
        subsamplerate = args.sampleRate ;
        size_t loops = loopCounter.load();
        if (loops % 8 == 0) {
            checkInputs(args);
            setParams(args);
        }

        getExtClock(args);
        
        getDry(args);
            setFeed();
            processDelay(args);
        setOutputs(args);
        
        if (loops % 16 == 0) {
            setLights(args);
            //loopCounter.store(0);
        }
        //else {
            loopCounter.store(loops + 1);
        //}

        
    }

    void getExtClock(const ProcessArgs& args) {
        for (int c = 0; c < 2; ++c) {
            if (isinClock[c]) {
                float extclk = inputs[CLOCK_INPUT + c].getVoltage(0);
                _extClock[c].store(extclk, args.sampleTime);
                extClockTime[c] = _extClock[c].average() * args.sampleRate;
            }
        }
    }


    void getDry(const ProcessArgs& args) {
        //copy mono to stereo if no        
        float tempdryl = isinAudL ? inputs[AUDIO_INPUT + 0].getVoltage(0) : 0.f;
        float tempdryr = isinAudR ? inputs[AUDIO_INPUT + 1].getVoltage(0) : tempdryl;
        // pre-filter incoming audio into all channels
        for (int d = 0; d < NUM_CHAN; d += 2) {
            Dry[d] = Filter::lazyAlias(tempdryl, Dry[d]);
            Dry[d + 1] = Filter::lazyAlias(tempdryr, Dry[d + 1]);
        }

        
    }
    void setFeed() {
        float Feed[NUM_CHAN]{ 0.f, 0.f };
        switch (State) {
        default: {
            //fall through to default to dual mono
        }
        case DUAL_MONO: {
            //both lines get respective Drys
            Feed[0] = Dry[0] + lastWet[0];
            Feed[1] = Dry[1] + lastWet[1];
            for (int a = 2; a < NUM_CHAN; ++a) {
                int s = (a - 2) / 2;
                float feed = /*Dry[a]*/ lastWet[a % 2] * Scatter[s] + lastWet[a];
                Feed[a] = feed;          
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
            Feed[0] = FeedL;
            Feed[1] = FeedR;
            for (int a = 2; a < NUM_CHAN; a += 2) {
                int b = a + 1;
                int s = (a - 2) / 2;
                int t = (b - 2) / 2;
                Feed[a] = /*Dry[a]*/ lastWet[0] * Scatter[s] + lastWet[b];
                Feed[b] = /*Dry[b]*/ lastWet[1] * Scatter[t] + lastWet[a];
            }

            break;
        }
        case CROSS_FEED: {
            //both get Dry and their own output, + a percentage of others output
            float FeedL = (Dry[0] + (lastWet[0] * 0.57) + (lastWet[1] * 0.43f));
            float FeedR = (Dry[1] + (lastWet[1]*0.57) + (lastWet[0] * 0.43f));
            Feed[0] = FeedL;
            Feed[1] = FeedR;
            for (int a = 2; a < NUM_CHAN; ++a) {
                int s = (a - 2) / 2;
                Feed[a] = /*Dry[a]*/ lastWet[a % 2] * Scatter[s] + (lastWet[a] * 0.57) + (lastWet[NUM_CHAN - (a - 2)] * 0.43f);
            }
            break;
        }
        }
        _Delay.PushInput(Feed);
    }
    // modes: dual mono, pingpong, cross-feed
    void processDelay(const ProcessArgs& args) {

        _Delay.BlockProcess();
        float* Wet = _Delay.CrossfadeOutput();
        float OutCo[2];
        FrameStereo<float, NUM_CHAN / 2> lastL;
        FrameStereo<float, NUM_CHAN / 2> lastR;
        //main stereo pair
        for (int a = 0; a < 2; ++a) {
            OutCo[a] = Wet[a];
        }
        lastL[0] = Wet[0] * Feedback;
        lastR[0] = Wet[1] * Feedback;
        for (int l = 2; l < NUM_CHAN; l += 2) {
            int r = l + 1;
            OutCo[l % 2] += Wet[l];
            OutCo[r % 2] += Wet[r];
            int id = l / 2;
            lastL[id] = Wet[l] * Feedback; 
            lastR[id] = Wet[r] * Feedback;
        }
        //lastL = _Stone[0].process(lastL, stonePar[0]);
        //lastR = _Stone[1].process(lastR, stonePar[1]);
        lastWet.interleave_LR(lastL, lastR);
        for (int b = 0; b < 2; ++b) {
            
            float Output = _deClick[b].process(OutCo[b]);
            lastSample[b] = DelayOutput[b];
            DelayOutput[b] =  _Stone[b].process(Output, stonePar[b]);
        }
        //debugOutput();
    }

    void debugOutput() {
        //debug outputs
       //float w1 = (_Delay.inBuf._Rhead % (_Delay.inBuf.SI)) / float(_Delay.inBuf.SI);
       //float w2 = (_Delay.inBuf._Whead % (_Delay.inBuf.SI)) / float(_Delay.inBuf.SI);
       //float test[2] = { w1, w2 };
       //float test1 =  _Delay._blockWindow.getWindowInd(_Delay.outBuf[0]._Rhead % DELAY_BUFFER_SPACE);
       //float test2 =  _Delay._blockWindow.getWindowInd(_Delay.outBuf[1]._Rhead % DELAY_BUFFER_SPACE);
       //outputs[TESTOUT].setChannels(2);
       //outputs[TESTOUT2].setChannels(2);
       //for (int c = 0; c < 2; ++c) {
           //outputs[TESTOUT].setVoltage(test[c], c);
       //}
       //outputs[TESTOUT2].setVoltage(test1, 0);
       //outputs[TESTOUT2].setVoltage(test2, 1);
    }

    void setOutputs(const ProcessArgs& args) {
        float dryMixIn = isinDryMix ? inputs[DRY_INPUT].getVoltage(0) / 5.f : 0.f;
        float dryMix = rack::math::clamp((dryMixPar + dryMixIn), 0.f, 1.f);
        float wetMixIn = isinWetMix ? inputs[WET_INPUT].getVoltage(0) / 5.f : 0.f;
        float wetMix = rack::math::clamp((wetMixPar + wetMixIn), 0.f, 1.f);

        for (int a = 0; a < 2; ++a) {
            float upsamp = Filter::lazyAlias(DelayOutput[a], lastSample[a]);
           // upsamp = _PitchSmooth[a].process(upsamp, pitchPar);
            float mixOut = (Dry[a] * dryMix) + (upsamp * wetMix);
            outputs[AUDIO_OUTPUT + a].setVoltage(mixOut, 0);
        }
    }

    void sendDisplayClock(int clk) {
        ParamQuantity* delayQuantity = getParamQuantity(DELAY_PARAM + clk);
        DivDisplay* delayDisplay = dynamic_cast<DivDisplay*>(delayQuantity);
        delayDisplay->isClocked(isinClock[clk]);

    }
    void checkInputs(const ProcessArgs& args) {
        for (int a = 0; a < 2; ++a) {
            isinDelay[a] = inputs[DELAY_INPUT + a].isConnected();
            isinStone[a] = inputs[STONE_INPUT + a].isConnected();
            isinClock[a] = inputs[CLOCK_INPUT + a].isConnected();

            sendDisplayClock(a);
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


        float feedbackPar = params[FEEDBACK_PARAM].value * 1.f;
        Feedback = feedbackPar * (isinFeed ? (inputs[FEEDBACK_INPUT].getVoltage(0) / 5.f) : 1.f);
        Feedback = rack::math::clamp(Feedback, 0.f, 1.f);

        dryMixPar = params[DRY_PARAM].value;
        wetMixPar = params[WET_PARAM].value;
        float pPar = params[PITCH_PARAM].value / 12.f;
        float froloop = Feedback * subsamplerate * 2.f; //crawl inupt for frozen, up to ~1 second size
        float pitch = pPar + ((isinPitch) ? inputs[PITCH_INPUT].getVoltage(0) : 0.f);
        pitch = rack::math::clamp(pitch, -2.f, 2.f);
        //no matter what direction pitch moves, turn the cutoff down
        pitchPar = -abs(pitch);
        float pitcut = VoltToFreq(pitchPar, 0.f, 900.f);
        _Delay.setPitch(pitch);
        for (int p = 0; p < 2; ++p) {
            _PitchSmooth[p].setCut(pitcut, subsamplerate);
        }

        float scatPar = params[SCATTER_PARAM].value;
        float scatlr = scatPar * (isinScat ? inputs[SCATTER_INPUT].getVoltage(0) / 5.f : 1.f);
        scatlr = rack::math::clamp(scatlr, 0.f, 4.f);
        for (int i = 0; i < (NUM_CHAN - 2) / 2; ++i) {
            float wght = scatlr - int(scatlr);
            float gain = scatlr < i ? 0.f : wght;
            gain = scatlr >= i + 1.f ? 1.f : gain;
            Scatter[i] = gain;
        }
        // /5.f should give avg of -1-1
        float delay_input[2];
        delay_input[0] = isinDelay[0] ? inputs[DELAY_INPUT + 0].getVoltage(0) / 5.f : 0.f;
        delay_input[1] = isinDelay[1] ? inputs[DELAY_INPUT + 1].getVoltage(0) / 5.f : delay_input[0];
        for (int a = 0; a < 2; ++a) {
            float dp = params[DELAY_PARAM + a].value;
            float dp2 = dp * 2.f;
            float delpar = dp2 * dp2 * dp2;//cubic from 0-2 gets 0-8 keeping 1 = 1(0.5parval)
            float cv_atten = params[DELAY_CV_PARAM + a].value;
            //input can also scrub whole length but its linear
            float delayIn = (delay_input[a] * cv_atten) * 8.f;
            float delayTime = rack::math::clamp((delpar + delayIn) / 8.f, 0.001f, 1.3f);

            if (isinClock[a]) {
                float dl = dp * dp;
                dl *= 16.f; //sqr then scale so 0.5 =0.25=index 4
                float lin = rack::math::clamp(dl + (delayIn * 2), 0.f, 15.99f);
                int lindex = int(lin); 
                float delayfreq = clocked_fractions[lindex]; 
                float realtime = extClockTime[a] * delayfreq; //requested time with division/multiple in seconds
                delayTime = realtime;
                //delayTime *= INPUT_BUFFER_SPACE;
            }
            else {
                delayTime *= EIGHT_SECONDS;
            }
            totaldelay[a] = rack::math::clamp(delayTime, 10.f, (float)INPUT_BUFFER_SPACE);
        }

        for (int c = 2; c < NUM_CHAN; c += 2) {
            int s = (c - 2) / 2;      
            float scal = rack::math::clamp(totaldelay[0] * scatmult[s], 10.f, float(INPUT_BUFFER_SPACE));
            float scar = rack::math::clamp(totaldelay[1] * scatmult[s], 10.f, float(INPUT_BUFFER_SPACE));
            totaldelay[c] = scal;
            totaldelay[c + 1] = scar;
        }
        _Delay.setDelay(totaldelay);

        float pickfreezeD = isinFreezeD ? inputs[FREEZE_DELAY_INPUT].getVoltage(0) : params[FREEZE_DELAY_BUTTON].value;
        frozenDelay = pickfreezeD >= 1.f;
        _Delay.setFreeze(frozenDelay, froloop);

        float pickreverse = isinReverse ? inputs[REVERSE_INPUT].getVoltage(0) : params[REVERSE_BUTTON].value;
        reverseDelay = pickreverse >= 1.f;
        _Delay.setReverse(reverseDelay, froloop * 2.f);


        float stone_input[2];
        stone_input[0] = isinStone[0] ? inputs[STONE_INPUT + 0].getVoltage(0) / 5.f : 0.f;
        stone_input[1] = isinStone[1] ? inputs[STONE_INPUT + 1].getVoltage(0) / 5.f : stone_input[0];
        for (int s = 0; s < 2; ++s) {
            stonePar[s] = params[STONE_PARAM + s].value;
            float cv_atten = params[STONE_CV_PARAM + s].value;
            float stonein = stone_input[s] * cv_atten;
            stonePar[s] = stonePar[s] + stonein;
            stonePar[s] = rack::math::clamp(stonePar[s], 0.f, 1.f);
            float stone = VoltToFreq(stonePar[s] * 2.f, 1.f, 300.f);
            _Stone[s].setCut(stone, subsamplerate);
        }         

        for (int a = 0; a < 2; ++a) {
            _deClick[a].setParameters(_deClick[a].Type::LOWPASS, 0.32676f, 1.f, 1.f);
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
        float huel = 25 + (totaldelay[0] / (8.f * args.sampleRate)) * 460.f;
        float huer = 25 + (totaldelay[1] / (8.f * args.sampleRate)) * 460.f;
        Components::HSLtoRGB(huel, 0.9, 0.4, &lr, &lb, &lg);
        Components::HSLtoRGB(huer, 0.9, 0.4, &rr, &rb, &rg);
        float Ltime = 1.f - ((_lightTime[0].getTime() * subsamplerate) / totaldelay[0]);
        if (Ltime < 0.f) _lightTime[0].reset();
        Ltime = rack::math::clamp(Ltime - 0.2f, 0.f, 1.f);
        float Rtime = 1.f - ((_lightTime[1].getTime() * subsamplerate) / totaldelay[1]);
        if (Rtime < 0.f) _lightTime[1].reset();
        Rtime = rack::math::clamp(Rtime - 0.2f, 0.f, 1.f);

        lights[LEFT_WALL_LIGHT + 0].setBrightness(Ltime * (lr / 255.f));
        lights[LEFT_WALL_LIGHT + 1].setBrightness(Ltime * (lg / 255.f));
        lights[LEFT_WALL_LIGHT + 2].setBrightness(Ltime * (lb / 255.f));
        lights[RIGHT_WALL_LIGHT + 0].setBrightness(Rtime * (rr / 255.f));
        lights[RIGHT_WALL_LIGHT + 1].setBrightness(Rtime * (rg / 255.f));
        lights[RIGHT_WALL_LIGHT + 2].setBrightness(Rtime * (rb / 255.f));

        lights[SUN_LIGHT + 0].setBrightness(stonePar[1]);
        lights[SUN_LIGHT + 1].setBrightness(stonePar[1] * 0.5f);
        lights[SUN_LIGHT + 2].setBrightness(0.f);
        lights[MOON_LIGHT + 0].setBrightness(0.f);
        lights[MOON_LIGHT + 1].setBrightness(stonePar[0] * 0.2f);
        lights[MOON_LIGHT + 2].setBrightness(stonePar[0]);
    }

    

    void onReset(const ResetEvent& e) override {
        _Delay.clear();
        for (int a = 0; a < 2; ++a) {
   
            _lightTime[a].reset();
            _extClock[a].reset();
        }
        loopCounter.store(0);
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
struct CanyonLLight : Components::TColorSVGLight_Stroke {
    CanyonLLight() {
        this->setSvg(Svg::load(asset::plugin(pluginInstance, "res/CanyonLights/CanWallL.svg")));
    }
};
//using CanyonLLight = TCanyonLLight<>;

//template <typename TBase = RedGreenBlueLight>
struct CanyonRLight : Components::TColorSVGLight_Stroke {
    CanyonRLight() {
        this->setSvg(Svg::load(asset::plugin(pluginInstance, "res/CanyonLights/CanWallR.svg")));
    }
};
//using CanyonRLight = TCanyonRLight<>;
struct MoonLight : Components::TColorSVGLight {
    MoonLight() {
        this->setSvg(Svg::load(asset::plugin(pluginInstance, "res/CanyonLights/Moon.svg")));
    }
};
struct SunLight : Components::TColorSVGLight {
    SunLight() {
        this->setSvg(Svg::load(asset::plugin(pluginInstance, "res/CanyonLights/Sun.svg")));
    }
};
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

      


        addParam(createParam<RoundLargeBlackKnob>(Vec(10, 50.8), module, CanyonModule::DELAY_PARAM + 0));
        addParam(createParam<RoundLargeBlackKnob>(Vec(104, 50.8), module, CanyonModule::DELAY_PARAM + 1));
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

        float xins[5] = { 7.553f, 35.553f, 63.553f, 91.553f, 119.553f };
        float yins[3] = { 272.5f, 305.f, 338.f };

        addInput(createInput<PurplePort>(Vec(xins[0], yins[2]), module, CanyonModule::AUDIO_INPUT + 0));
        addInput(createInput<PurplePort>(Vec(xins[1], yins[2]), module, CanyonModule::AUDIO_INPUT + 1));
        addInput(createInput<PurplePort>(Vec(xins[0], yins[0]), module, CanyonModule::DELAY_INPUT + 0));
        addInput(createInput<PurplePort>(Vec(xins[1], yins[0]), module, CanyonModule::DELAY_INPUT + 1));
        addInput(createInput<PurplePort>(Vec(xins[0], yins[1]), module, CanyonModule::STONE_INPUT + 0));
        addInput(createInput<PurplePort>(Vec(xins[1], yins[1]), module, CanyonModule::STONE_INPUT + 1));
        addInput(createInput<PurplePort>(Vec(xins[3], yins[0]), module, CanyonModule::PITCH_INPUT));
        addInput(createInput<PurplePort>(Vec(xins[2], yins[0]), module, CanyonModule::SCATTER_INPUT));
        addInput(createInput<PurplePort>(Vec(xins[3], yins[1]), module, CanyonModule::FEEDBACK_INPUT));
        addInput(createInput<PurplePort>(Vec(xins[4], yins[0]), module, CanyonModule::FREEZE_DELAY_INPUT));
        addInput(createInput<PurplePort>(Vec(xins[4], yins[1]), module, CanyonModule::REVERSE_INPUT));
        addInput(createInput<PurplePort>(Vec(xins[2], yins[1]), module, CanyonModule::DRY_INPUT));
        addInput(createInput<PurplePort>(Vec(xins[2], yins[2]), module, CanyonModule::WET_INPUT));

        addInput(createInput<PurplePort>(Vec(49.6, 34.5), module, CanyonModule::CLOCK_INPUT + 0));
        addInput(createInput<PurplePort>(Vec(77.4, 34.5), module, CanyonModule::CLOCK_INPUT + 1));


        addOutput(createOutput<PurplePort>(Vec(xins[3], yins[2]), module, CanyonModule::AUDIO_OUTPUT + 0));
        addOutput(createOutput<PurplePort>(Vec(xins[4], yins[2]), module, CanyonModule::AUDIO_OUTPUT + 1));
        //enable for debug
        /*addOutput(createOutput<PurplePort>(Vec(0, 88), module, CanyonModule::TESTOUT));
        addOutput(createOutput<PurplePort>(Vec(0, 120), module, CanyonModule::TESTOUT2));*/
        if (module) {
            addChild(createLight<CanButtonLight>(Vec(120, 162), module, CanyonModule::FREEZE_LIGHT + 0));
            addChild(createLight<CanButtonLight>(Vec(120, 202), module, CanyonModule::REVERSE_LIGHT + 0));
            addChild(createLight<CanButtonLight>(Vec(64.5, 153.5), module, CanyonModule::MODE_LIGHT + 0));

            addChild(createLight<CanyonLLight>(Vec(0, 100.344), module, CanyonModule::LEFT_WALL_LIGHT));
            addChild(createLight<CanyonRLight>(Vec(79.895, 100.849), module, CanyonModule::RIGHT_WALL_LIGHT));
            addChild(createLight<SunLight>(Vec(109.107, 95.484), module, CanyonModule::SUN_LIGHT));
            addChild(createLight<MoonLight>(Vec(25.597, 98.605), module, CanyonModule::MOON_LIGHT));

            //must be called 'logoPos'for all modules 
            Vec logoPos = Vec(((15.f * HP) / 2.f) - 12.5, 363.f);
            CanyonModule* module = dynamic_cast<CanyonModule*>(this->module);
            assert(module);
            #include "Theme/LogoChild.h"
            
        }

    }

};

Model* modelCanyon = createModel<CanyonModule, CanyonPanelWidget>("Canyon-Echo");