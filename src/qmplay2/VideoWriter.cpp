/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2019  Błażej Szczygieł

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

#include <HWAccelInterface.hpp>

VideoWriter *VideoWriter::createOpenGL2(HWAccelInterface *hwAccelInterface)
{
    for (Module *pluginInstance : QMPlay2Core.getPluginsInstance())
        for (const Module::Info &mod : pluginInstance->getModulesInfo())
            if (mod.type == Module::WRITER && mod.extensions.contains("video"))
            {
                VideoWriter *videoWriter = (VideoWriter *)pluginInstance->createInstance("OpenGL 2");
                if (videoWriter)
                {
                    if (hwAccelInterface)
                        videoWriter->setHWAccelInterface(hwAccelInterface);
                    if (!videoWriter->open())
                    {
                        delete videoWriter;
                        videoWriter = nullptr;
                    }
                    return videoWriter;
                }
            }
    delete hwAccelInterface;
    return nullptr;
}

VideoWriter::VideoWriter() :
    m_hwAccelInterface(nullptr)
{}
VideoWriter::~VideoWriter()
{
    delete m_hwAccelInterface;
}

QMPlay2PixelFormats VideoWriter::supportedPixelFormats() const
{
    return {QMPlay2PixelFormat::YUV420P};
}

bool VideoWriter::hwAccelError() const
{
    return false;
}

void VideoWriter::setHWAccelInterface(HWAccelInterface *hwAccelInterface)
{
    m_hwAccelInterface = hwAccelInterface;
}

qint64 VideoWriter::write(const QByteArray &)
{
    return -1;
}

bool VideoWriter::hwAccelGetImg(const VideoFrame &videoFrame, void *dest, ImgScaler *nv12ToRGB32) const
{
    if (m_hwAccelInterface)
        return m_hwAccelInterface->getImage(videoFrame, dest, nv12ToRGB32);
    return false;
}
