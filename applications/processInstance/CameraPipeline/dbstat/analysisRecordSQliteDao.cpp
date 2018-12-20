
#include <QDebug>

#include <QBuffer>
#include <QByteArray>
#include <QThread>
#include <QMutex>
#include <QDir>
#include <QFileInfo>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlRecord>
#include <QVariant>

#include "analysisRecordSQliteDao.h"

#define ALIGN_TO_SEC(n)  (((n)/1000)*1000)

AnalysisRecordSQLiteDao* AnalysisRecordSQLiteDao::m_instance = NULL;

AnalysisRecordSQLiteDao::AnalysisRecordSQLiteDao(const QString& dbPath) : m_dbFile(dbPath)
{
    DEBUG_MESSAGE0("AnalysisRecordSQLiteDao", "AnalysisRecordSQLiteDao() called");
    m_DB = QSqlDatabase::addDatabase("QPSQL");
    m_DB.setHostName("localhost");
    m_DB.setPort(5432);
    m_DB.setDatabaseName(dbPath);
    m_DB.setUserName("va");
    m_DB.setPassword("theorema");
    m_DB.open();
    //m_DB = QSqlDatabase::addDatabase("QSQLITE");
    //m_DB.setDatabaseName(m_dbFile.absoluteFilePath());
    //m_DB.open();

    if (!m_DB.isOpen())
    {
        ERROR_MESSAGE1(ERR_TYPE_CRITICAL,
                       "AnalysisRecordSQLiteDao",
                       "Failed to open database: %s",
                       m_DB.lastError().text().toUtf8().constData());
    }
}

AnalysisRecordSQLiteDao::~AnalysisRecordSQLiteDao()
{
    DEBUG_MESSAGE0("AnalysisRecordSQLiteDao", "~AnalysisRecordSQLiteDao() called");
    m_DB.close();
    DEBUG_MESSAGE0("AnalysisRecordSQLiteDao", "~AnalysisRecordSQLiteDao() finished");
}

AnalysisRecordSQLiteDao *AnalysisRecordSQLiteDao::Instance(const QString &dbPath)
{
    static QMutex mutex;
    if (NULL == m_instance)
    {
        mutex.lock();

        m_instance = new AnalysisRecordSQLiteDao(dbPath);

        mutex.unlock();
    }
    return m_instance;
}

void AnalysisRecordSQLiteDao::Drop()
{
    static QMutex mutex;
    mutex.lock();

    delete m_instance;
    m_instance = NULL;

    mutex.unlock();
}

ErrorCode AnalysisRecordSQLiteDao::InsertRecord(const AnalysisRecordModel& record)
{
    QSqlQuery   query;
    QByteArray  heatmap;
    QBuffer     heatmapBuffer(&heatmap);

    query.prepare("INSERT INTO records "
                  "(cam, date, start_time, end_time, media_source, video_archive, heatmap, stats_data) "
                  "VALUES "
                  "(:cam, :date, :startTime, :endTime, :mediaSource, :videoArchive, :heatmap, :statsData)");

    heatmapBuffer.open(QIODevice::WriteOnly);

    // Cam name
    query.bindValue(":cam", (DataDirectoryInstance::instance())->getPipelineParams().pipelineName().c_str());

    // Frame thumbnail (full-size image)
    if (!record.heatmap.isNull())
    {
        record.heatmap.save(&heatmapBuffer, "PNG");
    }
    heatmapBuffer.close();
    query.bindValue(":heatmap", QString(heatmap.toBase64()));

    // Date
    if (record.date.isValid())
    {
        query.bindValue(":date", (int)record.date.toJulianDay());
    }

    // Start time
    if (record.startTime.isValid())
    {
        query.bindValue(":startTime", record.startTime.msecsSinceStartOfDay());
    }
    else
    {
        query.bindValue(":startTime", "0");
    }

    // End time
    if (record.endTime.isValid())
    {
        query.bindValue(":endTime", record.endTime.msecsSinceStartOfDay());
    }
    else
    {
        query.bindValue(":endTime", "0");
    }

    // Media source
    query.bindValue(":mediaSource", record.mediaSource);

    // Video archive path
    query.bindValue(":videoArchive", record.videoArchive);

    // Statistics (blob)
    if (!record.statsData.isNull())
    {
        query.bindValue(":statsData", QString(record.statsData.toBase64()));
    }
    else
    {
        QByteArray empty;
        query.bindValue(":statsData", empty);
        ERROR_MESSAGE0(ERR_TYPE_MESSAGE, "AnalysisRecordSQLiteDao", "NULL stats data object inserted");
    }

    // Run insert query
    if (query.exec())
    {
        DEBUG_MESSAGE0("AnalysisRecordSQLiteDao", "Record inserted successfully");
    }
    else
    {
        ERROR_MESSAGE2(ERR_TYPE_DISPOSABLE,
                       "AnalysisRecordSQLiteDao",
                       "Failed to execute query: %s\nSQL Error: %s",
                       query.lastQuery().toUtf8().constData(),
                       query.lastError().text().toUtf8().constData());

        return CAMERA_PIPELINE_ERROR;
    }
    return CAMERA_PIPELINE_OK;
}

