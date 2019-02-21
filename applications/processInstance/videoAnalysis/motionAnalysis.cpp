
#include "motionAnalysis.h"

#include <opencv2/core/core.hpp>
#include <opencv2/video/video.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

inline int Round(float mv)
{
    return (mv < 0.0f) ? (int)(mv - 0.5f) : (int)(mv + 0.5f);
}

MotionFlow::MotionFlow(int w, int h, int bsize) :
    width(w / bsize),
    height(h / bsize),
    blockSize(bsize)
{
    pVectors = new mv_t[width*height];
}

MotionFlow::~MotionFlow()
{
    SAFE_DELETE_ARRAY(pVectors);
}

mv_t MotionFlow::GetVector(int x, int y, float scale)
{
    int xb = (int)((x / scale) / blockSize);
    int yb = (int)((y / scale) / blockSize);
    return pVectors[yb*width + xb];
}

mv_t MotionFlow::GetPopularNonzeroVectorNB(int x, int y, int radius)
{
    mv_t    res;
    int     mvxHist[65];
    int     mvyHist[65];
    int     mvxMax = 0;
    int     mvyMax = 0;
    int     xIdx = 0;
    int     yIdx = 0;

    memset(mvxHist, 0, 65*sizeof(int));
    memset(mvyHist, 0, 65*sizeof(int));

    res.x = 0.0f;
    res.y = 0.0f;
    res.sad = -1.0f;
    res.confidence = -1.0f;

    for (int j = -radius; j <= radius; j++)
    {
        for (int i = -radius; i <= radius; i++)
        {
            mv_t cur = pVectors[MAX(0, MIN(y + j, height - 1))*width + MAX(0, MIN(x + i, width - 1))];

            if (cur.x !=0 || cur.y != 0)
            {
                int mvx = (MIN(64, MAX(-64, Round(cur.x))) + 64) >> 1;
                int mvy = (MIN(64, MAX(-64, Round(cur.y))) + 64) >> 1;

                mvxHist[mvx]++;
                mvyHist[mvy]++;

                if (mvxHist[mvx] > mvxMax)
                {
                    mvxMax = mvxHist[mvx];
                    xIdx = mvx;
                }

                if (mvyHist[mvy] > mvyMax)
                {
                    mvyMax = mvyHist[mvy];
                    yIdx = mvy;
                }
            }
        }
    }

    // Only vectors appeared more than 2 times can be treated as "popular"
    if (mvxMax > 2 || mvyMax > 2)
    {
        res.x = (xIdx << 1) - 64;
        res.y = (yIdx << 1) - 64;
    }
    return res;
}



MotionMap::MotionMap() :
    m_width(0),
    m_height(0),
    m_blockSize(0),
    m_blocksCount(0),
    m_frameNumber(0),
    m_validBinHeight(MIN_MV_BIN_HEIGHT),
    m_pModel(NULL)
{

}

MotionMap::~MotionMap()
{
    SAFE_DELETE_ARRAY(m_pModel);
}

void MotionMap::Init(int width, int height, int blockSize)
{
    m_width = width;
    m_height = height;
    m_blockSize = blockSize;
    m_blocksCount = width*height;

    SAFE_DELETE_ARRAY(m_pModel);
    m_pModel = new MotionModel_t[m_blocksCount];

    // Initialize all possible motion directions
    m_pDirectionTable[0] = atan2(0, 0);
    m_pDirectionTable[1] = atan2(0.5, 0.5);
    m_pDirectionTable[2] = atan2(1, 0);
    m_pDirectionTable[3] = atan2(0.5, -0.5);
    m_pDirectionTable[4] = atan2(0, -1.0);
    m_pDirectionTable[5] = atan2(-0.5, -0.5);
    m_pDirectionTable[6] = atan2(-1, 0);
    m_pDirectionTable[7] = atan2(-0.5, 0.5);

    // Initialize other model values
    for (int p = 0; p < m_blocksCount; p++)
    {
        for (int i = 0; i < MAX_MV_SAMPLES; i++)
        {
            m_pModel[p].bins[i].height = 0.0f;
            m_pModel[p].bins[i].mvx = 0.0f;
            m_pModel[p].bins[i].mvy = 0.0f;
            m_pModel[p].bins[i].directionIdx = i;
        }
    }
    m_frameNumber = 0;
}

