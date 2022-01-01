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
