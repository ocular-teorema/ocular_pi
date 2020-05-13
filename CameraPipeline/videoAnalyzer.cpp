
#include <QThread>
#include <QDateTime>

#include "videoAnalyzer.h"

VideoAnalyzer::VideoAnalyzer() :
    QObject(NULL),
    m_pProcessingTimer(NULL),
    m_pObjectDetector(NULL),
    m_pMotionEstimator(NULL)
{

}

VideoAnalyzer::~VideoAnalyzer()
{
    DEBUG_MESSAGE0("VideoAnalyzer", "~VideoAnalyzer() called");
    SAFE_DELETE(m_pProcessingTimer);
    SAFE_DELETE(m_pObjectDetector);
    SAFE_DELETE(m_pMotionEstimator);
    DEBUG_MESSAGE0("VideoAnalyzer", "~VideoAnalyzer() finished");
}

void VideoAnalyzer::StartAnalyze()
{
    DEBUG_MESSAGE0("VideoAnalyzer", "StartAnalyze() called");

    m_pObjectDetector = new ObjectDetector();

    DataDirectory* pDataDirectory = DataDirectoryInstance::instance();

    // Get fixed step to keep analysis fps as low as possible and save resources
    m_frameStep = 1;
    m_frameNumber = 0;
    while (((double)pDataDirectory->pipelineParams.fps / (double)(m_frameStep + 1)) > 6.99)
    {
        m_frameStep++;
    }
    ERROR_MESSAGE1(ERR_TYPE_MESSAGE, "VideoAnalyzer", "Every %d frame will be analyzed", m_frameStep);

    if (pDataDirectory->analysisParams.differenceBasedAnalysis ||
        pDataDirectory->analysisParams.motionBasedAnalysis)
    {
        m_pProcessingTimer = new QTimer;
        if (NULL == m_pProcessingTimer)
        {
            ERROR_MESSAGE0(ERR_TYPE_CRITICAL, "VideoAnalyzer", "Cannot create processing timer");
            return;
        }

        m_pProcessingTimer->setTimerType(Qt::PreciseTimer);
        m_pProcessingTimer->setInterval(5);
        QObject::connect(m_pProcessingTimer, SIGNAL(timeout()), this, SLOT(DoAnalyze()));
        ERROR_MESSAGE0(ERR_TYPE_MESSAGE, "VideoAnalyzer", "ProcessingTimer is connected. Interval set to 5 ms");

        m_pProcessingTimer->start();
    }
}

void VideoAnalyzer::StopAnalyze()
{
    DEBUG_MESSAGE0("VideoAnalyzer", "Stop() called");
    m_pProcessingTimer->stop();
}

void VideoAnalyzer::EnqueueFrame(QSharedPointer<VideoFrame> pCurrFrame)
{
    DEBUG_MESSAGE1("VideoAnalyzer", "Analysis started, frames in queue = %d", m_frameQueue.size());
    m_mutex.lock();
    if (((++m_frameNumber % m_frameStep) == 0) && m_frameQueue.size() < MAX_FRAMES_IN_QUEUE)
    {
        m_frameQueue.enqueue(pCurrFrame);
    }
    else
    {
        ERROR_MESSAGE0(ERR_TYPE_MESSAGE, "VideoAnalyzer", "Analysis queue is full. Skipping current frame");
    }
    m_mutex.unlock();
}

void VideoAnalyzer::DoAnalyze()
{
    DEBUG_MESSAGE1("VideoAnalyzer", "DoAnalyze called. Frames in queue: %d", m_frameQueue.size());

    QSharedPointer<VideoFrame>  pCurrFrame(NULL);

    m_mutex.lock();
    if (!m_frameQueue.empty())
    {
        pCurrFrame = m_frameQueue.dequeue();
    }
    m_mutex.unlock();

    if (!pCurrFrame.isNull())
    {
        // Zero current results
        currentResults.timestamp = pCurrFrame->userTimestamp;
        currentResults.percentMotion = 0;
        currentResults.wasCalibrationError = 0;
        currentResults.pCurFlow = NULL;
        currentResults.pDiffBuffer = NULL;
        currentResults.objects.clear();

        // Main analysis here
        ProcessFrame(pCurrFrame.data());

        DEBUG_MESSAGE2("VideoAnalyzer",
                       "Analysis finished. FrameMotion = %f, Objects count = %d",
                       (float)currentResults.percentMotion,
                       currentResults.objects.size());

        // Inform that analysis is done
        emit AnalysisFinished(pCurrFrame, &currentResults);
    }
    // Send ping that analysis is alive and keeps reading frames
    emit Ping("VideoAnalyzer", HANG_TIMEOUT_MSEC);
}

void VideoAnalyzer::ProcessNewStats(QList<IntervalStatistics *> curStatsList)
{
    Q_UNUSED(curStatsList);
}
