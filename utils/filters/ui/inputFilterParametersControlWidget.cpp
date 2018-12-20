/**
 * \file inputFilterParametersControlWidget.cpp
 * \attention This file is automatically generated and should not be in general modified manually
 *
 * \date MMM DD, 20YY
 * \author autoGenerator
 */

#include "inputFilterParametersControlWidget.h"
#include "ui_inputFilterParametersControlWidget.h"
#include "qSettingsGetter.h"
#include "qSettingsSetter.h"


InputFilterParametersControlWidget::InputFilterParametersControlWidget(QWidget *parent, bool _autoInit, QString _rootPath)
    : FilterParametersControlWidgetBase(parent)
    , mUi(new Ui::InputFilterParametersControlWidget)
    , autoInit(_autoInit)
    , rootPath(_rootPath)
{
    mUi->setupUi(this);

    QObject::connect(mUi->inputTypeComboBox, SIGNAL(currentIndexChanged(int)), this, SIGNAL(paramsChanged()));
}

InputFilterParametersControlWidget::~InputFilterParametersControlWidget()
{

    delete mUi;
}

void InputFilterParametersControlWidget::loadParamWidget(WidgetLoader &loader)
{
    InputFilterParameters *params = createParameters();
    loader.loadParameters(*params, rootPath);
    setParameters(*params);
    delete params;
}

void InputFilterParametersControlWidget::saveParamWidget(WidgetSaver  &saver)
{
    InputFilterParameters *params = createParameters();
    saver.saveParameters(*params, rootPath);
    delete params;
}

 /* Composite fields are NOT supported so far */
void InputFilterParametersControlWidget::getParameters(InputFilterParameters& params) const
{

    params.setInputType        (static_cast<InputType::InputType>(mUi->inputTypeComboBox->currentIndex()));

}

InputFilterParameters *InputFilterParametersControlWidget::createParameters() const
{

    /**
     * We should think of returning parameters by value or saving them in a preallocated place
     **/


    InputFilterParameters *result = new InputFilterParameters(
          static_cast<InputType::InputType>(mUi->inputTypeComboBox->currentIndex())
    );
    return result;
}

void InputFilterParametersControlWidget::setParameters(const InputFilterParameters &input)
{
    // Block signals to send them all at once
    bool wasBlocked = blockSignals(true);
    mUi->inputTypeComboBox->setCurrentIndex(input.inputType());
    blockSignals(wasBlocked);
    emit paramsChanged();
}

void InputFilterParametersControlWidget::setParametersVirtual(void *input)
{
    // Modify widget parameters from outside
    InputFilterParameters *inputCasted = static_cast<InputFilterParameters *>(input);
    setParameters(*inputCasted);
}
