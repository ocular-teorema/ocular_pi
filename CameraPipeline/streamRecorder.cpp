
#include <QMutex>
#include <QtConcurrent/QtConcurrent>
#include "streamRecorder.h"

StreamRecorder::StreamRecorder() :
    QObject(NULL),
    m_firstDts(AV_NOPTS_VALUE),
    m_fileOpened(false),
    m_needStartNewFile(false),
    m_pCodecParams(NULL),
    m_pFormatCtx(NULL),
    m_pVideoStream(NULL)
{

}

StreamRecorder::~StreamRecorder()
{
    DEBUG_MESSAGE0("StreamRecorder", "~StreamRecorder() called");

    if (NULL != m_pCodecParams)
    {
        avcodec_parameters_free(&m_pCodecParams);
    }

    if (NULL != m_pFormatCtx)
    {
        avformat_free_context(m_pFormatCtx);
        m_pFormatCtx = NULL;
    }

    SAFE_DELETE(m_pIntervalTimer);
    SAFE_DELETE(m_pPacketBuffer);
    DEBUG_MESSAGE0("StreamRecorder", "~StreamRecorder() finished");
}

void StreamRecorder::Open()
{
    DataDirectory* pDataDirectory = DataDirectoryInstance::instance();
    m_pIntervalTimer = new IntervalTimer(pDataDirectory->pipelineParams.processingIntervalSec);
    QObject::connect(m_pIntervalTimer, SIGNAL(Started(QDateTime)), this, SLOT(IntervalStarted(QDateTime)));
    QObject::connect(m_pIntervalTimer, SIGNAL(Interval(QDateTime)), this, SLOT(IntervalFinished(QDateTime)));

    m_fileNamePattern = pDataDirectory->pipelineParams.archivePath;
    m_fileNamePattern += '/';
    m_fileNamePattern += pDataDirectory->pipelineParams.pipelineName;
    m_fileNamePattern += '/';
    m_fileNamePattern += pDataDirectory->pipelineParams.pipelineName;
    m_fileNamePattern += "_%time%.mp4";

    m_shortNamePattern += '/';
    m_shortNamePattern += pDataDirectory->pipelineParams.pipelineName;
    m_shortNamePattern += '/';
    m_shortNamePattern += pDataDirectory->pipelineParams.pipelineName;
    m_shortNamePattern += "_%time%.mp4";

    m_pPacketBuffer = new PacketBuffer();
}

void StreamRecorder::Close()
{
    DEBUG_MESSAGE0("StreamRecorder", "Close() called");

    // At first - close current file
    CloseFile();

    if (NULL != m_pCodecParams)
    {
        avcodec_parameters_free(&m_pCodecParams);
    }

    if (NULL != m_pFormatCtx)
    {
        avformat_free_context(m_pFormatCtx);
        m_pFormatCtx = NULL;
    }

    SAFE_DELETE(m_pIntervalTimer);
    SAFE_DELETE(m_pPacketBuffer);
    DEBUG_MESSAGE0("StreamRecorder", "Close() finished");
}

void StreamRecorder::CopyCodecParameters(AVStream* pVideoStream)
{
    if (NULL != m_pCodecParams)
    {
        avcodec_parameters_free(&m_pCodecParams);
    }
    m_pCodecParams = avcodec_parameters_alloc();
    avcodec_parameters_copy(m_pCodecParams, pVideoStream->codecpar);
    m_inputTimebase = pVideoStream->time_base;

    // Same for PacketBuffer
    if (NULL != m_pPacketBuffer->inputCodecParams)
    {
        avcodec_parameters_free(&m_pPacketBuffer->inputCodecParams);
    }
    m_pPacketBuffer->inputCodecParams = avcodec_parameters_alloc();
    avcodec_parameters_copy(m_pPacketBuffer->inputCodecParams, pVideoStream->codecpar);
    m_pPacketBuffer->inputTimeBase = pVideoStream->time_base;
}

