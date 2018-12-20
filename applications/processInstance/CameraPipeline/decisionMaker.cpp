
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

    if (pDataDirectory->getParams().useVirtualDate())
    {
        curDate.setDate(pDataDirectory->getParams().dateParams().year(),
                        pDataDirectory->getParams().dateParams().month(),
                        pDataDirectory->getParams().dateParams().day());

        curTime.setHMS(pDataDirectory->getParams().dateParams().hour(), pDataDirectory->getParams().dateParams().minute(), 0);
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
        //// Debug output
        //{
        //    IntervalStatistics* pIntervalStats = statsList.at(i);
        //    FILE * f = fopen("Weights.txt", "a");
        //    fprintf(f, "Date = %s, Time = %s, weight = %f (%f)\n",
        //            pIntervalStats->date.toString().toUtf8().constData(),
        //            pIntervalStats->startTime.toString().toUtf8().constData(),
        //            weights[i], weights[i] / totalWeight);
        //    fclose(f);
        //}
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

    if (!pDataDirectory->getParams().differenceBasedAnalysis())
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
    }
    // Output accumulated heatmap to Debug
    //if (pDataDirectory->getParams().produceDebug())
    //{
    //    m_heatmapBuffer.SetSize(m_totalAccBuffer.GetWidth(), m_totalAccBuffer.GetHeight());
    //    m_totalAccBuffer.ConvertTo(&m_heatmapBuffer);
    //    pDataDirectory->newImage("AccumulatedHeatmap", QSharedPointer<QImage>(m_heatmapBuffer.GetQImagePtr(2.0f)));
    //}
}

void AreaDecisionMaker::ProcessResults(AnalysisResults* pResults)
{
    int   i;
    int   j;
    int   k;

    DataDirectory* pDataDirectory = DataDirectoryInstance::instance();

    float           diffCount = 0.0f;
    float           confidence = 0.0f;
    int             accStride = m_totalAccBuffer.GetStride();
    int             diffStride = pResults->pDiffBuffer->GetStride();
    int             tmpStride;
    int             width = m_totalAccBuffer.GetWidth();
    int             height = m_totalAccBuffer.GetHeight();
    unsigned char*  pTmpDiff;
    unsigned char*  pDiff = pResults->pDiffBuffer->GetPlaneData();
    float*          pAcc = m_totalAccBuffer.GetPlaneData();

    int   diffThr = pDataDirectory->getParams().diffThreshold();

    DEBUG_MESSAGE0("AreaDecisionMaker", "ProcessResults()");

    m_diffPointsBuffer.SetSize(width, height);
    m_diffPointsBuffer.SetVal(0);

    if (!pDataDirectory->getParams().differenceBasedAnalysis())
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

    pTmpDiff = m_diffPointsBuffer.GetPlaneData();
    tmpStride = m_diffPointsBuffer.GetStride();

    for (j = 0; j < height; j++)
    {
        for (i = 0; i < width; i++)
        {
            // We should check, if there is a place in current difference buffer,
            // that not present in total accumulated differene for interval
            if ((pDiff[j*diffStride + i] > 0) && (pAcc[j*accStride + i] < diffThr))
            {
                float c = 100.0f * (float)(diffThr - pAcc[j*accStride + i]) / (float)(diffThr);
                pTmpDiff[j*tmpStride + i] = (int)c;
                confidence += c;
                diffCount = diffCount + 1.0f;
            }
        }
    }

    // Send pointer to difference buffer
    decision.pInfoBuffer = &m_diffPointsBuffer;

    // Get average confidence value
    decision.confidence = confidence / diffCount;

    // Check for the alert condition
    diffCount = (diffCount * 100.0f) / (float)(width * height); // threshold in %

    if (diffCount > pDataDirectory->getParams().totalThreshold())
    {
        decision.alertType = ALERT_TYPE_AREA;

        // Find object(s), that produced this motion
        for (k = 0; k < decision.objects.size(); k++)
        {
            float           numDiffPoints = 0.0f;
            DetectedObject  object = pResults->objects[k];

            // Scan all points inside object area
            for(j = 0; j < object.sizeY; j++)
            {
                for(i = 0; i < object.sizeX; i++)
                {
                    // Check if this is detected point inside object area
                    if (pTmpDiff[(j + object.coordY)*tmpStride + (i + object.coordX)] > 0)
                    {
                        numDiffPoints += 1.0f;
                    }
                }
            }
            decision.objects[k].isValid = (numDiffPoints / (float)(object.sizeX*object.sizeY)) < 0.2f;
        }
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

    if (!pDataDirectory->getParams().motionBasedAnalysis())
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
}

void MotionDecisionMaker::ProcessResults(AnalysisResults* pResults)
{
    DataDirectory* pDataDirectory = DataDirectoryInstance::instance();

    DEBUG_MESSAGE0("MotionDecisionMaker", "ProcessResults()");

    if (!pDataDirectory->getParams().motionBasedAnalysis())
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

