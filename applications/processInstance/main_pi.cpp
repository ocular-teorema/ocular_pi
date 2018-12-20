/**
 * \file main_qt_recorder.cpp
 * \brief Entry point for the recorder application
 *
 * \date Sep 17, 2010
 * \author Sergey Levi
 */

#include <string>
#include <iostream>
#include <QtCore/qobjectdefs.h>

#include "configManager.h"


#include "log.h"
#include "utils.h"
#include "qtFileLoader.h"
#include "g12Image.h"
#include "cameraPipeline.h"

#include "qSettingsGetter.h"
#include "networkUtils/dataDirectory.h"

#include "Generated/analysisParams.h"

#ifdef WITH_DEBUG_UI
#include <QApplication>
#include "ui/instanceUIWindow.h"
#endif

using namespace std;
using namespace corecvs;

int main(int argc, char *argv[])
{
    setSegVHandler();
    setStdTerminateHandler();

    Q_INIT_RESOURCE(main);

#ifdef WITH_DEBUG_UI
    QApplication application(argc, argv);
#else
    QCoreApplication application(argc, argv);
#endif

    // Parsing input
    if(argc < 2)
    {
        ConfigManager::setConfigName("theorem.conf");
    }
    else
    {
        ConfigManager::setConfigName(QString(argv[1]));
    }

    QSettings settings(ConfigManager::configName(), QSettings::IniFormat);
    int port = settings.value("HttpPort", 6781).toInt();

    // Preparing data
    DataDirectory mainData(port);
    AnalysisParams params;
    EventProcessingParams eventParams;
    CommonPipelineParameters pipelineParams;

    SettingsGetter  getter(ConfigManager::configName(), "AnalysisParams");
    SettingsGetter  getterEvents(ConfigManager::configName(), "EventParams");
    SettingsGetter  getterPipeline(ConfigManager::configName(), "PipelineParams");

    params.accept<SettingsGetter>(getter);
    eventParams.accept<SettingsGetter>(getterEvents);
    pipelineParams.accept<SettingsGetter>(getterPipeline);

    mainData.setPipelineParams(pipelineParams);
    mainData.setParams(params);
    mainData.setEventParams(eventParams);

    // Starting HTTP
//    HttpServer httpServer(port);
//    ImageListModule imageModule;
//    imageModule.mImages = static_cast<ImageListModuleDAO *>(&mainData);
//    httpServer.content()->mModuleList.push_back(&imageModule);

//    ReflectionListModule reflectionModule;
//    reflectionModule.mReflectionsDAO = static_cast<ReflectionModuleDAO *>(&mainData);
//    httpServer.content()->mModuleList.push_back(&reflectionModule);

//    StatisticsListModule statModule;
//    statModule.mStatisticsDAO = static_cast<StatisticsModuleDAO *>(&mainData);
//    httpServer.content()->mModuleList.push_back(&statModule);

//    GraphModule graphModule;
//    graphModule.mGraphData = static_cast<GraphModuleDAO *>(&mainData);
//    httpServer.content()->mModuleList.push_back(&graphModule);

//    AnalysisRecordSQLiteDao *db = AnalysisRecordSQLiteDao::Instance(QString::fromStdString(pipelineParams.databasePath()));

//    DBModule dbModule(db);
//    httpServer.content()->mModuleList.push_back(&dbModule);

//    SignalModule signalModule;
//    httpServer.content()->mModuleList.push_back(&signalModule);
//    signalModule.addSignal(SignalDescriptor(&mainData, "securityReaction(int, int)", "securityReaction", true ));

//    // Adding rewriter values
//    httpServer.mContentRewrites["name"] = QString::fromStdString(pipelineParams.pipelineName());

//    // Streams and longpolls
//    httpServer.addStream("test");
//    httpServer.addStream("events");
//    QObject::connect(&mainData, SIGNAL(eventSignalize(QString, QVariantMap)), &httpServer, SLOT(newContent(QString, QVariantMap)));

    // Set mainData pointer to singleton class
    DataDirectoryInstance::SetInstancePointer(&mainData);

    // Check, if data directory and event handler pointers are accessible
    DataDirectory*  pDataDir = DataDirectoryInstance::instance();
    ErrorHandler*   pErrHandler = ErrorHandler::instance();

    if (NULL == pDataDir)
    {
        printf("Data directory is not accessible via singleton class. Exiting");
        return -1;
    }

    if (NULL == pErrHandler)
    {
        printf("Error handler is not accessible via singleton class. Exiting");
        return -1;
    }

    // Starting camera pipeline
    CameraPipeline  pipeline(&application);

    // Close pipeline when application is going to be closed
    QObject::connect(&application, SIGNAL(aboutToQuit()), &pipeline, SLOT(StopPipeline()));

    // Quit application, when processing is finished
    QObject::connect(&pipeline, SIGNAL(ProcessingStopped()), &application, SLOT(quit()));

    // Run our pipeline after application EventLoop started
    QTimer::singleShot(0, &pipeline, SLOT(RunPipeline()));

//    signalModule.addSignal(SignalDescriptor(&pipeline, "PausePipeline"));
//    signalModule.addSignal(SignalDescriptor(&pipeline, "UnpausePipeline"));
//    signalModule.addSignal(SignalDescriptor(&pipeline, "StepPipeline"));
//    signalModule.addSignal(SignalDescriptor(&pipeline, "StopPipeline"));

#ifdef WITH_DEBUG_UI
    InstanceUIWindow ui;
    ui.setDataDirectory(&mainData);

    QObject::connect(&ui, SIGNAL(eventPause()) , &pipeline, SLOT(PausePipeline()));
    QObject::connect(&ui, SIGNAL(eventResume()), &pipeline, SLOT(UnpausePipeline()));
    QObject::connect(&ui, SIGNAL(eventStep())  , &pipeline, SLOT(StepPipeline()));
    QObject::connect(&ui, SIGNAL(eventStop())  , &pipeline, SLOT(StopPipeline()));
    ui.show();
#endif

    // Main loop
    application.exec();

//    httpServer.stop();
}