void StreamRecorder::StartFile(QString startTime)
{
    DataDirectory* pDataDirectory = DataDirectoryInstance::instance();

    if (pDataDirectory->analysisParams.useVirtualDate)
    {
        ERROR_MESSAGE0(ERR_TYPE_MESSAGE, "StreamRecorder", "Archive recording is disabled if virtual date is used");
        return;
    }

    if (!m_fileOpened)
    {
        QString fileName = m_fileNamePattern;
        QString shortFileName = m_shortNamePattern;

        fileName.replace("%time%", startTime);
        shortFileName.replace("%time%", startTime);

        DEBUG_MESSAGE1("StreamRecorder", "StartFile() called for %s", fileName.toUtf8().constData());

        avformat_alloc_output_context2(&m_pFormatCtx, NULL, NULL, fileName.toUtf8().constData());
        if (NULL == m_pFormatCtx)
        {
            ERROR_MESSAGE0(ERR_TYPE_ERROR, "StreamRecorder", "avformat_alloc_output_context2() failed");
            return;
        }

        m_pVideoStream = avformat_new_stream(m_pFormatCtx, NULL);
        if (NULL == m_pVideoStream)
        {
            ERROR_MESSAGE0(ERR_TYPE_ERROR, "StreamRecorder", "avformat_new_stream() failed");
            return;
        }

        m_pVideoStream->index = 0; // Default video stream index
        m_pVideoStream->time_base = av_make_q(1, DEFAULT_TIMEBASE);

        if (0 > avcodec_parameters_copy(m_pVideoStream->codecpar, m_pCodecParams))
        {
            ERROR_MESSAGE0(ERR_TYPE_ERROR, "StreamRecorder", "avcodec_parameters_copy() failed");
            return;
        }

        // Open output file, if it is allowed by format
        if (!(m_pFormatCtx->oformat->flags & AVFMT_NOFILE))
        {
            int res = avio_open(&m_pFormatCtx->pb, fileName.toUtf8().constData(), AVIO_FLAG_WRITE);
            if (res < 0)
            {
                char err[256];
                av_make_error_string(err, 255, res);
                ERROR_MESSAGE2(ERR_TYPE_ERROR, "StreamRecorder", "avio_open() failed for %s: %s",
                               fileName.toUtf8().constData(), err);
                return;
            }
        }

        if (0 > avformat_write_header(m_pFormatCtx, NULL))
        {
            ERROR_MESSAGE0(ERR_TYPE_ERROR, "RTSPCapture", "avformat_write_header() failed");
            return;
        }

        // Inform all subscribers, that we started new archive file
        emit NewFileOpened(shortFileName);

        m_fileOpened = true;
        m_firstDts = AV_NOPTS_VALUE;
        m_needStartNewFile = false;
        DEBUG_MESSAGE0("StreamRecorder", "StartFile() finished. File opened for writing");
    }
}

void StreamRecorder::CloseFile()
{
    DEBUG_MESSAGE0("StreamRecorder", "CloseFile() called");
    if (m_fileOpened)
    {
        // Write format trailer
        av_write_trailer(m_pFormatCtx);
        avio_closep(&m_pFormatCtx->pb);
        // Free output context
        avformat_free_context(m_pFormatCtx);
        m_pFormatCtx = NULL;
        DEBUG_MESSAGE0("StreamRecorder", "Archive file format context closed");
        m_fileOpened = false;
    }
}

void StreamRecorder::IntervalStarted(QDateTime currentTime)
{
    Q_UNUSED(currentTime);
    m_needStartNewFile = true;
}

void StreamRecorder::IntervalFinished(QDateTime currentTime)
{
    Q_UNUSED(currentTime);
    m_needStartNewFile = true;
}

