#include "yuvColor.h"

namespace corecvs {


#ifdef REFLECTION_IN_CORE
Reflection YUVColor::reflect = YUVColor::staticInit();
#endif

/*
RGBColor RGBColor::rainbow(double x)
{
    x *= 6;
    if (x < 0.0) x = 0.0;
    if (x > 6.0) x = 6.0;

    switch ((int)x)
    {
        case 0:
            return lerpColor(   Red(), Orange(), x);
        case 1:
            return lerpColor(Orange(), Yellow(), x - 1);
        case 2:
            return lerpColor(Yellow(),  Green(), x - 2);
        case 3:
            return lerpColor( Green(),   Blue(), x - 3);
        case 4:
            return lerpColor(  Blue(), Indigo(), x - 4);
        case 5:
        default:
            return lerpColor(Indigo(), Violet(), x - 5);
    }

} */

} // namespace corecvs
