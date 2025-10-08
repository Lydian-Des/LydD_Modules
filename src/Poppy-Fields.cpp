

#include "plugin.hpp"
#include <complex>
#include <cmath>
#include <vector>


const std::complex<float>i(0.0, 1.0);
const float E = 2.7182818284590;





struct Brots {
    std::complex<float> andrewkayTan(std::complex<float> x) {
        const float pisqby4 = 2.4674011002723397f;
        const float oneminus8bypisq = 0.1894305308612978f;
        std::complex<float> xsq = x * x;
        return x * (pisqby4 - oneminus8bypisq * xsq) / (pisqby4 - xsq);
    }

    std::complex<float> mandelbrot(float EXP, std::complex<float> C, std::complex<float> Ztemp) {
        return pow(Ztemp, EXP) + C;
    }

    std::complex<float> burningShip(float EXP, std::complex<float> C, std::complex<float> Ztemp) {
        return pow(std::complex<float>(abs(real(Ztemp)), abs(imag(Ztemp))), EXP) + C;
    }

    std::complex<float> beetle(float EXP, std::complex<float> C, std::complex<float> Ztemp) {
        rack::simd::float_4 realZ(real(Ztemp));
        rack::simd::float_4 imagZ(imag(Ztemp));
        rack::simd::float_4 Zrnew = sin(realZ);
        rack::simd::float_4 Zinew = sin(imagZ);
        return pow(std::complex<float>(Zrnew[0], Zinew[0]), EXP) + C;
    }

    std::complex<float> bird(float EXP, std::complex<float> C, std::complex<float> Ztemp) {
        rack::simd::float_4 realZ(real(Ztemp));
        rack::simd::float_4 Zrnew = atan(realZ);
        return pow(std::complex<float>(Zrnew[0], abs(imag(Ztemp))), EXP) + C;
    }

    std::complex<float> daisy(float EXP, std::complex<float> C, std::complex<float> Ztemp, std::complex<float> Z) {
        return andrewkayTan(pow(C, EXP) * pow(Ztemp, E)) + (C - Ztemp);
    }

