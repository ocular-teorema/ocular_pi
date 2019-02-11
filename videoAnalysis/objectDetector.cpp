#include <QDebug>

#include "objectDetector.h"

#include <opencv2/imgproc/imgproc.hpp>

ObjectDetector::ObjectDetector() :
    pFGMask(NULL),
    m_pCurYPlane(NULL),
    m_pPrevYPlane(NULL)
{
    pBGSubstractor = new MultimodalBG();
    pFGMask = new VideoBuffer();
    m_pCurYPlane = new VideoBuffer();
    m_pPrevYPlane = new VideoBuffer();
}

ObjectDetector::~ObjectDetector()
{
    SAFE_DELETE(pFGMask);
    SAFE_DELETE(m_pCurYPlane);
    SAFE_DELETE(m_pPrevYPlane);
    SAFE_DELETE(pBGSubstractor);
}

void ObjectDetector::CheckSize(int width, int height)
{
    pFGMask->SetSize(width, height);
    m_pCurYPlane->SetSize(width, height);
    m_pPrevYPlane->SetSize(width, height);
}

bool compareObjectsBySize(const DetectedObject& obj1, const DetectedObject& obj2)
{
    return (obj1.sizeX*obj1.sizeY) > (obj2.sizeX*obj2.sizeY);
}

void ObjectDetector::Exec(VideoFrame *pCurFrame, VideoFrame *pPrevFrame, MotionFlow *pFlow)
{
    Q_UNUSED(pPrevFrame);

    // Check that input size and internal size are matching
    CheckSize(pCurFrame->GetWidth(), pCurFrame->GetHeight());

    m_pCurFlow = pFlow;

    // Detect FG ang collect BG
    pBGSubstractor->ProcessNewFrame(pCurFrame, pFGMask);

    prevObjectsList.clear();
    prevObjectsList = currObjectsList;
    currObjectsList.clear();

    cv::Mat pFGMat(pFGMask->GetHeight(), pFGMask->GetWidth(), CV_8UC1, pFGMask->GetPlaneData(), pFGMask->GetStride());
    cv::Mat labelImage(pFGMat.size(), CV_32S);

    int nLabels = connectedComponents(pFGMat, labelImage, 8);

    // Filtering objects less than 2 blocks 8x8 (which are used for motion analysis)
    for (int n = 1; n <= nLabels; n++)
    {
        cv::Mat tmp(labelImage.size(), CV_8U);

        cv::inRange(labelImage, cv::Scalar(n), cv::Scalar(n), tmp);

        cv::Rect bb = cv::boundingRect(tmp);

        if (bb.area() > 127)
        {
            currObjectsList.push_back(DetectedObject(bb.tl().x, bb.tl().y, bb.width, bb.height, n, true));
        }
    }

    // Sort objects by size
    qSort(currObjectsList.begin(), currObjectsList.end(), compareObjectsBySize);

    // Reassign object numbers
    for (int i = 0; i < currObjectsList.size(); i++)
    {
        currObjectsList[i].id = i;
    }
}
