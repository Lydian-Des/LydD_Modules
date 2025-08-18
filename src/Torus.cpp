#include "demo-plugin.hpp"
#include <vector>


static const int maxPolyphony = 1;

static rack::simd::float_4 Zero{ 0.f };
static rack::simd::float_4 One{ 1.f };


class Follow {
private:
    std::vector<rack::simd::float_4> Points;
    

public:
    void buildPoints(rack::simd::float_4 Point) {
        Points.push_back(Point);
        
        if (Points.size() > 200) {
            Points.erase(Points.begin());

        }
    }
    void PeekPoints(std::vector<rack::simd::float_4>* askP) {
        *askP = Points;
    }
};


struct PathEquate {

    rack::simd::float_4 Electron(float R, float r, rack::simd::float_4 tWind, rack::simd::float_4 tPhase, rack::simd::float_4 nWind, rack::simd::float_4 nPhase) {
        float Tabove = tWind[0] - (int)tWind[0];
        float Tbelow = 1 - Tabove;
        float Nabove = nWind[0] - (int)nWind[0];
        float Nbelow = 1 - Nabove;
        for (int i = 0; i < 4; ++i) {
            tWind[i] = (int)tWind[i];
            nWind[i] = (int)nWind[i];
        }

        rack::simd::float_4 tBetweencos = (Tabove * sse_mathfun_cos_ps((tWind.v + 1.f) * tPhase.v)) 
            + (Tbelow * sse_mathfun_cos_ps(tWind.v * tPhase.v));

        rack::simd::float_4 tnBetweensin = (Tabove * sse_mathfun_sin_ps((nWind.v + 1.f) * tPhase.v)) 
            + (Tbelow * sse_mathfun_sin_ps(nWind.v * tPhase.v));

        rack::simd::float_4 nBetweencos = (Nabove * sse_mathfun_cos_ps((nWind.v + 1.f) * nPhase.v)) 
            + (Nbelow * sse_mathfun_cos_ps(nWind.v * nPhase.v));

        rack::simd::float_4 nBetweensin = (Nabove * sse_mathfun_sin_ps((nWind.v + 1.f) * nPhase.v)) 
            + (Nbelow * sse_mathfun_sin_ps(nWind.v * nPhase.v));
        
        rack::simd::float_4 X = (((R * tBetweencos) + (r * nBetweencos)) * sse_mathfun_cos_ps(tPhase.v));
        rack::simd::float_4 Y = (((R * tBetweencos) + (r * nBetweencos)) * sse_mathfun_sin_ps(tPhase.v));
        rack::simd::float_4 Z = r * (nBetweensin * tnBetweensin);
        return rack::simd::float_4{ X[0], Y[0], Z[0], 0.f };
    }


    rack::simd::float_4 Folding(float R, float r, rack::simd::float_4 tWind, rack::simd::float_4 tPhase, rack::simd::float_4 nWind, rack::simd::float_4 nPhase) {
        float Tabove = tWind[0] - (int)tWind[0];
        float Tbelow = 1 - Tabove;
        float Nabove = nWind[0] - (int)nWind[0];
        float Nbelow = 1 - Nabove;
        for (int i = 0; i < 4; ++i) {
            tWind[i] = (int)tWind[i];
            nWind[i] = (int)nWind[i];
        }

        rack::simd::float_4 tBetweencos = (Tabove * sse_mathfun_cos_ps((tWind.v + 1.f) * tPhase.v))
            + (Tbelow * sse_mathfun_cos_ps(tWind.v * tPhase.v));

        rack::simd::float_4 nBetweencos = (Nabove * sse_mathfun_cos_ps((nWind.v + 1.f) * nPhase.v))
            + (Nbelow * sse_mathfun_cos_ps(nWind.v * nPhase.v));

        rack::simd::float_4 nBetweensin = (Nabove * sse_mathfun_sin_ps((nWind.v + 1.f) * nPhase.v))
            + (Nbelow * sse_mathfun_sin_ps(nWind.v * nPhase.v));
        
        rack::simd::float_4 X = ((R * tBetweencos) + (r * nBetweencos)) * (sse_mathfun_cos_ps(tPhase.v) * -sse_mathfun_sin_ps(nPhase.v));
        rack::simd::float_4 Y = ((R * tBetweencos) + (r * nBetweencos)) * (sse_mathfun_sin_ps(tPhase.v) * sse_mathfun_cos_ps(nPhase.v));
        rack::simd::float_4 Z = r * (nBetweensin * tBetweencos);
        return rack::simd::float_4{ X[0], Y[0], Z[0], 0.f };
    }


