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

#include <DirectDraw.hpp>

#include <QMPlay2OSD.hpp>
#include <Frame.hpp>
#include <Functions.hpp>

#include <QCoreApplication>
#include <QPainter>

#define DWM_EC_DISABLECOMPOSITION 0
#define DWM_EC_ENABLECOMPOSITION  1

#define ColorKEY 0x00000001

/**/

Drawable::Drawable(DirectDrawWriter &writer) :
    isOK(false), isOverlay(false), paused(false),
    writer(writer),
    flip(0),
    DDraw(NULL), DDClipper(NULL), DDSPrimary(NULL), DDSSecondary(NULL), DDSBackBuffer(NULL), DDrawColorCtrl(NULL),
    DwmEnableComposition(NULL)
{
    setMouseTracking(true);
    grabGesture(Qt::PinchGesture);
    if (DirectDrawCreate(NULL, &DDraw, NULL) == DD_OK && DDraw->SetCooperativeLevel(NULL, DDSCL_NORMAL) == DD_OK)
    {
        DDSURFACEDESC ddsd_primary = {sizeof ddsd_primary};
        ddsd_primary.dwFlags = DDSD_CAPS;
        ddsd_primary.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
        if (DDraw->CreateSurface(&ddsd_primary, &DDSPrimary, NULL) == DD_OK)
        {
            LPDIRECTDRAWSURFACE DDrawTestSurface;
            DDSURFACEDESC ddsd_test = {sizeof ddsd_test};
            ddsd_test.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
            ddsd_test.dwWidth  = 8;
            ddsd_test.dwHeight = 2;
            ddsd_test.ddpfPixelFormat.dwSize = sizeof ddsd_test.ddpfPixelFormat;
            ddsd_test.ddpfPixelFormat.dwFlags = DDPF_FOURCC;
            ddsd_test.ddpfPixelFormat.dwFourCC = MAKEFOURCC('Y', 'V', '1', '2');

            /* Overlay YV12 test */
            if (QSysInfo::windowsVersion() <= QSysInfo::WV_6_1) //Windows 8 and 10 can't disable DWM, so overlay won't work
            {
                DDCAPS ddCaps = {sizeof ddCaps};
                DDraw->GetCaps(&ddCaps, NULL);
                if (ddCaps.dwCaps & (DDCAPS_OVERLAY | DDCAPS_OVERLAYFOURCC | DDCAPS_OVERLAYSTRETCH))
                {
                    ddsd_test.ddsCaps.dwCaps = DDSCAPS_OVERLAY | DDSCAPS_VIDEOMEMORY;
                    if (DDraw->CreateSurface(&ddsd_test, &DDrawTestSurface, NULL) == DD_OK)
                    {
                        RECT destRect = {0, 0, 1, 1};
                        HRESULT res = DDrawTestSurface->UpdateOverlay(NULL, DDSPrimary, &destRect, DDOVER_SHOW, NULL);
                        if (res == DDERR_OUTOFCAPS && QSysInfo::windowsVersion() >= QSysInfo::WV_6_0)
                        {
                            /* Disable DWM to use overlay */
                            DwmEnableComposition = (DwmEnableCompositionProc)GetProcAddress(GetModuleHandleA("dwmapi.dll"), "DwmEnableComposition");
                            if (DwmEnableComposition)
                            {
                                if (DwmEnableComposition(DWM_EC_DISABLECOMPOSITION) == S_OK)
                                    res = DDrawTestSurface->UpdateOverlay(NULL, DDSPrimary, &destRect, DDOVER_SHOW, NULL);
                                else
                                    DwmEnableComposition = NULL;
                            }
                        }
                        if (res == DD_OK)
                        {
                            DDrawTestSurface->UpdateOverlay(NULL, DDSPrimary, NULL, DDOVER_HIDE, NULL);

                            setAutoFillBackground(true);
                            setPalette(QColor(ColorKEY));
                            connect(&QMPlay2Core, SIGNAL(videoDockMoved()), this, SLOT(updateOverlay()));
                            connect(&QMPlay2Core, SIGNAL(videoDockVisible(bool)), this, SLOT(overlayVisible(bool)));
                            connect(&visibleTim, SIGNAL(timeout()), this, SLOT(doOverlayVisible()));
                            visibleTim.setSingleShot(true);

                            isOK = isOverlay = true;
                        }
                        DDrawTestSurface->Release();
                    }
                }
            }

            /* YV12 test */
            if (!isOverlay)
            {
                ddsd_test.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY;
                if (DDraw->CreateSurface(&ddsd_test, &DDrawTestSurface, NULL) == DD_OK)
                {
                    setAttribute(Qt::WA_PaintOnScreen);
                    blackBrush = CreateSolidBrush(0x00000000);
                    if (DDraw->CreateClipper(0, &DDClipper, NULL) == DD_OK)
                        DDSPrimary->SetClipper(DDClipper);

                    isOK = true;

                    DDrawTestSurface->Release();
                }
            }
        }
    }
}
Drawable::~Drawable()
{
    releaseSecondary();
    if (DDSPrimary)
        DDSPrimary->Release();
    if (DDClipper)
        DDClipper->Release();
    if (DDraw)
        DDraw->Release();
    DeleteObject(blackBrush);
    if (DwmEnableComposition)
        DwmEnableComposition(DWM_EC_ENABLECOMPOSITION);
}

