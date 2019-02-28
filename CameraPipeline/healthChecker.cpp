
#include <QThread>
#include "healthChecker.h"
#include "errorHandler.h"
#include "pipelineConfig.h"
#include "cameraPipelineCommon.h"

HealthChecker::HealthChecker(QObject *parent) :
    QObject(parent),
    m_pCheckTimer(NULL)
{

}

HealthChecker::~HealthChecker()
{
    DEBUG_MESSAGE0("HealthChecker", "~HealthChecker() called");
    DEBUG_MESSAGE0("HealthChecker", "~HealthChecker() finished");
}

void HealthChecker::Start()
{
    // Health check performed on timers' timeout event
    // This made to prevent event loop blocking with while(1)
    // This timer should be created here, because InitCapture function
    // Is called after RTSPCapture object moved to it's separate thread
    m_pCheckTimer = new QTimer;
    m_pCheckTimer->setInterval(HEALTH_CHECK_INTERVAL_SEC*1000);
    QObject::connect(m_pCheckTimer, SIGNAL(timeout()), this, SLOT(CheckHealth()));

    QTimer::singleShot(HANG_TIMEOUT_MSEC, this, SLOT(CheckStartPipelineTimeout()));

    m_pCheckTimer->start();

    ERROR_MESSAGE1(ERR_TYPE_MESSAGE, "HealthChecker",
                   "Check timer created. Interval = %d ms."
                   "Pipeline activity check will be run in 60 sec", m_pCheckTimer->interval());
}

void HealthChecker::Stop()
{
    m_pCheckTimer->stop();
    SAFE_DELETE(m_pCheckTimer);

    qDeleteAll(m_entries);
    m_entries.clear();
}

void HealthChecker::Pong(const char *entryName, int timeoutMs)
{
    if (m_entries.contains(entryName))
    {
        m_entries[entryName]->timeoutMsec = timeoutMs;
        m_entries[entryName]->lastPing = QDateTime::currentMSecsSinceEpoch();
    }
    else // Add new entry
    {
        QString message;
        message.sprintf("Timeout detected in %s no activity for %d ms", entryName, timeoutMs);
        HealthCheckEntry* pNewEntry = new HealthCheckEntry(entryName, message, timeoutMs);
        m_entries.insert(entryName, pNewEntry);
    }
}

void HealthChecker::CheckHealth()
{
    int64_t currentTime = QDateTime::currentMSecsSinceEpoch();

    for (auto it = m_entries.begin(); it != m_entries.end(); it++)
    {
        HealthCheckEntry* pEntry = it.value();

        DEBUG_MESSAGE2("HealthChecker", "Entry %s, ping interval = %ld",
                       pEntry->name.toUtf8().constData(), currentTime - pEntry->lastPing);

        if ((pEntry->lastPing > 0) && ((currentTime - pEntry->lastPing) > pEntry->timeoutMsec))
        {
            ERROR_MESSAGE1(ERR_TYPE_WARNING, "HealthChecker",
                           "Timeout detected in %s", pEntry->name.toUtf8().constData());
            emit TimeoutDetected(pEntry->message);
        }
    }
}

void HealthChecker::CheckStartPipelineTimeout()
{
    DEBUG_MESSAGE1("HealthChecker", "Initial pipeline activity check ThreadID = %p", QThread::currentThreadId());
    if (m_entries.size() == 0)
    {
        // If there are still no pings came from pipeline's blocks - emit timeout signal
        emit TimeoutDetected("No activity in pipeline since 1 min after start. Restart required...");
    }
}
