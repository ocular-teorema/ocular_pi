#ifndef ANALYSISRECORDMODEL_H
#define ANALYSISRECORDMODEL_H

#include <QDateTime>
#include <QtGui/QImage>
#include <QMetaType>
#include <QObject>
#include <QByteArray>
#include <QUrl>

class AnalysisRecordModel : public QObject
{
    Q_OBJECT
public:
    AnalysisRecordModel();
    ~AnalysisRecordModel();

    int             id;             // Record ID
    QDate           date;           // Date
    QTime           startTime;      // Start time
    QTime           endTime;        // End time
    QString         mediaSource;    // Stream source adderss
    QString         videoArchive;   // Location of archive video file
    QImage          heatmap;        // Interval's heatmap with background
    QByteArray      statsData;      // Analysis statistics, converted to ByteArray


    QDateTime startDateTime();
    QDateTime endDateTime();

};

Q_DECLARE_METATYPE(AnalysisRecordModel*)

#endif // ANALYSISRECORDMODEL_H
