/*
	QMPlay2 is a video and audio player.
	Copyright (C) 2010-2018  Błażej Szczygieł

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

class Drawable final : public QWidget
{
public:
	Drawable(class QPainterWriter &);
	~Drawable();

	void draw(const VideoFrame &newVideoFrame, bool, bool);

	void resizeEvent(QResizeEvent *) override;

	VideoFrame videoFrame;
	QList<const QMPlay2OSD *> osd_list;
	int Brightness, Contrast;
	QMutex osd_mutex;
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

	QMPlay2PixelFormats supportedPixelFormats() const override;

	void writeVideo(const VideoFrame &videoFrame) override;
	void writeOSD(const QList<const QMPlay2OSD *> &) override;

	QString name() const override;

	bool open() override;

	/**/

	int outW, outH, flip;
	double aspect_ratio, zoom;

	Drawable *drawable;
};

#define QPainterWriterName "QPainter"
