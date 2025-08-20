#include "rack.hpp"
#include "BasicFunctions.h"
using namespace rack;

extern Plugin* pluginInstance;
struct PurpleSwitch : app::SvgSwitch {
    PurpleSwitch() {
        addFrame(Svg::load(asset::plugin(pluginInstance, "res/PurpleSwitch_0.svg")));
        addFrame(Svg::load(asset::plugin(pluginInstance, "res/PurpleSwitch_1.svg")));
        addFrame(Svg::load(asset::plugin(pluginInstance, "res/PurpleSwitch_2.svg")));
    }
};

struct PurplePort : app::SvgPort {
    PurplePort() {
        setSvg(Svg::load(asset::plugin(pluginInstance, "res/PurpleJack23px.svg")));
    }
};

extern Model* modelPoppy;
extern Model* modelDadMom;
extern Model* modelSimone;
extern Model* modelTorus;
