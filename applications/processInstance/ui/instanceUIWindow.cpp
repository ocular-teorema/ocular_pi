#include  "foldableWidget.h"

#include <sstream>
#include "instanceUIWindow.h"
#include "ui_instanceUIWindow.h"
#include "log.h"

InstanceUIWindow::InstanceUIWindow(QWidget *parent) :
    QMainWindow(parent),
    mDataDirectory(NULL),
    mainImage(NULL),
    ui(new Ui::InstanceUIWindow),
    analysisParams(NULL),
    commonPipelineParameters(NULL),
    eventProcessingParams(NULL)
{
    ui->setupUi(this);

    Log::mLogDrains.add(ui->loggingWidget);

    connect(ui->actionStep , SIGNAL(triggered())    , this, SIGNAL(eventStep()));
    connect(ui->actionPause, SIGNAL(triggered(bool)), this, SLOT(pausePressed()));
    connect(ui->actionStop , SIGNAL(triggered(bool)), this, SIGNAL(eventStop()));

    connect(ui->actionGraph     , SIGNAL(triggered(bool)), this, SLOT(showGraphDialog()));
    connect(ui->actionStatistics, SIGNAL(triggered(bool)), this, SLOT(showStatsDialog()));


    connect(ui->imageComboBox, SIGNAL(currentIndexChanged(QString)), this, SLOT(itemSelected(QString)));
    mainImage = new AdvancedImageWidget();
    setCentralWidget(mainImage);
    
    analysisParams           = new AnalysisParamsControlWidget;
    commonPipelineParameters = new CommonPipelineParametersControlWidget;
    eventProcessingParams    = new EventProcessingParamsControlWidget;

    dockWidget()->setLayout(new QVBoxLayout());

    FoldableWidget *f1 = new FoldableWidget(NULL, "Analysis Parameters",        analysisParams, false);
    FoldableWidget *f2 = new FoldableWidget(NULL, "Common Pipeline Parameters", commonPipelineParameters, false);
    FoldableWidget *f3 = new FoldableWidget(NULL, "Event Processing Params",    eventProcessingParams, false);

    dockWidget()->layout()->addWidget(f1);
    dockWidget()->layout()->addWidget(f2);
    dockWidget()->layout()->addWidget(f3);
    dockWidget()->layout()->addItem(new QSpacerItem(0, 2000, QSizePolicy::Preferred));


    connect(analysisParams,           SIGNAL(paramsChanged()), this, SLOT(analysisUIUpdated()));
    connect(commonPipelineParameters, SIGNAL(paramsChanged()), this, SLOT(pipelineUIUpdated()));
    connect(eventProcessingParams,    SIGNAL(paramsChanged()), this, SLOT(eventUIUpdated()));

//    analysisParams->show();

}

QWidget * InstanceUIWindow::dockWidget(void)
{
    return ui->paramsList;
}

InstanceUIWindow::~InstanceUIWindow()
{
    Log::mLogDrains.detach(ui->loggingWidget);
    delete ui;
}

void InstanceUIWindow::setDataDirectory(DataDirectory *data)
{
    if (mDataDirectory != NULL)
    {
        disconnect(mDataDirectory, 0, this, 0);
    }

    mDataDirectory = data;
    if (mDataDirectory != NULL)
    {
        connect(mDataDirectory, SIGNAL(updatedImage(QString)), this, SLOT(updateImages(QString)));
        connect(mDataDirectory, SIGNAL(updatedParams()), this, SLOT(updateParams()));

        connect(mDataDirectory, SIGNAL(graphsUpdated()), this, SLOT(graphsUpdated()));
        connect(mDataDirectory, SIGNAL( statsUpdated()), this, SLOT(statsUpdated()));

        updateImages("");
        updateParams();
    }

}

void InstanceUIWindow::pausePressed()
{
    if (ui->actionPause->isChecked())
    {
        L_INFO_P("Setting pause on");
        emit eventPause();
    } else {
        L_INFO_P("Setting pause off");
        emit eventResume();
    }
}


/*virtual QList<QString> getImageNames();
virtual MetaImage getImage(QString name);*/

void InstanceUIWindow::updateImages(QString name)
{    
    /*if (!mImages.contains(name)) {
        ui->imageComboBox->insertItem(ui->imageComboBox->count(), name);
    }*/
    if(mDataDirectory == NULL)
        return;

    QComboBox *box = ui->imageComboBox;

    QList<QString> images = mDataDirectory->getImageNames();
    for (int i = 0; i < images.size(); i++)
    {
        QString newName = images[i];
        int j = 0;
        for (j = 0; j < box->count(); j++)
        {
            if (newName == box->itemText(j))
            {
                break;
            }
        }
        if (j == box->count())
        {
            box->insertItem(box->count(), images[i]);
        }
    }

    if (name == box->currentText())
    {
        itemSelected(name);
    }
}

void InstanceUIWindow::itemSelected(QString name)
{
    if(mDataDirectory == NULL)
        return;

    MetaImage image = mDataDirectory->getImage(name);
    if (image.mImage.isNull())
        return;

    mainImage->setImage(image.mImage);
}

void InstanceUIWindow::updateParams()
{
    if (mDataDirectory == NULL)
        return;

    analysisParams->blockSignals(true);
    analysisParams->setParameters( mDataDirectory->getParams());
    analysisParams->blockSignals(false);

    commonPipelineParameters->blockSignals(true);
    commonPipelineParameters->setParameters( mDataDirectory->getPipelineParams());
    commonPipelineParameters->blockSignals(false);

    eventProcessingParams->blockSignals(true);
    eventProcessingParams->setParameters( mDataDirectory->getEventParams());
    eventProcessingParams->blockSignals(false);
}

void InstanceUIWindow::analysisUIUpdated()
{
    if(mDataDirectory == NULL)
        return;
    AnalysisParams params;
    analysisParams->getParameters(params);
    mDataDirectory->setParams(params);
}

void InstanceUIWindow::pipelineUIUpdated()
{
    if(mDataDirectory == NULL)
        return;
    CommonPipelineParameters params;
    commonPipelineParameters->getParameters(params);
    mDataDirectory->setPipelineParams(params);
}

void InstanceUIWindow::eventUIUpdated()
{
    if(mDataDirectory == NULL)
        return;
    EventProcessingParams params;
    eventProcessingParams->getParameters(params);
    mDataDirectory->setEventParams(params);
}

void InstanceUIWindow::graphsUpdated()
{
    if(mDataDirectory == NULL)
        return;
    //L_INFO_P("InstanceUIWindow::graphUpdated()");
//    graph.addGraphPoint("test1", 23.0);
//    graph.update();

    GraphData *data = mDataDirectory->getGraphData();
    graph.replaceData(*data);


}

void InstanceUIWindow::statsUpdated()
{
    if(mDataDirectory == NULL)
        return;
    //L_INFO_P("InstanceUIWindow::statsUpdated()");
    corecvs::BaseTimeStatisticsCollector collector = mDataDirectory->getCollector();
    std::stringstream strstr;
    collector.printAdvanced(strstr);
    stats.setText(QString::fromStdString(strstr.str()));

}



void InstanceUIWindow::showGraphDialog()
{
    graph.show();
    graph.raise();
}

void InstanceUIWindow::showStatsDialog()
{
    stats.show();
    stats.raise();
}


void InstanceUIWindow::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event)
    qApp->quit();
}

