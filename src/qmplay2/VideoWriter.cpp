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

#include <VideoWriter.hpp>

VideoWriter::VideoWriter()
{}
VideoWriter::~VideoWriter()
{}

AVPixelFormats VideoWriter::supportedPixelFormats() const
{
    return {AV_PIX_FMT_YUV420P};
}

qint64 VideoWriter::write(const QByteArray &)
{
    return -1;
}

bool VideoWriter::setHWDecContext(const std::shared_ptr<HWDecContext> &hwDecContext)
{
    Q_UNUSED(hwDecContext)
    return false;
}
std::shared_ptr<HWDecContext> VideoWriter::hwDecContext() const
{
    return nullptr;
}
