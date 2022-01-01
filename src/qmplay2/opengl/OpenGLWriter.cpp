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

#include "OpenGLWriter.hpp"

#include "OpenGLWindow.hpp"
#include "OpenGLWidget.hpp"
#include "OpenGLHWInterop.hpp"

#include <Frame.hpp>

#include <QGuiApplication>
#ifdef Q_OS_WIN
#   include <QSysInfo>
#endif

using namespace std;

OpenGLWriter::OpenGLWriter()
{
    addParam("W");
    addParam("H");
    addParam("AspectRatio");
    addParam("Zoom");
    addParam("Spherical");
    addParam("Flip");
    addParam("Rotate90");
    addParam("ResetOther");

    m_useRtt = QMPlay2Core.isGlOnWindow();
    if (m_useRtt)
    {
        //Don't use rtt when videoDock has native window
        const QWidget *videoDock = QMPlay2Core.getVideoDock();
        m_useRtt = !videoDock->internalWinId() || (videoDock == videoDock->window());
    }
    if (m_useRtt)
        m_drawable = new OpenGLWidget;
    else
        m_drawable = new OpenGLWindow;

    auto w = m_drawable->widget();
    w->grabGesture(Qt::PinchGesture);
    w->setMouseTracking(true);

    set();

#ifdef Q_OS_WIN
    if (!QMPlay2Core.isGlOnWindow() && QSysInfo::windowsVersion() >= QSysInfo::WV_6_0)
        m_drawable->setWindowsBypassCompositor(m_bypassCompositor);
#endif
}
OpenGLWriter::~OpenGLWriter()
{
    m_drawable->deleteMe();
}

bool OpenGLWriter::set()
{
    bool doReset = false;

    auto &sets = QMPlay2Core.getSettings();

    m_drawable->setVSync(sets.getBool("OpenGL/VSync"));

    const auto bypassCompositor = sets.getBool("OpenGL/BypassCompositor");
    if (m_bypassCompositor != bypassCompositor)
    {
        m_bypassCompositor = bypassCompositor;

        if (QGuiApplication::platformName() == "xcb")
        {
            m_drawable->setX11BypassCompositor(m_bypassCompositor);
        }
#ifdef Q_OS_WIN
        else if (!QMPlay2Core.isGlOnWindow() && QSysInfo::windowsVersion() >= QSysInfo::WV_6_0)
        {
            doReset = true;
        }
#endif
    }

    return !doReset;
}

bool OpenGLWriter::readyWrite() const
{
    return m_drawable->isOK;
}

bool OpenGLWriter::processParams(bool *)
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
    if (m_drawable->aRatioRef() != aspectRatio || m_drawable->zoomRef() != zoom || m_drawable->isSphericalView() != spherical || m_drawable->verticesIdx != verticesIdx || m_drawable->videoAdjustment != videoAdjustment)
    {
        m_drawable->zoomRef() = zoom;
        m_drawable->aRatioRef() = aspectRatio;
        m_drawable->verticesIdx = verticesIdx;
        m_drawable->videoAdjustment = videoAdjustment;
        m_drawable->setSphericalView(spherical);
        doResizeEvent = m_drawable->widget()->isVisible();
    }
    if (getParam("ResetOther").toBool())
    {
        m_drawable->resetOffsets();
        modParam("ResetOther", false);
        if (!doResizeEvent)
            doResizeEvent = m_drawable->widget()->isVisible();
    }

    const int outW = getParam("W").toInt();
    const int outH = getParam("H").toInt();
    if (outW != m_drawable->outW || outH != m_drawable->outH)
    {
        m_drawable->clearImg();
        if (outW > 0 && outH > 0)
        {
            m_drawable->outW = outW;
            m_drawable->outH = outH;
        }
        emit QMPlay2Core.dockVideo(m_drawable->widget());
    }

    if (doResizeEvent)
        m_drawable->newSize(true);
    else
        m_drawable->doReset = true;

    return readyWrite();
}

AVPixelFormats OpenGLWriter::supportedPixelFormats() const
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

void OpenGLWriter::writeVideo(const Frame &videoFrame)
{
    m_drawable->isPaused = false;
    m_drawable->videoFrame = videoFrame;
    if (m_drawable->m_limited != m_drawable->videoFrame.isLimited() || m_drawable->m_colorSpace != m_drawable->videoFrame.colorSpace())
    {
        m_drawable->m_limited = m_drawable->videoFrame.isLimited();
        m_drawable->m_colorSpace = m_drawable->videoFrame.colorSpace();
        m_drawable->doReset = true;
    }
    m_drawable->updateGL(m_drawable->isSphericalView());
}
void OpenGLWriter::writeOSD(const QList<const QMPlay2OSD *> &osds)
{
    QMutexLocker mL(&m_drawable->osdMutex);
    m_drawable->osdList = osds;
}

void OpenGLWriter::pause()
{
    m_drawable->isPaused = true;
}

QString OpenGLWriter::name() const
{
    const int glver = m_drawable->m_glInstance->glVer;
    QString glStr = glver ? QString("%1.%2").arg(glver / 10).arg(glver % 10) : "2";
    if (m_drawable->m_hwInterop)
        glStr += " " + m_drawable->m_hwInterop->name();
    if (m_useRtt)
        glStr += " (render-to-texture)";
#ifdef OPENGL_ES2
    return "OpenGL|ES " + glStr;
#else
    return "OpenGL " + glStr;
#endif
}

bool OpenGLWriter::open()
{
    initialize(m_drawable->m_hwInterop);
    return true;
}

bool OpenGLWriter::setHWDecContext(const shared_ptr<HWDecContext> &hwDecContext)
{
    auto hwInterop = dynamic_pointer_cast<OpenGLHWInterop>(hwDecContext);
    if (hwDecContext && !hwInterop)
        return false;

    initialize(hwInterop);
    return readyWrite();
}
shared_ptr<HWDecContext> OpenGLWriter::hwDecContext() const
{
    return m_drawable->m_hwInterop;
}

void OpenGLWriter::addAdditionalParam(const QString &key)
{
    m_additionalParams.insert(key);
    addParam(key);
}

void OpenGLWriter::initialize(const shared_ptr<OpenGLHWInterop> &hwInterop)
{
    for (auto &&additionalParam : qAsConst(m_additionalParams))
        removeParam(additionalParam);
    m_additionalParams.clear();

    m_drawable->initialize(hwInterop);
    if (!readyWrite())
        return;

    bool hasBrightness = false, hasContrast = false, hasSharpness = false;
    if (!m_drawable->videoAdjustmentKeys.isEmpty())
    {
        for (const QString &key : qAsConst(m_drawable->videoAdjustmentKeys))
        {
            if (key == "Brightness")
                hasBrightness = true;
            else if (key == "Contrast")
                hasContrast = true;
            else if (key == "Sharpness")
                hasSharpness = true;
            addAdditionalParam(key);
        }
    }
    else if (m_drawable->numPlanes > 1)
    {
        addAdditionalParam("Saturation");
        if (m_drawable->canUseHueSharpness)
            addAdditionalParam("Hue");
    }
    if (!hasBrightness)
        addAdditionalParam("Brightness");
    if (!hasContrast)
        addAdditionalParam("Contrast");
    if (!hasSharpness && m_drawable->canUseHueSharpness)
        addAdditionalParam("Sharpness");
}
