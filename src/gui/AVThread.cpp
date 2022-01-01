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

#include <AVThread.hpp>

#include <Main.hpp>
#include <PlayClass.hpp>

#include <Writer.hpp>
#include <Decoder.hpp>

AVThread::AVThread(PlayClass &playC) :
    playC(playC),
    mutex(QMutex::Recursive)
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
    deleteLater();
}
