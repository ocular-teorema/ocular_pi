#include <QDir>
#include <QTimer>

#include "cameraPipeline.h"
#include "pipelineConfig.h"

CameraPipeline::CameraPipeline(QObject *parent) :
    QObject(parent),
    m_isRunning(false)
{
    // AVlib common initialization
    av_register_all();
    avformat_network_init();

    // Probe input stream and replace invalid parameters if any
    if (CAMERA_PIPELINE_OK != CheckParams())
    {
        ERROR_MESSAGE0(ERR_TYPE_ERROR, "CameraPipeline", "Failed to get parameters of input stream. Exiting...");
        return;
    }

    // Check for archive folders exist
    CheckRequiredFolders();

    DataDirectory* pDataDirectory = DataDirectoryInstance::instance();

    QString outputUrl = QString::fromStdString(pDataDirectory->getPipelineParams().outputUrl());
    QString srcOutputUrl = QString::fromStdString(pDataDirectory->getPipelineParams().sourceOutputUrl());

    DEBUG_MESSAGE0("CameraPipeline", "CameraPipeline constructor called\n");

    // Creating threads interface
    pCaptureThread = new QThread();
    pRecorderThread = new QThread();
    pNotifierThread = new QThread();
    pStatisticThread = new QThread();
    pProcessingThread = new QThread();
    pHealthCheckThread = new QThread();

    // Create FrameBuffer to pass it to capture and analyzing objects
    pFrameBuffer = new FrameCircularBuffer(DEFAULT_FRAME_BUFFER_SIZE);

    //
    // Create Objects in main processing thread
    //

    // Analyzer object
    pVideoAnalyzer = new VideoAnalyzer(pFrameBuffer);
    pVideoAnalyzer->moveToThread(pProcessingThread);
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
    pSourceOutput = new ResultVideoOutput(srcOutputUrl);
    pSourceOutput->moveToThread(pProcessingThread);
    QObject::connect(pProcessingThread, SIGNAL(finished()), pSourceOutput, SLOT(deleteLater()));

    pResultOutput = new ResultVideoOutput(outputUrl);
    pResultOutput->moveToThread(pProcessingThread);
    QObject::connect(pProcessingThread, SIGNAL(finished()), pResultOutput, SLOT(deleteLater()));

    //
    // Create Objects in other threads
    //

    // Now we can create Capture object
    // Move pRtspCapture object to its thread and connect proper signals
    pRtspCapture = new RTSPCapture(pFrameBuffer);
    pRtspCapture->moveToThread(pCaptureThread);
    QObject::connect(pCaptureThread, SIGNAL(started()),  pRtspCapture,   SLOT(StartCapture())); // Start processing, when thread starts
    QObject::connect(pCaptureThread, SIGNAL(finished()), pRtspCapture,   SLOT(deleteLater()));  // Delete object after thread is finished

    // Statistic DB interface object
    // Slow operations on statistic DB moved to separate thread
    pStatisticDBIntf = new StatisticDBInterface();
    qRegisterMetaType< QList<IntervalStatistics* > >("QList<IntervalStatistics* >");
    pStatisticDBIntf->moveToThread(pStatisticThread);
    QObject::connect(pStatisticThread, SIGNAL(finished()), pStatisticDBIntf, SLOT(deleteLater()));
    QObject::connect(pStatisticThread, SIGNAL(finished()), pStatisticThread, SLOT(deleteLater()));

    // Stream recorder object
    // Writing archive and event files to HDD in separate thread
    pStreamRecorder = new StreamRecorder();
    pStreamRecorder->moveToThread(pRecorderThread);
    QObject::connect(pRecorderThread, SIGNAL(started()),  pStreamRecorder, SLOT(Open()));
    QObject::connect(pRecorderThread, SIGNAL(finished()), pStreamRecorder, SLOT(deleteLater()));

    // Event notifier object
    // Event notification via email or sms also moved to separate thread
    pNotifier = new EventNotifier();
    qRegisterMetaType< EventDescription >("EventDescription");
    pNotifier->moveToThread(pNotifierThread);
    QObject::connect(pNotifierThread, SIGNAL(started()),  pNotifier,       SLOT(Init()));
    QObject::connect(pNotifierThread, SIGNAL(finished()), pNotifier,       SLOT(deleteLater()));

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

    if (!pProcessingThread->wait(100))
        pProcessingThread->terminate();

    if (!pHealthCheckThread->wait(100))
        pHealthCheckThread->terminate();

    if (!pStatisticThread->wait(100))
        pStatisticThread->terminate();

    if (!pRecorderThread->wait(100))
        pRecorderThread->terminate();

    if (!pNotifierThread->wait(100))
        pNotifierThread->terminate();

    if (!pCaptureThread->wait(100))
        pCaptureThread->terminate();

    delete pCaptureThread;
    delete pNotifierThread;
    delete pRecorderThread;
    delete pStatisticThread;
    delete pProcessingThread;
    delete pHealthCheckThread;

    delete pFrameBuffer;
    DEBUG_MESSAGE0("CameraPipeline", "~CameraPipeline() finished");
}

