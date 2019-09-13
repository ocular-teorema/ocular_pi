
#include <stdio.h>

#include "decisionMaker.h"

DecisionMakerBase::DecisionMakerBase() :
    QObject(NULL),
    m_validStatPresent(0)
{

}

DecisionMakerBase::~DecisionMakerBase()
{

}

bool DecisionMakerBase::ProcessAnalysisResults(AnalysisResults* pResults)
{
    // Reset Decision to its defaults
    decision.alertType = 0;
    decision.confidence = 0.0f;
    decision.timestamp = pResults->timestamp;
    decision.pInfoBuffer = pResults->pDiffBuffer;
    decision.objects = pResults->objects;

    // Make decision on current frame (if we have valid period statistics)
    if (m_validStatPresent)
    {
        ProcessResults(pResults);
    }
    return m_validStatPresent;
}

//
// Statistic intervals processing logic
//
static bool IsHoliday(QDate date1)
{
    if (date1.dayOfWeek() < 6)
    {
        if (date1.day() < 10 && date1.month() == 1) // 1 - 9 january
        {
            return true;
        }
        if (date1.day() == 23 && date1.month() == 2) // 23 february
        {
            return true;
        }
        if (date1.day() == 8 && date1.month() == 3) // 8 march
        {
            return true;
        }
        if (date1.day() == 1 && date1.month() == 5) // 1 may
        {
            return true;
        }
        if (date1.day() == 9 && date1.month() == 5) // 9 may
        {
            return true;
        }
        if (date1.day() == 12 && date1.month() == 6) // 12 june
        {
            return true;
        }
        if (date1.day() == 7 && date1.month() == 11) // 7 november
        {
            return true;
        }
        if (date1.day() == 31 && date1.month() == 12) // 31 december
        {
            return true;
        }
        return false;
    }
    return true;
}

static bool IsSameWorkingDay(QDate date1, QDate date2)
{
    if (IsHoliday(date1) && IsHoliday(date2))
    {
        return true;
    }

    if (!IsHoliday(date1) && !IsHoliday(date2))
    {
        return true;
    }

    return false;
}

void DecisionMakerBase::GetIntervalWeights(QList<IntervalStatistics *> statsList, float *weights)
{
    int     i;
    float   totalWeight = 0.0f;

    DataDirectory* pDataDirectory = DataDirectoryInstance::instance();

    QDate curDate = QDateTime::currentDateTime().date();
    QTime curTime = QDateTime::currentDateTime().time();

    DEBUG_MESSAGE0("DecisionMakerBase", "GetIntervalWeights() called");

    if (pDataDirectory->analysisParams.useVirtualDate)
    {
        curDate.setDate(pDataDirectory->analysisParams.year,
                        pDataDirectory->analysisParams.month,
                        pDataDirectory->analysisParams.day);

        curTime.setHMS(pDataDirectory->analysisParams.hour, pDataDirectory->analysisParams.minute, 0);
    }

    // Calculate intervals weights
    memset(weights, 0, statsList.size()*sizeof(float));

    for (i = 0; i < statsList.size(); i++)
    {
        IntervalStatistics* pIntervalStats = statsList.at(i);

        if (curDate.dayOfWeek() == pIntervalStats->date.dayOfWeek())
        {
            if (pIntervalStats->startTime.secsTo(curTime) < 60)
            {
                // Same day and same time
                weights[i] = 3.0f;
            }
            else
            {
                // Same day, but +- 10 minutes
                weights[i] = 2.0f;
            }
        }
        else
        {
            if( IsSameWorkingDay(curDate, pIntervalStats->date))
            {
                // Not same day, but working day (or holiday) as well
                weights[i] = 1.0f;
            }
            else
            {
                // Not same day, and not same working day (nor holiday)
                weights[i] = 0.0f;
            }
        }
        totalWeight += weights[i];
    }

    // Normalize intervals weights
    for (i = 0; i < statsList.size(); i++)
    {
        weights[i] /= totalWeight;
    }
}

void DecisionMakerBase::ProcessStats(QList<IntervalStatistics *> curStatsList)
{
    float weights[64] = {0.0f};

    DEBUG_MESSAGE1("DecisionMakerBase", "New period statistics (%d records) received from DB", curStatsList.size());

    m_validStatPresent = 0;

    if (curStatsList.size() > 0)
    {
        GetIntervalWeights(curStatsList, weights);

        ProcessStatistics(curStatsList, weights);

        // We got the stats
        m_validStatPresent = 1;
    }
}


///
/// Area violation check
///
AreaDecisionMaker::AreaDecisionMaker() : DecisionMakerBase()
{

}

AreaDecisionMaker::~AreaDecisionMaker()
{

}

