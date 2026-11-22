// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <OpenThr.hpp>

#include <QCoreApplication>

extern "C"
{
    #include <libavformat/avformat.h>
}

void AbortContext::abort()
{
    QMutexLocker locker(&openMutex);
    isAborted = true;
    openCond.wakeOne();
}

/**/

void OpenThr::drop()
{
    moveToThread(qApp->thread()); //Execute "deleteLater()" in main thread, because in this thread "processEvents()" might not be called.
}

OpenThr::OpenThr(const QByteArray &url, AVDictionary *options, std::shared_ptr<AbortContext> &abortCtx) :
    m_url(url),
    m_options(options),
    m_abortCtx(abortCtx),
    m_finished(false)
{
    connect(this, SIGNAL(finished()), this, SLOT(deleteLater()));
}

bool OpenThr::waitForOpened() const
{
    QMutexLocker locker(&m_abortCtx->openMutex);
    if (!m_finished && !m_abortCtx->isAborted)
        m_abortCtx->openCond.wait(&m_abortCtx->openMutex);
    if (m_abortCtx->isAborted)
        return false;
    return true;
}

bool OpenThr::wakeIfNotAborted()
{
    QMutexLocker locker(&m_abortCtx->openMutex);
    if (!m_abortCtx->isAborted)
    {
        m_finished = true;
        m_abortCtx->openCond.wakeOne();
        return true;
    }
    return false;
}
