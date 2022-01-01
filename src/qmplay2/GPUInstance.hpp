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
