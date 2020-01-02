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

#include <OpenGL2Writer.hpp>

#include <OpenGL2Window.hpp>
#include <OpenGL2Widget.hpp>

#include <HWAccelInterface.hpp>
#include <Frame.hpp>

#include <QGuiApplication>
#ifdef Q_OS_WIN
#   include <QSysInfo>
#endif

OpenGL2Writer::OpenGL2Writer(Module &module)
    : drawable(nullptr)
    , allowPBO(true)
{
    addParam("W");
    addParam("H");
    addParam("AspectRatio");
    addParam("Zoom");
    addParam("Spherical");
    addParam("Flip");
    addParam("Rotate90");
    addParam("ResetOther");

    SetModule(module);
}
OpenGL2Writer::~OpenGL2Writer()
{
    if (drawable)
        drawable->deleteMe();
}

bool OpenGL2Writer::set()
{
    bool doReset = false;

    const bool newAllowPBO = sets().getBool("AllowPBO");
    if (newAllowPBO != allowPBO)
    {
        allowPBO = newAllowPBO;
        doReset = true;
    }

    const bool newHqScaling = sets().getBool("HQScaling");
    if (newHqScaling != m_hqScaling)
    {
        m_hqScaling = newHqScaling;
        doReset = true;
    }

    vSync = sets().getBool("VSync");
    if (drawable)
        drawable->setVSync(vSync);

    const auto bypassCompositor = sets().getBool("BypassCompositor");
    if (m_bypassCompositor != bypassCompositor)
    {
        m_bypassCompositor = bypassCompositor;

        if (QGuiApplication::platformName() == "xcb")
        {
            if (drawable)
                drawable->setX11BypassCompositor(m_bypassCompositor);
        }
#ifdef Q_OS_WIN
        else if (!QMPlay2Core.isGlOnWindow() && QSysInfo::windowsVersion() >= QSysInfo::WV_6_0)
        {
            doReset = true;
        }
#endif
    }

    return !doReset && sets().getBool("Enabled");
}

bool OpenGL2Writer::readyWrite() const
{
    return drawable->isOK;
}

bool OpenGL2Writer::hwAccelError() const
{
    return drawable->hwAccelError;
}

bool OpenGL2Writer::processParams(bool *)
{
    bool doResizeEvent = false;

    const double aspectRatio = getParam("AspectRatio").toDouble();
    const double zoom = getParam("Zoom").toDouble();
    const bool spherical = getParam("Spherical").toBool();
    const int flip = getParam("Flip").toInt();
    const bool rotate90 = getParam("Rotate90").toBool();

    const VideoAdjustment videoAdjustment = {
        (qint16)getParam("Brightness").toInt(),
        (qint16)getParam("Contrast").toInt(),
        (qint16)getParam("Saturation").toInt(),
        (qint16)getParam("Hue").toInt(),
        (qint16)getParam("Sharpness").toInt()
    };

    const int verticesIdx = rotate90 * 4 + flip;
    if (drawable->aspectRatio != aspectRatio || drawable->zoom != zoom || drawable->sphericalView != spherical || drawable->verticesIdx != verticesIdx || drawable->videoAdjustment != videoAdjustment)
    {
        drawable->zoom = zoom;
        drawable->aspectRatio = aspectRatio;
        drawable->verticesIdx = verticesIdx;
        drawable->videoAdjustment = videoAdjustment;
        drawable->setSpherical(spherical);
        doResizeEvent = drawable->widget()->isVisible();
    }
    if (getParam("ResetOther").toBool())
    {
        drawable->videoOffset = drawable->osdOffset = QPointF();
        modParam("ResetOther", false);
        if (!doResizeEvent)
            doResizeEvent = drawable->widget()->isVisible();
    }

    const int outW = getParam("W").toInt();
    const int outH = getParam("H").toInt();
    if (outW != drawable->outW || outH != drawable->outH)
    {
        drawable->clearImg();
        if (outW > 0 && outH > 0)
        {
            drawable->outW = outW;
            drawable->outH = outH;
        }
        emit QMPlay2Core.dockVideo(drawable->widget());
    }

    if (doResizeEvent)
        drawable->newSize();
    else
        drawable->doReset = true;

    return readyWrite();
}

