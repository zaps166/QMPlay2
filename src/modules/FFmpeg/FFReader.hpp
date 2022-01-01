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

#include <Reader.hpp>

#include <OpenThr.hpp>

struct AVIOContext;

class FFReader final : public Reader
{
public:
    FFReader();
private:
    bool readyRead() const override;
    bool canSeek() const override;

    bool seek(qint64) override;
    QByteArray read(qint64) override;
    void pause() override;
    bool atEnd() const override;
    void abort() override;

    qint64 size() const override;
    qint64 pos() const override;
    QString name() const override;

    bool open() override;

    /**/

    ~FFReader();

    AVIOContext *avioCtx;
    bool paused, canRead;
    std::shared_ptr<AbortContext> abortCtx;
};
