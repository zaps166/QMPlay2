/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2019  Błażej Szczygieł

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

#include <VAAPIWriter.hpp>
#include <FFCommon.hpp>

#include <QMPlay2OSD.hpp>
#include <VideoFrame.hpp>
#include <Functions.hpp>
#include <ImgScaler.hpp>

#include <QCoreApplication>
#include <QPainter>
#include <QtMath>
#include <QDebug>

#include <va/va_x11.h>

VAAPIWriter::VAAPIWriter(Module &module, VAAPI *vaapi) :
    vaapi(vaapi)
{
    unsigned numSubpicFmts = vaMaxNumSubpictureFormats(vaapi->VADisp);
    VAImageFormat subpicFmtList[numSubpicFmts];
    unsigned subpicFlags[numSubpicFmts];
    if (vaQuerySubpictureFormats(vaapi->VADisp, subpicFmtList, subpicFlags, &numSubpicFmts) == VA_STATUS_SUCCESS)
    {
        for (unsigned i = 0; i < numSubpicFmts; ++i)
        {
            if (!qstrncmp((const char *)&subpicFmtList[i].fourcc, "BGRA", 4))
            {
                m_subpictDestIsScreenCoord = (subpicFlags[i] & VA_SUBPICTURE_DESTINATION_IS_SCREEN_COORD);
                memcpy(&m_rgbImgFmt, &subpicFmtList[i], sizeof(VAImageFormat));
                break;
            }
        }
    }

    m_vaImg.image_id = VA_INVALID_ID;

    setAttribute(Qt::WA_PaintOnScreen);
    grabGesture(Qt::PinchGesture);
    setMouseTracking(true);

    connect(&drawTim, &QTimer::timeout,
            this, [this] {
        draw();
    });
    drawTim.setSingleShot(true);

    SetModule(module);
}
VAAPIWriter::~VAAPIWriter()
{
    m_frames.clear(); // Must be freed before destroying vaapi
    clearVaImage();
    delete vaapi;
}

bool VAAPIWriter::set()
{
    return true;
}

bool VAAPIWriter::readyWrite() const
{
    return vaapi->ok;
}

bool VAAPIWriter::processParams(bool *)
{
    zoom = getParam("Zoom").toDouble();
    deinterlace = getParam("Deinterlace").toInt();
    aspect_ratio = getParam("AspectRatio").toDouble();

    const int _Hue = getParam("Hue").toInt();
    const int _Saturation = getParam("Saturation").toInt();
    const int _Brightness = getParam("Brightness").toInt();
    const int _Contrast = getParam("Contrast").toInt();
    if (_Hue != Hue || _Saturation != Saturation || _Brightness != Brightness || _Contrast != Contrast)
    {
        Hue = _Hue;
        Saturation = _Saturation;
        Brightness = _Brightness;
        Contrast = _Contrast;
        vaapi->applyVideoAdjustment(Brightness, Contrast, Saturation, Hue);
    }

    if (!isVisible())
        emit QMPlay2Core.dockVideo(this);
    else
    {
        resizeEvent(nullptr);
        if (!drawTim.isActive())
            drawTim.start(paused ? 1 : drawTimeout);
    }

    return readyWrite();
}
void VAAPIWriter::writeVideo(const VideoFrame &videoFrame)
{
    VASurfaceID id;
    int field = Functions::getField(videoFrame, deinterlace, 0, VA_TOP_FIELD, VA_BOTTOM_FIELD);
    if (vaapi->filterVideo(videoFrame, id, field))
    {
        m_frames.remove(this->m_id);
        if (videoFrame.surfaceId == id)
            m_frames.insert(id, videoFrame);
        draw(id, field);
    }
    paused = false;
}
void VAAPIWriter::writeOSD(const QList<const QMPlay2OSD *> &osds)
{
    if (m_rgbImgFmt.fourcc)
    {
        osd_mutex.lock();
        osd_list = osds;
        osd_mutex.unlock();
    }
}
void VAAPIWriter::pause()
{
    paused = true;
}

bool VAAPIWriter::hwAccelGetImg(const VideoFrame &videoFrame, void *dest, ImgScaler *nv12ToRGB32) const
{
    return vaapi->getImage(videoFrame, dest, nv12ToRGB32);
}

QString VAAPIWriter::name() const
{
    return VAAPIWriterName;
}

bool VAAPIWriter::open()
{
    addParam("Zoom");
    addParam("AspectRatio");
    addParam("Deinterlace");
    addParam("PrepareForHWBobDeint", true);
    addParam("Hue");
    addParam("Saturation");
    addParam("Brightness");
    addParam("Contrast");
    return true;
}