void MotionMap::Reset()
{
    if (m_pModel == NULL)
    {
        return;
    }

    for (int p = 0; p < m_blocksCount; p++)
    {
        for (int i = 0; i < MAX_MV_SAMPLES; i++)
        {
            m_pModel[p].bins[i].height = 0.0f;
            m_pModel[p].bins[i].mvx = 0.0f;
            m_pModel[p].bins[i].mvy = 0.0f;
            m_pModel[p].bins[i].directionIdx = i;
        }
    }
    m_frameNumber = 0;
}

void MotionMap::AverageMV(MotionBin_t* pBin, mv_t& curMV)
{
    const int SuspiciousMVThreshold = 20.0f;

    // Check for suspicious MV values - they should not vary too much from current average value
    if ((fabs(curMV.x) > SuspiciousMVThreshold) || (fabs(curMV.y) > SuspiciousMVThreshold))
    {
        if ((fabs(curMV.x) > 3.0f*fabs(pBin->mvx)) || (fabs(curMV.y) > 3.0f*fabs(pBin->mvy)))
        {
            return; // No update - motion is too high
        }
        else
        {
            pBin->mvx = (pBin->mvx + curMV.x) / 2.0f;
            pBin->mvy = (pBin->mvy + curMV.y) / 2.0f;
            pBin->height += 1.0f;
        }
    }
    else
    {
        pBin->mvx = (pBin->mvx + curMV.x) / 2.0f;
        pBin->mvy = (pBin->mvy + curMV.y) / 2.0f;
        pBin->height += 1.0f;
    }
}

int MotionMap::CheckVelocity(MotionBin_t* pBin, mv_t curMV, float* confidence)
{
    float conf = 0.0f;

    if ((pBin->height < m_validBinHeight) || ((fabs(curMV.x) < 2*MOTION_THR) && (fabs(curMV.y) < 2*MOTION_THR)))
    {
        return 0;
    }

    conf = sqrt(curMV.x*curMV.x + curMV.y*curMV.y) / sqrt(pBin->mvx*pBin->mvx + pBin->mvy*pBin->mvy);
    conf = conf*conf*10; //  2x times=40%    3x times=90%
    conf = MIN(100.0f, conf);

    *confidence += conf;

    // Doing this check to avoid a lot of "formal" velocity alerts
    return (conf > 20.0) ? 1 : 0;
}

MotionBin_t* MotionMap::GetBinByDirection(int p, int direction)
{
    for (int s = 0; s < MAX_MV_SAMPLES; s++)
    {
        if (direction == m_pModel[p].bins[s].directionIdx)
        {
            return &(m_pModel[p].bins[s]);
        }
    }
    return NULL; // Should never get here
}

void MotionMap::Update(MotionFlow *pCurFlow)
{
    int p;
    int s;
    int idx;
    int width = pCurFlow->width;
    int height = pCurFlow->height;

    if ((width*height != m_blocksCount) || (m_blockSize != pCurFlow->blockSize))
    {
        Init(width, height, pCurFlow->blockSize);
    }

    for (p = 0; p < m_blocksCount; p++)
    {
        mv_t         curMV = pCurFlow->pVectors[p];
        MotionBin_t* pBins = m_pModel[p].bins;
        int          curDirection = GetDirectionIdx(curMV.x, curMV.y);

        if (curDirection >= 0)
        {
            for (s = 0; s < MAX_MV_SAMPLES; s++)
            {
                //try to associate the current vector direction values to an existing bin
                if (curDirection == pBins[s].directionIdx)
                {
                    AverageMV(&pBins[s], curMV);

                    // Keep bins sorted by height
                    idx = s;
                    while (idx > 0 && (pBins[idx].height > pBins[idx-1].height))
                    {
                        SwapMotionBins(p, idx, idx - 1);
                        idx--;
                    }
                    break;
                }
            }
        }
    }
    m_frameNumber++;
}

