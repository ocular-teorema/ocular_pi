#include <QDebug>

#include "objectDetector.h"
#include "abstractPainter.h"
#include "morphological.h"
#include "segmentator.h"

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/features2d/features2d.hpp>

using namespace corecvs;

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

class ObjectSegment : public BaseSegment<ObjectSegment>
{
public:
    int size;
    DetectedObject *payload;
    Vector2dd sum;
    Vector2d32 min;
    Vector2d32 max;

    ObjectSegment() :
        size(0),
        payload(NULL),
        sum(Vector2dd(0.0)),
        min(Vector2d32(numeric_limits<int32_t>::max())),
        max(Vector2d32(0))
    {}

    void dadify()
    {
        BaseSegment<ObjectSegment>::dadify();
        if (father != NULL && father != this)
        {
            father->size += this->size;
            father->sum += this->sum;
            father->min = father->min.minimum(min);
            father->max = father->max.maximum(max);
        }
    }

    Vector2dd getAverage()
    {
        return this->sum / (double)(this->size);
    }

    void addPoint(int i, int j, const uint16_t & /*tile*/)
    {
        this->size++;
        this->sum += Vector2dd(j, i);

        if (min.x() > j) min.x() = j;
        if (min.y() > i) min.y() = i;

        if (max.x() < j) max.x() = j;
        if (max.y() < i) max.y() = i;
    }

};

class ObjectSegmentator : public Segmentator<ObjectSegmentator, ObjectSegment>
{
public:
    int threshold;

    ObjectSegmentator(int _threshold) :
        threshold(_threshold)
    { }

    static int xZoneSize()
    {
        return 1;
    }

    static int yZoneSize()
    {
        return 1;
    }

    bool canStartSegment(int /*i*/, int /*j*/, const uint16_t &element)
    {
        return element > threshold;
    }

    static bool sortPredicate(ObjectSegment *a1, ObjectSegment *a2)
    {
        return a1->size >  a2->size;
    }
};

bool compareObjectsBySize(const DetectedObject& obj1, const DetectedObject& obj2)
{
    return (obj1.sizeX*obj1.sizeY) > (obj2.sizeX*obj2.sizeY);
}

void ObjectDetector::Exec(VideoFrame *pCurFrame, VideoFrame *pPrevFrame, MotionFlow *pFlow)
{
    stats = Statistics();
    Q_UNUSED(pPrevFrame);

    // Check that input size and internal size are matching
    CheckSize(pCurFrame->GetWidth(), pCurFrame->GetHeight());

    m_pCurFlow = pFlow;

    // Calculate foreground mask
    stats.startInterval();

    // Detect FG ang collect BG
    pBGSubstractor->ProcessNewFrame(pCurFrame, pFGMask);

    stats.resetInterval("Foreground detection");

    // Segmentation
    G8Buffer tmpFGBuf(pFGMask->GetHeight(), pFGMask->GetWidth());
    pFGMask->CopyTo(&tmpFGBuf);
    ObjectSegmentator segmentator(127);
    ObjectSegmentator::SegmentationResult *segmented = segmentator.segment<G8Buffer>(&tmpFGBuf);

    // Filter by size
    sort(segmented->segments->begin(), segmented->segments->end(), ObjectSegmentator::sortPredicate);

    // Only clusters larger then mTileCount are considered
    prevObjectsList.clear();
    prevObjectsList = currObjectsList;
    currObjectsList.clear();

    int minSize = mParams.minimumCluster();
    unsigned segCount = 0;
    for (segCount = 0; segCount < segmented->segmentNumber(); segCount++)
    {
        DetectedObject  obj;
        ObjectSegment*  runnerSegment = segmented->segment(segCount);
        Vector2d32      size = runnerSegment->max - runnerSegment->min;

        if (runnerSegment->size < minSize)
        {
            break;
        }

        obj.coordX = runnerSegment->getAverage().x();
        obj.coordY = runnerSegment->getAverage().y();

        obj.sizeX = size.x();
        obj.sizeY = size.y();

        // Coordinates should be left and top
        obj.coordX = MAX(0, obj.coordX - (obj.sizeX >> 1));
        obj.coordY = MAX(0, obj.coordY - (obj.sizeY >> 1));

        obj.id = segCount;
        obj.isValid = true;

        currObjectsList.push_back(obj);
        runnerSegment->payload = &currObjectsList.back();
    }
    delete_safe(segmented);
    stats.resetInterval("Segmenation");

    // Sort objects by size
    qSort(currObjectsList.begin(), currObjectsList.end(), compareObjectsBySize);

    // Reassign object numbers
    for (int i = 0; i < currObjectsList.size(); i++)
    {
        currObjectsList[i].id = i;
    }
}
