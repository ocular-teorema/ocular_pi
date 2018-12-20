#ifndef REFLECTION_GENERATOR_H_
#define REFLECTION_GENERATOR_H_
/**
 * \file reflectionGenerator.h
 * \brief A header for reflectionGenerator.c
 *
 *
 *
 * \date Apr 19, 2012
 * \author alexander
 * \ingroup cppcorefiles
 */

#include <QString>
#include "reflection.h"

using corecvs::BaseField;
using corecvs::SimpleScalarField;
using corecvs::SimpleVectorField;
using corecvs::BoolField;
using corecvs::DoubleField;
using corecvs::StringField;
using corecvs::CompositeField;
using corecvs::CompositeArrayField;
using corecvs::EnumField;
using corecvs::PointerField;
using corecvs::ReflectionNaming;
using corecvs::Reflection;
using corecvs::EnumReflection;
using corecvs::EnumOption;


class ReflectionGen : public Reflection
{
public:
    const char *uiBaseClass;

    ReflectionGen () : uiBaseClass(NULL) {}
};

template<typename Type>
class SimpleScalarFieldGen : public SimpleScalarField<Type>
{
public:
    SimpleScalarFieldGen (
            const Type _defaultValue,
            const ReflectionNaming &_nameing,
            bool _hasAdditionalValues = false,
            Type _min = Type(0),
            Type _max = Type(0),
            Type _step = Type(0)
    ) : SimpleScalarField<Type>(
            BaseField::UNKNOWN_ID,
            BaseField::UNKNOWN_OFFSET,
            _defaultValue,
            _nameing,
            _hasAdditionalValues,
            _min,
            _max,
            _step
    ) {}

    virtual BaseField* clone() const
    {
        return new SimpleScalarFieldGen(*this);
    }
};

typedef SimpleScalarFieldGen<int>         IntFieldGen;
typedef SimpleScalarFieldGen<int64_t>     TimestampFieldGen;
//typedef SimpleScalarFieldGen<double>      DoubleFieldGen;
//typedef SimpleScalarFieldGen<std::string> StringFieldGen;


template<typename Type>
class SimpleVectorFieldGen : public SimpleVectorField<Type>
{
public:
    SimpleVectorFieldGen (
            const Type _defaultValue,
            int _defaultSize,
            const ReflectionNaming &_nameing,
            bool _hasAdditionalValues = false,
            Type _min = Type(0),
            Type _max = Type(0),
            Type _step = Type(0)
    ) : SimpleVectorField<Type>(
            BaseField::UNKNOWN_ID,
            BaseField::UNKNOWN_OFFSET,
            _defaultValue,
            _defaultSize,
            _nameing,
            _hasAdditionalValues,
            _min,
            _max,
            _step
    ) {}

    virtual BaseField* clone() const
    {
        return new SimpleVectorFieldGen(*this);
    }
};

typedef SimpleVectorFieldGen<int>         IntVectorFieldGen;
typedef SimpleVectorFieldGen<double>      DoubleVectorFieldGen;


class StringFieldGen : public StringField
{
public:
    StringFieldGen (
            const std::string _defaultValue,
            const ReflectionNaming &_nameing
    ) : StringField (
            BaseField::UNKNOWN_ID,
            BaseField::UNKNOWN_OFFSET,
            _defaultValue,
            _nameing
    ) {}

    virtual BaseField* clone() const
    {
        return new StringFieldGen(*this);
    }
};


enum BoolWidgetType {
    checkBox,
    radioButton
};

class BoolFieldGen : public BoolField
{
public:
    BoolWidgetType widgetType;

    BoolFieldGen(
            bool _defaultValue,
            BoolWidgetType _widgetType,
            const ReflectionNaming &_nameing
    ) : BoolField (
            BaseField::UNKNOWN_ID,
            BaseField::UNKNOWN_OFFSET,
            _defaultValue,
            _nameing
        ),
        widgetType(_widgetType)
    {}

    virtual BaseField* clone() const
    {
        return new BoolFieldGen(*this);
    }
};

enum DoubleWidgetType {
    doubleSpinBox,
    exponentialSlider
};

class DoubleFieldGen : public DoubleField
{
public:
    DoubleWidgetType widgetType;
    QString prefix;
    QString suffix;

    DoubleFieldGen(
        double _defaultValue,
        DoubleWidgetType _widgetType,
        QString _prefix,
        QString _suffix,
        const ReflectionNaming &_nameing,
        bool _hasAdditionalValues = false,
        double _min = 0.0,
        double _max = 0.0,
        double _step = 0.1
        ) : DoubleField (
            BaseField::UNKNOWN_ID,
            BaseField::UNKNOWN_OFFSET,
            _defaultValue,
            _nameing,
            _hasAdditionalValues,
            _min,
            _max,
            _step
        ),
        widgetType(_widgetType),
        prefix(_prefix),
        suffix(_suffix)
    {}

    virtual BaseField* clone() const
    {
        return new DoubleFieldGen(*this);
    }
};


class CompositeFieldGen : public CompositeField
{
public:
    CompositeFieldGen(
            const ReflectionNaming &_nameing,
            const char *_typeName,
            const Reflection *_reflection = NULL
      ) : CompositeField (
              BaseField::UNKNOWN_ID,
              BaseField::UNKNOWN_OFFSET,
              _nameing,
             _typeName,
             _reflection
      ) {}

    virtual BaseField* clone() const
    {
        return new CompositeFieldGen(*this);
    }
};

class CompositeArrayFieldGen : public CompositeArrayField
{
public:
    CompositeArrayFieldGen(
            const ReflectionNaming &_nameing,
            const char *_typeName,
            int _size,
            const Reflection *_reflection = NULL
      ) : CompositeArrayField (
              BaseField::UNKNOWN_ID,
              BaseField::UNKNOWN_OFFSET,
              _nameing,
              _typeName,
              _size,
              _reflection
      ) {}

    virtual BaseField* clone() const
    {
        return new CompositeArrayFieldGen(*this);
    }
};


enum EnumWidgetType {
    comboBox,
    tabWidget
};

class EnumOptionGen : public EnumOption
{
public:
    QString icon;

    EnumOptionGen(
        int _id,
        const ReflectionNaming &_nameing,
        QString _icon = QString("")
    ) : EnumOption(_id,_nameing),
        icon(_icon)
    {

    }
};

class EnumFieldGen : public EnumField
{
public:
    EnumWidgetType widgetType;


    EnumFieldGen(
            int _defaultValue,
            EnumWidgetType _widgetType,
            const ReflectionNaming &_nameing,
            const EnumReflection *_enumReflection,
            QString _icon = QString("")
    ) : EnumField (
            BaseField::UNKNOWN_ID,
            BaseField::UNKNOWN_OFFSET,
            _defaultValue,
            _nameing,
            _enumReflection
        ),
        widgetType(_widgetType)
    {}

    virtual BaseField* clone() const
    {
        return new EnumFieldGen(*this);
    }
};

class PointerFieldGen : public PointerField
{
public:
    PointerFieldGen(
        const ReflectionNaming &_nameing,
        const char *_typeName
    ) :
        PointerField(
            BaseField::UNKNOWN_ID,
            BaseField::UNKNOWN_OFFSET,
            NULL,
            _nameing,
            _typeName
        )
    {}


    virtual BaseField* clone() const
    {
        return new PointerFieldGen(*this);
    }
};


#endif  /* #ifndef REFLECTION_GENERATOR_H_ */


