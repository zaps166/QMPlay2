// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <AVThread.hpp>

#include <Main.hpp>
#include <PlayClass.hpp>

#include <Writer.hpp>
#include <Decoder.hpp>

AVThread::AVThread(PlayClass &playC)
    : playC(playC)
{
    connect(this, SIGNAL(finished()), this, SLOT(deleteLater()));
    mutex.lock();
}
AVThread::~AVThread()
{
    delete dec;
    delete writer;
}

void AVThread::maybeStartThread()
{
    if (writer)
        start();
}

void AVThread::setDec(Decoder *_dec)
{
    dec = _dec;
}

void AVThread::destroyDec()
{
    delete dec;
    dec = nullptr;
}

bool AVThread::lock()
{
    br2 = true;
    if (!mutex.tryLock(MUTEXWAIT_TIMEOUT))
    {
        emit QMPlay2Core.waitCursor();
        const bool ret = mutex.tryLock(MUTEXWAIT_TIMEOUT * 2);
        emit QMPlay2Core.restoreCursor();
        if (!ret)
        {
            br2 = false;
            return false;
        }
    }
    return true;
}
void AVThread::unlock()
{
    br2 = false;
    mutex.unlock();
}

void AVThread::stop(bool _terminate)
{
    if (_terminate)
        return terminate();

    br = true;
    mutex.unlock();
    playC.emptyBufferCond.wakeAll();

    if (!wait(TERMINATE_TIMEOUT))
        terminate();
}

bool AVThread::hasError() const
{
    return false;
}
bool AVThread::hasDecoderError() const
{
    return false;
}

void AVThread::terminate()
{
    disconnect(this, SIGNAL(finished()), this, SLOT(deleteLater()));
    QThread::terminate();
    wait(1000);
    emit QMPlay2Core.statusBarMessage(tr("A/V thread has been incorrectly terminated!"), 2000);
}
