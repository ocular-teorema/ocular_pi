
#include "g8Buffer.h"

#include <stdio.h>
#include <QMutex>
#include <QDateTime>
#include <QTextStream>

#include "cameraPipelineCommon.h"

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>

VideoFrame::VideoFrame()
{
    m_width = 0;
    m_height = 0;
    m_stride = 0;
    m_strideUV = 0;
    m_strideRGB24 = 0;

    m_pYBuffer = NULL;
    m_pUBuffer = NULL;
    m_pVBuffer = NULL;
    m_pRGB24Buffer = NULL;
    m_pSwsContext = NULL;

    nativeTimeInSeconds = 0.0;
    userTimestamp = 0;
    number = 0;
}

VideoFrame::VideoFrame(int width, int height) : VideoFrame()
{
    SetSize(width, height);
}

VideoFrame::VideoFrame(AVFrame* parent) : VideoFrame()
{
    CopyFromAVFrame(parent);
}

VideoFrame::VideoFrame(VideoFrame *parent) : VideoFrame()
{
    CopyFromVideoFrame(parent);
}

void VideoFrame::CopyFromVideoFrame(VideoFrame* pSrcFrame)
{
    if (NULL == pSrcFrame)
    {
        return;
    }

    SetSize(pSrcFrame->GetWidth(), pSrcFrame->GetHeight());

    // Copy data from parent frame
    memcpy(m_pYBuffer, pSrcFrame->GetYData(), m_stride * m_height);
    memcpy(m_pUBuffer, pSrcFrame->GetUData(), m_strideUV * (m_height/2));
    memcpy(m_pVBuffer, pSrcFrame->GetVData(), m_strideUV * (m_height/2));
    //memcpy(m_pRGB24Buffer, pSrcFrame->GetRGB24Data(), m_strideRGB24*m_height);

    number = pSrcFrame->number;
    userTimestamp = pSrcFrame->userTimestamp;
    nativeTimeInSeconds = pSrcFrame->nativeTimeInSeconds;
}

void VideoFrame::CopyFromAVFrame(AVFrame* pSrcFrame)
{
    if (NULL == pSrcFrame)
    {
        return;
    }

    SetSize(pSrcFrame->width, pSrcFrame->height);

    // Copy data from parent frame
    for (int i = 0; i < m_height; i++)
    {
        memcpy(m_pYBuffer + i*m_stride, pSrcFrame->data[0] + pSrcFrame->linesize[0]*i, m_width);

        if (i < (m_height/2))
        {
            memcpy(m_pUBuffer + i*m_strideUV, pSrcFrame->data[1] + pSrcFrame->linesize[1]*i, (m_width/2));
            memcpy(m_pVBuffer + i*m_strideUV, pSrcFrame->data[2] + pSrcFrame->linesize[1]*i, (m_width/2));
        }
    }
}

void VideoFrame::CopyToAVFrame(AVFrame* pDstFrame)
{
    if (NULL == pDstFrame)
    {
        return;
    }

    // Check, if ve have valid buffers of same size
    if ( (m_width != pDstFrame->width)   ||
         (m_height != pDstFrame->height) ||
         (m_pYBuffer == NULL)            ||
         (m_pUBuffer == NULL)            ||
         (m_pVBuffer == NULL)            ||
         (pDstFrame->data[0] == NULL)    ||
         (pDstFrame->data[1] == NULL)    ||
         (pDstFrame->data[2] == NULL)
       )
    {
        return;
    }

    // Copy Y plane
    for(int j = 0; j < m_height; j++)
    {
        memcpy(pDstFrame->data[0] + j*pDstFrame->linesize[0], m_pYBuffer + j*m_stride, m_width);
    }
    // Copy UV planes
    for(int j = 0; j < m_height/2; j++)
    {
        memcpy(pDstFrame->data[1] + j*pDstFrame->linesize[1], m_pUBuffer + j*m_strideUV, m_width/2);
        memcpy(pDstFrame->data[2] + j*pDstFrame->linesize[1], m_pVBuffer + j*m_strideUV, m_width/2);
    }
    pDstFrame->pts = userTimestamp;
}

