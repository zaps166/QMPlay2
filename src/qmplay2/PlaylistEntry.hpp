#pragma once

#include <QString>
#include <QVector>

class PlaylistEntry
{
public:
    enum Flags
    {
        Selected = 0x1,
        Skip = 0x2,
        StopAfter = 0x4,
        Locked = 0x8,
        AlwaysSync = 0x10,
    };

    PlaylistEntry() = default;
    inline PlaylistEntry(const QString &name, const QString &url, double length = -1.0)
        : name(name)
        , url(url)
        , length(length)
    {}

    QString name, url;
    double length = -1.0;
    qint32 flags = 0, queue = 0, GID = 0, parent = 0;
};
using PlaylistEntries = QVector<PlaylistEntry>;
