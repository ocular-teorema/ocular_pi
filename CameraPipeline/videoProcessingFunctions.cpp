
#include "videoAnalyzer.h"

void VideoAnalyzer::ProcessFrame(VideoFrame *pCurFrame)
{
    DataDirectory*  pDataDirectory = DataDirectoryInstance::instance();
    VideoFrame      scaledCurFrame;

    // Get scaled width and height
    int scaledWidth  = (int)(pCurFrame->GetWidth() * pDataDirectory->analysisParams.downscaleCoeff + 0.5f);
    int scaledHeight = (int)(pCurFrame->GetHeight() * pDataDirectory->analysisParams.downscaleCoeff + 0.5f);

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

    // Difference
    if (pDataDirectory->analysisParams.differenceBasedAnalysis)
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
        m_diffBuffer.AddVal(-pDataDirectory->analysisParams.diffThreshold);           // Soft threshold operation
        avgDiff = m_diffBuffer.Binarize(1, 1);

        currentResults.pDiffBuffer = &m_diffBuffer;

        avgDiff /= (float)(scaledWidth * scaledHeight);
        currentResults.percentMotion = avgDiff*100.0f;
    }

    // Complex motion and objects based analysis (do not performed, if input queue is too long)
    if (pDataDirectory->analysisParams.motionBasedAnalysis)
    {
        // RGB data required for background detection algorithm
        scaledCurFrame.UpdateRGB();

        // Motion flow
        {
            m_pMotionEstimator->ProcessFrame(&scaledCurFrame, true);
        }

        // Object detection
        {
            m_pObjectDetector->pBGSubstractor->SetThreshold(
                        pDataDirectory->analysisParams.bgThreshold,
                        pDataDirectory->analysisParams.fgThreshold);

            m_pObjectDetector->Exec(&scaledCurFrame, &m_scaledPrevFrame, m_pMotionEstimator->GetFlow());
        }

        // Difference masking
        m_diffBuffer.Mask(m_pObjectDetector->pFGMask);

        // Update result structure
        currentResults.objects = m_pObjectDetector->currObjectsList;
        currentResults.wasCalibrationError = !m_pObjectDetector->pBGSubstractor->isFGValid;
        currentResults.pCurFlow = m_pMotionEstimator->GetFlow();
    }
    // Save current downscaled frame as previous for the next time
    m_scaledPrevFrame.CopyFromVideoFrame(&scaledCurFrame);
}