    rack::simd::float_4 Torus(float R, float r, rack::simd::float_4 tWind, rack::simd::float_4 tPhase, rack::simd::float_4 nWind, rack::simd::float_4 nPhase) {
        float Tabove = tWind[0] - (int)tWind[0];
        float Tbelow = 1 - Tabove;
        float Nabove = nWind[0] - (int)nWind[0];
        float Nbelow = 1 - Nabove;
        for (int i = 0; i < 4; ++i) {
            tWind[i] = (int)tWind[i];
            nWind[i] = (int)nWind[i];
        }

        rack::simd::float_4 tBetweencos = (Tabove * sse_mathfun_cos_ps((tWind.v + 1.f) * tPhase.v)) 
            + (Tbelow * sse_mathfun_cos_ps(tWind.v * tPhase.v));

        rack::simd::float_4 tBetweensin = (Tabove * sse_mathfun_sin_ps((tWind.v + 1.f) * tPhase.v)) 
            + (Tbelow * sse_mathfun_sin_ps(tWind.v * tPhase.v));

        rack::simd::float_4 nBetweencos = (Nabove * sse_mathfun_cos_ps((nWind.v + 1.f) * nPhase.v)) 
            + (Nbelow * sse_mathfun_cos_ps(nWind.v * nPhase.v));

        rack::simd::float_4 nBetweensin = (Nabove * sse_mathfun_sin_ps((nWind.v + 1.f) * nPhase.v)) 
            + (Nbelow * sse_mathfun_sin_ps(nWind.v * nPhase.v));

        rack::simd::float_4 X = (R + (r * nBetweencos)) * tBetweencos;
        rack::simd::float_4 Y = (R + (r * nBetweencos)) * tBetweensin;
        rack::simd::float_4 Z = (r * nBetweensin + sse_mathfun_cos_ps(tPhase.v));
        return rack::simd::float_4{ X[0], Y[0], Z[0], 0.f };
        /*rack::simd::float_4 X = (R + (r * sse_mathfun_cos_ps(nWind.v * nPhase.v))) * sse_mathfun_cos_ps(tWind.v * tPhase.v);
        rack::simd::float_4 Y = (R + (r * sse_mathfun_cos_ps(nWind.v * nPhase.v))) * sse_mathfun_sin_ps(tWind.v * tPhase.v);
        rack::simd::float_4 Z = (r * sse_mathfun_sin_ps(nWind.v * nPhase.v) + sse_mathfun_cos_ps(tPhase.v));*/
    }

};

struct TorusModule : Module
{
    enum ParamIds {
        BIG_PITCH_PARAM,
        LITTLE_PITCH_PARAM,
        EQUATION_SWITCH_PARAM,
        LFO2_BUTTON_PARAM,
        LFO1_BUTTON_PARAM,
        BIG_WIND_PARAM,
        LITTLE_WIND_PARAM,
        FM_PARAM,
        R_DIFF_PARAM,
        DRIVE_PARAM,

        NUM_PARAMS
	};
    enum InputIds {
        BIG_PITCH_INPUT,
        LITTLE_PITCH_INPUT,
        BIG_WIND_INPUT,
        LITTLE_WIND_INPUT,
        FM_INPUT,
        R_DIFF_INPUT,
        DRIVE_INPUT,
        NUM_INPUTS
	};
    enum OutputIds {
        X_OUTPUT,
        Y_OUTPUT,
        Z_OUTPUT,

        NUM_OUTPUTS
	};
	enum LightIds {
        NUM_LIGHTS
    };

    TorusModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(BIG_PITCH_PARAM, -5.f, 5.f, 0.f, "Big Circle");
        configParam(LITTLE_PITCH_PARAM, -5.f, 5.f, 0.f, "Little Circle");
        configParam(R_DIFF_PARAM, -10.f, 42.f, 10.f, "Radial Difference");
        configParam(DRIVE_PARAM, 0.f, 11.f, 0.f, "Drive");

