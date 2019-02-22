/**
 * \file analysisParamsControlWidget.cpp
 * \attention This file is automatically generated and should not be in general modified manually
 *
 * \date MMM DD, 20YY
 * \author autoGenerator
 */

#include "analysisParamsControlWidget.h"
#include "ui_analysisParamsControlWidget.h"
#include "qSettingsGetter.h"
#include "qSettingsSetter.h"


AnalysisParamsControlWidget::AnalysisParamsControlWidget(QWidget *parent, bool _autoInit, QString _rootPath)
    : ParametersControlWidgetBase(parent)
    , mUi(new Ui::AnalysisParamsControlWidget)
    , autoInit(_autoInit)
    , rootPath(_rootPath)
{
    mUi->setupUi(this);

    QObject::connect(mUi->downscaleCoeffSpinBox, SIGNAL(valueChanged(double)), this, SIGNAL(paramsChanged()));
    QObject::connect(mUi->motionThresholdSpinBox, SIGNAL(valueChanged(double)), this, SIGNAL(paramsChanged()));
    QObject::connect(mUi->diffThresholdSpinBox, SIGNAL(valueChanged(int)), this, SIGNAL(paramsChanged()));
    QObject::connect(mUi->produceDebugCheckBox, SIGNAL(stateChanged(int)), this, SIGNAL(paramsChanged()));
    QObject::connect(mUi->debugImageIndexSpinBox, SIGNAL(valueChanged(int)), this, SIGNAL(paramsChanged()));
    QObject::connect(mUi->differenceBasedAnalysisCheckBox, SIGNAL(stateChanged(int)), this, SIGNAL(paramsChanged()));
    QObject::connect(mUi->motionBasedAnalysisCheckBox, SIGNAL(stateChanged(int)), this, SIGNAL(paramsChanged()));
    QObject::connect(mUi->totalThresholdSpinBox, SIGNAL(valueChanged(double)), this, SIGNAL(paramsChanged()));
    QObject::connect(mUi->validMotionBinHeightSpinBox, SIGNAL(valueChanged(int)), this, SIGNAL(paramsChanged()));
    QObject::connect(mUi->debugObjectsCheckBox, SIGNAL(stateChanged(int)), this, SIGNAL(paramsChanged()));
    QObject::connect(mUi->experimentalCheckBox, SIGNAL(stateChanged(int)), this, SIGNAL(paramsChanged()));
    QObject::connect(mUi->minimumClusterSpinBox, SIGNAL(valueChanged(int)), this, SIGNAL(paramsChanged()));
    QObject::connect(mUi->dilateSizeSpinBox, SIGNAL(valueChanged(int)), this, SIGNAL(paramsChanged()));
    QObject::connect(mUi->bgThresholdSpinBox, SIGNAL(valueChanged(int)), this, SIGNAL(paramsChanged()));
    QObject::connect(mUi->fgThresholdSpinBox, SIGNAL(valueChanged(double)), this, SIGNAL(paramsChanged()));
    QObject::connect(mUi->useVirtualDateCheckBox, SIGNAL(stateChanged(int)), this, SIGNAL(paramsChanged()));
    QObject::connect(mUi->yearSpinBox, SIGNAL(valueChanged(int)), this, SIGNAL(paramsChanged()));
    QObject::connect(mUi->monthSpinBox, SIGNAL(valueChanged(int)), this, SIGNAL(paramsChanged()));
    QObject::connect(mUi->daySpinBox, SIGNAL(valueChanged(int)), this, SIGNAL(paramsChanged()));
    QObject::connect(mUi->hourSpinBox, SIGNAL(valueChanged(int)), this, SIGNAL(paramsChanged()));
    QObject::connect(mUi->minuteSpinBox, SIGNAL(valueChanged(int)), this, SIGNAL(paramsChanged()));
}

AnalysisParamsControlWidget::~AnalysisParamsControlWidget()
{

    delete mUi;
}

void AnalysisParamsControlWidget::loadParamWidget(WidgetLoader &loader)
{
    AnalysisParams *params = createParameters();
    loader.loadParameters(*params, rootPath);
    setParameters(*params);
    delete params;
}

void AnalysisParamsControlWidget::saveParamWidget(WidgetSaver  &saver)
{
    AnalysisParams *params = createParameters();
    saver.saveParameters(*params, rootPath);
    delete params;
}

 /* Composite fields are NOT supported so far */
