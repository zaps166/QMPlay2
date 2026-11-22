// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QCoreApplication>
#include <QStringList>
#include <QThread>
#include <QMutex>

#include <atomic>

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

    virtual bool lock();
    virtual void unlock();

    inline bool isWaiting() const
    {
        return waiting;
    }

    virtual void stop(bool terminate = false);

    virtual bool hasError() const;
    virtual bool hasDecoderError() const;

    Decoder *dec = nullptr;
    Writer *writer = nullptr;
protected:
    AVThread(PlayClass &playC);
    virtual ~AVThread();

    void maybeStartThread();

    void terminate();

    PlayClass &playC;

    std::atomic_bool br = false, br2 = false;
    bool waiting = false;
    QRecursiveMutex mutex;
    QMutex updateMutex;
};