        configParam(LITTLE_WIND_PARAM, 1.f, 6.f, 1.f, "Little Windings");
        configParam(BIG_WIND_PARAM, 1.f, 6.f, 1.f, "Big Windings");
        configParam(FM_PARAM, -1.f, 1.f, 0.f, "FM");

        configParam(LFO2_BUTTON_PARAM, 0.f, 1.f, 0.f, "LFO->2");
        configParam(LFO1_BUTTON_PARAM, 0.f, 1.f, 0.f, "LFO->1");
        configParam(EQUATION_SWITCH_PARAM, 0.f, 2.f, 0.f, "Equation");

    }
    Follow* follow = new(Follow);
    BaseFunctions Functions;
    BaseButtons Buttons;
    BaseMatrices Matrix;
    PathEquate Paths;
    int currentPolyphony = 1;
    int currentBanks = 1;
    int loopCounter = 0;
  
  

    // TORUS
    float x = 1.0;
    float y = 1.0;
    float z = 1.0;
    rack::simd::float_4 Coord;
    rack::simd::float_4 CoordZero;
    float R = 45.0;
    float r = 45.0;
    float rdiff = 10.0;
    float tPitch = 0.0;
    float nPitch = 0.0;
    float tPhase = 0.0;
    float nPhase = 0.0;
    float tWind = 1.0;
    float nWind = 2.0;
    float drivepar = 0.0;
    float drive = 0.0;
    rack::simd::float_4 tPitches; 
    rack::simd::float_4 tPhases;
    rack::simd::float_4 tWindsL;
    rack::simd::float_4 tWindsA;
    rack::simd::float_4 nPitches;
    rack::simd::float_4 nPhases;
    rack::simd::float_4 nWindsL;
    rack::simd::float_4 nWindsA;

    int step = 0;
    int phasenum = 0;
    int nearcnt = 0;
    
    bool dirty = false;
    bool LFOmode1 = false;
    bool LFO1not = false;
    bool LFOmode2 = false;
    bool LFO2not = false;
    int equation = 0;
    bool eqset = false;
    float refFreq = 261.625565;
    
    
    std::vector<rack::simd::float_4> pathToDraw;


 

    rack::dsp::SlewLimiter _slewlimit{};
   // rack::dsp::SlewLimiter _slewlimitX{};
   

    void process(const ProcessArgs& args) override {

      
    
        if (loopCounter % 64 == 0) {

            checkInputs(args);
          
        }
        generateOutput(args);

        if (loopCounter % 4 == 0) {

            follow->buildPoints(pathToDraw[0]);

        }
       /* Functions.incrementPhase(tPitch, args.sampleRate, &tPhase);
        Functions.incrementPhase(nPitch, args.sampleRate, &nPhase);*/
        
        loopCounter++;
        if (loopCounter % 2048 == 0) {
            loopCounter = 1;
        }
    }

    void checkInputs(const ProcessArgs& args) {
        Buttons.latchButton(params[LFO1_BUTTON_PARAM].value, &LFOmode1, &LFO1not);
        Buttons.latchButton(params[LFO2_BUTTON_PARAM].value, &LFOmode2, &LFO2not);
        
        equation = (int)params[EQUATION_SWITCH_PARAM].value;
        rdiff = params[R_DIFF_PARAM].value + ((inputs[R_DIFF_INPUT].isConnected()) ? Functions.lerp(-10.f, 45.f, -5.f, 5.f, inputs[R_DIFF_INPUT].getVoltage(0)) : 0.f);
        rdiff = rack::math::clamp(rdiff, -10.f, 45.f);
        r = R - rdiff;
    
        tWind = params[BIG_WIND_PARAM].value + ((inputs[BIG_WIND_INPUT].isConnected()) ? (inputs[BIG_WIND_INPUT].getVoltage(0))  : 0.f);
        nWind = params[LITTLE_WIND_PARAM].value + ((inputs[LITTLE_WIND_INPUT].isConnected()) ? (inputs[LITTLE_WIND_INPUT].getVoltage(0)) : 0.f);
        tWind = rack::math::clamp(abs(tWind), 0.f, 5.f);
        nWind = rack::math::clamp(abs(nWind), 0.f, 5.f);
        
        tWindsL = rack::simd::float_4{ tWind };
        nWindsL = rack::simd::float_4{ nWind };

        
        tPitch = (inputs[BIG_PITCH_INPUT].isConnected()) ? Functions.VoltToFreq(params[BIG_PITCH_PARAM].value + inputs[BIG_PITCH_INPUT].getVoltage(0), 0.0, refFreq) : Functions.VoltToFreq(params[BIG_PITCH_PARAM].value, 0.0, refFreq);
        nPitch = (inputs[LITTLE_PITCH_INPUT].isConnected()) ? Functions.VoltToFreq(params[LITTLE_PITCH_PARAM].value + inputs[LITTLE_PITCH_INPUT].getVoltage(0), 1.0, refFreq) : Functions.VoltToFreq(params[LITTLE_PITCH_PARAM].value, 1.0, refFreq);
        
        tPitches = rack::simd::float_4{ tPitch };
        nPitches = rack::simd::float_4{ nPitch };

        if (LFOmode1) {
            tPitches = tPitches / 1024.f;
        }
        if (LFOmode2) {
            nPitches = nPitches / 1024.f;
        }

        drivepar = params[DRIVE_PARAM].value;
        
    }

    void generateOutput(const ProcessArgs& args) {
        //float inFM = (((inputs[FM_INPUT].isConnected()) ? inputs[FM_INPUT].getVoltage(0) * params[FM_PARAM].value : params[FM_PARAM].value));
        float inPM = (((inputs[FM_INPUT].isConnected()) ? inputs[FM_INPUT].getVoltage(0) * params[FM_PARAM].value : params[FM_PARAM].value));
        rack::simd::float_4 tFM = tPitches;// *(inFM + 1.f);
        rack::simd::float_4 nFM = nPitches;// *(inFM + 1.f);
        rack::simd::float_4 tPM = tPitches * inPM * args.sampleTime;
        rack::simd::float_4 nPM = nPitches * inPM * args.sampleTime;
        float eqrange = 1;

        switch (equation) {
        case 0: {
            eqrange = 3.4;
            Coord = Paths.Electron(R, r, tWindsL, tPhases, nWindsL, nPhases);
            CoordZero = Paths.Electron(R, r, tWindsL, rack::simd::float_4{ 0.f }, nWindsL, rack::simd::float_4{ 0.f });

            break;
        }
        case 1: {
            eqrange = 1.5;
            Coord = Paths.Folding(R, r, tWindsL, tPhases, nWindsL, nPhases);
            CoordZero = Paths.Folding(R, r, tWindsL, rack::simd::float_4{ 0.f }, nWindsL, rack::simd::float_4{ 0.f });

            break;
        }
        case 2: {
            eqrange = 3.1;
            Coord = Paths.Torus(R, r, tWindsL, tPhases, nWindsL, nPhases);
            CoordZero = Paths.Torus(R, r, tWindsL, rack::simd::float_4{ 0.f }, nWindsL, rack::simd::float_4{ 0.f });

            break;
        }
        }

        drive = (inputs[DRIVE_INPUT].isConnected()) ? inputs[DRIVE_INPUT].getVoltage(0) * (drivepar / 5.f) : drivepar;


        rack::simd::float_4 normwave = Coord / (R + r);
        normwave *= pow((drive / 6.f), 3) + 1.f;

        normwave = rack::simd::clamp(normwave, -3.f, 3.f);
        normwave *=  (27.f + normwave * normwave) / (27.f + 9.f * normwave * normwave);
       

        float Xout = normwave[0];
        float Yout = normwave[1];
        float Zout = normwave[2];

        rack::simd::float_4 path{ Xout, Yout, Zout, 0.f };
        pathToDraw = std::vector<rack::simd::float_4>{ Coord, Zero, Zero, Zero };

        float xLerp = Functions.lerp(-5, 5, -(eqrange ), eqrange , Xout);
        float yLerp = Functions.lerp(-5, 5, -(eqrange), eqrange, Yout);
        float zLerp = Functions.lerp(-5, 5, -(eqrange), eqrange, Zout);

        if (step % 800 == 0 ) {
            dirty = true;
        }
        outputs[X_OUTPUT].setVoltage(xLerp, 0);
        outputs[Y_OUTPUT].setVoltage(yLerp, 0);
        outputs[Z_OUTPUT].setVoltage(zLerp, 0);
        step++;
        step %= 2000;


        Functions.incrementPhase(tFM, args.sampleRate, &tPhases, 48.f * PI);
        Functions.incrementPhase(nFM, args.sampleRate, &nPhases, 48.f * PI);
        tPhases += tPM;
        nPhases += nPM;

    }

    json_t* dataToJson() override {
        json_t* rootJ = json_object();

        json_t* LFO1J = json_boolean(LFOmode1);
        json_t* LFO2J = json_boolean(LFOmode2);

        json_object_set_new(rootJ, "Lfo-1", LFO1J);
        json_object_set_new(rootJ, "Lfo-2", LFO2J);

        return rootJ;
    }

    void dataFromJson(json_t* rootJ) override {
        json_t* LFO1J = json_object_get(rootJ, "Lfo-1");
        json_t* LFO2J = json_object_get(rootJ, "Lfo-2");

        if (LFO1J) {
            LFOmode1 = json_boolean_value(LFO1J);
        }
        if (LFO2J) {
            LFOmode2 = json_boolean_value(LFO2J);
        }

    }

};