void Drawable::dock()
{
    emit QMPlay2Core.dockVideo(this);
    if (DDClipper)
        DDClipper->SetHWnd(0, (HWND)winId());
}
bool Drawable::createSecondary()
{
    releaseSecondary();

    DDSURFACEDESC ddsd = {sizeof ddsd};
    ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
    ddsd.dwWidth = writer.outW;
    ddsd.dwHeight = writer.outH;
    ddsd.ddsCaps.dwCaps = DDSCAPS_VIDEOMEMORY;
    if (!isOverlay)
        ddsd.ddsCaps.dwCaps |= DDSCAPS_OFFSCREENPLAIN;
    else
    {
        ddsd.ddsCaps.dwCaps |= DDSCAPS_OVERLAY | DDSCAPS_COMPLEX | DDSCAPS_FLIP;
        ddsd.dwFlags |= DDSD_BACKBUFFERCOUNT;
        ddsd.dwBackBufferCount = 1;
    }
    ddsd.ddpfPixelFormat.dwSize = sizeof ddsd.ddpfPixelFormat;
    ddsd.ddpfPixelFormat.dwFlags = DDPF_FOURCC;
    ddsd.ddpfPixelFormat.dwFourCC = MAKEFOURCC('Y', 'V', '1', '2');

    if (DDraw->CreateSurface(&ddsd, &DDSSecondary, NULL) == DD_OK)
    {
        if (isOverlay)
        {
            DDSCAPS ddsCaps = {sizeof ddsCaps};
            ddsCaps.dwCaps = DDSCAPS_BACKBUFFER;
            DDSSecondary->GetAttachedSurface(&ddsCaps, &DDSBackBuffer);
            DDSSecondary->QueryInterface(IID_IDirectDrawColorControl, (LPVOID *)&DDrawColorCtrl);
        }
        if (!DDSBackBuffer)
            DDSBackBuffer = DDSSecondary;
        if (DDSBackBuffer->Lock(NULL, &ddsd, DDLOCK_WAIT, NULL) == DD_OK) //Black on start
        {
            const DWORD size = ddsd.lPitch * ddsd.dwHeight;
            memset((BYTE *)ddsd.lpSurface, 0x00, size);
            memset((BYTE *)ddsd.lpSurface + size, 0x7F, size / 2);
            if (DDSBackBuffer->Unlock(NULL) == DD_OK)
            {
                if (isOverlay)
                    DDSSecondary->Flip(NULL, DDFLIP_WAIT);
                return true;
            }
        }
    }

    return false;
}
void Drawable::releaseSecondary()
{
    if (DDSSecondary)
    {
        if (DDrawColorCtrl)
        {
            DDrawColorCtrl->Release();
            DDrawColorCtrl = NULL;
        }
        if (isOverlay)
            DDSSecondary->UpdateOverlay(NULL, DDSPrimary, NULL, DDOVER_HIDE, NULL);
        DDSSecondary->Release();
        DDSSecondary = NULL;
        DDSBackBuffer = NULL;
    }
}
void Drawable::videoEqSet()
{
    if (DDrawColorCtrl)
    {
        DDCOLORCONTROL DDCC =
        {
            sizeof DDCC,
            DDCOLOR_HUE | DDCOLOR_SATURATION | DDCOLOR_BRIGHTNESS | DDCOLOR_CONTRAST,
            Functions::scaleEQValue(writer.Brightness, 0, 1500),
            Functions::scaleEQValue(writer.Contrast, 0, 20000),
            Functions::scaleEQValue(writer.Hue, -180, 180),
            Functions::scaleEQValue(writer.Saturation, 0, 20000)
        };
        DDrawColorCtrl->SetColorControls(&DDCC);
    }
}
void Drawable::setFlip()
{
    const bool doHFlip = (writer.flip & Qt::Horizontal) != (flip & Qt::Horizontal);
    const bool doVFlip = (writer.flip & Qt::Vertical) != (flip & Qt::Vertical);
    if (doHFlip || doVFlip)
    {
        DDSURFACEDESC ddsd = {sizeof ddsd};
        if (paused && !isOverlay && canDraw() && DDSBackBuffer->Lock(NULL, &ddsd, DDLOCK_WAIT | DDLOCK_NOSYSLOCK, NULL) == DD_OK)
        {
            if (doHFlip)
                Functions::hFlip((quint8 *)ddsd.lpSurface, ddsd.lPitch, ddsd.dwHeight, ddsd.dwWidth);
            if (doVFlip)
                Functions::vFlip((quint8 *)ddsd.lpSurface, ddsd.lPitch, ddsd.dwHeight);
            DDSBackBuffer->Unlock(NULL);
        }
        flip = writer.flip;
    }
}

