

#include "plugin.hpp"
#include <complex>
#include <cmath>
#include <thread>
#include <mutex>
#include <atomic>

#define MODULE_NAME PoppyModule
#define PANEL "fractal_panel.svg"
#define HP 20

//maximum sequence length
#define ITERS 64
const float E = 2.7182818284590;


using namespace LydD;


using Brot_Pick = std::complex<float>(*)(float, std::complex<float>, std::complex<float>);
    std::complex<float> andrewkayTan(std::complex<float> x) {
        const float pisqby4 = 2.4674011002723397f;
        const float oneminus8bypisq = 0.1894305308612978f;
        std::complex<float> xsq = x * x;
        return x * (pisqby4 - oneminus8bypisq * xsq) / (pisqby4 - xsq);
    }

    std::complex<float> Mandelbrot(float EXP, std::complex<float> C, std::complex<float> Ztemp) {
        return pow(Ztemp, EXP) + C;
    }

    std::complex<float> BurningShip(float EXP, std::complex<float> C, std::complex<float> Ztemp) {
        return pow(std::complex<float>(abs(real(Ztemp)), abs(imag(Ztemp))), EXP) + C;
    }

    std::complex<float> Beetle(float EXP, std::complex<float> C, std::complex<float> Ztemp) {
        float realZ = sinApproxNick(real(Ztemp));
        float imagZ = sinApproxNick(imag(Ztemp));
        //rack::simd::float_4 realZ(real(Ztemp));
       // rack::simd::float_4 imagZ(imag(Ztemp));
       // rack::simd::float_4 Zrnew = sin(realZ);
       // rack::simd::float_4 Zinew = sin(imagZ);
        return pow(std::complex<float>(realZ, imagZ), EXP) + C;
    }

    std::complex<float> Bird(float EXP, std::complex<float> C, std::complex<float> Ztemp) {
        rack::simd::float_4 realZ(real(Ztemp));
        rack::simd::float_4 Zrnew = atan(realZ);
        return pow(std::complex<float>(Zrnew[0], abs(imag(Ztemp))), EXP) + C;
    }

    std::complex<float> Daisy(float EXP, std::complex<float> C, std::complex<float> Ztemp) {
        return andrewkayTan(pow(C, EXP) * pow(Ztemp, E)) + (C - Ztemp);
    }

    std::complex<float> Unicorn(float EXP, std::complex<float> C, std::complex<float> Ztemp) {
        return andrewkayTan(pow(Ztemp, EXP) + Ztemp) + (C - Ztemp);
    }
    //returns the function itself to reduce # of switches run
    struct brotPicker {

        Brot_Pick chooseFractal(int fractal) {
            switch (fractal) {
            case 0: {
                return Mandelbrot;
                break;
            }
            case 1: {
                return BurningShip;
                break;
            }
            case 2: {
                return Beetle;
                break;
            }
            case 3: {
                return Bird;
                break;
            }
            case 4: {
                return Daisy;
                break;
            }
            case 5: {
                return Unicorn;
                break;
            }
            }
            return nullptr;
        }
    };
    

struct PoppyModule : Module
{
    enum ParamIds {
        Z_X_PARAM,
        Z_YI_PARAM,
        C_X_PARAM,
        C_YI_PARAM,
        EXP_X_PARAM,
        ITERS_PARAM,
        SEQ_START_PARAM,
        SHIFT_REGISTER_PARAM,
        RANGE_SWITCH_PARAM,
        Y_RANGE_SWITCH_PARAM,
        MOVE_X_PARAM,
        MOVE_YI_PARAM,
        ZOOM_PARAM,
        SLEW_X_PARAM,
        SLEW_Y_PARAM,
        FRACT_BUTTON_PARAM,
        JULIA_BUTTON_PARAM,
        INVERT_BUTTON_PARAM,
        AUX_BUTTON_PARAM,
        MIRROR_BUTTON_PARAM,
        CLOCK_BUTTON_PARAM,
        RESET_BUTTON_PARAM,
        REVERSE_BUTTON_PARAM,
        QUALITY_BUTTON_PARAM,
        C_TO_MOVE_BUTTON_PARAM,
        MOVE_TO_Z_BUTTON_PARAM,
        NUM_PARAMS
	};
    enum InputIds {
        CLOCK_INPUT,
        YI_CLOCK_INPUT,
        RESET_INPUT,
        REVERSE_INPUT,
        SEQ_LENGTH_INPUT,
        SEQ_START_INPUT,
        SLEW_X_INPUT,
        SLEW_Y_INPUT,
        Z_X_INPUT,
        Z_YI_INPUT,
        C_X_INPUT,
        C_YI_INPUT,
        EXP_X_INPUT,
        
        NUM_INPUTS
	};
	enum OutputIds {
        X_CV_OUTPUT,
        Y_CV_OUTPUT,
        XSHIFT_CV_OUTPUT,
        YSHIFT_CV_OUTPUT,
        X_TRIG_OUTPUT,
        Y_TRIG_OUTPUT,
        AUX_OUTPUT,
        NUM_OUTPUTS
	};
    enum LightIds {
        INVERT_LIGHT,
        MIRROR_LIGHT,
        JULIA_LIGHT,
        REVERSE_LIGHT,
        ENUMS(QUALITY_LIGHT, 3),
        ENUMS(FRACTAL_TYPE_LIGHT, 3),
        ENUMS(AUX_TYPE_LIGHT, 3),
        NUM_LIGHTS
    };

    //panel variable holder
#include "Theme/PanelVars.h"

    brotPicker Brot;

    int loopCounter = 0;
    //init chosen fractal
    Brot_Pick brotType = Brot.chooseFractal(0);
    //vectors being filled with sequence
    //worker thread has to use Coords, C, Z , EXP
    float xCoord[ITERS + 1];
    float yCoord[ITERS + 1];

    //output ranges and bounding box for zoom
    float range = 2.f;
    float _range = 0.f;
    float rangeY = 2.f;
    float _rangeY = 0.f;
    float XplaceMin = -2;
    float XplaceMax = 2;
    float YplaceMin = -2;
    float YplaceMax = 2;
    float MOVEX = 0.f;
    float MOVEY = 0.f;
    float ZOOM = 0.f;
    std::atomic<bool> _ANYCHANGE{ false };
    std::atomic<bool> _DRAWCHANGE{ false };

    //bools for connected ins  
    bool inZxconnect = false;
    bool inZyconnect = false;
    bool inCxconnect = false;
    bool inCyconnect = false;
    bool inEXPconnect = false;
    bool inYClockconnect = false;
    bool inReverseconnect = false;
    bool inStartconnect = false;
    bool inSizeconnect = false;
    bool inResetconnect = false;
    bool inslewX = false;
    bool inslewY = false;

    //bools and ints for buttontypes    
    bool auxbutton = 0;
    int auxType = 0;
    bool fract = 0;
    int fractal = 0;
    bool qual = 0;
    int quality = 1;
    bool julia = false;
    bool jul = false;
    bool reverse = false;
    bool rev = false;
    bool mirror = false;
    bool mir = false;
    bool invert = false;
    bool inv = false;
    bool reset = false;
    bool res = false;
    bool clockTapSet = false;
    bool clockTapReset = false;
    bool CtoMove = false;
    bool CM = false;
    bool MovetoZ = false;
    bool MZ = false;
    bool isPolarC = false;
    //sequence generation members
    float Cx = 0.f;
    float Cyi = 0.f;
    float Zx = 0.f;
    float Zyi = 0.f;
    float EXP = 2.f;
    float Mutate = 0.f;