struct TorusDrawWidgetBuffer : FramebufferWidget {
    TorusModule* Tora;
    TorusDrawWidgetBuffer(TorusModule* m) {
        Tora = m;
    }
    void step() override {
        FramebufferWidget::dirty = false;
        if (Tora->dirty) {
            FramebufferWidget::dirty = true;
            Tora->dirty = false;
        }
        FramebufferWidget::step();
    }
};

struct TorusDrawWidget : Widget {
    
    TorusModule* Tora;

    TorusDrawWidget(TorusModule* module, Vec topLeft) {
        Tora = module;
        box.pos = topLeft;

    }
    BaseFunctions Functions;
    BaseMatrices Matrix;
    PathEquate Paths;

    int frames = 0;
    std::vector<rack::simd::float_4> RotMatrixZ;
    std::vector<rack::simd::float_4> RotMatrixX;
    std::vector<rack::simd::float_4> ProjectMatrix;
    rack::simd::float_4 angle{ 30.0 };
    rack::simd::float_4 anglerot{ 0.0 };
    void drawLayer(const DrawArgs& args, int layer) override {

        RotMatrixZ = Matrix.RotationXY(anglerot);
        RotMatrixX = Matrix.RotationYZ(angle);
        ProjectMatrix = Matrix.Projection(1.f / 1.5f);

        if (layer == 1) {
            int drawboxX = box.size.x;
            int drawboxY = box.size.y;
            nvgScissor(args.vg, 0, 0, drawboxX, drawboxY);
            nvgBeginPath(args.vg);
            nvgFillColor(args.vg, nvgRGBAf(0.62, 0.52, 0.75, 0.12));
            nvgRect(args.vg, 0, 0, drawboxX, drawboxY);
            nvgFill(args.vg);
            nvgClosePath(args.vg);
            //if (frames %= 16) {
                anglerot += _2PI / 360.f;
                angle += _2PI / 420.f;
            //}
            if (angle[0] > _2PI) {
                angle -=  _2PI;
            }
            if (anglerot[0] > _2PI) {
                anglerot -=  _2PI;
            }
            rack::simd::float_4 color{ Tora->tWind, Tora->nWind, Tora->drive, 1.f };

            std::vector<rack::simd::float_4> Line;
            Tora->follow->PeekPoints(&Line);
                
            frames++;
            frames %= 64;
                
            for (int d = 0; d < (int)Line.size(); ++d) {

                std::vector<rack::simd::float_4> LineprevVec{ Line[abs(d - 1)], Zero, Zero, Zero };
                LineprevVec = Matrix.MatrixMult(RotMatrixZ, LineprevVec);
                LineprevVec = Matrix.MatrixMult(RotMatrixX, LineprevVec);
                LineprevVec = Matrix.MatrixMult(ProjectMatrix, LineprevVec);
                rack::simd::float_4 Tailprev = LineprevVec[0];// *drawboxX / 1.6f;

                std::vector<rack::simd::float_4> LineVec{ Line[d], Zero, Zero, Zero};
                LineVec = Matrix.MatrixMult(RotMatrixZ, LineVec);
                LineVec = Matrix.MatrixMult(RotMatrixX, LineVec);
                LineVec = Matrix.MatrixMult(ProjectMatrix, LineVec);
                rack::simd::float_4 Tail = LineVec[0];// *drawboxX / 1.6f;

                nvgBeginPath(args.vg);
                nvgMoveTo(args.vg, Tailprev[0] + (drawboxX / 2), Tailprev[1] + (drawboxY / 2));
                nvgLineTo(args.vg, Tail[0] + (drawboxX / 2), Tail[1] + (drawboxY / 2));
                nvgStrokeWidth(args.vg, 1.62 );
                nvgStrokeColor(args.vg, nvgRGBAf(color[2] / 10.0, color[1] / 5.0, color[0] / 5.0, color[3]));
                nvgStroke(args.vg);
            }
               
        }
 
        Widget::drawLayer(args, layer);
    }
};

