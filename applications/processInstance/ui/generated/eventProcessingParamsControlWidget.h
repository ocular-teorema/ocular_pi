#pragma once
#include <QWidget>
#include "Generated/eventProcessingParams.h"
#include "ui_eventProcessingParamsControlWidget.h"
#include "parametersControlWidgetBase.h"


namespace Ui {
    class EventProcessingParamsControlWidget;
}

class EventProcessingParamsControlWidget : public ParametersControlWidgetBase
{
    Q_OBJECT

public:
    explicit EventProcessingParamsControlWidget(QWidget *parent = 0, bool autoInit = false, QString rootPath = QString());
    ~EventProcessingParamsControlWidget();

    EventProcessingParams* createParameters() const;
    void getParameters(EventProcessingParams &param) const;
    void setParameters(const EventProcessingParams &input);
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
    Ui_EventProcessingParamsControlWidget *mUi;
    bool autoInit;
    QString rootPath;
};

