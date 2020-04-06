
#include <QThread>
#include <QTimer>
#include <QElapsedTimer>

#include "intervalStatistics.h"

void IntervalStatistics::Reset()
{
    accBuffer.Reset();
    backgroundBuffer.SetVal(0);
    perFrameMovements.clear();
    motionMap.Reset();
}

IntervalStatistics::~IntervalStatistics()
{
    perFrameMovements.clear();
}

void IntervalStatistics::CopyFrom(IntervalStatistics& src)
{
    accBuffer.CopyFrom(&src.accBuffer);
    backgroundBuffer.CopyFrom(&src.backgroundBuffer);
    perFrameMovements = src.perFrameMovements;
    motionMap.CopyFrom(&src.motionMap);
    date = src.date;
    endDate = src.endDate;
    startTime = src.startTime;
    endTime = src.endTime;
}

ErrorCode IntervalStatistics::ToByteArray(QByteArray* bytes)
{
    unsigned int size;
    unsigned int width;
    unsigned int height;
    unsigned int magic = 0xDEADBEEF;
    unsigned int version = 0x00000001;

    if (NULL == bytes)
    {
        return CAMERA_PIPELINE_ERROR;
    }

    // Validation numbers
    bytes->append((char *)&magic,   sizeof(magic));
    bytes->append((char *)&version, sizeof(version));

    // Accumulator buffer size and data
    width = accBuffer.GetWidth();
    height = accBuffer.GetHeight();

    bytes->append((char *)&(width), sizeof(width));
    bytes->append((char *)&(height), sizeof(height));

    bytes->append((char*)accBuffer.GetPlaneData(), width*height*sizeof(float));

    // Validation numbers
    magic = 0x00000000;
    bytes->append((char *)&magic, sizeof(magic));

    // Movements vector size and data
    size = perFrameMovements.size();
    bytes->append((char *)&(size), sizeof(size));
    bytes->append((char *)(perFrameMovements.constData()), sizeof(perFrameMovements[0]) * size);

    // Motion map
    motionMap.ToByteArray(bytes);

    return CAMERA_PIPELINE_OK;
}

ErrorCode IntervalStatistics::FromByteArray(QByteArray* bytes)
{
    unsigned int    size;
    unsigned int    width;
    unsigned int    height;
    unsigned int    magic;
    unsigned int    version;
    QDataStream     in(bytes, QIODevice::ReadOnly);

    if (NULL == bytes)
    {
        return CAMERA_PIPELINE_ERROR;
    }

    // Validation numbers
    in.readRawData((char *)&magic, sizeof(magic));
    if (magic != 0xDEADBEEF)
    {
        return CAMERA_PIPELINE_ERROR;
    }

    in.readRawData((char *)&version, sizeof(version));
    if (version != 0x00000001)
    {
        return CAMERA_PIPELINE_ERROR;
    }

    // Accumulator buffer size and data
    in.readRawData((char *)&(width), sizeof(width));
    in.readRawData((char *)&(height), sizeof(height));
    accBuffer.SetSize(width, height);
    in.readRawData((char*)accBuffer.GetPlaneData(), width*height*sizeof(float));

    // Validation numbers
    in.readRawData((char *)&magic, sizeof(magic));
    if (magic != 0x00000000)
    {
        return CAMERA_PIPELINE_ERROR;
    }

    // Movements vector size and data
    in.readRawData((char *)&(size), sizeof(size));
    perFrameMovements.resize(size);
    in.readRawData((char *)perFrameMovements.data(), sizeof(perFrameMovements[0]) * size);

    // Motion map
    motionMap.FromDataStream(in);

    return CAMERA_PIPELINE_OK;
}

VideoStatistics::VideoStatistics() :
    m_pCurrentFrame(NULL)
{
    DataDirectory* pDataDirectory = DataDirectoryInstance::instance();
    m_pIntervalTimer = new IntervalTimer(pDataDirectory->pipelineParams.processingIntervalSec);
    QObject::connect(m_pIntervalTimer, SIGNAL(Started(QDateTime)), this, SLOT(IntervalStarted(QDateTime)));
    QObject::connect(m_pIntervalTimer, SIGNAL(Interval(QDateTime)), this, SLOT(IntervalFinished(QDateTime)));
}

