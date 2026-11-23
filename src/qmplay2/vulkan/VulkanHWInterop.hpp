// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QMPlay2Headers.hpp>

#include <HWDecContext.hpp>

namespace QmVk {

using namespace std;

class QMPLAY2SHAREDLIB_EXPORT HWInterop : public HWDecContext
{
public:
    class SyncData
    {
    public:
        virtual ~SyncData() = default;
    };
    using SyncDataPtr = unique_ptr<SyncData>;

public:
    virtual void map(Frame &frame) = 0;
    virtual void clear() = 0;

    virtual SyncDataPtr sync(const vector<Frame> &frames, vk::SubmitInfo *submitInfo = nullptr) = 0;

    virtual void updateInfo(const vector<Frame> &frames);

protected:
    bool syncNow(vk::SubmitInfo &submitInfo);

private:
    shared_ptr<CommandBuffer> m_commandBuffer;
};

}