void Drawable::draw(const Frame &videoFrame)
{
    DDSURFACEDESC ddsd = {sizeof ddsd};
    if (restoreLostSurface() && isOverlay)
        updateOverlay();
    if (DDSBackBuffer->Lock(NULL, &ddsd, DDLOCK_WAIT | DDLOCK_NOSYSLOCK, NULL) == DD_OK)
    {
        const int dataSize = (ddsd.lPitch * ddsd.dwHeight * 3) >> 1;
        BYTE *surface = (BYTE *)ddsd.lpSurface;

        osd_mutex.lock();
        BYTE *dest = ((flip && !isOverlay) || !osd_list.isEmpty()) ? new BYTE[dataSize] : surface;

        videoFrame.copyYV12(dest, ddsd.lPitch, ddsd.lPitch >> 1);
        if (!isOverlay)
        {
            if (flip & Qt::Horizontal)
                Functions::hFlip(dest, ddsd.lPitch, ddsd.dwHeight, ddsd.dwWidth);
            if (flip & Qt::Vertical)
                Functions::vFlip(dest, ddsd.lPitch, ddsd.dwHeight);
        }

        if (!osd_list.isEmpty())
        {
            if (osdImg.size() != QSize(ddsd.dwWidth, ddsd.dwHeight))
            {
                osdImg = QImage(ddsd.dwWidth, ddsd.dwHeight, QImage::Format_ARGB32);
                osdImg.fill(0);
            }
            Functions::paintOSDtoYV12(dest, osdImg, W, H, ddsd.lPitch, ddsd.lPitch >> 1, osd_list, osd_ids);
        }
        osd_mutex.unlock();

        if (surface != dest)
        {
            memcpy(surface, dest, dataSize);
            delete[] dest;
        }

        if (DDSBackBuffer->Unlock(NULL) == DD_OK)
        {
            if (isOverlay)
                DDSSecondary->Flip(NULL, DDFLIP_WAIT);
            else
            {
                DDraw->WaitForVerticalBlank(DDWAITVB_BLOCKBEGIN, NULL); //Sometimes it works :D
                blit();
            }
        }
    }
}

