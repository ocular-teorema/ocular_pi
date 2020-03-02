#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>

#include "errorHandler.h"
#include "dataDirectory.h"

DataDirectory::DataDirectory(QSettings& settings) :
    QObject(NULL),
    mTcpSocket(NULL)
{
    int port = settings.value("Common/Port", 6781).toInt();
    analysisParams.readParameters(settings);
    pipelineParams.readParameters(settings);
    eventParams.readParameters(settings);

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
        QByteArray out(doc.toJson(QJsonDocument::Compact));
        mTcpSocket->write(out.append(QByteArray(4, 0)));
    }
}

void DataDirectory::errorMessageHandler(int type, QString owner, QString message)
{
    if (NULL != mTcpSocket)
    {
        QJsonObject  object;

        object.insert("camId", pipelineParams.cameraName);
        object.insert("errorType", type);
        object.insert("moduleName", owner);
        object.insert("errorMessage", message);

        //ERROR_MESSAGE1(ERR_TYPE_MESSAGE, "DataDirectory", "Writing json to TCP: %s", doc.toJson().constData());
        QJsonDocument doc(object);
        QByteArray out(doc.toJson(QJsonDocument::Compact));
        mTcpSocket->write(out.append(QByteArray(4, 0)));
    }
}
