#pragma once
#include <QWidget>
#include "Generated/dateParams.h"
#include "ui_dateParamsControlWidget.h"
#include "parametersControlWidgetBase.h"


namespace Ui {
    class DateParamsControlWidget;
}

class DateParamsControlWidget : public ParametersControlWidgetBase
{
    Q_OBJECT

public:
    explicit DateParamsControlWidget(QWidget *parent = 0, bool autoInit = false, QString rootPath = QString());
    ~DateParamsControlWidget();

    DateParams* createParameters() const;
    void getParameters(DateParams &param) const;
    void setParameters(const DateParams &input);
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
    Ui_DateParamsControlWidget *mUi;
    bool autoInit;
    QString rootPath;
};