    //sequence output members
    int Xstep = 0;
    int Ystep = 0;
    int shiftOffset = 0;
    int XshiftStep = 0;
    int YshiftStep = 0;
    int seqstart = 0;
    int seqsize = ITERS;
    int Xseqstep = 0;
    int Yseqstep = 0;
    float zoomSpread = 1.f;
    float nowX = 0.f;
    float nowY = 0.f;
    float shiftnowX = 0.f;
    float shiftnowY = 0.f;
    float fractOffset = 0.f;

    //estimated time calculations
    float dtX = 0.f;
    float dtY = 0.f;
    float speedX = 0.f;
    float speedY = 0.f;

    //rack shit
    rack::dsp::Timer timerX;
    rack::dsp::Timer timerY;
    rack::dsp::SchmittTrigger TriggerX;
    rack::dsp::SchmittTrigger TriggerY;
    rack::dsp::BooleanTrigger _EOCtrig;
    rack::dsp::PulseGenerator _EOCpulse;
    rack::dsp::SlewLimiter _slewlimitY{};
    rack::dsp::SlewLimiter _slewlimitX{};
    rack::dsp::TRCFilter<float> deClick;

    //std::thread SEQ;
    std::mutex fracTex;
    std::atomic<bool> Quitting{ false };

    PoppyModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(Z_X_PARAM, -1.8f, 1.8f, 0.f, "Z-X");
        configParam(Z_YI_PARAM, -1.8f, 1.8f, 0.f, "Z-Yi");
        configParam(C_X_PARAM, -2.f, 2.f, 0.f, "C-X");
        configParam(C_YI_PARAM, -2.f, 2.f, 0.f, "C-Yi");
        configParam(EXP_X_PARAM, 1.75f, 5.f, 2.f, "Exponent-X");
        configParam(ITERS_PARAM, 1.f, ITERS, 32.f, "Iterations");
        paramQuantities[ITERS_PARAM]->snapEnabled = true;
        configParam(SEQ_START_PARAM, 0.f, ITERS, 0.f, "Start Point");
        paramQuantities[SEQ_START_PARAM]->snapEnabled = true;
        configParam(SHIFT_REGISTER_PARAM, 1.f, 4.f, 1.f, "Shift");
        paramQuantities[SHIFT_REGISTER_PARAM]->snapEnabled = true;
        configParam(SLEW_X_PARAM, 0.f, 1.f, 0.f, "Slew X");
        configParam(SLEW_Y_PARAM, 0.f, 1.f, 0.f, "Slew Y");
        configParam(MOVE_X_PARAM, -1.8, 1.8, 0, "Move-X");
        configParam(MOVE_YI_PARAM, -1.8f, 1.8f, 0.f, "Move-Yi");
        configParam(ZOOM_PARAM, 1.f, 15.f, 1.f, "Zoom");
       
        configSwitch(FRACT_BUTTON_PARAM, 0.f, 1.f, 0.f, "Fractal");
        configSwitch(JULIA_BUTTON_PARAM, 0.f, 1.f, 0.f, "Julia");
        configSwitch(RANGE_SWITCH_PARAM, 0.f, 2.f, 0.f, "Range", { "0/2", "-2/2", "-5/5" });
        configSwitch(Y_RANGE_SWITCH_PARAM, 0.f, 2.f, 0.f, "Range", { "0/2", "-2/2", "-5/5" });
        configSwitch(AUX_BUTTON_PARAM, 0.f, 1.f, 0.f, "Aux");
        configSwitch(MIRROR_BUTTON_PARAM, 0.f, 1.f, 0.f, "Mirror Y-axis");
        configSwitch(INVERT_BUTTON_PARAM, 0.f, 1.f, 0.f, "Invert Notes");
        configSwitch(CLOCK_BUTTON_PARAM, 0.f, 1.f, 0.f, "Step");
        configSwitch(RESET_BUTTON_PARAM, 0.f, 1.f, 0.f, "Reset");
        configSwitch(REVERSE_BUTTON_PARAM, 0.f, 1.f, 0.f, "Reverse");
        configSwitch(QUALITY_BUTTON_PARAM, 0.f, 1.f, 0.f, "Visuals");
        configSwitch(C_TO_MOVE_BUTTON_PARAM, 0.f, 1.f, 0.f, "C->Move");
        configSwitch(MOVE_TO_Z_BUTTON_PARAM, 0.f, 1.f, 0.f, "Move->Z");
        
        configInput(CLOCK_INPUT, "Clock In");
        configInput(YI_CLOCK_INPUT, "Yi-Clock In");
        configInput(RESET_INPUT, "Reset In");
        configInput(REVERSE_INPUT, "Reverse In");
        configInput(Z_X_INPUT, "Z-X In");
        configInput(Z_YI_INPUT, "Z-Yi In");
        configInput(C_X_INPUT, "C-X In");
        configInput(C_YI_INPUT, "C-Yi In");
        configInput(EXP_X_INPUT, "Exponent-X In");
        configInput(SEQ_START_INPUT, "Start Point");
        configInput(SEQ_LENGTH_INPUT, "Sequence Length");
        configInput(SLEW_X_INPUT, "Slew-X");
        configInput(SLEW_Y_INPUT, "Slew-Y");
   
        configOutput(X_CV_OUTPUT, "X CV Out");
        configOutput(XSHIFT_CV_OUTPUT, "X Shift register CV Out");
        configOutput(X_TRIG_OUTPUT, "X trig Out");
        configOutput(Y_CV_OUTPUT, "Y CV Out");
        configOutput(YSHIFT_CV_OUTPUT, "Y Shift Register CV Out");
        configOutput(Y_TRIG_OUTPUT, "Y trig Out");
        configOutput(AUX_OUTPUT, "Aux Out");


    #include "Theme/setDefaultInit.h"

