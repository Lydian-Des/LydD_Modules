
#include "plugin.hpp"

#define MODULE_NAME DadrasModule
#define PANEL "Dadras_panel.svg"
#define HP 16

#define MAX_LINE 512
using namespace LydD;
using namespace LydD::Matrix;
static const int maxPolyphony = 1;


template<size_t MAX = 1024>
class Lines {
private:
    //each float_4 of each vect is a point: x, y, z, w
    
    rack::simd::float_4 Spawnline[MAX * 2];
    rack::simd::float_4 Momline[MAX * 2];
    rack::simd::float_4 Dadline[MAX * 2];
    size_t write = 0;
    size_t read = 0;
public:
    Lines() {
        this->empty();
    }
    size_t getHead() {
        return this->write;
    }
    size_t getRead() {
        return this->read;
    }
    void empty() {
        std::memset(Spawnline, 0, sizeof(rack::simd::float_4) * MAX * 2);
        std::memset(Momline, 0, sizeof(rack::simd::float_4) * MAX * 2);
        std::memset(Dadline, 0, sizeof(rack::simd::float_4) * MAX * 2);
        write = 0;
        read = 0;
    }
    void build(rack::simd::float_4 Spawnpush, rack::simd::float_4 Mompush, rack::simd::float_4 Dadpush) {
        Spawnline[write] = Spawnpush;
        Momline[write] = Mompush;
        Dadline[write] = Dadpush;
        Spawnline[write + MAX] = Spawnpush;
        Momline[write + MAX] = Mompush;
        Dadline[write + MAX] = Dadpush;
        ++write;
        write &= (MAX - 1);

    }

    void peekAll(rack::simd::float_4* linespawn, rack::simd::float_4* linemom, rack::simd::float_4* linedad) {
        std::memcpy(linespawn, &Spawnline[write], sizeof(rack::simd::float_4) * MAX);
        std::memcpy(linemom, &Momline[write], sizeof(rack::simd::float_4) * MAX);
        std::memcpy(linedad, &Dadline[write], sizeof(rack::simd::float_4) * MAX);
        ++read;
        read &= (MAX - 1);
    }

};

struct MomDadEq {

    //used to be bigger, independant math for each axis and so on. could just be a function now but i like the wrapper 
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
    MomDadEq Paths;
    Lines<MAX_LINE>* lines;


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
        EDIT_BUTTON_PARAM,
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

    //panel variable holder
    #include "Theme/PanelVars.h"

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
        configParam(EDIT_BUTTON_PARAM, 0.f, 1.f, 0.f, "EDIT - constants");

        configInput(SPEED_INPUT, "Speed");
        configInput(SPREAD_INPUT, "Separation");
        configInput(TIME_PHASE_INPUT, "Phase");
        configInput(SPEED_SHIFT_INPUT, "Difference");
        configInput(INFLUENCE_DAD_INPUT, "Dad's Influence");
        configInput(INFLUENCE_MOM_INPUT, "Mom's Influence");
        configInput(SYNCH_INPUT, "Sync Lines (To Child)");
        configInput(RESET_INPUT, "Reset");
        configInput(A_INPUT, "Force");
        configInput(B_INPUT, "Split");
        configInput(G_INPUT, "Dwell");
        configInput(O_INPUT, "Hold");

        configOutput(L_X_OUTPUT, "Mom-X");
        configOutput(L_Y_OUTPUT, "Mom-Y");
        configOutput(L_Z_OUTPUT, "Mom-Z");
        configOutput(L_W_OUTPUT, "Mom-W");
        configOutput(C_X_OUTPUT, "Spawn-X");
        configOutput(C_Y_OUTPUT, "Spawn-Y");
        configOutput(C_Z_OUTPUT, "Spawn-Z");
        configOutput(C_W_OUTPUT, "Spawn-W");
        configOutput(R_X_OUTPUT, "Dad-X");
        configOutput(R_Y_OUTPUT, "Dad-Y");
        configOutput(R_Z_OUTPUT, "Dad-Z");
        configOutput(R_W_OUTPUT, "Dad-W");

        lines = new(Lines<MAX_LINE>);


