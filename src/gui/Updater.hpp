/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2022  Błażej Szczygieł

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published
    by the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <NetworkAccess.hpp>

#include <QFile>

#ifdef UPDATER
    #include <QDialog>

    class QProgressBar;
    class QPushButton;
    class QLabel;
#endif

class Updater final : public
#ifdef UPDATER
    QDialog
#else
    QObject
#endif
{
    Q_OBJECT

public:
#ifdef UPDATER
    Updater(QWidget *parent);
#else
    Updater(QObject *parent);
#endif
    ~Updater();

#ifdef UPDATER
    bool downloading() const;
#endif

public slots:
    void downloadUpdate();

private slots:
    void infoFinished();
#ifdef UPDATER
    void writeToFile();
    void downloadprogress(int bytesReceived, int bytesTotal);
    void downloadFinished();

    void applyUpdate();
#endif

private:
#ifdef UPDATER
    NetworkReply *getFile(const QString &url);
    void endWork(const QString &str);
#endif

    NetworkAccess net;
    QFile infoFile;
    bool busy;
#ifdef UPDATER
    bool firstChunk;
    QFile updateFile;
    QLabel *infoL;
    QProgressBar *progressB;
    QPushButton *downloadUpdateB, *installB;
#endif
};
