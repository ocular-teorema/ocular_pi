#ifndef YUV422PLANES_H
#define YUV422PLANES_H

#include "abstractBuffer.h"
#include "yuvColor.h"
#include "g8Buffer.h"

namespace corecvs {

class YUV422PlanesAccessor
{
public:
    uint8_t *yPtr;
    uint8_t *uPtr;
    uint8_t *vPtr;

    YUV422PlanesAccessor(
            uint8_t *_yPtr,
            uint8_t *_uPtr,
            uint8_t *_vPtr
    ) : yPtr(_yPtr),
        uPtr(_uPtr),
        vPtr(_vPtr)
    {}

    void operator = (const YUVColor &color)
    {
        *yPtr = color.y();
        if ((reinterpret_cast<uintptr_t>(uPtr) & 0x1) == 0) {
            *uPtr = color.u();
            *vPtr = color.v();
        }
    }

    operator YUVColor() const
    {
        return YUVColor(*yPtr, *uPtr, *vPtr);
    }

};


class YUV422Planes
{
public:
    G8Buffer yPlane;
    G8Buffer uPlane;
    G8Buffer vPlane;

    YUV422Planes();

 /*   YUV422Planes(int h, v);*/

    YUV422PlanesAccessor element(int i, int j)
    {
        return YUV422PlanesAccessor(&yPlane.element(i,j), &uPlane.element(i / 2,j / 2), &vPlane.element(i / 2,j / 2));
    }

    void drawRect (int y, int x, int h, int w, const YUVColor &color)
    {
        yPlane.fillRectangleWith(x, y, h, w, color.y());
        uPlane.fillRectangleWith(x / 2, y / 2, h / 2, w / 2, color.u());
        vPlane.fillRectangleWith(x / 2, y / 2, h / 2, w / 2, color.v());
    }

};

} // namespace corecvs

#endif // YUV422PLANES_H
