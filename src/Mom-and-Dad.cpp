
#include "plugin.hpp"
#include <vector>



 

static const int maxPolyphony = 1;



class Lines {
private:
    //each float_4 of each vect is a point: x, y, z, w
    std::vector<rack::simd::float_4> Spawnline;
    std::vector<rack::simd::float_4> Momline;
    std::vector<rack::simd::float_4> Dadline;
public:
    void build(rack::simd::float_4 Spawnpush, rack::simd::float_4 Mompush, rack::simd::float_4 Dadpush){
        Spawnline.push_back(Spawnpush);
        Momline.push_back(Mompush);
        Dadline.push_back(Dadpush);
        if (Spawnline.size() > 1000) {
            Spawnline.erase(Spawnline.begin());
            Momline.erase(Momline.begin());
            Dadline.erase(Dadline.begin());
        }
    }
    void peekSpawn(std::vector<rack::simd::float_4>* lineask) {
        *lineask = Spawnline;
    }
    void peekMom(std::vector<rack::simd::float_4>* lineask) {
        *lineask = Momline;
    }
    void peekDad(std::vector<rack::simd::float_4>* lineask) {
        *lineask = Dadline;
    }

};

struct MomDadEq {

       


    rack::simd::float_4 MOMDAD(float a, float b, float g, float o, float dt, rack::simd::float_4 Coord, bool wType) {
        //Coord given as X, Y, Z, W
        
        float dx = ((a * Coord[0]) - (Coord[2] * Coord[1]) - Coord[3]) / dt;
        float dy = ((Coord[0] * Coord[2]) - (b * Coord[1]) + Coord[2]) / dt;
        float dz = ((Coord[0] * Coord[1]) - (g * Coord[2]) + Coord[0]) / dt;
        float dw = 0.f;
        if (!wType) {
            dw = ((-Coord[0] * Coord[2]) - (o * Coord[3])) / dt;	//wander
        }
        else {
            dw = ((-Coord[3] * Coord[2]) / (o - Coord[0])) / dt;		//scrawl
        }
        
        return Coord + rack::simd::float_4{ dx, dy, dz, dw };
    }


};

struct DadrasModule : Module
{
    enum ParamIds {
        SPEED_PARAM,
        SPREAD_PARAM,
        TIME_PHASE_PARAM,
        SPEED_SHIFT_PARAM,
        SYNCH_BUTTON_PARAM,
        RESET_BUTTON_PARAM,
        AXIS_SWITCH_PARAM,
        WTYPE_BUTTON_PARAM,
        INFLUENCE_DAD_PARAM,
        INFLUENCE_MOM_PARAM,
        A_PARAM,
        B_PARAM,
        G_PARAM,
        O_PARAM,
        NUM_PARAMS
	};
	enum InputIds {
        SPEED_INPUT,
        SPREAD_INPUT,
        TIME_PHASE_INPUT,
        SPEED_SHIFT_INPUT,
        INFLUENCE_DAD_INPUT,
        INFLUENCE_MOM_INPUT,
        SYNCH_INPUT,
        RESET_INPUT,
        A_INPUT,
        B_INPUT,
        G_INPUT,
        O_INPUT,
        NUM_INPUTS
	};
    enum OutputIds {
        L_X_OUTPUT,
        L_Y_OUTPUT,
        L_Z_OUTPUT,
        L_W_OUTPUT,
        C_X_OUTPUT,
        C_Y_OUTPUT,
        C_Z_OUTPUT,
        C_W_OUTPUT,
        R_X_OUTPUT,
        R_Y_OUTPUT,
        R_Z_OUTPUT,
        R_W_OUTPUT,
        NUM_OUTPUTS
	};
	enum LightIds {
        ENUMS(WTYPE_LIGHT, 2),
        NUM_LIGHTS
    };

    DadrasModule() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(SPEED_PARAM, -5.f, 5.f, 0.f, "Speed");
        configParam(SPREAD_PARAM, -1.f, 1.f, 0.f, "Separate");
        configParam(SPEED_SHIFT_PARAM, -2.f, 2.f, 0.f, "Speed Shift");
        configParam(TIME_PHASE_PARAM, -3.f, 3.f, 0.f, "Time Phase");
        configParam(INFLUENCE_DAD_PARAM, 0.f, 1.f, 0.f, "Fathers Influence");
        configParam(INFLUENCE_MOM_PARAM, 0.f, 1.f, 0.f, "Mothers Influence");
        configParam(A_PARAM, -5.f, 5.f, 0.f, "Force");
        configParam(B_PARAM, -5.f, 5.f, 0.f, "Split");
        configParam(G_PARAM, -5.f, 5.f, 0.f, "Dwell");
        configParam(O_PARAM, -5.f, 5.f, 0.f, "Throw");

