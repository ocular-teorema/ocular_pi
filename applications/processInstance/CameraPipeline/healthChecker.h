#ifndef HEALTHCHECKER_H
#define HEALTHCHECKER_H

#include <QMap>
#include <QTimer>
#include <QObject>

/*
 * Class for pipeline health check (hang detection)
 */
class HealthChecker : public QObject
{
    Q_OBJECT
public:
    HealthChecker(QObject* parent = NULL);
    ~HealthChecker();

    void RemoveEntry(const char *name);

signals:
    void TimeoutDetected(QString message);

public slots:
    void Pong(const char* entryName, int timeoutMs);
    void Start();
    void Stop();

private slots:
    void CheckHealth();
    void CheckStartPipelineTimeout();

private:
    QTimer*     m_pCheckTimer;

    class HealthCheckEntry
    {
    public:
        HealthCheckEntry(QString n, QString m, int t)
        {
            name = n;
            message = m;
            timeoutMsec = t;
            lastPing = -1;
        }

        QString  name;
        QString  message;
        int      timeoutMsec;
        int64_t  lastPing;
    };

    QMap<QString, HealthCheckEntry*>    m_entries;
};


#endif // HEALTHCHECKER_H