QImage VideoFrame::CreateQImageFromY(float scale)
{
    QImage img(m_pYBuffer, m_width, m_height, m_stride, QImage::Format_Indexed8);
    for(int i = 0; i < 256; i++)
    {
        img.setColor(i, qRgb(i, i, i));
    }
    return img.scaled(m_width*scale, m_height*scale);
}

QImage VideoFrame::CreateQImageFromRGB(float scale)
{
    QImage img(m_pRGB24Buffer, m_width, m_height, m_strideRGB24, QImage::Format_RGB888);
    return img.scaled(m_width*scale, m_height*scale).rgbSwapped();
}

QImage* VideoFrame::GetQImagePtrFromY(float scale)
{
    QImage img(m_pYBuffer, m_width, m_height, m_stride, QImage::Format_Indexed8);
    for(int i = 0; i < 256; i++)
    {
        img.setColor(i, qRgb(i, i, i));
    }
    return new QImage(img.scaled(m_width*scale, m_height*scale));
}

QImage* VideoFrame::GetQImagePtrFromRGB(float scale)
{
    QImage img(m_pRGB24Buffer, m_width, m_height, m_strideRGB24, QImage::Format_RGB888);
    return new QImage(img.scaled(m_width*scale, m_height*scale).rgbSwapped());
}

void VideoFrame::SetSize(int width, int height)
{
    // Check, if ve have valid buffers of same size
    if ( (m_width != width) ||
         (m_height != height) ||
         (m_pYBuffer == NULL) ||
         (m_pUBuffer == NULL) ||
         (m_pVBuffer == NULL) ||
         (m_pRGB24Buffer == NULL)
       )
    {
        // delete old buffers
        SAFE_DELETE_ARRAY(m_pYBuffer);
        SAFE_DELETE_ARRAY(m_pUBuffer);
        SAFE_DELETE_ARRAY(m_pVBuffer);
        SAFE_DELETE_ARRAY(m_pRGB24Buffer);

        m_width = width;
        m_height = height;
        m_stride = ((width + 15) >> 4) << 4; // Align stride
        m_strideUV = m_stride >> 1;
        m_strideRGB24 = m_stride*4;

        // Allocate internal buffers
        m_pYBuffer     = new unsigned char[m_stride * m_height];
        m_pUBuffer     = new unsigned char[m_strideUV * ((m_height + 1)/2)];
        m_pVBuffer     = new unsigned char[m_strideUV * ((m_height + 1)/2)];
        m_pRGB24Buffer = new unsigned char[m_strideRGB24 * m_height];

        // Fill with black
        memset(m_pYBuffer, 0, m_stride * m_height);
        memset(m_pUBuffer, 128, m_strideUV * ((m_height + 1)/2));
        memset(m_pVBuffer, 128, m_strideUV * ((m_height + 1)/2));
        memset(m_pRGB24Buffer, 0, m_strideRGB24 * m_height);

        // Realloc sws context for yuv->rgb conversion
        if (NULL != m_pSwsContext)
        {
            sws_freeContext(m_pSwsContext);
            m_pSwsContext = NULL;
        }
        m_pSwsContext = sws_getContext(m_width,
                                       m_height,
                                       AV_PIX_FMT_YUV420P,
                                       m_width,
                                       m_height,
                                       AV_PIX_FMT_BGR24,
                                       SWS_FAST_BILINEAR, NULL, NULL, NULL);
    }
}

void VideoFrame::UpdateRGB()
{
    if (NULL != m_pSwsContext)
    {
        unsigned char* pPlanesIn[3]   = { m_pYBuffer, m_pUBuffer, m_pVBuffer };
        unsigned char* pPlanesOut[3]  = { m_pRGB24Buffer, NULL, NULL };
        int            pStridesIn[3]  = { m_stride, m_strideUV, m_strideUV };
        int            pStridesOut[3] = { m_strideRGB24, 0, 0 };

        // Convert yuv2rgb
        sws_scale(m_pSwsContext,
                  pPlanesIn,
                  pStridesIn,
                  0,
                  m_height,
                  pPlanesOut,
                  pStridesOut);
    }
    else
    {
        ERROR_MESSAGE0(ERR_TYPE_ERROR, "VideoFrame", "Invalid SwsContext in UpdateRGB()");
    }
}

