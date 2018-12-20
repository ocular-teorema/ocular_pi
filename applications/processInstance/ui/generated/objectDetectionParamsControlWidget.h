#pragma once
#include <QWidget>
#include "Generated/objectDetectionParams.h"
#include "ui_objectDetectionParamsControlWidget.h"
#include "parametersControlWidgetBase.h"


namespace Ui {
    class ObjectDetectionParamsControlWidget;
}

class ObjectDetectionParamsControlWidget : public ParametersControlWidgetBase
{
    Q_OBJECT

public:
    explicit ObjectDetectionParamsControlWidget(QWidget *parent = 0, bool autoInit = false, QString rootPath = QString());
    ~ObjectDetectionParamsControlWidget();

    ObjectDetectionParams* createParameters() const;
    void getParameters(ObjectDetectionParams &param) const;
    void setParameters(const ObjectDetectionParams &input);
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
    Ui_ObjectDetectionParamsControlWidget *mUi;
    bool autoInit;
    QString rootPath;
};

