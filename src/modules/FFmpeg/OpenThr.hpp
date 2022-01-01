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
