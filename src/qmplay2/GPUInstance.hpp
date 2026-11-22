// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QMPlay2Core.hpp>

class HWDecContext;
class VideoWriter;

class QMPLAY2SHAREDLIB_EXPORT GPUInstance
{
public:
    static std::shared_ptr<GPUInstance> create();

public:
    virtual ~GPUInstance() = default;

    virtual void prepareDestroy();

    virtual QString name() const = 0;
    virtual QMPlay2CoreClass::Renderer renderer() const = 0;

    virtual VideoWriter *createOrGetVideoOutput() = 0;

    virtual bool checkFiltersSupported() const;

    std::shared_ptr<HWDecContext> getHWDecContext() const;

    template<typename T>
    inline std::shared_ptr<T> getHWDecContext() const
    {
        return std::dynamic_pointer_cast<T>(getHWDecContext());
    }

    bool setHWDecContextForVideoOutput(const std::shared_ptr<HWDecContext> &hwDecContext);

    void clearVideoOutput();
    void resetVideoOutput();

protected:
    VideoWriter *m_videoWriter = nullptr;
};
