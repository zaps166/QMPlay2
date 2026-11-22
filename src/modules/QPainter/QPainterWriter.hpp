// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <VideoWriter.hpp>
#include <Frame.hpp>
#include <ImgScaler.hpp>

#include <QWidget>

class QPainterWriter;
class QMPlay2OSD;

class Drawable final : public QWidget
{
public:
    Drawable(class QPainterWriter &);
    ~Drawable();

    void draw(const Frame &newVideoFrame, bool, bool);

    void resizeEvent(QResizeEvent *) override;

    Frame videoFrame;
    QMPlay2OSDList osd_list;
    int Brightness, Contrast;
private:
    void paintEvent(QPaintEvent *) override;
    bool event(QEvent *) override;

    int X, Y, W, H, imgW, imgH;
    QPainterWriter &writer;
    QImage img;
    ImgScaler imgScaler;
    bool m_scaleByQt = false;
};

/**/

class QPainterWriter final : public VideoWriter
{
    friend class Drawable;
public:
    QPainterWriter(Module &);
private:
    ~QPainterWriter();

    bool set() override;

    bool readyWrite() const override;

    bool processParams(bool *paramsCorrected) override;

    AVPixelFormats supportedPixelFormats() const override;

    void writeVideo(const Frame &videoFrame, QMPlay2OSDList &&osdList) override;

    QString name() const override;

    bool open() override;

    /**/

    int outW, outH, flip;
    double aspect_ratio, zoom;

    Drawable *drawable;
};

#define QPainterWriterName "QPainter"
