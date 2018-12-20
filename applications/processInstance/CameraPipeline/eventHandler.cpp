
#include "eventHandler.h"

static const char* EventReactionStr[5] = {"REACTION_UNSET", "REACTION_IGNORE", "REACTION_ALERT", "REACTION_FALSE"};
static const char* EvenTypeStr[20] = {"", "AREA", "CALIBRATION", "", "MOTION", "", "", "", "VELOCITY", "", "", "", "", "", "", "", "", "", "", ""};


EventHandler::EventHandler() :
    QObject(NULL),
    m_continiousEventId(0)
{
    DEBUG_MESSAGE0("EventHandler", "EventHandler() called");

    DataDirectory* pDataDirectory = DataDirectoryInstance::instance();

    QString dbPath = QString::fromStdString(pDataDirectory->getPipelineParams().databasePath());
    m_DB = AnalysisRecordSQLiteDao::Instance(dbPath);

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

    DataDirectory*  pDataDirectory = DataDirectoryInstance::instance();
    Statistics      stats;

    stats.startInterval();

    // Make a frame copy because we should not draw on input frames
    m_outputFrame.CopyFromVideoFrame(pCurrentFrame);

    // Check for calibration errors
    pCalibEventHandler->ProcessResults(pResults);

    // Check for area violation
    pAreaEventHandler->ProcessResults(pResults);

    // Check for motion vectors violation
    pMotionEventHandler->ProcessResults(pResults);

    stats.endInterval("Decision");
    pDataDirectory->addStats(stats);

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
    m_DB->storeEvent(*event);    // Write event to the DB

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
    currentEvent.confidence = MAX(0, MIN(100, (int)m_confidence));
    currentEvent.archiveFileName2 = archiveFileName; // Store archive file name, actual for event end

    if (m_reaction == REACTION_FALSE)
    {
        m_falseAlertsInRow++;
        // How much to wait
        m_skipFrames  = pDataDirectory->getEventParams().ignoreIntervalW() / 1000; // Because ignore interval is in milliseconds
        m_skipFrames *= pDataDirectory->getPipelineParams().fps();
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

    int         minEventDuration = pDataDirectory->getEventParams().minimalEventDurationX();
    int         maxEventDuration = pDataDirectory->getEventParams().maximalEventDurationY();
    int         idleEventDuration = pDataDirectory->getEventParams().idleEventDurationZ();

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





EventNotifier::EventNotifier() :
    QObject(NULL),
    m_pReply(NULL),
    m_pRequest(NULL),
    m_pNetworkManager(NULL)
{

}

void EventNotifier::Init()
{
    DEBUG_MESSAGE0("EventNotifier", "EventNotifier() called");

    QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
    sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);

    m_pEmailServer = new Smtp();

    m_pRequest = new QNetworkRequest();
    m_pRequest->setSslConfiguration(sslConfig);
    m_pNetworkManager = new QNetworkAccessManager();

    QObject::connect(m_pNetworkManager, SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>)),
                     this, SLOT(SslErrors(QNetworkReply*,QList<QSslError>)));

    QObject::connect(m_pNetworkManager, SIGNAL(finished(QNetworkReply*)),
                     this, SLOT(RequestFinished(QNetworkReply*)));
}

void EventNotifier::DeInit()
{
    SAFE_DELETE(m_pEmailServer);
    SAFE_DELETE(m_pRequest);
    SAFE_DELETE(m_pNetworkManager);
}

EventNotifier::~EventNotifier()
{
    SAFE_DELETE(m_pEmailServer);
    SAFE_DELETE(m_pRequest);
    SAFE_DELETE(m_pNetworkManager);
}

void EventNotifier::SslErrors(QNetworkReply *reply, const QList<QSslError> &errors)
{
    for (int i = 0; i < errors.size(); i++)
    {
        ERROR_MESSAGE1(ERR_TYPE_MESSAGE, "EventHandler", "SSL error: %s", errors[i].errorString().toUtf8().constData());
    }
    reply->ignoreSslErrors();
}

void EventNotifier::RequestFinished(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError)
    {
        ERROR_MESSAGE1(ERR_TYPE_MESSAGE, "EventHandler", "Http request error: %s", reply->errorString().toUtf8().constData());
    }
    else
    {
        ERROR_MESSAGE0(ERR_TYPE_MESSAGE, "EventHandler", "SMS sent");
    }
}

