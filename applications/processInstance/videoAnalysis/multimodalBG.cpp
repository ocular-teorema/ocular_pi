
#include <opencv2/core/core.hpp>
#include <opencv2/video/video.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "multimodalBG.h"

MultimodalBG::MultimodalBG() :
    isFGValid(0),
    m_width(0),
    m_height(0),
    m_pixelCount(0),
    m_frameNumber(0),
    m_bgThreshold(BG_THRESHOLD),
    m_fgThreshold(FG_THRESHOLD),
    m_pBGModel(NULL),
    m_pPrevFG(NULL),
    m_pBG(NULL)
{

}

MultimodalBG::~MultimodalBG()
{
    SAFE_DELETE(m_pBG);
    SAFE_DELETE(m_pPrevFG);
    SAFE_DELETE_ARRAY(m_pBGModel);
}

void MultimodalBG::ProcessNewFrame(VideoFrame *pFrame, VideoBuffer *pFGMask)
{
    if (pFrame->GetWidth()*pFrame->GetHeight() != m_pixelCount)
    {
        Init(pFrame->GetWidth(), pFrame->GetHeight());
    }

    if ((m_frameNumber % BG_UPDATE_PERIOD) == 0)
    {
        UpdateBG(pFrame);
    }

    GetFG(pFrame, pFGMask);

    if (m_bgThreshold & 1)
    {
        HSVSuppressionOCV(pFrame, pFGMask);
        FilterFG(pFGMask);
    }

    m_frameNumber++;
}

float MultimodalBG::GetBG(VideoFrame **ppBG, int binIndex)
{
    float   avgHeight = 0.0f;

    int p = 0;
    int stride = m_pBG->GetStrideRGB();
    unsigned char* pFrameData = m_pBG->GetRGB24Data();

    for (int j = 0; j < m_height; j++)
    {
        for (int i = 0; i < m_width; i++)
        {
            if (m_pBGModel[p].bins[binIndex].height > MIN_BG_BIN_HEIGHT)
            {
                pFrameData[j*stride + i*3 + 0] = m_pBGModel[p].bins[binIndex].values[0];
                pFrameData[j*stride + i*3 + 1] = m_pBGModel[p].bins[binIndex].values[1];
                pFrameData[j*stride + i*3 + 2] = m_pBGModel[p].bins[binIndex].values[2];
            }
            else
            {
                pFrameData[j*stride + i*3 + 0] = 0;
                pFrameData[j*stride + i*3 + 1] = 0;
                pFrameData[j*stride + i*3 + 2] = 0;
            }
            avgHeight += m_pBGModel[p].bins[binIndex].height;
            p++;
        }
    }
    *ppBG = m_pBG;

    return avgHeight / m_pixelCount;
}

void MultimodalBG::Init(int width, int height)
{
    m_width = width;
    m_height = height;
    m_pixelCount = m_width*m_height;

    SAFE_DELETE_ARRAY(m_pBGModel);
    m_pBGModel = new BgModel_t[m_pixelCount];

    SAFE_DELETE(m_pPrevFG);
    m_pPrevFG = new VideoBuffer(width, height);

    SAFE_DELETE(m_pBG);
    m_pBG = new VideoFrame(width, height);

    for (int p = 0; p < m_pixelCount; p++)
    {
        for (int i=0; i < MAX_BG_SAMPLES; i++)
        {
            m_pBGModel[p].bins[i].lastUpdate = 0;
            m_pBGModel[p].bins[i].height = 0;
            m_pBGModel[p].bins[i].values[0] = 0;
            m_pBGModel[p].bins[i].values[1] = 0;
            m_pBGModel[p].bins[i].values[2] = 0;
        }
    }
    m_frameNumber = 0;
}

