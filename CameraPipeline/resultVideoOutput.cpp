
#include "resultVideoOutput.h"

#include "libavutil/opt.h"

ResultVideoOutput::ResultVideoOutput(QString outputURL) :
    pWebSocketServer(NULL),
    m_outputUrl(outputURL),
    m_outputInitialized(false),
    m_pFormatCtx(NULL),
    m_pVideoStream(NULL),
    m_pAVIOCtx(NULL),
    m_pAvioCtxBuffer(NULL)
{
    m_pEncoderParams = avcodec_parameters_alloc();
}

ResultVideoOutput::~ResultVideoOutput()
{
    DEBUG_MESSAGE0("ResultVideoOutput", "~ResultVideoOutput() called");
    avcodec_parameters_free(&m_pEncoderParams);
    DEBUG_MESSAGE0("ResultVideoOutput", "~ResultVideoOutput() finished");
}

void ResultVideoOutput::OnWsConnected()
{
    QWebSocket *pSocket = pWebSocketServer->nextPendingConnection();

    if (!initialFragments.size())
    {
        ERROR_MESSAGE0(ERR_TYPE_DISPOSABLE, "ResultVideoOutput",
                       "Client connected before initial fragments have been generated");
        pSocket->disconnect();
    }
    else
    {
        qDebug() << "Connection established from " << pSocket->peerAddress().toString();
        connect(pSocket, SIGNAL(disconnected()), this, SLOT(OnWsDisconnected()));
        clients << pSocket;
        // Sent initial data
        pSocket->sendBinaryMessage(initialFragments);
        qDebug() << "Number of active connections is " << clients.size();
    }
}

void ResultVideoOutput::OnWsDisconnected()
{
    QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());
    qDebug() << "WsDisconnected:" << pClient;

    if (pClient)
    {
        clients.removeAll(pClient);
        pClient->deleteLater();
    }
}

static int WritePacketCallback(void* opaque, uint8_t* buf, int size)
{
    ResultVideoOutput* pOutput = reinterpret_cast<ResultVideoOutput*>(opaque);

    const uint8_t ftypTag[4] = {'f','t','y','p'};
    const uint8_t moovTag[4] = {'m','o','o','v'};
    const uint8_t sidxTag[4] = {'s','i','d','x'};
    const uint8_t moofTag[4] = {'m','o','o','f'};

    if (size < 8)
    {
        ERROR_MESSAGE0(ERR_TYPE_MESSAGE, "ResultVideoOutput", "Too short fragment passed to callback");
    }

    // FTYP
    if (!memcmp(buf + 4, ftypTag, 4))
    {
        pOutput->initialFragments.clear();
        pOutput->initialFragments.append(QByteArray((char *)buf, size));
    }
    // MOOV
    else if (!memcmp(buf + 4, moovTag, 4))
    {
        pOutput->initialFragments.append(QByteArray((char *)buf, size));
    }
    // MOOF (or sidx+moov)
    else if (!memcmp(buf + 4, moofTag, 4) || !memcmp(buf + 4, sidxTag, 4))
    {
        Q_FOREACH (QWebSocket* client, pOutput->clients)
        {
            client->sendBinaryMessage(QByteArray((char *)buf, size));
        }
    }
    else
    {
        ERROR_MESSAGE0(ERR_TYPE_MESSAGE, "ResultVideoOutput", "Unsupported fragment type received");
    }
    return size;
}