       // if (!SEQ.joinable()) SEQ = std::thread(&PoppyModule::createSequence, this);
    }
    ~PoppyModule() {
        Quitting.store(true);
        _ANYCHANGE.store(false);
        //if (SEQ.joinable()) SEQ.join();
    }

    
   

    void process(const ProcessArgs& args) override {

        if (loopCounter % 64 == 0) {
            //fracTex.lock();
            connections(args);
            Lights(args);
            //fracTex.unlock();
        }
        if (loopCounter % 16 == 0) {
            //fracTex.lock();
            combineParameters(args);
            //this reeeeeally should eventually be tossed to a thread
            //fracTex.unlock();
            createSequence();           
        }
        
        generateOutput(args);
        ++loopCounter;
        loopCounter %= 2520;
       
    }
    void Lights(const ProcessArgs& args) {

        lights[MIRROR_LIGHT].setBrightness(mirror);
        lights[INVERT_LIGHT].setBrightness(invert);
        lights[JULIA_LIGHT].setBrightness(julia);
        lights[REVERSE_LIGHT].setBrightness(reverse);

        switch (fractal) {
        case 0: {
            lights[FRACTAL_TYPE_LIGHT + 0].setBrightness(1.0);
            lights[FRACTAL_TYPE_LIGHT + 1].setBrightness(0.0);
            lights[FRACTAL_TYPE_LIGHT + 2].setBrightness(0.0);
            paramQuantities[FRACT_BUTTON_PARAM]->name = "Mandelbrot";
            break;
        }
        case 1: {
            lights[FRACTAL_TYPE_LIGHT + 0].setBrightness(0.0);
            lights[FRACTAL_TYPE_LIGHT + 1].setBrightness(1.0);
            lights[FRACTAL_TYPE_LIGHT + 2].setBrightness(0.0);
            paramQuantities[FRACT_BUTTON_PARAM]->name = "Burning Ship";
            break;
        }
        case 2: {
            lights[FRACTAL_TYPE_LIGHT + 0].setBrightness(0.05);
            lights[FRACTAL_TYPE_LIGHT + 1].setBrightness(0.0);
            lights[FRACTAL_TYPE_LIGHT + 2].setBrightness(0.95);
            paramQuantities[FRACT_BUTTON_PARAM]->name = "Beetlebrot";
            break;
        }
        case 3: {
            lights[FRACTAL_TYPE_LIGHT + 0].setBrightness(0.62);
            lights[FRACTAL_TYPE_LIGHT + 1].setBrightness(0.38);
            lights[FRACTAL_TYPE_LIGHT + 2].setBrightness(0.0);
            paramQuantities[FRACT_BUTTON_PARAM]->name = "Bird";
            break;
        }
        case 4: {
            lights[FRACTAL_TYPE_LIGHT + 0].setBrightness(0.0);
            lights[FRACTAL_TYPE_LIGHT + 1].setBrightness(0.36);
            lights[FRACTAL_TYPE_LIGHT + 2].setBrightness(0.64);
            paramQuantities[FRACT_BUTTON_PARAM]->name = "Daisybrot-High CPU";
            break;
        }
        case 5: {
            lights[FRACTAL_TYPE_LIGHT + 0].setBrightness(0.54);
            lights[FRACTAL_TYPE_LIGHT + 1].setBrightness(0.06);
            lights[FRACTAL_TYPE_LIGHT + 2].setBrightness(0.4);
            paramQuantities[FRACT_BUTTON_PARAM]->name = "Unicron-High CPU";
            break;
        }
        }

        switch (auxType) {
        case 0: {
            lights[AUX_TYPE_LIGHT + 0].setBrightness(0.6);
            lights[AUX_TYPE_LIGHT + 1].setBrightness(0.4);
            lights[AUX_TYPE_LIGHT + 2].setBrightness(0.0);
            paramQuantities[AUX_BUTTON_PARAM]->name = "Sum";
            break;
        }
        case 1: {
            lights[AUX_TYPE_LIGHT + 0].setBrightness(0.0);
            lights[AUX_TYPE_LIGHT + 1].setBrightness(0.5);
            lights[AUX_TYPE_LIGHT + 2].setBrightness(0.6);
            paramQuantities[AUX_BUTTON_PARAM]->name = "Magnitude";
            break;
        }
        case 2: {
            lights[AUX_TYPE_LIGHT + 0].setBrightness(0.6);
            lights[AUX_TYPE_LIGHT + 1].setBrightness(0.0);
            lights[AUX_TYPE_LIGHT + 2].setBrightness(0.5);
            paramQuantities[AUX_BUTTON_PARAM]->name = "Centroid";
            break;
        }
        case 3: {
            lights[AUX_TYPE_LIGHT + 0].setBrightness(0.6);
            lights[AUX_TYPE_LIGHT + 1].setBrightness(0.4);
            lights[AUX_TYPE_LIGHT + 2].setBrightness(0.5);
            paramQuantities[AUX_BUTTON_PARAM]->name = "Beginning of Cycle";
            break;
        }
        }

        switch (quality) {
        case 0: {
            paramQuantities[QUALITY_BUTTON_PARAM]->name = "Blind Lines - low GPU";
            break;
        }
        case 1: {
            paramQuantities[QUALITY_BUTTON_PARAM]->name = "MonoChrome";
            break;
        }
        case 2: {
            paramQuantities[QUALITY_BUTTON_PARAM]->name = "Flower";
            break;
        }
        case 3: {
            paramQuantities[QUALITY_BUTTON_PARAM]->name = "BuddhaBrot";
            break;
        }
        }
        lights[QUALITY_LIGHT + 0].setBrightness(quality == 3 && quality != 0);
        lights[QUALITY_LIGHT + 1].setBrightness(quality == 2 && quality != 0);
        lights[QUALITY_LIGHT + 2].setBrightness(quality == 1 && quality != 0);
    }
    void connections(const ProcessArgs& args) {
        for (int o = X_CV_OUTPUT; o != NUM_OUTPUTS; ++o) {
            outputs[o].setChannels(1);
        }

        inZxconnect = inputs[Z_X_INPUT].isConnected();
        inZyconnect = inputs[Z_YI_INPUT].isConnected();
        inCxconnect = inputs[C_X_INPUT].isConnected();
        inCyconnect = inputs[C_YI_INPUT].isConnected();
        inEXPconnect = inputs[EXP_X_INPUT].isConnected();
        inYClockconnect = inputs[YI_CLOCK_INPUT].isConnected();
        inResetconnect = inputs[RESET_INPUT].isConnected();
        inStartconnect = inputs[SEQ_START_INPUT].isConnected();
        inSizeconnect = inputs[SEQ_LENGTH_INPUT].isConnected();
        inReverseconnect = inputs[REVERSE_INPUT].isConnected();
        inslewX = inputs[SLEW_X_INPUT].isConnected();
        inslewY = inputs[SLEW_Y_INPUT].isConnected();
        

        switch ((int)params[RANGE_SWITCH_PARAM].value) {
        case 0: {
            range = 2.f;
            _range = 0.f;
            break;
        }
        case 1: {
            range = 2.f;
            _range = -2.f;
            break;
        }
        case 2: {
            range = 5.f;
            _range = -5.f;
            break;
        }
        }

        switch ((int)params[Y_RANGE_SWITCH_PARAM].value) {
        case 0: {
            rangeY = 2.f;
            _rangeY = 0.f;
            break;
        }
        case 1: {
            rangeY = 2.f;
            _rangeY = -2.f;
            break;
        }
        case 2: {
            rangeY = 5.f;
            _rangeY = -5.f;
            break;
        }
        }
        deClick.setCutoffFreq(2000.f / args.sampleRate);

        //this sets the actual fractal equation used
        brotType = Brot.chooseFractal(fractal);
    }
    void combineParameters(const ProcessArgs& args) {

        //need to only update sequence when parameters change, but OOOh so many of them
        float movexpr = MOVEX;
        float moveypr = MOVEY;
        float zoompr = ZOOM;
        bool julpr = julia;
        bool mirpr = mirror;
        int fracpr = fractal;
        int qualpr = quality;

        latchButton(params[MIRROR_BUTTON_PARAM].value, &mirror, &mir);
        latchButton(params[INVERT_BUTTON_PARAM].value, &invert, &inv);
        momentButton(params[CLOCK_BUTTON_PARAM].value, &clockTapSet, &clockTapReset);
        momentButton(params[C_TO_MOVE_BUTTON_PARAM].value, &CtoMove, &CM);
        momentButton(params[MOVE_TO_Z_BUTTON_PARAM].value, &MovetoZ, &MZ);

        incrementButton(params[FRACT_BUTTON_PARAM].value, &fract, 6, &fractal);
        incrementButton(params[AUX_BUTTON_PARAM].value, &auxbutton, 4, &auxType);
        incrementButton(params[QUALITY_BUTTON_PARAM].value, &qual, 4, &quality);
        latchButton(params[JULIA_BUTTON_PARAM].value, &julia, &jul);

        if (inResetconnect) {
            momentButton(inputs[RESET_INPUT].getVoltage(0), &reset, &res);
        }
        else {
            momentButton(params[RESET_BUTTON_PARAM].value, &reset, &res);
        }

        if (inReverseconnect) {
            latchButton(inputs[REVERSE_INPUT].getVoltage(0), &reverse, &rev);
        }
        else {
            latchButton(params[REVERSE_BUTTON_PARAM].value, &reverse, &rev);
        }

        seqsize = params[ITERS_PARAM].value;
        if (inSizeconnect) {
            seqsize = lerp(1, ITERS, 0, 5, rack::math::clamp(int(inputs[SEQ_LENGTH_INPUT].getVoltage(0)), 0, 5));
            seqsize = seqsize > 1 ? seqsize : 1;
        }
        seqstart = params[SEQ_START_PARAM].value;
        if (inStartconnect) {
            seqstart = lerp(0, ITERS, 0, 5, rack::math::clamp(int(inputs[SEQ_START_INPUT].getVoltage(0)), 0, 5));
            seqstart = seqstart > 0 ? seqstart : 0;
        }
        shiftOffset = params[SHIFT_REGISTER_PARAM].value;
        if (reset) {
            Xseqstep = 0;
            Yseqstep = 0;
        }
        

        float EXPparam = params[EXP_X_PARAM].value;
        float EXPin = (inEXPconnect) ? abs(inputs[EXP_X_INPUT].getVoltage(0)) : 0.f;
        float Expcombo = EXPparam + EXPin;

        float Zxpar = params[Z_X_PARAM].value;
        float Zypar = params[Z_YI_PARAM].value;
        float Zxin = (inZxconnect) ? inputs[Z_X_INPUT].getVoltage(0) : 0.f;
        float Zyin = (inZyconnect) ? inputs[Z_YI_INPUT].getVoltage(0) : 0.f;
        float Zxcombo = rack::math::clamp(lerp(-1.f, 1.f, -5.f, 5.f, Zxin) + Zxpar, -1.5f, 1.5f);
        float Zycombo = rack::math::clamp(lerp(-1.f, 1.f, -5.f, 5.f, Zyin) + Zypar, -1.5f, 1.5f);

        float Cxpar = params[C_X_PARAM].value;
        float Cypar = params[C_YI_PARAM].value;
        float Cxin = (inCxconnect) ? inputs[C_X_INPUT].getVoltage(0) / 5.f : 0.f;
        float Cyin = (inCyconnect) ? -(inputs[C_YI_INPUT].getVoltage(0)) / 5.f : 0.f;
        //default etch-a-sketch style cardinal
        float CX = Cxin + Cxpar;
        float CY = Cyin + Cypar;
        //turn soil into polar knobs instead cuz circles are fun
        if (isPolarC) {
            float TY = CY * _PI;
            TY = wrapFree(TY, -_PI, _PI);
            PolartoCart(CX, TY, &CX, &CY);
        }
        float Cxcombo = lerp(XplaceMin, XplaceMax, -1.8f, 1.8f, CX);
        Cxcombo = rack::math::clamp(Cxcombo, XplaceMin, XplaceMax);
        float Cycombo = lerp(YplaceMin, YplaceMax, -1.8f, 1.8f, CY);
        Cycombo = rack::math::clamp(Cycombo, YplaceMin, YplaceMax);

        float CenterX = params[MOVE_X_PARAM].value;
        float CenterY = params[MOVE_YI_PARAM].value;
        //fun little adjustment check to move to actual fractal space location
        if (CtoMove && ((Cycombo != CenterY) || (Cxcombo != CenterX))) {
            params[MOVE_X_PARAM].setValue(Cxcombo);
            params[MOVE_YI_PARAM].setValue(Cycombo);
            //zero soil knobs to retain location
            params[C_X_PARAM].setValue(0.f);
            params[C_YI_PARAM].setValue(0.f);

        }
        //this ones way easier
        if (MovetoZ) {
            params[Z_X_PARAM].setValue(CenterX);
            params[Z_YI_PARAM].setValue(CenterY);
        }

        //update places after potential MOVE adjustment
        ZOOM = params[ZOOM_PARAM].value;
        MOVEX = params[MOVE_X_PARAM].value;
        MOVEY = params[MOVE_YI_PARAM].value;
        zoomSpread = ZOOM;
        float zoomin = -2 / (ZOOM * ZOOM);
        float zoomax = 2 / (ZOOM * ZOOM);
        XplaceMin = zoomin;
        XplaceMin += MOVEX;
        XplaceMax = zoomax;
        XplaceMax += MOVEX;
        YplaceMin = zoomin;
        YplaceMin += MOVEY;
        YplaceMax = zoomax;
        YplaceMax += MOVEY;

        //check for any change in parameters that could result in a different sequence
        //this also is necessary to tell when to draw the picture again
        //group the writing of shared parameters so i can lock it once here
        //how in tf do i make something like this 'lockless'
        {
            //std::lock_guard<std::mutex> lock(fracTex);

            float cxpr = Cx;
            float cypr = Cyi;
            float zxpr = Zx;
            float zypr = Zyi;
            float expr = EXP;

            Cx = Cxcombo;
            Cyi = Cycombo;
            Zx = Zxcombo;
            Zyi = Zycombo;
            EXP = Expcombo;

            bool drawneeded = movexpr != MOVEX;
            drawneeded |= moveypr != MOVEY;
            drawneeded |= zoompr != ZOOM;;
            drawneeded |= zxpr != Zxcombo;
            drawneeded |= zypr != Zycombo;
            drawneeded |= expr != Expcombo;
            drawneeded |= julpr != julia;
            drawneeded |= fracpr != fractal;

            bool allchange = drawneeded;
            allchange |= cxpr != Cxcombo;
            allchange |= cypr != Cycombo;
            allchange |= mirpr != mirror;
            //tell the worker that something changed
            if (allchange) _ANYCHANGE.store(true);

            drawneeded |= qualpr != quality;
            //tell the widget it needs to calculate
            if (drawneeded) _DRAWCHANGE.store(true);
        }


        //making slew limiter RiseFall proportional to estimated time between clock pulses(speedX, speedY). 
        float smoothnessX = -(params[SLEW_X_PARAM].value) + 1;
        if (inslewX) {
            float smooXput = lerp(0.f, 1.f, 0.f, 5.f, rack::math::clamp(inputs[SLEW_X_INPUT].getVoltage(0), 0.f, 5.f));
            smoothnessX = ((smooXput > 0) ? smooXput : 0);
        }
        _slewlimitX.setRiseFall(lerp(speedX, 20000.f, 0.f, 1.f, pow(smoothnessX, 4.f)), lerp(speedX, 20000.f, 0.f, 1.f, pow(smoothnessX, 4.f)));
        float smoothnessY = -(params[SLEW_Y_PARAM].value) + 1;
        if (inslewY) {
            float smooYput = lerp(0.f, 1.f, 0.f, 5.f, rack::math::clamp(inputs[SLEW_Y_INPUT].getVoltage(0), 0.f, 5.f));
            smoothnessY = ((smooYput > 0) ? smooYput : 0);
        }
        _slewlimitY.setRiseFall(lerp(speedY, 20000.f, 0.f, 1.f, pow(smoothnessY, 4.f)), lerp(speedY, 20000.f, 0.f, 1.f, pow(smoothnessY, 4.f)));

    }

    void createSequence() {
       // while (true) {
            //if theres been no change or the module is being deleted, just leave so no lock gets lost
            if (Quitting) return;

            //if it gets here it will need access to the main parameters
            std::complex<float>C(0,0);
            std::complex<float>Z(0,0);
            float expon = 2;
            float xco[ITERS + 1];
            float yco[ITERS + 1];
            //still make sure math only runs when theres been a change
            if (_ANYCHANGE) {
                //lock direct access to shared data
                //ACTUALLY maybe worker cant ever lock main out
                {
                    //std::lock_guard<std::mutex> lock(fracTex);
                    if (mirror) {
                        Cyi = -Cyi;
                        Zyi = -Zyi;
                    }
                    C = std::complex<float>(Cx, Cyi);
                    Z = std::complex<float>(Zx, Zyi);
                    expon = EXP;
                    if (julia) {
                        std::complex<float>Zswap = Z;
                        Z = C;
                        C = Zswap;
                    }
                }
                //now this can run without stopping anything
                for (int i = 0; i <= ITERS; ++i) {
                    std::complex<float>pastVal = Z;
                    Z = brotType(expon, C, pastVal);
                    if (abs(Z) <= 2.f) {
                        xco[i] = (real(Z));
                        yco[i] = (imag(Z));
                    }
                    else if (abs(Z) > 2.f || (rack::math::isNear(real(Z), real(pastVal), 0.0416) && rack::math::isNear(imag(Z), imag(pastVal), 0.0416))) {
                        //if it doesnt get to do any iterations, just copy in the C value
                        if (i == 0) {
                            for (int p = 0; p <= ITERS; ++p) {
                                xco[p] = real(C);
                                yco[p] = imag(C);
                            }
                        }
                        else {
                            //repeat instead of calculating, way lighter
                            int q = 0;
                            for (int p = i; p <= ITERS; ++p) {
                                int rptind = q % i;
                                xco[p] = xco[rptind];
                                yco[p] = yco[rptind];
                                ++q;
                            }
                        }
                        break;

                    }
                }
                //lock again to transfer
                {
                    //std::lock_guard<std::mutex> lock(fracTex);
                    for (int i = 0; i <= ITERS; ++i) {
                        xCoord[i] = xco[i];
                        yCoord[i] = yco[i];
                    }
                }

                //wait for another change to occur
                _ANYCHANGE.store(false);
            }
            //fracTex.unlock();
            
        //}
    }


    void generateOutput(const ProcessArgs& args) {
     
        //wrap any given sequence around the max size(ITERS)
        int bX = (Xseqstep + seqstart);
        Xstep = bX % ITERS;
        XshiftStep = wraparound(bX - shiftOffset, ITERS);// (Xseqstep + seqstart - shiftOffset) % ITERS;
        //XshiftStep = (XshiftStep < 0) ? 0 : XshiftStep;
        int bY = (Yseqstep + seqstart);
        Ystep = bY % ITERS;
        YshiftStep = wraparound(bY - shiftOffset, ITERS); // (Yseqstep + seqstart - shiftOffset) % ITERS;
        //YshiftStep = (YshiftStep < 0) ? 0 : YshiftStep;


        float cvValX = 0;
        float cvValY = 0;
        float shiftValX = 0;
        float shiftValY = 0;
        //lock worker out to pull coordinates
        {
            //std::lock_guard<std::mutex> lock(fracTex);
            //lets get wierd with it
            if (!julia) {
                Mutate = rack::dsp::sqrtBipolar((Zx * Zx) + (Zyi * Zyi));
            }
            else {
                Mutate = rack::dsp::sqrtBipolar((Cx * Cx) + (Cyi * Cyi));
            }

            cvValX = xCoord[Xstep];
            cvValY = yCoord[Ystep];
            shiftValX = xCoord[XshiftStep];
            shiftValY = yCoord[YshiftStep];
        }
        if (invert) {
            cvValX = -cvValX;
            cvValY = -cvValY;
            shiftValX = -shiftValX;
            shiftValY = -shiftValY;
        }
        //Zooming in will spread outputs around middle of range
        float zoomFactor = 1.f + log10(zoomSpread);
        cvValX *= zoomFactor;
        cvValY *= zoomFactor;
        shiftValX *= zoomFactor;
        shiftValY *= zoomFactor;

        float timestepX = timerX.process(args.sampleTime);
        float timestepY = timerY.process(args.sampleTime);

        float baseClock = inputs[CLOCK_INPUT].getVoltage(0) + clockTapSet;
        float yClock = baseClock;
           
        if (inYClockconnect) {
            yClock = inputs[YI_CLOCK_INPUT].getVoltage(0);
        }

        bool isX = TriggerX.process(baseClock, 0.8f, 1.f);
        if (isX) {
            if (reverse) {
                Xseqstep -= 1;
            }
            else {
                Xseqstep += 1;
            }
            if (Xseqstep > seqsize) {
                Xseqstep = 0;
            }
            if (Xseqstep < 0) {
                Xseqstep = seqsize;
            }
            nowX = lerp(_range - Mutate, range + Mutate, -2.f, 2.f, cvValX) + fractOffset;
            shiftnowX = lerp(_range - Mutate, range + Mutate, -2.f, 2.f, shiftValX) + fractOffset;
            dtX = timestepX;
            timerX.reset();
        }
        outputs[X_TRIG_OUTPUT].setVoltage(TriggerX.isHigh() * 5.0f, 0);

       
        speedX = 1.f / dtX;

        bool isY = TriggerY.process(yClock, 0.8f, 1.f);
        if (isY) {
                    
                    
            if (reverse) {
                Yseqstep -= 1;
            }
            else {
                Yseqstep += 1;
            }
            if (Yseqstep > seqsize) {
                Yseqstep = 0;
            }
            if (Yseqstep < 0) {
                Yseqstep = seqsize;
            }
            nowY = lerp(_rangeY - Mutate, rangeY + Mutate, -2.f, 2.f, cvValY) + fractOffset;
            shiftnowY = lerp(_rangeY - Mutate, rangeY + Mutate, -2.f, 2.f, shiftValY) + fractOffset;
            dtY = timestepY;
            timerY.reset();
        }
        outputs[Y_TRIG_OUTPUT].setVoltage(TriggerY.isHigh() * 5.0f, 0);

        speedY = 1.f / dtY;

            
        //gotta put in 2 more copies of the slew limiter for the shift register outputs. or Not.
        float slewingX = _slewlimitX.process(args.sampleTime, nowX);
        outputs[X_CV_OUTPUT].setVoltage(slewingX, 0);

        outputs[XSHIFT_CV_OUTPUT].setVoltage(shiftnowX, 0);

        float slewingY = _slewlimitY.process(args.sampleTime, nowY);
        outputs[Y_CV_OUTPUT].setVoltage(slewingY, 0);

        outputs[YSHIFT_CV_OUTPUT].setVoltage(shiftnowY, 0);

        //is it the beginning? is it the end?           are they the same thing?
        bool BOC = (Xstep == seqstart);
        bool BOCtrig = _EOCtrig.process(BOC);
        if (BOCtrig) _EOCpulse.trigger(0.8f);
        bool BOCpulse = _EOCpulse.process(args.sampleTime);

        switch (auxType) {
            
        case 0: {
            float sumOut = (slewingX + slewingY);
            outputs[AUX_OUTPUT].setVoltage(sumOut, 0);
            break;
        }
        case 1: {
            float magOut =  loosesqrt(slewingX * slewingX + slewingY * slewingY);
            outputs[AUX_OUTPUT].setVoltage(magOut, 0);
            break;
        }
        case 2: {
            float averageform  = 0.f;
            for (int a = seqstart; a < (seqstart + seqsize); ++a) {
                averageform += xCoord[a % ITERS] + yCoord[a % ITERS];
            }
            averageform /= seqsize;
            deClick.process(averageform);
            averageform = deClick.lowpass();
            float avelerp = lerp(_range, range, -2.f, 2.f, averageform);
            outputs[AUX_OUTPUT].setVoltage(avelerp, 0);
            break;
        }
        case 3: {                
            outputs[AUX_OUTPUT].setVoltage(BOCpulse * 10.f, 0);
            break;
        }
            
        }
        
    }

    json_t* dataToJson() override {
        json_t* rootJ = json_object();

        json_t* VisualJ = json_real(quality);
        json_t* AuxJ = json_real(auxType);
        json_t* FractalJ = json_real(fractal);
        json_t* JuliaJ = json_boolean(julia);
        json_t* panelJ = json_integer(currPanel);
        json_t* polarJ = json_boolean(isPolarC);


        json_object_set_new(rootJ, "Visuals", VisualJ);
        json_object_set_new(rootJ, "AuxType", AuxJ);
        json_object_set_new(rootJ, "Fractal", FractalJ);
        json_object_set_new(rootJ, "Julia", JuliaJ);
        json_object_set_new(rootJ, "Panel", panelJ);
        json_object_set_new(rootJ, "Polar", polarJ);

        return rootJ;
    }

    void dataFromJson(json_t* rootJ) override {
        json_t* FractalJ = json_object_get(rootJ, "Fractal");
        json_t* JuliaJ = json_object_get(rootJ, "Julia");
        json_t* AuxJ = json_object_get(rootJ, "AuxType");
        json_t* VisualJ = json_object_get(rootJ, "Visuals");
        json_t* panelJ = json_object_get(rootJ, "Panel");
        json_t* polarJ = json_object_get(rootJ, "Polar");
        if (FractalJ) {
            fractal = json_real_value(FractalJ);
        }
        if (JuliaJ) {
            julia = json_boolean_value(JuliaJ);
        }
        if (AuxJ) {
            auxType = json_real_value(AuxJ);
        }
        if (VisualJ) {
            quality = json_real_value(VisualJ);
        }
        if (panelJ) currPanel = json_integer_value(panelJ);
        if (polarJ) isPolarC = json_boolean_value(polarJ);
    }

};

