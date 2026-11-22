// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

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
