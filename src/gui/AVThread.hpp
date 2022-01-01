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

#include <QCoreApplication>
#include <QStringList>
#include <QThread>
#include <QMutex>

class PlayClass;
class Decoder;
class Writer;

class AVThread : public QThread
{
    Q_DECLARE_TR_FUNCTIONS(AVThread)

public:
    virtual void setDec(Decoder *_dec);
    void destroyDec();

    inline bool updateTryLock()
    {
        return updateMutex.tryLock();
    }
    inline void updateUnlock()
    {
        updateMutex.unlock();
    }

    bool lock();
    void unlock();

    inline bool isWaiting() const
    {
        return waiting;
    }

    virtual void stop(bool terminate = false);

    virtual bool hasDecoderError() const;

    Decoder *dec = nullptr;
    Writer *writer = nullptr;
protected:
    AVThread(PlayClass &playC);
    virtual ~AVThread();

    void maybeStartThread();

    void terminate();

    PlayClass &playC;

    volatile bool br = false, br2 = false;
    bool waiting = false;
    QMutex mutex, updateMutex;
};
