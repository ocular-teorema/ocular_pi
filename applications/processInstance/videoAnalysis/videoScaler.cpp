
#include "videoScaler.h"

VideoScaler::VideoScaler()
{
    m_pSwsContext = NULL;
    m_dstWidth = 0;
    m_dstHeight = 0;
}

VideoScaler::~VideoScaler()
{
    if(m_pSwsContext != NULL)
    {
        sws_freeContext(m_pSwsContext);
    }
}

void VideoScaler::ScaleFrame(VideoFrame *pInFrame, VideoFrame *pOutFrame)
{
    unsigned char* pPlanes[3] = {0, 0, 0};
    unsigned char* pPlanesDs[3] = {0, 0, 0};

    // Strides array has size 4 because swscale() function checks dstStride[3] and
    // valgrind produce warning about uninitialized variable access
    // In our case only first value of this array is really used
    int stride[4] = { pInFrame->GetStride(), pInFrame->GetStrideUV(), pInFrame->GetStrideUV(), 0 };
    int strideDs[4] = { pOutFrame->GetStride(), pOutFrame->GetStrideUV(), pOutFrame->GetStrideUV(), 0 };

    // Check if we have another input size or new downscaling coefficient (or it is a first frame)
    if ((m_dstWidth != pOutFrame->GetWidth()) || (m_dstHeight != pOutFrame->GetHeight()))
    {
        // Reallocate scale context
        if (m_pSwsContext != NULL)
        {
            sws_freeContext(m_pSwsContext);
        }

        m_pSwsContext = sws_getContext(pInFrame->GetWidth(),
                                       pInFrame->GetHeight(),
                                       AV_PIX_FMT_YUV420P,
                                       pOutFrame->GetWidth(),
                                       pOutFrame->GetHeight(),
                                       AV_PIX_FMT_YUV420P,
                                       SWS_LANCZOS, NULL, NULL, NULL);

        m_dstWidth = pOutFrame->GetWidth();
        m_dstHeight = pOutFrame->GetHeight();
    }

    // Downscaling
    pInFrame->GetYUVData(&pPlanes[0], &pPlanes[1], &pPlanes[2]);
    pOutFrame->GetYUVData(&pPlanesDs[0], &pPlanesDs[1], &pPlanesDs[2]);

    // Scale current frame
    sws_scale(m_pSwsContext,
              pPlanes,
              stride,
              0,
              pInFrame->GetHeight(),
              pPlanesDs,
              strideDs);

    pOutFrame->userTimestamp = pInFrame->userTimestamp;
    pOutFrame->nativeTimeInSeconds = pInFrame->nativeTimeInSeconds;
    pOutFrame->number = pInFrame->number;
}

void VideoScaler::ScaleFrame(AVFrame *pInFrame, VideoFrame *pOutFrame)
{
    unsigned char* pPlanesDs[3] = {0, 0, 0};

    // Strides array has size 4 because swscale() function checks dstStride[3] and
    // valgrind produce warning about uninitialized variable access
    // In our case only first value of this array is really used
    int strideDs[4] = { pOutFrame->GetStride(), pOutFrame->GetStrideUV(), pOutFrame->GetStrideUV(), 0 };

    if ((m_dstWidth != pOutFrame->GetWidth()) || (m_dstHeight != pOutFrame->GetHeight()))
    {
        // Reallocate scale context
        if (m_pSwsContext != NULL)
        {
            sws_freeContext(m_pSwsContext);
        }

        m_pSwsContext = sws_getContext(pInFrame->width,
                                       pInFrame->height,
                                       AV_PIX_FMT_YUV420P,
                                       pOutFrame->GetWidth(),
                                       pOutFrame->GetHeight(),
                                       AV_PIX_FMT_YUV420P,
                                       SWS_LANCZOS, NULL, NULL, NULL);

        m_dstWidth = pOutFrame->GetWidth();
        m_dstHeight = pOutFrame->GetHeight();
    }

    pOutFrame->GetYUVData(&pPlanesDs[0], &pPlanesDs[1], &pPlanesDs[2]);

    // Scale current frame
    sws_scale(m_pSwsContext,
              pInFrame->data,
              pInFrame->linesize,
              0,
              pInFrame->height,
              pPlanesDs,
              strideDs);

    // Copy timestamp
    if (av_q2d(pInFrame->sample_aspect_ratio) < 1.0)
    {
        ERROR_MESSAGE0(ERR_TYPE_WARNING, "VideoScaler", "Input AVFrame timebase not set properly in pInFrame->sample_aspect_ratio");
    }

    pOutFrame->nativeTimeInSeconds = av_q2d(pInFrame->sample_aspect_ratio)*pInFrame->pkt_dts;
}

void VideoScaler::ScaleFrame(VideoFrame *pInFrame, AVFrame *pOutFrame)
{
    if ((m_dstWidth != pOutFrame->width) || (m_dstHeight != pOutFrame->height))
    {
        // Reallocate scale context
        if (m_pSwsContext != NULL)
        {
            sws_freeContext(m_pSwsContext);
        }

        m_pSwsContext = sws_getContext(pInFrame->GetWidth(),
                                       pInFrame->GetHeight(),
                                       AV_PIX_FMT_YUV420P,
                                       pOutFrame->width,
                                       pOutFrame->height,
                                       AV_PIX_FMT_YUV420P,
                                       SWS_LANCZOS, NULL, NULL, NULL);

        m_dstWidth = pOutFrame->width;
        m_dstHeight = pOutFrame->height;
    }

    int stride[4] = { pInFrame->GetStride(), pInFrame->GetStrideUV(), pInFrame->GetStrideUV(), 0 };
    unsigned char* pPlanes[3] = {0, 0, 0};

    pInFrame->GetYUVData(&pPlanes[0], &pPlanes[1], &pPlanes[2]);

    // Scale current frame
    sws_scale(m_pSwsContext,
              pPlanes,
              stride,
              0,
              pInFrame->GetHeight(),
              pOutFrame->data,
              pOutFrame->linesize);
}