void MotionMap::CopyFrom(MotionMap* pMap)
{
    MotionModel_t*  pInModelPtr = pMap->GetModelPtr();

    if ((pMap->GetBlocksCount() != m_blocksCount) || (pMap->GetBlockSize() != m_blockSize))
    {
        Init(pMap->GetWidth(), pMap->GetHeight(), pMap->GetBlockSize());
    }

    memcpy(m_pModel, pInModelPtr, m_blocksCount*sizeof(MotionModel_t));
}

void MotionMap::AddMap(MotionMap *pMap, float weight)
{
    int p;
    int s1;
    int s2;

    if ((pMap->GetBlocksCount() != m_blocksCount) || (pMap->GetBlockSize() != m_blockSize))
    {
        ERROR_MESSAGE4(ERR_TYPE_WARNING, "MotionMap",
                       "Different buffer size in AddMap() (%dx%d) (%dx%d). Init() will be called",
                       m_width, m_height,
                       pMap->GetWidth(),
                       pMap->GetHeight());

        Init(pMap->GetWidth(), pMap->GetHeight(), pMap->GetBlockSize());
    }

    MotionModel_t* pInModelPtr = pMap->GetModelPtr();

    for (p = 0; p < m_blocksCount; p++)
    {
        MotionBin_t* pBins1 = m_pModel[p].bins;
        MotionBin_t* pBins2 = pInModelPtr[p].bins;

        for (s1 = 0; s1 < MAX_MV_SAMPLES; s1++)
        {
            for (s2 = 0; s2 < MAX_MV_SAMPLES; s2++)
            {
                //find corresponding direction in target motion map
                if (pBins1[s1].directionIdx == pBins2[s2].directionIdx)
                {
                    if (fabs(pBins2[s2].mvx) > 0.5f || fabs(pBins2[s2].mvy) > 0.5f)
                    {
                        pBins1[s1].mvx = (fabs(pBins1[s1].mvx) > 0.0f) ? (pBins1[s1].mvx + pBins2[s2].mvx) / 2.0f : pBins2[s2].mvx;
                        pBins1[s1].mvy = (fabs(pBins1[s1].mvy) > 0.0f) ? (pBins1[s1].mvy + pBins2[s2].mvy) / 2.0f : pBins2[s2].mvy;
                        pBins1[s1].height = pBins1[s1].height + pBins2[s2].height * weight;
                    }
                    break;
                }
            }
        }

        // Sort bins by height
        for (s1 = 0; s1 < MAX_MV_SAMPLES; s1++)
        {
            for (s2 = s1; s2 < MAX_MV_SAMPLES; s2++)
            {
                if( pBins1[s2].height > pBins1[s1].height)
                {
                    SwapMotionBins(p, s1, s2);
                }
            }
        }
    }
}

void MotionMap::SwapMotionBins(int p, int idx1, int idx2)
{
    MotionBin_t  tmp;

    tmp.height          = m_pModel[p].bins[idx1].height;
    tmp.directionIdx    = m_pModel[p].bins[idx1].directionIdx;
    tmp.mvx             = m_pModel[p].bins[idx1].mvx;
    tmp.mvy             = m_pModel[p].bins[idx1].mvy;

    m_pModel[p].bins[idx1].height       = m_pModel[p].bins[idx2].height;
    m_pModel[p].bins[idx1].directionIdx = m_pModel[p].bins[idx2].directionIdx;
    m_pModel[p].bins[idx1].mvx          = m_pModel[p].bins[idx2].mvx;
    m_pModel[p].bins[idx1].mvy          = m_pModel[p].bins[idx2].mvy;

    m_pModel[p].bins[idx2].height       = tmp.height;
    m_pModel[p].bins[idx2].directionIdx = tmp.directionIdx;
    m_pModel[p].bins[idx2].mvx          = tmp.mvx;
    m_pModel[p].bins[idx2].mvy          = tmp.mvy;
}