struct TorusPanelWidget : ModuleWidget {
    TorusPanelWidget(TorusModule* module) {
        setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Torus_panel.svg")));

		addChild(createWidget<ScrewSilver>(Vec(15, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 30, 0)));
		addChild(createWidget<ScrewSilver>(Vec(15, 365)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 30, 365)));


        addParam(createParam<RoundHugeBlackKnob>(Vec(25, 165), module, TorusModule::BIG_PITCH_PARAM));
        addParam(createParam<RoundBlackKnob>(Vec(20, 240), module, TorusModule::LITTLE_PITCH_PARAM));
        addParam(createParam<RoundSmallBlackKnob>(Vec(98, 162), module, TorusModule::FM_PARAM));
        addParam(createParam<RoundSmallBlackKnob>(Vec(151, 230), module, TorusModule::BIG_WIND_PARAM));
        addParam(createParam<RoundSmallBlackKnob>(Vec(199, 238), module, TorusModule::LITTLE_WIND_PARAM));       
        addParam(createParam<RoundSmallBlackKnob>(Vec(185, 192), module, TorusModule::R_DIFF_PARAM));
        addParam(createParam<RoundBlackKnob>(Vec(85, 228), module, TorusModule::DRIVE_PARAM));
        
        addParam(createParam<VCVButton>(Vec(210, 134), module, TorusModule::LFO2_BUTTON_PARAM));
        addParam(createParam<VCVButton>(Vec(10, 134), module, TorusModule::LFO1_BUTTON_PARAM));
        addParam(createParam<PurpleSwitch>(Vec(10, 74), module, TorusModule::EQUATION_SWITCH_PARAM));