void Drawable::resizeEvent(QResizeEvent *e)
{
    const qreal dpr = devicePixelRatioF();
    Functions::getImageSize(writer.aspect_ratio, writer.zoom, width() * dpr, height() * dpr, W, H, &X, &Y);

    if (isOverlay)
        updateOverlay();
    else if (!e)
        update();

    if (e)
        QWidget::resizeEvent(e);
}

void Drawable::getRects(RECT &srcRect, RECT &dstRect)
{
    const QPoint point = mapToGlobal(QPoint()) * devicePixelRatioF() + QPoint(X, Y);
    const RECT videoRect = {point.x(), point.y(), point.x() + W + 1, point.y() + H + 1};
    const RECT screenRect = {0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN)};

    IntersectRect(&dstRect, &videoRect, &screenRect);

    if (videoRect.right - videoRect.left && videoRect.bottom - videoRect.top)
    {
        srcRect.left   = (dstRect.left - videoRect.left) * writer.outW / (videoRect.right - videoRect.left);
        srcRect.top    = (dstRect.top - videoRect.top)   * writer.outH / (videoRect.bottom - videoRect.top);
        srcRect.right  = writer.outW - (videoRect.right - dstRect.right  ) * writer.outW / (videoRect.right - videoRect.left);
        srcRect.bottom = writer.outH - (videoRect.bottom - dstRect.bottom) * writer.outH / (videoRect.bottom - videoRect.top);
    }
    else
        memset(&srcRect, 0, sizeof srcRect);
}
void Drawable::fillRects()
{
    const qreal dpr = devicePixelRatioF();
    HWND hwnd = (HWND)winId();
    HDC hdc = GetDC(hwnd);
    RECT rect;
    if (Y > 0)
    {
        SetRect(&rect, 0, 0, width() * dpr, Y);
        FillRect(hdc, &rect, blackBrush);
        SetRect(&rect, 0, Y + H, width() * dpr, 2 * Y + H + 1);
        FillRect(hdc, &rect, blackBrush);
    }
    if (X > 0)
    {
        SetRect(&rect, 0, 0, X, height() * dpr);
        FillRect(hdc, &rect, blackBrush);
        SetRect(&rect, X + W, 0, 2 * X + W + 1, height() * dpr);
        FillRect(hdc, &rect, blackBrush);
    }
    ReleaseDC(hwnd, hdc);
}

void Drawable::updateOverlay()
{
    if (canDraw())
    {
        RECT srcRect, dstRect;
        getRects(srcRect, dstRect);

        restoreLostSurface();

        DDOVERLAYFX ovfx = {sizeof ovfx};
        ovfx.dckDestColorkey.dwColorSpaceHighValue = ovfx.dckDestColorkey.dwColorSpaceLowValue = ColorKEY;
        if (flip & Qt::Horizontal)
            ovfx.dwDDFX |= DDOVERFX_MIRRORLEFTRIGHT;
        if (flip & Qt::Vertical)
            ovfx.dwDDFX |= DDOVERFX_MIRRORUPDOWN;
        DDSSecondary->UpdateOverlay(&srcRect, DDSPrimary, &dstRect, DDOVER_SHOW | DDOVER_KEYDESTOVERRIDE | DDOVER_DDFX, &ovfx);
    }
}
void Drawable::overlayVisible(bool v)
{
    const bool visible = v && (visibleRegion() != QRegion() || QMPlay2Core.getVideoDock()->visibleRegion() != QRegion());
    visibleTim.setProperty("overlayVisible", visible);
    visibleTim.start(1);
}
void Drawable::doOverlayVisible()
{
    if (visibleTim.property("overlayVisible").toBool())
        updateOverlay();
    else if (canDraw())
        DDSSecondary->UpdateOverlay(NULL, DDSPrimary, NULL, DDOVER_HIDE, NULL);
}
void Drawable::blit()
{
    RECT srcRect, dstRect;
    getRects(srcRect, dstRect);

    DDSPrimary->Blt(&dstRect, DDSSecondary, &srcRect, DDBLT_WAIT, NULL);
}