VideoStatistics::~VideoStatistics()
{
    SAFE_DELETE(m_pIntervalTimer);
}



void VideoStatistics::IntervalStarted(QDateTime currentDateTime)
{
    DEBUG_MESSAGE0("VideoStatistics", "Interval started");
    m_currentStats.Reset();
    emit StatisticPeriodStarted(currentDateTime);           // Get new period statistics for decision maker
}

void VideoStatistics::IntervalFinished(QDateTime currentDateTime)
{
    DataDirectory*  pDataDirectory = DataDirectoryInstance::instance();
    QDateTime       currentRecordTime = m_pIntervalTimer->GetLastIntervalStart();

    DEBUG_MESSAGE0("VideoStatistics", "Interval finished");
    emit StatisticPeriodStarted(currentDateTime);   // Get new period statistics

    m_currentStats.date = currentRecordTime.date();
    m_currentStats.endDate = currentDateTime.date();
    m_currentStats.startTime = currentRecordTime.time();
    m_currentStats.endTime = currentDateTime.time();

    // Set motion map internal buffers size
    if (NULL != m_pCurrentFrame)
    {
        m_currentStats.backgroundBuffer.CopyFrom(m_pCurrentFrame, 0);
    }

    m_currentStatsCopy.CopyFrom(m_currentStats);

    // Writing new interval to DB
    if (!pDataDirectory->analysisParams.useVirtualDate)
    {
        DEBUG_MESSAGE0("VideoStatistics", "StatisticPeriodReady() emitting");
        emit StatisticPeriodReady(&m_currentStatsCopy);
    }
    // Reset accumulated statistic
    m_currentStats.Reset();
}

void VideoStatistics::ProcessAnalyzedFrame(VideoFrame *pCurrentFrame, AnalysisResults *results)
{
    DataDirectory*  pDataDirectory = DataDirectoryInstance::instance();

    DEBUG_MESSAGE0("VideoStatistics", "ProcessAnalyzedFrame() called");

    if (pDataDirectory->analysisParams.differenceBasedAnalysis ||
        pDataDirectory->analysisParams.motionBasedAnalysis)
    {
        // Store current frame pointer (for creating thumbnail)
        m_pCurrentFrame = pCurrentFrame;

        m_pIntervalTimer->Tick(pCurrentFrame->userTimestamp);

        // This call is effective on first frame only
        m_currentStats.accBuffer.SetSize(results->pDiffBuffer->GetWidth(), results->pDiffBuffer->GetHeight());

        //--- Accumulating interval statistics ---
        if (pDataDirectory->analysisParams.differenceBasedAnalysis)
        {
            m_currentStats.perFrameMovements.append(results->percentMotion);
            m_currentStats.accBuffer.AddBuffer(results->pDiffBuffer);
        }

        if (pDataDirectory->analysisParams.motionBasedAnalysis)
        {
            // Update Motion map
            if (NULL != results->pCurFlow)
            {
                m_currentStats.motionMap.Update(results->pCurFlow);
            }
        }
        DEBUG_MESSAGE0("VideoStatistics", "ProcessAnalyzedFrame() finished");
    }
    else
    {
        DEBUG_MESSAGE0("VideoStatistics", "Do not processing statistics when analysis is not enabled");
    }
}

// This function is a stub for adding records to DB when record-only mode is active
void VideoStatistics::ProcessSourcePacket(QSharedPointer<AVPacket> pInPacket)
{
    m_pIntervalTimer->Tick(pInPacket->pos);
}