void VideoFrame::GetYUVData(unsigned char** pY, unsigned char** pU, unsigned char** pV)
{
    if (pY != NULL)
    {
        *pY = m_pYBuffer;
    }

    if (pU != NULL)
    {
        *pU = m_pUBuffer;
    }

    if (pV != NULL)
    {
        *pV = m_pVBuffer;
    }
}

VideoFrame::~VideoFrame()
{
    // delete buffers
    SAFE_DELETE_ARRAY(m_pYBuffer);
    SAFE_DELETE_ARRAY(m_pUBuffer);
    SAFE_DELETE_ARRAY(m_pVBuffer);
    SAFE_DELETE_ARRAY(m_pRGB24Buffer);
    if (NULL != m_pSwsContext)
    {
        sws_freeContext(m_pSwsContext);
        m_pSwsContext = NULL;
    }
}





VideoBuffer::VideoBuffer()
{
    m_width = 0;
    m_height = 0;
    m_stride = 0;
    m_pBuffer = NULL;
    nativeTimeInSeconds = 0.0;
    userTimestamp = 0;
}

VideoBuffer::VideoBuffer(VideoBuffer* parent) : VideoBuffer()
{
    CopyFrom(parent);
}

VideoBuffer::VideoBuffer(VideoFrame *parent, int planeIdx) : VideoBuffer()
{
    CopyFrom(parent, planeIdx);
}

VideoBuffer::VideoBuffer(int width, int height) : VideoBuffer()
{
    SetSize(width, height);
}

void VideoBuffer::CopyFrom(VideoFrame* pSrcFrame, int planeIdx)
{
    if(planeIdx == 0)
    {
        // Check, if ve have valid buffer of same size
        if ((m_width != pSrcFrame->GetWidth()) || (m_height != pSrcFrame->GetHeight()) || (m_pBuffer == NULL))
        {
            SetSize(pSrcFrame->GetWidth(), pSrcFrame->GetHeight());
        }

        // Copy data from parent frame
        for(int i = 0; i < m_height; i++)
        {
            memcpy(m_pBuffer + i*m_stride, pSrcFrame->GetYData() + i*pSrcFrame->GetStride(), m_width);
        }
    }
    else
    {
        // Check, if ve have valid buffer of same size
        if ((m_width != pSrcFrame->GetWidth()/2) || (m_height != pSrcFrame->GetHeight()/2) || (m_pBuffer == NULL))
        {
            SetSize(pSrcFrame->GetWidth()/2, pSrcFrame->GetHeight()/2);
        }

        // Copy data from parent frame
        if(planeIdx == 1)
        {
            for(int i = 0; i < m_height; i++)
            {
                memcpy(m_pBuffer + i*m_stride, pSrcFrame->GetUData() + i*pSrcFrame->GetStrideUV(), m_width);
            }
        }
        else
        {
            for(int i = 0; i < m_height; i++)
            {
                memcpy(m_pBuffer + i*m_stride, pSrcFrame->GetVData() + i*pSrcFrame->GetStrideUV(), m_width);
            }
        }
    }
    nativeTimeInSeconds = pSrcFrame->nativeTimeInSeconds;
    userTimestamp = pSrcFrame->userTimestamp;
}

void VideoBuffer::CopyFrom(VideoBuffer *pSrc)
{
    SetSize(pSrc->GetWidth(), pSrc->GetHeight());
    memcpy(m_pBuffer, pSrc->GetPlaneData(), m_stride*m_height);
}

