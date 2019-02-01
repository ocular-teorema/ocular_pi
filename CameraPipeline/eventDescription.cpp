#include "eventDescription.h"


QVariantMap EventDescription::toVariant()
{
    QVariantMap toReturn;

    toReturn.insert("id", id);
    toReturn.insert("start_timestamp", startTime.toMSecsSinceEpoch());
    toReturn.insert("end_timestamp", endTime.toMSecsSinceEpoch());
    toReturn.insert("type", eventType);
    toReturn.insert("confidence", confidence);
    toReturn.insert("reaction", QString::number(reaction));
    toReturn.insert("archiveStartHint", archiveFileName1);
    toReturn.insert("archiveEndHint"  , archiveFileName2);
    toReturn.insert("file_offset_sec"  , offset);

    if (isActive)
    {
        toReturn.insert("isStarted", true);
    }
    else
    {
        toReturn.insert("isFinished", true);
    }

    return toReturn;
}
