#pragma once
#include <QWidget>
#include "Generated/decisionParams.h"
#include "ui_decisionParamsControlWidget.h"
#include "parametersControlWidgetBase.h"


namespace Ui {
    class DecisionParamsControlWidget;
}

class DecisionParamsControlWidget : public ParametersControlWidgetBase
{
    Q_OBJECT

public:
    explicit DecisionParamsControlWidget(QWidget *parent = 0, bool autoInit = false, QString rootPath = QString());
    ~DecisionParamsControlWidget();

    DecisionParams* createParameters() const;
    void getParameters(DecisionParams &param) const;
    void setParameters(const DecisionParams &input);
    virtual void setParametersVirtual(void *input);
    
    virtual void loadParamWidget(WidgetLoader &loader);
    virtual void saveParamWidget(WidgetSaver  &saver);
    
public slots:
    void changeParameters()
    {
        // emit paramsChanged();
    }

signals:
    void valueChanged();
    void paramsChanged();

private:
    Ui_DecisionParamsControlWidget *mUi;
    bool autoInit;
    QString rootPath;
};

