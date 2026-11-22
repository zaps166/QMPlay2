// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QWaitCondition>
#include <QThread>
#include <QMutex>

#include <memory>

class AbortContext
{
public:
    void abort();

    QWaitCondition openCond;
    QMutex openMutex;
    bool isAborted = false;
};

/**/

struct AVFormatContext;
struct AVInputFormat;
struct AVDictionary;

class OpenThr : public QThread
{
public:
    void drop();

protected:
    OpenThr(const QByteArray &url, AVDictionary *options, std::shared_ptr<AbortContext> &abortCtx);

    bool waitForOpened() const;

    bool wakeIfNotAborted();

    const QByteArray m_url;
    AVInputFormat *m_inputFmt;
    AVDictionary *m_options;

    std::shared_ptr<AbortContext> m_abortCtx;

private:
    bool m_finished;
};
