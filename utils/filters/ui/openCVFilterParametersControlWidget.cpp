/**
 * \file openCVFilterParametersControlWidget.cpp
 * \attention This file is automatically generated and should not be in general modified manually
 *
 * \date MMM DD, 20YY
 * \author autoGenerator
 */

#include "openCVFilterParametersControlWidget.h"
#include "ui_openCVFilterParametersControlWidget.h"
#include "qSettingsGetter.h"
#include "qSettingsSetter.h"


OpenCVFilterParametersControlWidget::OpenCVFilterParametersControlWidget(QWidget *parent, bool _autoInit, QString _rootPath)
    : FilterParametersControlWidgetBase(parent)
    , mUi(new Ui::OpenCVFilterParametersControlWidget)
    , autoInit(_autoInit)
    , rootPath(_rootPath)
{
    mUi->setupUi(this);

    QObject::connect(mUi->openCVFilterComboBox, SIGNAL(currentIndexChanged(int)), this, SIGNAL(paramsChanged()));
    QObject::connect(mUi->param1SpinBox, SIGNAL(valueChanged(int)), this, SIGNAL(paramsChanged()));
    QObject::connect(mUi->param2SpinBox, SIGNAL(valueChanged(int)), this, SIGNAL(paramsChanged()));
}

OpenCVFilterParametersControlWidget::~OpenCVFilterParametersControlWidget()
{

    delete mUi;
}

void OpenCVFilterParametersControlWidget::loadParamWidget(WidgetLoader &loader)
{
    OpenCVFilterParameters *params = createParameters();
    loader.loadParameters(*params, rootPath);
    setParameters(*params);
    delete params;
}

void OpenCVFilterParametersControlWidget::saveParamWidget(WidgetSaver  &saver)
{
    OpenCVFilterParameters *params = createParameters();
    saver.saveParameters(*params, rootPath);
    delete params;
}

 /* Composite fields are NOT supported so far */
void OpenCVFilterParametersControlWidget::getParameters(OpenCVFilterParameters& params) const
{

    params.setOpenCVFilter     (static_cast<OpenCVBinaryFilterType::OpenCVBinaryFilterType>(mUi->openCVFilterComboBox->currentIndex()));
    params.setParam1           (mUi->param1SpinBox->value());
    params.setParam2           (mUi->param2SpinBox->value());

}

OpenCVFilterParameters *OpenCVFilterParametersControlWidget::createParameters() const
{

    /**
     * We should think of returning parameters by value or saving them in a preallocated place
     **/


    OpenCVFilterParameters *result = new OpenCVFilterParameters(
          static_cast<OpenCVBinaryFilterType::OpenCVBinaryFilterType>(mUi->openCVFilterComboBox->currentIndex())
        , mUi->param1SpinBox->value()
        , mUi->param2SpinBox->value()
    );
    return result;
}

void OpenCVFilterParametersControlWidget::setParameters(const OpenCVFilterParameters &input)
{
    // Block signals to send them all at once
    bool wasBlocked = blockSignals(true);
    mUi->openCVFilterComboBox->setCurrentIndex(input.openCVFilter());
    mUi->param1SpinBox->setValue(input.param1());
    mUi->param2SpinBox->setValue(input.param2());
    blockSignals(wasBlocked);
    emit paramsChanged();
}

void OpenCVFilterParametersControlWidget::setParametersVirtual(void *input)
{
    // Modify widget parameters from outside
    OpenCVFilterParameters *inputCasted = static_cast<OpenCVFilterParameters *>(input);
    setParameters(*inputCasted);
}
