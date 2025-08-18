
#include "plugin.hpp"
#include <vector>



class Follow {
private:
    std::vector<rack::simd::float_4> PointsX;
    std::vector<rack::simd::float_4> PointsY;

public:
    void buildPoints(rack::simd::float_4 X, rack::simd::float_4 Y) {
        PointsX.push_back(X);
        PointsY.push_back(Y);
        if (PointsX.size() > 20) {
            PointsX.erase(PointsX.begin());
            PointsY.erase(PointsY.begin());
        }
    }
    void PeekPoints(std::vector<rack::simd::float_4>* askX, std::vector<rack::simd::float_4>* askY) {
        *askX = PointsX;
        *askY = PointsY;
    }
};

struct PathEquate {
    rack::simd::float_4 xchange(float a, rack::simd::float_4 xprev, rack::simd::float_4 yprev, float wave) {
        rack::simd::float_4 dx;
        //for (int i = 0; i < 4; ++i) {
           dx.v = sse_mathfun_sin_ps((xprev.v * xprev.v) - (yprev.v * yprev.v) + a) * sse_mathfun_cos_ps((yprev.v * wave));
        //}
        return dx; // sin(xprev + (dx - yprev / dt));
    }
    rack::simd::float_4 ychange(float b, rack::simd::float_4 xprev, rack::simd::float_4 yprev, float wave) {
        rack::simd::float_4 dy;
        //for (int i = 0; i < 4; ++i) {
          dy.v = sse_mathfun_cos_ps(((2 * xprev.v * yprev.v) + b)) * -sse_mathfun_sin_ps((xprev.v * wave + PI / 2.f));
        //}
        return dy;// cos(yprev + (dy - xprev / dt));
    }
    
};



struct SimoneModule : Module
{
    enum ParamIds {
        SPEED_PARAM,
        FM_PARAM,
        PM_PARAM,
        POSITION1_PARAM,
        POSITION2_PARAM,
        POSITION3_PARAM,
        POSITION4_PARAM,
        WAVE_PARAM,
        T_RAD_PARAM,
        A_PARAM,
        B_PARAM,
        ITER1_PARAM,
        ITER2_PARAM,
        ITER3_PARAM,
        ITER4_PARAM,
        RANGE_BUTTON_PARAM,
        ENUMS(OFFSET_PARAMS, 3),
        NUM_PARAMS
    };
    enum InputIds {
        SPEED1_INPUT,
        SPEED2_INPUT,
        SPEED3_INPUT,
        SPEED4_INPUT,
        FM_INPUT,
        PM_INPUT,
        POSITION1_INPUT,
        POSITION2_INPUT,
        POSITION3_INPUT,
        POSITION4_INPUT,
        WAVE_INPUT,
        T_RAD_INPUT,
        A_INPUT,
        B_INPUT,
        ITER1_INPUT,
        ITER2_INPUT,
        ITER3_INPUT,
        ITER4_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        X_1_OUTPUT,
        Y_1_OUTPUT,
        X_2_OUTPUT,
        Y_2_OUTPUT,
        X_3_OUTPUT,
        Y_3_OUTPUT,
        X_4_OUTPUT,
        Y_4_OUTPUT,
        MIX_X_OUTPUT,
        MIX_Y_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightIds {
        NUM_LIGHTS
    };

    SimoneModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(SPEED_PARAM, -4.f, 4.f, 0.f, "Speed");
        configParam(FM_PARAM, -1.f, 1.f, 0.f, "FM");
        configParam(PM_PARAM, -1.f, 1.f, 0.f, "PM");
        configParam(POSITION1_PARAM, -1.5, 1.5, 0.0f, "Pos1");
        configParam(POSITION2_PARAM, -1.5, 1.5, 0.0f, "Pos2");
        configParam(POSITION3_PARAM, -1.5, 1.5, 0.0f, "Pos3");
        configParam(POSITION4_PARAM, -1.5, 1.5, 0.0f, "Pos4");
        configParam(WAVE_PARAM, -2.f, 2.f, 0.f, "Wave");
        configParam(T_RAD_PARAM, 0.f, 1.5f, 0.5f, "Gain");
        configParam(A_PARAM, -PI, PI, 0.f, "Alpha");
        configParam(B_PARAM, -PI, PI, 0.f, "Beta");
        configParam(ITER1_PARAM, 1.f, 5.f, 1.f, "Pull1");
        configParam(ITER2_PARAM, 1.f, 5.f, 1.f, "Pull2");
        configParam(ITER3_PARAM, 1.f, 5.f, 1.f, "Pull3");
        configParam(ITER4_PARAM, 1.f, 5.f, 1.f, "Pull4");
        configParam(RANGE_BUTTON_PARAM, 0.f, 1.f, 0.f, "Range");
        for (int i = 0; i < 3; ++i) {
            configParam(OFFSET_PARAMS + i, -12.f, 12.f, 0.f, "offset");
        }
    }


