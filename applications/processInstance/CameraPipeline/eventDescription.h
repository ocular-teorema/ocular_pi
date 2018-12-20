#ifndef EVENTDESCRIPTION_H
#define EVENTDESCRIPTION_H

#include <QString>
#include <QDateTime>
#include <QVariantMap>

enum ReactionType
{
    REACTION_UNSET  =-1,
    REACTION_IGNORE = 0,
    REACTION_ALERT  = 1,
    REACTION_FALSE  = 2
};

class EventDescription
{
public:
    EventDescription() :
        id(-1),
        isActive(false),
        confidence(0),
        reaction(REACTION_UNSET),
        archiveFileName1("empty"),
        archiveFileName2("empty"),
        offset(0)
    { }

    int                         id;
    bool                        isActive;           /// For convenient tracking
    unsigned int                eventType;          /// Bit flags indicated what's happened (see pipelineCommonTypes.h)
    int                         confidence;         /// Event confidence in percent (0-100)
    ReactionType                reaction;           /// Security officer reaction

    QDateTime                   startTime;
    QDateTime                   endTime;
    QString                     archiveFileName1;   /// Archive file, where event started
    QString                     archiveFileName2;   /// Archive file, where event finished
    qlonglong                   offset;

    QVariantMap toVariant();
};

#endif // EVENTDESCRIPTION_H
