#include <QDir>
#include <QTimer>

#include "cameraPipeline.h"
#include "pipelineConfig.h"

CameraPipeline::CameraPipeline(QObject *parent) :
    QObject(parent),
    m_isRunning(false)
{
    // AVlib common initialization
    // av_register_all(); // Deprecated since ffmpeg 4.x
    avformat_network_init();

    DataDirectory* pDataDirectory = DataDirectoryInstance::instance();
    ErrorHandler*  pErrorHandler = ErrorHandler::instance();

    // Post error messages to DataDirectory
    QObject::connect(pErrorHandler, SIGNAL(Message(int, QString, QString)),
                     pDataDirectory, SLOT(errorMessageHandler(int, QString, QString)));

    // Error handler also has functionality to report on critical errors
    QObject::connect(pErrorHandler, SIGNAL(CriticalError(QString)), this, SLOT(CriticalErrorHappened(QString)));

    // Check for archive folders exist
    CheckRequiredFolders();

    DEBUG_MESSAGE0("CameraPipeline", "CameraPipeline constructor called\n");

    // Creating threads interface
    pCaptureThread = new QThread();
    pRecorderThread = new QThread();
    pStatisticThread = new QThread();
    pProcessingThread = new QThread();
    pHealthCheckThread = new QThread();

    QObject::connect(pCaptureThread, SIGNAL(finished()), pCaptureThread, SLOT(deleteLater()));
    QObject::connect(pRecorderThread, SIGNAL(finished()), pRecorderThread, SLOT(deleteLater()));
    QObject::connect(pStatisticThread, SIGNAL(finished()), pStatisticThread, SLOT(deleteLater()));
    QObject::connect(pProcessingThread, SIGNAL(finished()), pProcessingThread, SLOT(deleteLater()));
    QObject::connect(pHealthCheckThread, SIGNAL(finished()), pHealthCheckThread, SLOT(deleteLater()));

    //
    // Create Objects in main processing thread
    //

    // Analyzer object
    pVideoAnalyzer = new VideoAnalyzer();
    pVideoAnalyzer->moveToThread(pProcessingThread);
    QObject::connect(pProcessingThread, SIGNAL(started()), pVideoAnalyzer, SLOT(StartAnalyze()));
    QObject::connect(pProcessingThread, SIGNAL(finished()), pVideoAnalyzer, SLOT(deleteLater()));

    // Interval statistic handler object
    pVideoStatistics = new VideoStatistics();
    pVideoStatistics->moveToThread(pProcessingThread);
    QObject::connect(pProcessingThread, SIGNAL(finished()), pVideoStatistics, SLOT(deleteLater()));

    // Event handler object
    pEventHandler = new EventHandler();
    pEventHandler->moveToThread(pProcessingThread);
    QObject::connect(pProcessingThread, SIGNAL(finished()), pEventHandler, SLOT(deleteLater()));

    // Stream output objects
    pSourceOutput = new ResultVideoOutput(pDataDirectory->pipelineParams.sourceOutputUrl);
    pSourceOutput->moveToThread(pProcessingThread);
    QObject::connect(pProcessingThread, SIGNAL(finished()), pSourceOutput, SLOT(deleteLater()));

    pResultOutput = new ResultVideoOutput(pDataDirectory->pipelineParams.outputUrl);
    pResultOutput->moveToThread(pProcessingThread);
    QObject::connect(pProcessingThread, SIGNAL(finished()), pResultOutput, SLOT(deleteLater()));

    if (!pDataDirectory->pipelineParams.smallOutputUrl.trimmed().isEmpty())
    {
        pSmallStreamOutput = new ResultVideoOutput(pDataDirectory->pipelineParams.smallOutputUrl);
        pSmallStreamOutput->moveToThread(pProcessingThread);
        QObject::connect(pProcessingThread, SIGNAL(finished()), pSmallStreamOutput, SLOT(deleteLater()));
    }
    else
    {
        pSmallStreamOutput = NULL;
    }

    //
    // Create Objects in other threads
    //

    // Now we can create Capture object
    // Move pRtspCapture object to its thread and connect proper signals
    pRtspCapture = new RTSPCapture(pDataDirectory->pipelineParams.inputStreamUrl);
    pRtspCapture->moveToThread(pCaptureThread);
    QObject::connect(pCaptureThread, SIGNAL(started()),  pRtspCapture,   SLOT(StartCapture())); // Start processing, when thread starts
    QObject::connect(pCaptureThread, SIGNAL(finished()), pRtspCapture,   SLOT(deleteLater()));  // Delete object after thread is finished

    if (!pDataDirectory->pipelineParams.smallStreamUrl.trimmed().isEmpty())
    {
        pRtspSmallStreamCapture = new RTSPCapture(pDataDirectory->pipelineParams.smallStreamUrl);
        pRtspSmallStreamCapture->moveToThread(pCaptureThread);
        QObject::connect(pCaptureThread, SIGNAL(started()),  pRtspSmallStreamCapture, SLOT(StartCapture()));
        QObject::connect(pCaptureThread, SIGNAL(finished()), pRtspSmallStreamCapture, SLOT(deleteLater()));
    }
    else
    {
        pRtspSmallStreamCapture = NULL;
    }

    // Statistic DB interface object
    // Slow operations on statistic DB moved to separate thread
    pStatisticDBIntf = new StatisticDBInterface();
    qRegisterMetaType< QList<IntervalStatistics* > >("QList<IntervalStatistics* >");
    pStatisticDBIntf->moveToThread(pStatisticThread);
    QObject::connect(pStatisticThread, SIGNAL(started()),  pStatisticDBIntf, SLOT(OpenDB()));
    QObject::connect(pStatisticThread, SIGNAL(finished()), pStatisticDBIntf, SLOT(deleteLater()));
    QObject::connect(pStatisticThread, SIGNAL(finished()), pStatisticThread, SLOT(deleteLater()));

    // Stream recorder object
    // Writing archive and event files to HDD in separate thread
    pStreamRecorder = new StreamRecorder();
    pStreamRecorder->moveToThread(pRecorderThread);
    QObject::connect(pRecorderThread, SIGNAL(started()),  pStreamRecorder, SLOT(Open()));
    QObject::connect(pRecorderThread, SIGNAL(finished()), pStreamRecorder, SLOT(deleteLater()));

    // Health checker
    // And the las thread if for pipeline health checker
    pHealthChecker = new HealthChecker();
    pHealthChecker->moveToThread(pHealthCheckThread);
    QObject::connect(pHealthCheckThread, SIGNAL(started()),  pHealthChecker,     SLOT(Start()));
    QObject::connect(pHealthCheckThread, SIGNAL(finished()), pHealthChecker,     SLOT(deleteLater()));

    //
    // Connect all signals
    //
    ConnectSignals();

#ifdef QT_DEBUG
    //av_log_set_level(AV_LOG_DEBUG);
#else
    av_log_set_level(AV_LOG_ERROR);
#endif
}

