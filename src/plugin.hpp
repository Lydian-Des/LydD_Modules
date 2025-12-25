#include "rack.hpp"
#include "BasicFunctions.h"
using namespace rack;

extern Plugin* pluginInstance;
struct PurpleSwitch : app::SvgSwitch {
    PurpleSwitch() {
        addFrame(Svg::load(asset::plugin(pluginInstance, "res/components/PurpleSwitch_0.svg")));
        addFrame(Svg::load(asset::plugin(pluginInstance, "res/components/PurpleSwitch_1.svg")));
        addFrame(Svg::load(asset::plugin(pluginInstance, "res/components/PurpleSwitch_2.svg")));
    }
};

struct PurplePort : app::SvgPort {
    PurplePort() {
        setSvg(Svg::load(asset::plugin(pluginInstance, "res/components/PurpleJack23px.svg")));
    }
};

/*Taken straight from VCV Fundamental, for segment text/numerical display*/
struct DigitalDisplay : Widget {
    std::string fontPath;
    std::string bgText;
    std::string text;
    float fontSize;
    NVGcolor bgColor = nvgRGB(0x46, 0x46, 0x46);
    NVGcolor fgColor = rack::color::CYAN;
    Vec textPos;

    void prepareFont(const DrawArgs& args) {
        // Get font
        std::shared_ptr<Font> font = APP->window->loadFont(fontPath);
        if (!font)
            return;
        nvgFontFaceId(args.vg, font->handle);
        nvgFontSize(args.vg, fontSize);
        nvgTextLetterSpacing(args.vg, 0.0);
        nvgTextAlign(args.vg, NVG_ALIGN_RIGHT);
    }

    void draw(const DrawArgs& args) override {
        // Background
        nvgBeginPath(args.vg);
        nvgRoundedRect(args.vg, 0, 0, box.size.x, box.size.y, 2);
        nvgFillColor(args.vg, nvgRGB(0x19, 0x19, 0x19));
        nvgFill(args.vg);

        prepareFont(args);

        // Background text
        nvgFillColor(args.vg, bgColor);
        nvgText(args.vg, textPos.x, textPos.y, bgText.c_str(), NULL);
    }

    void drawLayer(const DrawArgs& args, int layer) override {
        if (layer == 1) {
            prepareFont(args);

            // Foreground text
            nvgFillColor(args.vg, fgColor);
            nvgText(args.vg, textPos.x, textPos.y, text.c_str(), NULL);
        }
        Widget::drawLayer(args, layer);
    }
};

extern Model* modelPoppy;
extern Model* modelDadMom;
extern Model* modelSimone;
extern Model* modelTorus;
extern Model* modelClock;
extern Model* modelDobbs;
extern Model* modelLedger;
extern Model* modelOnceler;
extern Model* modelShear;