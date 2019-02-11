#ifndef RESULTVIDEOOUTPUT_H
#define RESULTVIDEOOUTPUT_H

#include <QObject>
#include <QString>
#include <QtWebSockets>

#include "networkUtils/dataDirectory.h"
#include "cameraPipelineCommon.h"

class AnalysisResults;

#define  DEFAULT_AVIO_BUFSIZE    (1024*1024)    // 1m buffer to be sure that it is enough for single video frame

class ResultVideoOutput : public QObject
{
    Q_OBJECT
public:
    ResultVideoOutput(QString outputURL);
    ~ResultVideoOutput();

    // Output to framented mp4 using websockets
    QByteArray          initialFragments;
    QQueue<QByteArray>  bufferedData;
    int64_t             lastSentTimestamp;

    QWebSocketServer*   pWebSocketServer;
    QList<QWebSocket*>  clients;

public slots:
    void    Close();
    void    Open(AVStream* pInStream);
    void    WritePacket(QSharedPointer<AVPacket> pInPacket);

    void    OnWsConnected();
    void    OnWsDisconnected();

private:
    QString             m_outputUrl;            /// Output stream location (network, file, etc...)
    int64_t             m_firstDts;             /// First packet timestamp
    bool                m_outputInitialized;    /// indicates, if output format initialized correctly

    /// AVLib stuff
    AVFormatContext*    m_pFormatCtx;
    AVStream*           m_pVideoStream;
    AVRational          m_inputTimeBase;
    AVCodecParameters*  m_pEncoderParams;
    AVIOContext*        m_pAVIOCtx;
    uint8_t*            m_pAvioCtxBuffer;
};

#endif // RESULTVIDEOOUTPUT_H