void VideoBuffer::CopyFrom(G8Buffer *pSrc)
{
    SetSize(pSrc->w, pSrc->h);

    for (int j = 0; j < pSrc->h; j++)
    {
        memcpy(m_pBuffer + j*m_stride, pSrc->data + j*pSrc->w, pSrc->w);
    }
}

void VideoBuffer::CopyTo(G8Buffer *pSrc)
{
    if (NULL == pSrc)
    {
        return;
    }

    if (m_width != pSrc->w || m_height != pSrc->h || NULL == m_pBuffer)
    {
        return;
    }

    for (int j = 0; j < pSrc->h; j++)
    {
        memcpy(pSrc->data + j*pSrc->w, m_pBuffer + j*m_stride, pSrc->w);
    }
}

void VideoFrame::WriteToYUVFile(const char *fileName)
{
    FILE* f = fopen(fileName, "ab");

    for(int i = 0; i < m_height; i++)
    {
        fwrite(m_pYBuffer + i*m_stride, 1, m_width, f);
    }
    for(int i = 0; i < (m_height>>1); i++)
    {
        fwrite(m_pUBuffer + i*m_strideUV, 1, m_width>>1, f);
    }
    for(int i = 0; i < (m_height>>1); i++)
    {
        fwrite(m_pVBuffer + i*m_strideUV, 1, m_width>>1, f);
    }
    fclose(f);
}

QImage VideoBuffer::CreateQImage(float scale)
{
    QImage img(m_pBuffer, m_width, m_height, m_stride, QImage::Format_Indexed8);
    for(int i = 0; i < 256; i++)
    {
        img.setColor(i, qRgb(i, i, i));
    }
    return img.scaled(m_width*scale, m_height*scale);
}

QImage* VideoBuffer::GetQImagePtr(float scale)
{
    QImage img(m_pBuffer, m_width, m_height, m_stride, QImage::Format_Indexed8);
    for(int i = 0; i < 256; i++)
    {
        img.setColor(i, qRgb(i, i, i));
    }
    return new QImage(img.scaled(m_width*scale, m_height*scale));
}

void VideoBuffer::WriteToYUVFile(const char *fileName)
{
    FILE* f = fopen(fileName, "ab");

    unsigned char* dummyBuf = new unsigned char[(m_width*m_height)>>1];
    memset(dummyBuf, 128, (m_width*m_height)>>1);
    for (int j = 0; j < m_height; j++)
    {
        fwrite(m_pBuffer + j*m_stride, 1, m_width, f);
    }
    fwrite(dummyBuf, 1, (m_width*m_height)>>1, f);
    fclose(f);
    delete [] dummyBuf;
}

void VideoBuffer::SetSize(int width, int height)
{
    // Check, if ve have valid buffer of same size
    if ((m_width != width) || (m_height != height) || (m_pBuffer == NULL))
    {
        // delete old buffer
        SAFE_DELETE_ARRAY(m_pBuffer);

        m_width = width;
        m_height = height;
        m_stride = ((width + 15)>>4)<<4;

        // Allocate internal buffer
        m_pBuffer = new unsigned char[m_stride * m_height];
        memset(m_pBuffer, 0, m_stride*m_height);
    }
}

void VideoBuffer::SetRawData(int* pData, int width, int height, int stride)
{
    if (NULL == pData || NULL == m_pBuffer || m_width != width || m_height != height)
    {
        return;
    }

    // Copy
    for(int j = 0; j < height; j++)
    {
        for(int i = 0; i < width; i++)
        {
            m_pBuffer[j*m_stride + i] = (unsigned char) MIN(pData[j*stride + i], 255);
        }
    }
}

void VideoBuffer::SetRawData(float* pData, int width, int height, int stride)
{
    if (NULL == pData || NULL == m_pBuffer || m_width != width || m_height != height)
    {
        return;
    }

    // Copy
    for(int j = 0; j < height; j++)
    {
        for(int i = 0; i < width; i++)
        {
            m_pBuffer[j*m_stride + i] = (unsigned char) MIN(pData[j*stride + i], 255.0f);
        }
    }
}

