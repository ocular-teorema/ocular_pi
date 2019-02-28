#pragma once

#include "networkUtils/dataDirectory.h"
#include "pipelineCommonTypes.h"

#include <opencv2/core/core.hpp>

class MotionEstimator
{
public:
    MotionEstimator(int width, int height, int blockSize);
    ~MotionEstimator();

    void            ProcessFrame(VideoFrame* pCurFrame, int doFilter = false);
    MotionFlow*     GetFlow() { return m_pCurrFlow; }

    static void     DrawFlow(MotionFlow* pFlow, cv::Mat* pFlowMap = NULL);

private:
    int             m_width;
    int             m_height;
    int             m_blocksCount;
    int             m_blockSize;

    VideoFrame*     m_pPrevFrame;
    MotionFlow*     m_pCurrFlow;
    MotionFlow*     m_pPrevFlow;

    void Init(int width, int height, int blockSize);

    void ZeroCheck(VideoFrame* pCurFrame, VideoFrame* pPrevFrame, MotionFlow* pFlow);
    void FilterMVF(VideoFrame* pCurFrame, VideoFrame* pPrevFrame, MotionFlow* pFlow);
};
