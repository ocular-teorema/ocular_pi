
#include "frameCircularBuffer.h"

FrameCircularBuffer::FrameCircularBuffer(int size) : QObject(NULL)
{
    m_size = size;
    m_pFrameBuffer = new VideoFrame[size];
    m_readIndex = 0;
    m_writeIndex = 0;
    m_totalWritten = 0;
    m_totalRead = 0;
    m_firstFrameTime = -1.0;
}

FrameCircularBuffer::~FrameCircularBuffer()
{
    DEBUG_MESSAGE0("FrameCircularBuffer", "~FrameCircularBuffer() called");
    delete [] m_pFrameBuffer;
    DEBUG_MESSAGE0("FrameCircularBuffer", "~FrameCircularBuffer() finished");
}

void FrameCircularBuffer::GetFrame(VideoFrame** pCurrentFrame)
{
    int64_t         currentMsec = QDateTime::currentDateTime().toMSecsSinceEpoch();
    DataDirectory*  pDataDirectory = DataDirectoryInstance::instance();

    if (pDataDirectory->analysisParams.useVirtualDate)
    {
        QDate     newDate;
        QTime     newTime;
        QDateTime newDateTime;
        int64_t   frameDuration = 1000 / pDataDirectory->pipelineParams.fps; // in milliseconds

        newTime.setHMS( pDataDirectory->analysisParams.hour,
                        pDataDirectory->analysisParams.minute,
                        0);

        newDate.setDate( pDataDirectory->analysisParams.year,
                         pDataDirectory->analysisParams.month,
                         pDataDirectory->analysisParams.day);

        newDateTime.setDate(newDate);
        newDateTime.setTime(newTime);

        currentMsec = newDateTime.toMSecsSinceEpoch() + m_totalWritten*frameDuration;
    }

    DEBUG_MESSAGE1("FrameCircularBuffer", "GetFrame() called. Queue length = %d", m_totalWritten - m_totalRead);

    // Lock this section
    m_mutex.lock();

    int numFramesInBuf = m_totalWritten - m_totalRead;

    if (numFramesInBuf > 0)
    {
        *pCurrentFrame  = (m_pFrameBuffer + m_readIndex);
        // Set user timestamp to current msec from epoch
        (*pCurrentFrame)->userTimestamp = currentMsec;

        m_readIndex = (m_readIndex + 1) % m_size;
        m_totalRead++;
    }
    else
    {
        if (m_totalWritten)
        {
            int lastWrittenIndex = (m_writeIndex == 0) ? (m_size - 1) : (m_writeIndex - 1);
            *pCurrentFrame = m_pFrameBuffer + lastWrittenIndex;

            // Set user timestamp to current msec from epoch
            (*pCurrentFrame)->userTimestamp = currentMsec;

            DEBUG_MESSAGE0("FrameCircularBuffer", "No new frames in buffer yet. Repeating the last written.");
        }
        else
        {
            *pCurrentFrame = NULL;
            ERROR_MESSAGE0(ERR_TYPE_WARNING, "FrameCircularBuffer", "GetFrame() called, but no frames in buffer");
        }
    }
    m_mutex.unlock();
}

void FrameCircularBuffer::AddFrame(AVFrame* pNewFrame, double time)
{
    DEBUG_MESSAGE0("FrameCircularBuffer", "AddFrame() called");

    // Lock this section
    m_mutex.lock();
    m_pFrameBuffer[m_writeIndex].CopyFromAVFrame(pNewFrame);
    m_pFrameBuffer[m_writeIndex].nativeTimeInSeconds = time;
    m_pFrameBuffer[m_writeIndex].number = m_totalWritten;
    // Move to next position
    m_writeIndex = (m_writeIndex + 1) % m_size;
    m_totalWritten++;

    DEBUG_MESSAGE2("FrameCircularBuffer", "Frame added. Queue length = %d, native time = %f",
                   m_totalWritten - m_totalRead, m_pFrameBuffer[m_writeIndex].nativeTimeInSeconds);

    // Set first frame time
    if (m_firstFrameTime < 0.0)
    {
        m_firstFrameTime = m_pFrameBuffer[m_writeIndex].nativeTimeInSeconds;
    }

    emit FrameAdded();

    // Check, if buffer was overflowed
    if (m_totalWritten - m_totalRead >= m_size)
    {
        ERROR_MESSAGE0(ERR_TYPE_DISPOSABLE, "FrameCircularBuffer", "Frame buffer overflow");
        // Skip frames
        m_readIndex = m_writeIndex;
        m_totalRead = m_totalWritten;
    }
    m_mutex.unlock();
}
