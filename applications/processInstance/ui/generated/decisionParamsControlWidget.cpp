/**
 * \file decisionParamsControlWidget.cpp
 * \attention This file is automatically generated and should not be in general modified manually
 *
 * \date MMM DD, 20YY
 * \author autoGenerator
 */

#include "decisionParamsControlWidget.h"
#include "ui_decisionParamsControlWidget.h"
#include "qSettingsGetter.h"
#include "qSettingsSetter.h"


DecisionParamsControlWidget::DecisionParamsControlWidget(QWidget *parent, bool _autoInit, QString _rootPath)
    : ParametersControlWidgetBase(parent)
    , mUi(new Ui::DecisionParamsControlWidget)
    , autoInit(_autoInit)
    , rootPath(_rootPath)
{
    mUi->setupUi(this);

    QObject::connect(mUi->totalThresholdSpinBox, SIGNAL(valueChanged(double)), this, SIGNAL(paramsChanged()));
    QObject::connect(mUi->validMotionBinHeightSpinBox, SIGNAL(valueChanged(int)), this, SIGNAL(paramsChanged()));
}

DecisionParamsControlWidget::~DecisionParamsControlWidget()
{

    delete mUi;
}

void DecisionParamsControlWidget::loadParamWidget(WidgetLoader &loader)
{
    DecisionParams *params = createParameters();
    loader.loadParameters(*params, rootPath);
    setParameters(*params);
    delete params;
}

void DecisionParamsControlWidget::saveParamWidget(WidgetSaver  &saver)
{
    DecisionParams *params = createParameters();
    saver.saveParameters(*params, rootPath);
    delete params;
}

 /* Composite fields are NOT supported so far */
void DecisionParamsControlWidget::getParameters(DecisionParams& params) const
{

    params.setTotalThreshold   (mUi->totalThresholdSpinBox->value());
    params.setValidMotionBinHeight(mUi->validMotionBinHeightSpinBox->value());

}

DecisionParams *DecisionParamsControlWidget::createParameters() const
{

    /**
     * We should think of returning parameters by value or saving them in a preallocated place
     **/


    DecisionParams *result = new DecisionParams(
          mUi->totalThresholdSpinBox->value()
        , mUi->validMotionBinHeightSpinBox->value()
    );
    return result;
}

void DecisionParamsControlWidget::setParameters(const DecisionParams &input)
{
    // Block signals to send them all at once
    bool wasBlocked = blockSignals(true);
    mUi->totalThresholdSpinBox->setValue(input.totalThreshold());
    mUi->validMotionBinHeightSpinBox->setValue(input.validMotionBinHeight());
    blockSignals(wasBlocked);
    emit paramsChanged();
}

void DecisionParamsControlWidget::setParametersVirtual(void *input)
{
    // Modify widget parameters from outside
    DecisionParams *inputCasted = static_cast<DecisionParams *>(input);
    setParameters(*inputCasted);
}