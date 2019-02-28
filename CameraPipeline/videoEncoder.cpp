#include "videoEncoder.h"

VideoEncoder::VideoEncoder() :
    QObject(NULL),
    m_pCodecContext(NULL),
    m_pCodecParams(NULL),
    m_pFrame(NULL),
    m_isOpen(false),
    m_frameWidth(0),
    m_frameHeight(0)
{
    DataDirectory* pDataDir = DataDirectoryInstance::instance();

    m_bitrate = pDataDir->getPipelineParams().outputStreamBitrate();
    m_framerate = pDataDir->getPipelineParams().fps();
    m_scale = MAX(0.0f, MIN(4.0f, pDataDir->getPipelineParams().globalScale()));  // Limit scale coefficient
    m_targetWidth = 0;
}

VideoEncoder::VideoEncoder(int targetWidth) :
    QObject(NULL),
    m_pCodecContext(NULL),
    m_pCodecParams(NULL),
    m_pFrame(NULL),
    m_isOpen(false),
    m_frameWidth(0),
    m_frameHeight(0)
{
    DataDirectory* pDataDir = DataDirectoryInstance::instance();

    m_bitrate = pDataDir->getPipelineParams().outputStreamBitrate();
    m_framerate = pDataDir->getPipelineParams().fps();
    m_scale = 1.0f; // Scale coeff will be calculated later
    m_targetWidth = targetWidth;
}

VideoEncoder::~VideoEncoder()
{
    DEBUG_MESSAGE0("VideoEncoder", "~VideoEncoder() called");
    CloseCodecContext();
    DEBUG_MESSAGE0("VideoEncoder", "~VideoEncoder() finished");
}

void VideoEncoder::Open(AVCodecParameters *pParams)
{
    m_frameWidth = pParams->width;
    m_frameHeight = pParams->height;

    if (fabs(1.0f - m_scale) > 0.1f) // If we are going to scale input frames
    {
        m_frameWidth = (int)(pParams->width*m_scale + 0.5f)  & 0xFFFFFFFC; // width and height is better when even
        m_frameHeight = (int)(pParams->height*m_scale + 0.5f) & 0xFFFFFFFC;
    }
    else if (m_targetWidth > 0)
    {
        m_scale = (float)m_targetWidth / (float)pParams->width;
        m_frameWidth = (int)(pParams->width*m_scale + 0.5f)  & 0xFFFFFFFC; // width and height is better when even
        m_frameHeight = (int)(pParams->height*m_scale + 0.5f) & 0xFFFFFFFC;
    }

    OpenCodecContext();
}

void VideoEncoderH264::OpenCodecContext()
{
    AVCodec* pCodec;

    m_isOpen = false;
    DEBUG_MESSAGE0("VideoEncoderH264", "Open() called");

    // Find encoder
    pCodec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (NULL == pCodec)
    {
        ERROR_MESSAGE0(ERR_TYPE_ERROR, "VideoEncoderH264", "H264 encoder not found");
        return;
    }

    // Allocate codec context
    m_pCodecContext = avcodec_alloc_context3(pCodec);
    if (NULL == m_pCodecContext)
    {
        ERROR_MESSAGE0(ERR_TYPE_ERROR, "VideoEncoderH264", "Failed to allocate codec context");
        return;
    }

    m_pCodecContext->time_base = av_make_q(1, m_framerate);
    m_pCodecContext->gop_size = m_framerate;
    m_pCodecContext->pix_fmt = AV_PIX_FMT_YUV420P;
    m_pCodecContext->width = m_frameWidth;
    m_pCodecContext->height = m_frameHeight;
    m_pCodecContext->profile = FF_PROFILE_H264_BASELINE;

    // Open codec
    {
        AVDictionary*   options(0);

        av_dict_set(&options, "preset", "ultrafast", 0);
        av_dict_set(&options, "tune", "zerolatency", 0);
        av_dict_set(&options, "crf", "21", 0);

        if (0 > avcodec_open2(m_pCodecContext, pCodec, &options))
        {
            ERROR_MESSAGE0(ERR_TYPE_ERROR, "VideoEncoderH264", "Failed to open video codec");
            av_dict_free(&options);
            return;
        }
        av_dict_free(&options);
    }

    // Allocate and init AVFrame
    m_pFrame = av_frame_alloc();
    if (NULL == m_pFrame)
    {
        ERROR_MESSAGE0(ERR_TYPE_ERROR, "VideoEncoderH264", "Failed to allocate AVFrame");
        return;
    }

    m_currentPts = 0;

    m_pFrame->pts = m_currentPts;
    m_pFrame->format = AV_PIX_FMT_YUV420P;
    m_pFrame->width = m_frameWidth;
    m_pFrame->height = m_frameHeight;

    if (0 > av_frame_get_buffer(m_pFrame, 32))  // allocates aligned buffer for y,u,v planes
    {
        ERROR_MESSAGE0(ERR_TYPE_ERROR, "VideoEncoderH264", "Failed to alocate frame buffer");
        return;
    }

    // Get current codec parameters and send them to subscribers
    m_pCodecParams = avcodec_parameters_alloc();
    avcodec_parameters_from_context(m_pCodecParams, m_pCodecContext);

    emit NewParameters(m_pCodecParams);

    m_isOpen = true;
    DEBUG_MESSAGE0("VideoEncoderH264", "Video encoder opened successfully");
}