ErrorCode AnalysisRecordSQLiteDao::FindStatsForPeriod(
    const QDateTime&              curDateTime,
    int                           periodDays,
    int                           intervalSeconds,
    QList<AnalysisRecordModel*>&  results
    )
{
    QDate       curDate = curDateTime.date();
    QTime       curTime = curDateTime.time();
    QSqlQuery   query;
    QString     sql = "SELECT * FROM records WHERE ";

    DEBUG_MESSAGE1("AnalysisRecordSQLiteDao", "Get statistics called ThreadID = %p", QThread::currentThreadId());

    // Here we need to be wery careful with intervals around 00:00 time
    // Please note, that at 00:00 - msecs since start of day is 0!!!
    // This time interval should be handled in separate way.
    // @TODO - add this feature in future
    sql += "( cam = '";
    sql += QString((DataDirectoryInstance::instance())->getPipelineParams().pipelineName().c_str());
    sql += "' AND ";

    sql += " date >= ";
    sql += QString::number(curDate.addDays(-periodDays).toJulianDay());
    sql += " AND date <= ";
    sql += QString::number(curDate.toJulianDay());

    if (curTime.msecsSinceStartOfDay() >= 85800000) // 23:50
    {
        sql += " AND start_time >= ";
        sql += QString::number(curTime.addSecs(-(intervalSeconds * 1.1f)).msecsSinceStartOfDay());
    }
    else if (curTime.msecsSinceStartOfDay() <= 600000) // 00:10
    {
        sql += " AND start_time <= ";
        sql += QString::number(curTime.addSecs((intervalSeconds * 1.1f)).msecsSinceStartOfDay());
    }
    else
    {
        sql += " AND start_time >= ";
        sql += QString::number(curTime.addSecs(-(intervalSeconds * 1.1f)).msecsSinceStartOfDay());
        sql += " AND start_time <= ";
        sql += QString::number(curTime.addSecs( (intervalSeconds * 1.1f)).msecsSinceStartOfDay());
    }

    sql += " )";

    // This "forward only" drastically increases results parsing speed
    query.setForwardOnly(true);

    query.prepare(sql);

    if (query.exec())
    {
        results.clear();

        while(query.next())
        {
            AnalysisRecordModel* pRecord = new AnalysisRecordModel; // Memory must be deleted by user after usage

            pRecord->id = query.value(0).toInt();                                               // id
            pRecord->date = QDate::fromJulianDay(query.value(2).toInt());                       // date
            pRecord->startTime = QTime::fromMSecsSinceStartOfDay(query.value(3).toInt());       // start_time
            pRecord->endTime = QTime::fromMSecsSinceStartOfDay(query.value(4).toInt());         // end_time
            pRecord->mediaSource = query.value(5).toString();                                   // media_source
            pRecord->videoArchive = query.value(6).toString();                                  // video_archive

            pRecord->heatmap = QImage::fromData(QByteArray::fromBase64(query.value(7).toString().toUtf8()), "PNG");           // heatmap

            pRecord->statsData = QByteArray::fromBase64(query.value(8).toString().toUtf8());            // stats_data

            results.append(pRecord);
        }
    }
    else
    {
        ERROR_MESSAGE2(ERR_TYPE_DISPOSABLE,
                       "AnalysisRecordSQLiteDao",
                       "Failed to execute query: %s\nSQL Error: %s",
                       query.lastQuery().toUtf8().constData(),
                       query.lastError().text().toUtf8().constData());

        return CAMERA_PIPELINE_ERROR;
    }

    return CAMERA_PIPELINE_OK;
}