void VideoBuffer::SetRawData(unsigned char *pData, int width, int height, int stride)
{
    if (NULL == pData || NULL == m_pBuffer || m_width != width || m_height != height)
    {
        return;
    }

    // Copy
    for(int j = 0; j < height; j++)
    {
        for(int i = 0; i < width; i++)
        {
            m_pBuffer[j*m_stride + i] = (unsigned char) MIN(pData[j*stride + i], 255.0f);
        }
    }
}

void VideoBuffer::Add(VideoBuffer *pBufferToAdd, float k)
{
    int             stride = pBufferToAdd->GetStride();
    unsigned char*  pBuf = pBufferToAdd->GetPlaneData();

    if (NULL == pBuf || NULL == m_pBuffer)
    {
        return;
    }

    if (m_width != pBufferToAdd->GetWidth() || m_height != pBufferToAdd->GetHeight())
    {
        return;
    }

    // Add
    for(int j = 0; j < m_height; j++)
    {
        for(int i = 0; i < m_width; i++)
        {
            int add = MIN((float)pBuf[j*stride + i] * k, 255);
            m_pBuffer[j*m_stride + i] = MIN(m_pBuffer[j*m_stride + i] + add, 255);
        }
    }
}

void VideoBuffer::AddVal(int val)
{
    // Add
    for(int j = 0; j < m_height; j++)
    {
        for(int i = 0; i < m_width; i++)
        {
            m_pBuffer[j*m_stride + i] = MAX(0, MIN(m_pBuffer[j*m_stride + i] + val, 255));
        }
    }
}

void VideoBuffer::Avg(float *avg)
{
    // Add
    float sum = 0.0f;
    for(int j = 0; j < m_height; j++)
    {
        for(int i = 0; i < m_width; i++)
        {
            sum += m_pBuffer[j*m_stride + i];
        }
    }
    *avg = sum / (m_width * m_height);
}

void VideoBuffer::Mul(VideoBuffer *pBufferToMul)
{
    int             stride = pBufferToMul->GetStride();
    unsigned char*  pBuf = pBufferToMul->GetPlaneData();

    if (NULL == pBuf || NULL == m_pBuffer)
    {
        return;
    }

    if (m_width != pBufferToMul->GetWidth() || m_height != pBufferToMul->GetHeight())
    {
        return;
    }

    // Multiplication
    for(int j = 0; j < m_height; j++)
    {
        for(int i = 0; i < m_width; i++)
        {
            m_pBuffer[j*m_stride + i] = MIN(m_pBuffer[j*m_stride + i] * pBuf[j*stride + i], 255);
        }
    }
}

void VideoBuffer::MulToVal(float val)
{
    // Multiplication
    for(int j = 0; j < m_height; j++)
    {
        for(int i = 0; i < m_width; i++)
        {
            m_pBuffer[j*m_stride + i] = (unsigned char)MIN((float)m_pBuffer[j*m_stride + i] * val, 255.0f);
        }
    }
}

void VideoBuffer::SetVal(unsigned char val)
{
    if(NULL == m_pBuffer)
    {
        return;
    }
    memset(m_pBuffer, val, m_height*m_stride);
}

int VideoBuffer::Binarize(int threshold, int value)
{
    int totalAbove = 0;

    if(NULL == m_pBuffer)
    {
        return -1;
    }

    // Add
    for(int j = 0; j < m_height; j++)
    {
        for(int i = 0; i < m_width; i++)
        {
            if (m_pBuffer[j*m_stride + i] > threshold)
            {
                m_pBuffer[j*m_stride + i] = value;
                totalAbove++;
            }
            else
            {
                m_pBuffer[j*m_stride + i] = 0;
            }
        }
    }
    return totalAbove;
}

void VideoBuffer::Mask(VideoBuffer* pMsk)
{
    int             stride = pMsk->GetStride();
    unsigned char*  pBuf = pMsk->GetPlaneData();

    if( m_width != pMsk->GetWidth() || m_height != pMsk->GetHeight())
    {
        return;
    }

    // Mask
    for(int j = 0; j < m_height; j++)
    {
        for(int i = 0; i < m_width; i++)
        {
            m_pBuffer[j*m_stride + i] = (pBuf[j*stride + i] > 0) ? m_pBuffer[j*m_stride + i] : 0;
        }
    }
}