        configParam(SYNCH_BUTTON_PARAM, 0.f, 1.f, 0.f, "Synchronize");
        configParam(RESET_BUTTON_PARAM, 0.f, 1.f, 0.f, "Reset");
        configParam(AXIS_SWITCH_PARAM, 0.f, 2.f, 0.f, "Axis");
        configParam(WTYPE_BUTTON_PARAM, 0.f, 1.f, 0.f, "Wander");

    }
  
    BaseFunctions Functions;
    BaseButtons Buttons;
    MomDadEq Paths;
    Lines* lines = new(Lines);

    int loopCounter = 0;
    bool dirty = false;
    int bounds = 200;
    float runSpeed = 20.f;
    bool synchro = false;
    bool syn = false;
    bool reSet = false;
    bool reReset = false;
    bool rev = false;
    bool WSet = false;
    bool WReset = false;

    float dt = 900;
    float a = 7.6;
    float b = 34.2;
    float g = 10.5;
    float o = 2.85;
    float startX = 11;
    float startY = 1;
    float startZ = -6;
    float startW = -1;

    rack::simd::float_4 Start{startX, startY, startZ, startW};
    rack::simd::float_4 CoordC = Start;
    rack::simd::float_4 CoordL = Start;
    rack::simd::float_4 CoordR = Start;


    float refFreq = 261.625565;
    float basePitch = refFreq;
    float PitchL = basePitch;
    float PitchR = basePitch;
    rack::simd::float_4 Pitch{ refFreq };
    float basePhase = 0.f;
    float PhaseL = basePhase;
    float PhaseR = basePhase;
    rack::simd::float_4 Phase{ 0.f };
    float _2pi = 2.f * PI;
    bool click1 = false;
    bool click2 = false;
    bool click3 = false;
    int axis = 0;
    bool axbut = false;
    
    float spreadX = 0.0;
    float spreadY = 0.0;
    float spreadZ = 0.0;
    rack::simd::float_4 Spread{ 0.f };

    float phaseShift;
    
    float dadsInf = 0.f;
    float momsInf = 0.f;
  
   


    void process(const ProcessArgs& args) override {     
        if (loopCounter % 32 == 0) {
            checkInputs(args);
        }
        if (loopCounter % 2520 == 0) {
            dirty = true;
            loopCounter = 0;
        }
        ++loopCounter;
        generateOutput(args);

        Functions.incrementPhase(Pitch, args.sampleRate, &Phase, _2PI);

    }

   

    void checkInputs(const ProcessArgs& args) {

        axis = (int)params[AXIS_SWITCH_PARAM].value;

        Buttons.momentButton(params[RESET_BUTTON_PARAM].value, &reSet, &reReset);
        if (inputs[RESET_INPUT].isConnected()) {
            Buttons.momentButton(inputs[RESET_INPUT].getVoltage(0), &reSet, &reReset);
        }

        Buttons.momentButton(params[SYNCH_BUTTON_PARAM].value, &synchro, &syn);
        if (inputs[SYNCH_INPUT].isConnected()) {
            Buttons.momentButton(inputs[SYNCH_INPUT].getVoltage(0), &synchro, &syn);
        }

        Buttons.latchButton(params[WTYPE_BUTTON_PARAM].value, &WSet, &WReset);
        lights[WTYPE_LIGHT + 0].setBrightness(!WSet);
        lights[WTYPE_LIGHT + 1].setBrightness(WSet);
        if (WSet) {
            paramQuantities[WTYPE_BUTTON_PARAM]->name = "Scratch";
        }

        float A = params[A_PARAM].value;
        float B = params[B_PARAM].value;
        float G = params[G_PARAM].value;
        float O = params[O_PARAM].value;
        if (inputs[A_INPUT].isConnected()) {
            A = A + inputs[A_INPUT].getVoltage(0);
        }
        if (inputs[B_INPUT].isConnected()) {
            B = B + inputs[B_INPUT].getVoltage(0);
        }
        if (inputs[G_INPUT].isConnected()) {
            G = G + inputs[G_INPUT].getVoltage(0);
        }
        if (inputs[O_INPUT].isConnected()) {
            O = O + inputs[O_INPUT].getVoltage(0);
        }
        a = Functions.lerp(5, 9, -5, 5, A);
        b = Functions.lerp(22, 45, -5, 5, B);
        g = Functions.lerp(6, 12, -5, 5, G);
        o = Functions.lerp(1, 4, -5, 5, O);

        float parSpeed = (params[SPEED_PARAM].value);
        float inSpeed = (inputs[SPEED_INPUT].isConnected()) ? inputs[SPEED_INPUT].getVoltage(0) : 0.f;
        float parshiftSpeed = (params[SPEED_SHIFT_PARAM].value);
        float inshiftSpeed = (inputs[SPEED_SHIFT_INPUT].isConnected()) ? inputs[SPEED_SHIFT_INPUT].getVoltage(0) : 0.f;
        basePitch = Functions.VoltToFreq(parSpeed + inSpeed, 0.0, refFreq);
        PitchL = Functions.VoltToFreq((parSpeed + inSpeed) - (parshiftSpeed + inshiftSpeed), 0.0, refFreq);
        PitchR = Functions.VoltToFreq((parSpeed + inSpeed) + (parshiftSpeed + inshiftSpeed), 0.0, refFreq);
        Pitch = rack::simd::float_4{ basePitch, PitchL, PitchR, 0.f };

        float parSpread = params[SPREAD_PARAM].value;
        float inSpread = (inputs[SPREAD_INPUT].isConnected()) ? inputs[SPREAD_INPUT].getVoltage(0) : 0.f;
        spreadX = (parSpread + inSpread) / 500;
        spreadY = (parSpread + inSpread) / 200;
        spreadZ = (parSpread - inSpread) / 400;
        
        Spread = rack::simd::float_4{ spreadX, spreadY, spreadZ, 0.f };

        float parphaseShift = params[TIME_PHASE_PARAM].value;
        float inphaseShift = (inputs[TIME_PHASE_INPUT].isConnected()) ? inputs[TIME_PHASE_INPUT].getVoltage(0) : 0.f;
        phaseShift = rack::math::clamp(parphaseShift + inphaseShift, -3.14, 3.14);

        float indad = (inputs[INFLUENCE_DAD_INPUT].isConnected()) ? Functions.lerp(0, 1, 0, 5, abs(inputs[INFLUENCE_DAD_INPUT].getVoltage(0))) : 0;
        float inmom = (inputs[INFLUENCE_MOM_INPUT].isConnected()) ? Functions.lerp(0, 1, 0, 5, abs(inputs[INFLUENCE_MOM_INPUT].getVoltage(0))) : 0;
        dadsInf = rack::math::clamp(params[INFLUENCE_DAD_PARAM].value + indad, 0.f, 1.f);
        momsInf = rack::math::clamp(params[INFLUENCE_MOM_PARAM].value + inmom, 0.f, 1.f);
        
    }

    void generateOutput(const ProcessArgs& args) {   
        float dtL = dt + (phaseShift * 100);
        float dtR = dt - (phaseShift * 100);
           
        if (synchro) {
            CoordL = CoordC;
            CoordR = CoordC;
        }
        if (reSet) {
            CoordC = Start;
            CoordL = Start;
            CoordR = Start;
        }

        for (int p = 0; p < 4; ++p) {
            if (CoordC[p] > bounds + 100) {
                CoordC = Start;
            }
            if (CoordL[p] > bounds + 100) {
                CoordL = Start;
            }
            if (CoordR[p] > bounds + 100) {
                CoordR = Start;
            }
        }

        rack::simd::float_4 CoordprevC = CoordC;
        rack::simd::float_4 CoordprevL = CoordL + Spread;
        rack::simd::float_4 CoordprevR = CoordR - Spread;

        if (Phase[0] <= PI) {
            click1 = true;
        }
        if (Phase[1] <= PI) {
            click2 = true;
        }
        if (Phase[2] <= PI) {
            click3 = true;
        }
        if (Phase[0] > PI && click1) {
            lines->build(CoordC, CoordL, CoordR);

            CoordprevC += Functions.incrementToward(CoordprevC, CoordL, 100) * momsInf;
            CoordprevC += Functions.incrementToward(CoordprevC, CoordR, 100) * dadsInf;
            CoordC = Paths.MOMDAD(a, b, g, o, dt, CoordprevC, WSet);
            
            click1 = false;
        }
        if (Phase[1] > PI && click2) {

            CoordL = Paths.MOMDAD(a, b, g, o, dtL, CoordprevL, WSet);

            click2 = false;
        }
        if (Phase[2] > PI && click3) {

            CoordR = Paths.MOMDAD(a, b, g, o, dtR, CoordprevR, WSet);


            click3 = false;
        }
        float outputC[4];
        float outputL[4];
        float outputR[4];
        for (int i = 0; i < 4; ++i) {
            outputC[i] = Functions.lerp(-5.f, 5.f, -bounds, bounds, CoordC[i]);
            outputC[i] = rack::math::clamp(outputC[i], -5.f, 5.f);
            outputL[i] = Functions.lerp(-5.f, 5.f, -bounds, bounds, CoordL[i]);
            outputL[i] = rack::math::clamp(outputL[i], -5.f, 5.f);
            outputR[i] = Functions.lerp(-5.f, 5.f, -bounds, bounds, CoordR[i]);
            outputR[i] = rack::math::clamp(outputR[i], -5.f, 5.f);
        }
        
          
            outputs[C_X_OUTPUT].setVoltage(outputC[0], 0);
            outputs[C_Y_OUTPUT].setVoltage(outputC[1], 0);
            outputs[C_Z_OUTPUT].setVoltage(outputC[2], 0);
            outputs[C_W_OUTPUT].setVoltage(outputC[3], 0);
            outputs[L_X_OUTPUT].setVoltage(outputL[0], 0);
            outputs[L_Y_OUTPUT].setVoltage(outputL[1], 0);
            outputs[L_Z_OUTPUT].setVoltage(outputL[2], 0);
            outputs[L_W_OUTPUT].setVoltage(outputL[3], 0);
            outputs[R_X_OUTPUT].setVoltage(outputR[0], 0);
            outputs[R_Y_OUTPUT].setVoltage(outputR[1], 0);
            outputs[R_Z_OUTPUT].setVoltage(outputR[2], 0);
            outputs[R_W_OUTPUT].setVoltage(outputR[3], 0);

    }
};