    BaseFunctions Functions;
    BaseButtons Buttons;
    PathEquate Paths;
    Follow* follow = new(Follow);
    rack::dsp::TRCFilter<rack::simd::float_4> dcRemoveX;
    rack::dsp::TRCFilter<rack::simd::float_4> dcRemoveY;


    int loopCounter = 0;
    bool dirty;

    float range = 2.04375;
    int rangetype = 2;
    bool setRange = false;


    int maxsize = 7;
    float apar = 0.f;
    float a = 0.f;
    float bpar = 0.f;
    float b = 0.f;
    float wavepar = 0.f;
    float wave = 0.f;
    float rad = 0.5;
    float Tradius = 0.5;
    bool isinA = false;
    bool isinB = false;
    bool isinWAVE = false;
    bool isinRAD = false;
    bool isinIT1 = false;
    bool isinIT2 = false;
    bool isinIT3 = false;
    bool isinIT4 = false;
    bool isinPOS1 = false;
    bool isinPOS2 = false;
    bool isinPOS3 = false;
    bool isinPOS4 = false;
    bool isinFM = false;
    bool isinPM = false;
    bool isinSpeed1 = false;
    bool isinSpeed2 = false;
    bool isinSpeed3 = false;
    bool isinSpeed4 = false;


    rack::simd::float_4 DT{ 0.f };
    rack::simd::float_4 timePitch{ 0.f };
    
    rack::simd::float_4 Xs{ 0.f };
    rack::simd::float_4 Ys{ 0.f };
    rack::simd::float_4 PlacePar{ 0.f };
    rack::simd::float_4 PlaceparamY{ 0.f };
    rack::simd::float_4 PlaceInX{ 0.f };
    rack::simd::float_4 PlaceInY{ 0.f };
    rack::simd::float_4 PositionX{ 0.f };
    rack::simd::float_4 PositionY{ 0.f };
    rack::simd::float_4 PlaceX{ 0.f };
    rack::simd::float_4 PlaceY{ 0.f };    
    rack::simd::float_4 Xouts{ 0.f };
    rack::simd::float_4 Youts{ 0.f };
 
    std::vector<float> CurrentPar;
    std::vector<float> CurrentIn;
    std::vector<float> Current;


    void process(const ProcessArgs& args) override {

      
        if (loopCounter % 64 == 0) {
            
            checkInputs(args);

        }
        if (loopCounter % 16 == 0) {
            follow->buildPoints(Xouts, Youts);
            buildParams(args);
            
        }
        generateOutput(args);
        
        loopCounter++;
        if (loopCounter % 2048 == 0) {
            dirty = true;
            loopCounter = 1;
        }
    }

    void checkInputs(const ProcessArgs& args) {
        isinWAVE = inputs[WAVE_INPUT].isConnected();
        isinRAD = inputs[T_RAD_INPUT].isConnected();
        isinA = inputs[A_INPUT].isConnected();
        isinB = inputs[B_INPUT].isConnected();
        isinIT1 = inputs[ITER1_INPUT].isConnected();
        isinIT2 = inputs[ITER2_INPUT].isConnected();
        isinIT3 = inputs[ITER3_INPUT].isConnected();
        isinIT4 = inputs[ITER4_INPUT].isConnected();
        isinPOS1 = inputs[POSITION1_INPUT].isConnected();
        isinPOS2 = inputs[POSITION2_INPUT].isConnected();
        isinPOS3 = inputs[POSITION3_INPUT].isConnected();
        isinPOS4 = inputs[POSITION4_INPUT].isConnected();
        isinFM = inputs[FM_INPUT].isConnected();
        isinPM = inputs[PM_INPUT].isConnected();
        isinSpeed1 = inputs[SPEED1_INPUT].isConnected();
        isinSpeed2 = inputs[SPEED2_INPUT].isConnected();
        isinSpeed3 = inputs[SPEED3_INPUT].isConnected();
        isinSpeed4 = inputs[SPEED4_INPUT].isConnected();
        dcRemoveX.setCutoffFreq(10.6f / args.sampleRate);
        dcRemoveY.setCutoffFreq(10.6f / args.sampleRate);
    }

