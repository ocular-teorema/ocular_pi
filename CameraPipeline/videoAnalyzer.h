#ifndef VIDEOANALYZER_H
#define VIDEOANALYZER_H

#include <QList>
#include <QTimer>
#include <QQueue>
#include <QThread>
#include <QObject>

#include "networkUtils/dataDirectory.h"
#include "pipelineCommonTypes.h"

#include "objectDetector.h"
#include "motionAnalysis.h"
#include "denoiseFilter.h"
#include "videoScaler.h"

#define MAX_FRAMES_IN_QUEUE 8

class VideoAnalyzer : public QObject
{
    Q_OBJECT
public:
    VideoAnalyzer();
    ~VideoAnalyzer();

    AnalysisResults     currentResults;                                     /// Analyis results here available for other classes

signals:
    void    AnalysisFinished(QSharedPointer<VideoFrame> pCurrentFrame, AnalysisResults *results);
    void    Ping(const char* name, int timeoutMs);                          /// Ping signal for health checker

public slots:
    void    StartAnalyze();                                                 /// Start processing
    void    StopAnalyze();                                                  /// Stop processing
    void    DoAnalyze();                                                    /// Main processing loop here
    void    EnqueueFrame(QSharedPointer<VideoFrame > pCurrFrame);           /// Add frame to analysis queue
    void    ProcessNewStats(QList<IntervalStatistics *>  curStatsList);     /// For processing period stats

private:
    QMutex                                  m_mutex;
    QQueue<QSharedPointer<VideoFrame > >    m_frameQueue;

    QTimer*                 m_pProcessingTimer;     /// Frame processing timer

    uint64_t                m_frameNumber;
    int                     m_frameStep;

    VideoFrame              m_scaledPrevFrame;      /// Buffer for downscaled previous frame
    VideoBuffer             m_diffBuffer;           /// Buffer for difference calculation (downscaled, luma only)

    VideoScaler             m_scaler;               /// Scaler for yuv frames

    ObjectDetector*         m_pObjectDetector;      /// Object detector and tracker
    MotionEstimator*        m_pMotionEstimator;     /// Motion estimator

    void ProcessFrame(VideoFrame* pCurFrame);       /// Processing algorithms here
};

#endif // VIDEOANALYZER_H