void EventNotifier::SendNotification(EventDescription event)
{
    QString  videoUrl;
    QString  message;
    QString  eventText;

    DataDirectory* pDataDirectory = DataDirectoryInstance::instance();

    DEBUG_MESSAGE0("EventNotifier", "SendNotification() called");

    CommonPipelineParameters params = pDataDirectory->getPipelineParams();
    QString  camName = QString::fromStdString(params.cameraName());

    videoUrl.sprintf("http://%s:%d/%s/alertFragments/alert%d.mp4",
                     params.serverAddress().data(),
                     8080,
                     params.pipelineName().data(),
                     event.id);

    // Prepare message
    eventText += (event.eventType & ALERT_TYPE_AREA)               ? "Motion in restricted area "   : "";
    eventText += (event.eventType & ALERT_TYPE_CALIBRATION_ERROR)  ? "Calibration error "           : "";
    eventText += (event.eventType & ALERT_TYPE_INVALID_MOTION)     ? "Invalid motion vector "       : "";
    eventText += (event.eventType & ALERT_TYPE_VELOCITY)           ? "Invalid motion speed "        : "";

    // Inform python environment about event via stdout
    message  = QString("<CamInfoEventMessage>");
    message += QString(" CamId:""%1""").arg(camName);
    message += QString(" eventNum:""%1""").arg(event.id);
    message += QString(" eventId:""%1""").arg(event.eventType);
    message += QString(" eventReaction:""%1""").arg(event.reaction);
    message += QString(" eventConfidence:""%1""").arg(event.confidence);
    message += QString(" text:""%1""").arg(eventText);
    message += QString(" time:""%1""").arg(event.startTime.toString("hh:mm:ss"));
    message += QString(" link:""%1""").arg(videoUrl);
    message += QString("/n");

    // Sending email/sms should be performed here
}

void EventNotifier::CustomEventNotify(QString description)
{
    const int ErrorBufSize = 8192;

    DataDirectory* pDataDirectory = DataDirectoryInstance::instance();
    CommonPipelineParameters params = pDataDirectory->getPipelineParams();

    char buf[ErrorBufSize + 1] = {0};

    QString  camName = QString::fromStdString(params.cameraName());
    QString  emailTo = QString::fromStdString(params.notificationSyserrEmail());
    QString  emailLogin = QString::fromStdString(pDataDirectory->getPipelineParams().notificationSmtpLogin());
    QString  emailServer = QString::fromStdString(pDataDirectory->getPipelineParams().notificationSmtpAddress());
    QString  emailPassword = QString::fromStdString(pDataDirectory->getPipelineParams().notificationSmtpPassword());
    QString  curTime = QDateTime::currentDateTime().toString("dd_MM_yyyy___HH_mm_ss");
    QString  fileName = QString("Err_report_") + curTime + QString(".txt");

    FILE*    f = fopen(fileName.toLatin1().constData(), "w");

    if (NULL != f)
    {
        FILE* pErrLogFile;

        fprintf(f, "%s", description.toUtf8().data());

        pErrLogFile = fopen(ErrorHandler::DefaultLogFileName, "r");

        if (NULL != pErrLogFile)
        {
            int64_t size = 0;

            fseek(pErrLogFile, 0, SEEK_END);
            size = ftell(pErrLogFile);
            fseek(pErrLogFile, MAX(0, size - ErrorBufSize), SEEK_SET);
            if (0 == fread(buf, MIN(size, ErrorBufSize), 1, pErrLogFile))
            {
                ERROR_MESSAGE0(ERR_TYPE_ERROR, "EventHandler", "CustomEventNotify() cannot read ErrorLog file");
            }
        }
        fclose(f);
    }
    else
    {
        ERROR_MESSAGE0(ERR_TYPE_ERROR, "EventHandler", "CustomEventNotify() cannot open error report file");
    }

    if (emailTo.contains('@'))   // Sily check :)
    {
        QString subject = QString("Видеоаналитика: критическая ошибка. Камера %1").arg(camName);
        QString message = QString("Время: %1\n%2\n\nErrorLog.txt:\n%3").arg(curTime, description, QString(buf));
        // Send email
        m_pEmailServer->sendMail(emailServer, emailLogin, emailPassword, emailTo, subject, message);
    }
}