void AreaDecisionMaker::ProcessStatistics(QList<IntervalStatistics *> statsList, float* weights)
{
    int i;
    DataDirectory* pDataDirectory = DataDirectoryInstance::instance();

    DEBUG_MESSAGE0("AreaDecisionMaker", "ProcessStatistics()");

    if (!pDataDirectory->analysisParams.differenceBasedAnalysis)
    {
        return;
    }

    m_totalAccBuffer.Reset();

    if (statsList.size())
    {
        m_totalAccBuffer.SetSize(statsList.at(0)->accBuffer.GetWidth(),
                                 statsList.at(0)->accBuffer.GetHeight());

        // Accumulate interval's heatmap
        for (i = 0; i < statsList.size(); i++)
        {
            IntervalStatistics* pIntervalStats = statsList.at(i);
            m_totalAccBuffer.Add(&pIntervalStats->accBuffer, weights[i]);
        }

        // Draw collected heatmap for debug
        if (pDataDirectory->analysisParams.produceDebug)
        {
            CreateDebugImages();
        }
    }
}

void AreaDecisionMaker::ProcessResults(AnalysisResults* pResults)
{
    int   i;
    int   j;

    DataDirectory* pDataDirectory = DataDirectoryInstance::instance();

    float           diffCount = 0.0f;
    float           confidence = 0.0f;
    int             width = m_totalAccBuffer.GetWidth();
    int             height = m_totalAccBuffer.GetHeight();
    int             accStride = m_totalAccBuffer.GetStride();
    int             diffStride = pResults->pDiffBuffer->GetStride();
    unsigned char*  pDiff = pResults->pDiffBuffer->GetPlaneData();
    float*          pAcc = m_totalAccBuffer.GetPlaneData();

    int   diffThr = pDataDirectory->analysisParams.diffThreshold;

    DEBUG_MESSAGE0("AreaDecisionMaker", "ProcessResults()");

    if (!pDataDirectory->analysisParams.differenceBasedAnalysis)
    {
        return;
    }

    // If there was camera calibration error - do not perform further analysis
    if (pResults->wasCalibrationError)
    {
        return;
    }

    // All buffers must have the same width and height
    if ((pResults->pDiffBuffer->GetWidth() != width) || (pResults->pDiffBuffer->GetHeight() != height))
    {
        ERROR_MESSAGE4(ERR_TYPE_ERROR, "DecisionMaker",
                       "Size of accumulator and difference buffers do not match acc=%dx%d diff=%dx%d",
                       width,
                       height,
                       pResults->pDiffBuffer->GetWidth(),
                       pResults->pDiffBuffer->GetHeight());
        return;
    }

    for (j = 0; j < height; j++)
    {
        for (i = 0; i < width; i++)
        {
            // We should check, if there is a place in current difference buffer,
            // that not present in total accumulated differene for interval
            if ((pDiff[j*diffStride + i] > 0) && (pAcc[j*accStride + i] < diffThr))
            {
                confidence += 100.0f * (float)(diffThr - pAcc[j*accStride + i]) / (float)(diffThr);
                diffCount = diffCount + 1.0f;
            }
        }
    }

    // Get average confidence value
    decision.confidence = confidence / diffCount;

    // Check for the alert condition
    diffCount = (diffCount * 100.0f) / (float)(width * height); // threshold in %

    if (diffCount > pDataDirectory->analysisParams.totalThreshold)
    {
        decision.alertType = ALERT_TYPE_AREA;
    }
}

void AreaDecisionMaker::CreateDebugImages()
{
    DataDirectory* pDataDirectory = DataDirectoryInstance::instance();
    QString debugFolder = pDataDirectory->pipelineParams.archivePath;
            debugFolder += '/';
            debugFolder += pDataDirectory->pipelineParams.pipelineName;
            debugFolder += QString("/debugImages/");

    QString thumbFile = (DataDirectoryInstance::instance())->pipelineParams.archivePath;
    thumbFile += '/';
    thumbFile += (DataDirectoryInstance::instance())->pipelineParams.pipelineName;
    thumbFile += "/thumb.jpg";

    // Read thumbnail image for background
    VideoBuffer bkgr;

    if (CAMERA_PIPELINE_OK == bkgr.LoadFromFile(thumbFile))
    {
        // Make file name corresponding to number of its 10-minute period
        int64_t currentMsec = QDateTime::currentDateTime().time().msecsSinceStartOfDay();
        int number = int(currentMsec / (1000 * pDataDirectory->pipelineParams.processingIntervalSec));

        QString outputFile = QString("%1/heatmap_%2.png").arg(debugFolder).arg(number);

        QImage res = StatisticDBInterface::CreateHeatmap(&bkgr, &m_totalAccBuffer);

        res.save(outputFile);
    }
}

///
/// Motion vectors violation check
///
MotionDecisionMaker::MotionDecisionMaker() : DecisionMakerBase()
{

}

MotionDecisionMaker::~MotionDecisionMaker()
{

}

void MotionDecisionMaker::ProcessStatistics(QList<IntervalStatistics *> statsList, float *weights)
{
    DataDirectory* pDataDirectory = DataDirectoryInstance::instance();

    DEBUG_MESSAGE0("MotionDecisionMaker", "ProcessStatistics()");

    if (!pDataDirectory->analysisParams.motionBasedAnalysis)
    {
        return;
    }

    m_motionMap.Reset();

    // Accumulate interval's heatmap
    for (int i = 0; i < statsList.size(); i++)
    {
        IntervalStatistics* pIntervalStats = statsList.at(i);
        m_motionMap.AddMap(&pIntervalStats->motionMap, weights[i]);
    }

    // Draw collected motion map for debug
    if (pDataDirectory->analysisParams.produceDebug)
    {
        CreateDebugImages();
    }
}