    float exponlerp(float newmin, float newmax, float oldmin, float oldmax, float pos) {

        return newmin * pow((newmax / newmin), ((pos - oldmin) / (oldmax - oldmin)));
    }

    void buildParams(const ProcessArgs& args) {

        Buttons.incrementButton(params[RANGE_BUTTON_PARAM].value, &setRange, 3, &rangetype);

        

        switch (rangetype) {
        case 0: {
            range = 0.0319336;
            paramQuantities[RANGE_BUTTON_PARAM]->name = "Torpid";
            break;
        }
        case 1: {
            range = 4.0875;
            paramQuantities[RANGE_BUTTON_PARAM]->name = "Waves";
            break;
        }
        case 2: {
            range = 130.8; //65.4;
            paramQuantities[RANGE_BUTTON_PARAM]->name = "Voice";
            break;
        }
        }
        //need 0 -> 1, 1 -> 2, & -1 -> 0.5
        
        float pitchoffset[3];
        /*float paroffset[3]{ (params[OFFSET_PARAMS + 0].value / 12.f),
                                (params[OFFSET_PARAMS + 1].value / 12.f),
                                (params[OFFSET_PARAMS + 2].value / 12.f) };*/
        for (int i = 0; i < 3; ++i) {
            pitchoffset[i] = exponlerp(0.5f, 2.f, -1.f, 1.f, params[OFFSET_PARAMS + i].value / 12.f);
        }
        float baseFreq = range;
        float basePitch = Functions.VoltToFreq(params[SPEED_PARAM].value, 0.0, baseFreq);
        float timePitch1 = (isinSpeed1) ? Functions.VoltToFreq(params[SPEED_PARAM].value + inputs[SPEED1_INPUT].getVoltage(0), 0.0, baseFreq) : basePitch;
        float timePitch2 = ((isinSpeed2) ? Functions.VoltToFreq(params[SPEED_PARAM].value + inputs[SPEED2_INPUT].getVoltage(0), 0.0, baseFreq) : timePitch1);
        float timePitch3 = ((isinSpeed3) ? Functions.VoltToFreq(params[SPEED_PARAM].value + inputs[SPEED3_INPUT].getVoltage(0), 0.0, baseFreq) : timePitch2);
        float timePitch4 = ((isinSpeed4) ? Functions.VoltToFreq(params[SPEED_PARAM].value + inputs[SPEED4_INPUT].getVoltage(0), 0.0, baseFreq) : timePitch3);
        timePitch2 *= pitchoffset[0];
        timePitch3 *= pitchoffset[1];
        timePitch4 *= pitchoffset[2];
        timePitch = rack::simd::float_4{ timePitch1, timePitch2, timePitch3, timePitch4 };

        wavepar = params[WAVE_PARAM].value; 
        rad = params[T_RAD_PARAM].value;
        

        apar = params[A_PARAM].value;
        bpar = params[B_PARAM].value;
        
        
        float Current1 = params[ITER1_PARAM].value;  
        float Current2 = params[ITER2_PARAM].value; 
        float Current3 = params[ITER3_PARAM].value;
        float Current4 = params[ITER4_PARAM].value; 
        CurrentPar = std::vector<float>{ Current1, Current2, Current3, Current4 };

        
        
        float place1 = params[POSITION1_PARAM].value; 
        float place2 = params[POSITION2_PARAM].value; 
        float place3 = params[POSITION3_PARAM].value; 
        float place4 = params[POSITION4_PARAM].value; 
        PlacePar = rack::simd::float_4{ place1, place2, place3, place4 };


        
        
    }

