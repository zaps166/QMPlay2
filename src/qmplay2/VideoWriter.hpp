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

#include <Frame.hpp>
#include <Writer.hpp>

class HWDecContext;
class QMPlay2OSD;

class QMPLAY2SHAREDLIB_EXPORT VideoWriter : public Writer
{
public:
    VideoWriter();
    virtual ~VideoWriter();

    virtual AVPixelFormats supportedPixelFormats() const;

    qint64 write(const QByteArray &) override final;

    virtual void writeVideo(const Frame &videoFrame) = 0;
    virtual void writeOSD(const QList<const QMPlay2OSD *> &osd) = 0;

    virtual bool setHWDecContext(const std::shared_ptr<HWDecContext> &hwDecContext);
    virtual std::shared_ptr<HWDecContext> hwDecContext() const;

    virtual bool open() override = 0;
};