CameraPipeline::~CameraPipeline()
{
    DEBUG_MESSAGE0("CameraPipeline", "~CameraPipeline() called");

    // Stop all processing threads
    pCaptureThread->quit();
    if (!pCaptureThread->wait(500))
        pCaptureThread->terminate();

    pProcessingThread->quit();
    if (!pProcessingThread->wait(500))
        pProcessingThread->terminate();

    pHealthCheckThread->quit();
    if (!pHealthCheckThread->wait(500))
        pHealthCheckThread->terminate();

    pStatisticThread->quit();
    if (!pStatisticThread->wait(500))
        pStatisticThread->terminate();

    pRecorderThread->quit();
    if (!pRecorderThread->wait(500))
        pRecorderThread->terminate();

    DEBUG_MESSAGE0("CameraPipeline", "~CameraPipeline() finished");
}

void CameraPipeline::ConnectSignals()
{
    ErrorHandler*  pErrorHandler = ErrorHandler::instance();
    DataDirectory* pDataDirectory = DataDirectoryInstance::instance();

    // Connect all objects that needs to be regulary checked for their activity
    QObject::connect(pRtspCapture,     SIGNAL(Ping(const char*, int)), pHealthChecker, SLOT(Pong(const char*, int)));
    QObject::connect(pStreamRecorder,  SIGNAL(Ping(const char*, int)), pHealthChecker, SLOT(Pong(const char*, int)));
    QObject::connect(pVideoAnalyzer,   SIGNAL(Ping(const char*, int)), pHealthChecker, SLOT(Pong(const char*, int)));
    QObject::connect(pStatisticDBIntf, SIGNAL(Ping(const char*, int)), pHealthChecker, SLOT(Pong(const char*, int)));

    QObject::connect(this, SIGNAL(ProcessingStopped()), pRtspCapture, SLOT(StopCapture()));
    QObject::connect(this, SIGNAL(ProcessingStopped()), pVideoAnalyzer, SLOT(StopAnalyze()));
    QObject::connect(this, SIGNAL(ProcessingStopped()), pSourceOutput, SLOT(Close()));
    QObject::connect(this, SIGNAL(ProcessingStopped()), pResultOutput, SLOT(Close()));
    QObject::connect(this, SIGNAL(ProcessingStopped()), pStreamRecorder, SLOT(Close()));
    QObject::connect(this, SIGNAL(ProcessingStopped()), pHealthChecker, SLOT(Stop()));

    if (NULL != pSmallStreamOutput && NULL != pRtspSmallStreamCapture)
    {
        QObject::connect(this, SIGNAL(ProcessingStopped()), pSmallStreamOutput, SLOT(Close()));
        QObject::connect(this, SIGNAL(ProcessingStopped()), pRtspSmallStreamCapture, SLOT(StopCapture()));
    }

    // We should restart on timeout erros and fire notifications!
    QObject::connect(pHealthChecker, SIGNAL(TimeoutDetected(QString)), this, SLOT(TimeoutHappened(QString)));

    // Event handler also has functionality to report on critical errors
    QObject::connect(pErrorHandler, SIGNAL(CriticalError(QString)), this, SLOT(CriticalErrorHappened(QString)));

    // Source stream output and stream recorder will work for both modes (record-only and analysis)
    QObject::connect(pRtspCapture, SIGNAL(NewCodecParams(AVStream*, AVStream*)),
                     pSourceOutput, SLOT(Open(AVStream*, AVStream*)));

    QObject::connect(pRtspCapture, SIGNAL(NewCodecParams(AVStream*, AVStream*)),
                     pResultOutput, SLOT(Open(AVStream*, AVStream*)));

    QObject::connect(pRtspCapture, SIGNAL(NewCodecParams(AVStream*, AVStream*)),
                     pStreamRecorder, SLOT(CopyCodecParameters(AVStream*, AVStream*)));

    qRegisterMetaType< QSharedPointer<AVPacket > >("QSharedPointer<AVPacket >");
    QObject::connect(pRtspCapture, SIGNAL(NewPacketReceived(QSharedPointer<AVPacket>)),
                     pSourceOutput, SLOT(WritePacket(QSharedPointer<AVPacket>)));

    qRegisterMetaType< QSharedPointer<AVPacket > >("QSharedPointer<AVPacket >");
    QObject::connect(pRtspCapture, SIGNAL(NewPacketReceived(QSharedPointer<AVPacket>)),
                     pResultOutput, SLOT(WritePacket(QSharedPointer<AVPacket>)));

    QObject::connect(pRtspCapture, SIGNAL(NewPacketReceived(QSharedPointer<AVPacket>)),
                     pStreamRecorder, SLOT(WritePacket(QSharedPointer<AVPacket>)));
    QObject::connect(pRtspCapture, SIGNAL(NewAudioPacketReceived(QSharedPointer<AVPacket>)),
                     pStreamRecorder, SLOT(WriteAudioPacket(QSharedPointer<AVPacket>)));

    if (NULL != pSmallStreamOutput && NULL != pRtspSmallStreamCapture)
    {
        QObject::connect(pRtspSmallStreamCapture, SIGNAL(NewCodecParams(AVStream*)),
                         pSmallStreamOutput, SLOT(Open(AVStream*)));

        QObject::connect(pRtspSmallStreamCapture, SIGNAL(NewPacketReceived(QSharedPointer<AVPacket>)),
                         pSmallStreamOutput, SLOT(WritePacket(QSharedPointer<AVPacket>)));
    }

    // Special case if we do not have any analysis at all (record only)
    if (!pDataDirectory->analysisParams.differenceBasedAnalysis &&
        !pDataDirectory->analysisParams.motionBasedAnalysis)
    {
        QObject::connect(pRtspCapture, SIGNAL(NewPacketReceived(QSharedPointer<AVPacket>)),
                         pVideoStatistics, SLOT(ProcessSourcePacket(QSharedPointer<AVPacket>)));

        QObject::connect(pStreamRecorder, SIGNAL(NewFileOpened(QString)),
                         pStatisticDBIntf, SLOT(NewArchiveFileName(QString)));

        QObject::connect(pVideoStatistics, SIGNAL(StatisticPeriodReady(IntervalStatistics*)),
                         pStatisticDBIntf, SLOT(PerformWriteStatistic(IntervalStatistics*)));
    }
    else // If we have any analysis on
    {
        qRegisterMetaType< QSharedPointer<VideoFrame > >("QSharedPointer<VideoFrame >");

        // Interaction between FrameBuffer and Analyzer module
        QObject::connect(pRtspCapture, SIGNAL(NewFrameDecoded(QSharedPointer<VideoFrame >)),
                         pVideoAnalyzer, SLOT(EnqueueFrame(QSharedPointer<VideoFrame >)), Qt::DirectConnection);

        // Inform EventHandler and VideoStatistics that new archive file is started
        QObject::connect(pStreamRecorder, SIGNAL(NewFileOpened(QString)),
                         pStatisticDBIntf, SLOT(NewArchiveFileName(QString)));

        QObject::connect(pStreamRecorder, SIGNAL(NewFileOpened(QString)),
                         pEventHandler, SLOT(NewArchiveFileName(QString)));

        // Interaction between Video analyzer and Statistics module
        QObject::connect(pVideoAnalyzer, SIGNAL(AnalysisFinished(QSharedPointer<VideoFrame >, AnalysisResults*)),
                         pVideoStatistics, SLOT(ProcessAnalyzedFrame(QSharedPointer<VideoFrame >, AnalysisResults*)));

        // Update heatmap (for false alerts)
        QObject::connect(pEventHandler, SIGNAL(EventDiffBufferUpdate(VideoBuffer*)),
                         pVideoStatistics, SLOT(AddFalseEventDiffBuffer(VideoBuffer*)));

        // Interaction between IntervalStatistics and StatisticDBInterface
        QObject::connect(pVideoStatistics, SIGNAL(StatisticPeriodStarted(QDateTime)),
                         pStatisticDBIntf, SLOT(PerformGetStatistic(QDateTime)));

        QObject::connect(pVideoStatistics, SIGNAL(StatisticPeriodReady(IntervalStatistics*)),
                         pStatisticDBIntf, SLOT(PerformWriteStatistic(IntervalStatistics*)));

        // Interaction between Video analyzer and Decision makers
        QObject::connect(pVideoAnalyzer, SIGNAL(AnalysisFinished(QSharedPointer<VideoFrame >, AnalysisResults*)),
                         pEventHandler, SLOT(ProcessAnalysisResults(QSharedPointer<VideoFrame >,AnalysisResults*)));

        // Inform DecisionMaker, that new period statistics received from DB
        QObject::connect(pStatisticDBIntf, SIGNAL(NewPeriodStatistics(QList<IntervalStatistics*>)),
                         pEventHandler, SLOT(ProcessIntervalStats(QList<IntervalStatistics*>)));

        // Store all events in database
        QObject::connect(pEventHandler,  SIGNAL(EventFinished(EventDescription)),
                         pStatisticDBIntf, SLOT(StoreEvent(EventDescription)));

        // Interaction between Event handler and frontend (via DataDirectory)
        qRegisterMetaType<EventDescription>("EventDescription");
        QObject::connect(pEventHandler,  SIGNAL(EventStarted(EventDescription)),
                         pDataDirectory, SLOT  (eventHandler(EventDescription)));

        QObject::connect(pEventHandler,  SIGNAL(EventFinished(EventDescription)),
                         pDataDirectory, SLOT  (eventHandler(EventDescription)));

        QObject::connect(pDataDirectory, SIGNAL(securityReactionEvent(EventDescription)),
                         pEventHandler,  SLOT  (SecurityReaction(EventDescription)));

        // Interaction between Event handler and event notifier
        QObject::connect(pEventHandler,  SIGNAL(EventFinished(EventDescription)),
                         pStreamRecorder, SLOT(WriteEventFile(EventDescription)));
    }
}

