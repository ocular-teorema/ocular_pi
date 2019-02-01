
#include <string>
#include <iostream>

#include "cameraPipeline.h"
#include "networkUtils/dataDirectory.h"

using namespace std;

int main(int argc, char *argv[])
{
    QCoreApplication application(argc, argv);

    QString cfgPath;

    if(argc < 2)
        cfgPath = "theorem.conf";
    else
        cfgPath = QString(argv[1]);

    QSettings settings(cfgPath, QSettings::IniFormat);

    // Preparing data
    DataDirectory mainData(settings);

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

    // Main loop
    application.exec();
}