void CameraPipeline::ConnectSignals()
{
    ErrorHandler*  pErrorHandler = ErrorHandler::instance();
    DataDirectory* pDataDirectory = DataDirectoryInstance::instance();
    AnalysisParams params = pDataDirectory->getParams();

    // Connect all objects that needs to be regulary checked for their activity
    QObject::connect(pRtspCapture,     SIGNAL(Ping(const char*, int)), pHealthChecker, SLOT(Pong(const char*, int)));
    QObject::connect(pStreamRecorder,  SIGNAL(Ping(const char*, int)), pHealthChecker, SLOT(Pong(const char*, int)));
    QObject::connect(pVideoAnalyzer,   SIGNAL(Ping(const char*, int)), pHealthChecker, SLOT(Pong(const char*, int)));
    QObject::connect(pStatisticDBIntf, SIGNAL(Ping(const char*, int)), pHealthChecker, SLOT(Pong(const char*, int)));

    // We should restart on timeout erros and fire notifications!
    QObject::connect(pHealthChecker, SIGNAL(TimeoutDetected(QString)), this, SLOT(TimeoutHappened(QString)));
    QObject::connect(this, SIGNAL(TimeoutError(QString)), pNotifier, SLOT(CustomEventNotify(QString)));

    // Event handler also has functionality to report on critical errors
    QObject::connect(pErrorHandler, SIGNAL(CriticalError(QString)), this, SLOT(CriticalErrorHappened(QString)));

    // Source stream output and stream recorder will work for both modes (record-only and analysis)
    QObject::connect(pRtspCapture, SIGNAL(NewCodecParams(AVStream*)),
                     pSourceOutput, SLOT(Open(AVStream*)));

    QObject::connect(pRtspCapture, SIGNAL(NewCodecParams(AVStream*)),
                     pResultOutput, SLOT(Open(AVStream*)));

    QObject::connect(pRtspCapture, SIGNAL(NewCodecParams(AVStream*)),
                     pStreamRecorder, SLOT(CopyCodecParameters(AVStream*)));

    qRegisterMetaType< QSharedPointer<AVPacket > >("QSharedPointer<AVPacket >");
    QObject::connect(pRtspCapture, SIGNAL(NewPacketReceived(QSharedPointer<AVPacket>)),
                     pSourceOutput, SLOT(WritePacket(QSharedPointer<AVPacket>)));

    qRegisterMetaType< QSharedPointer<AVPacket > >("QSharedPointer<AVPacket >");
    QObject::connect(pRtspCapture, SIGNAL(NewPacketReceived(QSharedPointer<AVPacket>)),
                     pResultOutput, SLOT(WritePacket(QSharedPointer<AVPacket>)));

    QObject::connect(pRtspCapture, SIGNAL(NewPacketReceived(QSharedPointer<AVPacket>)),
                     pStreamRecorder, SLOT(WritePacket(QSharedPointer<AVPacket>)));

    // Special case if we do not have any analysis at all (record only)
    if (params.differenceBasedAnalysis() || params.motionBasedAnalysis())
    {
        // Interaction between FrameBuffer and Analyzer module
        QObject::connect(pFrameBuffer, SIGNAL(FirstFrameAdded()), pVideoAnalyzer, SLOT(StartAnalyze()));

        // Inform EventHandler and VideoStatistics that new archive file is started
        QObject::connect(pStreamRecorder, SIGNAL(NewFileOpened(QString)),
                         pStatisticDBIntf, SLOT(NewArchiveFileName(QString)));

        QObject::connect(pStreamRecorder, SIGNAL(NewFileOpened(QString)),
                         pEventHandler, SLOT(NewArchiveFileName(QString)));

        // Interaction between Video analyzer and Statistics module
        QObject::connect(pVideoAnalyzer, SIGNAL(AnalysisFinished(VideoFrame*, AnalysisResults*)),
                         pVideoStatistics, SLOT(ProcessAnalyzedFrame(VideoFrame*, AnalysisResults*)));

        // Interaction between IntervalStatistics and StatisticDBInterface
        QObject::connect(pVideoStatistics, SIGNAL(StatisticPeriodStarted(QDateTime)),
                         pStatisticDBIntf, SLOT(PerformGetStatistic(QDateTime)));

        QObject::connect(pVideoStatistics, SIGNAL(StatisticPeriodReady(IntervalStatistics*)),
                         pStatisticDBIntf, SLOT(PerformWriteStatistic(IntervalStatistics*)));

        // Interaction between Video analyzer and Decision makers
        QObject::connect(pVideoAnalyzer, SIGNAL(AnalysisFinished(VideoFrame*, AnalysisResults*)),
                         pEventHandler, SLOT(ProcessAnalysisResults(VideoFrame*,AnalysisResults*)));

        // Inform DecisionMaker, that new period statistics received from DB
        QObject::connect(pStatisticDBIntf, SIGNAL(NewPeriodStatistics(QList<IntervalStatistics*>)),
                         pEventHandler, SLOT(ProcessIntervalStats(QList<IntervalStatistics*>)));

        // Interaction between Event handler and frontend (via DataDirectory)
        QObject::connect(pEventHandler,  SIGNAL(EventStarted(EventDescription)),
                         pDataDirectory, SLOT  (eventHandler(EventDescription)));

        QObject::connect(pEventHandler,  SIGNAL(EventFinished(EventDescription)),
                         pDataDirectory, SLOT  (eventHandler(EventDescription)));

        QObject::connect(pDataDirectory, SIGNAL(securityReactionEvent(EventDescription)),
                         pEventHandler,  SLOT  (SecurityReaction(EventDescription)));

        // Interaction between Event handler and event notifier
        QObject::connect(pEventHandler,  SIGNAL(EventFinished(EventDescription)),
                         pStreamRecorder, SLOT(WriteEventFile(EventDescription)));

        QObject::connect(pEventHandler,  SIGNAL(EventFinished(EventDescription)),
                         pNotifier,      SLOT(SendNotification(EventDescription)));
    }
}

