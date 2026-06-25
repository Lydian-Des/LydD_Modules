#include "LydEQ.h"


namespace LydD {
namespace Filter {
    //atttempts at finding 4 main bands of Freq, Res, and Gain for various materials
    //if I end up actually using this it'll get more detailed
    float WoodF[4] = { 2000.f, 455.f, 760.f, 200.f };
    float WoodQ[4] = { 0.1f, 0.1f, 0.1f, 0.1f };
    float WoodG[4] = { 1, 1, 1, 1 };
    float SteelF[4] = { 1900.f, 975.f, 1250.f, 300.f };
    float SteelQ[4] = { 1.4f, .2f, .3f, .42f };
    float SteelG[4] = { 0.87f, .87f, .8f, 0.86f };
    float BoardF[4] = { 3000.f, 1975.f, 2250.f, 700.f };
    float BoardQ[4] = { 1.4f, .82f, .62f, 1.12f };
    float BoardG[4] = { .87f, 0.87f, .68f, 0.56f };

}
}
