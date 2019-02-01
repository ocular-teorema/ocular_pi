#include "analysisRecordModel.h"

AnalysisRecordModel::AnalysisRecordModel() :
    QObject(NULL),
    id(-1),
    startTime(),
    endTime(),
    mediaSource(""),
    videoArchive(""),
    heatmap(),
    statsData()
{

}

AnalysisRecordModel::~AnalysisRecordModel()
{

}

QDateTime AnalysisRecordModel::startDateTime()
{
    QDateTime result(date);
    result.setTime(startTime);
    return result;
}

QDateTime AnalysisRecordModel::endDateTime()
{
    QDateTime result(date);
    result.setTime(endTime);
    return result;
}
