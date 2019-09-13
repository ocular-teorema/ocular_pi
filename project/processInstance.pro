
TARGET   = processInstance
TEMPLATE = app

QT += sql
QT += opengl
QT += testlib
QT += websockets

INCLUDEPATH += ../
INCLUDEPATH += ../CameraPipeline
INCLUDEPATH += ../CameraPipeline/dbstat
INCLUDEPATH += ../videoAnalysis
INCLUDEPATH += /usr/include

HEADERS += \
    ../CameraPipeline/pipelineCommonTypes.h \
    ../CameraPipeline/eventHandler.h \
    ../CameraPipeline/eventDescription.h \
    ../CameraPipeline/errorHandler.h \
    ../CameraPipeline/dataDirectoryInstance.h \
    ../CameraPipeline/frameCircularBuffer.h \
    ../CameraPipeline/healthChecker.h \
    ../CameraPipeline/pipelineConfig.h \
    ../CameraPipeline/parameters.h \
    ../CameraPipeline/rtspCapture.h \
    ../CameraPipeline/cameraPipelineCommon.h \
    ../CameraPipeline/videoAnalyzer.h \
    ../CameraPipeline/streamRecorder.h \
    ../CameraPipeline/cameraPipeline.h \
    ../CameraPipeline/resultVideoOutput.h \
    ../CameraPipeline/decisionMaker.h \
    ../CameraPipeline/dbstat/analysisRecordModel.h \
    ../CameraPipeline/dbstat/analysisRecordSQliteDao.h \
    ../CameraPipeline/dbstat/intervalStatistics.h \
    ../videoAnalysis/objectDetector.h \
    ../videoAnalysis/multimodalBG.h \
    ../videoAnalysis/motionAnalysis.h \
    ../videoAnalysis/motionTypes.h \
    ../videoAnalysis/videoScaler.h \
    ../videoAnalysis/denoiseFilter.h \
    ../networkUtils/dataDirectory.h

SOURCES += \
    ../main_pi.cpp \
    ../CameraPipeline/eventHandler.cpp \
    ../CameraPipeline/eventDescription.cpp \
    ../CameraPipeline/errorHandler.cpp \
    ../CameraPipeline/dataDirectoryInstance.cpp \
    ../CameraPipeline/frameCircularBuffer.cpp \
    ../CameraPipeline/healthChecker.cpp \
    ../CameraPipeline/parameters.cpp \
    ../CameraPipeline/rtspCapture.cpp \
    ../CameraPipeline/cameraPipelineCommon.cpp \
    ../CameraPipeline/videoAnalyzer.cpp \
    ../CameraPipeline/videoProcessingFunctions.cpp \
    ../CameraPipeline/cameraPipeline.cpp \
    ../CameraPipeline/streamRecorder.cpp \
    ../CameraPipeline/resultVideoOutput.cpp \
    ../CameraPipeline/decisionMaker.cpp \
    ../CameraPipeline/dbstat/analysisRecordModel.cpp \
    ../CameraPipeline/dbstat/analysisRecordSQliteDao.cpp \
    ../CameraPipeline/dbstat/intervalStatistics.cpp \
    ../videoAnalysis/objectDetector.cpp \
    ../videoAnalysis/multimodalBG.cpp \
    ../videoAnalysis/motionAnalysis.cpp \
    ../videoAnalysis/denoiseFilter.cpp \
    ../videoAnalysis/videoScaler.cpp \
    ../networkUtils/dataDirectory.cpp

QMAKE_CXXFLAGS += -std=gnu++11

# LIBS
LIBS +=  -L/usr/lib
LIBS +=  -lavformat -lavcodec -lavutil -lswresample -lswscale -lxcb-xfixes -lxcb-render -lxcb-shape -lxcb -lX11 -lx264 -lm -lz
LIBS +=  -lopencv_imgcodecs -lopencv_imgproc -lopencv_highgui -lopencv_objdetect -lopencv_core -lopencv_features2d -lopencv_video