    void generateOutput(const ProcessArgs& args) {

        rack::simd::float_4 FM = timePitch * (((isinFM) ? inputs[FM_INPUT].getVoltage(0) * params[FM_PARAM].value : params[FM_PARAM].value) + 1);
        float inPM = inputs[PM_INPUT].getVoltage(0) * (((isinPM) ? params[PM_PARAM].value : 1.f));
        rack::simd::float_4 PM = timePitch * inPM * args.sampleTime;

        Functions.incrementPhase(FM, args.sampleRate, &DT, _2PI);
        DT += PM;

            float ain = (isinA) ? Functions.lerp(-PI, PI, -5, 5, inputs[A_INPUT].getVoltage(0)) : 0.0;
            float bin = (isinB) ? Functions.lerp(-PI, PI, -5, 5, inputs[B_INPUT].getVoltage(0)) : 0.0;
            a = rack::math::clamp(apar + ain, -_2PI, _2PI);
            b = rack::math::clamp(bpar + bin, -_2PI, _2PI);

            float wavein = (isinWAVE) ? (Functions.lerp(-2, 2, -5, 5, inputs[WAVE_INPUT].getVoltage(0))) : 1.0;
            wave = rack::math::clamp((wavein * wavepar), -2.f, 2.f);

            float radin = (isinRAD) ? (abs(Functions.lerp(-1.5, 1.5, -5, 5, inputs[T_RAD_INPUT].getVoltage(0)))) : 1.0;
            Tradius = rack::math::clamp((radin * rad), 0.f, 1.5f);

            //float InCurrent1 = rack::math::clamp(CurrentPar[0] + abs((isinIT1) ? inputs[ITER1_INPUT].getVoltage(0) : 0.f), 1.f, 6.f);
            //float InCurrent2 = rack::math::clamp(CurrentPar[1] + abs((isinIT2) ? inputs[ITER2_INPUT].getVoltage(0) : 0.f), 1.f, 6.f);
            //float InCurrent3 = rack::math::clamp(CurrentPar[2] + abs((isinIT3) ? inputs[ITER3_INPUT].getVoltage(0) : 0.f), 1.f, 6.f);
            //float InCurrent4 = rack::math::clamp(CurrentPar[3] + abs((isinIT4) ? inputs[ITER4_INPUT].getVoltage(0) : 0.f), 1.f, 6.f);
            
            float currentCasc1 = (isinIT1) ? abs(inputs[ITER1_INPUT].getVoltage(0)) : 0.f;
            float currentCasc2 = (isinIT2) ? abs(inputs[ITER2_INPUT].getVoltage(0)) : currentCasc1;
            float currentCasc3 = (isinIT3) ? abs(inputs[ITER3_INPUT].getVoltage(0)) : currentCasc2;
            float currentCasc4 = (isinIT4) ? abs(inputs[ITER4_INPUT].getVoltage(0)) : currentCasc3;
            float Incurrent1 = rack::math::clamp(CurrentPar[0] + (currentCasc1), 1.f, 8.f);
            float Incurrent2 = rack::math::clamp(CurrentPar[1] + (currentCasc2), 1.f, 8.f);
            float Incurrent3 = rack::math::clamp(CurrentPar[2] + (currentCasc3), 1.f, 8.f);
            float Incurrent4 = rack::math::clamp(CurrentPar[3] + (currentCasc4), 1.f, 8.f);
            Current = std::vector<float>{ Incurrent1, Incurrent2, Incurrent3, Incurrent4 };
            float placeCasc1 = (isinPOS1) ? Functions.lerp(-1.5, 1.5, -5, 5, inputs[POSITION1_INPUT].getVoltage(0)) : 0.f;
            float placeCasc2 = (isinPOS2) ? Functions.lerp(-1.5, 1.5, -5, 5, inputs[POSITION2_INPUT].getVoltage(0)) : placeCasc1;
            float placeCasc3 = (isinPOS3) ? Functions.lerp(-1.5, 1.5, -5, 5, inputs[POSITION3_INPUT].getVoltage(0)) : placeCasc2;
            float placeCasc4 = (isinPOS4) ? Functions.lerp(-1.5, 1.5, -5, 5, inputs[POSITION4_INPUT].getVoltage(0)) : placeCasc3;
            float Inplace1 = rack::math::clamp(PlacePar[0] + (placeCasc1), -2.f, 2.f);
            float Inplace2 = rack::math::clamp(PlacePar[1] + (placeCasc2), -2.f, 2.f);
            float Inplace3 = rack::math::clamp(PlacePar[2] + (placeCasc3), -2.f, 2.f);
            float Inplace4 = rack::math::clamp(PlacePar[3] + (placeCasc4), -2.f, 2.f);
            PlaceInX = rack::simd::float_4{ Inplace1, Inplace2, -Inplace3 * 2, -Inplace4 * 2 };
            PlaceInY = rack::simd::float_4{ Inplace1 * 2, -Inplace2 * 2, Inplace3, -Inplace4 };
            
            PlaceX.v = PlaceInX.v + sse_mathfun_sin_ps(DT.v) * Tradius;
            PlaceY.v = PlaceInY.v + sse_mathfun_cos_ps(DT.v) * Tradius;
            Xs.v = PlaceX.v;
            Ys.v = PlaceY.v; 

           
            //Current = CurrentPar + CurrentIn;
            for (int p = 0; p < maxsize; ++p) {
                rack::simd::float_4 P(p);
                rack::simd::float_4 Xsprev = Xs;
                rack::simd::float_4 Ysprev = Ys;
                Xs = Paths.xchange(a, Xsprev, Ysprev, wave);
                Ys = Paths.ychange(b, Xsprev, Ysprev, wave);
                for (int s = 0; s < 4; ++s) {
                    float itbelow = 0;
                    float itabove = 1;
                    if (P[s] == (int)Current[s]) {
                        itabove = Current[s] - (int)Current[s];
                        itbelow = 1.0 - itabove;
                        Xouts[s] = (itabove * Xs[s]) + (itbelow * Xsprev[s]);
                        Youts[s] = -((itabove * Ys[s]) + (itbelow * Ysprev[s]));
                        
                    }
                    
                }
            }

            rack::simd::float_4 Xoutend = Xouts;
            rack::simd::float_4 Youtend = Youts;
            if (rangetype == 2) {
                dcRemoveX.process(Xouts);
                dcRemoveY.process(Youts);
                Xoutend = dcRemoveX.highpass();
                Youtend = dcRemoveY.highpass();

            }

            outputs[X_1_OUTPUT].setVoltage(Xoutend[0] * 5.f, 0);
            outputs[Y_1_OUTPUT].setVoltage(Youtend[0] * 5.f, 0);
            outputs[X_2_OUTPUT].setVoltage(Xoutend[1] * 5.f, 0);
            outputs[Y_2_OUTPUT].setVoltage(Youtend[1] * 5.f, 0);
            outputs[X_3_OUTPUT].setVoltage(Xoutend[2] * 5.f, 0);
            outputs[Y_3_OUTPUT].setVoltage(Youtend[2] * 5.f, 0);
            outputs[X_4_OUTPUT].setVoltage(Xoutend[3] * 5.f, 0);
            outputs[Y_4_OUTPUT].setVoltage(Youtend[3] * 5.f, 0);

            float mixX = 0.f;
            for (int m = 0; m < 4; ++m) {
                mixX += Xoutend[m] * 2.f;
            }

            outputs[MIX_X_OUTPUT].setVoltage(mixX, 0);

            float mixY = 0.f;
            for (int m = 0; m < 4; ++m) {
                mixY += Youtend[m] * 2.f;
            }
            outputs[MIX_Y_OUTPUT].setVoltage(mixY, 0);
    }

