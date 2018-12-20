#ifndef FRAMECIRCULARBUFFER_H
#define FRAMECIRCULARBUFFER_H

#include "cameraPipelineCommon.h"
#include "videoScaler.h"


/*
 * Class for storing video frames from capture interface
 * It allows to get latest current and previous frames for processing
 * and check, if there was an overflow
 */
class FrameCircularBuffer : public QObject
{
    Q_OBJECT
public:
    FrameCircularBuffer(int size);
    ~FrameCircularBuffer();

    void    GetFrame(VideoFrame** pFrame, double time);
    void    AddFrame(AVFrame* pNewFrame, double time);

signals:
    void    FirstFrameAdded();

private:
    unsigned int m_size;            /// Buffer size
    unsigned int m_readIndex;       /// Position for reading next frame
    unsigned int m_writeIndex;      /// Position for writing next frame
    unsigned int m_totalWritten;    /// How many frames already written
    unsigned int m_totalRead;       /// How many frames has been read
    double       m_firstFrameTime;  /// Native timestamp of first frame (in seconds)

    VideoFrame*  m_pFrameBuffer;    /// Buffer with allocated frames
    QMutex       m_mutex;
};

#endif // FRAMECIRCULARBUFFER_H