struct DadWidgetBuffer : FramebufferWidget {
    DadrasModule* Momeni;
    DadWidgetBuffer(DadrasModule* m) {
        Momeni = m;
    }
    void step() override {
        FramebufferWidget::dirty = false;
        if (Momeni->dirty) {
            FramebufferWidget::dirty = true;
            Momeni->dirty = false;
        }
        FramebufferWidget::step();
    }
};

struct DadWidget : Widget{
    
    DadrasModule* Momeni;

    DadWidget(DadrasModule* module, Vec topLeft) {
        Momeni = module;
        box.pos = topLeft;

    }
    BaseFunctions Functions;
    BaseMatrices Matrix;


    rack::simd::float_4 spinX = 60 ;
    rack::simd::float_4 spinY = 30 ;
    rack::simd::float_4 spinZ = 20 ;
    rack::simd::float_4 spinW = 0 ;
    bool tiltback = false;
    bool tiltbackY = false;
    bool tiltbackZ = false;
    bool tiltbackW = false;
    bool addSpin = false;
    int frames = 0;
    
    void drawLayer(const DrawArgs& args, int layer) override {
        if (layer == 1 /*&& Momeni->dirty*/) {
            int drawboxX = box.size.x;
            int drawboxY = box.size.y;
            int bound = Momeni->bounds;
            
            
                   
            nvgScissor(args.vg, 0, 0, drawboxX, drawboxY);

            //drawing a little room for the snakes to live in
            nvgStrokeWidth(args.vg, 1.2);
            nvgStrokeColor(args.vg, nvgRGBAf(0.4, 0.4, 0.2, 0.4));
            nvgFillColor(args.vg, nvgRGBAf(0.68, 0.57, 0.91, 0.21));
            nvgBeginPath(args.vg);            
            nvgMoveTo(args.vg, 0, 0);
            nvgLineTo(args.vg, drawboxX / 5, drawboxY / 5);
            nvgLineTo(args.vg, drawboxX / 5, 4 * drawboxY / 5);
            nvgLineTo(args.vg, 0, drawboxY);
            nvgClosePath(args.vg);
            nvgStroke(args.vg);
            nvgFill(args.vg);

            nvgBeginPath(args.vg);
            nvgMoveTo(args.vg, 0, 0);
            nvgLineTo(args.vg, drawboxX / 5, drawboxY / 5);
            nvgLineTo(args.vg, 4 * drawboxX / 5, drawboxY / 5);
            nvgLineTo(args.vg, drawboxX, 0);
            nvgClosePath(args.vg);
            nvgStroke(args.vg);
            nvgFill(args.vg);

            nvgBeginPath(args.vg);
            nvgMoveTo(args.vg, drawboxX, 0);
            nvgLineTo(args.vg, 4 * drawboxX / 5, drawboxY / 5);
            nvgLineTo(args.vg, 4 * drawboxX / 5, 4 * drawboxY / 5);
            nvgLineTo(args.vg, drawboxX, drawboxY);
            nvgClosePath(args.vg);
            nvgStroke(args.vg);
            nvgFill(args.vg);

            nvgBeginPath(args.vg);
            nvgMoveTo(args.vg, drawboxX, drawboxY);
            nvgLineTo(args.vg, 4 * drawboxX / 5, 4 *  drawboxY / 5);
            nvgLineTo(args.vg, drawboxX / 5, 4 * drawboxY / 5);
            nvgLineTo(args.vg, 0, drawboxY);
            nvgClosePath(args.vg);
            nvgStroke(args.vg);
            nvgFill(args.vg);

            nvgBeginPath(args.vg);
            nvgFillColor(args.vg, nvgRGBAf(0.62, 0.52, 0.75, 0.21));
            nvgMoveTo(args.vg, drawboxX / 5, drawboxY / 5);
            nvgLineTo(args.vg, 4 * drawboxX / 5, drawboxY / 5);
            nvgLineTo(args.vg, 4 * drawboxX / 5, 4 * drawboxY / 5);
            nvgLineTo(args.vg, drawboxX / 5, 4 * drawboxY / 5);
            nvgClosePath(args.vg);
            nvgStroke(args.vg);
            nvgFill(args.vg);


            nvgBeginPath(args.vg);
            nvgStrokeWidth(args.vg, 1.2);
            nvgStrokeColor(args.vg, nvgRGBAf(0.4, 0.4, 0.2, 0.4));
            nvgFillColor(args.vg, nvgRGBAf(0.53, 0.81, 0.92, 0.16));
            nvgMoveTo(args.vg, 1.7 * drawboxX / 5, 1.7 * drawboxY / 5);
            nvgLineTo(args.vg, 1.7 * drawboxX / 5, 3 * drawboxY / 5);
            nvgLineTo(args.vg, 3.3 * drawboxX / 5, 3 * drawboxY / 5);
            nvgLineTo(args.vg, 3.3 * drawboxX / 5, 1.7 * drawboxY / 5);
            nvgClosePath(args.vg);
            nvgStroke(args.vg);
            nvgFill(args.vg);

            nvgBeginPath(args.vg);
            nvgStrokeWidth(args.vg, 1.2);
            nvgStrokeColor(args.vg, nvgRGBAf(0.4, 0.4, 0.2, 0.4));
            nvgFillColor(args.vg, nvgRGBAf(0.53, 0.81, 0.92, 0.16));
            nvgMoveTo(args.vg, 4.7 * drawboxX / 5, 1.3 * drawboxY / 5);
            nvgLineTo(args.vg, 4.7 * drawboxX / 5, 3.25 * drawboxY / 5);
            nvgLineTo(args.vg, 4.3 * drawboxX / 5, 3 * drawboxY / 5);
            nvgLineTo(args.vg, 4.3 * drawboxX / 5, 1.7 * drawboxY / 5);
            nvgClosePath(args.vg);
            nvgStroke(args.vg);
            nvgFill(args.vg);
            

            if (Momeni->axis == 2) {
                addSpin = true;
            }
           //very slowly rock back and forth at different speeds along each axis
            if (frames % 4 == 0) {
                //how do i account for this window without all these damned IF's?
                if (spinX[0] <= 0) {
                    tiltback = false;
                }
                if (spinX[0] >= 360) {
                    tiltback = true;
                }
                if (spinY[0] <= 0) {
                    tiltbackY = false;
                }
                if (spinY[0] >= 360) {
                    tiltbackY = true;
                }
                if (spinZ[0] <= 0) {
                    tiltbackZ = false;
                }
                if (spinZ[0] >= 360) {
                    tiltbackZ = true;
                }
                if (spinW[0] <= 0) {
                    tiltbackW = false;
                }
                if (spinW[0] >= 360) {
                    tiltbackW = true;
                }
                if (tiltback) {
                    spinX -=  0.0025f;
                }
                else {
                    spinX +=  0.0025f;
                }              
                if (tiltbackY) {
                    spinY -= (addSpin) ? 0.005f : 0.0025f;
                }
                else {
                    spinY += (addSpin) ? 0.005f : 0.0025f;
                }
                if (tiltbackZ) {
                    spinZ -=  0.0055f;
                }
                else {
                    spinZ +=  0.0055f;
                }
                if (tiltbackW) {
                    spinW -= (addSpin) ? 0.0065f : 0.015f;
                }
                else {
                    spinW += (addSpin) ? 0.0065f : 0.015f;
                }

            }
            ++frames;
            frames %= 64;
  
           
            std::vector<rack::simd::float_4> Spawnp;
            Momeni->lines->peekSpawn(&Spawnp);
            std::vector<rack::simd::float_4> Momp;
            Momeni->lines->peekMom(&Momp);
            std::vector<rack::simd::float_4> Dadp;
            Momeni->lines->peekDad(&Dadp);
            //all three vecs will always be the same size, so only have to check with one.
            for (int s = 0; s < (int)Spawnp.size(); ++s) {
                //just a whole fuckton of matrix multiplication to draw these snake lines. really?
                rack::simd::float_4 Zero = 0;
                std::vector<rack::simd::float_4> rotationYZ = Matrix.RotationYZ(spinX);
                std::vector<rack::simd::float_4> rotationXZ = Matrix.RotationXZ(spinY);
                std::vector<rack::simd::float_4> rotationXY = Matrix.RotationXY(spinZ);
                std::vector<rack::simd::float_4> rotationZW = Matrix.RotationZW(spinW);
                std::vector<rack::simd::float_4> rotationYW = Matrix.RotationYW(spinW);

                std::vector<rack::simd::float_4> Wstyle = rotationZW;
                int member = 2;
                
                if (Momeni->axis == 1) {
                    Wstyle = rotationYW;
                    member = 1;
                }
                if (Momeni->axis == 2) {
                    Wstyle = rotationXY;
                    member = 0;
                }

                std::vector<rack::simd::float_4> LinesVec{  Spawnp[s], Momp[s], Dadp[s], Zero};
                std::vector<rack::simd::float_4> LinesVecprev{ Spawnp[abs(s - 1)], Momp[abs(s - 1)], Dadp[abs(s - 1)], Zero};

                std::vector<rack::simd::float_4> LinesRotate = Matrix.MatrixMult(rotationYZ, LinesVec);
                LinesRotate = Matrix.MatrixMult(rotationXZ, LinesRotate);
                LinesRotate = Matrix.MatrixMult(Wstyle, LinesRotate);

                std::vector<rack::simd::float_4> LinesRotateprev = Matrix.MatrixMult(rotationYZ, LinesVecprev);
                LinesRotateprev = Matrix.MatrixMult(rotationXZ, LinesRotateprev);
                LinesRotateprev = Matrix.MatrixMult(Wstyle, LinesRotateprev);

                    
                float distance = 1.05;
                float distconv = (Momeni->axis == 0) ? Functions.lerp(0, 1, -bound + 50, bound - 50, -Spawnp[s][2]) : Functions.lerp(0, 1, -bound + 50, bound - 50, LinesRotate[0][member]);
                float distconvprev = (Momeni->axis == 0) ? Functions.lerp(0, 1, -bound + 50, bound - 50, -Spawnp[abs(s - 1)][2]) : Functions.lerp(0, 1, -bound + 50, bound - 50, LinesRotateprev[0][member]);
                float Q = 1  / (distance - distconv);
                std::vector<rack::simd::float_4> projection = Matrix.Projection(Q);

                float E = 1  / (distance - distconvprev);
                std::vector<rack::simd::float_4> projectionprev = Matrix.Projection(E);


                std::vector<rack::simd::float_4> LinesProject = Matrix.MatrixMult(projection, LinesRotate);
                std::vector<rack::simd::float_4> LinesProjectprev = Matrix.MatrixMult(projection, LinesRotateprev);

                rack::simd::float_4 disp1 = LinesProject[0];
                rack::simd::float_4 disp2 = LinesProject[1];
                rack::simd::float_4 disp3 = LinesProject[2];
                rack::simd::float_4 disprev1 = LinesProjectprev[0];
                rack::simd::float_4 disprev2 = LinesProjectprev[1];
                rack::simd::float_4 disprev3 = LinesProjectprev[2];
                //change which pair gets displayed as the X and Y coordinates
                float line1screenX = disp1[0];
                float line1screenY = disp1[1];
                float line1screenXprev = disprev1[0];
                float line1screenYprev = disprev1[1];
                float line2screenX = disp2[0];
                float line2screenY = disp2[1];
                float line2screenXprev = disprev2[0];
                float line2screenYprev = disprev2[1];
                float line3screenX = disp3[0];
                float line3screenY = disp3[1];
                float line3screenXprev = disprev3[0];
                float line3screenYprev = disprev3[1];
                if (Momeni->axis == 1) {                    
                    line1screenY = disp1[2];
                    line2screenY = disp2[2];
                    line3screenY = disp3[2];
                    line1screenYprev = disprev1[2];
                    line2screenYprev = disprev2[2];
                    line3screenYprev = disprev3[2];                    
                }
                if (Momeni->axis == 2) {
                    line1screenX = disp1[1];
                    line1screenXprev = disprev1[1];
                    line2screenX = disp2[1];
                    line2screenXprev = disprev2[1];
                    line3screenX = disp3[1];
                    line3screenXprev = disprev3[1];
                    line1screenY = disp1[3];
                    line1screenYprev = disprev1[3];
                    line2screenY = disp2[3];
                    line2screenYprev = disprev2[3];
                    line3screenY = disp3[3];
                    line3screenYprev = disprev3[3];
                }
                float rotscreenX1 = Functions.lerp(0, drawboxX, -bound, bound, line1screenX);
                float rotscreenY1 = Functions.lerp(0, drawboxY, -bound, bound, -line1screenY);

                float rotscreenX2 = Functions.lerp(0, drawboxX, -bound, bound, line2screenX);
                float rotscreenY2 = Functions.lerp(0, drawboxY, -bound, bound, -line2screenY);

                float rotscreenX3 = Functions.lerp(0, drawboxX, -bound, bound, line3screenX);
                float rotscreenY3 = Functions.lerp(0, drawboxY, -bound, bound, -line3screenY);
               
                float rotscreenX1prev = Functions.lerp(0, drawboxX, -bound, bound, line1screenXprev);
                float rotscreenY1prev = Functions.lerp(0, drawboxY, -bound, bound, -line1screenYprev);

                float rotscreenX2prev = Functions.lerp(0, drawboxX, -bound, bound, line2screenXprev);
                float rotscreenY2prev = Functions.lerp(0, drawboxY, -bound, bound, -line2screenYprev);
            
                float rotscreenX3prev = Functions.lerp(0, drawboxX, -bound, bound, line3screenXprev);
                float rotscreenY3prev = Functions.lerp(0, drawboxY, -bound, bound, -line3screenYprev);
                
                
                
                float OP = distconv + 0.1 ;

                nvgBeginPath(args.vg);              
                nvgMoveTo(args.vg, rotscreenX1prev, rotscreenY1prev);
                nvgLineTo(args.vg, rotscreenX1, rotscreenY1);
                nvgStrokeWidth(args.vg, OP + 0.2 );
                nvgStrokeColor(args.vg, nvgRGBAf(0.0, 0.9, 1.0, OP));
                nvgStroke(args.vg);

                nvgBeginPath(args.vg);
                nvgMoveTo(args.vg, rotscreenX2prev, rotscreenY2prev);
                nvgLineTo(args.vg, rotscreenX2, rotscreenY2);
                nvgStrokeWidth(args.vg, OP + 0.2);
                nvgStrokeColor(args.vg, nvgRGBAf(0.8, 0.9, 0.0, OP));
                nvgStroke(args.vg);

                nvgBeginPath(args.vg);
                nvgMoveTo(args.vg, rotscreenX3prev, rotscreenY3prev);
                nvgLineTo(args.vg, rotscreenX3, rotscreenY3);
                nvgStrokeWidth(args.vg, OP + 0.2);
                nvgStrokeColor(args.vg, nvgRGBAf(0.8, 0.0, 0.9, OP));
                nvgStroke(args.vg);
                //draw little ellipses on the sneks heads
                if (s == (int)Spawnp.size() - 1) {
                    nvgBeginPath(args.vg);
                    nvgEllipse(args.vg, rotscreenX1, rotscreenY1, 2 + (abs(rotscreenX1 - rotscreenX1prev) / 2), 2 + (abs(rotscreenY1 - rotscreenY1prev) / 2));
                    nvgStrokeWidth(args.vg, OP + 0.2);
                    nvgStrokeColor(args.vg, nvgRGBAf(0.0, 0.9, 0.84, OP));
                    nvgStroke(args.vg);

                    nvgBeginPath(args.vg);
                    nvgEllipse(args.vg, rotscreenX2, rotscreenY2, 2 + (abs(rotscreenX2 - rotscreenX2prev) / 2), 2 + (abs(rotscreenY2 - rotscreenY2prev) / 2));
                    nvgStrokeWidth(args.vg, OP + 0.2);
                    nvgStrokeColor(args.vg, nvgRGBAf(0.84, 0.9, 0.0, OP));
                    nvgStroke(args.vg);

                    nvgBeginPath(args.vg);
                    nvgEllipse(args.vg, rotscreenX3, rotscreenY3, 2 + (abs(rotscreenX3 - rotscreenX3prev) / 2), 2 + (abs(rotscreenY3 - rotscreenY3prev) / 2));
                    nvgStrokeWidth(args.vg, OP + 0.2);
                    nvgStrokeColor(args.vg, nvgRGBAf(0.9, 0.0, 0.84, OP));
                    nvgStroke(args.vg);
                }
                
            }

        }
        Widget::drawLayer(args, layer);
    }
};