    json_t* dataToJson() override {
		json_t* rootJ = json_object();

		json_t* speedRangeJ = json_integer(rangetype);

		json_object_set_new(rootJ, "Speed-Range", speedRangeJ);

		return rootJ;
    }

	void dataFromJson(json_t* rootJ) override {
		json_t* speedRangeJ = json_object_get(rootJ, "Speed-Range");
		if (speedRangeJ) {

					rangetype = json_integer_value(speedRangeJ);
			
		}

	}

};


struct SimWidgetBuffer : FramebufferWidget {
    SimoneModule* Simon;
    SimWidgetBuffer(SimoneModule* m) {
        Simon = m;
    }
    void step() override {
        FramebufferWidget::dirty = false;
        if (Simon->dirty) {
            FramebufferWidget::dirty = true;
            Simon->dirty = false;
        }
        FramebufferWidget::step();
    }
};

struct SimoneWidget : Widget {
    
    SimoneModule* Simon;

    SimoneWidget(SimoneModule* module, Vec topLeft) {
        Simon = module;
        box.pos = topLeft;

    }
    BaseFunctions Functions;
    PathEquate Paths;
    
    rack::simd::float_4 Wdt = 0.0;
    bool dtdown = false;
    void drawLayer(const DrawArgs& args, int layer) override {
        if (layer == 1) {
            int drawboxX = box.size.x;
            int drawboxY = box.size.y;
            nvgScissor(args.vg, 0, 0, drawboxX, drawboxY);
            float b = Simon->b;
            float a = Simon->a;
            float wave = Simon->wave;
            float rad = Simon->Tradius;
            rack::simd::float_4 spread(0.032, 0.0751, 0.121, 0.104); // = Simon->PlaceX;

            float xd = 0.f;
            float yd = 0.f;
            float Lx = xd;
            float Rx = xd;
            float Ly = yd;
            float Ry = yd;

            rack::simd::float_4 xds(0.f);
            rack::simd::float_4 yds(0.f);
            int maxsize = Simon->maxsize;
            nvgBeginPath(args.vg);
            nvgFillColor(args.vg, nvgRGBAf(0.62, 0.52, 0.75, 0.12));
            nvgRect(args.vg, 0, 0, drawboxX, drawboxY);
            nvgFill(args.vg);
            nvgClosePath(args.vg);
            Functions.incrementPhase(Simon->timePitch, 4410, &Wdt, _2PI);
            for (int i = 0; i < 30; i += 2) {
                for (int j = 0; j < 30; j += 2) {

                    
                    xd = i + simd::sin(Wdt[0]) * rad;
                    yd = j + simd::cos(Wdt[0]) * rad;
                    Lx = xd - spread[0];
                    Ly = yd - spread[1];
                    Rx = -xd + spread[2];
                    Ry = -yd + spread[3];
                    xds = rack::simd::float_4{ xd, Lx, Rx, 0 };
                    yds = rack::simd::float_4{ yd, Ly, Ry, 0 };
                    for (int k = 0; k <= maxsize; k++) {

                        rack::simd::float_4 XDsprev = xds;
                        rack::simd::float_4 YDsprev = yds;

                        xds = Paths.xchange(a, XDsprev, YDsprev, wave);
                        yds = Paths.ychange(b, XDsprev, YDsprev, wave);

                        nvgBeginPath(args.vg);
                        nvgFillColor(args.vg, nvgRGBAf(Functions.lerp(0, 1, -PI, PI, a),
                            Functions.lerp(0, 1, -PI, PI, b),
                            Functions.lerp(0, 1, -_2PI, _2PI, a + xds[0] + yds[0]),
                            0.36));
                        nvgRect(args.vg, Functions.lerp(0, drawboxX, -1, 1, xds[0]), 
                            Functions.lerp(0, drawboxY, -1, 1, yds[0]), 1, 1);
                        nvgFill(args.vg);

                        nvgBeginPath(args.vg);
                        nvgFillColor(args.vg, nvgRGBAf(Functions.lerp(0, 1, -PI, PI, a),
                            Functions.lerp(0, 1, -PI, PI, -b),
                            Functions.lerp(0, 1, -_2PI, _2PI, b + xds[1] + yds[1]),
                            0.32));
                        nvgRect(args.vg, Functions.lerp(0, drawboxX, -1, 1, xds[1]), Functions.lerp(0, drawboxY, -1, 1, yds[1]), 1, 1);
                        nvgFill(args.vg);

                        nvgBeginPath(args.vg);
                        nvgFillColor(args.vg, nvgRGBAf(Functions.lerp(0, 1, -PI, PI, a),
                            Functions.lerp(0, 1, -_2PI, _2PI, xds[2] + yds[2]),
                            Functions.lerp(0, 1, -PI, PI, b),
                            0.32));
                        nvgRect(args.vg, Functions.lerp(0, drawboxX, -1, 1, xds[2]), Functions.lerp(0, drawboxY, -1, 1, yds[2]), 1, 1);
                        nvgFill(args.vg);

                          
                        
                    }
                }
            }
            std::vector<rack::simd::float_4> TrailsX;
            std::vector<rack::simd::float_4> TrailsY;
            Simon->follow->PeekPoints(&TrailsX, &TrailsY);
            for (int t = 0; t < (int)TrailsX.size(); ++t) {

                nvgBeginPath(args.vg);
                nvgFillColor(args.vg, nvgRGBAf(1.0, 1.0, 0.0, 0.36));
                nvgRect(args.vg, Functions.lerp(0, drawboxX, -1, 1, TrailsX[t][0]),
                    Functions.lerp(0, drawboxY, -1, 1, -TrailsY[t][0]), 2, 2);
                nvgFill(args.vg);

                nvgBeginPath(args.vg);
                nvgFillColor(args.vg, nvgRGBAf(1.0, 0.0, 1.0, 0.36));
                nvgRect(args.vg, Functions.lerp(0, drawboxX, -1, 1, TrailsX[t][1]),
                    Functions.lerp(0, drawboxY, -1, 1, -TrailsY[t][1]), 2, 2);
                nvgFill(args.vg);

                nvgBeginPath(args.vg);
                nvgFillColor(args.vg, nvgRGBAf(0.0, 0.0, 1.0, 0.36));
                nvgRect(args.vg, Functions.lerp(0, drawboxX, -1, 1, TrailsX[t][2]),
                    Functions.lerp(0, drawboxY, -1, 1, -TrailsY[t][2]), 2, 2);
                nvgFill(args.vg);

                nvgBeginPath(args.vg);
                nvgFillColor(args.vg, nvgRGBAf(0.0, 1.0, 0.0, 0.36));
                nvgRect(args.vg, Functions.lerp(0, drawboxX, -1, 1, TrailsX[t][3]),
                    Functions.lerp(0, drawboxY, -1, 1, -TrailsY[t][3]), 2, 2);
                nvgFill(args.vg);
            }
        }
        Widget::drawLayer(args, layer);
    }
};

