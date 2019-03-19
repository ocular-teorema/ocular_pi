#pragma once
#include <QObject>
#include <QList>
#include <QDateTime>
#include <QByteArray>

#include "analysisRecordSQliteDao.h"
#include "networkUtils/dataDirectory.h"
#include "pipelineCommonTypes.h"

/// Сlass for DB related work (will be executed in separate thread)
class StatisticDBInterface : public QObject
{
    Q_OBJECT
public:
    StatisticDBInterface();
    ~StatisticDBInterface();

signals:
    void NewPeriodStatistics(QList<IntervalStatistics* > curStatsList);  /// Inform all about new statistics
    void Ping(const char* name, int timeoutMs);                          /// Ping signal for health checker

public slots:
    void PerformGetStatistic(QDateTime currentDateTime);
    void PerformWriteStatistic(IntervalStatistics* stats);
    void NewArchiveFileName(QString newFileName);

private slots:
    // These functions need to avoid simultaneous DB access
    // when several instances are working on a same server
    // Read and write functions should be called with random delay
    void  DelayedGet();
    void  DelayedWrite();

private:
    QList<IntervalStatistics *> m_currentPeriodStatistic;   /// Current period statistic
    QString                     m_archiveFileName;          /// Actual archive file name from stream recorder

    QDateTime                   m_statisticsToGetTime;
    IntervalStatistics*         m_statisticsToWritePtr;

    QImage CreateHeatmap(VideoBuffer* pBkgrBuffer, AccumlatorBuffer* pAccBuffer);
};

class VideoStatistics : public QObject
{
    Q_OBJECT
public:
    VideoStatistics();
    ~VideoStatistics();

signals:
    void  StatisticPeriodStarted(QDateTime currentDateTime);
    void  StatisticPeriodReady(IntervalStatistics* stats);

public slots:
    void  ProcessAnalyzedFrame(VideoFrame* pCurrentFrame, AnalysisResults* results);

private slots:
    void  IntervalStarted(QDateTime currentTime);
    void  IntervalFinished(QDateTime currentTime);

private:
    VideoFrame*                 m_pCurrentFrame;
    IntervalTimer*              m_pIntervalTimer;           /// Intervals handler
    IntervalStatistics          m_currentStats;             /// Current interval statistic (for averaging)
    IntervalStatistics          m_currentStatsCopy;         /// Copy of current statistic (for passing to DB interface)
};