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

#include <VideoWriter.hpp>

#include <QWidget>
#include <QTimer>

#include <ddraw.h>

class DirectDrawWriter;
class QMPlay2OSD;

class Drawable final : public QWidget
{
    Q_OBJECT
public:
    Drawable(DirectDrawWriter &);
    ~Drawable();

    inline bool canDraw() const
    {
        return DDSSecondary;
    }

    void dock();
    bool createSecondary();
    void releaseSecondary();
    void videoEqSet();
    void setFlip();

    void draw(const Frame &videoFrame);

    void resizeEvent(QResizeEvent *) override;

    QList<const QMPlay2OSD *> osd_list;
    QMutex osd_mutex;
    bool isOK, isOverlay, paused;
private:
    void getRects(RECT &, RECT &);
    void fillRects();

    Q_SLOT void updateOverlay();
    Q_SLOT void overlayVisible(bool);
    Q_SLOT void doOverlayVisible();
    void blit();

    bool restoreLostSurface();

    void paintEvent(QPaintEvent *) override;
    bool event(QEvent *) override;

    QPaintEngine *paintEngine() const override;

    QImage osdImg;
    QVector<quint64> osd_ids;

    QTimer visibleTim;

    DirectDrawWriter &writer;

    int X, Y, W, H, flip;

    HBRUSH blackBrush;
    LPDIRECTDRAW DDraw;
    LPDIRECTDRAWCLIPPER DDClipper;
    LPDIRECTDRAWSURFACE DDSPrimary, DDSSecondary, DDSBackBuffer;
    LPDIRECTDRAWCOLORCONTROL DDrawColorCtrl;

    using DwmEnableCompositionProc = HRESULT (WINAPI *)(UINT uCompositionAction);
    DwmEnableCompositionProc DwmEnableComposition;
};

/**/

class DirectDrawWriter final : public VideoWriter
{
    friend class Drawable;
public:
    DirectDrawWriter(Module &);
private:
        ~DirectDrawWriter();

    bool set() override;

    bool readyWrite() const override;

    bool processParams(bool *paramsCorrected) override;
    void writeVideo(const Frame &videoFrame) override;
    void writeOSD(const QList<const QMPlay2OSD *> &) override;

    void pause() override;

    QString name() const override;

    bool open() override;

    /**/

    int outW, outH, flip, Hue, Saturation, Brightness, Contrast;
    double aspect_ratio, zoom;
    bool hasVideoSize;

    Drawable *drawable;
};

#define DirectDrawWriterName "DirectDraw"
