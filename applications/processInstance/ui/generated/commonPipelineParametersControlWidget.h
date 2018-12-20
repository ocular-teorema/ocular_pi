#pragma once
#include <QWidget>
#include "Generated/commonPipelineParameters.h"
#include "ui_commonPipelineParametersControlWidget.h"
#include "parametersControlWidgetBase.h"


namespace Ui {
    class CommonPipelineParametersControlWidget;
}

class CommonPipelineParametersControlWidget : public ParametersControlWidgetBase
{
    Q_OBJECT

public:
    explicit CommonPipelineParametersControlWidget(QWidget *parent = 0, bool autoInit = false, QString rootPath = QString());
    ~CommonPipelineParametersControlWidget();

    CommonPipelineParameters* createParameters() const;
    void getParameters(CommonPipelineParameters &param) const;
    void setParameters(const CommonPipelineParameters &input);
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
    Ui_CommonPipelineParametersControlWidget *mUi;
    bool autoInit;
    QString rootPath;
};

