// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <XVideoHeaders.hpp>

#include <VideoWriter.hpp>

#include <xv.hpp>

class XVideoDrawable final : public QWidget
{
    friend class XVideoWriter;
public:
    XVideoDrawable(class XVideoWriter &);
    ~XVideoDrawable() final = default;
private:
    void resizeEvent(QResizeEvent *) override;
    void paintEvent(QPaintEvent *) override;
    bool event(QEvent *) override;

    QPaintEngine *paintEngine() const override;

    int X, Y, W, H;
    QRect dstRect, srcRect;
    XVideoWriter &writer;
};

/**/

class XVideoWriter final : public VideoWriter
{
    friend class XVideoDrawable;
public:
    XVideoWriter(Module &);
private:
    ~XVideoWriter();

    bool set() override;

    bool readyWrite() const override;

    bool processParams(bool *paramsCorrected) override;
    void writeVideo(const Frame &videoFrame, QMPlay2OSDList &&osdList) override;

    QString name() const override;

    bool open() override;

    /**/

    int outW, outH, Hue, Saturation, Brightness, Contrast;
    double aspect_ratio, zoom;
    QString adaptorName;
    bool hasVideoSize;
    bool useSHM;

    XVideoDrawable *drawable;
    XVIDEO *xv;

    QMPlay2OSDList osd_list;
};

#define XVideoWriterName "XVideo"
