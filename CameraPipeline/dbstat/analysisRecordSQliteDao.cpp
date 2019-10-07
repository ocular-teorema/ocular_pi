
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

AnalysisRecordDao::AnalysisRecordDao()
{
    DEBUG_MESSAGE0("AnalysisRecordSQLiteDao", "AnalysisRecordSQLiteDao() called");
    m_DB = QSqlDatabase::addDatabase("QPSQL");
    m_DB.setHostName("localhost");
    m_DB.setPort(5432);
    m_DB.setDatabaseName((DataDirectoryInstance::instance())->pipelineParams.databasePath);
    m_DB.setUserName("va");
    m_DB.setPassword("theorema");
    m_DB.open();
    //m_DB = QSqlDatabase::addDatabase("QSQLITE");
    //m_DB.setDatabaseName(m_dbFile.absoluteFilePath());
    //m_DB.open();

    if (!m_DB.isOpen())
    {
        ERROR_MESSAGE1(ERR_TYPE_CRITICAL,
                       "AnalysisRecordDao",
                       "Failed to open database: %s",
                       m_DB.lastError().text().toUtf8().constData());
    }
}

AnalysisRecordDao::~AnalysisRecordDao()
{
    DEBUG_MESSAGE0("AnalysisRecordDao", "~AnalysisRecordDao() called");
    m_DB.close();
    DEBUG_MESSAGE0("AnalysisRecordDao", "~AnalysisRecordDao() finished");
}

ErrorCode AnalysisRecordDao::InsertRecord(const AnalysisRecordModel& record)
{
    QSqlQuery   query;
    QByteArray  heatmap;
    QBuffer     heatmapBuffer(&heatmap);

    query.prepare("INSERT INTO records "
                  "(cam, date, start_time, end_time, media_source, video_archive, heatmap, stats_data, start_posix_time, end_posix_time) "
                  "VALUES "
                  "(:cam, :date, :startTime, :endTime, :mediaSource, :videoArchive, :heatmap, :statsData, :startPosix, :endPosix)");

    heatmapBuffer.open(QIODevice::WriteOnly);

    // Cam name
    query.bindValue(":cam", (DataDirectoryInstance::instance())->pipelineParams.pipelineName.toUtf8().constData());

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
        query.bindValue(":startPosix", QDateTime(record.date, record.startTime).toTime_t());
    }
    else
    {
        query.bindValue(":startTime", "0");
        query.bindValue(":startPosix", "0");
    }

    // End time
    if (record.endTime.isValid())
    {
        query.bindValue(":endTime", record.endTime.msecsSinceStartOfDay());
        query.bindValue(":endPosix", QDateTime(record.endDate, record.endTime).toTime_t());
    }
    else
    {
        query.bindValue(":endTime", "0");
        query.bindValue(":endPosix", "0");
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

ErrorCode AnalysisRecordDao::FindStatsForPeriod(
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
    sql += (DataDirectoryInstance::instance())->pipelineParams.pipelineName;
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
            pRecord->endDate = pRecord->date; // We don't use this here
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

void AnalysisRecordDao::storeEvent(EventDescription event)
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
    query.bindValue(":cam", (DataDirectoryInstance::instance())->pipelineParams.pipelineName);

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
        query.bindValue(":file_offset_sec", std::max(0, offset));
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
