#ifndef ANALYSISRECORDSQLITEDAO_H
#define ANALYSISRECORDSQLITEDAO_H

#include <QList>
#include <QObject>
#include <QDateTime>
#include <QFileInfo>
#include <QtSql/QSqlDatabase>

#include "cameraPipelineCommon.h"
#include "analysisRecordModel.h"

class AnalysisRecordDao : public QObject
{
    Q_OBJECT
public:
    AnalysisRecordDao();
     ~AnalysisRecordDao();

    ErrorCode   InsertRecord(const AnalysisRecordModel &record);
    ErrorCode   FindStatsForPeriod(const QDateTime& curDateTime,
                                   int periodDays,
                                   int intervalMinutes,
                                   QList<AnalysisRecordModel*>& results);
public slots:
    void    storeEvent(EventDescription event);

private:
    QSqlDatabase    m_DB;
};

#endif // ANALYSISRECORDSQLITEDAO_H