ErrorCode AnalysisRecordSQLiteDao::metaForPeriod(QDate startDate, QDate endDate, QTime startTime, QTime endTime, QList<AnalysisRecordModel *> &results)
{
    DataDirectory*            pDataDirectory = DataDirectoryInstance::instance();
    AnalysisParams            params = pDataDirectory->getParams();

    // If we have analysis - it means that archive info can be taken from DB
    if (params.motionBasedAnalysis() || params.differenceBasedAnalysis())
    {
        QSqlQuery   query;
        QString     sql = "SELECT  id, date, start_time, end_time, media_source, video_archive FROM records WHERE ";

        sql += "( cam = '";
        sql += QString((DataDirectoryInstance::instance())->getPipelineParams().pipelineName().c_str());
        sql += "' AND ";

        sql += " date >= ";
        sql += QString::number(startDate.toJulianDay());
        sql += " AND date <= ";
        sql += QString::number(endDate.toJulianDay());
        sql += " AND start_time >= ";
        sql += QString::number(ALIGN_TO_SEC(startTime.msecsSinceStartOfDay())); // We need to have aligned start an end time (without miliseconds)
        sql += " AND start_time < ";
        sql += QString::number(ALIGN_TO_SEC(endTime.msecsSinceStartOfDay()));
        sql += " )";

        // This "forward only" drastically increases results parsing speed
        query.setForwardOnly(true);

        query.prepare(sql);

        if (query.exec())
        {
            results.clear();

            while(query.next())
            {
                AnalysisRecordModel* pRecord = new AnalysisRecordModel; // Memory must be deleted by user after usage

                pRecord->id             = query.value(0).toInt();
                pRecord->date           = QDate::fromJulianDay(query.value(1).toInt());
                pRecord->startTime      = QTime::fromMSecsSinceStartOfDay(ALIGN_TO_SEC(query.value(2).toInt()));
                pRecord->endTime        = QTime::fromMSecsSinceStartOfDay(ALIGN_TO_SEC(query.value(3).toInt()));
                pRecord->mediaSource    = query.value(4).toString();
                pRecord->videoArchive   = query.value(5).toString();

                results.append(pRecord);
            }
        }
        else
        {
            ERROR_MESSAGE2(ERR_TYPE_DISPOSABLE,
                           "AnalysisRecordSQLiteDao",
                           "Failed to execute query: %s\nSQL Error: %s",
                           query.lastQuery().toUtf8().constData(),
                           query.lastError().text().toUtf8().constData());

            return CAMERA_PIPELINE_ERROR;
        }
    }
    else  // In record only mode we need to get archive files list directly from VideoArchive folder
    {
        QString archivePath(pDataDirectory->getPipelineParams().archivePath().data());
        QString pipelineName(pDataDirectory->getPipelineParams().pipelineName().data());

        QDir            directory(archivePath + QString('/') + pipelineName);
        QFileInfoList   filesList;

        filesList = directory.entryInfoList(QStringList("*.mp4"), QDir::NoFilter, QDir::Name);

        if (filesList.size() > 0)
        {
            QDateTime startDateTime;
            QDateTime endDateTime;

            startDateTime.setDate(startDate);
            startDateTime.setTime(startTime);

            endDateTime.setDate(endDate);
            endDateTime.setTime(endTime);

            results.clear();

            while(!filesList.isEmpty())
            {
                QFileInfo  fi = filesList.takeFirst();
                QString    fileTimeStr = fi.fileName();
                QDateTime  fileTime;

                fileTimeStr.chop(4);    // remove file extension
                fileTimeStr.remove(0, fileTimeStr.length() - 21); // we need only 'dd_MM_yyyy___HH_mm_ss' (21 character)
                fileTime = QDateTime::fromString(fileTimeStr, "dd_MM_yyyy___HH_mm_ss");

                if (fileTime >= startDateTime && fileTime <= endDateTime)
                {
                    AnalysisRecordModel* pRecord = new AnalysisRecordModel; // Memory must be deleted by user after usage

                    pRecord->id             = 0;
                    pRecord->date           = fi.created().date();
                    pRecord->startTime      = fi.created().time();
                    pRecord->endTime        = QTime();
                    pRecord->mediaSource    = "";
                    pRecord->videoArchive   = QString('/') + pipelineName + QString('/') + fi.fileName();

                    results.append(pRecord);
                }
            }
        }
    }
    return CAMERA_PIPELINE_OK;
}