void VideoStatistics::AddFalseEventDiffBuffer(VideoBuffer *buffer)
{
    DataDirectory*  pDataDirectory = DataDirectoryInstance::instance();
    DEBUG_MESSAGE0("VideoStatistics", "AddFalseEventDiffBuffer() called");

    if (pDataDirectory->analysisParams.differenceBasedAnalysis)
    {
        //--- Accumulating interval statistics ---
        if ((m_currentStats.accBuffer.GetWidth() == buffer->GetWidth()) &&
            (m_currentStats.accBuffer.GetHeight() == buffer->GetHeight()))
        {
            m_currentStats.accBuffer.AddBuffer(buffer, pDataDirectory->analysisParams.falseEventCoeff);
        }
    }
    DEBUG_MESSAGE0("VideoStatistics", "AddFalseEventDiffBuffer() finished");
}

StatisticDBInterface::StatisticDBInterface() : QObject(NULL), m_pDAO(NULL)
{
    // Initialize random number generator with int value from pointer
    // should be different for different application instances
    size_t  seed = (size_t)this;
    srand((unsigned int)seed);
}

StatisticDBInterface::~StatisticDBInterface()
{
    DEBUG_MESSAGE0("StatisticDBInterface", "~StatisticDBInterface() called");

    while (!m_currentPeriodStatistic.isEmpty()) // Delete all previously allocated entries in stat list
    {
        delete m_currentPeriodStatistic.takeFirst();
    }
    m_currentPeriodStatistic.clear();

    SAFE_DELETE(m_pDAO);

    DEBUG_MESSAGE0("StatisticDBInterface", "~StatisticDBInterface() finished");
}

void StatisticDBInterface::OpenDB()
{
    // Create new database access object
    m_pDAO = new AnalysisRecordDao();
}

void StatisticDBInterface::NewArchiveFileName(QString newFileName)
{
    DEBUG_MESSAGE0("VideoStatistics", "New archive file name received");
    m_archiveFileName = newFileName;
}

void StatisticDBInterface::StoreEvent(EventDescription event)
{
    if (NULL == m_pDAO)
    {
        ERROR_MESSAGE0(ERR_TYPE_ERROR, "StatisticDBInterface", "StoreEvent() failed: data access object is NULL");
        return;
    }
    m_pDAO->storeEvent(event);
}

void StatisticDBInterface::PerformGetStatistic(QDateTime currentDateTime)
{
    int delay = (rand() % 6) * 10;

    if (NULL == m_pDAO)
    {
        ERROR_MESSAGE0(ERR_TYPE_ERROR, "StatisticDBInterface", "GetStatictic() failed: data access object is NULL");
        return;
    }

    // Store time to be processed in delayed slot
    m_statisticsToGetTime = currentDateTime;

    ERROR_MESSAGE2(ERR_TYPE_MESSAGE, "StatisticDBInterface",
                   "GetStatistics() for %s will be called in %d seconds",
                   m_statisticsToGetTime.toString("dd.MM.yyyy HH:mm:ss").toUtf8().constData(), delay);

    QTimer::singleShot(delay*1000, this, SLOT(DelayedGet()));
}