void AnalysisParamsControlWidget::getParameters(AnalysisParams& params) const
{

    params.setDownscaleCoeff   (mUi->downscaleCoeffSpinBox->value());
    params.setMotionThreshold  (mUi->motionThresholdSpinBox->value());
    params.setDiffThreshold    (mUi->diffThresholdSpinBox->value());
    params.setProduceDebug     (mUi->produceDebugCheckBox->isChecked());
    params.setDebugImageIndex  (mUi->debugImageIndexSpinBox->value());
    params.setDifferenceBasedAnalysis(mUi->differenceBasedAnalysisCheckBox->isChecked());
    params.setMotionBasedAnalysis(mUi->motionBasedAnalysisCheckBox->isChecked());
    params.setTotalThreshold   (mUi->totalThresholdSpinBox->value());
    params.setValidMotionBinHeight(mUi->validMotionBinHeightSpinBox->value());
    params.setDebugObjects     (mUi->debugObjectsCheckBox->isChecked());
    params.setExperimental     (mUi->experimentalCheckBox->isChecked());
    params.setMinimumCluster   (mUi->minimumClusterSpinBox->value());
    params.setDilateSize       (mUi->dilateSizeSpinBox->value());
    params.setBgThreshold      (mUi->bgThresholdSpinBox->value());
    params.setFgThreshold      (mUi->fgThresholdSpinBox->value());
    params.setUseVirtualDate   (mUi->useVirtualDateCheckBox->isChecked());
    params.setYear             (mUi->yearSpinBox->value());
    params.setMonth            (mUi->monthSpinBox->value());
    params.setDay              (mUi->daySpinBox->value());
    params.setHour             (mUi->hourSpinBox->value());
    params.setMinute           (mUi->minuteSpinBox->value());

}

AnalysisParams *AnalysisParamsControlWidget::createParameters() const
{

    /**
     * We should think of returning parameters by value or saving them in a preallocated place
     **/


    AnalysisParams *result = new AnalysisParams(
          mUi->downscaleCoeffSpinBox->value()
        , mUi->motionThresholdSpinBox->value()
        , mUi->diffThresholdSpinBox->value()
        , mUi->produceDebugCheckBox->isChecked()
        , mUi->debugImageIndexSpinBox->value()
        , mUi->differenceBasedAnalysisCheckBox->isChecked()
        , mUi->motionBasedAnalysisCheckBox->isChecked()
        , mUi->totalThresholdSpinBox->value()
        , mUi->validMotionBinHeightSpinBox->value()
        , mUi->debugObjectsCheckBox->isChecked()
        , mUi->experimentalCheckBox->isChecked()
        , mUi->minimumClusterSpinBox->value()
        , mUi->dilateSizeSpinBox->value()
        , mUi->bgThresholdSpinBox->value()
        , mUi->fgThresholdSpinBox->value()
        , mUi->useVirtualDateCheckBox->isChecked()
        , mUi->yearSpinBox->value()
        , mUi->monthSpinBox->value()
        , mUi->daySpinBox->value()
        , mUi->hourSpinBox->value()
        , mUi->minuteSpinBox->value()
    );
    return result;
}

void AnalysisParamsControlWidget::setParameters(const AnalysisParams &input)
{
    // Block signals to send them all at once
    bool wasBlocked = blockSignals(true);
    mUi->downscaleCoeffSpinBox->setValue(input.downscaleCoeff());
    mUi->motionThresholdSpinBox->setValue(input.motionThreshold());
    mUi->diffThresholdSpinBox->setValue(input.diffThreshold());
    mUi->produceDebugCheckBox->setChecked(input.produceDebug());
    mUi->debugImageIndexSpinBox->setValue(input.debugImageIndex());
    mUi->differenceBasedAnalysisCheckBox->setChecked(input.differenceBasedAnalysis());
    mUi->motionBasedAnalysisCheckBox->setChecked(input.motionBasedAnalysis());
    mUi->totalThresholdSpinBox->setValue(input.totalThreshold());
    mUi->validMotionBinHeightSpinBox->setValue(input.validMotionBinHeight());
    mUi->debugObjectsCheckBox->setChecked(input.debugObjects());
    mUi->experimentalCheckBox->setChecked(input.experimental());
    mUi->minimumClusterSpinBox->setValue(input.minimumCluster());
    mUi->dilateSizeSpinBox->setValue(input.dilateSize());
    mUi->bgThresholdSpinBox->setValue(input.bgThreshold());
    mUi->fgThresholdSpinBox->setValue(input.fgThreshold());
    mUi->useVirtualDateCheckBox->setChecked(input.useVirtualDate());
    mUi->yearSpinBox->setValue(input.year());
    mUi->monthSpinBox->setValue(input.month());
    mUi->daySpinBox->setValue(input.day());
    mUi->hourSpinBox->setValue(input.hour());
    mUi->minuteSpinBox->setValue(input.minute());
    blockSignals(wasBlocked);
    emit paramsChanged();
}

void AnalysisParamsControlWidget::setParametersVirtual(void *input)
{
    // Modify widget parameters from outside
    AnalysisParams *inputCasted = static_cast<AnalysisParams *>(input);
    setParameters(*inputCasted);
}