void StreamRecorder::WritePacket(QSharedPointer<AVPacket> pInPacket)
{
    DEBUG_MESSAGE1("StreamRecorder", "NewEncodedFrame() called. TS = %ld", pInPacket->dts);

    QDateTime curDateTime = QDateTime::fromMSecsSinceEpoch(pInPacket->pos);

    // Store packet in buffer
    m_pPacketBuffer->AddPacket(pInPacket);

    // Check, if we have keyframe and need to start new file
    m_pIntervalTimer->Tick(curDateTime);

    if ((pInPacket->flags & AV_PKT_FLAG_KEY) && m_needStartNewFile)
    {
        CloseFile();
        StartFile(curDateTime.toString("dd_MM_yyyy___HH_mm_ss"));
        FillSPSPPS(m_pFormatCtx->streams[0]->codecpar, pInPacket.data());
        m_firstDts = pInPacket->dts;
    }

    if (m_fileOpened)
    {
        AVPacket* pPacket = av_packet_clone(pInPacket.data());
        pPacket->pts -= m_firstDts;
        pPacket->dts -= m_firstDts;
        pPacket->stream_index = 0; // Default video stream index
        av_packet_rescale_ts(pPacket, m_inputTimebase, m_pVideoStream->time_base);
        int res = av_interleaved_write_frame(m_pFormatCtx, pPacket);
        av_packet_free(&pPacket);
        if (res < 0)
        {
            ERROR_MESSAGE0(ERR_TYPE_ERROR, "StreamRecorder", "av_interleaved_write_frame() failed");
            return;
        }
    }
}

void StreamRecorder::WriteEventFile(EventDescription event)
{
    DataDirectory* pDataDirectory = DataDirectoryInstance::instance();

    QString videoPath;

    videoPath.sprintf("%s/%s/alertFragments/alert%d.mp4",
                      pDataDirectory->pipelineParams.archivePath.toUtf8().constData(),
                      pDataDirectory->pipelineParams.pipelineName.toUtf8().constData(),
                      event.id);

    DEBUG_MESSAGE1("StreamRecorder", "WriteEventFile() called. File name: %s", videoPath.toUtf8().constData());

    m_pPacketBuffer->EnqueueWrite10SecFile(event.startTime.toMSecsSinceEpoch(), videoPath);
}








//------------------------------------------------------------
//------------------------------------------------------------
//--- Small packet buffer for storing last minute of video ---
//------- And writing requested video interval to file -------
//------------------------------------------------------------
//------------------------------------------------------------

PacketBuffer::PacketBuffer()
{
    inputCodecParams = NULL;
    m_currentIndex = 0;
    m_packetsWritten = 0;
    m_bufferSize = PACKET_BUFFER_SECONDS * 30; // Assuming 30 fps is maximum that we will have
    m_pPackets.resize(m_bufferSize);
    m_pPackets.setSharable(true);
}

PacketBuffer::~PacketBuffer()
{
    avcodec_parameters_free(&inputCodecParams);
    m_pPackets.clear();
}

void PacketBuffer::AddPacket(QSharedPointer<AVPacket> pPacket)
{
    m_pPackets[m_currentIndex] = pPacket;
    m_currentIndex = (m_currentIndex + 1) % m_bufferSize;
    m_packetsWritten++;

    // Check, if we have enough packets to write next 10sec file
    if (!m_startTimeQueue.empty())
    {
        if (pPacket->pos - m_startTimeQueue.front() > 10000)
        {
            int64_t start = m_startTimeQueue.front(); m_startTimeQueue.pop_front();
            QString name = m_fileNamesQueue.front(); m_fileNamesQueue.pop_front();

            QtConcurrent::run(this, &PacketBuffer::Write10SecFile, start, name);
            //Write10SecFile(start, name);
        }
    }
}

void PacketBuffer::EnqueueWrite10SecFile(int64_t startTime, QString fileName)
{
    m_startTimeQueue.push_back(startTime);
    m_fileNamesQueue.push_back(fileName);
}