    std::complex<float> unicorn(float EXP, std::complex<float> C, std::complex<float> Ztemp) {
        return andrewkayTan(pow(Ztemp, EXP) + Ztemp) + (C - Ztemp);
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

    //maximum sequence length
    const int iters = 64;

    PoppyModule() {
        // Your module must call config from its constructor, passing in
        // how many ins, outs, etc... it has.
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(Z_X_PARAM, -1.8, 1.8, 0, "Z-X");
        configParam(Z_YI_PARAM, -1.8, 1.8, 0, "Z-Yi");
        configParam(C_X_PARAM, -1.8, 1.8, 0, "C-X");
        configParam(C_YI_PARAM, -1.8, 1.8, 0, "C-Yi");
        configParam(EXP_X_PARAM, 1.75, 5, 2, "Exponent-X");
        configParam(ITERS_PARAM, 1, iters, 32, "Iterations");
        paramQuantities[ITERS_PARAM]->snapEnabled = true;
        configParam(SEQ_START_PARAM, 0, iters, 0, "Start Point");
        paramQuantities[SEQ_START_PARAM]->snapEnabled = true;
        configParam(SHIFT_REGISTER_PARAM, 1, 4, 1, "Shift");
        paramQuantities[SHIFT_REGISTER_PARAM]->snapEnabled = true;
        configParam(SLEW_X_PARAM, 0, 1, 0, "Slew X");
        configParam(SLEW_Y_PARAM, 0, 1, 0, "Slew Y");
        configParam(MOVE_X_PARAM, -1.8, 1.8, 0, "Move-X");
        configParam(MOVE_YI_PARAM, -1.8, 1.8, 0, "Move-Yi");
        configParam(ZOOM_PARAM, 1, 50, 1, "Zoom");
       
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
       
        
        configOutput(X_CV_OUTPUT, "X CV Out");
        configOutput(XSHIFT_CV_OUTPUT, "X Shift register CV Out");
        configOutput(X_TRIG_OUTPUT, "X trig Out");
        configOutput(Y_CV_OUTPUT, "Y CV Out");
        configOutput(YSHIFT_CV_OUTPUT, "Y Shift Register CV Out");
        configOutput(Y_TRIG_OUTPUT, "Y trig Out");
        configOutput(AUX_OUTPUT, "Aux Out");
    }

    BaseFunctions Funct;
    BaseButtons button;
    Brots Brot;

    int loopCounter = 0;

    //vectors being filled with sequence
    std::vector<float> xCoord;
    std::vector<float> yCoord;

    //output ranges and bounding box for zoom
    int range = 2;
    int _range = 0;
    int rangeY = 2;
    int _rangeY = 0;   
    float XplaceMin = -2;
    float XplaceMax = 2;
    float YplaceMin = -2;
    float YplaceMax = 2;

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
    bool auxbutton = false;
    int auxType = 0;
    bool fract = false;
    int fractal = 0;
    bool qual = false;
    int quality = 0;
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
    bool dirty = false;
    bool dirtyonce = false;
    bool CtoMove = false;
    bool CM = false;
    bool MovetoZ = false;
    bool MZ = false;
    
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
    int seqsize = iters;
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
    rack::dsp::SchmittTrigger EOC;
    rack::dsp::SlewLimiter _slewlimitY{};
    rack::dsp::SlewLimiter _slewlimitX{};
    rack::dsp::TRCFilter<float> deClick;

    //used in Aux type == 1 : magnitude
    float loosesqrt(float x) {
        float g = x / 2.f;
        for (int n = 0; n < 4; ++n) {
            float gtemp = g;
            g = (gtemp + (x / gtemp)) / 2.f;
        }
        return g;
    }

    void process(const ProcessArgs& args) override {

        if (loopCounter % 64 == 0) {
            connections(args);
            Lights(args);
        }
        if (loopCounter % 16 == 0) {            
            createSequence(args);           
        }
        
        generateOutput(args);
        ++loopCounter;

        if (loopCounter % (int)(args.sampleRate / 2) == 0) {
            dirty = true;
            loopCounter = 0;
        }
       
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
            paramQuantities[QUALITY_BUTTON_PARAM]->name = "Pond";
            break;
        }
        case 1: {
            paramQuantities[QUALITY_BUTTON_PARAM]->name = "Flower";
            break;
        }
        case 2: {
            paramQuantities[QUALITY_BUTTON_PARAM]->name = "BuddhaBrot";
            break;
        }
        }
        lights[QUALITY_LIGHT + 0].setBrightness(quality == 2);
        lights[QUALITY_LIGHT + 1].setBrightness(quality == 1);
        lights[QUALITY_LIGHT + 2].setBrightness(quality == 0);
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
            range = 2;
            _range = 0;
            break;
        }
        case 1: {
            range = 2;
            _range = -2;
            break;
        }
        case 2: {
            range = 5;
            _range = -5;
            break;
        }
        }

        switch ((int)params[Y_RANGE_SWITCH_PARAM].value) {
        case 0: {
            rangeY = 2;
            _rangeY = 0;
            break;
        }
        case 1: {
            rangeY = 2;
            _rangeY = -2;
            break;
        }
        case 2: {
            rangeY = 5;
            _rangeY = -5;
            break;
        }
        }
        deClick.setCutoffFreq(2000.f / args.sampleRate);

    }


    void createSequence(const ProcessArgs& args) {
    
        button.incrementButton(params[FRACT_BUTTON_PARAM].value, &fract, 6, &fractal);
        button.incrementButton(params[AUX_BUTTON_PARAM].value, &auxbutton, 4, &auxType);
        button.incrementButton(params[QUALITY_BUTTON_PARAM].value, &qual, 3, &quality);
        button.latchButton(params[JULIA_BUTTON_PARAM].value, &julia, &jul);
        button.latchButton(params[MIRROR_BUTTON_PARAM].value, &mirror, &mir);
        button.latchButton(params[INVERT_BUTTON_PARAM].value, &invert, &inv);
        button.latchButton(params[REVERSE_BUTTON_PARAM].value, &reverse, &rev);
        button.momentButton(params[CLOCK_BUTTON_PARAM].value, &clockTapSet, &clockTapReset);
        button.momentButton(params[RESET_BUTTON_PARAM].value, &reset, &res);
        button.momentButton(params[C_TO_MOVE_BUTTON_PARAM].value, &CtoMove, &CM);
        button.momentButton(params[MOVE_TO_Z_BUTTON_PARAM].value, &MovetoZ, &MZ);

        shiftOffset = params[SHIFT_REGISTER_PARAM].value;

        seqsize = params[ITERS_PARAM].value;
        if (inSizeconnect) {
            seqsize = (int)Funct.lerp(1, iters, 0, 5, rack::math::clamp(inputs[SEQ_LENGTH_INPUT].getVoltage(0), 0.f, 5.f));
            seqsize = seqsize > 1 ? seqsize : 1;
        }
        seqstart = params[SEQ_START_PARAM].value;
        if (inStartconnect) {
            seqstart = (int)Funct.lerp(0, iters, 0, 5, rack::math::clamp(inputs[SEQ_START_INPUT].getVoltage(0), 0.f, 5.f));
            seqstart = seqstart > 0 ? seqstart : 0;
        }
        if (inReverseconnect) {
            button.latchButton(inputs[REVERSE_INPUT].getVoltage(0), &reverse, &rev);
        }
        if (inResetconnect) {
            button.momentButton(inputs[RESET_INPUT].getVoltage(0), &reset, &res);
        }
        if (reset) {
            Xseqstep = 0;
            Yseqstep = 0;
        }
        xCoord.clear();
        yCoord.clear();
        
        float Cxpar = params[C_X_PARAM].value;
        float Cypar = params[C_YI_PARAM].value;
        float Cxloc = Funct.lerp(XplaceMin, XplaceMax, -1.8f, 1.8f, Cxpar);
        float Cyloc = Funct.lerp(YplaceMin, YplaceMax, -1.8f, 1.8f, Cypar);
        float CenterX = params[MOVE_X_PARAM].value;
        float CenterY = params[MOVE_YI_PARAM].value;
        //fun little adjustment check to move to actual fractal space location
        if (CtoMove && ((Cyloc != CenterY) || (Cxloc != CenterX))) {
            params[MOVE_X_PARAM].setValue(Cxloc);
            params[MOVE_YI_PARAM].setValue(Cyloc);
            params[C_X_PARAM].setValue(0.0);
            params[C_YI_PARAM].setValue(0.0);

        }
        //this ones way easier
        if (MovetoZ) {
            params[Z_X_PARAM].setValue(CenterX);
            params[Z_YI_PARAM].setValue(CenterY);
        }
        
        float ZOOM = params[ZOOM_PARAM].value;
        float MOVEX = params[MOVE_X_PARAM].value;
        float MOVEY = params[MOVE_YI_PARAM].value;

        XplaceMin = -2 / (ZOOM * ZOOM);
        XplaceMin += MOVEX;
        XplaceMax = 2 / (ZOOM * ZOOM);
        XplaceMax += MOVEX;
        YplaceMin = -2 / (ZOOM * ZOOM);
        YplaceMin += MOVEY;
        YplaceMax = 2 / (ZOOM * ZOOM);
        YplaceMax += MOVEY;

        float EXPparam = params[EXP_X_PARAM].value;
        float EXPin = (inEXPconnect) ? abs(inputs[EXP_X_INPUT].getVoltage(0)) : 0.0;
        EXP = EXPparam + EXPin;

        float Cxin = (inCxconnect) ? inputs[C_X_INPUT].getVoltage(0) : 0.0;
        float Cyin = (inCyconnect) ? -(inputs[C_YI_INPUT].getVoltage(0)) : 0.0;
        float adderx = (inCxconnect) ? 6.f : 1.8f; // must interpolate differently for variance in size of input vs param
        float addery = (inCyconnect) ? 6.f : 1.8f;
        float Cxcombo = Funct.lerp(XplaceMin, XplaceMax, -adderx, adderx, Cxin + Cxpar);
        float Cycombo = Funct.lerp(YplaceMin, YplaceMax, -addery, addery, Cyin + Cypar);
        Cx = rack::math::clamp(Cxcombo, XplaceMin, XplaceMax);
        Cyi = rack::math::clamp(Cycombo, YplaceMin, YplaceMax);
        
        float Zxpar = params[Z_X_PARAM].value;
        float Zypar = params[Z_YI_PARAM].value;
        float Zxin = (inZxconnect) ? inputs[Z_X_INPUT].getVoltage(0) : 0;
        float Zyin = (inZyconnect) ? inputs[Z_YI_INPUT].getVoltage(0) : 0;
        Zx = rack::math::clamp(Funct.lerp(-1, 1, -5, 5, Zxin) + Zxpar, -1.5f, 1.5f);
        Zyi = rack::math::clamp(Funct.lerp(-1, 1, -5, 5, Zyin) + Zypar, -1.5f, 1.5f);
              
        if (mirror) {
            Cyi = -Cyi;
            Zyi = -Zyi;
        }

        std::complex<float>C(Cx, Cyi);
        std::complex<float>Z(Zx, Zyi);

        if (julia) {
            std::complex<float>Zswap = Z;
            Z = C;
            C = Zswap;
        }

        for (int i = 0; i <= iters; ++i) {
            std::complex<float>pastVal = Z;

            switch (fractal) {
            case 0: {
                Z = Brot.mandelbrot(EXP, C, pastVal);
                fractOffset = 0.f;
                break;
            }
            case 1: {
                Z = Brot.burningShip(EXP, C, pastVal);
                fractOffset = 0.1666667f;
                break;
            }
            case 2: {
                Z = Brot.beetle(EXP, C, pastVal);
                fractOffset = -0.1666667f;
                break;
            }
            case 3: {
                Z = Brot.bird(EXP, C, pastVal);
                fractOffset = 0.0833333f;
                break;
            }
            case 4: {
                Z = Brot.daisy(EXP, C, pastVal, Z);
                fractOffset = 0.3333333f;
                break;
            }
            case 5: {
                Z = Brot.unicorn(EXP, C, pastVal);
                fractOffset = -0.3333333f;
                break;
            }
            }
            if (abs(Z) > 2 || (rack::math::isNear(real(Z), real(pastVal), 0.0416) && rack::math::isNear(imag(Z), imag(pastVal), 0.0416))) {
                Z = std::complex<float>(Cx, Cyi);
            }
            xCoord.emplace_back(real(Z));
            yCoord.emplace_back(imag(Z));
        }

        zoomSpread = params[ZOOM_PARAM].value;

        //making slew limiter RiseFall proportional to estimated time between clock pulses(speedX, speedY). 
        float smoothnessX = -(params[SLEW_X_PARAM].value) + 1;
        if (inslewX) {
            float smooXput = Funct.lerp(0, 1, 0, 5, rack::math::clamp(inputs[SLEW_X_INPUT].getVoltage(0), 0.f, 5.f));
            smoothnessX = ((smooXput > 0) ? smooXput : 0);
        }
        _slewlimitX.setRiseFall(Funct.lerp(speedX, 20000, 0, 1, pow(smoothnessX, 4)), Funct.lerp(speedX, 20000, 0, 1, pow(smoothnessX, 4)));
        float smoothnessY = -(params[SLEW_Y_PARAM].value) + 1;
        if (inslewY) {
            float smooYput = Funct.lerp(0, 1, 0, 5, rack::math::clamp(inputs[SLEW_Y_INPUT].getVoltage(0), 0.f, 5.f));
            smoothnessY = ((smooYput > 0) ? smooYput : 0);
        }
        _slewlimitY.setRiseFall(Funct.lerp(speedY, 20000, 0, 1, pow(smoothnessY, 4)), Funct.lerp(speedY, 20000, 0, 1, pow(smoothnessY, 4)));

    }


    void generateOutput(const ProcessArgs& args) {
     
        //wrap any given sequence around the max size(iters)
        Xstep = (Xseqstep + seqstart) % iters;
        XshiftStep = (Xseqstep + seqstart - shiftOffset) % iters;
        XshiftStep = (XshiftStep < 0) ? 0 : XshiftStep;
        Ystep = (Yseqstep + seqstart) % iters;
        YshiftStep = (Yseqstep + seqstart - shiftOffset) % iters;
        YshiftStep = (YshiftStep < 0) ? 0 : YshiftStep;

        //lets get wierd with it
        if (!julia) {
            Mutate = rack::dsp::sqrtBipolar((Zx * Zx) + (Zyi * Zyi)) ;
        }
        else {
            Mutate = rack::dsp::sqrtBipolar((Cx * Cx) + (Cyi * Cyi));
        }

        float cvValX = xCoord[Xstep];
        float cvValY = yCoord[Ystep];
        float shiftValX = xCoord[XshiftStep];
        float shiftValY = yCoord[YshiftStep];

        if (invert) {
            cvValX = -cvValX;
            cvValY = -cvValY;
            shiftValX = -shiftValX;
            shiftValY = -shiftValY;
        }
        //Zooming in will spread outputs around 0
        cvValX *= log10(zoomSpread) + 1; //Funct.lerp(1, 2, 1, 50, zoomSpread);
        cvValY *= log10(zoomSpread) + 1; //Funct.lerp(1, 2, 1, 50, zoomSpread);
        shiftValX *= log10(zoomSpread) + 1; //Funct.lerp(1, 2, 1, 50, zoomSpread);
        shiftValY *= log10(zoomSpread) + 1; //Funct.lerp(1, 2, 1, 50, zoomSpread);

        //trying to change it up a bit(a 5th to be real) if it moves straight horizontal or vertical
        bool xClose = rack::math::isNear(xCoord[abs(Xstep - 1)], xCoord[Xstep], 0.0416f);
        if (xClose) {
            cvValX += 0.583333;
        }
        bool yClose = rack::math::isNear(yCoord[abs(Ystep - 1)], yCoord[Ystep], 0.0416f);
        if (yClose) {
            cvValY += 0.583333;

        }

        //is it the beginning? is it the end?           are they the same thing?
        bool BOC = Xstep == seqstart;
        bool BOCtrig = EOC.process(BOC, 0.8f, 1.0f);

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
            nowX = Funct.lerp(_range - Mutate, range + Mutate, -2.f, 2.f, cvValX) + fractOffset;
            shiftnowX = Funct.lerp(_range - Mutate, range + Mutate, -2.f, 2.f, shiftValX) + fractOffset;
            dtX = timestepX;
            timerX.reset();
        }
        if (TriggerX.isHigh()) {
            outputs[X_TRIG_OUTPUT].setVoltage(5.0f, 0);
        }
        else {
            outputs[X_TRIG_OUTPUT].setVoltage(0.0f, 0);
        }
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
            nowY = Funct.lerp(_rangeY - Mutate, rangeY + Mutate, -2.f, 2.f, cvValY) + fractOffset;
            shiftnowY = Funct.lerp(_rangeY - Mutate, rangeY + Mutate, -2.f, 2.f, shiftValY) + fractOffset;
            dtY = timestepY;
            timerY.reset();
        }
        if (TriggerY.isHigh()) {
            outputs[Y_TRIG_OUTPUT].setVoltage(5.0f, 0);
        }
        else {
            outputs[Y_TRIG_OUTPUT].setVoltage(0.0f, 0);
        }
        speedY = 1.f / dtY;

            
        //gotta put in 2 more copies of the slew limiter for the shift register outputs. or Not.
        float slewingX = _slewlimitX.process(args.sampleTime, nowX);
        outputs[X_CV_OUTPUT].setVoltage(slewingX, 0);

        outputs[XSHIFT_CV_OUTPUT].setVoltage(shiftnowX, 0);

        float slewingY = _slewlimitY.process(args.sampleTime, nowY);
        outputs[Y_CV_OUTPUT].setVoltage(slewingY, 0);

        outputs[YSHIFT_CV_OUTPUT].setVoltage(shiftnowY, 0);

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
                averageform += xCoord[a % iters] + yCoord[a % iters];
            }
            averageform /= seqsize;
            deClick.process(averageform);
            averageform = deClick.lowpass();
            float avelerp = Funct.lerp(_range, range, -2.f, 2.f, averageform);
            outputs[AUX_OUTPUT].setVoltage(avelerp, 0);
            break;
        }
        case 3: {                
            outputs[AUX_OUTPUT].setVoltage(BOCtrig, 0);
            break;
        }
            
        }
        
    }

    json_t* dataToJson() override {
        json_t* rootJ = json_object();

        json_t* VisualJ = json_integer(quality);
        json_t* AuxJ = json_integer(auxType);
        json_t* FractalJ = json_integer(fractal);
        json_t* JuliaJ = json_boolean(julia);

        json_object_set_new(rootJ, "Visuals", VisualJ);
        json_object_set_new(rootJ, "AuxType", AuxJ);
        json_object_set_new(rootJ, "Fractal", FractalJ);
        json_object_set_new(rootJ, "Julia", JuliaJ);

        return rootJ;
    }

    void dataFromJson(json_t* rootJ) override {
        json_t* FractalJ = json_object_get(rootJ, "Fractal");
        json_t* JuliaJ = json_object_get(rootJ, "Julia");
        json_t* AuxJ = json_object_get(rootJ, "AuxType");
        json_t* VisualJ = json_object_get(rootJ, "Visuals");
        if (FractalJ) {
            fractal = json_integer_value(FractalJ);
        }
        if (JuliaJ) {
            julia = json_boolean_value(JuliaJ);
        }
        if (AuxJ) {
            auxType = json_integer_value(AuxJ);
        }
        if (VisualJ) {
            quality = json_integer_value(VisualJ);
        }
    }

};

