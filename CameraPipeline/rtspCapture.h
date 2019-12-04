#ifndef RTSPCAPTURE_H
#define RTSPCAPTURE_H

#include <QObject>
#include <QTimer>

#include "networkUtils/dataDirectory.h"
#include "cameraPipelineCommon.h"
#include "frameCircularBuffer.h"

class RTSPCapture : public QObject
{
    Q_OBJECT
public:
    RTSPCapture(QString uri, FrameCircularBuffer *pFrameBuffer = NULL);
    ~RTSPCapture();

    static int ProbeStreamParameters(const char *url, double* fps, int* w, int* h);

signals:
    void    NewCodecParams(AVStream* pCodecParams);                 /// Signal about new input codec parameters
    void    NewPacketReceived(QSharedPointer<AVPacket> pPacket);    /// Signal that we have read new packet
    void    Ping(const char* name, int timeoutMs);                  /// Ping signal for health checker

public slots:
    void    StopCapture();
    void    StartCapture();
    void    SetPause(bool on);
    void    StepFrame();

private slots:
    void    CaptureNewFrame();      /// Main function

private:
    QTimer*     m_pCaptureTimer;    /// Main loop timer. It will call CaptureNewFrame()
                                    /// as often as possible and keep all events alive.
                                    /// Must be created within the same thread Capture instance is running in

    QString     m_rtspUri;          /// RTSP (or other) stream network address

    float       m_fps;              /// Expected fps value
    bool        m_doDecoding;       /// If we don't have any video analysis - received frames should not be decoded

    FrameCircularBuffer*    m_pFrameBuffer; /// Pointer to circular buffer for frames exchange with analyzer (must be set from outside)

    /// AVLib related stuff
    AVFormatContext*        m_pInputContext;
    AVCodecContext*         m_pCodecContext;
    AVFrame*                m_pFrame;           /// Decoded frame locates here
    int                     m_videoStreamIndex;
    int                     m_readErrorNumber;  /// Number of read frame error in a row
    bool                    m_stop;             /// Flag to exit from while loop
    int64_t                 m_framesToSnapshot; /// Number of frames left to next snapshot

    ErrorCode   InitCapture();                      /// All init and allocation AVLib routines
    ErrorCode   DecodeSingleKey(AVPacket *pPacket); /// Decode keyframe time-to-time to create snapshot
};

#endif // RTSPCAPTURE_H