float VideoBuffer::AbsDiff(VideoBuffer *pBufferToDif)
{
    float           avg = 0.0f;
    int             stride = pBufferToDif->GetStride();
    unsigned char*  pBuf = pBufferToDif->GetPlaneData();

    if (m_width != pBufferToDif->GetWidth() || m_height != pBufferToDif->GetHeight() || NULL == m_pBuffer)
    {
        return -1.0f;
    }

    // Add
    for(int j = 0; j < m_height; j++)
    {
        for(int i = 0; i < m_width; i++)
        {
            int diff = abs((int)m_pBuffer[j*m_stride + i] - (int)pBuf[j*stride + i]);
            m_pBuffer[j*m_stride + i] = diff;
            avg += diff;
        }
    }
    return avg / (m_width*m_height);
}

float VideoBuffer::AbsDiffLuma(VideoFrame *pFrameToDif)
{
    float           avg = 0.0f;
    int             frameStride = pFrameToDif->GetStride();
    unsigned char*  pBuf = pFrameToDif->GetYData();

    if (m_width != pFrameToDif->GetWidth() || m_height != pFrameToDif->GetHeight() || NULL == m_pBuffer)
    {
        return -1.0f;
    }

    // Add
    for(int j = 0; j < m_height; j++)
    {
        for(int i = 0; i < m_width; i++)
        {
            int diff = abs((int)m_pBuffer[j*m_stride + i] - (int)pBuf[j*frameStride + i]);
            m_pBuffer[j*m_stride + i] = diff;
            avg += diff;
        }
    }
    return avg / (m_width*m_height);
}

void VideoBuffer::Blur(int radius, float gain)
{
    // Blur should be performed only on even region (to avoid falitures inside opencv)
    cv::Mat  cur(m_height, m_width, CV_8UC1, m_pBuffer, m_stride);

    cv::blur(cur, cur, cv::Size(radius, radius));

    if (gain != 1.0f)
    {
        cur *= gain;
    }
}

void VideoBuffer::Morph(int radius, int openOrClose)
{
    // Blur should be performed only on even region (to avoid falitures inside opencv)
    cv::Mat  cur(m_height, m_width, CV_8UC1, m_pBuffer, m_stride);
    cv::Mat  kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(radius, radius));

    cv::morphologyEx(cur, cur, openOrClose ? CV_MOP_OPEN : CV_MOP_CLOSE, kernel);
}

VideoBuffer::~VideoBuffer()
{
    // delete buffer
    SAFE_DELETE_ARRAY(m_pBuffer);
}




AccumlatorBuffer::AccumlatorBuffer()
{
    m_width = 0;
    m_height = 0;
    m_stride = 0;
    m_count = 0;
    m_pBuffer = NULL;
}

AccumlatorBuffer::AccumlatorBuffer(int width, int height) : AccumlatorBuffer()
{
    SetSize(width, height);
}

void AccumlatorBuffer::CopyFrom(AccumlatorBuffer* pSrc)
{
    SetSize(pSrc->GetWidth(), pSrc->GetHeight());
    SetRawData(pSrc->GetWidth(), pSrc->GetHeight(), pSrc->GetPlaneData(), pSrc->GetStride());
}

void AccumlatorBuffer::SetSize(int width, int height)
{
    if ((m_width != width) || (m_height != height) || (NULL == m_pBuffer))
    {
        m_width = width;
        m_height = height;
        m_stride = ((width + 15)>>4)<<4;
        SAFE_DELETE_ARRAY(m_pBuffer);
        m_pBuffer = new float[m_stride * m_height];
        memset(m_pBuffer, 0, m_stride*m_height*sizeof(float));
    }
}

