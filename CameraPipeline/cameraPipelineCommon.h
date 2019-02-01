#pragma once

#include <QMutex>
#include <QObject>
#include <QImage>
#include <QFile>
#include <QTimeZone>

#include "errorHandler.h"
#include "dataDirectoryInstance.h"
#include "pipelineConfig.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

#define SAFE_DELETE_ARRAY(arr)  if(NULL!=arr) { delete [] arr; arr = NULL; };
#define SAFE_DELETE(ptr)  if(NULL!=ptr) { delete ptr; ptr = NULL; };

/*
 * Class for handling time intervals
 */
class IntervalTimer : public QObject
{
    Q_OBJECT
public:
    IntervalTimer(int intervalLengthSec);
    ~IntervalTimer();

    void    Reset();
    void    Tick(QDateTime currentDateTime);
    void    Tick(int64_t currentMs) { Tick(QDateTime::fromMSecsSinceEpoch(currentMs,
                                                                          QTimeZone(QTimeZone::systemTimeZoneId()))); }

    QDateTime GetCurrentDateTime()   { return m_currentDateTime; }
    QDateTime GetIntervalStartTime() { return m_currentDateTime; }
    QDateTime GetLastIntervalStart() { return m_lastIntervalStartTime; }
    bool      IsStarted()            { return m_intervalStarted; }
    bool      IsFinished()           { return m_intervalFinished; }
    int64_t   GetCurrentSecond()     { return m_currentDateTime.secsTo(m_intervalStartTime); }

signals:
    void    Started(QDateTime dateTime);
    void    Interval(QDateTime dateTime);

private:
    int        m_processingIntervalSec;
    QDateTime  m_currentDateTime;
    QDateTime  m_intervalStartTime;
    QDateTime  m_lastIntervalStartTime;

    bool       m_intervalStarted;
    bool       m_intervalFinished;
};

class VideoBuffer;

/*
 * Class-wrapper for decoded video frames handling
 * It stores raw yuv (rgb) data, user can control memory allocation/deallocation
 * Also it can provide cozy interface for other video frame classes such opencv, avframe, etc.
 */
class VideoFrame
{
public:
    VideoFrame();
    VideoFrame(int width, int height);
    VideoFrame(AVFrame* parent);
    VideoFrame(VideoFrame* parent);
    ~VideoFrame();

    void   CopyFromVideoFrame(VideoFrame* pSrcFrame);
    void   CopyFromAVFrame(AVFrame* pSrcFrame);
    void   CopyToAVFrame(AVFrame* pDstFrame);
    QImage      CreateQImageFromY(float scale = 1.0f);
    QImage      CreateQImageFromRGB(float scale = 1.0f);
    QImage*     GetQImagePtrFromY(float scale = 1.0f);
    QImage*     GetQImagePtrFromRGB(float scale = 1.0f);
    void        WriteToYUVFile(const char* fileName);

    void            GetYUVData(unsigned char** pY, unsigned char** pU, unsigned char** pV);
    unsigned char*  GetYData() { return m_pYBuffer; }
    unsigned char*  GetUData() { return m_pUBuffer; }
    unsigned char*  GetVData() { return m_pVBuffer; }
    unsigned char*  GetRGB24Data() { return m_pRGB24Buffer; }

    int         GetWidth() { return m_width; }
    int         GetHeight() { return m_height; }
    int         GetStride() { return m_stride; }
    int         GetWidthUV() { return m_width / 2; }
    int         GetHeightUV() { return m_height / 2; }
    int         GetStrideUV() { return m_strideUV; }
    int         GetStrideRGB() { return m_strideRGB24; }

    void        SetSize(int width, int height); /// Function to change frame size (with reallocating memory)
    void        UpdateRGB();                    /// Convert current YUV data to RGB

    double      nativeTimeInSeconds;
    int64_t     userTimestamp;
    int64_t     number;

    bool        isValid; /// Indicates error while decoding this frame

protected:
    int         m_width;
    int         m_height;
    int         m_stride;
    int         m_strideUV;
    int         m_strideRGB24;

