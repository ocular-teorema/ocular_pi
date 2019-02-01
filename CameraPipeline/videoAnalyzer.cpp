
#include <QThread>
#include <QDateTime>

#include "videoAnalyzer.h"

VideoAnalyzer::VideoAnalyzer(FrameCircularBuffer *pFrameBuffer) :
    QObject(NULL),
    m_pInputFrameBuffer(pFrameBuffer),
    m_pProcessingTimer(NULL),
    m_pMotionEstimator(NULL)
{

}

VideoAnalyzer::~VideoAnalyzer()
{
    DEBUG_MESSAGE0("VideoAnalyzer", "~VideoAnalyzer() called");
    SAFE_DELETE(m_pDenoiseFilter);
    SAFE_DELETE(m_pObjectDetector);
    SAFE_DELETE(m_pMotionEstimator);
    SAFE_DELETE(m_pProcessingTimer);
    DEBUG_MESSAGE0("VideoAnalyzer", "~VideoAnalyzer() finished");
}

void VideoAnalyzer::StartAnalyze()
{
    DEBUG_MESSAGE0("VideoAnalyzer", "Start() called");

    m_pObjectDetector = new ObjectDetector();

    DataDirectory* pDataDirectory = DataDirectoryInstance::instance();

    m_frameInterval = 1000.0/(double)pDataDirectory->pipelineParams.fps;

    // Frame capturing performed on capture timers' timeout event
    // This made to prevent event loop blocking with while(1)
    // This timer should be created here, because InitCapture function
    // Is called after RTSPCapture object moved to it's separate thread
    m_pProcessingTimer = new QTimer;
    m_pProcessingTimer->setTimerType(Qt::PreciseTimer);
    m_pProcessingTimer->setInterval(m_frameInterval);
    QObject::connect(m_pProcessingTimer, SIGNAL(timeout()), this, SLOT(DoAnalyze()));
    ERROR_MESSAGE1(ERR_TYPE_MESSAGE, "VideoAnalyzer", "ProcessingTimer is connected. Interval was set to %f ms", m_frameInterval);

    m_nextFrameTime = 0.0;

    if (NULL == m_pProcessingTimer)
    {
        ERROR_MESSAGE0(ERR_TYPE_CRITICAL, "VideoAnalyzer", "Cannot create processing timer");
        return;
    }

    if (pDataDirectory->analysisParams.differenceBasedAnalysis ||
        pDataDirectory->analysisParams.motionBasedAnalysis)
    {
        m_pProcessingTimer->start();
    }
    else
    {
        ERROR_MESSAGE0(ERR_TYPE_MESSAGE, "VideoAnalyzer", "Record only mode activated. Analysis process timer switched off.");
    }
}

void VideoAnalyzer::StopAnalyze()
{
    DEBUG_MESSAGE0("VideoAnalyzer", "Stop() called");
    if (NULL != m_pProcessingTimer)
    {
        m_pProcessingTimer->stop();
    }
}

void VideoAnalyzer::DoAnalyze()
{
    VideoFrame*     pCurrFrame = NULL;

    DEBUG_MESSAGE1("VideoAnalyzer", "Analysis started, m_nextFrameTime = %f", m_nextFrameTime);

    // Measure interframe interval and log it
    if (m_pInputFrameBuffer == NULL)
    {
        ERROR_MESSAGE0(ERR_TYPE_ERROR, "VideoAnalyzer", "Invalid input frame buffer pointer");
        return;
    }

    // Try to get new frame
    m_pInputFrameBuffer->GetFrame(&pCurrFrame, m_nextFrameTime);

    // Increase next frame time
    m_nextFrameTime += m_frameInterval * 0.001; // in seconds

    // Send ping that analysis is alive and keeps reading frames
    emit Ping("VideoAnalyzer", HANG_TIMEOUT_MSEC);

    if (pCurrFrame != NULL) // If we got new frame for analysis
    {
        // Zero current results
        currentResults.timestamp = pCurrFrame->userTimestamp;
        currentResults.percentMotion = 0;
        currentResults.wasCalibrationError = 0;
        currentResults.pCurFlow = NULL;
        currentResults.pDiffBuffer = NULL;
        currentResults.objects.clear();

        // Main analysis here
        ProcessFrame(pCurrFrame);

        DEBUG_MESSAGE2("VideoAnalyzer",
                       "Analysis finished. FrameMotion = %f, Objects count = %d",
                       (float)currentResults.percentMotion,
                       currentResults.objects.size());

        // Inform that analysis is done
        emit AnalysisFinished(pCurrFrame, &currentResults);
    }
}

void VideoAnalyzer::ProcessNewStats(QList<IntervalStatistics *> curStatsList)
{
    Q_UNUSED(curStatsList);
    m_motionMap.Reset();
}