void VideoEncoder::CloseCodecContext()
{
    if(NULL != m_pCodecContext)
    {
        avcodec_close(m_pCodecContext);
        avcodec_free_context(&m_pCodecContext);
    }

    if(NULL != m_pCodecParams)
    {
        avcodec_parameters_free(&m_pCodecParams);
    }

    if(NULL != m_pFrame)
    {
        av_frame_free(&m_pFrame);
        m_pFrame = NULL;
    }
}

void VideoEncoder::EncodeFrame(VideoFrame* pCurrentFrame)
{
    DataDirectory*  pDataDirectory = DataDirectoryInstance::instance();
    Statistics      stats;

    // Encode frame
    if (m_isOpen)
    {
        int  encodeRes = 0;
        int  recvRes = 0;

        QSharedPointer<AVPacket> pPacket(av_packet_alloc(), [](AVPacket *pkt){av_packet_free(&pkt);});

        stats.startInterval();

        // Copy YUV data
        if (fabs(1.0f - m_scale) > 0.1f) // If we are going to scale input frames
        {
            m_VideoScaler.ScaleFrame(pCurrentFrame, m_pFrame);
        }
        else
        {
            pCurrentFrame->CopyToAVFrame(m_pFrame);
        }

        // Increment pts
        m_pFrame->pts = m_currentPts++;

        encodeRes = avcodec_send_frame(m_pCodecContext, m_pFrame);
        recvRes = avcodec_receive_packet(m_pCodecContext, pPacket.data()); // Generates ref-counted packet

        stats.endInterval("Encoding out");
        pDataDirectory->addStats(stats);

        if (!encodeRes && !recvRes)
        {
            // Packet duration is always 1 in 1/fps timebase
            pPacket.data()->duration = 1;

            // This is necessary for stream recorder and results output modules to know source frame's timestamp
            pPacket.data()->pos = pCurrentFrame->userTimestamp;

            DEBUG_MESSAGE0("VideoEncoder", "PacketReady() emitted. packet");
            emit PacketReady(pPacket);
        }
        else
        {
            char    err1[255] = {0};
            char    err2[255] = {0};
            av_make_error_string(err1, 255, encodeRes);
            av_make_error_string(err2, 255, recvRes);
            ERROR_MESSAGE2(ERR_TYPE_ERROR, "VideoEncoder", "Error encoding video frame   " \
                           "avcodec_send_frame(): %s\tavcodec_receive_packet(): %s", err1, err2);
            return;
        }
    }
    else
    {
        ERROR_MESSAGE0(ERR_TYPE_ERROR, "VideoEncoder", "Video encoder context has not been initialized properly");
    }
}