struct SimonePanelWidget : ModuleWidget {
    SimonePanelWidget(SimoneModule* module) {
        setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Simone_panel.svg")));

		addChild(createWidget<ScrewSilver>(Vec(15, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 30, 0)));
		addChild(createWidget<ScrewSilver>(Vec(15, 365)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 30, 365)));


        addParam(createParam<RoundHugeBlackKnob>(Vec(123, 158), module, SimoneModule::SPEED_PARAM));
        addParam(createParam<RoundSmallBlackKnob>(Vec(234, 150), module, SimoneModule::FM_PARAM));
        addParam(createParam<RoundSmallBlackKnob>(Vec(234, 190), module, SimoneModule::PM_PARAM));
        addParam(createParam<RoundSmallBlackKnob>(Vec(43, 235), module, SimoneModule::POSITION1_PARAM));
        addParam(createParam<RoundSmallBlackKnob>(Vec(43, 269), module, SimoneModule::POSITION2_PARAM));
        addParam(createParam<RoundSmallBlackKnob>(Vec(43, 303), module, SimoneModule::POSITION3_PARAM));
        addParam(createParam<RoundSmallBlackKnob>(Vec(43, 338), module, SimoneModule::POSITION4_PARAM));
        addParam(createParam<RoundSmallBlackKnob>(Vec(40, 119), module, SimoneModule::WAVE_PARAM));
        addParam(createParam<RoundSmallBlackKnob>(Vec(40, 160), module, SimoneModule::T_RAD_PARAM));
        addParam(createParam<RoundSmallBlackKnob>(Vec(40, 37), module, SimoneModule::A_PARAM));
        addParam(createParam<RoundSmallBlackKnob>(Vec(40, 78), module, SimoneModule::B_PARAM));
        addParam(createParam<RoundSmallBlackKnob>(Vec(231, 235), module, SimoneModule::ITER1_PARAM));
        addParam(createParam<RoundSmallBlackKnob>(Vec(231, 269), module, SimoneModule::ITER2_PARAM));
        addParam(createParam<RoundSmallBlackKnob>(Vec(231, 303), module, SimoneModule::ITER3_PARAM));
        addParam(createParam<RoundSmallBlackKnob>(Vec(231, 338), module, SimoneModule::ITER4_PARAM));
        addParam(createParam<Trimpot>(Vec(105, 251), module, SimoneModule::OFFSET_PARAMS + 0));
        addParam(createParam<Trimpot>(Vec(178, 251), module, SimoneModule::OFFSET_PARAMS + 1));
        addParam(createParam<Trimpot>(Vec(205, 226), module, SimoneModule::OFFSET_PARAMS + 2));

        
        addParam(createParam<VCVButton>(Vec(270, 75), module, SimoneModule::RANGE_BUTTON_PARAM));