void VAAPIWriter::draw(VASurfaceID id, int field)
{
    if (id != VA_INVALID_SURFACE && field > -1)
    {
        if (m_id != id || field == m_field)
            vaSyncSurface(vaapi->VADisp, id);
        m_id = id;
        m_field = field;
    }
    if (m_id == VA_INVALID_SURFACE || visibleRegion().isNull())
        return;

    bool associated = false;

    osd_mutex.lock();
    if (!osd_list.isEmpty())
    {
        const auto scaleW = (qreal)W / vaapi->outW;
        const auto scaleH = (qreal)H / vaapi->outH;

        QRect bounds;
        bool mustRepaint = Functions::mustRepaintOSD(osd_list, osd_ids, &scaleW, &scaleH, &bounds);

        const QSize newImgSize = m_subpictDestIsScreenCoord
            ? bounds.size()
            : QSize(qCeil(bounds.width() / scaleW), qCeil(bounds.height() / scaleH));

        if (!mustRepaint)
            mustRepaint = (QSize(m_vaImg.width, m_vaImg.height) != newImgSize);

        bool canAssociate = !mustRepaint;

        if (mustRepaint)
        {
            clearVaImage();
            if (vaCreateImage(vaapi->VADisp, &m_rgbImgFmt, newImgSize.width(), newImgSize.height(), &m_vaImg) == VA_STATUS_SUCCESS)
            {
                if (vaCreateSubpicture(vaapi->VADisp, m_vaImg.image_id, &m_vaSubpicID) != VA_STATUS_SUCCESS)
                    clearVaImage();
            }
            if (m_vaSubpicID != VA_INVALID_ID)
            {
                quint8 *buff = nullptr;
                if (vaMapBuffer(vaapi->VADisp, m_vaImg.buf, (void **)&buff) == VA_STATUS_SUCCESS)
                {
                    QImage osdImg(buff + m_vaImg.offsets[0], m_vaImg.width, m_vaImg.height, m_vaImg.pitches[0], QImage::Format_ARGB32);
                    osdImg.fill(0);
                    QPainter p(&osdImg);
                    if (!m_subpictDestIsScreenCoord)
                    {
                        p.scale(1.0 / scaleW, 1.0 / scaleH);
                        p.setRenderHint(QPainter::SmoothPixmapTransform);
                    }
                    p.translate(-bounds.topLeft());
                    Functions::paintOSD(true, osd_list, scaleW, scaleH, p, &osd_ids);
                    vaUnmapBuffer(vaapi->VADisp, m_vaImg.buf);
                    canAssociate = true;
                }
            }
        }
        if (m_vaSubpicID != VA_INVALID_ID && canAssociate)
        {
            if (m_subpictDestIsScreenCoord)
            {
                associated = vaAssociateSubpicture(
                    vaapi->VADisp,
                    m_vaSubpicID,
                    &m_id,
                    1,

                    0,
                    0,
                    bounds.width(),
                    bounds.height(),

                    bounds.x() + X,
                    bounds.y() + Y,
                    bounds.width(),
                    bounds.height(),

                    VA_SUBPICTURE_DESTINATION_IS_SCREEN_COORD
                ) == VA_STATUS_SUCCESS;
            }
            else
            {
                // 05.04.2019: AMDGPU driver doesn't allow to change image size in dest rect. Driver crashes
                // or paints garbage. As a workaround, scale image to fit the surface size (lower quality).
                const double sW = (double)srcQRect.width()  / dstQRect.width();
                const double sH = (double)srcQRect.height() / dstQRect.height();
                const qreal dpr = devicePixelRatioF();
                const int Xoffset = (dstQRect.width()  == int(width()  * dpr)) ? X : 0;
                const int Yoffset = (dstQRect.height() == int(height() * dpr)) ? Y : 0;
                associated = vaAssociateSubpicture(
                    vaapi->VADisp,
                    m_vaSubpicID,
                    &m_id,
                    1,

                    0,
                    0,
                    m_vaImg.width,
                    m_vaImg.height,

                    (bounds.x() + Xoffset) * sW,
                    (bounds.y() + Yoffset) * sH,
                    m_vaImg.width,
                    m_vaImg.height,

                    0
                ) == VA_STATUS_SUCCESS;
            }
        }
    }
    osd_mutex.unlock();

    const int err = vaPutSurface(
        vaapi->VADisp,
        m_id,
        winId(),

        srcQRect.x(),
        srcQRect.y(),
        srcQRect.width(),
        srcQRect.height(),

        dstQRect.x(),
        dstQRect.y(),
        dstQRect.width(),
        dstQRect.height(),

        nullptr,
        0,
        m_field | VA_CLEAR_DRAWABLE
    );
    if (err != VA_STATUS_SUCCESS)
        QMPlay2Core.log(QString("vaPutSurface() - ") + vaErrorStr(err));

    if (associated)
        vaDeassociateSubpicture(vaapi->VADisp, m_vaSubpicID, &m_id, 1);

    if (drawTim.isActive())
        drawTim.stop();
}

void VAAPIWriter::resizeEvent(QResizeEvent *)
{
    const qreal dpr = devicePixelRatioF();
    Functions::getImageSize(aspect_ratio, zoom, width() * dpr, height() * dpr, W, H, &X, &Y, &dstQRect, &vaapi->outW, &vaapi->outH, &srcQRect);
}
void VAAPIWriter::paintEvent(QPaintEvent *)
{
    if (!drawTim.isActive())
        drawTim.start(paused ? 1 : drawTimeout);
}
bool VAAPIWriter::event(QEvent *e)
{
    /* Pass gesture and touch event to the parent */
    switch (e->type())
    {
        case QEvent::TouchBegin:
        case QEvent::TouchUpdate:
        case QEvent::TouchEnd:
        case QEvent::Gesture:
            return QCoreApplication::sendEvent(parent(), e);
        default:
            return QWidget::event(e);
    }
}

QPaintEngine *VAAPIWriter::paintEngine() const
{
    return nullptr;
}

void VAAPIWriter::clearVaImage()
{
    if (m_vaSubpicID != VA_INVALID_ID)
    {
        vaDestroySubpicture(vaapi->VADisp, m_vaSubpicID);
        m_vaSubpicID = VA_INVALID_ID;
    }
    if (m_vaImg.image_id != VA_INVALID_ID)
    {
        vaDestroyImage(vaapi->VADisp, m_vaImg.image_id);
        m_vaImg.width = m_vaImg.height = 0;
        m_vaImg.image_id = VA_INVALID_ID;
    }
}