int MotionMap::CheckObject(MotionFlow *pCurFlow, int* decision, float *confidence, int x, int y, int w, int h)
{
    const float BadBlocksThreshold = 0.6f;

    int p;
    int numMovedBlocks = 0;
    int numWrongBlocks = 0;
    int numBadVelocityBlocks = 0;

    float  vectorConfidence = 0.0f;
    float  velocityConfidence = 0.0f;

    int xb = (int)((float)x / (float)m_blockSize + 0.5f);
    int yb = (int)((float)y / (float)m_blockSize + 0.5f);
    int wb = (int)((float)(w + m_blockSize - 1) / (float)m_blockSize + 0.5f);
    int hb = (int)((float)(h + m_blockSize - 1) / (float)m_blockSize + 0.5f);

    for (int j = 0; j < hb; j++)
    {
        for (int i = 0; i < wb; i++)
        {
            p = MIN(yb + j, m_height - 1)*m_width + MIN(xb + i, m_width - 1);

            int curDirection = GetDirectionIdx(pCurFlow->pVectors[p].x, pCurFlow->pVectors[p].y);

            // We can check only first bin to define whether current map cell is active or not
            // Because all bins are sorted by their height
            bool isBinActive = m_pModel[p].bins[0].height > m_validBinHeight;

            // Comparing only blocks with valid motion vectors
            if (curDirection >= 0)
            {
                numMovedBlocks++;

                MotionBin_t* pModelBin = GetBinByDirection(p, curDirection);

                // We can take current block as wrong direction only if
                // corresponding map cell is active (i.e. have enough statistics)
                if (pModelBin->height < m_validBinHeight && isBinActive)
                {
                    vectorConfidence += 100.0f * (m_validBinHeight - pModelBin->height) / m_validBinHeight;
                    numWrongBlocks++;
                }
                else
                {
                    numBadVelocityBlocks += CheckVelocity(pModelBin,
                                                          pCurFlow->pVectors[p],
                                                          &velocityConfidence); // Confidence accumulated inside
                }
            }
        }
    }

    // If we have too low moved blocks count - we consider this object as non valid for motion comparison
    if (numMovedBlocks > 3)
    {
        if ((float)numWrongBlocks / (float)(numMovedBlocks) > BadBlocksThreshold)
        {
            *decision = ALERT_TYPE_INVALID_MOTION;
            *confidence = vectorConfidence / (float)numWrongBlocks;
            return 0;
        }
        else
        {
            if ((float)numBadVelocityBlocks / (float)(numMovedBlocks - numWrongBlocks) > BadBlocksThreshold)
            {
                *decision = ALERT_TYPE_VELOCITY;
                *confidence = velocityConfidence / (float)numBadVelocityBlocks;
                return 0;
            }
        }
    }

    // default - no alert
    *decision = 0;
    *confidence = 0.0f;
    return -1;
}

int MotionMap::GetDirectionIdx(float mvx, float mvy)
{
    const double  DirectionDelta = M_PI / (double)MAX_MV_SAMPLES;
    double        curDirection = atan2(mvy, mvx);

    if ((abs(mvx) > MOTION_THR) || (abs(mvy) > MOTION_THR))
    {
        for (int s = 0; s < MAX_MV_SAMPLES; s++)
        {
            if (fabs(m_pDirectionTable[s] - curDirection) < DirectionDelta)
            {
                return s;
            }
        }
        // This is corner case when we have direction between positive and negative angles
        return 4;
    }
    return -1; // Invalid vector
}