void CameraPipeline::DisconnectSignals()
{
    // Disconnect all objects
    pRtspCapture->disconnect();
    pVideoAnalyzer->disconnect();
    pVideoStatistics->disconnect();
    pStatisticDBIntf->disconnect();
    pEventHandler->disconnect();
    pSourceOutput->disconnect();
    pResultOutput->disconnect();
    pNotifier->disconnect();
    pStreamRecorder->disconnect();
    DataDirectoryInstance::instance()->disconnect();
}

void CameraPipeline::StopPipeline()
{
    ERROR_MESSAGE0(ERR_TYPE_WARNING, "CameraPipeline","CameraPipeline StopPipeline() called");

    if (m_isRunning)
    {
        // RtspCapture instance in now running in other thread,
        // So, we cannot just call pRtspCapture->StopCapture(),
        // But we should perform QMetaObject::invokeMethod() instead
        // Also we need to close result output context, stop stream recorder, health checker and event notifier
        QMetaObject::invokeMethod(pVideoAnalyzer, "StopAnalyze", Qt::QueuedConnection);
        QMetaObject::invokeMethod(pRtspCapture,   "StopCapture", Qt::QueuedConnection);
        QMetaObject::invokeMethod(pSourceOutput,  "Close",       Qt::QueuedConnection);
        QMetaObject::invokeMethod(pResultOutput,  "Close",       Qt::QueuedConnection);
        QMetaObject::invokeMethod(pStreamRecorder,"Close",       Qt::QueuedConnection);
        QMetaObject::invokeMethod(pHealthChecker, "Stop",        Qt::QueuedConnection);
        QMetaObject::invokeMethod(pNotifier,      "DeInit",      Qt::QueuedConnection);

        // Stop all processing threads
        pHealthCheckThread->quit();
        pProcessingThread->quit();
        pStatisticThread->quit();
        pRecorderThread->quit();
        pNotifierThread->quit();
        pCaptureThread->quit();

        m_isRunning = false;

        // Tell, that we are through
        emit ProcessingStopped();
    }
    QThread::msleep(1000); // Waiting here is just in case
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
    // Send special notification
    QMetaObject::invokeMethod(pNotifier, "CustomEventNotify", Qt::QueuedConnection, Q_ARG(QString, msg));

    // Stop pipeline and restart (from external restart script) in 10 seconds
    QTimer::singleShot(10000, this, SLOT(StopPipeline()));
}

