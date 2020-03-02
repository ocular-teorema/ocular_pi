#ifndef DATADIRECTORY_H
#define DATADIRECTORY_H

#include <QObject>
#include <QMutex>
#include <QTcpServer>
#include <QTcpSocket>
#include <QSharedPointer>

#include "parameters.h"
#include "eventDescription.h"

class DataDirectory : public QObject
{
    Q_OBJECT
public:
    DataDirectory(QSettings& settings);
    ~DataDirectory();

    CommonPipelineParameters     pipelineParams;
    AnalysisParameters           analysisParams;
    EventProcessingParameters    eventParams;

public slots:
   void  eventHandler(EventDescription eventDescription);
   void  errorMessageHandler(int type, QString owner, QString message);

signals:
    void securityReactionEvent(EventDescription eventDescription);

private:
   // External communication
   QTcpServer* mTcpServer;
   QTcpSocket* mTcpSocket;

private slots:
   void onTCPConnect();
   void onTCPRead();
   void onTCPDisconnect();
};

#endif // DATADIRECTORY_H
