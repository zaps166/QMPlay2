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

#include <GPUInstance.hpp>
#include <Settings.hpp>

#ifdef USE_OPENGL
#   include <opengl/OpenGLInstance.hpp>
#endif
#include <VideoWriter.hpp>

#include <QDebug>

using namespace std;

shared_ptr<GPUInstance> GPUInstance::create()
{
    const auto renderer = QMPlay2Core.getSettings().getString("Renderer");

#ifdef USE_OPENGL
    if (renderer == "opengl")
    {
        auto glInstance = make_shared<OpenGLInstance>();
        if (!glInstance->init())
        {
            qWarning() << "OpenGL is unable to work with QMPlay2 on this platform";
            return nullptr;
        }
        return glInstance;
    }
#endif

    return nullptr;
}

shared_ptr<HWDecContext> GPUInstance::getHWDecContext() const
{
    if (auto videoWriter = getVideoOutput())
        return videoWriter->hwDecContext();
    return nullptr;
}

bool GPUInstance::setHWDecContextForVideoOutput(const shared_ptr<HWDecContext> &hwDecContext)
{
    return createOrGetVideoOutput()->setHWDecContext(hwDecContext);
}

void GPUInstance::resetVideoOutput()
{
    delete m_videoWriter;
    clearVideoOutput();
}