AVPixelFormats OpenGL2Writer::supportedPixelFormats() const
{
    return {
        AV_PIX_FMT_YUV420P,
        AV_PIX_FMT_YUVJ420P,
        AV_PIX_FMT_YUV422P,
        AV_PIX_FMT_YUVJ422P,
        AV_PIX_FMT_YUV444P,
        AV_PIX_FMT_YUVJ444P,
        AV_PIX_FMT_YUV410P,
        AV_PIX_FMT_YUV411P,
        AV_PIX_FMT_YUVJ411P,
        AV_PIX_FMT_YUV440P,
        AV_PIX_FMT_YUVJ440P,
    };
}

void OpenGL2Writer::writeVideo(const Frame &videoFrame)
{
    drawable->isPaused = false;
    drawable->videoFrame = videoFrame;
    if (drawable->m_limited != drawable->videoFrame.isLimited() || drawable->m_colorSpace != drawable->videoFrame.colorSpace())
    {
        drawable->m_limited = drawable->videoFrame.isLimited();
        drawable->m_colorSpace = drawable->videoFrame.colorSpace();
        drawable->doReset = true;
    }
    drawable->updateGL(drawable->sphericalView);
}
void OpenGL2Writer::writeOSD(const QList<const QMPlay2OSD *> &osds)
{
    QMutexLocker mL(&drawable->osdMutex);
    drawable->osdList = osds;
}

void OpenGL2Writer::pause()
{
    drawable->isPaused = true;
}

QString OpenGL2Writer::name() const
{
    QString glStr = drawable->glVer ? QString("%1.%2").arg(drawable->glVer / 10).arg(drawable->glVer % 10) : "2";
    if (drawable->hwAccellnterface)
        glStr += " " + drawable->hwAccellnterface->name();
    if (useRtt)
        glStr += " (render-to-texture)";
#ifdef OPENGL_ES2
    return "OpenGL|ES " + glStr;
#else
    return "OpenGL " + glStr;
#endif
}

bool OpenGL2Writer::open()
{
    useRtt = QMPlay2Core.isGlOnWindow();
    if (useRtt)
    {
        //Don't use rtt when videoDock has native window
        const QWidget *videoDock = QMPlay2Core.getVideoDock();
        useRtt = !videoDock->internalWinId() || (videoDock == videoDock->window());
    }
    if (useRtt)
        drawable = new OpenGL2Widget;
    else
        drawable = new OpenGL2Window;
    drawable->hwAccellnterface = m_hwAccelInterface;
    drawable->allowPBO = allowPBO;
    drawable->hqScaling = m_hqScaling;
    if (drawable->testGL())
    {
        drawable->setVSync(vSync);

        if (QGuiApplication::platformName() == "xcb")
            drawable->setX11BypassCompositor(m_bypassCompositor);
#ifdef Q_OS_WIN
        else if (!QMPlay2Core.isGlOnWindow() && QSysInfo::windowsVersion() >= QSysInfo::WV_6_0)
            drawable->setWindowsBypassCompositor(m_bypassCompositor);
#endif

        bool hasBrightness = false, hasContrast = false, hasSharpness = false;
        if (!drawable->videoAdjustmentKeys.isEmpty())
        {
            for (const QString &key : asConst(drawable->videoAdjustmentKeys))
            {
                if (key == "Brightness")
                    hasBrightness = true;
                else if (key == "Contrast")
                    hasContrast = true;
                else if (key == "Sharpness")
                    hasSharpness = true;
                addParam(key);
            }
        }
        else if (drawable->numPlanes > 1)
        {
            addParam("Saturation");
            if (drawable->canUseHueSharpness)
                addParam("Hue");
        }
        if (!hasBrightness)
            addParam("Brightness");
        if (!hasContrast)
            addParam("Contrast");
        if (!hasSharpness && drawable->canUseHueSharpness)
            addParam("Sharpness");
        return true;
    }
    return false;
}