void PacketBuffer::Write10SecFile(int64_t startTime, QString fileName)
{
    AVFormatContext*  pFormatCtx = NULL;
    AVStream*         pStream = NULL;

    int  k = 0;
    bool keyFound = false;
    int  packetIndex = (m_currentIndex > 0) ? m_currentIndex - 1 : m_bufferSize - 1;

    // Find start index in buffer
    while (k < (m_bufferSize - 1))
    {
        if ((m_pPackets[packetIndex]->pos <= startTime) &&
            (m_pPackets[packetIndex]->flags & AV_PKT_FLAG_KEY))
        {
            keyFound = true;
            break;
        }
        // Navigate backward in circular buffer
        packetIndex = (packetIndex > 0) ? packetIndex - 1 : m_bufferSize - 1;
        k++;
    }

    if (!keyFound) // In theory we should never get here
    {
        ERROR_MESSAGE0(ERR_TYPE_DISPOSABLE, "StreamRecorder::PacketBuffer",
                       "Cannot write 10sec event file. I-frame before start position not found in circular buffer");
        return;
    }

    // Trying to allocate output format context
    avformat_alloc_output_context2(&pFormatCtx, NULL, "mp4", fileName.toUtf8().constData());
    if (NULL == pFormatCtx)
    {
        ERROR_MESSAGE0(ERR_TYPE_ERROR, "StreamRecorder::PacketBuffer",
                       "Cannot open format context for event video fragment file");
        return;
    }

    // Creating output stream
    pStream = avformat_new_stream(pFormatCtx, NULL);
    if (NULL == pStream)
    {
        ERROR_MESSAGE0(ERR_TYPE_DISPOSABLE, "StreamRecorder::PacketBuffer",
                       "Cannot create video stream for event video fragment file");
        return;
    }

    // Assign video stream defaults
    pStream->index = 0;
    pStream->time_base = av_make_q(1, DEFAULT_TIMEBASE);

    // Copy encoders parameters
    avcodec_parameters_copy(pStream->codecpar, inputCodecParams);

    if (0 > avio_open(&pFormatCtx->pb, fileName.toUtf8().constData(), AVIO_FLAG_WRITE))
    {
        ERROR_MESSAGE0(ERR_TYPE_DISPOSABLE, "StreamRecorder::PacketBuffer",
                       "Cannot open open avio for event video fragment file");
        return;
    }

    if (0 > avformat_write_header(pFormatCtx, NULL))
    {
        ERROR_MESSAGE0(ERR_TYPE_DISPOSABLE, "StreamRecorder::PacketBuffer",
                       "Cannot write format header into event video fragment file");
        return;
    }

    // Write packets to file
    int64_t firstDts = m_pPackets[packetIndex]->dts;
    int64_t firstMillis = m_pPackets[packetIndex]->pos;

    FillSPSPPS(pStream->codecpar, m_pPackets[packetIndex].data());

    while ((k--) && ((m_pPackets[packetIndex]->pos - firstMillis) < 10000))
    {
        AVPacket* pPacket = av_packet_clone(m_pPackets[packetIndex].data());

        pPacket->stream_index = 0;
        pPacket->dts -= firstDts;
        pPacket->pts -= firstDts;
        av_packet_rescale_ts(pPacket, inputTimeBase, pStream->time_base);

        if (0 != av_interleaved_write_frame(pFormatCtx, pPacket))
        {
            ERROR_MESSAGE0(ERR_TYPE_DISPOSABLE, "StreamRecorder::PacketBuffer",
                           "Error writing event fragment file av_interleaved_write_frame() failed");
            avformat_free_context(pFormatCtx);
            return;
        }
        packetIndex = (packetIndex + 1) % m_bufferSize;
    }

    if (0 != av_write_trailer(pFormatCtx))
    {
        ERROR_MESSAGE0(ERR_TYPE_DISPOSABLE, "StreamRecorder::PacketBuffer",
                       "Error writing trailer to event fragment file av_write_trailer() failed");
        return;
    }

    if (0 != avio_close(pFormatCtx->pb))
    {
        ERROR_MESSAGE0(ERR_TYPE_DISPOSABLE, "StreamRecorder::PacketBuffer",
                       "Error while closing event fragment file avio_close() failed");
        return;
    }
    avformat_free_context(pFormatCtx);
    return;
}

