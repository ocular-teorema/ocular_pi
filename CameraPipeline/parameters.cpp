#include "parameters.h"

void AnalysisParameters::readParameters(QSettings& ini)
{
    downscaleCoeff          = ini.value("AnalysisParams/Downscale Coeff", 0.25).toDouble();
    motionThreshold         = ini.value("AnalysisParams/Motion Threshold", 0.2).toDouble();
    diffThreshold           = ini.value("AnalysisParams/Diff Threshold", 20).toInt();
    differenceBasedAnalysis = ini.value("AnalysisParams/Difference based analysis", false).toBool();
    motionBasedAnalysis     = ini.value("AnalysisParams/Motion based analysis", false).toBool();
    totalThreshold          = ini.value("AnalysisParams/Total Threshold", 0.05).toDouble();
    falseEventCoeff         = ini.value("AnalysisParams/False event coeff", 5.00).toDouble();
    validMotionBinHeight    = ini.value("AnalysisParams/Valid motion bin height", 8).toInt();
    produceDebug            = ini.value("AnalysisParams/Produce debug", false).toBool();
    experimental            = ini.value("AnalysisParams/Experimental", false).toBool();
    minimumCluster          = ini.value("AnalysisParams/Minimum Cluster", 50).toInt();
    dilateSize              = ini.value("AnalysisParams/Dilate Size", 10).toInt();
    bgThreshold             = ini.value("AnalysisParams/Bg threshold", 15).toInt();
    fgThreshold             = ini.value("AnalysisParams/Fg threshold", 40).toInt();
    useVirtualDate          = ini.value("AnalysisParams/Use virtual date", false).toBool();
    year                    = ini.value("AnalysisParams/Year", 2018).toInt();
    month                   = ini.value("AnalysisParams/Month", 10).toInt();
    day                     = ini.value("AnalysisParams/Day", 20).toInt();
    hour                    = ini.value("AnalysisParams/Hour", 14).toInt();
    minute                  = ini.value("AnalysisParams/Minute", 45).toInt();
}

void CommonPipelineParameters::readParameters(QSettings &ini)
{
    pipelineName            = ini.value("PipelineParams/Pipeline Name", "cam1").toString();
    cameraName              = ini.value("PipelineParams/Camera name", "Camera #1").toString();
    inputStreamUrl          = ini.value("PipelineParams/Input Stream Url", "rtmp://localhost/live/input").toString();
    smallStreamUrl          = ini.value("PipelineParams/Small Stream Url", "").toString();
    outputUrl               = ini.value("PipelineParams/Output Url", "ws://localhost:1234").toString();
    smallOutputUrl          = ini.value("PipelineParams/Small Output Url", "").toString();
    sourceOutputUrl         = ini.value("PipelineParams/Source Output Url", "rtmp://localhost/live/cam1").toString();
    fps                     = ini.value("PipelineParams/fps", 10).toInt();
    globalScale             = ini.value("PipelineParams/Global Scale", 1.0).toDouble();
    databasePath            = ini.value("PipelineParams/Database Path", "video_analytics").toString();
    archivePath             = ini.value("PipelineParams/Archive Path", "VideoArchive").toString();
    processingIntervalSec   = ini.value("PipelineParams/Processing Interval Sec", 600).toInt();
    statisticIntervalSec    = ini.value("PipelineParams/Statistic Interval Sec", 600).toInt();
    statisticPeriodDays     = ini.value("PipelineParams/Statistic Period Days", 14).toInt();
    doRecording             = ini.value("PipelineParams/DoRecording", true).toBool();
}

void EventProcessingParameters::readParameters(QSettings& ini)
{
    minimalEventDuration    = ini.value("EventParams/Minimal Event Duration", 1000 ).toInt();
    maximalEventDuration    = ini.value("EventParams/Maximal Event Duration", 60000).toInt();
    idleEventDuration       = ini.value("EventParams/Idle Event Duration",    5000 ).toInt();
    ignoreInterval          = ini.value("EventParams/Ignore Interval",        12000).toInt();
}
