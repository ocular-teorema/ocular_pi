#ifndef DATADIRECTORYINSTANCE_H
#define DATADIRECTORYINSTANCE_H

#include "errorHandler.h"
#include "networkUtils/dataDirectory.h"

class DataDirectoryInstance
{
public:
    static DataDirectory* instance()
    {
        if (m_instance == NULL)
        {
            ERROR_MESSAGE0(ERR_TYPE_CRITICAL, "DataDirectoryInstance", "DataDirectory pointer is NULL");
            return NULL;
        }
        else
        {
            return m_instance;
        }
    }

    ~DataDirectoryInstance() { m_instance = NULL; }

    static void SetInstancePointer(DataDirectory* pInstance) { m_instance = pInstance; }

private:

    static DataDirectory* m_instance;

    DataDirectoryInstance() { }
};

#endif // DATADIRECTORYINSTANCE_H