void AccumlatorBuffer::Add(AccumlatorBuffer *pInBuffer, float weight)
{
    int     width = pInBuffer->GetWidth();
    int     height = pInBuffer->GetHeight();
    int     stride = pInBuffer->GetStride();
    float*  pBuf = pInBuffer->GetPlaneData();

    if (width != m_width || height != m_height)
    {
        ERROR_MESSAGE4(ERR_TYPE_WARNING, "AccumlatorBuffer",
                       "Add() input AccumlatorBuffer has different size (%dx%d) (%dx%d)",
                       m_width, m_height, width, height);
        width = MIN(m_width, width);
        height = MIN(m_height, height);
    }

    // Add
    for (int j = 0; j < height; j++)
    {
        for(int i = 0; i < width; i++)
        {
            m_pBuffer[j*m_stride + i] += weight * pBuf[j*stride + i];
        }
    }
    m_count++;
}

void AccumlatorBuffer::AddBuffer(VideoBuffer* pInBuffer)
{
    unsigned char*  pBuf = pInBuffer->GetPlaneData();
    int             width = pInBuffer->GetWidth();
    int             height = pInBuffer->GetHeight();
    int             stride = pInBuffer->GetStride();

    if (width != m_width || height != m_height)
    {
        ERROR_MESSAGE4(ERR_TYPE_WARNING, "AccumlatorBuffer",
                       "Add() input VideoBuffer has different size (%dx%d) (%dx%d)",
                       m_width, m_height, width, height);
        width = MIN(m_width, width);
        height = MIN(m_height, height);
    }


    // Add
    for (int j = 0; j < height; j++)
    {
        for(int i = 0; i < width; i++)
        {
            m_pBuffer[j*m_stride + i] += (float)pBuf[j*stride + i];
        }
    }
    m_count++;
}

void AccumlatorBuffer::Blur(int radius)
{
    // Blur should be performed only on even region (to avoid falitures inside opencv)
    cv::Mat  cur((m_height & 0xfffffffe), (m_width & 0xfffffffe), CV_32FC1, m_pBuffer, m_stride*sizeof(float));
    cv::blur(cur, cur, cv::Size(radius, radius));
}

void AccumlatorBuffer::ConvertTo(VideoBuffer *dst, bool doAverage)
{
    int            stride = dst->GetStride();
    unsigned char* pBuf = dst->GetPlaneData();

    if ((dst->GetWidth() != m_width) || (dst->GetHeight() != m_height))
    {
        return;
    }

    for (int j = 0; j < m_height; j++)
    {
        for (int i = 0; i < m_width; i++)
        {
            int val;
            if (doAverage)
            {
                val = (int)(m_pBuffer[j*m_stride + i] / (float)m_count + 0.5f);
            }
            else
            {
                val = (int)(m_pBuffer[j*m_stride + i] + 0.5f);
            }
            pBuf[j*stride + i] = MIN(val, 255);
        }
    }
}

void AccumlatorBuffer::WriteToYUVFile(const char *fileName, float div)
{
    VideoBuffer tmpBuf(m_width, m_height);
    float*      pTmpArr = new float[m_width*m_height];

    for (int j = 0; j < m_height; j++)
    {
        for (int i = 0; i < m_width; i++)
        {
            pTmpArr[j*m_width + i] = m_pBuffer[j*m_stride + i] / div;
        }
    }
    tmpBuf.SetRawData(pTmpArr, m_width, m_height, m_width);
    tmpBuf.WriteToYUVFile(fileName);
    delete [] pTmpArr;
}

void AccumlatorBuffer::SetRawData(int width, int height, float *pData, int stride)
{
    if( m_width != width || m_height != height || NULL == pData || NULL == m_pBuffer)
    {
        return;
    }

    for (int j = 0; j < m_height; j++)
    {
        memcpy(m_pBuffer + j*m_stride, pData + j*stride, m_width*sizeof(float));
    }
}

void AccumlatorBuffer::Reset()
{
    m_count = 0;

    if (NULL != m_pBuffer)
    {
        memset(m_pBuffer, 0, m_stride*m_height*sizeof(float));
    }
}

