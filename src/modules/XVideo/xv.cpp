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

#include <xv.hpp>

#include <QMPlay2OSD.hpp>
#include <Frame.hpp>
#include <Functions.hpp>

#include <QPainter>

#include <cmath>

#include <sys/shm.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xvlib.h>

struct XVideoPrivate
{
    XvImageFormatValues *fo;
    XvAdaptorInfo *ai;
    Display *display;
    XvImage *image;
    XvPortID port;
    GC gc;
    XShmSegmentInfo shmInfo;
    QImage osdImg;
};
#define fo priv->fo
#define ai priv->ai
#define disp priv->display
#define image priv->image
#define port priv->port
#define gc priv->gc
#define shmInfo priv->shmInfo
#define osdImg priv->osdImg

XVIDEO::XVIDEO() :
    _isOK(false),
    _flip(0),
    priv(new XVideoPrivate)
{
    _flip = 0;
    ai = nullptr;
    clrVars();
    invalidateShm();
    _isOK = false;
    disp = XOpenDisplay(nullptr);
    if (!disp || XvQueryAdaptors(disp, DefaultRootWindow(disp), &adaptors, &ai) != Success)
        return;
    if (adaptors < 1)
        return;
    _isOK = true;
}
XVIDEO::~XVIDEO()
{
    close();
    if (ai)
        XvFreeAdaptorInfo(ai);
    if (disp)
        XCloseDisplay(disp);
    delete priv;
}

void XVIDEO::open(int W, int H, unsigned long _handle, const QString &adaptorName, bool useSHM)
{
    if (isOpen())
        close();

    width = W;
    height = H;
    handle = _handle;

    for (uint i = 0; i < adaptors; i++)
    {
        if ((ai[i].type & (XvInputMask | XvImageMask)) == (XvInputMask | XvImageMask))
        {
            if (!adaptorName.isEmpty() && adaptorName != ai[i].name)
                continue;
            for (XvPortID _p = ai[i].base_id; _p < ai[i].base_id + ai[i].num_ports; _p++)
            {
                if (!XvGrabPort(disp, _p, CurrentTime))
                {
                    port = _p;
                    break;
                }
            }
        }
        if (port)
            break;
    }
    if (!port)
        return close();

    int formats;
    fo = XvListImageFormats(disp, port, &formats);
    if (!fo)
        return close();

    int format_id = 0;
    for (int i = 0; i < formats; i++)
    {
        if (!qstrncmp(fo[i].guid, "YV12", 4))
        {
            format_id = fo[i].id;
            break;
        };
    };
    if (!format_id)
        return close();

    gc = XCreateGC(disp, handle, 0L, nullptr);
    if (!gc)
        return close();

    if (useSHM)
    {
        if (!XShmQueryExtension(disp))
            useSHM = false;
        else
        {
            image = XvShmCreateImage(disp, port, format_id, nullptr, width, height, &shmInfo);
            if (!image)
                useSHM = false;
            else
            {
                shmInfo.shmid = shmget(IPC_PRIVATE, image->data_size, IPC_CREAT | 0600);
                if (shmInfo.shmid < 0)
                    useSHM = false;
                else
                {
                    shmInfo.shmaddr = (char *)shmat(shmInfo.shmid, 0, 0);
                    if (shmInfo.shmaddr == (char *)-1)
                    {
                        shmInfo.shmaddr = nullptr;
                        useSHM = false;
                    }
                    else
                    {
                        const bool attached = XShmAttach(disp, &shmInfo);
                        XSync(disp, false);
                        if (!attached)
                            useSHM = false;
                        else
                            image->data = shmInfo.shmaddr;
                    }
                }
                if (!useSHM)
                    freeImage();
            }
        }
    }

    if (!useSHM)
    {
        image = XvCreateImage(disp, port, format_id, nullptr, width, height);
        if (image)
            image->data = new char[image->data_size];
    }

    if (!image)
        return close();

    osdImg = QImage(image->width, image->height, QImage::Format_ARGB32);
    osdImg.fill(0);

    _isOpen = true;
}

void XVIDEO::freeImage()
{
    if (shmInfo.shmid < 0)
        delete[] image->data;
    else
    {
        XShmDetach(disp, &shmInfo);
        shmctl(shmInfo.shmid, IPC_RMID, 0);
        if (shmInfo.shmaddr)
            shmdt(shmInfo.shmaddr);
        invalidateShm();
    }
    XFree(image);
}
void XVIDEO::invalidateShm()
{
    shmInfo.shmseg = 0;
    shmInfo.shmid = -1;
    shmInfo.shmaddr = nullptr;
    shmInfo.readOnly = false;
}
void XVIDEO::close()
{
    if (image)
        freeImage();
    if (gc)
        XFreeGC(disp, gc);
    if (port)
        XvUngrabPort(disp, port, CurrentTime);
    if (fo)
        XFree(fo);
    clrVars();
}

