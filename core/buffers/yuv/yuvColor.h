#ifndef YUVCOLOR_H
#define YUVCOLOR_H

#include "fixedVector.h"
#include "reflection.h"

namespace corecvs {

class YUVColor : public FixedVector<uint8_t, 3> {
public:
    typedef FixedVector<uint8_t, 3> YUVColorBase;

    enum FieldId {
        FIELD_Y = 0,
        FIELD_U = 1,
        FIELD_V = 2,
    };

    YUVColor(const YUVColorBase& base) : YUVColorBase(base) {}

    YUVColor(uint8_t _y, uint8_t _u, uint8_t _v)
    {
        this->y() = _y;
        this->u() = _u;
        this->v() = _v;
    }


    YUVColor()
    {}

    inline uint8_t &y()
    {
        return (*this)[FIELD_Y];
    }

    inline uint8_t &u()
    {
        return (*this)[FIELD_U];
    }

    inline uint8_t &v()
    {
        return (*this)[FIELD_V];
    }

    /* Constant versions for read-only form const colors */
    inline const uint8_t &y() const
    {
        return (*this)[FIELD_Y];
    }

    inline const uint8_t &u() const
    {
        return (*this)[FIELD_U];
    }

    inline const uint8_t &v() const
    {
        return (*this)[FIELD_V];
    }


    static YUVColor Red()
    {
        return YUVColor(82, 90, 240);
    }

    static Reflection reflect;

    static Reflection staticInit()
    {
        Reflection reflection;
        reflection.fields.push_back( new IntField(FIELD_Y, 0, "y") );
        reflection.fields.push_back( new IntField(FIELD_U, 0, "u") );
        reflection.fields.push_back( new IntField(FIELD_V, 0, "v") );
        return reflection;
    }

template<class VisitorType>
    void accept(VisitorType &visitor)
    {
        visitor.visit(y(), static_cast<const IntField *>(reflect.fields[FIELD_Y]));
        visitor.visit(u(), static_cast<const IntField *>(reflect.fields[FIELD_U]));
        visitor.visit(v(), static_cast<const IntField *>(reflect.fields[FIELD_V]));
     }

    friend ostream & operator <<(ostream &out, const YUVColor &color)
    {
        out << "[";
            out << (int)color.y() << ", " << (int)color.u() << " (" << (int)color.v() << ")";
        out << "]";
        return out;
    }

};



} //namespace corecvs

#endif // YUVCOLOR_H
