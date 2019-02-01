#ifndef STREAMRECORDER_H
#define STREAMRECORDER_H

#include <QDateTime>
#include <QQueue>
#include <QObject>

#include "networkUtils/dataDirectory.h"
#include "cameraPipelineCommon.h"

#define  PACKET_BUFFER_SECONDS  70  // Store 70 sec of video stream (for writing event fragment files)

class PacketBuffer
{
public:
    PacketBuffer();
    ~PacketBuffer();

    void  AddPacket(QSharedPointer<AVPacket> pPacket);
    void  EnqueueWrite10SecFile(int64_t startTime, QString fileName);

    QString errorString;

    AVCodecParameters* inputCodecParams;
    AVRational         inputTimeBase;

private:
    QQueue<int64_t> m_startTimeQueue;
    QQueue<QString> m_fileNamesQueue;
    QVector<QSharedPointer<AVPacket>> m_pPackets;

    int         m_bufferSize;
    int         m_packetsWritten;
    int         m_currentIndex;

    void Write10SecFile(int64_t startTime, QString fileName);
};

class StreamRecorder : public QObject
{
    Q_OBJECT
public:
    StreamRecorder();
    ~StreamRecorder();

signals:
    void    NewFileOpened(QString newFileName);     /// Inform subscribers about actual archive file name
    void    Ping(const char* name, int timeoutMs);  /// Ping signal for health checker

public slots:
    void    CopyCodecParameters(AVStream *pVideoStream); /// Store input contexts parameters
    void    WritePacket(QSharedPointer<AVPacket> pInPacket);
    void    WriteEventFile(EventDescription event);
    void    Open();                             /// Init
    void    Close();                            /// Close current file and deinit

private slots:
    void    IntervalStarted(QDateTime currentTime);
    void    IntervalFinished(QDateTime currentTime);

private:
    QString             m_fileNamePattern;      /// Name pattern for output files
    QString             m_shortNamePattern;     /// Name pattern for archive file url

    int64_t             m_firstDts;             /// First packet dts for each file
    bool                m_fileOpened;           /// Indicates, whether archive file has been opened
    bool                m_needStartNewFile;     /// Indicates that we need to start new file on next received keyframe

    IntervalTimer*      m_pIntervalTimer;       /// Intervals handler

    AVRational          m_inputTimebase;
    AVCodecParameters*  m_pCodecParams;         /// Codec parameters from VideoEncoder (for writing packets from packetBuffer)
    AVFormatContext*    m_pFormatCtx;           /// Output format context
    AVStream*           m_pVideoStream;         /// Video stream pointer

    PacketBuffer*       m_pPacketBuffer;        /// Small packet buffer for storing last minute of video stream

    void  StartFile(QString startTime);         /// Open new file
    void  CloseFile();                          /// Close current file
};

#endif // STREAMRECORDER_H