        addInput(createInput<PurplePort>(Vec(10, 300), module, TorusModule::BIG_PITCH_INPUT));
        addInput(createInput<PurplePort>(Vec(10, 332), module, TorusModule::LITTLE_PITCH_INPUT));
        addInput(createInput<PurplePort>(Vec(50, 288), module, TorusModule::FM_INPUT));
        addInput(createInput<PurplePort>(Vec(50, 322), module, TorusModule::DRIVE_INPUT));
        addInput(createInput<PurplePort>(Vec(168, 322), module, TorusModule::BIG_WIND_INPUT));
        addInput(createInput<PurplePort>(Vec(205, 332), module, TorusModule::LITTLE_WIND_INPUT));
        addInput(createInput<PurplePort>(Vec(168, 288), module, TorusModule::R_DIFF_INPUT));
        
        addOutput(createOutput<PurplePort>(Vec(88, 288), module, TorusModule::X_OUTPUT));
        addOutput(createOutput<PurplePort>(Vec(130, 288), module, TorusModule::Y_OUTPUT));
        addOutput(createOutput<PurplePort>(Vec(108.5, 332), module, TorusModule::Z_OUTPUT));

        
        if (module) {
            TorusDrawWidgetBuffer* TorBuffer = new TorusDrawWidgetBuffer(module);
            TorusDrawWidget* TorWidget = new TorusDrawWidget(module, Vec(40, 31));
            TorWidget->setSize(Vec(160, 120));
            TorBuffer->addChild(TorWidget);
            addChild(TorBuffer);
        }

    }

};

Model* modelTorus = createModel<TorusModule, TorusPanelWidget>("Torus-Osc");
