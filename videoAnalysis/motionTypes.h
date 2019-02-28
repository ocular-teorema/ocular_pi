#pragma once

#include <QByteArray>
#include <QDataStream>

#define     ME_BLOCK_SIZE           8
                                            // Attention!
#define     MAX_MV_SAMPLES          8       // Requires manually hardcoded direction table!!!!!
                                            // Do not change without need
#define     MIN_MV_BIN_HEIGHT       8

#define     MOTION_THR              2.0f


typedef struct mv
{
    float   x;
    float   y;
    float   sad;
    float   confidence;
} mv_t;

typedef struct
{
    float     mvx;
    float     mvy;
    float     height;
    int       directionIdx;
} MotionBin_t;

typedef struct {
    MotionBin_t     bins[MAX_MV_SAMPLES];
} MotionModel_t;

class MotionFlow
{
public:
    MotionFlow(int w, int h, int bsize);
    ~MotionFlow();

    int     width;
    int     height;
    int     blockSize;
    mv_t*   pVectors;

    mv_t    GetVector(int x, int y, float scale = 1.0f);
    mv_t    GetPopularNonzeroVectorNB(int x, int y, int radius = 1);
};

class MotionMap
{
public:
    MotionMap();
    ~MotionMap();

    void    Reset();
    void    Update(MotionFlow* pCurFlow);
    void    CopyFrom(MotionMap* pMap);
    void    AddMap(MotionMap* pMap, float weight = 1.0f);
    void    DrawMotionMap(VideoBuffer *pRes, int index, float scale);
    void    ToByteArray(QByteArray* bytes);
    void    FromDataStream(QDataStream& in);
    float   CompareWith(MotionFlow* pCurFlow, VideoBuffer* pResultMask);
    int     CheckObject(MotionFlow *pCurFlow, int *decision, float* confidence, int x, int y, int w, int h);

    void    SetValidBinHeight(int validBinHeight) { m_validBinHeight = (float)validBinHeight; }

    int     GetDirectionIdx(float mvx, float mvy);

    int     GetWidth() { return m_width; }
    int     GetHeight() { return m_height; }
    int     GetBlockSize() { return m_blockSize; }
    int     GetBlocksCount() { return m_blocksCount; }
    MotionModel_t* GetModelPtr() { return m_pModel; }
private:
    int             m_width;
    int             m_height;
    int             m_blockSize;
    int             m_blocksCount;
    int             m_frameNumber;
    float           m_validBinHeight;
    MotionModel_t*  m_pModel;
    double          m_pDirectionTable[MAX_MV_SAMPLES];

    void    Init(int width, int height, int blockSize);
    void    SwapMotionBins(int p, int idx1, int idx2);
    void    AverageMV(MotionBin_t *pBin, mv_t &curMV);
    int     CheckVelocity(MotionBin_t *pBin, mv_t curMV, float *confidence);

    MotionBin_t* GetBinByDirection(int p, int direction);
};
