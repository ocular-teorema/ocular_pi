#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>

#include "errorHandler.h"
#include "dataDirectory.h"

const QString DataDirectory::ANALISYS_PARAM_NAME = "AnalysisParams";
const QString DataDirectory::PIPELINE_PARAM_NAME = "PipelineParams";
const QString DataDirectory::EVENT_PARAM_NAME    = "EventParams";

AnalysisParams DataDirectory::getParams()
{
    AnalysisParams toReturn;
    dynamicAnalysisParamsMutex.lock();
    toReturn = analysisParameters;
    dynamicAnalysisParamsMutex.unlock();
    return toReturn;
}

CommonPipelineParameters DataDirectory::getPipelineParams()
{
    CommonPipelineParameters toReturn;
    dynamicPipelineParamsMutex.lock();
    toReturn = pipelineParametes;
    dynamicPipelineParamsMutex.unlock();
    return toReturn;
}

EventProcessingParams DataDirectory::getEventParams()
{
    EventProcessingParams toReturn;
    dynamicEventParamsMutex.lock();
    toReturn = eventParameters;
    dynamicEventParamsMutex.unlock();
    return toReturn;
}

void DataDirectory::setParams(const AnalysisParams &params)
{
    dynamicAnalysisParamsMutex.lock();
    analysisParameters = params;
    dynamicAnalysisParamsMutex.unlock();
    emit updatedParams();
}

void DataDirectory::setPipelineParams(const CommonPipelineParameters &params)
{
    dynamicPipelineParamsMutex.lock();
    pipelineParametes = params;
    dynamicPipelineParamsMutex.unlock();
    emit updatedParams();
}

void DataDirectory::setEventParams(const EventProcessingParams &params)
{
    dynamicEventParamsMutex.lock();
    eventParameters = params;
    dynamicEventParamsMutex.unlock();
    emit updatedParams();
}

/*========================================================================================*/

void DataDirectory::resetStats()
{
    mStatsLock.lock();
    mStats.reset();
    mStatsLock.unlock();
    emit statsUpdated();
}

void DataDirectory::addStats(Statistics stats)
{
    mStatsLock.lock();
    mStats.addStatistics(stats);
    mStatsLock.unlock();
    emit statsUpdated();
}

BaseTimeStatisticsCollector DataDirectory::getCollector()
{
    BaseTimeStatisticsCollector toReturn;
    mStatsLock.lock();
    toReturn = mStats;
    mStatsLock.unlock();
    return toReturn;
}

DataDirectory::DataDirectory(int port) :
    QObject(NULL),
    mTcpSocket(NULL)
{
    mTcpServer = new QTcpServer();
    connect(mTcpServer, SIGNAL(newConnection()), this, SLOT(onTCPConnect()));

    // Start TCP Server
    if (!mTcpServer->listen(QHostAddress::AnyIPv4, port))
    {
        ERROR_MESSAGE0(ERR_TYPE_ERROR, "DataDirectory", "TCPServer not started");
    }
    else
    {
        ERROR_MESSAGE1(ERR_TYPE_MESSAGE, "DataDirectory", "TCPServer is listening on %d", port);
    }
}

DataDirectory::~DataDirectory()
{
    if (NULL != mTcpServer)
    {
        delete mTcpServer;
    }
}

/*========================================================================================*/

void DataDirectory::onTCPConnect()
{
    mTcpSocket = mTcpServer->nextPendingConnection();
    mTcpSocket->write("Hello from DataDirectory\r\n");

    connect(mTcpSocket, SIGNAL(readyRead()), this, SLOT(onTCPRead()));
    connect(mTcpSocket, SIGNAL(disconnected()), this, SLOT(onTCPDisconnect()));
    qDebug() << "TCP client connected to the DataDirectory\n";
}

void DataDirectory::onTCPDisconnect()
{
    qDebug() << "TCP client disconnected from the DataDirectory\n";
}

void DataDirectory::onTCPRead()
{
    QByteArray array;
    while (mTcpSocket->bytesAvailable())
    {
        array.append(mTcpSocket->readAll());
    }

    QJsonDocument inJson = QJsonDocument::fromJson(array);
    if (!inJson.isEmpty())
    {
        QJsonObject object = inJson.object();
        if (!object.isEmpty())
        {
            EventDescription descr;
            QVariantMap values = object.toVariantMap();

            descr.id = values["id"].toInt();
            descr.reaction = ((0 == values["reaction"].toInt()) ? REACTION_FALSE : REACTION_ALERT);

            emit securityReactionEvent(descr);
        }
        else
        {
            ERROR_MESSAGE1(ERR_TYPE_ERROR, "DataDirectory", "Invalid object received: %s", array.constData());
        }
    }
    else
    {
        ERROR_MESSAGE1(ERR_TYPE_ERROR, "DataDirectory", "Invalid object received: %s", array.constData());
    }
}


void DataDirectory::eventHandler(EventDescription eventDescription)
{
    QJsonDocument doc = QJsonDocument::fromVariant(eventDescription.toVariant());
    //ERROR_MESSAGE1(ERR_TYPE_MESSAGE, "DataDirectory", "Writing json to TCP: %s", doc.toJson().constData());
    if (NULL != mTcpSocket)
    {
        mTcpSocket->write(doc.toJson(QJsonDocument::Compact));
    }
}