void StatisticDBInterface::DelayedGet()
{
    DataDirectory*              pDataDirectory = DataDirectoryInstance::instance();
    QElapsedTimer               processingTimer;
    QList<AnalysisRecordModel*> recordsList;
    int                         periodDays = pDataDirectory->pipelineParams.statisticPeriodDays;
    int                         intervalSeconds = pDataDirectory->pipelineParams.statisticIntervalSec;

    processingTimer.restart();
    DEBUG_MESSAGE1("StatisticDBInterface", "DelayedGet() called ThreadID = %p", QThread::currentThreadId());

    if(CAMERA_PIPELINE_OK != m_pDAO->FindStatsForPeriod(
                m_statisticsToGetTime, periodDays, intervalSeconds, recordsList))
    {
        ERROR_MESSAGE0(ERR_TYPE_ERROR, "StatisticDBInterface", "Failed to get statistics for given period");
    }

    ERROR_MESSAGE4(ERR_TYPE_MESSAGE, "StatisticDBInterface",
                   "%d records selected for period from %s to %s (%d ms)",
                   recordsList.size(),
                   m_statisticsToGetTime.addSecs(-(periodDays*24*3600 + intervalSeconds)).toString("dd.MM.yyyy HH:mm:ss").toUtf8().constData(),
                   m_statisticsToGetTime.toString("dd.MM.yyyy HH:mm:ss").toUtf8().constData(),
                   (int)processingTimer.elapsed());

    //----------------------------------------------
    //-- Converting records to intreval statistic --
    //----------------------------------------------

    while (!m_currentPeriodStatistic.isEmpty()) // Delete all previously allocated entries in stat list
    {
        delete m_currentPeriodStatistic.takeFirst();
    }
    m_currentPeriodStatistic.clear();

    while (!recordsList.isEmpty())
    {
        IntervalStatistics*   pNewIntervalStats = new IntervalStatistics; // Memory must be deleted by user after usage
        AnalysisRecordModel*  pRecord = recordsList.takeFirst();

        pNewIntervalStats->date          = pRecord->date;
        pNewIntervalStats->endDate       = pRecord->endDate;
        pNewIntervalStats->startTime     = pRecord->startTime;
        pNewIntervalStats->endTime       = pRecord->endTime;

        if (CAMERA_PIPELINE_OK != pNewIntervalStats->FromByteArray(&pRecord->statsData))
        {
            ERROR_MESSAGE0(ERR_TYPE_ERROR, "StatisticDBInterface", "Unable to convert BLOB to interval statistics data");
            delete pRecord;
            return;
        }
        delete pRecord; // This memory was allocated in FindStatsForPeriod(), and should be deleted after usage

        m_currentPeriodStatistic.append(pNewIntervalStats);  // Add new IntervalStatistics entry
    }

    if (m_currentPeriodStatistic.size())
    {
        DEBUG_MESSAGE0("StatisticDBInterface", "NewPeriodStatistics() emitted");
        emit NewPeriodStatistics(m_currentPeriodStatistic); // Emit NewStatistics signal
    }

    // Send ping to health checker that read statistics is working
    emit Ping("ReadStatistics", 30*60*1000); // Timeout = 30 min
}

void StatisticDBInterface::PerformWriteStatistic(IntervalStatistics* stats)
{
    int delay = (rand() & 7) * 20 + 60;

    if (NULL == m_pDAO)
    {
        ERROR_MESSAGE0(ERR_TYPE_ERROR, "StatisticDBInterface", "WriteStatistic() failed: data access object is NULL");
        return;
    }

    // Store current archive name (because it will be changed whe DelayedWrite() will be called)
    stats->archiveFile = m_archiveFileName;

    // Store time to be processed in delayed slot
    m_statisticsToWritePtr = stats;

    ERROR_MESSAGE2(ERR_TYPE_MESSAGE, "StatisticDBInterface",
                   "WriteStatistics() for %s will be called in %d seconds",
                   stats->startTime.toString("HH:mm:ss").toUtf8().constData(),
                   delay);

    QTimer::singleShot(delay*1000, this, SLOT(DelayedWrite()));
}

