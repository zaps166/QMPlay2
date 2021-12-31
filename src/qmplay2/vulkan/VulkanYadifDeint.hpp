/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2021  Błażej Szczygieł

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

#include <VideoFilter.hpp>

namespace QmVk {

using namespace std;

class Instance;
class Sampler;
class ComputePipeline;

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
        shared_ptr<ComputePipeline> computes[3];
        shared_ptr<CommandBuffer> commandBuffer;
    } m;
};

}