AccumlatorBuffer::~AccumlatorBuffer()
{
    SAFE_DELETE_ARRAY(m_pBuffer);
    m_width = 0;
    m_height = 0;
    m_stride = 0;
}

void FillSPSPPS(AVCodecParameters* pCodecContext, AVPacket* pPacket)
{
    int i;
    int spsPpsDatalength = 0;
    int spsPpsStart = -1;
    int spsPpsEnd = -1;

    unsigned char* pCodedFrame = pPacket->data;
    int frameLength = pPacket->size;

    if (frameLength < 4)    // Frame is too short
    {
        return;
    }

    if (NULL != pCodecContext->extradata && pCodecContext->extradata_size > 0)   // sps and pps already filled
    {
        if (pCodedFrame[3] == 0x67 || pCodedFrame[3] == 0x68 ||
            pCodedFrame[4] == 0x67 || pCodedFrame[4] == 0x68)
        {
            return; // SPS/PPS already there
        }
        // Allocate space for sps/pps
        av_grow_packet(pPacket, pCodecContext->extradata_size);
        // Copy them to the end
        memcpy(pPacket->data + frameLength, pCodecContext->extradata, pCodecContext->extradata_size);

        return;
    }

    // Find sps pps headers and mark their position
    for(i = 0; i < frameLength - 3; i++)
    {
        if ( (pCodedFrame[i + 0] == 0x0) &&
             (pCodedFrame[i + 1] == 0x0) &&
             (pCodedFrame[i + 2] == 0x1)
           )
        {
            // NAL header found

            if (pCodedFrame[i + 3] == 0x67) // SPS nal
            {
                spsPpsStart = i;
            }
            else
            {
                if (pCodedFrame[i + 3] != 0x68) // PPS should be also included
                {
                    spsPpsEnd = i;
                }
            }

            if ( spsPpsStart >= 0 && spsPpsEnd > 0)
            {
                break;  // Sps and pps found and can be copied to extradata
            }
        }
    }

    spsPpsDatalength = (spsPpsEnd - spsPpsStart);

    if ( (spsPpsEnd <= 0) || (spsPpsDatalength < 8))  // Invalid sps pps sequence size
    {
        return;
    }

    pCodecContext->extradata = (unsigned char *)malloc(spsPpsDatalength);
    pCodecContext->extradata_size = spsPpsDatalength;
    memcpy(pCodecContext->extradata, &pCodedFrame[spsPpsStart], spsPpsDatalength);
}

IntervalTimer::IntervalTimer(int intervalLengthSec) :
    QObject(NULL),
    m_intervalStarted(false),
    m_intervalFinished(false)
{
    m_processingIntervalSec = intervalLengthSec;
}

IntervalTimer::~IntervalTimer()
{

}

void IntervalTimer::Reset()
{
    m_intervalStartTime = QDateTime();
    m_currentDateTime = QDateTime();
}

void IntervalTimer::Tick(QDateTime currentDateTime)
{
    int currentSec = currentDateTime.toMSecsSinceEpoch() / 1000;
    m_currentDateTime = currentDateTime;

    m_intervalStarted = false;
    m_intervalFinished = false;

    // Check for first frame
    if ( !m_intervalStartTime.isValid() && (0 == (currentSec % m_processingIntervalSec)))
    {
        m_intervalStartTime = m_currentDateTime;
        m_intervalStarted = true;
        emit Started(m_currentDateTime);
    }

    // Check for full interval passed
    if (m_intervalStartTime.isValid())
    {
        if ( ((m_intervalStartTime.secsTo(m_currentDateTime) > 0.9*m_processingIntervalSec) &&
             (0 == (currentSec % m_processingIntervalSec))) ||
             (m_intervalStartTime.msecsTo(m_currentDateTime) > m_processingIntervalSec*1000)
           )
        {
            m_lastIntervalStartTime = m_intervalStartTime;
            m_intervalStartTime = m_currentDateTime;
            m_intervalFinished = true;
            emit Interval(currentDateTime);
        }
    }
}
