#pragma once

#include <opencv2/core/core.hpp>
//C++
#include <vector>

#include "cameraPipelineCommon.h"

#define BIN_LIFE_TIME       600
#define BG_UPDATE_PERIOD    4
#define MAX_BG_SAMPLES      8
#define MIN_BG_BIN_HEIGHT   32
#define BG_THRESHOLD        17
#define FG_THRESHOLD        40.0
#define SAME_BIN_THRESHOLD  4
#define AVG_COEFF           0.5

class MultimodalBG
{
public:
    MultimodalBG();
    ~MultimodalBG();

    void    ProcessNewFrame(VideoFrame* pFrame, VideoBuffer* pFGMask);    // Updates BG and calculates FG
    float   GetBG(VideoFrame** ppBG, int binIndex);                       // Returns average bin height for given index

    void    SetThreshold(int bgThr, double fgThr) { m_bgThreshold = bgThr; m_fgThreshold = fgThr; }

    int     isFGValid;

private:

    //struct for modeling the background values for a single pixel
    typedef struct
    {
        uchar   values[3];   // averaged R,G,B values
        uint    height;      // bin height
        uint    lastUpdate;  // whel last update happened
    } BGBin_t;

    typedef struct {
        BGBin_t     bins[MAX_BG_SAMPLES];
    } BgModel_t;

    int             m_width;
    int             m_height;
    int             m_pixelCount;
    int             m_frameNumber;
    int             m_bgThreshold;
    double          m_fgThreshold;

    BgModel_t*      m_pBGModel;

    VideoBuffer*    m_pPrevFG;

    VideoFrame*     m_pBG;

    void Init(int width, int height);
    void UpdateBG(VideoFrame* pFrame);
    void GetFG(VideoFrame *pFrame, VideoBuffer* pFGMask);
    void SwapBGBins(int p, int idx1, int idx2);
    void FilterFG(VideoBuffer* pFGMask);
    void HSVSuppressionOCV(VideoFrame *pFrame, VideoBuffer *pFGMask);
    void ConvertImageRGBtoHSV(int width, int height, int stride, unsigned char *pRGBData, cv::Mat& imageHSV);
};

