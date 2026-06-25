#pragma once
#include "rack.hpp"


namespace LydD {
namespace Components {
    using namespace rack;
    extern Plugin* plugInst;

    //set by plugin.h(.c) itself during init
    void setPlugin(Plugin* p);

    void HSLtoRGB(float h, float s, float l, float* r, float* g, float* b);


    struct PurpleSwitch : app::SvgSwitch {
        PurpleSwitch();
    };
    
    struct PurplePort : app::SvgPort {
        PurplePort();
    };

    struct BigButton : app::SvgSwitch {
        BigButton();
    };

    struct LilButton : app::SvgSwitch {
        LilButton();
    };

    //createLight<ColorSVGLight<RedGreenBlueLight>>??
    // TEMPLATING THIS BROKE IT AND IDK WHY
    //template<typename TBase = RedGreenBlueLight>
    struct TColorSVGLight : TSvgLight<RedGreenBlueLight> {
        TColorSVGLight() {
            this->setSvg(Svg::load(asset::plugin(plugInst, "res/Logo.svg"))); //load logo by defult lol
        }
        void draw(const DrawArgs& args) override {}
        void drawLayer(const DrawArgs& args, int layer) override {
            if (layer == 1) {

                if (!sw->svg)
                    return;

                if (module) {

                    for (auto s = sw->svg->handle->shapes; s; s = s->next) {
                        s->fill.color = ((int)(color.a * 255) << 24) + (((int)(color.b * 255)) << 16) + (((int)(color.g * 255)) << 8) + (int)(color.r * 255);
                        s->fill.type = NSVG_PAINT_COLOR;
                    }

                    nvgGlobalCompositeBlendFunc(args.vg, NVG_ONE, NVG_ONE_MINUS_SRC_ALPHA);
                    svgDraw(args.vg, sw->svg->handle);
                }
            }
            Widget::drawLayer(args, layer);
        }
        void drawHalo(const DrawArgs& args) override {

           /* if (!sw->svg)
                return;

            if (module) {

                for (auto s = sw->svg->handle->shapes; s; s = s->next) {
                    s->fill.color = ((int)(255) << 24) + (((int)(color.b * 255)) << 16) + (((int)(color.g * 255)) << 8) + (int)(color.r * 255);
                    s->fill.type = NSVG_PAINT_COLOR;
                }

                nvgGlobalCompositeBlendFunc(args.vg, NVG_ONE_MINUS_DST_COLOR, NVG_ONE);
            }*/
        }
    };
    //using ColorSVGLight = TColorSVGLight<>;
}
}