//still wanna eventually try to write this as something like a shader if I ca nwwrap my head around it
class picture {
public:
    int* kTerm;
    size_t buf_size;
    unsigned char* Kvals;
    picture(){
        kTerm = new int[160 * 120];
        buf_size = 160 * 120 * 4 * sizeof(unsigned char);
        Kvals = new unsigned char[buf_size];
        memset(Kvals, 1, buf_size);
    }
    void Empty() {
        
        for (size_t i = 0; i < buf_size; ++i) {
            this->Kvals[i] = 1;
        }
    }

    void MakeColor(int index, float red, float green, float blue, float alpha) {
        this->Kvals[index * 4 + 0] = red;
        this->Kvals[index * 4 + 1] = green;
        this->Kvals[index * 4 + 2] = blue;
        this->Kvals[index * 4 + 3] = alpha;
    }


    ~picture() {
        delete[] this->Kvals;
        delete[] this->kTerm;
    }
};

struct FracWidgetBuffer : FramebufferWidget {
    PoppyModule* Fracking;
    FracWidgetBuffer(PoppyModule* m) {
        Fracking = m;
        
    }
    void step() override {
        //trying to only refresh the screen every so often. DOESNT WORK
        /*if (Fracking->dirty) {
            FramebufferWidget::setDirty(true);
            Fracking->dirty = false;
        }
        else {
            FramebufferWidget::setDirty(false);
        }*/
        FramebufferWidget::step();
    }
    
};
//someday im gonna do openGL i swear
//struct GLFracWidget : OpenGlWidget {
//    brotPicker Brot;
//    PoppyModule* Fracking;
//    bool isWindowOpen;
//    GLFracWidget(PoppyModule* module, Vec topLeft) {
//        Fracking = module;
//        box.pos = topLeft;
//        isWindowOpen = true;
//    }
//
//    void drawFrac(void) {
//
//    }
//
//    void drawFramebuffer() override
//    {
//        glViewport(0.0, 0.0, getFramebufferSize().x, getFramebufferSize().y);
//        glClearColor(0.0, 0.0, 0.0, 1.0);
//        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
//
//        glMatrixMode(GL_PROJECTION);
//        glLoadIdentity();
//        glOrtho(0, 600, 0, 600, -1, 1);
//        displayParticles();
//        glEnd();
//    }
//};