    SwsContext* m_pSwsContext;

    unsigned char* m_pYBuffer;
    unsigned char* m_pUBuffer;
    unsigned char* m_pVBuffer;
    unsigned char* m_pRGB24Buffer;
};

/*
 * Class-wrapper for pixel buffer handling
 * It stores plane data, has several useful functions such add, average, etc.
 * In future, can be replaced with OpenCV buffer interface, or wrapper it
 */
class VideoBuffer
{
public:
    VideoBuffer();
    VideoBuffer(VideoBuffer* parent);
    VideoBuffer(VideoFrame* parent, int planeIdx);
    VideoBuffer(int width, int height);
    VideoBuffer(AVFrame* parent);
    ~VideoBuffer();

    void  CopyFrom(VideoBuffer *pSrc);
    void  CopyFrom(VideoFrame* pSrcFrame, int planeIdx);

    int             GetWidth() { return m_width; }
    int             GetHeight() { return m_height; }
    int             GetStride() { return m_stride; }
    unsigned char*  GetPlaneData() { return m_pBuffer; }

    QImage  CreateQImage(float scale = 1.0f);
    QImage* GetQImagePtr(float scale = 1.0f);
    void    WriteToYUVFile(const char* fileName);

    void  SetSize(int width, int height); /// Function to change size (with reallocating memory)

    void  SetRawData(int *pData, int width, int height, int stride);
    void  SetRawData(float *pData, int width, int height, int stride);
    void  SetRawData(unsigned char *pData, int width, int height, int stride);

    void  Add(VideoBuffer* pBufferToAdd, float k = 1.0f); /// Buf1 + k*Buf2
    void  Mul(VideoBuffer* pBufferToMul);                 /// Buf1 * Buf2
    void  MulToVal(float val);                            /// Buf * val
    void  AddVal(int val);                                /// Buf + val
    void  Avg(float *avg);                                /// average value of buffer
    void  Blur(int radius, float gain = 1.0f);
    void  Morph(int radius, int openOrClose);             /// 1 = open 0 = close
    void  SetVal(unsigned char val);                      /// Buf[i] = val
    void  Zero() { SetVal(0); }                           /// Buf[i] = 0
    void  Mask(VideoBuffer* pMsk);                        /// Buf[i] = (pMsk[i]) ? Buf[i] : 0;
    int   Binarize(int threshold, int value = 255);       /// Buf[i] = (Buf[i] > thr) ? value : 0;
    float AbsDiff(VideoBuffer* pBufferToDif);             /// Buf[i] = abs(Buf[i] - Buf2[i]);
    float AbsDiffLuma(VideoFrame* pFrameToDif);

    double      nativeTimeInSeconds;
    u_int64_t   userTimestamp;

protected:
    int             m_width;
    int             m_height;
    int             m_stride;
    unsigned char*  m_pBuffer;
};


/*
 * Class for pixel buffers accumulation
 */
class AccumlatorBuffer
{
public:
    AccumlatorBuffer();
    AccumlatorBuffer(int width, int height);
    ~AccumlatorBuffer();

    void CopyFrom(AccumlatorBuffer* pSrc);

    void Reset();
    void SetSize(int width, int height);

    void Add(AccumlatorBuffer* pInBuffer, float weight = 1.0f);
    void AddBuffer(VideoBuffer* pInBuffer);

    void Blur(int radius = 3);

    void ConvertTo(VideoBuffer* dst, bool doAverage = false);

    void WriteToYUVFile(const char* fileName, float div = 1.0f);

    void SetRawData(int width, int height, float* pData, int stride);

    int     GetWidth()  { return m_width; }
    int     GetHeight() { return m_height; }
    int     GetStride() { return m_stride; }
    float*  GetPlaneData() { return m_pBuffer; }

private:
    float*          m_pBuffer;
    int             m_width;
    int             m_height;
    int             m_stride;
    int             m_count;
};

/*
 *  Function for copy sps/pps data from nal unit to AVFormat codec parameters
 */
void FillSPSPPS(AVCodecParameters* pCodecContext, AVPacket* pPacket);