void MotionDecisionMaker::ProcessResults(AnalysisResults* pResults)
{
    DataDirectory* pDataDirectory = DataDirectoryInstance::instance();

    DEBUG_MESSAGE0("MotionDecisionMaker", "ProcessResults()");

    if (!pDataDirectory->analysisParams.motionBasedAnalysis)
    {
        return;
    }

    // Output accumulated motion map to Debug
    // Do this on each frame to be able to change motion bins index dynamically
    //if (pDataDirectory->getParams().produceDebug() && pDataDirectory->getParams().motionBasedAnalysis())
    //{
    //    m_motionMapBuffer.SetSize(m_motionMap.GetWidth()*m_motionMap.GetBlockSize(), m_motionMap.GetHeight()*m_motionMap.GetBlockSize());
    //    m_motionMapBuffer.SetVal(0);
    //    m_motionMap.DrawMotionMap(&m_motionMapBuffer, pDataDirectory->getParams().debugImageIndex(), 1.0f);
    //    pDataDirectory->newImage("AccumulatedMotionMap", QSharedPointer<QImage>(m_motionMapBuffer.GetQImagePtr(2.0f)));
    //}

    // If there was camera calibration error - do not perform further analysis
    if (!pResults->wasCalibrationError)
    {
        // Check for invalid motion
        if (NULL != pResults->pCurFlow)
        {
            // Scan all founded objects
            for (int k = 0; k < decision.objects.size(); k++)
            {
                DetectedObject  object = decision.objects[k];

                m_motionMap.CheckObject(pResults->pCurFlow,
                                        &decision.alertType,
                                        &decision.confidence,
                                        object.coordX, object.coordY, object.sizeX, object.sizeY);

                if (decision.alertType > 0)
                {
                    decision.objects[k].isValid = false;
                    //ERROR_MESSAGE(ERR_TYPE_MESSAGE, "DecisionMaker",
                    //             (QString("Alert %1 in object# %2 (conf = %3)").arg(decision.alertType).arg(k).arg(decision.confidence)).toUtf8().constData());
                }
            }
        }
    }
}

void MotionDecisionMaker::CreateDebugImages()
{
    DataDirectory* pDataDirectory = DataDirectoryInstance::instance();
    QString debugFolder = pDataDirectory->pipelineParams.archivePath;
            debugFolder += '/';
            debugFolder += pDataDirectory->pipelineParams.pipelineName;
            debugFolder += QString("/debugImages/");

    QString thumbFile = (DataDirectoryInstance::instance())->pipelineParams.archivePath;
    thumbFile += '/';
    thumbFile += (DataDirectoryInstance::instance())->pipelineParams.pipelineName;
    thumbFile += "/thumb.jpg";

    // Read thumbnail image for background
    VideoBuffer bkgr0;

    if (CAMERA_PIPELINE_OK == bkgr0.LoadFromFile(thumbFile))
    {
        VideoBuffer bkgr1(&bkgr0);
        VideoBuffer bkgr2(&bkgr0);

        // Make file name corresponding to number of its 10-minute period
        int64_t currentMsec = QDateTime::currentDateTime().time().msecsSinceStartOfDay();
        int number = int(currentMsec / (1000 * pDataDirectory->pipelineParams.processingIntervalSec));

        QString outputFile0 = QString("%1/motionMap0_%2.png").arg(debugFolder).arg(number);
        QString outputFile1 = QString("%1/motionMap1_%2.png").arg(debugFolder).arg(number);
        QString outputFile2 = QString("%1/motionMap2_%2.png").arg(debugFolder).arg(number);

        m_motionMap.DrawMotionMap(&bkgr0, 0, pDataDirectory->analysisParams.downscaleCoeff);
        m_motionMap.DrawMotionMap(&bkgr1, 1, pDataDirectory->analysisParams.downscaleCoeff);
        m_motionMap.DrawMotionMap(&bkgr2, 2, pDataDirectory->analysisParams.downscaleCoeff);

        QImage res0 = bkgr0.CreateQImage();
        QImage res1 = bkgr1.CreateQImage();
        QImage res2 = bkgr2.CreateQImage();

        res0.save(outputFile0);
        res1.save(outputFile1);
        res2.save(outputFile2);
    }
}




///
/// Calibration error check
///
CalibDecisionMaker::CalibDecisionMaker() : DecisionMakerBase()
{

}

CalibDecisionMaker::~CalibDecisionMaker()
{

}

void CalibDecisionMaker::ProcessResults(AnalysisResults* pResults)
{
    DEBUG_MESSAGE0("CalibDecisionMaker", "ProcessResults()");

    // If there was camera calibration error - do not perform further analysis
    if (pResults->wasCalibrationError)
    {
        decision.alertType = ALERT_TYPE_CALIBRATION_ERROR;
        decision.confidence = 61.0f;
    }
}

