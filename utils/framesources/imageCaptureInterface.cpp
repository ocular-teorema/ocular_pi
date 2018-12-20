/**
 * \file ImageCaptureInterface.cpp
 * \brief Add Comment Here
 *
 * \date Mar 14, 2010
 * \author alexander
 */
#include <iostream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <QtCore/QMetaType>

#include "global.h"

#include "imageCaptureInterface.h"
#include "cameraControlParameters.h"

#include "fileCapture.h"
#include "precCapture.h"

#ifdef Q_OS_LINUX
# include "V4L2Capture.h"
# include "V4L2CaptureDecouple.h"
#endif

#ifdef WITH_DIRECTSHOW
# include "directShowCapture.h"
# include "directShowCaptureDecouple.h"
#endif

#ifdef WITH_UEYE
# include "uEyeCapture.h"
#endif

#ifdef WITH_SYNCCAM
#include "syncCamerasCaptureInterface.h"
#endif

#ifdef WITH_AVCODEC
#include "aviCapture.h"
#include "rtspCapture.h"
#include "aviListBase.h"
#include "aviTGListBase.h"

#endif

#ifdef WITH_OPENCV
# include "openCVCapture.h"
# include "openCVFileCapture.h"
#endif

char const *CaptureStatistics::names[] =
{
    "Desync time",
    "Internal desync",
    "Decoding time",
    "Converting time",
    "Interframe delay",
    "Frame Data size"
};

STATIC_ASSERT(CORE_COUNT_OF(CaptureStatistics::names) == CaptureStatistics::MAX_ID, wrong_comment_num_capture_stats);


ImageCaptureInterface* ImageCaptureInterface::fabric(string input, bool isRGB)
{
    string file("file:");
    if (input.substr(0, file.size()).compare(file) == 0)
    {
        string tmp = input.substr(file.size());
        return new FileCaptureInterface(tmp);
    }

    string prec("prec:");
    if (input.substr(0, prec.size()).compare(prec) == 0)
    {
        string tmp = input.substr(prec.size());
        return new FilePreciseCapture(QString(tmp.c_str()), false);
    }

#ifdef WITH_SYNCCAM
    string sync("sync:");
    if (input.substr(0, sync.size()).compare(sync) == 0)
    {
        string tmp = input.substr(sync.size());
        return new SyncCamerasCaptureInterface(tmp);
    }
#endif

#ifdef Q_OS_LINUX
    string v4l2("v4l2:");
    if (input.substr(0, v4l2.size()).compare(v4l2) == 0)
    {
        string tmp = input.substr(v4l2.size());
        return new V4L2CaptureInterface(tmp, isRGB);
    }

    string v4l2d("v4l2d:");
    if (input.substr(0, v4l2d.size()).compare(v4l2d) == 0)
    {
        string tmp = input.substr(v4l2d.size());
        return new V4L2CaptureDecoupleInterface(tmp);
    }
#endif

#ifdef WITH_UEYE
    string ueye("ueye:");
    if (input.substr(0, ueye.size()).compare(ueye) == 0)
    {
        string tmp = input.substr(ueye.size());
        return new UEyeCaptureInterface(tmp);
    }
#endif

#ifdef WITH_DIRECTSHOW
    string dshow("dshow:");
    if (input.substr(0, dshow.size()).compare(dshow) == 0)
    {
        string tmp = input.substr(dshow.size());
        return new DirectShowCaptureInterface(tmp, isRGB);
    }

    string dshowd("dshowd:");
    if (input.substr(0, dshowd.size()).compare(dshowd) == 0)
    {
        string tmp = input.substr(dshowd.size());
        return new DirectShowCaptureDecoupleInterface(tmp);
    }
#endif

#ifdef WITH_AVCODEC
    string avcodec("avcodec:");
    if (input.substr(0, avcodec.size()).compare(avcodec) == 0)
    {
        SYNC_PRINT(("ImageCaptureInterface::fabric(): Creating avcodec input"));
        string tmp = input.substr(avcodec.size());
        return new AviCapture(QString(tmp.c_str()));
    }
    string rtsp("rtsp:");
    if (input.substr(0, rtsp.size()).compare(rtsp) == 0)
    {
        SYNC_PRINT(("ImageCaptureInterface::fabric(): Creating rtsp input"));
        return new RTSPCapture(QString(input.c_str()));
    }
    string avlist("avlist:");
    if (input.substr(0, avlist.size()).compare(avlist) == 0)
    {
        SYNC_PRINT(("ImageCaptureInterface::fabric(): Creating avlist input"));
        string tmp = input.substr(avlist.size());
        return new AviListBase(QString(tmp.c_str()));
    }

    string avtg("avtg:");
    if (input.substr(0, avtg.size()).compare(avtg) == 0)
    {
        SYNC_PRINT(("ImageCaptureInterface::fabric(): Creating avtg input"));
        string tmp = input.substr(avtg.size());
        return new AviTGListBase(QString(tmp.c_str()));
    }

#endif

#ifdef WITH_OPENCV
    string any("any:");
    if (input.substr(0, any.size()).compare(any) == 0)
    {
        string tmp = input.substr(any.size());
        return new OpenCVCaptureInterface(tmp, CAP_ANY);
    }

    string vfw("vfw:");
    if (input.substr(0, vfw.size()).compare(vfw) == 0)
    {
        string tmp = input.substr(vfw.size());
        return new OpenCVCaptureInterface(tmp, CAP_VFW);
    }

    string ds("ds:");
    if (input.substr(0, ds.size()).compare(ds) == 0)
    {
        string tmp = input.substr(ds.size());
        return new OpenCVCaptureInterface(tmp, CAP_DS);
    }

    string opencv_file("opencv_file:");
    if (input.substr(0, opencv_file.size()).compare(opencv_file) == 0)
    {
        string tmp = input.substr(opencv_file.size());
        return new OpenCvFileCapture(QString(tmp.c_str()));
    }
#endif

    return NULL;
}