struct DadrasWidget : ModuleWidget {
    DadrasWidget(DadrasModule* module) {
        setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Dadras_panel.svg")));

		addChild(createWidget<ScrewSilver>(Vec(15, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 30, 0)));
		addChild(createWidget<ScrewSilver>(Vec(15, 365)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 30, 365)));

        addChild(createLightCentered<MediumLight<GreenRedLight>>(Vec(212, 74), module, DadrasModule::WTYPE_LIGHT));

        addParam(createParam<RoundLargeBlackKnob>(Vec(82, 158), module, DadrasModule::SPEED_PARAM));
        addParam(createParam<RoundBlackKnob>(Vec(162, 200), module, DadrasModule::SPREAD_PARAM));
        addParam(createParam<RoundBlackKnob>(Vec(205, 120), module, DadrasModule::SPEED_SHIFT_PARAM));
        addParam(createParam<RoundBlackKnob>(Vec(202, 200), module, DadrasModule::TIME_PHASE_PARAM));
        addParam(createParam<RoundSmallBlackKnob>(Vec(118, 205), module, DadrasModule::INFLUENCE_DAD_PARAM));
        addParam(createParam<RoundSmallBlackKnob>(Vec(81, 205), module, DadrasModule::INFLUENCE_MOM_PARAM));
        addParam(createParam<RoundSmallBlackKnob>(Vec(44, 156), module, DadrasModule::A_PARAM));
        addParam(createParam<RoundSmallBlackKnob>(Vec(44, 186), module, DadrasModule::B_PARAM));
        addParam(createParam<RoundSmallBlackKnob>(Vec(44, 216), module, DadrasModule::G_PARAM));
        addParam(createParam<RoundSmallBlackKnob>(Vec(44, 246), module, DadrasModule::O_PARAM));


