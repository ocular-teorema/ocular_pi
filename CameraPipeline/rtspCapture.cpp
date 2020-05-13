
#include <QUrl>
#include <QThread>
#include <QElapsedTimer>

#include "rtspCapture.h"

RTSPCapture::RTSPCapture(QString uri) :
    QObject(NULL),
    m_pCaptureTimer(NULL),
    m_pInputContext(NULL),
    m_pCodecContext(NULL),
    m_pFrame(NULL),
    m_videoStreamIndex(0),
    m_audioStreamIndex(0),
    m_readErrorNumber(0),
    m_framesDecoded(0)
{
    m_fps = 30;
    m_rtspUri = uri;
    m_doDecoding = (DataDirectoryInstance::instance())->analysisParams.differenceBasedAnalysis ||
                   (DataDirectoryInstance::instance())->analysisParams.motionBasedAnalysis;
}

RTSPCapture::~RTSPCapture()
{
    DEBUG_MESSAGE0("RTSPCapture", "~RTSPCapture() called");
    SAFE_DELETE(m_pCaptureTimer);

    if(NULL != m_pFrame)
    {
        av_frame_free(&m_pFrame);
    }

    if(NULL != m_pCodecContext)
    {
        avcodec_close(m_pCodecContext);
        avcodec_free_context(&m_pCodecContext);
    }

    if(NULL != m_pInputContext)
    {
        avformat_close_input(&m_pInputContext);
    }

    DEBUG_MESSAGE0("RTSPCapture", "~RTSPCapture() finished");
}

ErrorCode RTSPCapture::DecodeSingleKey(AVPacket* pPacket)
{
    // Decode frame
    int sendRes = avcodec_send_packet(m_pCodecContext, pPacket);
    int decodeRes = avcodec_receive_frame(m_pCodecContext, m_pFrame);

    if (sendRes || decodeRes)
    {
        char err1[255] = {0};
        char err2[255] = {0};
        av_make_error_string(err1, 255, sendRes);
        av_make_error_string(err2, 255, decodeRes);
        ERROR_MESSAGE2(ERR_TYPE_DISPOSABLE, "RTSPCapture",
                       "Error decoding single keyframe avcodec_send_packet(): %s\tavcodec_receive_frame(): %s",
                       err1, err2);
        return CAMERA_PIPELINE_ERROR;
    }

    // Create Image and store it in archive folder
    QImage img(m_pFrame->data[0], m_pFrame->width, m_pFrame->height, m_pFrame->linesize[0], QImage::Format_Indexed8);
    for(int i = 0; i < 256; i++)
    {
        img.setColor(i, qRgb(i, i, i));
    }
    QString thumbFile = (DataDirectoryInstance::instance())->pipelineParams.archivePath;
    thumbFile += '/';
    thumbFile += (DataDirectoryInstance::instance())->pipelineParams.pipelineName;
    thumbFile += "/thumb.jpg";
    img.save(thumbFile);

    return CAMERA_PIPELINE_OK;
}