QImage AnalysisRecordSQLiteDao::heatmap(int id)
{
    QImage result;
    QSqlQuery   query;
    QString sql = "SELECT heatmap FROM records WHERE id=" + QString::number(id);

    query.setForwardOnly(true);
    query.prepare(sql);

    if (query.exec())
    {
        if (query.next())
        {
            result = QImage::fromData(QByteArray::fromBase64(query.value(0).toString().toUtf8()), "PNG");
        }
    }
    else
    {
        ERROR_MESSAGE2(ERR_TYPE_DISPOSABLE,
                       "AnalysisRecordSQLiteDao",
                       "Failed to execute query: %s\nSQL Error: %s",
                       query.lastQuery().toUtf8().constData(),
                       query.lastError().text().toUtf8().constData());
    }
    return result;
}

QByteArray AnalysisRecordSQLiteDao::dataBlob(int id)
{
    QSqlQuery   query;
    QString sql = "SELECT stats_data FROM records WHERE id=" + QString::number(id);
    query.setForwardOnly(true);
    query.prepare(sql);

    if (query.exec())
    {
        if (query.next())
        {
            return QByteArray::fromBase64(query.value(0).toString().toUtf8());
        }
    }
    else
    {
        ERROR_MESSAGE2(ERR_TYPE_DISPOSABLE,
                       "AnalysisRecordSQLiteDao",
                       "Failed to execute query: %s\nSQL Error: %s",
                       query.lastQuery().toUtf8().constData(),
                       query.lastError().text().toUtf8().constData());
    }
    return QByteArray();
}

