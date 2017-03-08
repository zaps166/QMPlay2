/*
	QMPlay2 is a video and audio player.
	Copyright (C) 2010-2017  Błażej Szczygieł

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

class Drawable : public QWidget
{
	friend class XVideoWriter;
public:
	Drawable(class XVideoWriter &);
	~Drawable() final = default;
private:
	void resizeEvent(QResizeEvent *) override final;
	void paintEvent(QPaintEvent *) override final;
	bool event(QEvent *) override final;

	QPaintEngine *paintEngine() const override final;

	int X, Y, W, H;
	QRect dstRect, srcRect;
	XVideoWriter &writer;
};

/**/

class QMPlay2OSD;

class XVideoWriter : public VideoWriter
{
	friend class Drawable;
public:
	XVideoWriter(Module &);
private:
	~XVideoWriter() final;

	bool set() override final;

	bool readyWrite() const override final;

	bool processParams(bool *paramsCorrected) override final;
	void writeVideo(const VideoFrame &videoFrame) override final;
	void writeOSD(const QList<const QMPlay2OSD *> &) override final;

	QString name() const override final;

	bool open() override final;

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
