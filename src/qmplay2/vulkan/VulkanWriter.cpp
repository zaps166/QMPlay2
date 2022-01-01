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

#include "VulkanWriter.hpp"
#include "VulkanWindow.hpp"
#include "VulkanHWInterop.hpp"

#include <QDebug>

namespace QmVk {

Writer::Writer()
    : m_window(new Window(m_vkHwInterop))
{
    addParam("W");
    addParam("H");
    addParam("AspectRatio");
    addParam("Zoom");
    addParam("Spherical");
    addParam("Flip");
    addParam("Rotate90");
    addParam("ResetOther");
    addParam("Brightness");
    addParam("Contrast");
    addParam("Hue");
    addParam("Saturation");
    addParam("Sharpness");

    set();
}
Writer::~Writer()
{
    delete m_window->widget();
}

bool Writer::set()
{
    auto &sets = QMPlay2Core.getSettings();

    bool mustRestart = false;

    const auto deviceID = sets.getByteArray("Vulkan/Device");
    if (m_lastDeviceID != deviceID)
    {
        m_lastDeviceID = deviceID;
        mustRestart = true;
    }

    const auto forceVulkanYadif = sets.getBool("Vulkan/ForceVulkanYadif");
    if (m_forceVulkanYadif != forceVulkanYadif)
    {
        m_forceVulkanYadif = forceVulkanYadif;
        if (m_vkHwInterop)
            mustRestart = true;
    }

    m_window->setConfig(
        sets.getWithBounds("Vulkan/VSync", Qt::Unchecked, Qt::Checked),
        sets.getBool("Vulkan/HQScaleDown"),
        sets.getBool("Vulkan/HQScaleUp"),
        sets.getBool("Vulkan/BypassCompositor")
    );

    return !mustRestart;
}

bool Writer::readyWrite() const
{
    return !m_window->hasError();
}

bool Writer::processParams(bool *paramsCorrected)
{
    Q_UNUSED(paramsCorrected)

    auto widget = m_window->widget();
    if (!widget->isVisible())
        emit QMPlay2Core.dockVideo(widget);

    if (getParam("ResetOther").toBool())
    {
        m_window->resetOffsets();
        modParam("ResetOther", false);
    }

    float sharpness = getParam("Sharpness").toInt();
    if (sharpness < 0.0f)
        sharpness /= 80.0f;
    else if (sharpness > 0.0f)
        sharpness /= 40.0f;

    m_window->setParams(
        QSize(getParam("W").toInt(), getParam("H").toInt()),
        getParam("AspectRatio").toDouble(),
        getParam("Zoom").toDouble(),
        getParam("Spherical").toBool(),
        getParam("Flip").toInt(),
        getParam("Rotate90").toBool(),
        getParam("Brightness").toInt() / 100.0f,
        (getParam("Contrast").toInt() + 100.0f) / 100.0f,
        getParam("Hue").toInt() / 200.0f,
        (getParam("Saturation").toInt() + 100.0f) / 100.0f,
        sharpness
    );

    return readyWrite();
}

AVPixelFormats Writer::supportedPixelFormats() const
{
    return m_window->supportedPixelFormats();
}

void Writer::writeVideo(const Frame &videoFrame)
{
    m_window->setFrame(videoFrame);
}
void Writer::writeOSD(const QList<const QMPlay2OSD *> &osd)
{
    m_window->setOSD(osd);
}

void Writer::pause()
{
}

QString Writer::name() const
{
    QString name = "Vulkan";

    QStringList additionalText;

    if (m_window->isDeepColor())
        additionalText += "Deep color";

    if (m_vkHwInterop)
        additionalText += m_vkHwInterop->name();

    if (!additionalText.isEmpty())
    {
        name += " (";
        for (int i = 0; i < additionalText.count(); ++i)
        {
            if (i > 0)
                name += ", ";
            name += additionalText.at(i);
        }
        name += ")";
    }

    return name;
}

bool Writer::open()
{
    return true;
}

bool Writer::setHWDecContext(const shared_ptr<HWDecContext> &hwDecContext)
{
    auto hwInterop = dynamic_pointer_cast<HWInterop>(hwDecContext);
    if (hwDecContext && !hwInterop)
        return false;

    m_vkHwInterop = hwInterop;
    return true;
}
shared_ptr<HWDecContext> Writer::hwDecContext() const
{
    return m_vkHwInterop;
}

}
