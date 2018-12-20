#ifndef EVENTHANDLER_H
#define EVENTHANDLER_H

#include <QObject>

#include "decisionMaker.h"
#include "eventDescription.h"
#include "dbstat/analysisRecordSQliteDao.h"
#include "Generated/eventProcessingParams.h"

#include <QNetworkReply>
#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include "networkUtils/smtp.h"

class SingleEventHandlerBase : public QObject
{
    Q_OBJECT
public:
    SingleEventHandlerBase();
    virtual ~SingleEventHandlerBase();

    EventDescription    currentEvent;           /// Current event parameters
    QString             archiveFileName;        /// Actual archive file name

    void    ProcessResults(AnalysisResults* pResults);
    void    ProcessStats(QList<IntervalStatistics*> curStatsList) { m_pDecisionMaker->ProcessStats(curStatsList); }
    void    SecurityReaction(EventDescription event);

signals:
    void EventStarted(EventDescription* eventDescription);
    void EventFinished(EventDescription* eventDescription);

protected:
    DecisionMakerBase*          m_pDecisionMaker;       /// Decision maker depending on alert type

private:
    /// Variables for event handling logic
    QDateTime                   m_eventStartTime;       /// When current event started
    QDateTime                   m_lastAlertReceived;    /// When last alert decision has been received
    QDateTime                   m_firstAlertReceived;   /// When first alert decision was received for current period
    ReactionType                m_reaction;             /// Security reaction
    bool                        m_active;               /// is event currently active
    float                       m_confidence;           /// Current event confidence for averaging
    int                         m_falseAlertsInRow;     /// How much false alerts in a row
    int                         m_skipFrames;           /// Time to wait after FALSE alert

    /// Start new event
    void OpenEvent(QDateTime currentTime);

    /// Finish current event
    void CloseEvent(QDateTime currentTime);

    /// Average current decision confidence
    void CalculateConfidence(float currConf);
};

class AreaEventHandler : public SingleEventHandlerBase
{
public:
    AreaEventHandler() : SingleEventHandlerBase()
    {
        m_pDecisionMaker = new AreaDecisionMaker();
    }
    ~AreaEventHandler() { SAFE_DELETE(m_pDecisionMaker); }
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



class EventNotifier : public QObject
{
    Q_OBJECT
public:
    EventNotifier();
    ~EventNotifier();

public slots:
    void    Init();                                     /// Initialization should be performed in the same thread with object itself
    void    DeInit();                                   /// DeInitialization as well
    void    CustomEventNotify(QString description);     /// Send notification about custom pipeline event
    void    SendNotification(EventDescription event);   /// Send e-mail or (and) SMS notifications

private slots:
    void    SslErrors(QNetworkReply *reply, const QList<QSslError> &errors);
    void    RequestFinished(QNetworkReply *reply);

private:
    // Network classes for notification sending
    Smtp*                       m_pEmailServer;
    QNetworkReply*              m_pReply;
    QNetworkRequest*            m_pRequest;
    QNetworkAccessManager*      m_pNetworkManager;
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

public slots:
    void    ProcessAnalysisResults(VideoFrame* pCurrentFrame, AnalysisResults* pResults);
    void    ProcessIntervalStats(QList<IntervalStatistics*> curStatsList);
    void    SecurityReaction(EventDescription eventDescription);
    void    NewArchiveFileName(QString newFileName);    /// Receive file name from StreamRecorder
    void    OpenEvent(EventDescription* event);
    void    CloseEvent(EventDescription* event);

private:
    AnalysisRecordSQLiteDao*    m_DB;                   /// Database instance for storing events

    int                         m_continiousEventId;    /// Events should have continious numeration in DB

    VideoFrame                  m_outputFrame;          /// Copy of frame to draw results and send it to output

    void  DrawRedPoint(VideoFrame* pFrame);             /// Draw red point indicator on output frames
};

#endif // EVENTHANDLER_H
