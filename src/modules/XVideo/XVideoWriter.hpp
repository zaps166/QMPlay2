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

#include <VideoWriter.hpp>

#include <xv.hpp>

#include <QWidget>

class Drawable : public QWidget
{
	friend class XVideoWriter;
public:
	Drawable(class XVideoWriter &);
private:
	void resizeEvent(QResizeEvent *);
	void paintEvent(QPaintEvent *);
	bool event(QEvent *);

	QPaintEngine *paintEngine() const;

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
	~XVideoWriter();

	bool set();

	bool readyWrite() const;

	bool processParams(bool *paramsCorrected);
	void writeVideo(const VideoFrame &videoFrame);
	void writeOSD(const QList<const QMPlay2OSD *> &);

	QString name() const;

	bool open();

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
