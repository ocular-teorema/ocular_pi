#ifndef CAMERAPIPELINE_H
#define CAMERAPIPELINE_H

#include <QObject>
#include <QThread>

#include "rtspCapture.h"
#include "videoAnalyzer.h"
#include "streamRecorder.h"
#include "resultVideoOutput.h"
#include "decisionMaker.h"
#include "eventHandler.h"
#include "videoEncoder.h"
#include "healthChecker.h"
#include "dbstat/intervalStatistics.h"
#include "networkUtils/dataDirectory.h"

class CameraPipeline : public QObject
{
    Q_OBJECT
public:

    CameraPipeline(QObject* parent);
    ~CameraPipeline();

    // Objects
    RTSPCapture*            pRtspCapture;           /// Capture object (works in separate thread)
    RTSPCapture*            pRtspSmallStreamCapture;/// Capture object (for small stream)
    StreamRecorder*         pStreamRecorder;        /// Recorder object (works in separate thread)
    VideoAnalyzer*          pVideoAnalyzer;         /// VideoAnalyzer object (works in processing thread)
    VideoStatistics*        pVideoStatistics;       /// Object for processing period statistics
    StatisticDBInterface*   pStatisticDBIntf;       /// Object for read/write statistics to DB
    EventHandler*           pEventHandler;          /// Object for handling alert decisions and event-related stuff
    ResultVideoOutput*      pSourceOutput;          /// Object for source stream output
    ResultVideoOutput*      pResultOutput;          /// Object for stream output
    ResultVideoOutput*      pSmallStreamOutput;     /// Object for small stream output
    HealthChecker*          pHealthChecker;         /// Object that performs pipeline health check

    QThread*                pCaptureThread;         /// Interface for capture thread
    QThread*                pRecorderThread;        /// Interface for stream recorder thread
    QThread*                pStatisticThread;       /// Interface for statistic thread
    QThread*                pProcessingThread;      /// Interface for main processing thread
    QThread*                pHealthCheckThread;     /// Interface for health checker thread

signals:
    void    ProcessingStopped();
    void    TimeoutError(QString message);

public slots:
    void    RunPipeline();
    void    StopPipeline();
    void    PausePipeline();
    void    StepPipeline();
    void    UnpausePipeline();
    void    TimeoutHappened(QString msg);
    void    CriticalErrorHappened(QString msg);

private:
    bool    m_isRunning;                        /// Flag, if processing is currently active

    int     CheckParams();
    void    ConnectSignals();
    void    DisconnectSignals();
    void    CheckRequiredFolders();             /// Check for archive folders exist and create if needed
};

#endif // CAMERAPIPELINE_H