        addParam(createParam<VCVButton>(Vec(11, 119.5), module, DadrasModule::SYNCH_BUTTON_PARAM));
        addParam(createParam<VCVButton>(Vec(11, 60.5), module, DadrasModule::RESET_BUTTON_PARAM));
        addParam(createParam<PurpleSwitch>(Vec(208, 32), module, DadrasModule::AXIS_SWITCH_PARAM));
        addParam(createParam<VCVButton>(Vec(210, 84), module, DadrasModule::WTYPE_BUTTON_PARAM));

        addInput(createInput<PurplePort>(Vec(130, 160), module, DadrasModule::SPEED_INPUT));
        addInput(createInput<PurplePort>(Vec(166, 242), module, DadrasModule::SPREAD_INPUT));
        addInput(createInput<PurplePort>(Vec(209, 162), module, DadrasModule::SPEED_SHIFT_INPUT));
        addInput(createInput<PurplePort>(Vec(204, 242), module, DadrasModule::TIME_PHASE_INPUT));
        addInput(createInput<PurplePort>(Vec(118, 242), module, DadrasModule::INFLUENCE_DAD_INPUT));
        addInput(createInput<PurplePort>(Vec(81, 242), module, DadrasModule::INFLUENCE_MOM_INPUT));
        addInput(createInput<PurplePort>(Vec(9, 90), module, DadrasModule::SYNCH_INPUT));
        addInput(createInput<PurplePort>(Vec(9, 31), module, DadrasModule::RESET_INPUT));
        addInput(createInput<PurplePort>(Vec(10, 156), module, DadrasModule::A_INPUT));
        addInput(createInput<PurplePort>(Vec(10, 186), module, DadrasModule::B_INPUT));
        addInput(createInput<PurplePort>(Vec(10, 216), module, DadrasModule::G_INPUT));
        addInput(createInput<PurplePort>(Vec(10, 246), module, DadrasModule::O_INPUT));

