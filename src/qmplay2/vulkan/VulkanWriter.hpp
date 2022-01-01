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

#include <VideoWriter.hpp>

#include <QCoreApplication>

namespace QmVk {

using namespace std;

class HWInterop;
class Window;

class Writer : public VideoWriter
{
    Q_DECLARE_TR_FUNCTIONS(VulkanWriter)

public:
    Writer();
    ~Writer();

    bool set() override;

    bool readyWrite() const override;

    bool processParams(bool *paramsCorrected) override;

    AVPixelFormats supportedPixelFormats() const override;

    void writeVideo(const Frame &videoFrame) override;
    void writeOSD(const QList<const QMPlay2OSD *> &osd) override;

    void pause() override;

    QString name() const override;

    bool open() override;

    bool setHWDecContext(const shared_ptr<HWDecContext> &hwDecContext) override;
    shared_ptr<HWDecContext> hwDecContext() const override;

private:
    Window *const m_window;
    QByteArray m_lastDeviceID;
    bool m_forceVulkanYadif = false;
    shared_ptr<HWInterop> m_vkHwInterop;
};

}