void ResultVideoOutput::Open(AVStream* pInStream)
{
    DEBUG_MESSAGE0("ResultVideoOutput", "ReallocContexts() called");
    m_outputInitialized = false;

    AVCodecParameters* pCodecParams = pInStream->codecpar;

    // Copy encoder parameters
    avcodec_parameters_copy(m_pEncoderParams, pCodecParams);
    m_inputTimeBase = pInStream->time_base;

    if(NULL != m_pFormatCtx)
    {
        avformat_free_context(m_pFormatCtx);
        m_pFormatCtx = NULL;
    }

    // Trying to allocate output format context
    // Format should be .flv for streaming to rtmp
    ERROR_MESSAGE1(ERR_TYPE_MESSAGE, "ResultVideoOutput", "Will try to open %s", m_outputUrl.toUtf8().constData());

    if (m_outputUrl.startsWith("rtp"))
    {
        avformat_alloc_output_context2(&m_pFormatCtx, NULL, "rtp", m_outputUrl.toUtf8().constData());
    }
    else if (m_outputUrl.startsWith("rtmp"))
    {
        avformat_alloc_output_context2(&m_pFormatCtx, NULL, "flv", m_outputUrl.toUtf8().constData());
    }
    else if (m_outputUrl.startsWith("ws"))
    {
        avformat_alloc_output_context2(&m_pFormatCtx, NULL, "mp4", "out.mp4");
    }
    else
    {
        ERROR_MESSAGE0(ERR_TYPE_ERROR, "ResultVideoOutput", "Unsupported output format");
        return;
    }

    if (NULL == m_pFormatCtx)
    {
        ERROR_MESSAGE0(ERR_TYPE_ERROR, "ResultVideoOutput", "Failed to allocate output format context");
        return;
    }

    if (m_outputUrl.startsWith("ws"))
    {
        // Create WebSocket server, if needed
        int port = QUrl(m_outputUrl).port();
        // Websocket server
        pWebSocketServer = new QWebSocketServer(QStringLiteral("WsServer"), QWebSocketServer::NonSecureMode, this);
        if (pWebSocketServer->listen(QHostAddress::AnyIPv4, port))
        {
            ERROR_MESSAGE1(ERR_TYPE_ERROR, "ResultVideoOutput", "WS server listening on port %d", port);
            connect(pWebSocketServer, SIGNAL(newConnection()), this, SLOT(OnWsConnected()));
        }
        else
        {
            ERROR_MESSAGE1(ERR_TYPE_ERROR, "ResultVideoOutput", "Failed to run WebSocket server for %s",
                           m_outputUrl.toUtf8().constData());
            return;
        }

        m_pAvioCtxBuffer = (uint8_t *)av_malloc(DEFAULT_AVIO_BUFSIZE);
        if (nullptr == m_pAvioCtxBuffer)
        {
            ERROR_MESSAGE1(ERR_TYPE_ERROR, "ResultVideoOutput", "Failed to allocate %dk avio internal buffer", DEFAULT_AVIO_BUFSIZE / 1024);
            return;
        }

        m_pAVIOCtx = avio_alloc_context(m_pAvioCtxBuffer, DEFAULT_AVIO_BUFSIZE, 1, this, nullptr, &WritePacketCallback, nullptr);
        if (nullptr == m_pAVIOCtx)
        {
            ERROR_MESSAGE0(ERR_TYPE_ERROR, "ResultVideoOutput", "Failed to allocate avio context");
            return;
        }

        m_pFormatCtx->pb = m_pAVIOCtx;
        m_pFormatCtx->flags |= AVFMT_FLAG_CUSTOM_IO;

        // Creating output stream
        m_pVideoStream = avformat_new_stream(m_pFormatCtx, NULL);
        if (NULL == m_pVideoStream)
        {
            ERROR_MESSAGE0(ERR_TYPE_ERROR, "ResultVideoOutput", "Could not create output video stream");
            return;
        }

        // Set video stream defaults
        m_pVideoStream->index = 0;
        m_pVideoStream->time_base = av_make_q(1, 90000);

        // Set encoding parameters
        avcodec_parameters_copy(m_pVideoStream->codecpar, pCodecParams);
    }
    else
    {
        // Creating output stream
        m_pVideoStream = avformat_new_stream(m_pFormatCtx, NULL);
        if (NULL == m_pVideoStream)
        {
            ERROR_MESSAGE0(ERR_TYPE_ERROR, "ResultVideoOutput", "Could not create output video stream");
            return;
        }

        // Set video stream defaults
        m_pVideoStream->index = 0;
        m_pVideoStream->time_base = av_make_q(1, 90000);

        // Set encoding parameters
        avcodec_parameters_copy(m_pVideoStream->codecpar, pCodecParams);

        // Open output file, if it is allowed by format
        if (0 > avio_open(&m_pFormatCtx->pb, m_outputUrl.toUtf8().constData(), AVIO_FLAG_WRITE))
        {
            ERROR_MESSAGE0(ERR_TYPE_ERROR, "ResultVideoOutput", "Failed to open avio for writing");
            return;
        }

        ERROR_MESSAGE1(ERR_TYPE_MESSAGE, "ResultVideoOutput", "Extradata size before avformat_write_header() is %d bytes",
                       m_pVideoStream->codecpar->extradata_size);
    }

    AVDictionary* opts(0);

    if (m_outputUrl.startsWith("ws"))
    {
        av_dict_set(&opts, "movflags", "empty_moov+dash+default_base_moof+frag_keyframe", 0);
    }

    if (0 > avformat_write_header(m_pFormatCtx, &opts))
    {
        ERROR_MESSAGE0(ERR_TYPE_ERROR, "ResultVideoOutput", "avformat_write_header() failed");
        return;
    }

    // Printf format informtion
    av_dump_format(m_pFormatCtx, 0, m_outputUrl.toUtf8().constData(), 1);

    m_firstDts = AV_NOPTS_VALUE;

    m_outputInitialized = true;

    DEBUG_MESSAGE0("ResultVideoOutput", "Context reallocated successfully");
}

