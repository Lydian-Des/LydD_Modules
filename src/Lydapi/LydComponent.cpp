#include "LydComponent.h"


namespace LydD {
namespace Components {
    using namespace rack;
    Plugin* plugInst;
    void setPlugin(Plugin* p) {
        plugInst = p;
    }
       
    void HSLtoRGB(float h, float s, float l, float* r, float* g, float* b)
    {
        if (h > 360.f) h -= 360.f;
        float c = (1 - abs(2.f * l - 1)) * s;
        float _h = h / 60.f;
        float x = c * (1 - abs(fmod(_h, 2) - 1));
        float m = l - (c / 2.f);
        float R1 = 0;
        float G1 = 0;
        float B1 = 0;
        if (_h >= 0 && _h < 1)
        {
            R1 = c;
            G1 = x;
            B1 = 0;
        }
        else if (_h >= 1 && _h < 2)
        {
            R1 = x;
            G1 = c;
            B1 = 0;
        }
        else if (_h >= 2 && _h < 3)
        {
            R1 = 0;
            G1 = c;
            B1 = x;
        }
        else if (_h >= 3 && _h < 4)
        {
            R1 = 0;
            G1 = x;
            B1 = c;
        }
        else if (_h >= 4 && _h < 5)
        {
            R1 = x;
            G1 = 0;
            B1 = c;
        }
        else if (_h >= 5 && _h < 6)
        {
            R1 = c;
            G1 = 0;
            B1 = x;
        }
        *r = (R1 + m) * 255;
        *g = (G1 + m) * 255;
        *b = (B1 + m) * 255;
    }

    PurpleSwitch::PurpleSwitch() {
            addFrame(Svg::load(asset::plugin(plugInst, "res/components/PurpleSwitch_0.svg")));
            addFrame(Svg::load(asset::plugin(plugInst, "res/components/PurpleSwitch_1.svg")));
            addFrame(Svg::load(asset::plugin(plugInst, "res/components/PurpleSwitch_2.svg")));
    }
               
    PurplePort::PurplePort() {
        setSvg(Svg::load(asset::plugin(plugInst, "res/components/PurpleJack23px.svg")));
    }
     
    BigButton::BigButton() {
        momentary = true;
        addFrame(Svg::load(asset::plugin(plugInst, "res/BigButton_0.svg")));
        addFrame(Svg::load(asset::plugin(plugInst, "res/BigButton_1.svg")));
    }

    LilButton::LilButton() {
        momentary = true;
        addFrame(Svg::load(asset::plugin(plugInst, "res/LilButton_0.svg")));
        addFrame(Svg::load(asset::plugin(plugInst, "res/LilButton_1.svg")));
    }

}
}