void AnalysisRecordSQLiteDao::storeEvent(EventDescription event)
{
    QSqlQuery   query;
    QString     fileTimeStr;
    QDateTime   fileStartTime;

    fileTimeStr = event.archiveFileName1;
    fileTimeStr.chop(4);    // remove file extension
    fileTimeStr.remove(0, fileTimeStr.length() - 21); // we need only 'dd_MM_yyyy___HH_mm_ss' (21 character)
    fileStartTime = QDateTime::fromString(fileTimeStr, "dd_MM_yyyy___HH_mm_ss");

    DEBUG_MESSAGE0("AnalysisRecordSQLiteDao", "storeEvent() started");

    query.prepare("INSERT INTO events "
                  "(cam, date, start_timestamp,  end_timestamp, type, confidence, reaction, archive_file1, archive_file2, file_offset_sec)"
                  " VALUES "
                  "(:cam, :date, :start_timestamp, :end_timestamp, :type, :confidence, :reaction, :archive_file1, :archive_file2, :file_offset_sec)");

    // Cam name
    query.bindValue(":cam", (DataDirectoryInstance::instance())->getPipelineParams().pipelineName().c_str());

    // Start time
    if (event.startTime.isValid())
    {
        query.bindValue(":date", event.startTime.date().toJulianDay());
        query.bindValue(":start_timestamp", event.startTime.toMSecsSinceEpoch());
    }
    else
    {
        ERROR_MESSAGE0(ERR_TYPE_ERROR, "AnalysisRecordSQLiteDao", "Cannot insert record: ivalid start time parameter");
    }

    // End time
    if (event.endTime.isValid())
    {
        query.bindValue(":end_timestamp", event.endTime.toMSecsSinceEpoch());
    }
    else
    {
        ERROR_MESSAGE0(ERR_TYPE_ERROR, "AnalysisRecordSQLiteDao", "Cannot insert record: ivalid end time parameter");
    }

    // Event type
    query.bindValue(":type", (int)event.eventType);

    // Event confidence
    query.bindValue(":confidence", (int)event.confidence);

    // Security reaction (can be null)
    query.bindValue(":reaction", (int)event.reaction);

    // Start file name
    query.bindValue(":archive_file1", event.archiveFileName1);

    // End file name (can be null)
    query.bindValue(":archive_file2", event.archiveFileName2);

    // Event offset from archive file start (in seconds)
    if (fileStartTime.isValid())
    {
        int offset = fileStartTime.secsTo(event.startTime);

        if (offset < -2)
        {
            ERROR_MESSAGE1(ERR_TYPE_DISPOSABLE, "AnalysisRecordSQLiteDao", "Negative event file offset: %d", offset);
        }
        query.bindValue(":file_offset_sec", MAX(0, offset));
    }
    else
    {
        query.bindValue(":file_offset_sec", 0);
    }

    // Execute query
    if (query.exec())
    {
        DEBUG_MESSAGE0("AnalysisRecordSQLiteDao", "Event record inserted successfully");
    }
    else
    {
        ERROR_MESSAGE2(ERR_TYPE_DISPOSABLE,
                       "AnalysisRecordSQLiteDao",
                       "Failed to execute query in storeEvent(): %s\nSQL Error: %s",
                       query.lastQuery().toUtf8().constData(),
                       query.lastError().text().toUtf8().constData());
    }
}

ErrorCode AnalysisRecordSQLiteDao::getEvents(QDateTime startDateTime, QDateTime endDateTime, QList<EventDescription> &results)
{
    Q_UNUSED(results);
    QSqlQuery   query;

    QString  camName = (DataDirectoryInstance::instance())->getPipelineParams().pipelineName().c_str();
    QString  startTime = QString::number(ALIGN_TO_SEC(startDateTime.toMSecsSinceEpoch()));
    QString  endTime   = QString::number(ALIGN_TO_SEC(endDateTime.toMSecsSinceEpoch()));

    if (!startDateTime.isValid() || !endDateTime.isValid())
    {
        ERROR_MESSAGE0(ERR_TYPE_DISPOSABLE, "AnalysisRecordSQLiteDao", "Invalid time parameters in getEvents()");
        return CAMERA_PIPELINE_ERROR;
    }

    QString sql = QString("SELECT id, start_timestamp,  end_timestamp, type, confidence, reaction, archive_file1, archive_file2, file_offset_sec "
                          "FROM events WHERE"
                          "(cam = '%1' AND end_timestamp >= %2 AND start_timestamp <= %3 )" ).arg(camName, startTime, endTime);

    query.setForwardOnly(true);

    query.prepare(sql);

    if (query.exec())
    {
        results.clear();
        while(query.next())
        {
            EventDescription event;

            event.id                = query.value(0).toLongLong();
            event.startTime         = QDateTime::fromMSecsSinceEpoch(query.value(1).toLongLong());
            event.endTime           = QDateTime::fromMSecsSinceEpoch(query.value(2).toLongLong());
            event.eventType         = query.value(3).toInt();
            event.confidence        = query.value(4).toInt();
            event.reaction          = (ReactionType)query.value(5).toInt();
            event.archiveFileName1  = query.value(6).toString();
            event.archiveFileName2  = query.value(7).toString();
            event.offset            = query.value(8).toLongLong();

            results.append(event);
        }
    }
    else
    {
        ERROR_MESSAGE2(ERR_TYPE_DISPOSABLE,
                       "AnalysisRecordSQLiteDao",
                       "Failed to execute query: %s\nSQL Error: %s",
                       query.lastQuery().toUtf8().constData(),
                       query.lastError().text().toUtf8().constData());

        return CAMERA_PIPELINE_ERROR;
    }
    return CAMERA_PIPELINE_OK;
}