struct FracWidget : Widget {
    brotPicker Brot;
    PoppyModule* Fracking;
    picture* pic;
    int frames = 0;
    int pictureColor = -1;
    bool isWindowOpen = false;
    int drawboxX = 0;
    int drawboxY = 0;

    FracWidget(PoppyModule* module, Vec topLeft) {
        Fracking = module;
        box.pos = topLeft;
        pic = new(picture);
        drawboxX = box.size.x;
        drawboxY = box.size.y;
        isWindowOpen = true;
    }
    ~FracWidget() {
        if (pic) delete pic;
    }

    //needed when dealing with image handles and a closeable window in a DAW
    void onContextCreate(const ContextCreateEvent& e) override {
        isWindowOpen = true;
        //onContextCreate(e);
    }
    void onContextDestroy(const ContextDestroyEvent& e) override
    {
        if (pictureColor != -1) {
            nvgDeleteImage(e.vg, pictureColor);
            pictureColor = -1;
        }
        //if (pic) delete pic;
        isWindowOpen = false;
       // onContextDestroy(e);
    }

    void drawLayer(const DrawArgs& args, int layer) override {
        Brot_Pick chosenBrot;
        drawboxX = box.size.x;
        drawboxY = box.size.y;
        //make sure its also being asked to draw anything
        if (layer == 1 && Fracking && isWindowOpen) {
            
            nvgSave(args.vg);
            nvgScissor(args.vg, 0, 0, drawboxX, drawboxY);
            nvgFillColor(args.vg, nvgRGBAf(0.0, 0.0, 0.0, 1.0));
            nvgBeginPath(args.vg);
            nvgRect(args.vg, 0.0, 0.0, drawboxX, drawboxY);
            nvgFill(args.vg);
            
            float xdrawMin = Fracking->XplaceMin;
            float xdrawMax = Fracking->XplaceMax;
            float ydrawMin = Fracking->YplaceMin;
            float ydrawMax = Fracking->YplaceMax;
            float ZparX = Fracking->Zx;
            float ZparY = Fracking->Zyi;
            int fractype = Fracking->fractal;
            chosenBrot = Brot.chooseFractal(fractype);

            if (pictureColor == -1) {
                pictureColor = nvgCreateImageRGBA(args.vg, drawboxX, drawboxY, 0, pic->Kvals);
            }
            else if (pictureColor != -1) {
                nvgUpdateImage(args.vg, pictureColor, pic->Kvals);
            }

            if (Fracking->quality == 0) {
                pic->Empty();
            }
            //if we're drawing the fractal, wait for a change to happen AND only every 8 frames to update
            if(Fracking->quality != 0 && (frames % 8 == 0 && Fracking->_DRAWCHANGE)){
                /*filling screenbuffer with RGBA values*/
                pic->Empty();
                int centerX = drawboxX * 0.5;
                int centerY = drawboxY * 0.5;
                int iteras = 25;
                float _red = 0;
                float _gre = 0;
                float _blu = 0;
                float _alp = 10;
                for (int j = 0; j < drawboxY; j += 1) {
                    for (int l = 0; l < drawboxX; l += 1) {

                        int index = j * drawboxX + l;
                        float distFromCenter = (abs(centerX - l) + abs(centerY - j)) / (float)(centerX + centerY);
                        distFromCenter = (-distFromCenter) + 1.f; //normalize then invert for central glow
                        std::complex<float>Z(ZparX, ZparY);
                        float ilerp = lerp(xdrawMin, xdrawMax, 0.f, (float)drawboxX, (float)l);
                        float jlerp = lerp(ydrawMin, ydrawMax, 0.f, (float)drawboxY, (float)j);
                        std::complex<float>C(ilerp, jlerp);

                        float expo = Fracking->EXP;

                        if (Fracking->julia) {
                            std::complex<float>Zswap = Z;
                            Z = C;
                            C = Zswap;
                        }

                        int k;
                        std::complex<float>Ztemp(0, 0);

                        for (k = 0; k < iteras; ++k) {
                            Ztemp = Z;
                            Z = chosenBrot(expo, C, Ztemp);
                            if (abs(Z) > 2.f) break;

                            //just don draw so many dots ok?
                            if (Fracking->quality == 3 && (k > 8 && j % 2 == 0 /*&& l % 4 == 0*/)) {                       
                                int Xloc = lerp(0.f, (float)drawboxX, xdrawMin, xdrawMax, real(Z));
                                int Yloc = lerp(0.f, (float)drawboxY, ydrawMin, ydrawMax, imag(Z));
                                if (Xloc > 10 && Xloc < drawboxX - 10 && Yloc > 10 && Yloc < drawboxY - 10) {
                                    int Zpixindex = Yloc * drawboxX + Xloc;
                                    float redadd = _red;
                                    float greadd = _gre;
                                    float bluadd = _blu;

                                    Components::HSLtoRGB(30 * (k % 10), /*abs(Z) **/ 0.5f, 0.5f, &_red, &_gre, &_blu);
                                    redadd = _red;
                                    greadd = _gre;
                                    bluadd = _blu;
                                    _alp = (_alp > 150) ? 150 : _alp + 5;
                                    pic->MakeColor(Zpixindex, redadd, greadd, bluadd, _alp);
                                }
                            }
                        }

                        pic->kTerm[index] = k;

                        

                        if (Fracking->quality == 1) {
                            Components::HSLtoRGB(300, 0.1, (float)(k / iteras) * distFromCenter, &_red, &_gre, &_blu);
                            pic->MakeColor(index, _red, _gre, _blu, 200.f);
                            
                        }
                        else if (Fracking->quality == 2) {
                            float colorval = ((k <= iteras - 1) ? 280 - pow(2.f, k * 0.75) : abs(Z - Ztemp) * 30.f);
                            Components::HSLtoRGB(colorval, 0.8 * distFromCenter, 0.5, &_red, &_gre, &_blu);
                            pic->MakeColor(index, _red, _gre, _blu, 200.f);
                            
                        }
             
                    }
                }
                frames = 0;
                Fracking->_DRAWCHANGE.store(false);
            }
            ++frames;
            
            nvgBeginPath(args.vg);
            NVGpaint picPaint = nvgImagePattern(args.vg, 0, 0, drawboxX, drawboxY, 0.0f, pictureColor, 1.0f);
            nvgRect(args.vg, 0, 0, drawboxX, drawboxY);
            nvgFillPaint(args.vg, picPaint);
            nvgFill(args.vg);
            //pic->Empty();


            /*the lines of the sequence itself, and tracking square*/
            nvgFillColor(args.vg, nvgRGBAf(0.4, 0.86, 1.0, 0.46));
            nvgBeginPath(args.vg);
            float Crectx = lerp(0.f, (float)drawboxX, xdrawMin, xdrawMax, Fracking->Cx);
            float Crecty = lerp(0.f, (float)drawboxY, ydrawMin, ydrawMax, Fracking->Cyi);

            nvgRect(args.vg, Crectx - 2, Crecty - 2, 4, 4);
            nvgFill(args.vg);
            for (int d = Fracking->seqstart; d < (Fracking->seqsize + Fracking->seqstart); ++d) {
                int draw = d % ITERS;
                float rectx = lerp(0.f, (float)drawboxX, xdrawMin, xdrawMax, Fracking->xCoord[draw]);
                float recty = lerp(0.f, (float)drawboxY, ydrawMin, ydrawMax, Fracking->yCoord[draw]);
                float Nrectx = lerp(0.f, (float)drawboxX, xdrawMin, xdrawMax, Fracking->xCoord[abs(draw - 1)]);
                float Nrecty = lerp(0.f, (float)drawboxY, ydrawMin, ydrawMax, Fracking->yCoord[abs(draw - 1)]);
                
                nvgBeginPath(args.vg);
                nvgMoveTo(args.vg, Nrectx, Nrecty);
                nvgLineTo(args.vg, rectx, recty);
                nvgStrokeWidth(args.vg, 0.7f);
                nvgStrokeColor(args.vg, nvgRGBAf(0.7, 0.4, 0.3, 0.37));
                nvgStroke(args.vg);

            }
            nvgFillColor(args.vg, nvgRGBAf(0.36, 1.0, 0.65, 0.18));
            nvgBeginPath(args.vg);
            float Seqrectx = lerp(0.f, (float)drawboxX, xdrawMin, xdrawMax, Fracking->xCoord[(Fracking->Xstep - 1) > 0 ? Fracking->Xstep - 1 : 0]);
            float Seqrecty = lerp(0.f, (float)drawboxY, ydrawMin, ydrawMax, Fracking->yCoord[(Fracking->Ystep - 1) > 0 ? Fracking->Ystep - 1 : 0]);
          
            nvgRect(args.vg, Seqrectx - 3, Seqrecty - 3, 6, 6);
            nvgFill(args.vg);
        }

        nvgRestore(args.vg);
        Widget::drawLayer(args, layer);
    }
};