        addInput(createInput<PurplePort>(Vec(88.5, 199), module, SimoneModule::SPEED1_INPUT));
        addInput(createInput<PurplePort>(Vec(118.5, 225), module, SimoneModule::SPEED2_INPUT));
        addInput(createInput<PurplePort>(Vec(158.5, 225), module, SimoneModule::SPEED3_INPUT));
        addInput(createInput<PurplePort>(Vec(188.5, 199), module, SimoneModule::SPEED4_INPUT));
        addInput(createInput<PurplePort>(Vec(266, 165), module, SimoneModule::FM_INPUT));
        addInput(createInput<PurplePort>(Vec(266, 202), module, SimoneModule::PM_INPUT));
        addInput(createInput<PurplePort>(Vec(10, 235), module, SimoneModule::POSITION1_INPUT));
        addInput(createInput<PurplePort>(Vec(10, 269), module, SimoneModule::POSITION2_INPUT));
        addInput(createInput<PurplePort>(Vec(10, 303), module, SimoneModule::POSITION3_INPUT));
        addInput(createInput<PurplePort>(Vec(10, 338), module, SimoneModule::POSITION4_INPUT));
        addInput(createInput<PurplePort>(Vec(10, 136), module, SimoneModule::WAVE_INPUT));
        addInput(createInput<PurplePort>(Vec(10, 177), module, SimoneModule::T_RAD_INPUT));
        addInput(createInput<PurplePort>(Vec(10, 54), module, SimoneModule::A_INPUT));
        addInput(createInput<PurplePort>(Vec(10, 95), module, SimoneModule::B_INPUT));
        addInput(createInput<PurplePort>(Vec(266, 235), module, SimoneModule::ITER1_INPUT));
        addInput(createInput<PurplePort>(Vec(266, 269), module, SimoneModule::ITER2_INPUT));
        addInput(createInput<PurplePort>(Vec(266, 303), module, SimoneModule::ITER3_INPUT));
        addInput(createInput<PurplePort>(Vec(266, 338), module, SimoneModule::ITER4_INPUT));