int CameraPipeline::CheckParams()
{
    DataDirectory*            pDataDirectory = DataDirectoryInstance::instance();
    AnalysisParams            params = pDataDirectory->getParams();
    CommonPipelineParameters  commonParams = pDataDirectory->getPipelineParams();

    if (commonParams.fps() <= 0 || params.downscaleCoeff() <= 0) {

        int ret;
        int w = 1280;
        int h = 720;
        double fps = 10.0;

        ERROR_MESSAGE0(ERR_TYPE_MESSAGE, "CameraPipeline", "Probing stream fps and size...");
        ret = RTSPCapture::ProbeStreamParameters(commonParams.inputStreamUrl().c_str(), &fps, &w, &h);

        if (ret != CAMERA_PIPELINE_OK)
        {
            return ret;
        }

        ERROR_MESSAGE3(ERR_TYPE_MESSAGE, "CameraPipeline", "Detected fps = %f, size = %dx%d", fps, w, h);

        if (commonParams.fps() <= 0)
        {
            commonParams.setFps(fps);
            pDataDirectory->setPipelineParams(commonParams);
        }

        if (params.downscaleCoeff() <= 0)
        {
            double coeff = 0.4;
            coeff = (w > 720)  ? 0.3  : coeff;
            coeff = (w > 1200) ? 0.25 : coeff;
            coeff = (w > 1900) ? 0.15 : coeff;
            params.setDownscaleCoeff(coeff);
            pDataDirectory->setParams(params);
        }

    }
    return CAMERA_PIPELINE_OK;
}

void CameraPipeline::CheckRequiredFolders()
{
    DataDirectory* pDataDirectory = DataDirectoryInstance::instance();

    QString archiveFolder;
    QString imagesFolder;

    archiveFolder = QString::fromStdString(pDataDirectory->getPipelineParams().archivePath());
    archiveFolder += '/';
    archiveFolder += QString::fromStdString(pDataDirectory->getPipelineParams().pipelineName());
    imagesFolder = archiveFolder + QString("/alertFragments");

    QDir archiveDir(archiveFolder);
    QDir imagesDir(imagesFolder);

    if (!archiveDir.exists())
    {
        archiveDir.mkpath(".");
    }

    if (!imagesDir.exists())
    {
        imagesDir.mkpath(".");
    }
}

void CameraPipeline::RunPipeline()
{
    // Start all threads
    pHealthCheckThread->start();
    pProcessingThread->start();
    pStatisticThread->start();
    pRecorderThread->start();
    pNotifierThread->start();
    pCaptureThread->start();

    m_isRunning = true;
}
