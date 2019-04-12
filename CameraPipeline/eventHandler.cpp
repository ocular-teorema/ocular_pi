
#include "eventHandler.h"

static const char* EventReactionStr[5] = {"REACTION_UNSET", "REACTION_IGNORE", "REACTION_ALERT", "REACTION_FALSE"};
static const char* EvenTypeStr[20] = {"", "AREA", "CALIBRATION", "", "MOTION", "", "", "", "VELOCITY", "", "", "", "", "", "", "", "", "", "", ""};


EventHandler::EventHandler() :
    QObject(NULL),
    m_continiousEventId(0)
{
    DEBUG_MESSAGE0("EventHandler", "EventHandler() called");

    pAreaEventHandler = new AreaEventHandler();
    pMotionEventHandler = new MotionEventHandler();
    pCalibEventHandler = new CalibEventHandler();

    QObject::connect(pAreaEventHandler, SIGNAL(EventStarted(EventDescription*)), this, SLOT(OpenEvent(EventDescription*)));
    QObject::connect(pMotionEventHandler, SIGNAL(EventStarted(EventDescription*)), this, SLOT(OpenEvent(EventDescription*)));
    QObject::connect(pCalibEventHandler, SIGNAL(EventStarted(EventDescription*)), this, SLOT(OpenEvent(EventDescription*)));

    QObject::connect(pAreaEventHandler, SIGNAL(EventFinished(EventDescription*)), this, SLOT(CloseEvent(EventDescription*)));
    QObject::connect(pMotionEventHandler, SIGNAL(EventFinished(EventDescription*)), this, SLOT(CloseEvent(EventDescription*)));
    QObject::connect(pCalibEventHandler, SIGNAL(EventFinished(EventDescription*)), this, SLOT(CloseEvent(EventDescription*)));
}

EventHandler::~EventHandler()
{
    DEBUG_MESSAGE0("EventHandler", "~EventHandler() called");
    SAFE_DELETE(pAreaEventHandler);
    SAFE_DELETE(pMotionEventHandler);
    SAFE_DELETE(pCalibEventHandler);
    DEBUG_MESSAGE0("EventHandler", "~EventHandler() finished");
}

void EventHandler::ProcessAnalysisResults(VideoFrame *pCurrentFrame, AnalysisResults *pResults)
{
    DEBUG_MESSAGE0("EventHandler", "ProcessAnalysisResults() started");

    // Make a frame copy because we should not draw on input frames
    m_outputFrame.CopyFromVideoFrame(pCurrentFrame);

    // Check for calibration errors
    pCalibEventHandler->ProcessResults(pResults);

    // Check for area violation
    pAreaEventHandler->ProcessResults(pResults);

    // Check for motion vectors violation
    pMotionEventHandler->ProcessResults(pResults);

    DEBUG_MESSAGE0("EventHandler", "ProcessAnalysisResults() finished. Emitting signal");
    // Send marked frame to encoder
    emit ResultsProcessed(&m_outputFrame);
}

void EventHandler::ProcessIntervalStats(QList<IntervalStatistics *> curStatsList)
{
    DEBUG_MESSAGE0("EventHandler", "ProcessIntervalStats() called");
    pAreaEventHandler->ProcessStats(curStatsList);
    pMotionEventHandler->ProcessStats(curStatsList);
    pCalibEventHandler->ProcessStats(curStatsList);
}

void EventHandler::SecurityReaction(EventDescription event)
{
    ERROR_MESSAGE2(ERR_TYPE_MESSAGE, "EventHandler",
                   "Security reaction %s has been received for event id = %d",
                   EventReactionStr[event.reaction + 1], event.id);

    pAreaEventHandler->SecurityReaction(event);
    pMotionEventHandler->SecurityReaction(event);
    pCalibEventHandler->SecurityReaction(event);
}

void EventHandler::NewArchiveFileName(QString newFileName)
{
    DEBUG_MESSAGE1("EventHandler", "NewArchiveFileName received - %s", newFileName.toUtf8().constData());

    pAreaEventHandler->archiveFileName = newFileName;
    pMotionEventHandler->archiveFileName = newFileName;
    pCalibEventHandler->archiveFileName = newFileName;
}

void EventHandler::OpenEvent(EventDescription* event)
{
    event->id = m_continiousEventId++; // Continious numeration for all type of events

    ERROR_MESSAGE3(ERR_TYPE_MESSAGE,
                   "EventHandler",
                   "Event started { id = %d, type = %s, confidence = %d }",
                   event->id, EvenTypeStr[event->eventType], event->confidence);

    emit EventStarted(*event);
}

void EventHandler::CloseEvent(EventDescription* event)
{
    ERROR_MESSAGE5(ERR_TYPE_MESSAGE,
                   "EventHandler",
                   "Event closed { id = %d, type = %s, duration = %lld sec, confidence = %d, reaction = %s }",
                   event->id,
                   EvenTypeStr[event->eventType],
                   event->startTime.secsTo(event->endTime),
                   event->confidence,
                   EventReactionStr[event->reaction + 1]);

    emit EventFinished(*event);
}




SingleEventHandlerBase::SingleEventHandlerBase() :
    QObject(NULL),
    archiveFileName("empty"),
    m_reaction(REACTION_UNSET),
    m_active(false),
    m_confidence(0.0f),
    m_falseAlertsInRow(0),
    m_skipFrames(0)
{

}

SingleEventHandlerBase::~SingleEventHandlerBase()
{

}

