// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <DirectXHeaders.hpp>

#include <VideoWriter.hpp>
#include <QMPlay2OSD.hpp>

class DirectDrawWriter;

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

    QMPlay2OSDList osd_list;
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
    void writeVideo(const Frame &videoFrame, QMPlay2OSDList &&osdList) override;

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
