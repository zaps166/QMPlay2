// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QMPlay2Headers.hpp>

#include <VideoFilter.hpp>

namespace QmVk {

using namespace std;

class Instance;

class QMPLAY2SHAREDLIB_EXPORT YadifDeint : public VideoFilter
{
public:
    YadifDeint(const shared_ptr<HWInterop> &hwInterop);
    ~YadifDeint();

    bool filter(QQueue<Frame> &framesQueue) override;

    bool processParams(bool *paramsCorrected) override;

private:
    bool ensureResources();

private:
    const bool m_spatialCheck;

    bool m_error = false;

    const shared_ptr<Instance> m_instance;

    struct
    {
        shared_ptr<Device> device;
        shared_ptr<Sampler> sampler;
        shared_ptr<ComputePipeline> computes[3][2];
        shared_ptr<CommandBuffer> commandBuffer;
        shared_ptr<function<void()>> filtersOnOtherQueueFamiliy;
    } m;
};

}
