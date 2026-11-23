// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "VulkanWriter.hpp"
#include "VulkanWindow.hpp"
#include "VulkanHWInterop.hpp"

#include <Functions.hpp>

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
    addParam("Gamma");
    addParam("ColorPrimaries");
    addParam("ColorTrc");
    addParam("Negative");

    set();
}
Writer::~Writer()
{
    m_window->deleteWidget();
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
        sets.getBool("Vulkan/NearestScaling"),
        sets.getBool("Vulkan/HQScaleDown"),
        sets.getBool("Vulkan/HQScaleUp"),
        sets.getBool("Vulkan/BypassCompositor"),
        sets.getBool("Vulkan/HDR"),
        sets.getBool("Vulkan/BT2020")
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

    const float gamma = Functions::sliderValueToGamma(getParam("Gamma").toInt());

    m_window->setParams(
        QSize(getParam("W").toInt(), getParam("H").toInt()),
        getParam("AspectRatio").toDouble(),
        getParam("Zoom").toDouble(),
        getParam("Spherical").toBool(),
        getParam("Flip").toInt(),
        getParam("Rotate90").toBool(),
        getParam("Brightness").toInt() / 100.0f,
        (getParam("Contrast").toInt() + 100.0f) / 100.0f,
        gamma,
        getParam("Hue").toInt() / 200.0f,
        (getParam("Saturation").toInt() + 100.0f) / 100.0f,
        sharpness,
        getParam("Negative").toBool(),
        static_cast<AVColorPrimaries>(getParam("ColorPrimaries").toInt()),
        static_cast<AVColorTransferCharacteristic>(getParam("ColorTrc").toInt())
    );

    return readyWrite();
}

AVPixelFormats Writer::supportedPixelFormats() const
{
    return m_window->supportedPixelFormats();
}

void Writer::writeVideo(const Frame &videoFrame, QMPlay2OSDList &&osdList)
{
    m_window->setFrame(videoFrame, std::move(osdList));
}

void Writer::pause()
{
}

QString Writer::name() const
{
    QString name = "Vulkan";

    QStringList additionalText;

    if (m_window->isHdr10St2084())
        additionalText += "HDR";

    if (m_window->isBt2020Linear())
        additionalText += QStringLiteral("BT.2020");

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
