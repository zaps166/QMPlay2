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

#include <xv.hpp>

#include <QWidget>

class Drawable final : public QWidget
{
    friend class XVideoWriter;
public:
    Drawable(class XVideoWriter &);
    ~Drawable() final = default;
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

class QMPlay2OSD;

class XVideoWriter final : public VideoWriter
{
    friend class Drawable;
public:
    XVideoWriter(Module &);
private:
    ~XVideoWriter();

    bool set() override;

    bool readyWrite() const override;

    bool processParams(bool *paramsCorrected) override;
    void writeVideo(const Frame &videoFrame) override;
    void writeOSD(const QList<const QMPlay2OSD *> &) override;

    QString name() const override;

    bool open() override;

    /**/

    int outW, outH, Hue, Saturation, Brightness, Contrast;
    double aspect_ratio, zoom;
    QString adaptorName;
    bool hasVideoSize;
    bool useSHM;

    Drawable *drawable;
    XVIDEO *xv;

    QList<const QMPlay2OSD *> osd_list;
    QMutex osd_mutex;
};

#define XVideoWriterName "XVideo"
