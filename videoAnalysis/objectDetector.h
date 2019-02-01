#pragma once
#include "pipelineCommonTypes.h"
#include "cameraPipelineCommon.h"

#include "multimodalBG.h"
#include "motionAnalysis.h"

#define  OBJ_MATCH_THRESHOLD        0.5
#define  MATCHED_COLOR_THRESHOLD    3
#define  MATCHED_COORD_THRESHOLD    2

class ObjectDetector
{
public:
    ObjectDetector();
    ~ObjectDetector();

    // BG/FG segmentator object
    MultimodalBG*           pBGSubstractor;
    VideoBuffer*            pFGMask;

    // Found objects and tracking data
    QVector<DetectedObject>  currObjectsList;
    QVector<DetectedObject>  prevObjectsList;

    // Methods
    void Exec(VideoFrame *pCurFrame, VideoFrame *pPrevFrame, MotionFlow* pFlow);
private:

    // Optical flow variables
    VideoBuffer*            m_pCurYPlane;
    VideoBuffer*            m_pPrevYPlane;
    MotionFlow*             m_pCurFlow;

    void  CheckSize(int width, int height);
    void  MatchObjects(VideoFrame *pCurFrame, VideoFrame *pPrevFrame);
    float CompareObjects(DetectedObject obj1, DetectedObject obj2);
};
