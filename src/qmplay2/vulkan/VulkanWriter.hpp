// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <VideoWriter.hpp>

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

    void writeVideo(const Frame &videoFrame, QMPlay2OSDList &&osdList) override;

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
