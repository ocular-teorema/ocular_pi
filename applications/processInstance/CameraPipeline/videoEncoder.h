#ifndef VIDEOENCODER_H
#define VIDEOENCODER_H

#include <QObject>
#include "videoScaler.h"

class VideoEncoder : public QObject
{
    Q_OBJECT
public:
    VideoEncoder();
    VideoEncoder(int targetWidth);
    ~VideoEncoder();

signals:
    void    PacketReady(QSharedPointer<AVPacket> pPacket);  // This packet should be unrefed inside VideoEncoder
    void    NewParameters(AVCodecParameters* pParams);

public slots:
    void    EncodeFrame(VideoFrame* pCurrentFrame);
    void    Close() { CloseCodecContext(); }
    void    Open(AVCodecParameters* pParams);

protected:
    uint64_t            m_currentPts;       /// Pts for output frames
    int                 m_bitrate;          /// Bitrate of output stream (or crf value if used)
    int                 m_framerate;        /// Framerate (for timebase calculation)

    AVCodecContext*     m_pCodecContext;
    AVCodecParameters*  m_pCodecParams;
    AVFrame*            m_pFrame;
    VideoScaler         m_VideoScaler;      /// Scaler for input frames

    float               m_scale;            /// Scaling coefficient for output frames
    int                 m_targetWidth;      /// Target width for scaling

    bool                m_isOpen;
    int                 m_frameWidth;
    int                 m_frameHeight;

    void                CloseCodecContext();
    virtual void        OpenCodecContext() = 0;
};

class VideoEncoderH264 : public VideoEncoder
{
public:
    VideoEncoderH264() : VideoEncoder() {}
    VideoEncoderH264(int targetWidth) : VideoEncoder(targetWidth) {}

protected:
    virtual void        OpenCodecContext();
};

//class VideoEncoderFLV : public VideoEncoder
//{
//protected:
//    virtual void        OpenCodecContext(int width, int height);
//};

#endif // VIDEOENCODER_H
