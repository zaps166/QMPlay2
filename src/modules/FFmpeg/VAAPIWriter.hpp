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

#ifndef VAAPIWRITER_HPP
#define VAAPIWRITER_HPP

#include <VideoWriter.hpp>
#include <VAAPI.hpp>

#include <QWidget>
#include <QTimer>

class VAAPIWriter : public QWidget, public VideoWriter
{
	Q_OBJECT
public:
	VAAPIWriter(Module &module, VAAPI *vaapi);
	~VAAPIWriter();

	bool set();

	bool readyWrite() const;

	bool processParams(bool *paramsCorrected);
	void writeVideo(const VideoFrame &videoFrame);
	void writeOSD(const QList<const QMPlay2OSD *> &osd);
	void pause();

	bool hwAccelGetImg(const VideoFrame &videoFrame, void *dest, ImgScaler *nv12ToRGB32) const;

	QString name() const;

	bool open();

	/**/

	inline VAAPI *getVAAPI() const
	{
		return vaapi;
	}

	void init();

private:
	Q_SLOT void draw(VASurfaceID _id = -1, int _field = -1);

	void resizeEvent(QResizeEvent *);
	void paintEvent(QPaintEvent *);
	bool event(QEvent *);

	QPaintEngine *paintEngine() const;

	void clearRGBImage();

	VAAPI *vaapi;

	bool paused;

	static const int drawTimeout = 40;
	QList<const QMPlay2OSD *> osd_list;
	bool subpict_dest_is_screen_coord;
	QList<QByteArray> osd_checksums;
	VASubpictureID vaSubpicID;
	VAImageFormat *rgbImgFmt;
	QMutex osd_mutex;
	QTimer drawTim;
	QSize vaImgSize;
	VAImage vaImg;

	QRect dstQRect, srcQRect;
	double aspect_ratio, zoom;
	VASurfaceID id;
	int field, X, Y, W, H, deinterlace, Hue, Saturation, Brightness, Contrast;
};

#endif //VAAPIWRITER_HPP