using namespace LydD::Components;
struct PoppyWidget : ModuleWidget {
    //include struct for logo here so it has modules name
    #include "Theme/LogoLight.h"
    //name for panel file, same name for every type
    std::string panel;

    PoppyWidget(PoppyModule* module) {

        setModule(module);

        panel = PANEL;
        //set panel on init
        #include "Theme/initChoosePanel.h"

		addChild(createWidget<ScrewSilver>(Vec(15, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 30, 0)));
		addChild(createWidget<ScrewSilver>(Vec(15, 365)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 30, 365)));

        addChild(createLightCentered<MediumLight<RedGreenBlueLight>>(Vec(135, 185), module, PoppyModule::FRACTAL_TYPE_LIGHT));
        addChild(createLightCentered<MediumLight<BlueLight>>(Vec(167, 185), module, PoppyModule::JULIA_LIGHT));
        addChild(createLightCentered<MediumLight<RedGreenBlueLight>>(Vec(190, 305), module, PoppyModule::AUX_TYPE_LIGHT));
        addChild(createLightCentered<MediumLight<BlueLight>>(Vec(190, 241), module, PoppyModule::INVERT_LIGHT));
        addChild(createLightCentered<MediumLight<BlueLight>>(Vec(108, 331), module, PoppyModule::MIRROR_LIGHT));
        addChild(createLightCentered<MediumLight<BlueLight>>(Vec(63, 160), module, PoppyModule::REVERSE_LIGHT));
        addChild(createLightCentered<MediumLight<RedGreenBlueLight>>(Vec(237,108), module, PoppyModule::QUALITY_LIGHT));


