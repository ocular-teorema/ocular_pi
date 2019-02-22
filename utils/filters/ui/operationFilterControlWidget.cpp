/**
 * \file operationFilterControlWidget.cpp
 * \attention This file is automatically generated and should not be in general modified manually
 *
 * \date MMM DD, 20YY
 * \author autoGenerator
 */

#include "operationFilterControlWidget.h"
#include "ui_operationFilterControlWidget.h"
#include "qSettingsGetter.h"
#include "qSettingsSetter.h"

#include "operationParametersControlWidget.h"

OperationFilterControlWidget::OperationFilterControlWidget(QWidget *parent, bool _autoInit, QString _rootPath)
    : ParametersControlWidgetBase(parent)
    , mUi(new Ui::OperationFilterControlWidget)
    , autoInit(_autoInit)
    , rootPath(_rootPath)
{
    mUi->setupUi(this);

    QObject::connect(mUi->operationControlWidget, SIGNAL(paramsChanged()), this, SIGNAL(paramsChanged()));
    QObject::connect(mUi->in1, SIGNAL(), this, SIGNAL(paramsChanged()));
    QObject::connect(mUi->in2, SIGNAL(), this, SIGNAL(paramsChanged()));
    QObject::connect(mUi->out, SIGNAL(), this, SIGNAL(paramsChanged()));
}

OperationFilterControlWidget::~OperationFilterControlWidget()
{

    delete mUi;
}
void OperationFilterControlWidget::loadFromQSettings(const QString &fileName, const QString &_root)
{
    OperationFilter *params = createParameters();
    SettingsGetter visitor(fileName, _root + rootPath);
    params->accept<SettingsGetter>(visitor);
    setParameters(*params);
    delete params;
}

void OperationFilterControlWidget::saveToQSettings  (const QString &fileName, const QString &_root)
{
    OperationFilter *params = createParameters();
    SettingsSetter visitor(fileName, _root + rootPath);
    params->accept<SettingsSetter>(visitor);
    delete params;
}


OperationFilter *OperationFilterControlWidget::createParameters() const
{

    /**
     * We should think of returning parameters by value or saving them in a preallocated place
     **/

    OperationParameters *tmp0 = NULL;

    OperationFilter *result = new OperationFilter(
          * (tmp0 = mUi->operationControlWidget->createParameters())
        , mUi->in1->
        , mUi->in2->
        , mUi->out->
    );
    delete tmp0;
    return result;
}

void OperationFilterControlWidget::setParameters(const OperationFilter &input)
{
    // Block signals to send them all at once
    bool wasBlocked = blockSignals(true);
    mUi->operationControlWidget->setParameters(input.operation());
    mUi->in1->(input.in1());
    mUi->in2->(input.in2());
    mUi->out->(input.out());
    blockSignals(wasBlocked);
    emit paramsChanged();
}