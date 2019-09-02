#pragma once

#include <QObject>

#include "videoAnalyzer.h"
#include "pipelineCommonTypes.h"

class DecisionMakerBase : public QObject
{
    Q_OBJECT
public:
    DecisionMakerBase();
    ~DecisionMakerBase();

    DecisionResults     decision;               /// Algorithm results

    void    ProcessStats(QList<IntervalStatistics*> curStatsList);
    bool    ProcessAnalysisResults(AnalysisResults* pResults);

private:
    int     m_validStatPresent;   /// Indicates, that we have good period statistics and can process new frames

    /// Weight of given statistic interval
    void GetIntervalWeights(QList<IntervalStatistics *> statsList, float* weights);

    /// Statistic processing function
    virtual void  ProcessStatistics(QList<IntervalStatistics*> statsList, float* weights) = 0;

    /// Main processing function
    virtual void  ProcessResults(AnalysisResults* pResults) = 0;
};

///
/// Decision about area violation
///
class AreaDecisionMaker: public DecisionMakerBase
{
public:
    AreaDecisionMaker();
    ~AreaDecisionMaker();

private:
    AccumlatorBuffer            m_totalAccBuffer;     /// Buffer to accumulate weighted difference for several intervals

    /// Statistic processing function
    void  ProcessStatistics(QList<IntervalStatistics*> statsList, float* weights);

    /// Main processing function
    void  ProcessResults(AnalysisResults* pResults);
};


///
/// Decision about motion vectors violation
///
class MotionDecisionMaker: public DecisionMakerBase
{
public:
    MotionDecisionMaker();
    ~MotionDecisionMaker();

private:
    MotionMap                   m_motionMap;        /// Accumulated motion map

    // Debug drawing buffers
    VideoBuffer                 m_motionMapBuffer;  /// Buffer for drawing accumulated motionmap

    /// Statistic processing function
    void  ProcessStatistics(QList<IntervalStatistics*> statsList, float* weights);

    /// Main processing function
    void  ProcessResults(AnalysisResults* pResults);
};


///
/// Decision about calibration error
///
class CalibDecisionMaker: public DecisionMakerBase
{
public:
    CalibDecisionMaker();
    ~CalibDecisionMaker();

private:
    /// Statistic processing function
    void  ProcessStatistics(QList<IntervalStatistics*> statsList, float* weights) { Q_UNUSED(statsList); Q_UNUSED(weights); }

    /// Main processing function
    void  ProcessResults(AnalysisResults* pResults);
};
