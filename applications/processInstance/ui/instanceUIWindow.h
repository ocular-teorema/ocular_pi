#ifndef INSTANCEUIWINDOW_H
#define INSTANCEUIWINDOW_H

#include <QHash>
#include <QMainWindow>


#include "advancedImageWidget.h"
#include "networkUtils/dataDirectory.h"

#include "graphPlotDialog.h"
#include "textLabelWidget.h"

//
// Generated
//
#include "generated/analysisParamsControlWidget.h"
#include "generated/commonPipelineParametersControlWidget.h"
#include "generated/eventProcessingParamsControlWidget.h"

namespace Ui {
class InstanceUIWindow;
}

class InstanceUIWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit InstanceUIWindow(QWidget *parent = 0);
    ~InstanceUIWindow();

    DataDirectory *mDataDirectory;
    QWidget *dockWidget();

    void closeEvent(QCloseEvent *event);
public slots:

    /**
     *  Doesn't take ownership
     **/
    void setDataDirectory(DataDirectory *data = NULL);

    /* UI related */
    void pausePressed();


    /* Image realted */
    void updateImages(QString name);
    void itemSelected(QString name);

    /* Parametes related */
    void updateParams();

    void analysisUIUpdated();
    void pipelineUIUpdated();
    void eventUIUpdated();

    /* Graph related */
    void graphsUpdated();
    void showGraphDialog();

    /* Stats related */
    void statsUpdated();
    void showStatsDialog();

signals:
    void eventPause();
    void eventResume();
    void eventStop();
    void eventStep();



private:
    struct {
        bool tmp;
        int row;
    };
    QHash<QString, int > mImages;

    AdvancedImageWidget *mainImage;
    Ui::InstanceUIWindow *ui;
    
    /**/
    AnalysisParamsControlWidget            *analysisParams;
    CommonPipelineParametersControlWidget  *commonPipelineParameters;
    EventProcessingParamsControlWidget     *eventProcessingParams;

    /* Subwindows */
    GraphPlotDialog graph;
    TextLabelWidget stats;
};

#endif // INSTANCEUIWINDOW_H
