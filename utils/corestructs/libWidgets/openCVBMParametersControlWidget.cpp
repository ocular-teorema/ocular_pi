/**
 * \file openCVBMParametersControlWidget.cpp
 * \attention This file is automatically generated and should not be in general modified manually
 *
 * \date MMM DD, 20YY
 * \author autoGenerator
 */

#include "openCVBMParametersControlWidget.h"
#include "ui_openCVBMParametersControlWidget.h"
#include "qSettingsGetter.h"
#include "qSettingsSetter.h"


OpenCVBMParametersControlWidget::OpenCVBMParametersControlWidget(QWidget *parent, bool _autoInit, QString _rootPath)
    : ParametersControlWidgetBase(parent)
    , mUi(new Ui::OpenCVBMParametersControlWidget)
    , autoInit(_autoInit)
    , rootPath(_rootPath)
{
    mUi->setupUi(this);

    QObject::connect(mUi->blockSizeSpinBox, SIGNAL(valueChanged(int)), this, SIGNAL(paramsChanged()));
    QObject::connect(mUi->disparitySearchSpinBox, SIGNAL(valueChanged(int)), this, SIGNAL(paramsChanged()));
    QObject::connect(mUi->preFilterCapSpinBox, SIGNAL(valueChanged(int)), this, SIGNAL(paramsChanged()));
    QObject::connect(mUi->minDisparitySpinBox, SIGNAL(valueChanged(int)), this, SIGNAL(paramsChanged()));
    QObject::connect(mUi->textureThresholdSpinBox, SIGNAL(valueChanged(int)), this, SIGNAL(paramsChanged()));
    QObject::connect(mUi->uniquenessRatioSpinBox, SIGNAL(valueChanged(int)), this, SIGNAL(paramsChanged()));
    QObject::connect(mUi->speckleWindowSizeSpinBox, SIGNAL(valueChanged(int)), this, SIGNAL(paramsChanged()));
    QObject::connect(mUi->speckleRangeSpinBox, SIGNAL(valueChanged(int)), this, SIGNAL(paramsChanged()));
    QObject::connect(mUi->disp12MaxDiffSpinBox, SIGNAL(valueChanged(int)), this, SIGNAL(paramsChanged()));
}

OpenCVBMParametersControlWidget::~OpenCVBMParametersControlWidget()
{

    delete mUi;
}

void OpenCVBMParametersControlWidget::loadParamWidget(WidgetLoader &loader)
{
    OpenCVBMParameters *params = createParameters();
    loader.loadParameters(*params, rootPath);
    setParameters(*params);
    delete params;
}

void OpenCVBMParametersControlWidget::saveParamWidget(WidgetSaver  &saver)
{
    OpenCVBMParameters *params = createParameters();
    saver.saveParameters(*params, rootPath);
    delete params;
}

 /* Composite fields are NOT supported so far */
void OpenCVBMParametersControlWidget::getParameters(OpenCVBMParameters& params) const
{

    params.setBlockSize        (mUi->blockSizeSpinBox->value());
    params.setDisparitySearch  (mUi->disparitySearchSpinBox->value());
    params.setPreFilterCap     (mUi->preFilterCapSpinBox->value());
    params.setMinDisparity     (mUi->minDisparitySpinBox->value());
    params.setTextureThreshold (mUi->textureThresholdSpinBox->value());
    params.setUniquenessRatio  (mUi->uniquenessRatioSpinBox->value());
    params.setSpeckleWindowSize(mUi->speckleWindowSizeSpinBox->value());
    params.setSpeckleRange     (mUi->speckleRangeSpinBox->value());
    params.setDisp12MaxDiff    (mUi->disp12MaxDiffSpinBox->value());

}

OpenCVBMParameters *OpenCVBMParametersControlWidget::createParameters() const
{

    /**
     * We should think of returning parameters by value or saving them in a preallocated place
     **/


    OpenCVBMParameters *result = new OpenCVBMParameters(
          mUi->blockSizeSpinBox->value()
        , mUi->disparitySearchSpinBox->value()
        , mUi->preFilterCapSpinBox->value()
        , mUi->minDisparitySpinBox->value()
        , mUi->textureThresholdSpinBox->value()
        , mUi->uniquenessRatioSpinBox->value()
        , mUi->speckleWindowSizeSpinBox->value()
        , mUi->speckleRangeSpinBox->value()
        , mUi->disp12MaxDiffSpinBox->value()
    );
    return result;
}

void OpenCVBMParametersControlWidget::setParameters(const OpenCVBMParameters &input)
{
    // Block signals to send them all at once
    bool wasBlocked = blockSignals(true);
    mUi->blockSizeSpinBox->setValue(input.blockSize());
    mUi->disparitySearchSpinBox->setValue(input.disparitySearch());
    mUi->preFilterCapSpinBox->setValue(input.preFilterCap());
    mUi->minDisparitySpinBox->setValue(input.minDisparity());
    mUi->textureThresholdSpinBox->setValue(input.textureThreshold());
    mUi->uniquenessRatioSpinBox->setValue(input.uniquenessRatio());
    mUi->speckleWindowSizeSpinBox->setValue(input.speckleWindowSize());
    mUi->speckleRangeSpinBox->setValue(input.speckleRange());
    mUi->disp12MaxDiffSpinBox->setValue(input.disp12MaxDiff());
    blockSignals(wasBlocked);
    emit paramsChanged();
}

void OpenCVBMParametersControlWidget::setParametersVirtual(void *input)
{
    // Modify widget parameters from outside
    OpenCVBMParameters *inputCasted = static_cast<OpenCVBMParameters *>(input);
    setParameters(*inputCasted);
}
