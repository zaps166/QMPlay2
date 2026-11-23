// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <PlaylistEntry.hpp>
#include <IOController.hpp>

class QMPLAY2SHAREDLIB_EXPORT Playlist
{
public:
    using Entry = PlaylistEntry;
    using Entries = PlaylistEntries;

    enum OpenMode
    {
        NoOpen,
        ReadOnly,
        WriteOnly
    };

    virtual ~Playlist() = default;

    static Entries read(const QString &url, QString *name = nullptr);
    static bool write(const Entries &list, const QString &url, QString *name = nullptr);
    static QStringList extensions();

    virtual Entries read() = 0;
    virtual bool write(const Entries &) = 0;

private:
    static Playlist *create(const QString &url, OpenMode openMode, QString *name = nullptr);

protected:
    static QString getPlaylistPath(const QString &url);

    QList<QByteArray> readLines();

    IOController<> ioCtrl;
};