void CameraPipeline::StopPipeline()
{
    ERROR_MESSAGE0(ERR_TYPE_WARNING, "CameraPipeline","CameraPipeline StopPipeline() called");

    if (m_isRunning)
    {
        m_isRunning = false;
        // Tell, that we are through
        emit ProcessingStopped();
        QThread::msleep(5000); // sleep for a while
    }
}

void CameraPipeline::PausePipeline()
{
    QMetaObject::invokeMethod(pRtspCapture, "SetPause", Qt::QueuedConnection, Q_ARG(bool, true));
}

void CameraPipeline::StepPipeline()
{
    QMetaObject::invokeMethod(pRtspCapture, "StepFrame", Qt::QueuedConnection);
}

void CameraPipeline::UnpausePipeline()
{
    QMetaObject::invokeMethod(pRtspCapture, "SetPause", Qt::QueuedConnection, Q_ARG(bool, false));
}

void CameraPipeline::TimeoutHappened(QString msg)
{
    static bool wasHere = false;

    if (!wasHere)
    {
        emit TimeoutError(msg);

        // Stop pipeline and restart (from external restart script) in 10 seconds
        QTimer::singleShot(10000, this, SLOT(StopPipeline()));

        wasHere = true;
    }
}

void CameraPipeline::CriticalErrorHappened(QString msg)
{
    // Save current log file
    ERROR_MESSAGE1(ERR_TYPE_ERROR, "CameraPipeline", "CriticalErrorHappened() %s", msg.toUtf8().constData());
    QString  curTime = QDateTime::currentDateTime().toString("dd_MM_yyyy___HH_mm_ss");
    QString  fileName = QString("Err_report_") + curTime + QString(".txt");
    (ErrorHandler::instance())->RenameLogFile(fileName);

    // Stop pipeline and restart (from external restart script) in 10 seconds
    QTimer::singleShot(10000, this, SLOT(StopPipeline()));
}

void CameraPipeline::CheckRequiredFolders()
{
    DataDirectory* pDataDirectory = DataDirectoryInstance::instance();

    QString archiveFolder;
    QString imagesFolder;
    QString debugInfoFolder;

    archiveFolder = pDataDirectory->pipelineParams.archivePath;
    archiveFolder += '/';
    archiveFolder += pDataDirectory->pipelineParams.pipelineName;
    imagesFolder = archiveFolder + QString("/alertFragments");
    debugInfoFolder = archiveFolder + QString("/debugImages");

    QDir archiveDir(archiveFolder);
    QDir imagesDir(imagesFolder);
    QDir debugDir(debugInfoFolder);

    if (!archiveDir.exists())
    {
        archiveDir.mkpath(".");
    }

    if (!imagesDir.exists())
    {
        imagesDir.mkpath(".");
    }

    if (!debugDir.exists() && pDataDirectory->analysisParams.produceDebug)
    {
        debugDir.mkpath(".");
    }
}

void CameraPipeline::RunPipeline()
{
    // Start all threads
    pHealthCheckThread->start();
    pProcessingThread->start();
    pStatisticThread->start();
    pRecorderThread->start();
    pCaptureThread->start();

    m_isRunning = true;
}