        addInput(createInput<PurplePort>(Vec(10, 30), module, PoppyModule::CLOCK_INPUT));
        addInput(createInput<PurplePort>(Vec(10, 65), module, PoppyModule::YI_CLOCK_INPUT));
        addInput(createInput<PurplePort>(Vec(10, 100), module, PoppyModule::RESET_INPUT));
        addInput(createInput<PurplePort>(Vec(10, 135), module, PoppyModule::REVERSE_INPUT));

        addInput(createInput<PurplePort>(Vec(10, 291), module, PoppyModule::Z_X_INPUT));
        addInput(createInput<PurplePort>(Vec(10, 331), module, PoppyModule::Z_YI_INPUT));
        addInput(createInput<PurplePort>(Vec(45, 291), module, PoppyModule::C_X_INPUT));
        addInput(createInput<PurplePort>(Vec(45, 331), module, PoppyModule::C_YI_INPUT));
        addInput(createInput<PurplePort>(Vec(80, 291), module, PoppyModule::EXP_X_INPUT));
        addInput(createInput<PurplePort>(Vec(267,191), module, PoppyModule::SEQ_LENGTH_INPUT));
        addInput(createInput<PurplePort>(Vec(267, 156), module, PoppyModule::SEQ_START_INPUT));
        addInput(createInput<PurplePort>(Vec(120, 331), module, PoppyModule::SLEW_X_INPUT));
        addInput(createInput<PurplePort>(Vec(155, 331), module, PoppyModule::SLEW_Y_INPUT));
        

