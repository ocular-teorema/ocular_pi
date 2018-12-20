#pragma once
#include <QWidget>
#include "Generated/analysisParams.h"
#include "ui_analysisParamsControlWidget.h"
#include "parametersControlWidgetBase.h"


namespace Ui {
    class AnalysisParamsControlWidget;
}

class AnalysisParamsControlWidget : public ParametersControlWidgetBase
{
    Q_OBJECT

public:
    explicit AnalysisParamsControlWidget(QWidget *parent = 0, bool autoInit = false, QString rootPath = QString());
    ~AnalysisParamsControlWidget();

    AnalysisParams* createParameters() const;
    void getParameters(AnalysisParams &param) const;
    void setParameters(const AnalysisParams &input);
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
    Ui_AnalysisParamsControlWidget *mUi;
    bool autoInit;
    QString rootPath;
};