void ImageCaptureInterface::notifyAboutNewFrame(frame_data_t frameData)
{
//    SYNC_PRINT(("ImageCaptureInterface::notifyAboutNewFrame()\n"));
    emit newFrameReady(frameData);
    emit newImageReady();
}

ImageCaptureInterface::ImageCaptureInterface()
   : mIsRgb(false)
{
   qRegisterMetaType<frame_data_t>("frame_data_t");
}

ImageCaptureInterface::~ImageCaptureInterface()
{
    return;
}

ImageCaptureInterface::CapErrorCode ImageCaptureInterface::setCaptureProperty(int /*id*/, int /*value*/)
{
    return FAILURE;
}

ImageCaptureInterface::CapErrorCode ImageCaptureInterface::getCaptureProperty(int /*id*/, int * /*value*/)
{
    return FAILURE;
}

ImageCaptureInterface::CapErrorCode ImageCaptureInterface::getCaptureName(QString & /*value*/)
{
    return FAILURE;
}

ImageCaptureInterface::CapErrorCode ImageCaptureInterface::getFormats(int * /*num*/, CameraFormat *& /*format*/)
{
    return FAILURE;
}

QString ImageCaptureInterface::getInterfaceName()
{
    return "";
}

ImageCaptureInterface::CapErrorCode ImageCaptureInterface::getDeviceName(int /*num*/, QString & /*name*/)
{
    return FAILURE;
}

string ImageCaptureInterface::getDeviceSerial(int num)
{
    return "";
}

ImageCaptureInterface::CapErrorCode ImageCaptureInterface::initCapture()
{
    return FAILURE;
}

ImageCaptureInterface::CapErrorCode ImageCaptureInterface::startCapture()
{
    return FAILURE;
}

ImageCaptureInterface::CapErrorCode ImageCaptureInterface::pauseCapture()
{
    return FAILURE;
}

ImageCaptureInterface::CapErrorCode ImageCaptureInterface::nextFrame()
{
    return FAILURE;
}

ImageCaptureInterface::CapErrorCode ImageCaptureInterface::queryCameraParameters(CameraParameters &parameter)
{
    Q_UNUSED(parameter)
    return ImageCaptureInterface::SUCCESS;
}

bool ImageCaptureInterface::supportPause()
{
    return false;
}
