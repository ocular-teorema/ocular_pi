#include "denoiseFilter.h"

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#define LERP(x0, x1, coef) ((x0) + (coef)*(x1 - x0))

static void ProcessPlane(
    unsigned char *pCur,
    unsigned char *pPrev,
    unsigned char *pOut,
    int     width,
    int     height,
    int     stride,
    float   strength,
    int     spatialRadius
)
{
    cv::Mat  dst(height, width, CV_8UC1, pOut, stride);
    cv::Mat  prev(height, width, CV_8UC1, pPrev, stride);
    cv::Mat  tmp(height, width, CV_8UC1);

    float highPeak = strength;
    float belowClip = 0.1f;
    float lowPeak = 1.5f;

    float c0 = ( 1.0f - belowClip ) / ( highPeak - lowPeak );
    float c1 = belowClip - lowPeak * c0;
    float c2 = belowClip;

    // Copy top and bottom rows
    memcpy(tmp.data + (0 * tmp.step1()), pCur + (0*stride), width);
    memcpy(tmp.data + (1 * tmp.step1()), pCur + (1*stride), width);
    memcpy(tmp.data + ((height - 2) * tmp.step1()), pCur + ((height - 2)*stride), width);
    memcpy(tmp.data + ((height - 1) * tmp.step1()), pCur + ((height - 1)*stride), width);

    for (int j = 2; j < height - 2; j++)
    {
        for (int i = 0; i < width; i++)
        {
            float absd;
            float temp;
            float alpha;
            float rMax;
            float rMin;

            float curr = pCur[j*stride + i];
            float prev = pPrev[j*stride + i];
            float above1 = pCur[(j + 1)*stride + i];
            float above2 = pCur[(j + 2)*stride + i];
            float below1 = pCur[(j - 1)*stride + i];
            float below2 = pCur[(j - 2)*stride + i];

            absd = std::abs( curr - prev );
            temp = absd * c0 + c1;
            alpha = MIN( MAX( temp, c2 ), 1 );
            temp  = LERP(prev, curr, alpha);

            rMax = MAX( MAX( above1, below1 ), curr );
            rMax = MAX( MAX( above2, below2 ), rMax );
            rMin = MIN( MIN( above1, below1 ), curr );
            rMin = MIN( MIN( above2, below2 ), rMin );

            temp = MIN(MAX(temp, rMin), rMax);

            tmp.at<unsigned char>(cv::Point(i, j)) = (unsigned char)(temp + 0.5f);
        }
    }

    // We need to store currently processed frame as previous for next step
    // This is a must for given denoise algorithm
    tmp.copyTo(prev);

    // Spatial filtering - simple blur
    if (spatialRadius > 1)
    {
        cv::blur(tmp, dst, cv::Size(spatialRadius, spatialRadius));
    }
    else
    {
        tmp.copyTo(dst);
    }
}


DenoiseFilter::DenoiseFilter()
{

}

DenoiseFilter::~DenoiseFilter()
{

}

void DenoiseFilter::ProcessFrame(
    VideoFrame *pCurFrame,
    VideoFrame *pOutFrame,
    float strength,
    int spatialRadius
)
{
    unsigned char* pCur[3];
    unsigned char* pPrev[3];
    unsigned char* pOut[3];

    int width = pCurFrame->GetWidth();
    int height = pCurFrame->GetHeight();
    int stride = pCurFrame->GetStride();
    int strideUV = pCurFrame->GetStrideUV();

    if (m_prevFrame.GetWidth() != width || m_prevFrame.GetHeight() != height)
    {
        m_prevFrame.CopyFromVideoFrame(pCurFrame);
    }

    pCurFrame->GetYUVData(&pCur[0], &pCur[1], &pCur[2]);
    pOutFrame->GetYUVData(&pOut[0], &pOut[1], &pOut[2]);
    m_prevFrame.GetYUVData(&pPrev[0], &pPrev[1], &pPrev[2]);

    ProcessPlane(pCur[0], pPrev[0], pOut[0], width, height, stride, strength, spatialRadius);
    ProcessPlane(pCur[1], pPrev[1], pOut[1], (width>>1), (height>>1), strideUV, strength, spatialRadius);
    ProcessPlane(pCur[2], pPrev[2], pOut[2], (width>>1), (height>>1), strideUV, strength, spatialRadius);
}

void DenoiseFilter::ProcessBuffer(
    VideoBuffer *pCur,
    VideoBuffer *pOut,
    float strength,
    int spatialRadius
)
{
    int width = pCur->GetWidth();
    int height = pCur->GetHeight();
    int stride = pCur->GetStride();

    if (m_prevBuffer.GetWidth() != width || m_prevBuffer.GetHeight() != height)
    {
        m_prevBuffer.CopyFrom(pCur);
    }

    ProcessPlane(pCur->GetPlaneData(),
                 m_prevBuffer.GetPlaneData(),
                 pOut->GetPlaneData(),
                 width, height, stride, strength, spatialRadius);
}
