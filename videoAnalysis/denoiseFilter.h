#ifndef DENOISEFILTER_H
#define DENOISEFILTER_H

#include "cameraPipelineCommon.h"

/*
 * Denoise filter (temporal + spatial)
*/

class DenoiseFilter
{
public:
    DenoiseFilter();
    ~DenoiseFilter();

    void  ProcessFrame(VideoFrame* pCur,
                       VideoFrame* pOut,
                       float strength = 16.0f,
                       int spatialRadius = 0);

    void  ProcessBuffer(VideoBuffer* pCur,
                        VideoBuffer* pOut,
                        float strength = 16.0f,
                        int spatialRadius = 0);
private:
    VideoFrame   m_prevFrame;
    VideoBuffer  m_prevBuffer;
};

#endif // DENOISEFILTER_H
