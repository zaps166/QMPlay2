// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <IOController.hpp>
#include <QMPlay2Lib.hpp>

#include <QCoreApplication>
#include <QProcess>

class NetworkReply;

class QMPLAY2SHAREDLIB_EXPORT YouTubeDL final : public BasicIO
{
    Q_DECLARE_TR_FUNCTIONS(YouTubeDL)
    Q_DISABLE_COPY(YouTubeDL)

public:
    static QString getFilePath();
    static QStringList getCommonArgs();

    static bool fixUrl(const QString &url, QString &outUrl, IOController<> *ioCtrl, QString *name, QString *extension, QString *error);

    YouTubeDL();
    ~YouTubeDL();

    void addr(const QString &url, const QString &param, QString *streamUrl, QString *name, QString *extension, QString *err = nullptr);

    QStringList exec(const QString &url, const QStringList &args, QString *silentErr = nullptr, bool rawOutput = false);

private:
    void abort() override;

private:
    bool prepare();

    bool download();
    bool update();

    void ensureExecutable();

    bool onProcessCantStart();

    void startProcess(QStringList args);

private:
    const QString m_ytDlPath;
    const QStringList m_commonArgs;
    IOController<NetworkReply> m_reply;
    QProcess m_process;
    bool m_aborted;
};
