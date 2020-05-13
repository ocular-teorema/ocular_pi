#ifndef EVENTHANDLER_H
#define EVENTHANDLER_H

#include <QObject>

#include "decisionMaker.h"
#include "eventDescription.h"
#include "dbstat/analysisRecordSQliteDao.h"

#include <QNetworkReply>
#include <QNetworkRequest>
#include <QNetworkAccessManager>

class SingleEventHandlerBase : public QObject
{
    Q_OBJECT
public:
    SingleEventHandlerBase();
    virtual ~SingleEventHandlerBase();

    EventDescription    currentEvent;           /// Current event parameters
    QString             archiveFileName;        /// Actual archive file name

    virtual void ProcessResults(AnalysisResults* pResults);

    void ProcessStats(QList<IntervalStatistics*> curStatsList) { m_pDecisionMaker->ProcessStats(curStatsList); }
    void SecurityReaction(EventDescription event);

signals:
    void EventStarted(EventDescription* eventDescription);
    void EventFinished(EventDescription* eventDescription);

protected:
    DecisionMakerBase*          m_pDecisionMaker;       /// Decision maker depending on alert type

    /// Start new event
    void OpenEvent(QDateTime currentTime);

    /// Finish current event
    void CloseEvent(QDateTime currentTime);

    /// Average current decision confidence
    void CalculateConfidence(float currConf);

    /// Variables for event handling logic
    QDateTime                   m_eventStartTime;       /// When current event started
    QDateTime                   m_lastAlertReceived;    /// When last alert decision has been received
    QDateTime                   m_firstAlertReceived;   /// When first alert decision was received for current period
    ReactionType                m_reaction;             /// Security reaction
    bool                        m_active;               /// is event currently active
    float                       m_confidence;           /// Current event confidence for averaging
    int                         m_falseAlertsInRow;     /// How much false alerts in a row
    int                         m_skipFrames;           /// Time to wait after FALSE alert
};

class AreaEventHandler : public SingleEventHandlerBase
{
    Q_OBJECT
public:
    AreaEventHandler() : SingleEventHandlerBase()
    {
        m_isEventActive = false;
        m_pDecisionMaker = new AreaDecisionMaker();
    }
    ~AreaEventHandler() { SAFE_DELETE(m_pDecisionMaker); }

    void ProcessResults(AnalysisResults* pResults);

signals:
    void UpdateDiffBuffer(VideoBuffer* pBuffer);

private:
    bool m_isEventActive;
    VideoBuffer m_eventDiffBuffer;
};

class MotionEventHandler : public SingleEventHandlerBase
{
public:
    MotionEventHandler() : SingleEventHandlerBase()
    {
        m_pDecisionMaker = new MotionDecisionMaker();
    }
    ~MotionEventHandler() { SAFE_DELETE(m_pDecisionMaker); }
};

class CalibEventHandler : public SingleEventHandlerBase
{
public:
    CalibEventHandler() : SingleEventHandlerBase()
    {
        m_pDecisionMaker = new CalibDecisionMaker();
    }
    ~CalibEventHandler() { SAFE_DELETE(m_pDecisionMaker); }
};

class EventHandler : public QObject
{
    Q_OBJECT
public:
    EventHandler();
    ~EventHandler();

    AreaEventHandler*       pAreaEventHandler;
    MotionEventHandler*     pMotionEventHandler;
    CalibEventHandler*      pCalibEventHandler;

signals:
    void    ResultsProcessed(VideoFrame* pFrame);
    void    EventStarted(EventDescription eventDescription);
    void    EventFinished(EventDescription eventDescription);
    void    NeedWriteEventFile(int64_t startTime, QString fileName);
    void    EventDiffBufferUpdate(VideoBuffer* eventDiff);

public slots:
    void    ProcessAnalysisResults(QSharedPointer<VideoFrame> pCurrentFrame, AnalysisResults* pResults);
    void    ProcessIntervalStats(QList<IntervalStatistics*> curStatsList);
    void    SecurityReaction(EventDescription eventDescription);
    void    NewArchiveFileName(QString newFileName);    /// Receive file name from StreamRecorder
    void    OpenEvent(EventDescription* event);
    void    CloseEvent(EventDescription* event);

private slots:
    void DiffBufferUpdateSlot(VideoBuffer* eventDiff);

private:
    int                         m_continiousEventId;    /// Events should have continious numeration in DB

    VideoFrame                  m_outputFrame;          /// Copy of frame to draw results and send it to output

    void  DrawRedPoint(VideoFrame* pFrame);             /// Draw red point indicator on output frames
};

#endif // EVENTHANDLER_H