//this drawing shit is too goddamn big and needs some serious work. 
//likely have to rewrite it entirely with an opengl widget.
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
        
        if (Fracking->dirty) {
            FramebufferWidget::setDirty(true);
            Fracking->dirty = false;
        }
        else {
            FramebufferWidget::setDirty(false);
        }
        FramebufferWidget::step();
    }
    
};

struct FracWidget : Widget {
    BaseFunctions Funct;
    Brots Brot;
    PoppyModule* Fracking;
    picture* pic;
    FracWidget(PoppyModule* module, Vec topLeft) {
        Fracking = module;
        box.pos = topLeft;
        pic = new(picture);
    }
   
    int pictureColor = -1;
    int frames = 0;

    void drawLayer(const DrawArgs& args, int layer) override {
        if (layer == 1) {
            int drawboxX = box.size.x;
            int drawboxY = box.size.y;
            nvgScissor(args.vg, 0, 0, drawboxX, drawboxY);
            nvgFillColor(args.vg, nvgRGBAf(0.0, 0.0, 0.0, 1.0));
            nvgBeginPath(args.vg);
            nvgRect(args.vg, 0.0, 0.0, drawboxX, drawboxY);
            nvgFill(args.vg);

            float xdrawMin = Fracking->XplaceMin;
            float xdrawMax = Fracking->XplaceMax;
            float ydrawMin = Fracking->YplaceMin;
            float ydrawMax = Fracking->YplaceMax;
            int pixel = 2;
            int iters = 25;
            if (pictureColor == -1) {
                pictureColor = nvgCreateImageRGBA(args.vg, drawboxX, drawboxY, 0, pic->Kvals);
            }
            /*filling screenbuffer with RGBA values*/ //turn it off if mode 2 for processor issues
            //if (Fracking->quality != 2) {
                for (int j = pixel; j < drawboxY - pixel; j += pixel) {
                    for (int l = pixel; l < drawboxX - pixel; l += pixel) {

                        int index = j * drawboxX + l;

                        std::complex<float>Z(Fracking->Zx, Fracking->Zyi);
                        float ilerp = Funct.lerp(xdrawMin, xdrawMax, 0, drawboxX, l);
                        float jlerp = Funct.lerp(ydrawMin, ydrawMax, 0, drawboxY, j);
                        std::complex<float>C(ilerp, jlerp);

                        float expo = Fracking->EXP;

                        if (Fracking->julia) {
                            std::complex<float>Zswap = Z;
                            Z = C;
                            C = Zswap;
                        }
                        
                        //essentially turn off drawing for performance
                        if (Fracking->quality == 2) {
                            pic->Kvals[index * 4 + 0] = 0.f;
                            pic->Kvals[index * 4 + 1] = 0.f;
                            pic->Kvals[index * 4 + 2] = 0.f;
                            pic->Kvals[index * 4 + 3] = 0.f;
                        }

                        int k;
                        std::complex<float>Ztemp(0, 0);

                        for (k = 0; k < iters; ++k) {
                            Ztemp = Z;
                            switch (Fracking->fractal) {
                            case 0: {
                                Z = Brot.mandelbrot(expo, C, Ztemp);
                                break;
                            }
                            case 1: {
                                Z = Brot.burningShip(expo, C, Ztemp);
                                break;
                            }
                            case 2: {
                                Z = Brot.beetle(expo, C, Ztemp);
                                break;
                            }
                            case 3: {
                                Z = Brot.bird(expo, C, Ztemp);
                                break;
                            }
                            case 4: {
                                Z = Brot.daisy(expo, C, Ztemp, Z);
                                break;
                            }
                            case 5: {
                                Z = Brot.unicorn(expo, C, Ztemp);
                                break;
                            }
                            }

                            if (Fracking->quality == 2) {

                                if ( j % 4 ==0 && l % 4 == 0) {
                                    int Xloc = rack::math::clamp((int)Funct.lerp(0, drawboxX, xdrawMin, xdrawMax, real(Z)), pixel, drawboxX - pixel);
                                    int Yloc = rack::math::clamp((int)Funct.lerp(0, drawboxY, ydrawMin, ydrawMax, imag(Z)), pixel, drawboxY - pixel);
                                    int Zpixindex = Yloc * drawboxX + Xloc;
                                    float redval = abs(real(Z) - imag(Z)) * 9;
                                    float greenval = k / 10.f;
                                    float blueval = 4 * (real(Z) * real(C)) + (imag(Z) * imag(C));
                                    float alphaval = 35;
                                    for (int r = 0; r < pixel; ++r) {
                                        for (int c = 0; c < pixel; ++c) {
                                            int Zpixsur = (Zpixindex + (drawboxX * r) + c);
                                            pic->Kvals[Zpixsur * 4 + 0] = std::min(pic->Kvals[Zpixsur * 4 + 0] + redval, 245.f);
                                            pic->Kvals[Zpixsur * 4 + 1] = std::min(pic->Kvals[Zpixsur * 4 + 1] + greenval, 245.f);
                                            pic->Kvals[Zpixsur * 4 + 2] = std::min(pic->Kvals[Zpixsur * 4 + 2] + blueval, 245.f);
                                            pic->Kvals[Zpixsur * 4 + 3] = std::min(pic->Kvals[Zpixsur * 4 + 3] + alphaval, 245.f);
                                        }
                                    }
                                }
                            }

                            if (abs(Z) > 2) {
                                break;
                            }
                        }

                            if (Fracking->quality == 0) {
                                if (k >= iters - 1) {
                                    float redval = abs(real(Z) - imag(Z)) * 50;
                                    float greenval = 0;
                                    float blueval = 40 - redval / 10;
                                    float alphaval = 210;
                                    for (int r = -pixel; r < pixel; ++r) {
                                        for (int c = -pixel; c < pixel; ++c) {
                                            pic->Kvals[(index + (drawboxX * r) + c) * 4 + 0] = redval;
                                            pic->Kvals[(index + (drawboxX * r) + c) * 4 + 1] = greenval;
                                            pic->Kvals[(index + (drawboxX * r) + c) * 4 + 2] = blueval;
                                            pic->Kvals[(index + (drawboxX * r) + c) * 4 + 3] = alphaval;
                                        }
                                    }
                                }
                                else {
                                    float redval =3 * k;
                                    float greenval = 20 + real(Z) * 8;
                                    float blueval = 130 - imag(Z) * 5;
                                    float alphaval = 180;
                                    for (int r = -pixel; r < pixel; ++r) {
                                        for (int c = -pixel; c < pixel; ++c) {
                                            pic->Kvals[(index + (drawboxX * r) + c) * 4 + 0] = redval;
                                            pic->Kvals[(index + (drawboxX * r) + c) * 4 + 1] = greenval;
                                            pic->Kvals[(index + (drawboxX * r) + c) * 4 + 2] = blueval;
                                            pic->Kvals[(index + (drawboxX * r) + c) * 4 + 3] = alphaval;

                                        }
                                    }
                                }
                            }
                            if (Fracking->quality == 1) {
                                if (k >= iters - 1) {
                                    
                                    float redval = 14 * abs(real(Z) - imag(Z));
                                    float greenval = abs(real(Z) - real(Ztemp)) * 70;
                                    float blueval = 200 - abs(imag(Z) - imag(Ztemp)) * 55;
                                    float alphaval = 180;
                                   
                                    for (int r = -pixel; r < pixel; ++r) {
                                        for (int c = -pixel; c < pixel; ++c) {
                                            int newindex = index + (drawboxX * r) + c;
                                            pic->Kvals[(newindex) * 4 + 0] = redval;
                                            pic->Kvals[(newindex) * 4 + 1] = greenval;
                                            pic->Kvals[(newindex) * 4 + 2] = blueval;
                                            pic->Kvals[(newindex) * 4 + 3] = alphaval;
                                            for (int nr = 0; nr < pixel; ++nr) {
                                                for (int nc = 0; nc < pixel; ++nc) {
                                                    if (nr != 0 && nc != 0) {
                                                        pic->Kvals[(newindex) * 4 + 0] += (0.2 / (abs(nr + nc) + 2))
                                                            * pic->Kvals[(newindex + (drawboxX * nr) + nc) * 4 + 0];
                                                        pic->Kvals[(newindex) * 4 + 2] += (0.3 / (abs(nr + nc) + 2))
                                                            * pic->Kvals[(newindex + (drawboxX * nr) + nc) * 4 + 2];
                                                    }

                                                }
                                            }
                                        }
                                    }
                                }
                                else {
                                    double Cbe = (abs(C) / 2.0) - (int)(abs(C) / 2.0);
                                    double Cab = 1 - Cbe;
                                    double Zbe = (abs(Z) / 2.0) - (int)(abs(Z) / 2.0);
                                    double Zab = 1 - Zbe;
                                    float klog = log(k + 1);
                                    float redval =(k == 0 ) ? 0.f : (4 * (((Cab * Zab) * klog) / ((Cbe * Zbe) * abs(klog - 1))));
                                    float greenval = 10 * (Cab * k + Cbe * abs(k - 1));
                                    float blueval = 20 * ((Zab * k * klog) + (Zbe * abs(k + klog - 1)));
                                    float alphaval = 210;
                                    for (int r = -pixel; r < pixel; ++r) {
                                        for (int c = -pixel; c < pixel; ++c) {
                                            int newindex = index + (drawboxX * r) + c;
                                            pic->Kvals[(newindex) * 4 + 0] = redval;
                                            pic->Kvals[(newindex) * 4 + 1] = greenval;
                                            pic->Kvals[(newindex) * 4 + 2] = blueval;
                                            pic->Kvals[(newindex) * 4 + 3] = alphaval;
                                            for (int nr = 0; nr < pixel; ++nr) {
                                                for (int nc = 0; nc < pixel; ++nc) {
                                                    if (nr != 0 && nc != 0) {
                                                        pic->Kvals[(newindex) * 4 + 0] += (0.2 / (abs(nr + nc) + 1))
                                                            * pic->Kvals[(newindex + (drawboxX * nr) + nc) * 4 + 0];
                                                        pic->Kvals[(newindex) * 4 + 1] += (0.2 / (abs(nr + nc) + 1))
                                                            * pic->Kvals[(newindex + (drawboxX * nr) + nc) * 4 + 1];
                                                        pic->Kvals[(newindex) * 4 + 2] += (0.2 / (abs(nr + nc) + 1))
                                                            * pic->Kvals[(newindex + (drawboxX * nr) + nc) * 4 + 2];
                                                    }

                                                }
                                            }
                                        }
                                    }
                                }
                            }                            

                        pic->kTerm[index] = k;
                    }
                }
            //}
            //else {
            //    pic.Empty();
            //}


                if (pictureColor != -1){
                    nvgUpdateImage(args.vg, pictureColor, pic->Kvals);    
                }
                NVGpaint picPaint = nvgImagePattern(args.vg, 0, 0, drawboxX, drawboxY, 0.0f, pictureColor, 1.0f);
                nvgBeginPath(args.vg);
                nvgRect(args.vg, 0, 0, drawboxX, drawboxY);
                nvgFillPaint(args.vg, picPaint);
                nvgFill(args.vg);
                pic->Empty();
            /*the lines of the sequence itself, and tracking squares*/
            nvgFillColor(args.vg, nvgRGBAf(0.4, 0.86, 1.0, 0.46));
            nvgBeginPath(args.vg);
            float Crectx = Funct.lerp(0, drawboxX, xdrawMin, xdrawMax, Fracking->Cx);
            float Crecty = Funct.lerp(0, drawboxY, ydrawMin, ydrawMax, Fracking->Cyi);

            nvgRect(args.vg, Crectx - 2, Crecty - 2, 4, 4);
            nvgFill(args.vg);
            for (int d = Fracking->seqstart; d < (Fracking->seqsize + Fracking->seqstart); ++d) {
                int draw = d % Fracking->iters;
                float rectx = Funct.lerp(0, drawboxX, xdrawMin, xdrawMax, Fracking->xCoord[draw]);
                float recty = Funct.lerp(0, drawboxY, ydrawMin, ydrawMax, Fracking->yCoord[draw]);
                float Nrectx = Funct.lerp(0, drawboxX, xdrawMin, xdrawMax, Fracking->xCoord[abs(draw - 1)]);
                float Nrecty = Funct.lerp(0, drawboxY, ydrawMin, ydrawMax, Fracking->yCoord[abs(draw - 1)]);
                
                nvgBeginPath(args.vg);
                nvgMoveTo(args.vg, Nrectx, Nrecty);
                nvgLineTo(args.vg, rectx, recty);
                nvgStrokeWidth(args.vg, 0.7f);
                nvgStrokeColor(args.vg, nvgRGBAf(0.9, 0.6, 0.2, 0.47));
                nvgStroke(args.vg);

            }
            nvgFillColor(args.vg, nvgRGBAf(0.36, 1.0, 0.65, 0.18));
            nvgBeginPath(args.vg);
            float Seqrectx = Funct.lerp(0, drawboxX, xdrawMin, xdrawMax, Fracking->xCoord[(Fracking->Xstep - 1) > 0 ? Fracking->Xstep - 1 : 0]);
            float Seqrecty = Funct.lerp(0, drawboxY, ydrawMin, ydrawMax, Fracking->yCoord[(Fracking->Ystep - 1) > 0 ? Fracking->Ystep - 1 : 0]);
          
            nvgRect(args.vg, Seqrectx - 3, Seqrecty - 3, 6, 6);
            nvgFill(args.vg);
        }
        frames++;
        if (frames % 100 == 0) {
            //nvgDeleteImage(args.vg, pictureColor);
            frames = 0;
        }
        Widget::drawLayer(args, layer);
    }
};

struct PoppyWidget : ModuleWidget {
    PoppyWidget(PoppyModule* module) {

        setModule(module);

        setPanel(createPanel(asset::plugin(pluginInstance, "res/fractal_panel.svg"), asset::plugin(pluginInstance, "res/fractal_panel-dark.svg")));

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
        addParam(createParam<PurpleSwitch>(Vec(231.5, 229), module, PoppyModule::RANGE_SWITCH_PARAM));
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
            FracWidget* myWidget = new FracWidget(module, Vec(69.5, 29.5));
            myWidget->setSize(Vec(160, 120));
            
            FracBuffer->addChild(myWidget);                      
        }        
    }    
};

Model* modelPoppy = createModel<PoppyModule, PoppyWidget>("Poppy-fields");