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
#include <VideoFrame.hpp>
#include <ImgScaler.hpp>

#include <QWidget>

class QPainterWriter;
class QMPlay2OSD;

class Drawable : public QWidget
{
public:
	Drawable(class QPainterWriter &);
	~Drawable() final;

	void draw(const VideoFrame &newVideoFrame, bool, bool);
	void clr();

	void resizeEvent(QResizeEvent *) override final;

	VideoFrame videoFrame;
	QList<const QMPlay2OSD *> osd_list;
	int Brightness, Contrast;
	QMutex osd_mutex;
private:
	void paintEvent(QPaintEvent *) override final;
	bool event(QEvent *) override final;

	int X, Y, W, H;
	QPainterWriter &writer;
	QImage img;
	ImgScaler imgScaler;
};

/**/

class QPainterWriter : public VideoWriter
{
	friend class Drawable;
public:
	QPainterWriter(Module &);
private:
	~QPainterWriter();

	bool set() override final;

	bool readyWrite() const override final;

	bool processParams(bool *paramsCorrected) override final;

	QMPlay2PixelFormats supportedPixelFormats() const override final;

	void writeVideo(const VideoFrame &videoFrame) override final;
	void writeOSD(const QList<const QMPlay2OSD *> &) override final;

	QString name() const override final;

	bool open() override final;

	/**/

	int outW, outH, flip;
	double aspect_ratio, zoom;

	Drawable *drawable;
};

#define QPainterWriterName "QPainter"