void MultimodalBG::UpdateBG(VideoFrame *pFrame)
{
    int     s;
    int     idx;
    unsigned char   curPix[3];
    int             stride = pFrame->GetStrideRGB();
    unsigned char*  pFrameData = pFrame->GetRGB24Data();

    //create a statistical model for each pixel (a set of bins of variable size)
    for(int p = 0; p < m_pixelCount; ++p)
    {
        int x = p % m_width;
        int y = p / m_width;

        BGBin_t* pBins = &(m_pBGModel[p].bins[0]);

        curPix[0] = pFrameData[y*stride + x*3 + 0];
        curPix[1] = pFrameData[y*stride + x*3 + 1];
        curPix[2] = pFrameData[y*stride + x*3 + 2];

        for(s = 0; s < MAX_BG_SAMPLES; s++)
        {
            //try to associate the current pixel values to an existing bin
            if( (unsigned int)std::abs(curPix[0] - pBins[s].values[0]) <= SAME_BIN_THRESHOLD &&
                (unsigned int)std::abs(curPix[1] - pBins[s].values[1]) <= SAME_BIN_THRESHOLD &&
                (unsigned int)std::abs(curPix[2] - pBins[s].values[2]) <= SAME_BIN_THRESHOLD )
            {

                pBins[s].values[0] = (pBins[s].values[0] * (1.0f - AVG_COEFF) + curPix[0] * AVG_COEFF);
                pBins[s].values[1] = (pBins[s].values[1] * (1.0f - AVG_COEFF) + curPix[1] * AVG_COEFF);
                pBins[s].values[2] = (pBins[s].values[2] * (1.0f - AVG_COEFF) + curPix[2] * AVG_COEFF);

                pBins[s].height++;
                pBins[s].lastUpdate = m_frameNumber;

                // Keep our bins sorted by height
                idx = s;
                while (idx > 0 && (pBins[idx].height > pBins[idx-1].height))
                {
                    SwapBGBins(p, idx, idx - 1);
                    idx--;
                }
                break;
            }
        }

        // If similar bin was not found
        // We need to insert newly arrived pixel value
        // Instead of some old and unfrequent bin
        if (s >= MAX_BG_SAMPLES)
        {
            uint minHeight = pBins[MAX_BG_SAMPLES - 1].height;
            uint maxAge = 0;

            // 2. Find the oldest from less frequent bins
            s = MAX_BG_SAMPLES - 2;
            idx = s;

            while (pBins[s].height == minHeight)
            {
                uint     curAge = m_frameNumber - pBins[s].lastUpdate;

                if (curAge >= maxAge)
                {
                    maxAge = curAge;
                    idx = s;
                }
                s--;

                if (s < 0)
                {
                    break;
                }
            }
            // Replace found bin with new value
            pBins[idx].values[0] = curPix[0];
            pBins[idx].values[1] = curPix[1];
            pBins[idx].values[2] = curPix[2];
            pBins[idx].height = 1;
            pBins[idx].lastUpdate = m_frameNumber;
        }

        // Check old bins that need to be decreased
        for(s = 0; s < MAX_BG_SAMPLES; s++)
        {
            if ((m_frameNumber - pBins[s].lastUpdate) > BIN_LIFE_TIME)
            {
                if (pBins[s].height > 0)
                {
                    pBins[s].height--;

                    // Keep our bins sorted by height
                    idx = s;
                    while (idx < MAX_BG_SAMPLES && (pBins[idx].height < pBins[idx + 1].height))
                    {
                        SwapBGBins(p, idx, idx + 1);
                        idx++;
                    }
                }
            }
        }
    }
}

void MultimodalBG::GetFG(VideoFrame* pFrame, VideoBuffer *pFGMask)
{
    int     s;
    bool    isFg;
    float   avgHeight0 = 0.0f;       // Average height of zero bin
    float   fgPercent = 0.0f;

    unsigned char   curPix[3];
    unsigned char*  pFrameData = pFrame->GetRGB24Data();
    int             stride = pFrame->GetStrideRGB();
    unsigned char*  pFGData = pFGMask->GetPlaneData();
    int             strideFg = pFGMask->GetStride();

    // We consider valid fg detection results by default
    isFGValid = 1;

    pFGMask->Zero();

    for(int p = 0; p < m_pixelCount; ++p)
    {
        int x = p % m_width;
        int y = p / m_width;

        curPix[0] = pFrameData[y*stride + x*3 + 0];
        curPix[1] = pFrameData[y*stride + x*3 + 1];
        curPix[2] = pFrameData[y*stride + x*3 + 2];

        // Calculating highest bing average height for further check for backgroung model validity
        avgHeight0 += m_pBGModel[p].bins[0].height;

        s = 0;
        isFg = true;
        while (m_pBGModel[p].bins[s].height >= MIN_BG_BIN_HEIGHT)
        {
            int d0 = std::abs(m_pBGModel[p].bins[s].values[0] - curPix[0]);
            int d1 = std::abs(m_pBGModel[p].bins[s].values[1] - curPix[1]);
            int d2 = std::abs(m_pBGModel[p].bins[s].values[2] - curPix[2]);
            int d = MAX(d2, MAX(d1, d0));

            if(d < m_bgThreshold)
            {
                isFg = false;
                break;
            }
            s++;
        }
        isFg = (s > 0) ? isFg : 0;                  // Reset fg if we do not have valid bg model for current pixel
        pFGData[y*strideFg + x] = (isFg) ? 255 : 0; // Set fg mask value
        fgPercent += (isFg) ? 1.0f : 0.0f;          // Calculate fg percent for scene change analysis
    }

    // Averaging
    fgPercent = (fgPercent * 100.0f) / (float)m_pixelCount;
    avgHeight0 /= (float)m_pixelCount;

    // Check, whether we have invalid background model, or suddenly changed background
    if (avgHeight0 < MIN_BG_BIN_HEIGHT || fgPercent > m_fgThreshold)
    {
        memset(pFGData, 0, m_pixelCount);
        isFGValid = 0;
    }

    // If background suddenly cahnged - we must reset current model
    if (fgPercent > m_fgThreshold && avgHeight0 > 2*MIN_BG_BIN_HEIGHT)
    {
        Init(m_width, m_height);    // This will reset current BG model
    }
}

