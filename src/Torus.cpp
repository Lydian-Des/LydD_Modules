#include "plugin.hpp"
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

    void Electron(float R, float r, rack::simd::float_4 tWind, rack::simd::float_4 tPhase, rack::simd::float_4 nWind, 
                                    rack::simd::float_4 nPhase, rack::simd::float_4 Coord[]) {
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
        Coord[0] = X;
        Coord[1] = Y;
        Coord[2] = Z;
   
    }


    void Folding(float R, float r, rack::simd::float_4 tWind, rack::simd::float_4 tPhase, rack::simd::float_4 nWind, 
                                    rack::simd::float_4 nPhase, rack::simd::float_4 Coord[]) {
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
        Coord[0] = X;
        Coord[1] = Y;
        Coord[2] = Z;
    }


    void Torus(float R, float r, rack::simd::float_4 tWind, rack::simd::float_4 tPhase, rack::simd::float_4 nWind, 
                                    rack::simd::float_4 nPhase, rack::simd::float_4 Coord[]) {
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
        Coord[0] = X;
        Coord[1] = Y;
        Coord[2] = Z;

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
        FOLLOW_BUTTON_PARAM,
        DETUNE_PARAM,
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
        SYNC_INPUT,
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
        configParam(DETUNE_PARAM, -1.f, 1.f, 0.f, "Detune");

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
    rack::simd::float_4 Coord[3];

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
    float detune = 0.0;
    rack::simd::float_4 tPitches; 
    rack::simd::float_4 tPhases;
    rack::simd::float_4 tWinds;
    rack::simd::float_4 tWindsA;
    rack::simd::float_4 nPitches;
    rack::simd::float_4 nPhases;
    rack::simd::float_4 nWinds;
    rack::simd::float_4 nWindsA;

    int step = 0;
    int phasenum = 0;
    int nearcnt = 0;
    
    bool dirty = false;
    bool LFOmode1 = false;
    bool LFO1not = false;
    bool LFOmode2 = false;
    bool LFO2not = false;
    bool LFOllow = false;
    bool LFOlnot = false;
    bool isinsync = false;
    int equation = 0;
    bool eqset = false;
    float refFreq = 130.813;
    
    
    std::vector<rack::simd::float_4> pathToDraw;


 
    rack::dsp::BooleanTrigger SyncBeat;
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
        Buttons.latchButton(params[FOLLOW_BUTTON_PARAM].value, &LFOllow, &LFOlnot);
        isinsync = inputs[SYNC_INPUT].isConnected();

        equation = (int)params[EQUATION_SWITCH_PARAM].value;
        rdiff = params[R_DIFF_PARAM].value + ((inputs[R_DIFF_INPUT].isConnected()) ? Functions.lerp(-10.f, 45.f, -5.f, 5.f, inputs[R_DIFF_INPUT].getVoltage(0)) : 0.f);
        rdiff = rack::math::clamp(rdiff, -10.f, 45.f);
        r = R - rdiff;
    
        detune = params[DETUNE_PARAM].value;

        tWind = params[BIG_WIND_PARAM].value + ((inputs[BIG_WIND_INPUT].isConnected()) ? (inputs[BIG_WIND_INPUT].getVoltage(0))  : 0.f);
        nWind = params[LITTLE_WIND_PARAM].value + ((inputs[LITTLE_WIND_INPUT].isConnected()) ? (inputs[LITTLE_WIND_INPUT].getVoltage(0)) : 0.f);
        tWind = rack::math::clamp(abs(tWind), 0.f, 5.f);
        nWind = rack::math::clamp(abs(nWind), 0.f, 5.f);
        
        tWinds = rack::simd::float_4{ tWind };
        nWinds = rack::simd::float_4{ nWind };

        float bigpitchpar = params[BIG_PITCH_PARAM].value;
        float bigpitchin = inputs[BIG_PITCH_INPUT].getVoltage(0);
        float bigpitch = (inputs[BIG_PITCH_INPUT].isConnected()) ? bigpitchpar + bigpitchin : bigpitchpar;
        tPitch = Functions.VoltToFreq(bigpitch, 0.0, refFreq);

        float lilpitchpar = params[LITTLE_PITCH_PARAM].value;
        float lilpitchin = inputs[LITTLE_PITCH_INPUT].getVoltage(0);
        float lilpitch = (inputs[LITTLE_PITCH_INPUT].isConnected()) ? lilpitchpar + lilpitchin : lilpitchpar;
        nPitch = (LFOllow) ? Functions.VoltToFreq(lilpitch + bigpitch, 0.0, refFreq) : Functions.VoltToFreq(lilpitch, 0.0, refFreq);
        
        float tDetune = (tPitch * (detune / 12.f)) / 5.f;
        float nDetune = (nPitch * (detune / 12.f)) / 5.f;

        tPitches = rack::simd::float_4{ tPitch, tPitch + tDetune, tPitch - tDetune, tPitch + (tDetune / 2.f) };
        nPitches = rack::simd::float_4{ nPitch, nPitch + nDetune, nPitch - nDetune, nPitch + (nDetune / 2.f) };

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
        //rack::simd::float_4 tFM = tPitches;// *(inFM + 1.f);
        //rack::simd::float_4 nFM = nPitches;// *(inFM + 1.f);
        rack::simd::float_4 tPM = tPitches * inPM * args.sampleTime;
        rack::simd::float_4 nPM = nPitches * inPM * args.sampleTime;
        float loudcomp = 3.1;
        switch (equation) {
        case 0: {

            Paths.Electron(R, r, tWinds, tPhases, nWinds, nPhases, Coord);


            break;
        }
        case 1: {
            loudcomp = 2.6;
            Paths.Folding(R, r, tWinds, tPhases, nWinds, nPhases,  Coord);


            break;
        }
        case 2: {
            loudcomp = 3.9;
            Paths.Torus(R, r, tWinds, tPhases, nWinds, nPhases, Coord);


            break;
        }
        }

        
        float Xout = 0;
        float Yout = 0;
        float Zout = 0;

        drive = (inputs[DRIVE_INPUT].isConnected()) ? abs(inputs[DRIVE_INPUT].getVoltage(0)) * (drivepar) : drivepar;
        rack::simd::float_4 distortOuts[3];
        for (int i = 0; i < 3; ++i) {
            //this drive is from VCV fundamental VCF
            rack::simd::float_4 normwave = Coord[i] / ((R + r));
            normwave *= pow((drive / 6.f), 3) + 1.f;
            normwave = rack::simd::clamp(normwave, -3.f, 3.f);
            normwave *= (27.f + normwave * normwave) / (27.f + 9.f * normwave * normwave);
            distortOuts[i] = normwave;
        }

        for (int i = 0; i < 4; ++i) {
            Xout += distortOuts[0][i];
            Yout += distortOuts[1][i];
            Zout += distortOuts[2][i];
        }

        rack::simd::float_4 path{ Coord[0][0], Coord[1][0], Coord[2][0], 0.f};
        pathToDraw = std::vector<rack::simd::float_4>{ path, Zero, Zero, Zero};

        float xLerp = Functions.lerp(-5, 5, -(loudcomp), loudcomp, Xout);
        float yLerp = Functions.lerp(-5, 5, -(loudcomp), loudcomp, Yout);
        float zLerp = Functions.lerp(-5, 5, -(loudcomp), loudcomp, Zout);

        if (step % 800 == 0 ) {
            dirty = true;
        }
        step++;
        step %= 2520;


        outputs[X_OUTPUT].setVoltage(xLerp, 0);
        outputs[Y_OUTPUT].setVoltage(yLerp, 0);
        outputs[Z_OUTPUT].setVoltage(zLerp, 0);



        bool syncset = (isinsync) ? inputs[SYNC_INPUT].getVoltage(0) > 0.f : false;
        bool sync = SyncBeat.process(syncset);
        if (sync) {
            tPhases = Zero;
            nPhases = Zero;
        }
        Functions.incrementPhase(tPitches, args.sampleRate, &tPhases, 48.f * PI);
        Functions.incrementPhase(nPitches, args.sampleRate, &nPhases, 48.f * PI);
        tPhases += tPM;
        nPhases += nPM;

    }

    json_t* dataToJson() override {
        json_t* rootJ = json_object();

        json_t* LFO1J = json_boolean(LFOmode1);
        json_t* LFO2J = json_boolean(LFOmode2);
        json_t* LFOllowJ = json_boolean(LFOllow);

        json_object_set_new(rootJ, "Lfo-1", LFO1J);
        json_object_set_new(rootJ, "Lfo-2", LFO2J);
        json_object_set_new(rootJ, "Lfo-follow", LFOllowJ);

        return rootJ;
    }

    void dataFromJson(json_t* rootJ) override {
        json_t* LFO1J = json_object_get(rootJ, "Lfo-1");
        json_t* LFO2J = json_object_get(rootJ, "Lfo-2");
        json_t* LFOllowJ = json_object_get(rootJ, "Lfo-follow");

        if (LFO1J) {
            LFOmode1 = json_boolean_value(LFO1J);
        }
        if (LFO2J) {
            LFOmode2 = json_boolean_value(LFO2J);
        }
        if (LFOllowJ) {
            LFOllow = json_boolean_value(LFOllowJ);
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
        addParam(createParam<RoundSmallBlackKnob>(Vec(145, 162), module, TorusModule::DETUNE_PARAM));

        
        addParam(createParam<VCVButton>(Vec(210, 79), module, TorusModule::FOLLOW_BUTTON_PARAM));
        addParam(createParam<VCVButton>(Vec(210, 134), module, TorusModule::LFO2_BUTTON_PARAM));
        addParam(createParam<VCVButton>(Vec(10, 134), module, TorusModule::LFO1_BUTTON_PARAM));
        addParam(createParam<PurpleSwitch>(Vec(10, 74), module, TorusModule::EQUATION_SWITCH_PARAM));

        addInput(createInput<PurplePort>(Vec(10, 300), module, TorusModule::BIG_PITCH_INPUT));
        addInput(createInput<PurplePort>(Vec(10, 332), module, TorusModule::LITTLE_PITCH_INPUT));
        addInput(createInput<PurplePort>(Vec(50, 288), module, TorusModule::FM_INPUT));
        addInput(createInput<PurplePort>(Vec(50, 322), module, TorusModule::DRIVE_INPUT));
        addInput(createInput<PurplePort>(Vec(168, 322), module, TorusModule::BIG_WIND_INPUT));
        addInput(createInput<PurplePort>(Vec(205, 332), module, TorusModule::LITTLE_WIND_INPUT));
        addInput(createInput<PurplePort>(Vec(205, 300), module, TorusModule::R_DIFF_INPUT));
        addInput(createInput<PurplePort>(Vec(168, 288), module, TorusModule::SYNC_INPUT));
        
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