ErrorCode RTSPCapture::InitCapture()
{
    int             res;
    QUrl            testUrl(m_rtspUri);
    AVDictionary*   inputOptions(NULL);
    AVCodec*        pCodec;

    if(!testUrl.isValid())
    {
        ERROR_MESSAGE0(ERR_TYPE_STATUS, "RTSPCapture", "Invalid rtsp url");
        ERROR_MESSAGE0(ERR_TYPE_ERROR,  "RTSPCapture", "Invalid rtsp url");
        return CAMERA_PIPELINE_ERROR;
    }

    ERROR_MESSAGE0(ERR_TYPE_STATUS, "RTSPCapture", "Connecting...");

    av_dict_set(&inputOptions, "rtsp_transport", "tcp", 0);
    res = avformat_open_input(&m_pInputContext, m_rtspUri.toLatin1().data(), NULL, &inputOptions);
    av_dict_free(&inputOptions);

    if (res < 0)
    {
        // Check return code
        switch (res) {
        case AVERROR_HTTP_BAD_REQUEST:
            ERROR_MESSAGE0(ERR_TYPE_STATUS, "RTSPCapture", "Bad request");
            break;
        case AVERROR_HTTP_UNAUTHORIZED:
            ERROR_MESSAGE0(ERR_TYPE_STATUS, "RTSPCapture", "Authorization error");
            break;
        case AVERROR_HTTP_FORBIDDEN:
            ERROR_MESSAGE0(ERR_TYPE_STATUS, "RTSPCapture", "Forbidden");
            break;
        case AVERROR_HTTP_NOT_FOUND:
            ERROR_MESSAGE0(ERR_TYPE_STATUS, "RTSPCapture", "Resource not found");
            break;
        case AVERROR_HTTP_SERVER_ERROR:
            ERROR_MESSAGE0(ERR_TYPE_STATUS, "RTSPCapture", "Internal server error");
            break;
        case AVERROR_HTTP_OTHER_4XX:
            ERROR_MESSAGE0(ERR_TYPE_STATUS, "RTSPCapture", "Unknown error");
            break;
        case AVERROR_INVALIDDATA:
            ERROR_MESSAGE0(ERR_TYPE_STATUS, "RTSPCapture", "Invalid data (default err)");
            break;
        case AVERROR(ETIMEDOUT):
            ERROR_MESSAGE0(ERR_TYPE_STATUS, "RTSPCapture", "Connection timed out");
            break;
        case AVERROR(EHOSTUNREACH):
            ERROR_MESSAGE0(ERR_TYPE_STATUS, "RTSPCapture", "No route to host");
            break;
        case AVERROR(ECONNREFUSED):
            ERROR_MESSAGE0(ERR_TYPE_STATUS, "RTSPCapture", "Connection refused");
            break;
        case AVERROR(ECONNRESET):
            ERROR_MESSAGE0(ERR_TYPE_STATUS, "RTSPCapture", "Connection reset by peer");
            break;
        case AVERROR(EPROTONOSUPPORT):
            ERROR_MESSAGE0(ERR_TYPE_STATUS, "RTSPCapture", "Protocol not supported");
            break;
        }

        char err[512] = {0};
        av_make_error_string(err, 511, res);
        ERROR_MESSAGE1(ERR_TYPE_ERROR, "RTSPCapture", "Failed to open rtsp stream: %s", err);
        return CAMERA_PIPELINE_ERROR;
    }

    res = avformat_find_stream_info(m_pInputContext, NULL);
    if (res < 0)
    {
        ERROR_MESSAGE0(ERR_TYPE_STATUS, "RTSPCapture", "Failed to get stream information");
        ERROR_MESSAGE0(ERR_TYPE_ERROR,  "RTSPCapture", "Failed to get rtsp stream info");
        return CAMERA_PIPELINE_ERROR;
    }

    // Dump information about file onto standard error
    av_dump_format(m_pInputContext, 0, m_rtspUri.toLatin1().data(), 0);

    // Find the first video stream
    m_videoStreamIndex = av_find_best_stream(m_pInputContext, AVMEDIA_TYPE_VIDEO, -1, -1, &pCodec, 0);
    m_audioStreamIndex = av_find_best_stream(m_pInputContext, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    if (m_videoStreamIndex < 0 || (NULL == pCodec))
    {
        ERROR_MESSAGE0(ERR_TYPE_STATUS, "RTSPCapture", "Bad video stream or unsupported codec");
        ERROR_MESSAGE0(ERR_TYPE_ERROR,  "RTSPCapture", "Unable to find video stream or appropriate decoder");
        return CAMERA_PIPELINE_ERROR;
    }

    // Check for mp4 compatible audio codec format
    // https://ru.wikipedia.org/wiki/MPEG-4_Part_14
    if (m_audioStreamIndex >= 0 &&
        m_pInputContext->streams[m_audioStreamIndex]->codecpar->codec_id != AV_CODEC_ID_AAC &&
        m_pInputContext->streams[m_audioStreamIndex]->codecpar->codec_id != AV_CODEC_ID_AAC_LATM &&
        m_pInputContext->streams[m_audioStreamIndex]->codecpar->codec_id != AV_CODEC_ID_MP1 &&
        m_pInputContext->streams[m_audioStreamIndex]->codecpar->codec_id != AV_CODEC_ID_MP2 &&
        m_pInputContext->streams[m_audioStreamIndex]->codecpar->codec_id != AV_CODEC_ID_MP3 &&
        m_pInputContext->streams[m_audioStreamIndex]->codecpar->codec_id != AV_CODEC_ID_AC3)
    {
        ERROR_MESSAGE0(ERR_TYPE_STATUS, "RTSPCapture", "Unsupported audio codec. Audio recording will be disabled");
        m_audioStreamIndex = -1;
    }

    // Set real framerate
    (DataDirectoryInstance::instance())->pipelineParams.fps =
        FFMIN(60, FFMAX(5, (int)(0.5 + av_q2d(m_pInputContext->streams[m_videoStreamIndex]->avg_frame_rate))));

    // Calculate proper downscale coefficient
    if ((DataDirectoryInstance::instance())->analysisParams.downscaleCoeff <= 0)
    {
        double coeff = 0.4;
        int w = m_pInputContext->streams[m_videoStreamIndex]->codecpar->width;
        coeff = (w > 720)  ? 0.3  : coeff;
        coeff = (w > 1200) ? 0.25 : coeff;
        coeff = (w > 1900) ? 0.15 : coeff;
        coeff = (w > 2000) ? 0.10 : coeff;
        coeff = (w > 2500) ? 0.08 : coeff;
        (DataDirectoryInstance::instance())->analysisParams.downscaleCoeff = coeff;
    }

    // Create codec context
    m_pCodecContext = avcodec_alloc_context3(pCodec);
    if (NULL == m_pCodecContext)
    {
        ERROR_MESSAGE0(ERR_TYPE_STATUS, "RTSPCapture", "Internal ffmpeg error");
        ERROR_MESSAGE0(ERR_TYPE_ERROR,  "RTSPCapture", "Unable to allocate decoder context");
        return CAMERA_PIPELINE_ERROR;
    }

    // Copy input codec parameters
    avcodec_parameters_to_context(m_pCodecContext, m_pInputContext->streams[m_videoStreamIndex]->codecpar);

    // This field need to be set for cathing errors while decoding
    m_pCodecContext->err_recognition = AV_EF_EXPLODE;

    // Set framerate value
    m_pCodecContext->framerate.num = m_fps;
    m_pCodecContext->framerate.den = 1;

    res = avcodec_open2(m_pCodecContext, pCodec, NULL);
    if (res < 0)
    {
        ERROR_MESSAGE0(ERR_TYPE_STATUS, "RTSPCapture", "Failed to open video decoder");
        ERROR_MESSAGE0(ERR_TYPE_ERROR,  "RTSPCapture", "Failed to open video decoder");
        return CAMERA_PIPELINE_ERROR;
    }

    // Required for flv format output (to avoid "avc1/0x31637661 incompatible with output codec id 28")
    m_pInputContext->streams[m_videoStreamIndex]->codecpar->codec_tag = 0;

    // Notify stream recorder, tha we have new codec context
    emit NewCodecParams(m_pInputContext->streams[m_videoStreamIndex],
                        (m_audioStreamIndex < 0) ? NULL : m_pInputContext->streams[m_audioStreamIndex]);

    m_pFrame = av_frame_alloc();
    if(m_pFrame == NULL)
    {
        ERROR_MESSAGE0(ERR_TYPE_STATUS, "RTSPCapture", "Failed to allocate AVFrame");
        ERROR_MESSAGE0(ERR_TYPE_ERROR,  "RTSPCapture", "Failed to allocate AVFrame");
        return CAMERA_PIPELINE_ERROR;
    }

    // Frame capturing performed on capture timers' timeout event
    // This made to prevent event loop blocking with while(1)
    // This timer should be created here, because InitCapture function
    // Is called after RTSPCapture object moved to it's separate thread
    m_pCaptureTimer = new QTimer;
    m_pCaptureTimer->setTimerType(Qt::PreciseTimer);
    m_pCaptureTimer->setInterval(5); // Read with high fps, since we might have a lot of streams including audio
    QObject::connect(m_pCaptureTimer, SIGNAL(timeout()), this, SLOT(CaptureNewFrame()));
    DEBUG_MESSAGE1("RTSPCapture", "Capture thread created id = %p", QThread::currentThreadId());

    ERROR_MESSAGE0(ERR_TYPE_STATUS, "RTSPCapture", "Successfully connected");
    return CAMERA_PIPELINE_OK;
}

void RTSPCapture::CaptureNewFrame()
{
    int             readRes = -1;
    int             sendRes = -1;
    int             decodeRes = -1;

    QSharedPointer<AVPacket> pPacket(av_packet_alloc(), [](AVPacket *pkt){av_packet_free(&pkt);});

    DEBUG_MESSAGE0("RTSPCapture", "Capture frame started");

    while (!m_stop && (readRes = av_read_frame(m_pInputContext, pPacket.data())) >= 0) // while we have available frames in stream
    {
        if (pPacket.data()->stream_index == m_videoStreamIndex)  // We need only video frames to be decoded
        {
            if (m_doDecoding)
            {
                // Decode frame
                sendRes = avcodec_send_packet(m_pCodecContext, pPacket.data());
                decodeRes = avcodec_receive_frame(m_pCodecContext, m_pFrame);

                if (sendRes || decodeRes)
                {
                    char err1[255] = {0};
                    char err2[255] = {0};
                    av_make_error_string(err1, 255, sendRes);
                    av_make_error_string(err2, 255, decodeRes);
                    ERROR_MESSAGE2(ERR_TYPE_MESSAGE, "RTSPCapture",
                                   "Error decoding video frame avcodec_send_packet(): %s\tavcodec_receive_frame(): %s",
                                   err1, err2);
                }
            }
            else
            {
                if (pPacket->flags & AV_PKT_FLAG_KEY)
                {
                    // Decode single keyframe each ~63 keyframes and create a snapshot from it
                    if ((m_framesDecoded & 63) == 0)
                    {
                        DecodeSingleKey(pPacket.data());
                    }
                    m_framesDecoded++;
                }
            }

            pPacket->pos = QDateTime::currentMSecsSinceEpoch(); // Set server's timestamp to packet

            if (!m_stop)
            {
                emit NewPacketReceived(pPacket);    // Send new packet signal
            }
            // Exit reading while
            break;
        }
        else if (pPacket.data()->stream_index == m_audioStreamIndex)
        {
            if (!m_stop)
            {
                emit NewAudioPacketReceived(pPacket);    // Send new packet signal
            }
            break;
        }
        av_packet_unref(pPacket.data());
    }

    // Send ping to health checker that reading in still in progress
    emit Ping("RTSPCapture", HANG_TIMEOUT_MSEC);

    // Add all decoded frames to the pipeline
    if (decodeRes == 0)
    {
        m_framesDecoded++;

        // Add new frame to analyzing queue
        if (m_doDecoding)
        {
            if (m_pFrame->format != AV_PIX_FMT_YUV420P && m_pFrame->format != AV_PIX_FMT_YUVJ420P)
            {
                ERROR_MESSAGE0(ERR_TYPE_DISPOSABLE, "RTSPCapture", "Unsupported frame format");
                return;
            }

            // Save snapshot for each ~4096 frames
            if ((m_framesDecoded & 4095) == 0)
            {
                // Create Image and store it in archive folder
                QImage img(m_pFrame->data[0], m_pFrame->width, m_pFrame->height, m_pFrame->linesize[0], QImage::Format_Indexed8);
                for(int i = 0; i < 256; i++)
                {
                    img.setColor(i, qRgb(i, i, i));
                }
                QString thumbFile = (DataDirectoryInstance::instance())->pipelineParams.archivePath;
                thumbFile += '/';
                thumbFile += (DataDirectoryInstance::instance())->pipelineParams.pipelineName;
                thumbFile += "/thumb.jpg";
                img.save(thumbFile);
            }

            double frameTime = 0.0f;
            if (m_pFrame->best_effort_timestamp != AV_NOPTS_VALUE)
                frameTime = av_q2d(m_pInputContext->streams[m_videoStreamIndex]->time_base) * m_pFrame->best_effort_timestamp;
            else if (m_pFrame->pkt_dts != AV_NOPTS_VALUE)
                frameTime = av_q2d(m_pInputContext->streams[m_videoStreamIndex]->time_base) * m_pFrame->pkt_dts;
            else
                frameTime = av_q2d(av_inv_q(m_pInputContext->streams[m_videoStreamIndex]->avg_frame_rate)) * m_pFrame->display_picture_number;

            QSharedPointer<VideoFrame> pDecodedFrame(new VideoFrame(m_pFrame));
            pDecodedFrame->userTimestamp = QDateTime::currentDateTime().toMSecsSinceEpoch();
            pDecodedFrame->nativeTimeInSeconds = frameTime;
            pDecodedFrame->number = m_framesDecoded;

            emit NewFrameDecoded(pDecodedFrame);
        }
    }

    if (readRes < 0)
    {
        char err[255];
        av_make_error_string(err, 255, readRes);
        ERROR_MESSAGE1(ERR_TYPE_MESSAGE, "RTSPCapture", "Read frame error: %s", err);
        m_readErrorNumber++;

        if (m_readErrorNumber > 300 && !m_stop)
        {
            StopCapture(); // Stop further reading attempts
            ERROR_MESSAGE0(ERR_TYPE_STATUS,   "RTSPCapture", "Error reading stream. Disconnecting.");
            ERROR_MESSAGE0(ERR_TYPE_CRITICAL, "RTSPCapture", "300 read frame errors in a row");
        }
    }
    else
    {
        m_readErrorNumber = 0;
    }
}

void RTSPCapture::StartCapture()
{
    DEBUG_MESSAGE0("RTSPCapture", "StartCapture() called");

    if (CAMERA_PIPELINE_OK != InitCapture())
    {
        ERROR_MESSAGE0(ERR_TYPE_CRITICAL, "RTSPCapture", "Failed to init capture process");
        return;
    }

    m_readErrorNumber = 0;
    m_stop = false;

    if (m_pCaptureTimer != NULL)
    {
        m_pCaptureTimer->start();
    }
}

void RTSPCapture::SetPause(bool on)
{
    if (on)
    {
        DEBUG_MESSAGE0("RTSPCapture", "SetPause(true) called, pausing");
        m_pCaptureTimer->stop();
    }
    else
    {
        DEBUG_MESSAGE0("RTSPCapture", "SetPause(false) called, resuming");
        m_pCaptureTimer->start();
    }
}

void RTSPCapture::StepFrame()
{
    if(!m_pCaptureTimer->isActive())
    {
        QTimer::singleShot(1000.0f / m_fps, this, SLOT(CaptureNewFrame()));
    }
}

void RTSPCapture::StopCapture()
{
    DEBUG_MESSAGE0("RTSPCapture", "StopCapture() called");

    m_stop = true;

    if (m_pCaptureTimer != NULL)
    {
        m_pCaptureTimer->stop();
    }
}