void SingleEventHandlerBase::OpenEvent(QDateTime currentTime)
{
    DEBUG_MESSAGE0("SingleEventHandler", "OpenEvent() called");
    if (m_active)
    {
        DEBUG_MESSAGE0("SingleEventHandler", "Trying to open active event");
        return;
    }

    m_active = true;
    m_reaction = REACTION_UNSET;
    m_eventStartTime = currentTime;

    currentEvent.id = -1;
    currentEvent.isActive = true;
    currentEvent.confidence = MAX(0, MIN(100, (int)m_confidence));
    currentEvent.archiveFileName1 = archiveFileName; // Store archive file name, actual for event start
    currentEvent.eventType = m_pDecisionMaker->decision.alertType;
    currentEvent.startTime = currentTime;
    currentEvent.endTime = QDateTime();
    currentEvent.reaction = REACTION_UNSET;

    emit EventStarted(&currentEvent);
}

void SingleEventHandlerBase::CloseEvent(QDateTime currentTime)
{
    DataDirectory* pDataDirectory = DataDirectoryInstance::instance();

    DEBUG_MESSAGE0("SingleEventHandler", "CloseEvent() called");

    if (!m_active)
    {
        DEBUG_MESSAGE0("SingleEventHandler", "Trying to close non-active event");
        return;
    }

    currentEvent.isActive = false;
    currentEvent.reaction = m_reaction;
    currentEvent.endTime = currentTime;
    currentEvent.confidence = std::max(0, std::min(100, (int)m_confidence));
    currentEvent.archiveFileName2 = archiveFileName; // Store archive file name, actual for event end

    if (m_reaction == REACTION_FALSE)
    {
        m_falseAlertsInRow++;
        // How much to wait
        m_skipFrames  = pDataDirectory->eventParams.ignoreInterval / 1000; // Because ignore interval is in milliseconds
        m_skipFrames *= pDataDirectory->pipelineParams.fps;
        m_skipFrames *= m_falseAlertsInRow;
    }
    else
    {
        m_falseAlertsInRow = 0;
        m_skipFrames = 0;
    }

    // Reset current event and other parameters
    m_eventStartTime = QDateTime();
    m_firstAlertReceived = QDateTime(); // Reset first alert time for next alert sequence
    m_lastAlertReceived = QDateTime();  // Reset last alert time for next alert sequence
    m_confidence = 0.0f;
    m_reaction = REACTION_UNSET;
    m_active = false;

    emit EventFinished(&currentEvent);
}

void SingleEventHandlerBase::CalculateConfidence(float currConf)
{
    const float AvgCoeff = 0.2f;
    m_confidence = (1.0f - AvgCoeff) * m_confidence + AvgCoeff * currConf;
}

void SingleEventHandlerBase::ProcessResults(AnalysisResults* pResults)
{
    DataDirectory* pDataDirectory = DataDirectoryInstance::instance();

    QDateTime   currentDateTime  = QDateTime::fromMSecsSinceEpoch(pResults->timestamp);

    int         minEventDuration = pDataDirectory->eventParams.minimalEventDuration;
    int         maxEventDuration = pDataDirectory->eventParams.maximalEventDuration;
    int         idleEventDuration = pDataDirectory->eventParams.idleEventDuration;

    DEBUG_MESSAGE0("SingleEventHandlerBase", "ProcessResults() called");

    if (!currentDateTime.isValid())
    {
        ERROR_MESSAGE0(ERR_TYPE_ERROR, "SingleEventHandlerBase", "Invalid frame timestamp");
        return;
    }

    // Make alert/no alert decision first
    if (!m_pDecisionMaker->ProcessAnalysisResults(pResults))
    {
        return; // Returns false, if no valid statistics present
    }


    // Wait for a while after false alerts
    if (m_skipFrames > 0)
    {
        DEBUG_MESSAGE1("SingleEventHandlerBase", "Waiting for %d frames after false alert", m_skipFrames);
        m_skipFrames--;
        return;
    }

    // Process frame decision
    if (m_pDecisionMaker->decision.alertType > 0)   // If current frame has alert indication
    {
        m_lastAlertReceived = currentDateTime;
        m_firstAlertReceived = (m_firstAlertReceived.isValid()) ? m_firstAlertReceived : currentDateTime;

        // Average current event confidence value
        CalculateConfidence(m_pDecisionMaker->decision.confidence);

        // Check for minimum event duration and start new event if OK
        if (m_firstAlertReceived.msecsTo(currentDateTime) > minEventDuration)
        {
            OpenEvent(m_firstAlertReceived);
        }
    }
    else  // If current frame has no alert indication
    {
        if (m_lastAlertReceived.isValid() && m_lastAlertReceived.msecsTo(currentDateTime) > idleEventDuration)
        {
            CloseEvent(m_lastAlertReceived);

            m_firstAlertReceived = QDateTime(); // Reset first alert time for next alert sequence
            m_lastAlertReceived = QDateTime();  // Reset last alert time for next alert sequence
        }
    }

    // Check for maximum event duration and stop event if needed
    if (m_eventStartTime.isValid() && (m_eventStartTime.msecsTo(currentDateTime) > maxEventDuration))
    {
        CloseEvent(currentDateTime);
    }

    // Process security offices reaactions (if any)
    if ((m_reaction == REACTION_ALERT) || (m_reaction == REACTION_FALSE))
    {
        CloseEvent(currentDateTime);
    }
}

void SingleEventHandlerBase::SecurityReaction(EventDescription event)
{
    m_reaction = event.reaction;
}
