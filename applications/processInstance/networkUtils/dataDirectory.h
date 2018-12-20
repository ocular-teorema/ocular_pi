#ifndef DATADIRECTORY_H
#define DATADIRECTORY_H

#include <QObject>
#include <QMutex>
#include <QTcpServer>
#include <QTcpSocket>
#include <QSharedPointer>

#include "calculationStats.h"


// And we should abstract ourselfs of the type
#include "Generated/analysisParams.h"
#include "Generated/commonPipelineParameters.h"
#include "Generated/eventProcessingParams.h"

#include "eventDescription.h"

class DataDirectory : public QObject
{
    Q_OBJECT
public:

    DataDirectory(int port);
    virtual ~DataDirectory();
    virtual corecvs::BaseTimeStatisticsCollector getCollector();

public slots:
   AnalysisParams            getParams();
   CommonPipelineParameters  getPipelineParams();
   EventProcessingParams     getEventParams();

   void  setParams(const AnalysisParams &params);
   void  setPipelineParams(const CommonPipelineParameters &params);
   void  setEventParams(const EventProcessingParams &params);
   void  resetStats();
   void  addStats(Statistics stats);
   void  eventHandler(EventDescription eventDescription);

signals:
    void statsUpdated();
    void updatedParams();
    void securityReactionEvent(EventDescription eventDescription);

private:
   const static QString ANALISYS_PARAM_NAME;
   const static QString PIPELINE_PARAM_NAME;
   const static QString EVENT_PARAM_NAME;

   CommonPipelineParameters     pipelineParametes;
   AnalysisParams               analysisParameters;
   EventProcessingParams        eventParameters;

   BaseTimeStatisticsCollector  mStats;

   QMutex dynamicAnalysisParamsMutex;
   QMutex dynamicPipelineParamsMutex;
   QMutex dynamicEventParamsMutex;
   QMutex mStatsLock;

   // External communication
   QTcpServer* mTcpServer;
   QTcpSocket* mTcpSocket;

private slots:
   void onTCPConnect();
   void onTCPRead();
   void onTCPDisconnect();
};

#endif // DATADIRECTORY_H
