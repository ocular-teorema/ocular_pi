#pragma once

#include <stdint.h>
#include <string>
#include <QtCore/QThread>
#include <QtCore/QMutex>

#include "capdll.h"
#include "imageCaptureInterface.h"
#include "preciseTimer.h"
#include "cameraControlParameters.h"

class DirectShowCameraDescriptor
{
public:
    static cchar*              codec_names[];
    static CAPTURE_FORMAT_TYPE codec_types[];

    DSCapDeviceId   deviceHandle;
    int             size;
    uint64_t        decodeTime;
    uint64_t        timestamp;
    bool            gotBuffer;
    G12Buffer      *buffer;
    RGB24Buffer    *buffer24;
    uint8_t        *rawBuffer;
    int             height;
    int             width;

    /* Codec descriptor */
    enum {
       UNCOMPRESSED_YUV = 0,
       UNCOMPRESSED_RGB = 1,
       COMPRESSED_JPEG = 2,
       COMPRESSED_FAST_JPEG = 3
    };

    DirectShowCameraDescriptor()
        : deviceHandle(-1)
        , size(0)
        , decodeTime(0)
        , timestamp(0)
        , gotBuffer(false)
        , buffer(NULL)
        , buffer24(NULL)
        , rawBuffer(NULL)
        , height(0)
        , width(0)
    {}

private:
    static void setFromCameraParam(CaptureParameter &param,CameraParameter &camParam);

public:
    int queryCameraParameters(CameraParameters &parameters);
    int setCaptureProperty(int id, int value);
    int getCaptureProperty(int id, int *value);
};

/* EOF */