void StatisticDBInterface::DelayedWrite()
{
    AnalysisRecordModel newRecord;
    IntervalStatistics* stats = m_statisticsToWritePtr;
    DataDirectory*      pDataDirectory = DataDirectoryInstance::instance();

    DEBUG_MESSAGE1("StatisticDBInterface", "DelayedWrite() called ThreadID = %p", QThread::currentThreadId());

    if (pDataDirectory->analysisParams.differenceBasedAnalysis || pDataDirectory->analysisParams.motionBasedAnalysis)
    {
        // Blur accumulator buffer
        stats->accBuffer.Blur(7);
    }

    newRecord.id            = 0; // will be autoincremented in DB
    newRecord.date          = stats->date;
    newRecord.endDate       = stats->endDate;
    newRecord.startTime     = stats->startTime;
    newRecord.endTime       = stats->endTime;
    newRecord.mediaSource   = pDataDirectory->pipelineParams.inputStreamUrl;
    newRecord.videoArchive  = stats->archiveFile;

    if (pDataDirectory->analysisParams.differenceBasedAnalysis || pDataDirectory->analysisParams.motionBasedAnalysis)
    {
        newRecord.heatmap = CreateHeatmap(&stats->backgroundBuffer, &stats->accBuffer);
        if (CAMERA_PIPELINE_OK != stats->ToByteArray(&newRecord.statsData))
        {
            ERROR_MESSAGE0(ERR_TYPE_ERROR, "StatisticDBInterface", "Unable to convert collected statistics to BLOB");
        }
    }
    else
    {
        newRecord.statsData = QByteArray();
        newRecord.heatmap = QImage();
    }

    m_pDAO->InsertRecord(newRecord);

    // Send ping to health checker that write statistics is working
    emit Ping("WriteStatistic", 30*60*1000); // Timeout = 30 min
}

struct TColor
{
    double r;
    double g;
    double b;
};

TColor GetColour(double v, double vmin, double vmax)
{
    TColor c;
    c.r = 1.0;
    c.g = 1.0;
    c.b = 1.0;
    double dv = vmax - vmin;

    v = std::max(v, vmin);
    v = std::min(v, vmax);

    if (v < (vmin + 0.25 * dv))
    {
        c.r = 0;
        c.g = 4 * (v - vmin) / dv;
    }
    else if (v < (vmin + 0.5 * dv))
    {
        c.r = 0;
        c.b = 1 + 4 * (vmin + 0.25 * dv - v) / dv;
    }
    else if (v < (vmin + 0.75 * dv))
    {
        c.r = 4 * (v - vmin - 0.5 * dv) / dv;
        c.b = 0;
    }
    else
    {
        c.g = 1 + 4 * (vmin + 0.75 * dv - v) / dv;
        c.b = 0;
    }
    return c;
}

QImage StatisticDBInterface::CreateHeatmap(VideoBuffer* pBkgrBuffer, AccumlatorBuffer* pAccBuffer)
{
    int     width = pBkgrBuffer->GetWidth();
    int     height = pBkgrBuffer->GetHeight();
    int     stride = pBkgrBuffer->GetStride();

    int     accWidth = pAccBuffer->GetWidth() - 1;      // For clamping
    int     accHeight = pAccBuffer->GetHeight() - 1;
    int     accStride = pAccBuffer->GetStride();

    if (width > 0 && height > 0 && accWidth > 0 && accHeight > 0)
    {
        float   scale = (float)accWidth / (float)width;

        float*         pAcc = pAccBuffer->GetPlaneData();
        unsigned char* pBkg = pBkgrBuffer->GetPlaneData();

        TColor  c;
        double  val1;
        double  val2;

        QImage  res(width, height, QImage::Format_RGB888);

        for (int j = 0; j < height; j++)
        {
            for (int i = 0; i < width; i++)
            {
                val1 = pAcc[std::min(accHeight, (int)(j*scale))*accStride + std::min(accWidth, (int)(i*scale))];
                val2 = pBkg[j*stride + i];

                val1  = std::min(val1, 255.0) / 255.0;
                val2 /= 255.0;

                if (val1 > 0.06)  // ~16 in absoulute value
                {
                    c = GetColour(val1, 0.0, 1.0); // Get coloured heatmap

                    // Blend with background
                    c.r = 0.7*c.r + 0.3*val2;
                    c.g = 0.7*c.g + 0.3*val2;
                    c.b = 0.7*c.b + 0.3*val2;
                }
                else
                {
                    c.r = val2;
                    c.g = val2;
                    c.b = val2;
                }
                // Set result pixel

                int r = c.r * 255.0;
                int g = c.g * 255.0;
                int b = c.b * 255.0;

                res.setPixel(i, j, qRgb(r, g, b));
            }
        }
        return res;
    }
    return QImage(16, 16, QImage::Format_RGB888);
}