void XVIDEO::draw(const Frame &videoFrame, const QRect &srcRect, const QRect &dstRect, int W, int H, const QList<const QMPlay2OSD *> &osd_list, QMutex &osd_mutex)
{
    videoFrame.copyYV12(image->data, image->pitches[0], image->pitches[1]);

    if (_flip & Qt::Horizontal)
        Functions::hFlip((quint8 *)image->data, image->pitches[0], image->height, width);
    if (_flip & Qt::Vertical)
        Functions::vFlip((quint8 *)image->data, image->pitches[0], image->height);

    osd_mutex.lock();
    if (!osd_list.isEmpty())
        Functions::paintOSDtoYV12((quint8 *)image->data, osdImg, W, H, image->pitches[0], image->pitches[1], osd_list, osd_ids);
    osd_mutex.unlock();

    putImage(srcRect, dstRect);
    hasImage = true;
}
void XVIDEO::redraw(const QRect &srcRect, const QRect &dstRect, int X, int Y, int W, int H, int winW, int winH)
{
    if (isOpen())
    {
        if (Y > 0)
        {
            XFillRectangle(disp, handle, gc, 0, 0, winW, Y);
            XFillRectangle(disp, handle, gc, 0, Y + H, winW, Y+1);
        }
        if (X > 0)
        {
            XFillRectangle(disp, handle, gc, 0, 0, X, winH);
            XFillRectangle(disp, handle, gc, X + W, 0, X+1, winH);
        }
        if (hasImage)
            putImage(srcRect, dstRect);
    }
}

void XVIDEO::setVideoEqualizer(int h, int s, int b, int c)
{
    if (isOpen())
    {
        int attrib_count;
        XvAttribute *attributes = XvQueryPortAttributes(disp, port, &attrib_count);
        if (attributes)
        {
            XvSetPortAttributeIfExists(attributes, attrib_count, "XV_HUE", h);
            XvSetPortAttributeIfExists(attributes, attrib_count, "XV_SATURATION", s);
            XvSetPortAttributeIfExists(attributes, attrib_count, "XV_BRIGHTNESS", b);
            XvSetPortAttributeIfExists(attributes, attrib_count, "XV_CONTRAST", c);
            XFree(attributes);
        }
    }
}
void XVIDEO::setFlip(int f)
{
    if (isOpen() && hasImage)
    {
        if ((f & Qt::Horizontal) != (_flip & Qt::Horizontal))
            Functions::hFlip((quint8 *)image->data, image->pitches[0], height, width);
        if ((f & Qt::Vertical) != (_flip & Qt::Vertical))
            Functions::vFlip((quint8 *)image->data, image->pitches[0], height);
    }
    _flip = f;
}

QStringList XVIDEO::adaptorsList()
{
    QStringList _adaptorsList;
    XVIDEO *xv = new XVIDEO;
    if  (xv->isOK())
    {
        for (unsigned i = 0; i < xv->adaptors; i++)
            if ((xv->ai[i].type & (XvInputMask | XvImageMask)) == (XvInputMask | XvImageMask))
                _adaptorsList += xv->ai[i].name;
    }
    delete xv;
    return _adaptorsList;
}

void XVIDEO::putImage(const QRect &srcRect, const QRect &dstRect)
{
    if (shmInfo.shmaddr)
        XvShmPutImage(disp, port, handle, gc, image, srcRect.x(), srcRect.y(), srcRect.width(), srcRect.height(), dstRect.x(), dstRect.y(), dstRect.width(), dstRect.height(), false);
    else
        XvPutImage(disp, port, handle, gc, image, srcRect.x(), srcRect.y(), srcRect.width(), srcRect.height(), dstRect.x(), dstRect.y(), dstRect.width(), dstRect.height());
    XSync(disp, false);
}

void XVIDEO::XvSetPortAttributeIfExists(void *attributes, int attrib_count, const char *k, int v)
{
    for (int i = 0; i < attrib_count; ++i)
    {
        const XvAttribute &attribute = ((XvAttribute *)attributes)[i];
        if (!qstrcmp(attribute.name, k) && (attribute.flags & XvSettable))
        {
            XvSetPortAttribute(disp, port, XInternAtom(disp, k, false), Functions::scaleEQValue(v, attribute.min_value, attribute.max_value));
            break;
        }
    }
}
void XVIDEO::clrVars()
{
    image = nullptr;
    gc = nullptr;
    port = 0;
    _isOpen = false;
    width = 0;
    height = 0;
    handle = 0;
    hasImage = false;
    fo = nullptr;
    osdImg = QImage();
    osd_ids.clear();
}