void ResultVideoOutput::WritePacket(QSharedPointer<AVPacket> pInPacket)
{
    DEBUG_MESSAGE2("ResultVideoOutput", "WritePacket() called, packet pts = %ld, size = %d", pInPacket->pts, pInPacket->size);

    if (!m_outputInitialized)
    {
        ERROR_MESSAGE0(ERR_TYPE_ERROR, "ResultVideoOutput", "Output context was not initialized properly");
        return;
    }

    // Clone packet because it is ref-counted and will be unrefed in av_interleaved_write_frame
    AVPacket*   pPacket = av_packet_clone(pInPacket.data());

    // Waiting for key frame to start file
    if (pInPacket->flags & AV_PKT_FLAG_KEY)
    {
        // Write format header to file
        FillSPSPPS(m_pVideoStream->codecpar, pPacket);
    }

    if ((AV_NOPTS_VALUE == m_firstDts))
    {
        m_firstDts = pPacket->dts;
    }

    pPacket->stream_index = m_pVideoStream->index;
    pPacket->dts -= m_firstDts;
    pPacket->pts -= m_firstDts;
    av_packet_rescale_ts(pPacket, m_inputTimeBase, m_pVideoStream->time_base);

    // Store frame timestamp
    // We are strongly counting on single call of WritePacketCallback
    // per each av_interleaved_write_frame() call
    lastSentTimestamp = pPacket->pts;

    int res = av_interleaved_write_frame(m_pFormatCtx, pPacket);
    av_packet_free(&pPacket);
    if (res < 0)
    {
        char err[255] = {0};
        av_make_error_string(err, 255, res);
        ERROR_MESSAGE1(ERR_TYPE_ERROR, "ResultVideoOutput", "av_interleaved_write_frame() error: %s", err);
        return;
    }
    DEBUG_MESSAGE0("ResultVideoOutput", "WritePacket() finished");
}

void ResultVideoOutput::Close()
{
    DEBUG_MESSAGE0("ResultVideoOutput", "CloseOutput() called");
    if (m_outputInitialized)
    {
        if (NULL != m_pFormatCtx)
        {
            av_write_trailer(m_pFormatCtx);
            if (!(m_pFormatCtx->flags & AVFMT_FLAG_CUSTOM_IO))
            {
                avio_closep(&m_pFormatCtx->pb);
            }
            avformat_free_context(m_pFormatCtx);
            m_pFormatCtx = NULL;
        }

        if (NULL != m_pAVIOCtx) // note: the internal buffer could have changed, and be != pAvioCtxBuffer
        {
            av_freep(&m_pAVIOCtx->buffer);
            av_freep(&m_pAVIOCtx);
        }
        SAFE_DELETE(pWebSocketServer);
        m_outputInitialized = false;
    }
    DEBUG_MESSAGE0("ResultVideoOutput", "CloseOutput() finished");
}
