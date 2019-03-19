#ifndef ANALYSISRECORDSQLITEDAO_H
#define ANALYSISRECORDSQLITEDAO_H

#include <QList>
#include <QObject>
#include <QDateTime>
#include <QFileInfo>
#include <QtSql/QSqlDatabase>

#include "cameraPipelineCommon.h"
#include "analysisRecordModel.h"

class SecurityEvent
{
public:
    enum EventType {
        SECURITY_EVENT_ALERT,
        SECURITY_EVENT_START,
        SECURITY_EVENT_END,
        SECURITY_REACTION_ALERT,
        SECURITY_REACTION_IGNORE
    };

    EventType type;
    uint64_t id;


    static QString TypeToString( EventType type)
    {
        switch (type)
        {
         //case SECURITY_EVENT_ALERT     : return "SECURITY_EVENT_ALERT"; break ;
         case SECURITY_EVENT_START     : return "SECURITY_EVENT_START"; break ;
         case SECURITY_EVENT_END       : return "SECURITY_EVENT_END"; break ;
         case SECURITY_REACTION_ALERT  : return "SECURITY_REACTION_ALERT"; break ;
         case SECURITY_REACTION_IGNORE : return "SECURITY_REACTION_IGNORE"; break ;
         default : return "Not in range"; break ;
        }
        return "Not in range";

    }

};

class AnalysisRecordDAO
{
public:
    virtual ErrorCode       InsertRecord(const AnalysisRecordModel &record) = 0;
    virtual ErrorCode       FindStatsForPeriod(
        const QDateTime&              curDateTime,
        int                           periodDays,
        int                           intervalMinutes,
        QList<AnalysisRecordModel *>& results) = 0;

    virtual ErrorCode       metaForPeriod(
            QDate startDate, QDate endDate,
            QTime startTime, QTime endTime,
            QList<AnalysisRecordModel *>& results) = 0;

    virtual QImage heatmap(int id) = 0;
    virtual QByteArray dataBlob(int id) = 0;


    // Functions relared to events
    virtual ErrorCode getEvents(
        QDateTime startDateTime,
        QDateTime endDateTime,
        QList<EventDescription>& results
    ) = 0;
};



class AnalysisRecordSQLiteDao : public QObject, public AnalysisRecordDAO
{
    Q_OBJECT
public:
     ~AnalysisRecordSQLiteDao();

    static AnalysisRecordSQLiteDao* Instance();
    static void Drop();

    virtual ErrorCode   InsertRecord(const AnalysisRecordModel &record);
    virtual ErrorCode   FindStatsForPeriod(const QDateTime& curDateTime,
                                           int periodDays,
                                           int intervalMinutes,
                                           QList<AnalysisRecordModel*>& results);

    virtual ErrorCode   metaForPeriod(QDate startDate,
                                      QDate endDate,
                                      QTime startTime,
                                      QTime endTime,
                                      QList<AnalysisRecordModel *>& results);

    virtual QImage heatmap(int id);
    virtual QByteArray dataBlob(int id);


    void storeEvent(EventDescription event);

    virtual ErrorCode getEvents(QDateTime startDateTime,
                                QDateTime endDateTime,
                                QList<EventDescription>& results);

private:
    AnalysisRecordSQLiteDao();

    AnalysisRecordSQLiteDao(const AnalysisRecordSQLiteDao& );               // hide copy constructor
    AnalysisRecordSQLiteDao& operator=(const AnalysisRecordSQLiteDao& );    // hide assign op

    static AnalysisRecordSQLiteDao* m_instance;

    QSqlDatabase    m_DB;
};

#endif // ANALYSISRECORDSQLITEDAO_H

/**
    CREATE TABLE "records" (
        "id"   INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
        "date" INTEGER NOT NULL,
        "start_time" INTEGER NOT NULL,
        "end_time" INTEGER NOT NULL,
        "media_source" TEXT,
        "video_archive" TEXT,
        "heatmap" BLOB,
        "stats_data" BLOB NOT NULL
  );
*/

/** Event related stuff **/
/**
    CREATE TABLE "events" (
        "id"   INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
        "date" INTEGER NOT NULL,
        "start_timestamp" INTEGER NOT NULL,
        "end_timestamp" INTEGER NOT NULL,
        "type" INTEGER,
        "confidence" INTEGER,
        "reaction" INTEGER,
        "archive_file1" TEXT NOT NULL,
        "archive_file2" TEXT,
        "file_offset_sec" INTEGER
   );
*/