void MotionMap::DrawMotionMap(VideoBuffer* pRes, int index, float scale)
{
    int         p;
    cv::Scalar  color = 255;
    cv::Mat     img(pRes->GetHeight(), pRes->GetWidth(), CV_8UC1, pRes->GetPlaneData(), pRes->GetStride());

    if (index > MAX_MV_SAMPLES)
    {
        return;
    }

    for (p = 0; p < m_blocksCount; p++)
    {
        int mvx = Round(m_pModel[p].bins[index].mvx);
        int mvy = Round(m_pModel[p].bins[index].mvy);
        int curX = (p % m_width)*m_blockSize;
        int curY = (p / m_width)*m_blockSize;

        curX += m_blockSize>>1;
        curY += m_blockSize>>1;

        curX = (int)((float)curX / scale);
        curY = (int)((float)curY / scale);

        mvx = (int)((float)mvx / scale);
        mvy = (int)((float)mvy / scale);

        color = MIN(255, 20 + 5 * (m_pModel[p].bins[index].height));
        cv::circle(img, cv::Point(curX, curY), 1, color);
        cv::line(img, cv::Point(curX, curY), cv::Point(curX + mvx, curY + mvy), color, 1);
    }
}

void MotionMap::ToByteArray(QByteArray* bytes)
{
    bytes->append((char *)&m_width, sizeof(m_width));
    bytes->append((char *)&m_height, sizeof(m_height));
    bytes->append((char *)&m_blockSize, sizeof(m_blockSize));
    bytes->append((char *)m_pModel, m_blocksCount*sizeof(MotionModel_t));
}

void MotionMap::FromDataStream(QDataStream& in)
{
    // Background buffer size and data
    in.readRawData((char *)&(m_width), sizeof(m_width));
    in.readRawData((char *)&(m_height), sizeof(m_height));
    in.readRawData((char *)&(m_blockSize), sizeof(m_blockSize));
    Init(m_width, m_height, m_blockSize);
    in.readRawData((char*)m_pModel, m_blocksCount*sizeof(MotionModel_t));
}






MotionEstimator::MotionEstimator(int width, int height, int blockSize) :
    m_pPrevFrame(NULL),
    m_pCurrFlow(NULL),
    m_pPrevFlow(NULL)
{
    Init(width, height, blockSize);
}

MotionEstimator::~MotionEstimator()
{
    SAFE_DELETE(m_pPrevFrame);
    SAFE_DELETE(m_pCurrFlow);
    SAFE_DELETE(m_pPrevFlow);
}

void MotionEstimator::Init(int width, int height, int blockSize)
{
    m_width = (width / blockSize);
    m_height = (height / blockSize);
    m_blockSize = blockSize;
    m_blocksCount = m_width*m_height;

    m_pCurrFlow = new MotionFlow(width, height, blockSize);
    m_pPrevFlow = new MotionFlow(width, height, blockSize);

    SAFE_DELETE(m_pPrevFrame);
}

void MotionEstimator::DrawFlow(MotionFlow* pFlow, cv::Mat* pFlowMap)
{
#ifdef WITH_DEBUG_UI
    int x,y;
    const cv::Scalar color = 255;

    cv::Mat scaled;
    cv::Mat cflowmap;
    cv::Size scaledSz;

    if (pFlowMap)
    {
        pFlowMap->copyTo(cflowmap);
    }
    else
    {
        cflowmap = cv::Mat::zeros(pFlow->width*pFlow->blockSize, pFlow->height*pFlow->blockSize, CV_8UC1);
    }

    for(y = 0; y < pFlow->height; y++)
    {
        for(x = 0; x < pFlow->width; x++)
        {
            mv_t    mv = pFlow->pVectors[y*pFlow->width + x];
            int     x0 = x*pFlow->blockSize + (pFlow->blockSize >> 1);
            int     y0 = y*pFlow->blockSize + (pFlow->blockSize >> 1);

            cv::line(cflowmap, cv::Point(x0, y0), cv::Point(cvRound(x0 + mv.x), cvRound(y0 + mv.y)), color);
        }
    }

    scaledSz = cv::Size(cflowmap.cols * 2, cflowmap.rows * 2);
    cv::resize(cflowmap, scaled, scaledSz);
    imshow("Optical flow", scaled);
#else
    Q_UNUSED(pFlow);
    Q_UNUSED(pFlowMap);
#endif
}

