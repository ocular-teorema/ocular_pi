
#include "videoAnalyzer.h"

void VideoAnalyzer::ProcessFrame(VideoFrame *pCurFrame)
{
    DataDirectory*  pDataDirectory = DataDirectoryInstance::instance();
    AnalysisParams  params = pDataDirectory->getParams();
    Statistics      stats;
    VideoFrame      scaledCurFrame;

    stats.startInterval();

    // Get scaled width and height
    int scaledWidth  = (int)(pCurFrame->GetWidth() * params.downscaleCoeff() + 0.5f);
    int scaledHeight = (int)(pCurFrame->GetHeight() * params.downscaleCoeff() + 0.5f);

    // Width and height should be aligned at least by 2
    scaledWidth  &= 0xFFFFFFFE;
    scaledHeight &= 0xFFFFFFFE;

    // Downscaling
    scaledCurFrame.SetSize(scaledWidth, scaledHeight);
    m_scaler.ScaleFrame(pCurFrame, &scaledCurFrame);

    // Check if we have another input size or new downscaling coefficient (or it is a first frame)
    if ((m_scaledPrevFrame.GetWidth() != scaledWidth) || (m_scaledPrevFrame.GetHeight() != scaledHeight))
    {
        m_scaledPrevFrame.CopyFromVideoFrame(&scaledCurFrame);

        // Recreate Motion Estimator
        SAFE_DELETE(m_pMotionEstimator);
        m_pMotionEstimator = new MotionEstimator(scaledWidth, scaledHeight, ME_BLOCK_SIZE);
    }

    //-----------------------------------------------------
    //-------------- Analysis starts here -----------------
    //-----------------------------------------------------

    if (params.experimental())
    {
        m_pDenoiseFilter->ProcessFrame(&scaledCurFrame, &scaledCurFrame, 32.0f, 3);
        //m_pDenoiseFilter->ProcessFrame(pCurFrame, pCurFrame, 60.0f, 5);
    }

    // Difference
    if (params.differenceBasedAnalysis())
    {
        float       avgDiff = 0.0f;

        m_diffBuffer.CopyFrom(&scaledCurFrame, 0);

        if (5.0f < m_diffBuffer.AbsDiffLuma(&m_scaledPrevFrame))  // Calulate abs difference
        {
            m_diffBuffer.SetVal(0);
        }

        // Blur it
        m_diffBuffer.Blur(5, 2.0);

        // Threshold
        m_diffBuffer.AddVal(-params.diffThreshold());           // Soft threshold operation
        avgDiff = m_diffBuffer.Binarize(1, 1);

        currentResults.pDiffBuffer = &m_diffBuffer;

        avgDiff /= (float)(scaledWidth * scaledHeight);
        currentResults.percentMotion = avgDiff*100.0f;
    }

    // Complex motion and objects based analysis (do not performed, if input queue is too long)
    if (params.motionBasedAnalysis())
    {
        // RGB data required for background detection algorithm
        scaledCurFrame.UpdateRGB();

        // Motion flow
        {
            m_pMotionEstimator->ProcessFrame(&scaledCurFrame, true);
        }

        // Object detection
        {
            m_pObjectDetector->mParams = params.detectorParams();
            m_pObjectDetector->pBGSubstractor->SetThreshold(params.bgThreshold(), params.fgThreshold());

            m_pObjectDetector->Exec(&scaledCurFrame, &m_scaledPrevFrame, m_pMotionEstimator->GetFlow());

            pDataDirectory->addStats(m_pObjectDetector->stats);
        }

        // Difference masking
        m_diffBuffer.Mask(m_pObjectDetector->pFGMask);

        // Update result structure
        currentResults.objects = m_pObjectDetector->currObjectsList;
        currentResults.wasCalibrationError = !m_pObjectDetector->pBGSubstractor->isFGValid;
        currentResults.pCurFlow = m_pMotionEstimator->GetFlow();

        //if (params.produceDebug())
        //{
        //    VideoFrame* pBGFrame;
        //    int   binIndex = params.debugImageIndex();
        //    float binHeight;
        //
        //    if (0 == ((pCurFrame->userTimestamp/1000) % pDataDirectory->getPipelineParams().processingIntervalSec()))
        //    {
        //        m_motionMap.Reset();
        //    }
        //
        //    m_motionMap.Update(m_pMotionEstimator->GetFlow());
        //    m_tmpBuffer.CopyFrom(&m_scaledPrevFrame, 0);
        //    m_motionMap.DrawMotionMap(&m_tmpBuffer, binIndex, 1.0f);
        //    //m_motionMap.CompareWith(currentResults.pCurFlow, &m_tmpBuffer);
        //
        //    binHeight = m_pObjectDetector->pBGSubstractor->GetBG(&pBGFrame, binIndex);
        //    pDataDirectory->addGraphPoint("Average bin height", binHeight, true);
        //    pDataDirectory->newImage("Background", QSharedPointer<QImage>(pBGFrame->GetQImagePtrFromRGB(4.0f)));
        //    pDataDirectory->newImage("Foreground", QSharedPointer<QImage>(m_pObjectDetector->pFGMask->GetQImagePtr(4.0f)));
        //    pDataDirectory->newImage("Motion", QSharedPointer<QImage>(m_tmpBuffer.GetQImagePtr(2.0f)));
        //}
    }
    // Save current downscaled frame as previous for the next time
    m_scaledPrevFrame.CopyFromVideoFrame(&scaledCurFrame);

    stats.endInterval("Analysis total");
    pDataDirectory->addStats(stats);
}