        addOutput(createOutput<PurplePort>(Vec(92, 295), module, SimoneModule::X_1_OUTPUT));
        addOutput(createOutput<PurplePort>(Vec(92, 324), module, SimoneModule::Y_1_OUTPUT));
        addOutput(createOutput<PurplePort>(Vec(122.5, 308), module, SimoneModule::X_2_OUTPUT));
        addOutput(createOutput<PurplePort>(Vec(122.5, 338), module, SimoneModule::Y_2_OUTPUT));
        addOutput(createOutput<PurplePort>(Vec(153.5, 308), module, SimoneModule::X_3_OUTPUT));
        addOutput(createOutput<PurplePort>(Vec(153.5, 338), module, SimoneModule::Y_3_OUTPUT));
        addOutput(createOutput<PurplePort>(Vec(185, 295), module, SimoneModule::X_4_OUTPUT));
        addOutput(createOutput<PurplePort>(Vec(185, 324), module, SimoneModule::Y_4_OUTPUT));
        addOutput(createOutput<PurplePort>(Vec(122.5, 278), module, SimoneModule::MIX_X_OUTPUT));
        addOutput(createOutput<PurplePort>(Vec(153.5, 278), module, SimoneModule::MIX_Y_OUTPUT));

        
        if (module) {
            SimWidgetBuffer* SimBuffer = new SimWidgetBuffer(module);
            SimoneWidget* simWidget = new SimoneWidget(module, Vec(69.5, 30.5));
            simWidget->setSize(Vec(160, 120));
            SimBuffer->addChild(simWidget);
            addChild(SimBuffer);
        }

    }

};

Model* modelSimone = createModel<SimoneModule, SimonePanelWidget>("Simone-Chaos");