        addOutput(createOutput<PurplePort>(Vec(14, 282), module, DadrasModule::L_X_OUTPUT));
        addOutput(createOutput<PurplePort>(Vec(52, 282), module, DadrasModule::L_Y_OUTPUT));
        addOutput(createOutput<PurplePort>(Vec(14, 320), module, DadrasModule::L_Z_OUTPUT));
        addOutput(createOutput<PurplePort>(Vec(52, 320), module, DadrasModule::L_W_OUTPUT));
        addOutput(createOutput<PurplePort>(Vec(90, 282), module, DadrasModule::C_X_OUTPUT));
        addOutput(createOutput<PurplePort>(Vec(128, 282), module, DadrasModule::C_Y_OUTPUT));
        addOutput(createOutput<PurplePort>(Vec(90, 320), module, DadrasModule::C_Z_OUTPUT));
        addOutput(createOutput<PurplePort>(Vec(128, 320), module, DadrasModule::C_W_OUTPUT));
        addOutput(createOutput<PurplePort>(Vec(166, 282), module, DadrasModule::R_X_OUTPUT));
        addOutput(createOutput<PurplePort>(Vec(204, 282), module, DadrasModule::R_Y_OUTPUT));
        addOutput(createOutput<PurplePort>(Vec(166, 320), module, DadrasModule::R_Z_OUTPUT));
        addOutput(createOutput<PurplePort>(Vec(204, 320), module, DadrasModule::R_W_OUTPUT));
        
        if (module) {
            DadWidgetBuffer* DadBuffer = new DadWidgetBuffer(module);
            DadWidget* myWidget = new DadWidget(module, Vec(40, 31));
            myWidget->setSize(Vec(160, 120));
            DadBuffer->addChild(myWidget);
            addChild(DadBuffer);
        }

    }

};

Model* modelDadMom = createModel<DadrasModule, DadrasWidget>("Dadras-Momeni-Chaos");