void MotionEstimator::ProcessFrame(VideoFrame *pCurFrame, int doFilter)
{
    int i;
    int j;
    int k;
    int l;

    if (NULL == m_pPrevFrame)
    {
        m_pPrevFrame = new VideoFrame(pCurFrame);
    }

    cv::Mat     pCur(pCurFrame->GetHeight(), pCurFrame->GetWidth(), CV_8UC1, pCurFrame->GetYData(), pCurFrame->GetStride());
    cv::Mat     pPrev(m_pPrevFrame->GetHeight(), m_pPrevFrame->GetWidth(), CV_8UC1, m_pPrevFrame->GetYData(), m_pPrevFrame->GetStride());
    cv::Mat     pFlow;

    calcOpticalFlowFarneback(pCur, pPrev, pFlow, 0.5, 3, 5, 1, 5, 1.7, 0);

    for (j = 0; j < m_height; j++)
    {
        for (i = 0; i < m_width; i++)
        {
            float amvx = 0.0f;
            float amvy = 0.0f;
            int curX = i*m_pCurrFlow->blockSize;
            int curY = j*m_pCurrFlow->blockSize;

            for (k = 0; k < m_pCurrFlow->blockSize; k++)
            {
                for (l = 0; l < m_pCurrFlow->blockSize; l++)
                {
                    const cv::Point2f fxy = pFlow.at<cv::Point2f>(curY + k, curX + l);
                    amvx += fxy.x;
                    amvy += fxy.y;
                }
            }

            m_pCurrFlow->pVectors[j*m_width + i].x = amvx / (float)(m_pCurrFlow->blockSize*m_pCurrFlow->blockSize);
            m_pCurrFlow->pVectors[j*m_width + i].y = amvy / (float)(m_pCurrFlow->blockSize*m_pCurrFlow->blockSize);
            m_pCurrFlow->pVectors[j*m_width + i].sad = -1;
        }
    }

    if (doFilter)
    {
        ZeroCheck(pCurFrame, m_pPrevFrame, m_pCurrFlow);
        FilterMVF(pCurFrame, m_pPrevFrame, m_pCurrFlow);
    }

    // Swap prev and cur frames
    m_pPrevFrame->CopyFromVideoFrame(pCurFrame);
#ifdef WITH_DEBUG_UI
    DrawFlow(m_pCurrFlow, &pCur);
#endif
}

int SAD(unsigned char* pCur, unsigned char* pPrev, int strideCur, int stridePrev, int sz)
{
    int i;
    int j;
    int sad = 0;

    for (j = 0; j < sz; j++)
    {
        for (i = 0; i < sz; i++)
        {
            sad += std::abs((int)pPrev[j*stridePrev + i] - (int)pCur[j*strideCur + i]);
        }
    }
    return sad;
}