bool Drawable::restoreLostSurface()
{
    if (DDSPrimary->IsLost() || DDSSecondary->IsLost() || DDSBackBuffer->IsLost())
    {
        bool restored = true;
        restored &= DDSPrimary->Restore() == DD_OK;
        restored &= DDSSecondary->Restore() == DD_OK;
        if (DDSSecondary != DDSBackBuffer)
            restored &= DDSBackBuffer->Restore() == DD_OK;
        if (restored)
            return true;
        QMPlay2Core.processParam("RestartPlaying");
    }
    return false;
}

void Drawable::paintEvent(QPaintEvent *)
{
    if (!isOverlay && canDraw())
    {
        restoreLostSurface();
        fillRects();
        blit();
    }
}
bool Drawable::event(QEvent *e)
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

QPaintEngine *Drawable::paintEngine() const
{
    return isOverlay ? QWidget::paintEngine() : NULL;
}

/**/

DirectDrawWriter::DirectDrawWriter(Module &module) :
    outW(-1), outH(-1), flip(0), Hue(0), Saturation(0), Brightness(0), Contrast(0),
    aspect_ratio(0.0), zoom(0.0),
    hasVideoSize(false),
    drawable(NULL)
{
    addParam("W");
    addParam("H");
    addParam("AspectRatio");
    addParam("Zoom");
    addParam("Hue");
    addParam("Saturation");
    addParam("Brightness");
    addParam("Contrast");
    addParam("Flip");

    SetModule(module);
}
DirectDrawWriter::~DirectDrawWriter()
{
    delete drawable;
}

bool DirectDrawWriter::set()
{
    return sets().getBool("DirectDrawEnabled");
}

bool DirectDrawWriter::readyWrite() const
{
    return drawable && (drawable->canDraw() || !hasVideoSize);
}

bool DirectDrawWriter::processParams(bool *)
{
    bool doResizeEvent = drawable->isVisible();

    const double _aspect_ratio = getParam("AspectRatio").toDouble();
    const double _zoom = getParam("Zoom").toDouble();
    const int _flip = getParam("Flip").toInt();
    if (_aspect_ratio != aspect_ratio || _zoom != zoom || _flip != flip)
    {
        zoom = _zoom;
        aspect_ratio = _aspect_ratio;
        flip = _flip;
        drawable->setFlip();
    }

    const int _outW = getParam("W").toInt();
    const int _outH = getParam("H").toInt();
    if (_outW != outW || _outH != outH)
    {
        if (_outW == 0 || _outH == 0)
        {
            drawable->releaseSecondary();
            hasVideoSize = false;
        }
        else if (_outW > 0 && _outH > 0)
        {
            outW = _outW;
            outH = _outH;
            if (drawable->createSecondary())
                drawable->dock();
            hasVideoSize = true;
        }
    }

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
        drawable->videoEqSet();
    }

    if (doResizeEvent)
        drawable->resizeEvent(NULL);

    return readyWrite();
}

void DirectDrawWriter::writeVideo(const Frame &videoFrame)
{
    drawable->paused = false;
    if (drawable->canDraw())
        drawable->draw(videoFrame);
}
void DirectDrawWriter::writeOSD(const QList<const QMPlay2OSD *> &osds)
{
    drawable->osd_mutex.lock();
    drawable->osd_list = osds;
    drawable->osd_mutex.unlock();
}

void DirectDrawWriter::pause()
{
    drawable->paused = true;
}

QString DirectDrawWriter::name() const
{
    return QString(DirectDrawWriterName) + (drawable->isOverlay ? "/Overlay" : "");
}

bool DirectDrawWriter::open()
{
    drawable = new Drawable(*this);
    return drawable->isOK;
}