        addParam(createParam<RoundHugeBlackKnob>(Vec(18, 205), module, PoppyModule::Z_X_PARAM));
        addParam(createParam<RoundBlackKnob>(Vec(30.6, 217.6), module, PoppyModule::Z_YI_PARAM));
        addParam(createParam<RoundBlackKnob>(Vec(76, 173), module, PoppyModule::EXP_X_PARAM));
        addParam(createParam<RoundSmallBlackKnob>(Vec(231, 191), module, PoppyModule::ITERS_PARAM));
        addParam(createParam<RoundSmallBlackKnob>(Vec(212, 156), module, PoppyModule::SEQ_START_PARAM));
        addParam(createParam<Trimpot>(Vec(199, 268), module, PoppyModule::SHIFT_REGISTER_PARAM));
        addParam(createParam<RoundHugeBlackKnob>(Vec(123.081, 205), module, PoppyModule::C_X_PARAM));
        addParam(createParam<RoundBlackKnob>(Vec(135.826, 217.8), module, PoppyModule::C_YI_PARAM));
        addParam(createParam<RoundHugeBlackKnob>(Vec(237, 26.5), module, PoppyModule::MOVE_X_PARAM));
        addParam(createParam<RoundBlackKnob>(Vec(249.8, 39.3), module, PoppyModule::MOVE_YI_PARAM));
        addParam(createParam<RoundLargeBlackKnob>(Vec(254, 106), module, PoppyModule::ZOOM_PARAM));
        addParam(createParam<RoundSmallBlackKnob>(Vec(120, 291), module, PoppyModule::SLEW_X_PARAM));
        addParam(createParam<RoundSmallBlackKnob>(Vec(155, 291), module, PoppyModule::SLEW_Y_PARAM));

        addParam(createParam<VCVButton>(Vec(41, 48), module, PoppyModule::CLOCK_BUTTON_PARAM));
        addParam(createParam<VCVButton>(Vec(41, 101), module, PoppyModule::RESET_BUTTON_PARAM));
        addParam(createParam<VCVButton>(Vec(41, 136), module, PoppyModule::REVERSE_BUTTON_PARAM));
        addParam(createParam<VCVButton>(Vec(125, 162), module, PoppyModule::FRACT_BUTTON_PARAM));
        addParam(createParam<VCVButton>(Vec(157, 162), module, PoppyModule::JULIA_BUTTON_PARAM));
        addParam(createParam<VCVButton>(Vec(199, 295), module, PoppyModule::AUX_BUTTON_PARAM));
        addParam(createParam<VCVButton>(Vec(82, 332), module, PoppyModule::MIRROR_BUTTON_PARAM));
        addParam(createParam<VCVButton>(Vec(198, 232), module, PoppyModule::INVERT_BUTTON_PARAM));        
        addParam(createParam<VCVButton>(Vec(235, 83), module, PoppyModule::QUALITY_BUTTON_PARAM));
        addParam(createParam<VCVButton>(Vec(100, 250), module, PoppyModule::C_TO_MOVE_BUTTON_PARAM));
        addParam(createParam<VCVButton>(Vec(69, 250), module, PoppyModule::MOVE_TO_Z_BUTTON_PARAM));
        addParam(createParam<PurpleSwitch>(Vec(231.78, 229), module, PoppyModule::RANGE_SWITCH_PARAM));
        addParam(createParam<PurpleSwitch>(Vec(262.5, 229), module, PoppyModule::Y_RANGE_SWITCH_PARAM));

        addOutput(createOutput<PurplePort>(Vec(232, 269), module, PoppyModule::XSHIFT_CV_OUTPUT));
        addOutput(createOutput<PurplePort>(Vec(267, 269), module, PoppyModule::YSHIFT_CV_OUTPUT));
        addOutput(createOutput<PurplePort>(Vec(232, 300), module, PoppyModule::X_CV_OUTPUT));
        addOutput(createOutput<PurplePort>(Vec(267, 300), module, PoppyModule::Y_CV_OUTPUT));
        addOutput(createOutput<PurplePort>(Vec(232, 331), module, PoppyModule::X_TRIG_OUTPUT));
        addOutput(createOutput<PurplePort>(Vec(267, 331), module, PoppyModule::Y_TRIG_OUTPUT));
        addOutput(createOutput<PurplePort>(Vec(197, 331), module, PoppyModule::AUX_OUTPUT));
        
        if (module) {
            FracWidgetBuffer* FracBuffer = new FracWidgetBuffer(module);
            addChild(FracBuffer);
            FracWidget* myWidget = new FracWidget(module, Vec(70, 30));
            myWidget->setSize(Vec(160, 120));
            
            FracBuffer->addChild(myWidget);         

            //must be called 'logoPos'for all modules 
            Vec logoPos = Vec(((15.f * HP) / 2.f) - 12.5, 363.f);
            PoppyModule* module = dynamic_cast<PoppyModule*>(this->module);
            assert(module);
            #include "Theme/LogoChild.h"
        }        
    }    

    //give struct to menu containing panel options
    #include "Theme/PanelList.h" 

    void appendContextMenu(Menu* menu) override {
        PoppyModule* module = dynamic_cast<PoppyModule*>(this->module);
        assert(module);
       
        menu->addChild(new MenuSeparator());

        //wow checkbox lambdas, how unique
        menu->addChild(createCheckMenuItem("Soil as Polar", "Outer[X] becomes Radius, Inner[Y] becomes Angle",
                [=]() {return module->isPolarC != false; },
                [=]() {module->isPolarC ^= true; }
            ));

        menu->addChild(new MenuSeparator());

        //create named shell for list of panel options
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

Model* modelPoppy = createModel<PoppyModule, PoppyWidget>("Poppy-fields");