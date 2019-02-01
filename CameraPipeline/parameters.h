#ifndef PARAMETERS_H
#define PARAMETERS_H

#include <QSettings>

struct AnalysisParameters
{
    double  downscaleCoeff;
    double  motionThreshold;
    double  totalThreshold;
    int     diffThreshold;
    int     validMotionBinHeight;
    bool    differenceBasedAnalysis;
    bool    motionBasedAnalysis;
    bool    experimental;

    int     minimumCluster;
    int     dilateSize;
    int     bgThreshold;
    double  fgThreshold;
    bool    useVirtualDate;
    int     year;
    int     month;
    int     day;
    int     hour;
    int     minute;

    void  readParameters(QSettings &ini);
};

struct CommonPipelineParameters
{
    QString     pipelineName;
    QString     cameraName;
    QString     inputStreamUrl;
    QString     outputUrl;
    QString     sourceOutputUrl;
    QString     databasePath;
    QString     archivePath;

    int         fps;
    double      globalScale;

    int         outputStreamBitrate;
    int         processingIntervalSec;
    int         statisticIntervalSec;
    int         statisticPeriodDays;

    void  readParameters(QSettings& ini);
};

struct EventProcessingParameters
{
    int     minimalEventDuration;
    int     maximalEventDuration;
    int     idleEventDuration;
    int     ignoreInterval;

    void  readParameters(QSettings &ini);
};

#endif // PARAMETERS_H
