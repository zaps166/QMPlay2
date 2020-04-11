/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2020  Błażej Szczygieł

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

#include <CuvidHWInterop.hpp>
#include <vulkan/VulkanHWInterop.hpp>

#include <atomic>

namespace QmVk {
class ImagePool;
class Image;
}

class CuvidVulkan final : public CuvidHWInterop, public QmVk::HWInterop
{
public:
    CuvidVulkan(const std::shared_ptr<CUcontext> &cuCtx);
    ~CuvidVulkan();

    QString name() const override;

    void map(Frame &frame) override;
    void clear() override;

private:
    const std::shared_ptr<QmVk::ImagePool> m_vkImagePool;
    CUstream m_cuStream = nullptr;
};