void MotionEstimator::ZeroCheck(VideoFrame *pCurFrame, VideoFrame *pPrevFrame, MotionFlow *pFlow)
{
    const float     ZeroMVCoeff = 1.2;

    int             i;
    int             j;
    int             width = pCurFrame->GetWidth();
    int             height = pCurFrame->GetHeight();
    int             curStride = pCurFrame->GetStride();
    int             prevStride = pPrevFrame->GetStride();
    unsigned char*  pCur = pCurFrame->GetYData();
    unsigned char*  pPrev = pPrevFrame->GetYData();

    for (j = 0; j < pFlow->height; j++)
    {
        for (i = 0; i < pFlow->width; i++)
        {
            mv_t    curMV = pFlow->pVectors[j*m_width + i];

            // Zero all boundary vectors
            if (i == 0 || j == 0)
            {
                pFlow->pVectors[j*m_width + i].x = 0.0f;
                pFlow->pVectors[j*m_width + i].y = 0.0f;
            }
            else
            {
                // Check all other non-zero vectors
                if (fabs(curMV.x) > 0.5f || fabs(curMV.y) > 0.5f)
                {
                    float   curSAD;
                    float   zeroSAD;
                    int     curX = i*m_blockSize;
                    int     curY = j*m_blockSize;

                    unsigned char*  pCurBlock = pCur + curY*curStride + curX;
                    unsigned char*  pPrevBlock = pPrev + \
                                                 MAX(0, MIN(height - m_blockSize, curY + Round(curMV.y)))*prevStride + \
                                                 MAX(0, MIN(width  - m_blockSize, curX + Round(curMV.x)));

                    curSAD = SAD(pCurBlock, pPrevBlock, curStride, prevStride, m_blockSize);
                    zeroSAD = SAD(pCurBlock, pPrev + curY*curStride + curX, curStride, prevStride, m_blockSize);

                    pFlow->pVectors[j*pFlow->width + i].sad = curSAD;   // Store current SAD to avoid recalculation

                    if ((zeroSAD / curSAD) < ZeroMVCoeff)
                    {
                        pFlow->pVectors[j*pFlow->width + i].x = 0.0f;
                        pFlow->pVectors[j*pFlow->width + i].y = 0.0f;
                        pFlow->pVectors[j*pFlow->width + i].sad = zeroSAD;
                    }
                }
            }
        }
    }
}

void MotionEstimator::FilterMVF(VideoFrame *pCurFrame, VideoFrame *pPrevFrame, MotionFlow *pFlow)
{
    const float     PopularMVCoeff = 1.2;

    int             i;
    int             j;
    int             width = pCurFrame->GetWidth();
    int             height = pCurFrame->GetHeight();
    int             curStride = pCurFrame->GetStride();
    int             prevStride = pPrevFrame->GetStride();
    unsigned char*  pCur = pCurFrame->GetYData();
    unsigned char*  pPrev = pPrevFrame->GetYData();

    for (j = 1; j < pFlow->height - 1; j++)
    {
        for (i = 1; i < pFlow->width - 1; i++)
        {
            mv_t    curMV = pFlow->pVectors[j*m_width + i];
            mv_t    popularMV = pFlow->GetPopularNonzeroVectorNB(i, j, 2);

            // Check non-zero vectors
            if (popularMV.x != 0 || popularMV.y != 0)
            {
                float   curSAD;
                float   popularSAD;
                int     curX = i*m_blockSize;
                int     curY = j*m_blockSize;

                unsigned char*  pCurBlock = pCur + curY*curStride + curX;
                unsigned char*  pPrevBlock = pPrev + \
                                             MAX(0, MIN(height - m_blockSize, curY + Round(curMV.y)))*prevStride + \
                                             MAX(0, MIN(width  - m_blockSize, curX + Round(curMV.x)));

                curSAD = SAD(pCurBlock, pPrevBlock, curStride, prevStride, m_blockSize);

                pPrevBlock = pPrev + \
                             MAX(0, MIN(height - m_blockSize, curY + Round(popularMV.y)))*prevStride + \
                             MAX(0, MIN(width  - m_blockSize, curX + Round(popularMV.x)));

                popularSAD = SAD(pCurBlock, pPrevBlock, curStride, prevStride, m_blockSize);

                if ((popularSAD / curSAD) < PopularMVCoeff)
                {
                    pFlow->pVectors[j*pFlow->width + i] = popularMV;
                }
            }
        }
    }
}
