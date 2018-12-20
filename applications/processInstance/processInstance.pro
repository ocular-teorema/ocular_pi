##
## Common config header (try use global config)
##
exists(../../../../config.pri) {
    ROOT_DIR=../../../..
} else {
    message(Using local config)
    ROOT_DIR=../..
}
!win32 {                                        # it dues to the "mocinclude.tmp" bug on win32!
    ROOT_DIR=$$PWD/$$ROOT_DIR
}
include($$ROOT_DIR/config.pri)

#
# APP and Qt related
#

TARGET   = processInstance
TEMPLATE = app

QT += sql
QT += opengl
QT += testlib
QT += websockets

with_piui {
    message("Building with UI support")
    CONFIG += with_ui
}

# We will need UTILS
UTILSDIR = ../../utils
include($$UTILSDIR/utils.pri)

INCLUDEPATH += .
INCLUDEPATH += /usr/include

HEADERS += \
    pipelineCommonTypes.h \
    CameraPipeline/eventHandler.h \
    CameraPipeline/eventDescription.h \
    videoAnalysis/objectDetector.h \
    videoAnalysis/multimodalBG.h \
    videoAnalysis/motionAnalysis.h \
    videoAnalysis/motionTypes.h \
    networkUtils/dataDirectory.h \
    networkUtils/smtp.h \
    CameraPipeline/errorHandler.h \
    CameraPipeline/dataDirectoryInstance.h \
    videoAnalysis/videoScaler.h \
    CameraPipeline/frameCircularBuffer.h \
    CameraPipeline/healthChecker.h \
    CameraPipeline/pipelineConfig.h \
    videoAnalysis/denoiseFilter.h

SOURCES += \
    main_pi.cpp \
    CameraPipeline/eventHandler.cpp \
    CameraPipeline/eventDescription.cpp \
    videoAnalysis/objectDetector.cpp \
    videoAnalysis/multimodalBG.cpp \
    videoAnalysis/motionAnalysis.cpp \
    networkUtils/dataDirectory.cpp \
    networkUtils/smtp.cpp \
    CameraPipeline/errorHandler.cpp \
    CameraPipeline/dataDirectoryInstance.cpp \
    videoAnalysis/videoScaler.cpp \
    CameraPipeline/frameCircularBuffer.cpp \
    CameraPipeline/healthChecker.cpp \
    videoAnalysis/denoiseFilter.cpp

# Pipeline code

SOURCES += \
    CameraPipeline/rtspCapture.cpp \
    CameraPipeline/cameraPipelineCommon.cpp \
    CameraPipeline/videoAnalyzer.cpp \
    CameraPipeline/videoProcessingFunctions.cpp \
    CameraPipeline/cameraPipeline.cpp \
    CameraPipeline/streamRecorder.cpp \
    CameraPipeline/resultVideoOutput.cpp \
    CameraPipeline/decisionMaker.cpp \
    CameraPipeline/dbstat/analysisRecordModel.cpp \
    CameraPipeline/dbstat/analysisRecordSQliteDao.cpp \
    CameraPipeline/dbstat/intervalStatistics.cpp


HEADERS += \
    CameraPipeline/rtspCapture.h \
    CameraPipeline/cameraPipelineCommon.h \
    CameraPipeline/videoAnalyzer.h \
    CameraPipeline/streamRecorder.h \
    CameraPipeline/cameraPipeline.h \
    CameraPipeline/resultVideoOutput.h \
    CameraPipeline/decisionMaker.h \
    CameraPipeline/dbstat/analysisRecordModel.h \
    CameraPipeline/dbstat/analysisRecordSQliteDao.h \
    CameraPipeline/dbstat/intervalStatistics.h

INCLUDEPATH += CameraPipeline
INCLUDEPATH += accumulator
INCLUDEPATH += CameraPipeline/dbstat
INCLUDEPATH += videoAnalysis

# Generated block
HEADERS += \
    Generated/dateParams.h                  \
    Generated/analysisParams.h              \
    Generated/decisionParams.h              \
    Generated/commonPipelineParameters.h    \
    Generated/eventProcessingParams.h       \
    Generated/objectDetectionParams.h       \


SOURCES += \
    Generated/dateParams.cpp                  \
    Generated/analysisParams.cpp              \
    Generated/decisionParams.cpp              \
    Generated/commonPipelineParameters.cpp    \
    Generated/eventProcessingParams.cpp       \
    Generated/objectDetectionParams.cpp       \

OTHER_FILES += \
  $$ROOT_DIR/tools/generator/xml/pi.xml  \
  $$ROOT_DIR/tools/generator/regen-pi.sh \
  $$ROOT_DIR/theorem.conf \
  $$ROOT_DIR/tools/sqlite/createdb.sh \

with_ui {
  message (Also building ui frontend)
   DEFINES += WITH_DEBUG_UI

   HEADERS += \
    ui/instanceUIWindow.h

   SOURCES += \
    ui/instanceUIWindow.cpp

   FORMS += \
    ui/instanceUIWindow.ui

    INCLUDEPATH += ui

# Generated
   GENERATED_PATH="ui/generated"

   GENERATED_CLASSES=             \
         analysisParams           \
         decisionParams           \
         objectDetectionParams    \
         eventProcessingParams    \
         dateParams               \
         commonPipelineParameters \

    for (GEN_CLASS, GENERATED_CLASSES) {
        message( Adding generated class $${GEN_CLASS} )

        FORMS   += $${GENERATED_PATH}/$${GEN_CLASS}ControlWidget.ui
        SOURCES += $${GENERATED_PATH}/$${GEN_CLASS}ControlWidget.cpp
        HEADERS += $${GENERATED_PATH}/$${GEN_CLASS}ControlWidget.h
    }
}

# LIBS
LIBS +=  -L/usr/lib
LIBS +=  -lavformat -lavcodec -lavutil -lswresample -lswscale -lxcb-xfixes -lxcb-render -lxcb-shape -lxcb -lX11 -lx264 -lm -lz
LIBS +=  -lopencv_imgproc -lopencv_highgui -lopencv_objdetect -lopencv_core -lopencv_features2d -lopencv_video
