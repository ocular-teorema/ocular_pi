#ifndef VIDEOANALYZER_H
#define VIDEOANALYZER_H

#include <QList>
#include <QTimer>
#include <QThread>
#include <QObject>

#include "networkUtils/dataDirectory.h"
#include "pipelineCommonTypes.h"

#include "objectDetector.h"
#include "motionAnalysis.h"
#include "denoiseFilter.h"
#include "frameCircularBuffer.h"

class VideoAnalyzer : public QObject
{
    Q_OBJECT
public:
    VideoAnalyzer(FrameCircularBuffer* pFrameBuffer);
    ~VideoAnalyzer();

    AnalysisResults     currentResults;                                     /// Analyis results here available for other classes

signals:
    void    AnalysisFinished(VideoFrame* pCurrentFrame, AnalysisResults *results);
    void    Ping(const char* name, int timeoutMs);                          /// Ping signal for health checker

public slots:
    void    StartAnalyze();                                                 /// Start processing
    void    StopAnalyze();                                                  /// Stop processing
    void    DoAnalyze();                                                    /// Main processing loop here
    void    ProcessNewStats(QList<IntervalStatistics *>  curStatsList);     /// For processing period stats

private:
    FrameCircularBuffer*    m_pInputFrameBuffer;    /// Pointer to buffer with input frames. Must be set from outside!
    QTimer*                 m_pProcessingTimer;     /// Frame processing timer

    double                  m_frameInterval;        /// Target distance between frames
    double                  m_nextFrameTime;        /// Relative time in seconds of next frame

    VideoFrame              m_scaledPrevFrame;      /// Buffer for downscaled previous frame
    VideoBuffer             m_diffBuffer;           /// Buffer for difference calculation (downscaled, luma only)

    VideoScaler             m_scaler;               /// Scaler for yuv frames

    ObjectDetector*         m_pObjectDetector;      /// Object detector and tracker
    MotionEstimator*        m_pMotionEstimator;     /// Motion estimator

    void ProcessFrame(VideoFrame* pCurFrame);    /// Processing algorithms here
};

#endif // VIDEOANALYZER_H