void MultimodalBG::SwapBGBins(int p, int idx1, int idx2)
{
    BGBin_t  tmp;

    tmp.height      = m_pBGModel[p].bins[idx1].height;
    tmp.lastUpdate  = m_pBGModel[p].bins[idx1].lastUpdate;
    tmp.values[0]   = m_pBGModel[p].bins[idx1].values[0];
    tmp.values[1]   = m_pBGModel[p].bins[idx1].values[1];
    tmp.values[2]   = m_pBGModel[p].bins[idx1].values[2];

    m_pBGModel[p].bins[idx1].height     = m_pBGModel[p].bins[idx2].height;
    m_pBGModel[p].bins[idx1].lastUpdate = m_pBGModel[p].bins[idx2].lastUpdate;
    m_pBGModel[p].bins[idx1].values[0]  = m_pBGModel[p].bins[idx2].values[0];
    m_pBGModel[p].bins[idx1].values[1]  = m_pBGModel[p].bins[idx2].values[1];
    m_pBGModel[p].bins[idx1].values[2]  = m_pBGModel[p].bins[idx2].values[2];

    m_pBGModel[p].bins[idx2].height     = tmp.height;
    m_pBGModel[p].bins[idx2].lastUpdate = tmp.lastUpdate;
    m_pBGModel[p].bins[idx2].values[0]  = tmp.values[0];
    m_pBGModel[p].bins[idx2].values[1]  = tmp.values[1];
    m_pBGModel[p].bins[idx2].values[2]  = tmp.values[2];
}

void MultimodalBG::FilterFG(VideoBuffer *pFGMask)
{
    // Dilate
    pFGMask->Blur(5, 1.0);
    pFGMask->Binarize(141);

    // Erode
    pFGMask->Blur(5, 1.1f);
    pFGMask->Binarize(160);
}


void MultimodalBG::HSVSuppressionOCV(VideoFrame* pFrame, VideoBuffer* pFGMask)
{
    const float alpha = 0.65f;
    const float beta = 1.15f;
    const float tau_s = 60.;
    const float tau_h = 40.;

    uchar h_i, s_i, v_i;
    uchar h_b, s_b, v_b;
    float h_diff, s_diff, v_ratio;

    cv::Mat                 rgb(pFrame->GetHeight(), pFrame->GetWidth(), CV_8UC3, pFrame->GetRGB24Data(), pFrame->GetStrideRGB());
    cv::Mat                 hsv;
    std::vector<cv::Mat>    imHSV;
    cv::Mat                 bgrPixel(cv::Size(1, 1), CV_8UC3);

    cv::cvtColor(rgb, hsv, CV_BGR2HSV_FULL);

    cv::split(hsv, imHSV);

    unsigned char* pFg = pFGMask->GetPlaneData();
    int            stride = pFGMask->GetStride();

    for (int p = 0; p < m_pixelCount; p++)
    {
        int x = p % m_width;
        int y = p / m_width;

        if(pFg[y*stride + x] > 0)
        {
            h_i = imHSV[0].data[p];
            s_i = imHSV[1].data[p];
            v_i = imHSV[2].data[p];

            for (int n = 0; n < MAX_BG_SAMPLES; n++)
            {
                if (m_pBGModel[p].bins[n].height > MIN_BG_BIN_HEIGHT)
                {
                    cv::Mat hsvPixel;

                    bgrPixel.at<cv::Vec3b>(0,0)[0] = m_pBGModel[p].bins[n].values[0];
                    bgrPixel.at<cv::Vec3b>(0,0)[1] = m_pBGModel[p].bins[n].values[1];
                    bgrPixel.at<cv::Vec3b>(0,0)[2] = m_pBGModel[p].bins[n].values[2];

                    cv::cvtColor(bgrPixel, hsvPixel, CV_BGR2HSV_FULL);

                    h_b = hsvPixel.at<cv::Vec3b>(0,0)[0];
                    s_b = hsvPixel.at<cv::Vec3b>(0,0)[1];
                    v_b = hsvPixel.at<cv::Vec3b>(0,0)[2];

                    v_ratio = (float)v_i / (float)v_b;
                    s_diff = std::abs(s_i - s_b);
                    h_diff = std::min( std::abs(h_i - h_b), 255 - std::abs(h_i - h_b));

                    if (h_diff <= tau_h && s_diff <= tau_s && v_ratio >= alpha && v_ratio < beta)
                    {
                        pFg[y*stride + x] = 0; // Shadow is not FG
                        break;
                    }
                }
            }
        }
    }
}
