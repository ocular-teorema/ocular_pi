
#pragma once

#include "cameraPipelineCommon.h"
#include "rgb24Buffer.h"
#include "motionTypes.h"

/// Detected object parameters
struct DetectedObject
{
    u_int64_t               id;

    int32_t                 coordX;
    int32_t                 coordY;

    int32_t                 sizeX;
    int32_t                 sizeY;

    int32_t                 isValid;
};

/// Class for handling all interval statistic
class IntervalStatistics
{
public:
    ~IntervalStatistics();

    AccumlatorBuffer        accBuffer;
    VideoBuffer             backgroundBuffer;
    QVector<double>         perFrameMovements;
    MotionMap               motionMap;

    QDate                   date;
    QTime                   startTime;
    QTime                   endTime;
    QString                 archiveFile;

    void                    Reset();
    void                    CopyFrom(IntervalStatistics& src);
    ErrorCode               ToByteArray(QByteArray* bytes);
    ErrorCode               FromByteArray(QByteArray* bytes);
    RGB24Buffer*            toImage();
};

/// Results of video analysis
struct AnalysisResults
{
    int64_t                  timestamp;
    double                   percentMotion;
    int                      wasCalibrationError;

    VideoBuffer*             pDiffBuffer;
    MotionFlow*              pCurFlow;
    QVector<DetectedObject>  objects;
};


#define  ALERT_TYPE_AREA                    0x1     // Motion inside inactive area
#define  ALERT_TYPE_CALIBRATION_ERROR       0x2     // Camera calibration error
#define  ALERT_TYPE_INVALID_MOTION          0x4     // Motion in wrong direction
#define  ALERT_TYPE_VELOCITY                0x8     // Too fast or too slow motion
#define  ALERT_TYPE_STATIC_OBJECT           0x10    // Left object

/// Results of decision maker algorithm (universal structure for all types of decisions)
struct DecisionResults
{
    int64_t                 timestamp;      /// Timestamp of processed frame
    int                     alertType;      /// Indicates, whats happned
    float                   confidence;     /// Confidence in percent of current decision
    VideoBuffer*            pInfoBuffer;    /// Array with flags for each pixel (if applicable)
    QVector<DetectedObject> objects;        /// Detected objects list with information
};
