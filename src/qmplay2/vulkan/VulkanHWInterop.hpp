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

#include <QMPlay2Lib.hpp>

#include <HWDecContext.hpp>

#include <vulkan/vulkan.hpp>

namespace QmVk {

using namespace std;

class CommandBuffer;

class QMPLAY2SHAREDLIB_EXPORT HWInterop : public HWDecContext
{
public:
    class SyncData
    {
    public:
        virtual ~SyncData() = default;
    };
    using SyncDataPtr = std::unique_ptr<SyncData>;

public:
    virtual void map(Frame &frame) = 0;
    virtual void clear() = 0;

    virtual SyncDataPtr sync(const std::vector<Frame> &frames, vk::SubmitInfo *submitInfo = nullptr) = 0;

protected:
    bool syncNow(vk::SubmitInfo &submitInfo);

private:
    shared_ptr<CommandBuffer> m_commandBuffer;
};

}