    #include "Theme/setDefaultInit.h"
    }
  
    ~DadrasModule() {
        if (lines) {
            delete lines;
        }
    }

    int loopCounter = 0;
    bool dirty = false;
    float bounds = 120;
    float runSpeed = 20.f;
    bool synchro = false;
    bool syn = false;
    bool reSet = false;
    bool reReset = false;
    bool WSet = false;
    bool WReset = false;
    //mostly stable 4 scroll starting positions
    float dt = 900;
    float a = 7.6;
    float b = 34.2;
    float g = 10.5;
    float o = 2.85;
    float startX = 11;
    float startY = 1;
    float startZ = -6;
    float startW = -1;

    //edit start positions
    bool editMode = false;
    bool editreset = false;
    rack::dsp::BooleanTrigger _modeSwitch;

    //initialize with base values
    rack::simd::float_4 Start{startX, startY, startZ, startW};
    rack::simd::float_4 Spread{ 0.f };
    rack::simd::float_4 CoordC = Start;
    rack::simd::float_4 CoordL = Start;
    rack::simd::float_4 CoordR = Start;

    float refFreq = 261.625565;
    rack::simd::float_4 Pitch{ refFreq };
    rack::simd::float_4 Phase{ 0.f };
    bool click1 = false;
    bool click2 = false;
    bool click3 = false;
    int axis = 0;
    bool axbut = false;
    
    float phaseShift = 0.f;
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
        

        //each phase ticks one time step per cycle
        if (!editMode) {
            generateOutput(args);
            incrementPhase(Pitch, args.sampleRate, &Phase, rack::simd::float_4(_2_PI));          
        }
        else {
            setEditStart(args);
        }
    }

   

    void checkInputs(const ProcessArgs& args) {
        for (int o = L_X_OUTPUT; o != NUM_OUTPUTS; ++o) {
            outputs[o].setChannels(1);
        }

        axis = (int)params[AXIS_SWITCH_PARAM].value;

        
        if (inputs[RESET_INPUT].isConnected()) {
            momentButton(inputs[RESET_INPUT].getVoltage(0), &reSet, &reReset);
        }
        else {
            momentButton(params[RESET_BUTTON_PARAM].value, &reSet, &reReset);
        }

        
        if (inputs[SYNCH_INPUT].isConnected()) {
            momentButton(inputs[SYNCH_INPUT].getVoltage(0), &synchro, &syn);
        }
        else {
            momentButton(params[SYNCH_BUTTON_PARAM].value, &synchro, &syn);
        }

        latchButton(params[WTYPE_BUTTON_PARAM].value, &WSet, &WReset);
        lights[WTYPE_LIGHT + 0].setBrightness(!WSet);
        lights[WTYPE_LIGHT + 1].setBrightness(WSet);
        if (WSet) {
            paramQuantities[WTYPE_BUTTON_PARAM]->name = "Scratch";
        }
        else {
            paramQuantities[WTYPE_BUTTON_PARAM]->name = "Wander";
        }

        float A = params[A_PARAM].value;
        float B = params[B_PARAM].value;
        float G = params[G_PARAM].value;
        float O = params[O_PARAM].value;
        if (inputs[A_INPUT].isConnected()) {
            A = rack::math::clamp(A + inputs[A_INPUT].getVoltage(0), -5.f, 5.f);
        }
        if (inputs[B_INPUT].isConnected()) {
            B = rack::math::clamp(B + inputs[B_INPUT].getVoltage(0), -5.f, 5.f);
        }
        if (inputs[G_INPUT].isConnected()) {
            G = rack::math::clamp(G + inputs[G_INPUT].getVoltage(0), -5.f, 5.f);
        }
        if (inputs[O_INPUT].isConnected()) {
            O = rack::math::clamp(O + inputs[O_INPUT].getVoltage(0), -5.f, 5.f);
        }
        a = lerp(5.f, 9.f, -5.f, 5.f, A);
        b = lerp(22.f, 45.f, -5.f, 5.f, B);
        g = lerp(6.f, 12.f, -5.f, 5.f, G);
        o = lerp(1.f, 4.f, -5.f, 5.f, O);

        float parSpeed = (params[SPEED_PARAM].value);
        float inSpeed = (inputs[SPEED_INPUT].isConnected()) ? inputs[SPEED_INPUT].getVoltage(0) : 0.f;
        float parshiftSpeed = (params[SPEED_SHIFT_PARAM].value);
        float inshiftSpeed = (inputs[SPEED_SHIFT_INPUT].isConnected()) ? inputs[SPEED_SHIFT_INPUT].getVoltage(0) : 0.f;        
        float basePitch = VoltToFreq(parSpeed + inSpeed, 0.0, refFreq);
        float PitchL = VoltToFreq((parSpeed + inSpeed) - (parshiftSpeed + inshiftSpeed), 0.0, refFreq);
        float PitchR = VoltToFreq((parSpeed + inSpeed) + (parshiftSpeed + inshiftSpeed), 0.0, refFreq);
        Pitch = rack::simd::float_4{ basePitch, PitchL, PitchR, 0.f };

        float parSpread = params[SPREAD_PARAM].value;
        float inSpread = (inputs[SPREAD_INPUT].isConnected()) ? inputs[SPREAD_INPUT].getVoltage(0) : 0.f;
        float spreadX = (parSpread + inSpread) / 500.f;
        float spreadY = (parSpread + inSpread) / 200.f;
        float spreadZ = (parSpread - inSpread) / 400.f;        
        Spread = rack::simd::float_4{ spreadX, spreadY, spreadZ, 0.f };

        float parphaseShift = params[TIME_PHASE_PARAM].value;
        float inphaseShift = (inputs[TIME_PHASE_INPUT].isConnected()) ? inputs[TIME_PHASE_INPUT].getVoltage(0) : 0.f;
        phaseShift = rack::math::clamp(parphaseShift + inphaseShift, -3.14, 3.14);

        float indad = (inputs[INFLUENCE_DAD_INPUT].isConnected()) ? lerp(0.f, 1.f, 0.f, 5.f, abs(inputs[INFLUENCE_DAD_INPUT].getVoltage(0))) : 0.f;
        float inmom = (inputs[INFLUENCE_MOM_INPUT].isConnected()) ? lerp(0.f, 1.f, 0.f, 5.f, abs(inputs[INFLUENCE_MOM_INPUT].getVoltage(0))) : 0.f;
        dadsInf = rack::math::clamp(params[INFLUENCE_DAD_PARAM].value + indad, 0.f, 1.f);
        momsInf = rack::math::clamp(params[INFLUENCE_MOM_PARAM].value + inmom, 0.f, 1.f);
        
        latchButton(params[EDIT_BUTTON_PARAM].value, &editMode, &editreset);
        if (_modeSwitch.process(editMode)) {
            resetspace();
        }
        if (editMode) {
            paramQuantities[A_PARAM]->name = "Edit - X";
            paramQuantities[B_PARAM]->name = "Edit - Y";
            paramQuantities[G_PARAM]->name = "Edit - Z";
            paramQuantities[O_PARAM]->name = "Edit - W";
        } 
        else {
            paramQuantities[A_PARAM]->name = "Force";
            paramQuantities[B_PARAM]->name = "Split";
            paramQuantities[G_PARAM]->name = "Dwell";
            paramQuantities[O_PARAM]->name = "Throw";
        }
    }
    void resetspace() {
        CoordC = Start;
        CoordL = Start + Spread;
        CoordR = Start - Spread;
        lines->empty();
    }
    void generateOutput(const ProcessArgs& args) {   
        float dtL = dt + (phaseShift * 100);
        float dtR = dt - (phaseShift * 100);
           
        if (synchro) {
            CoordL = CoordC;
            CoordR = CoordC;
            lines->empty();
        }
        if (reSet) {
            resetspace();
        }

        for (int p = 0; p < 4; ++p) {
            if (CoordC[p] > bounds) {
                CoordC = Start;
            }
            if (CoordL[p] > bounds) {
                CoordL = Start;
            }
            if (CoordR[p] > bounds) {
                CoordR = Start;
            }
        }
        
        //spread functions as continuous offset rather than initial conditions
        rack::simd::float_4 CoordprevC = CoordC;
        rack::simd::float_4 CoordprevL = CoordL + Spread;
        rack::simd::float_4 CoordprevR = CoordR - Spread;

        if (Phase[0] <= _PI) {
            click1 = true;
        }
        if (Phase[1] <= _PI) {
            click2 = true;
        }
        if (Phase[2] <= _PI) {
            click3 = true;
        }
        if (Phase[0] > _PI && click1) {
            lines->build(CoordC, CoordL, CoordR);
            //pull toward mom or dad w/ influence
            CoordprevC += incrementToward(CoordprevC, CoordL, 100) * momsInf;
            CoordprevC += incrementToward(CoordprevC, CoordR, 100) * dadsInf;
            CoordC = Paths.MOMDAD(a, b, g, o, dt, CoordprevC, WSet);
            click1 = false;
        }
        if (Phase[1] > _PI && click2) {
            CoordL = Paths.MOMDAD(a, b, g, o, dtL, CoordprevL, WSet);
            click2 = false;
        }
        if (Phase[2] > _PI && click3) {
            CoordR = Paths.MOMDAD(a, b, g, o, dtR, CoordprevR, WSet);
            click3 = false;
        }
        float outputC[4] = { 0 };
        float outputL[4] = { 0 };
        float outputR[4] = { 0 };
        for (int i = 0; i < 4; ++i) {
            outputC[i] = lerp(-8.f, 8.f, -bounds, bounds, CoordC[i]);
            outputC[i] = rack::math::clamp(outputC[i], -8.f, 8.f);
            outputL[i] = lerp(-8.f, 8.f, -bounds, bounds, CoordL[i]);
            outputL[i] = rack::math::clamp(outputL[i], -8.f, 8.f);
            outputR[i] = lerp(-8.f, 8.f, -bounds, bounds, CoordR[i]);
            outputR[i] = rack::math::clamp(outputR[i], -8.f, 8.f);
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
    void setEditStart(const ProcessArgs& args) {
        Start[0] = startX + params[A_PARAM].value;
        Start[1] = startY + params[B_PARAM].value;
        Start[2] = startZ + params[G_PARAM].value;
        Start[3] = startW + params[O_PARAM].value;
    }

    json_t* dataToJson() override {
        json_t* rootJ = json_object();
        json_t* panelJ = json_integer(currPanel);
        json_object_set_new(rootJ, "Panel", panelJ);
        json_t* Start1J = json_real(Start[0]);
        json_t* Start2J = json_real(Start[1]);
        json_t* Start3J = json_real(Start[2]);
        json_t* Start4J = json_real(Start[3]);
        json_object_set_new(rootJ, "start1", Start1J);
        json_object_set_new(rootJ, "start2", Start2J);
        json_object_set_new(rootJ, "start3", Start3J);
        json_object_set_new(rootJ, "start4", Start4J);

        return rootJ;
    }

    void dataFromJson(json_t* rootJ) override {
        json_t* panelJ = json_object_get(rootJ, "Panel");
        if (panelJ) currPanel = json_integer_value(panelJ);
        json_t* start1J = json_object_get(rootJ, "start1");
        json_t* start2J = json_object_get(rootJ, "start2");
        json_t* start3J = json_object_get(rootJ, "start3");
        json_t* start4J = json_object_get(rootJ, "start4");
        if (start1J && start2J && start3J && start4J) {
            Start = rack::simd::float_4(json_real_value(start1J), json_real_value(start2J),
                json_real_value(start3J), json_real_value(start4J));
        }

    }

};


struct DadWidgetBuffer : FramebufferWidget {
    DadrasModule* Momeni;
    DadWidgetBuffer(DadrasModule* m) {
        Momeni = m;
    }
    //this kind of dirty control has never worked for me. if needed i just count loops in the draw code(just poppy rn)
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

    float drawboxX;
    float drawboxY;
    float bound;
    bool editMode;

    rack::simd::float_4 spinX{ 60 };
    rack::simd::float_4 spinY{ 30 };
    rack::simd::float_4 spinZ{ 20 };
    rack::simd::float_4 spinW{ 0 };
    rack::simd::float_4 circle{ 360 };
    std::vector<rack::simd::float_4> rotationYZ;
    std::vector<rack::simd::float_4> rotationXZ;
    std::vector<rack::simd::float_4> rotationXY;
    std::vector<rack::simd::float_4> rotationZW;
    std::vector<rack::simd::float_4> rotationYW;
    std::vector<rack::simd::float_4> Wstyle;
    int depthDim;
    bool addSpin = false;
    int frames = 0;
    rack::simd::float_4 Zero{ 0 };

    Vec DrawSpawn[MAX_LINE];
    Vec DrawMom[MAX_LINE];
    Vec DrawDad[MAX_LINE];
    float Opacity[MAX_LINE];
    
    void drawRoom(const DrawArgs& args, float boxX, float boxY) {
        //drawing a little room for the snakes to live in
        nvgStrokeWidth(args.vg, 1.2);
        nvgStrokeColor(args.vg, nvgRGBAf(0.4, 0.4, 0.2, 0.4));
        nvgFillColor(args.vg, nvgRGBAf(0.68, 0.57, 0.91, 0.21));
        nvgBeginPath(args.vg);
        nvgMoveTo(args.vg, 0, 0);
        nvgLineTo(args.vg, boxX / 5, boxY / 5);
        nvgLineTo(args.vg, boxX / 5, 4 * boxY / 5);
        nvgLineTo(args.vg, 0, boxY);
        nvgClosePath(args.vg);
        nvgStroke(args.vg);
        nvgFill(args.vg);

        nvgBeginPath(args.vg);
        nvgMoveTo(args.vg, 0, 0);
        nvgLineTo(args.vg, boxX / 5, boxY / 5);
        nvgLineTo(args.vg, 4 * boxX / 5, boxY / 5);
        nvgLineTo(args.vg, boxX, 0);
        nvgClosePath(args.vg);
        nvgStroke(args.vg);
        nvgFill(args.vg);

        nvgBeginPath(args.vg);
        nvgMoveTo(args.vg, boxX, 0);
        nvgLineTo(args.vg, 4 * boxX / 5, boxY / 5);
        nvgLineTo(args.vg, 4 * boxX / 5, 4 * boxY / 5);
        nvgLineTo(args.vg, boxX, boxY);
        nvgClosePath(args.vg);
        nvgStroke(args.vg);
        nvgFill(args.vg);

        nvgBeginPath(args.vg);
        nvgMoveTo(args.vg, boxX, boxY);
        nvgLineTo(args.vg, 4 * boxX / 5, 4 * boxY / 5);
        nvgLineTo(args.vg, boxX / 5, 4 * boxY / 5);
        nvgLineTo(args.vg, 0, boxY);
        nvgClosePath(args.vg);
        nvgStroke(args.vg);
        nvgFill(args.vg);

        nvgBeginPath(args.vg);
        nvgFillColor(args.vg, nvgRGBAf(0.62, 0.52, 0.75, 0.21));
        nvgMoveTo(args.vg, boxX / 5, boxY / 5);
        nvgLineTo(args.vg, 4 * boxX / 5, boxY / 5);
        nvgLineTo(args.vg, 4 * boxX / 5, 4 * boxY / 5);
        nvgLineTo(args.vg, boxX / 5, 4 * boxY / 5);
        nvgClosePath(args.vg);
        nvgStroke(args.vg);
        nvgFill(args.vg);


        nvgBeginPath(args.vg);
        nvgStrokeWidth(args.vg, 1.2);
        nvgStrokeColor(args.vg, nvgRGBAf(0.4, 0.4, 0.2, 0.4));
        nvgFillColor(args.vg, nvgRGBAf(0.53, 0.81, 0.92, 0.16));
        nvgMoveTo(args.vg, 1.7 * boxX / 5, 1.7 * boxY / 5);
        nvgLineTo(args.vg, 1.7 * boxX / 5, 3 * boxY / 5);
        nvgLineTo(args.vg, 3.3 * boxX / 5, 3 * boxY / 5);
        nvgLineTo(args.vg, 3.3 * boxX / 5, 1.7 * boxY / 5);
        nvgClosePath(args.vg);
        nvgStroke(args.vg);
        nvgFill(args.vg);

        nvgBeginPath(args.vg);
        nvgStrokeWidth(args.vg, 1.2);
        nvgStrokeColor(args.vg, nvgRGBAf(0.4, 0.4, 0.2, 0.4));
        nvgFillColor(args.vg, nvgRGBAf(0.53, 0.81, 0.92, 0.16));
        nvgMoveTo(args.vg, 4.7 * boxX / 5, 1.3 * boxY / 5);
        nvgLineTo(args.vg, 4.7 * boxX / 5, 3.25 * boxY / 5);
        nvgLineTo(args.vg, 4.3 * boxX / 5, 3 * boxY / 5);
        nvgLineTo(args.vg, 4.3 * boxX / 5, 1.7 * boxY / 5);
        nvgClosePath(args.vg);
        nvgStroke(args.vg);
        nvgFill(args.vg);

    }

    void drawPolyLine(const DrawArgs& args, Vec* line, float* opacity, float color[3], int size) {
        nvgBeginPath(args.vg);
        
        nvgStrokeWidth(args.vg, opacity[0] + 0.2f);
        nvgStrokeColor(args.vg, nvgRGBAf(color[0], color[1], color[2], opacity[0]));
        nvgMoveTo(args.vg, line[0].x, line[0].y);
        for (int i = 1; i < size; ++i) {                      
            nvgLineTo(args.vg, line[i].x, line[i].y);
        }
        nvgStroke(args.vg);
        nvgClosePath(args.vg);
    }

    void step() override {
        editMode = Momeni->editMode;
        drawboxX = box.size.x;
        drawboxY = box.size.y;
        bound = Momeni->bounds;

        rotationYZ = RotationYZ(spinX);
        rotationXZ = RotationXZ(spinY);
        rotationXY = RotationXY(spinZ);
        rotationZW = RotationZW(spinW);
        rotationYW = RotationYW(spinW);
        Wstyle = rotationXZ;

        if (!editMode) {
            rack::simd::float_4 Spawnp[MAX_LINE]{ 0 };
            rack::simd::float_4 Momp[MAX_LINE]{ 0 };
            rack::simd::float_4 Dadp[MAX_LINE]{ 0 };
            if (Momeni->lines) Momeni->lines->peekAll(Spawnp, Momp, Dadp);

            depthDim = 3;

            if (Momeni->axis == 1) {
                Wstyle = rotationYW;
                depthDim = 1;
            }
            if (Momeni->axis == 2) {
                Wstyle = rotationXY;
                depthDim = 2;
            }

            
            for (int s = 0; (s < MAX_LINE); ++s) {

                //just a whole fuckton of matrix multiplication to draw these snake lines. really?
                std::vector<rack::simd::float_4> LinesVec{ Spawnp[s], Momp[s], Dadp[s], Zero };
                std::vector<rack::simd::float_4> LinesRotate = MatrixMult(rotationYZ, LinesVec);
                // LinesRotate = MatrixMult(rotationXZ, LinesRotate);
                LinesRotate = MatrixMult(Wstyle, LinesRotate);

                float distance = 1.55;
                float distconv = lerp(0.f, 1.f, -bound + 50.f, bound - 50.f, -Spawnp[s][depthDim]);
                float Q = 1.f / (distance - distconv);
                std::vector<rack::simd::float_4> projection = Projection(Q);

                std::vector<rack::simd::float_4> LinesProject = MatrixMult(projection, LinesRotate);

                rack::simd::float_4 disp1 = LinesProject[0];
                rack::simd::float_4 disp2 = LinesProject[1];
                rack::simd::float_4 disp3 = LinesProject[2];

                //change which pair gets displayed as the X and Y coordinates
                float line1screenX = disp1[0];
                float line1screenY = disp1[1];

                float line2screenX = disp2[0];
                float line2screenY = disp2[1];

                float line3screenX = disp3[0];
                float line3screenY = disp3[1];

                if (Momeni->axis == 1) {
                    line1screenY = disp1[2];
                    line2screenY = disp2[2];
                    line3screenY = disp3[2];

                }
                if (Momeni->axis == 2) {
                    line1screenX = disp1[1];
                    line2screenX = disp2[1];
                    line3screenX = disp3[1];
                    line1screenY = disp1[3];
                    line2screenY = disp2[3];
                    line3screenY = disp3[3];
                }
                //flip Y for inverse screen coordinates
                DrawSpawn[s].x = lerp(0.f, drawboxX, -bound, bound, line1screenX);
                DrawSpawn[s].y = lerp(0.f, drawboxY, -bound, bound, -line1screenY);

                DrawMom[s].x = lerp(0.f, drawboxX, -bound, bound, line2screenX);
                DrawMom[s].y = lerp(0.f, drawboxY, -bound, bound, -line2screenY);

                DrawDad[s].x = lerp(0.f, drawboxX, -bound, bound, line3screenX);
                DrawDad[s].y = lerp(0.f, drawboxY, -bound, bound, -line3screenY);

                Opacity[s] = distconv + 0.1;
            }
        }
        Widget::step();
    }

    void drawLayer(const DrawArgs& args, int layer) override {
        if (layer == 1) {

            
            
                   
            nvgScissor(args.vg, 0, 0, drawboxX, drawboxY);

           // drawRoom(args,drawboxX, drawboxY);
  
            if (Momeni->axis == 2) {
                addSpin = true;
            }
            //very slowly spin at different speeds along each axis
            if (frames % 4 == 0) {
                spinX += 0.015f;
                spinY -= (addSpin) ? 0.005f : 0.015f;
                spinZ += 0.015f;
                spinW -= (addSpin) ? 0.0065f : 0.015f;
                spinX = wrapFree(spinX, rack::simd::float_4(0.f), rack::simd::float_4(360.f));
                spinY = wrapFree(spinY, rack::simd::float_4(0.f), rack::simd::float_4(360.f));
                spinZ = wrapFree(spinZ, rack::simd::float_4(0.f), rack::simd::float_4(360.f));
                spinW = wrapFree(spinW, rack::simd::float_4(0.f), rack::simd::float_4(360.f));
            }
            ++frames;
            frames %= 64;


            

            if (!editMode) {

               
                //colors for lines
                float scol[3] = { 0.0, 0.9, 0.84 };
                float mcol[3] = { 0.84, 0.9, 0.0 };
                float dcol[3] = { 0.9, 0.0, 0.84 };
                //forgo a couple niceties for less path calls and context switches
                drawPolyLine(args, DrawSpawn, Opacity, scol, MAX_LINE);
                drawPolyLine(args, DrawMom, Opacity, mcol, MAX_LINE);
                drawPolyLine(args, DrawDad, Opacity, dcol, MAX_LINE);
                    //draw little ellipses on the sneks heads
 
                    nvgBeginPath(args.vg);
                    nvgEllipse(args.vg, DrawSpawn[MAX_LINE - 1].x, DrawSpawn[MAX_LINE - 1].y, 4, 4);
                    nvgStrokeWidth(args.vg, Opacity[MAX_LINE - 1] + 0.2);
                    nvgStrokeColor(args.vg, nvgRGBAf(0.0, 0.9, 0.84, Opacity[MAX_LINE - 1]));
                    nvgStroke(args.vg);

                    nvgBeginPath(args.vg);
                    nvgEllipse(args.vg, DrawMom[MAX_LINE - 1].x, DrawMom[MAX_LINE - 1].y, 2 + (abs(DrawMom[0].x - DrawMom[1].x) / 2), 2 + (abs(DrawMom[0].y - DrawMom[1].y) / 2));
                    nvgStrokeWidth(args.vg, Opacity[MAX_LINE - 1] + 0.2);
                    nvgStrokeColor(args.vg, nvgRGBAf(0.84, 0.9, 0.0, Opacity[MAX_LINE - 1]));
                    nvgStroke(args.vg);

                    nvgBeginPath(args.vg);
                    nvgEllipse(args.vg, DrawDad[MAX_LINE - 1].x, DrawDad[MAX_LINE - 1].y, 2 + (abs(DrawDad[0].x - DrawDad[1].x) / 2), 2 + (abs(DrawDad[0].y - DrawDad[1].y) / 2));
                    nvgStrokeWidth(args.vg, Opacity[MAX_LINE - 1] + 0.2);
                    nvgStrokeColor(args.vg, nvgRGBAf(0.9, 0.0, 0.84, Opacity[MAX_LINE - 1]));
                    nvgStroke(args.vg);
                    
                    
                
            }
            else {
                rack::simd::float_4 kidCoord = Momeni->Start;
                rack::simd::float_4 momCoord = Momeni->Start + Momeni->Spread * 300.f;
                rack::simd::float_4 dadCoord = Momeni->Start - Momeni->Spread * 300.f;

                std::vector<rack::simd::float_4> dotsVec{ kidCoord, momCoord, dadCoord, Zero };

                std::vector<rack::simd::float_4> dotsRotate = MatrixMult(rotationYZ, dotsVec);
                dotsRotate = MatrixMult(rotationXZ, dotsRotate);
                dotsRotate = MatrixMult(Wstyle, dotsRotate);
                float distance = 1.05;
                float distconv = lerp(0.f, 1.f, -60.f, 60.f, -kidCoord[depthDim]);
                float Q = 1.f / (distance - distconv);
                std::vector<rack::simd::float_4> projection = Projection(Q);
                std::vector<rack::simd::float_4> dotsProject = MatrixMult(projection, dotsRotate);

                rack::simd::float_4 kidproj = lerp(Zero, rack::simd::float_4(drawboxY),
                    rack::simd::float_4(-60), rack::simd::float_4(60), dotsProject[0]);
                rack::simd::float_4 momproj = lerp(Zero, rack::simd::float_4(drawboxY),
                    rack::simd::float_4(-60), rack::simd::float_4(60), dotsProject[1]);
                rack::simd::float_4 dadproj = lerp(Zero, rack::simd::float_4(drawboxY),
                    rack::simd::float_4(-60), rack::simd::float_4(60), dotsProject[2]);

                nvgBeginPath(args.vg);
                nvgEllipse(args.vg, kidproj[0], kidproj[1], 4.f + Q, 4.f + Q);
                nvgStrokeWidth(args.vg, 0.4f);
                nvgStrokeColor(args.vg, nvgRGBAf(0.0f, 0.9f, 0.84f, 1.f));
                nvgStroke(args.vg);

                nvgBeginPath(args.vg);
                nvgEllipse(args.vg, momproj[0], momproj[1], 4.f + Q, 4.f + Q);
                nvgStrokeWidth(args.vg, 0.4f);
                nvgStrokeColor(args.vg, nvgRGBAf(0.84f, 0.9f, 0.0f, 1.f));
                nvgStroke(args.vg);

                nvgBeginPath(args.vg);
                nvgEllipse(args.vg, dadproj[0], dadproj[1], 4.f + Q, 4.f + Q);
                nvgStrokeWidth(args.vg, 0.4f);
                nvgStrokeColor(args.vg, nvgRGBAf(0.9f, 0.0f, 0.84f, 1.f));
                nvgStroke(args.vg);

                //draw the edit triangle
                nvgBeginPath(args.vg);
                nvgFillColor(args.vg, nvgRGBAf(0.6f, 0.6f, 0.1f, 1.f));
                nvgMoveTo(args.vg, 5.f, 115.f);
                nvgLineTo(args.vg, 20.f, 115.f);
                nvgLineTo(args.vg, 12.5f, 102.f);
                nvgClosePath(args.vg);
                nvgFill(args.vg);
            }
        }
        Widget::drawLayer(args, layer);
    }
};
using namespace LydD::Components;
struct DadrasWidget : ModuleWidget {
    #include "Theme/LogoLight.h"
    //name for panel file, same name for every type
    std::string panel;

    DadrasWidget(DadrasModule* module) {
        setModule(module);

        panel = PANEL;
        //set panel on init
        #include "Theme/initChoosePanel.h"

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
        addParam(createParam<VCVButton>(Vec(165, 162), module, DadrasModule::EDIT_BUTTON_PARAM));

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
            //must be called 'logoPos'for all modules
            Vec logoPos = Vec(((15.f * HP) / 2.f) - 12.5, 363.f);
            DadrasModule* module = dynamic_cast<DadrasModule*>(this->module);
            assert(module);
            #include "Theme/LogoChild.h"           
        }

    }

    //give struct to menu containing panel options
    #include "Theme/PanelList.h" 

    void appendContextMenu(Menu* menu) override {
        DadrasModule* module = dynamic_cast<DadrasModule*>(this->module);
        assert(module);

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

Model* modelDadMom = createModel<DadrasModule, DadrasWidget>("Dadras-Momeni-Chaos");