/*
	QMPlay2 is a video and audio player.
	Copyright (C) 2010-2016  Błażej Szczygieł

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

#include <HWAccelHelper.hpp>
#include <VideoWriter.hpp>

#include <QWidget>
#include <QTimer>
#include <QQueue>

#include <va/va.h>

#if VA_VERSION_HEX >= 0x220000 // 1.2.0
	#include <va/va_vpp.h>

	#define NEW_CREATESURFACES
	#define HAVE_VPP
#endif

struct _XDisplay;

class VAAPIWriter : public QWidget, public HWAccelHelper, public VideoWriter
{
	Q_OBJECT
public:
	VAAPIWriter(Module &module);
	~VAAPIWriter();

	bool set();

	bool readyWrite() const;

	bool processParams(bool *paramsCorrected);
	void writeVideo(const VideoFrame &videoFrame);
	void pause();
	void writeOSD(const QList<const QMPlay2_OSD *> &osd);

	bool HWAccellGetImg(const VideoFrame &videoFrame, void *dest, ImgScaler *yv12ToRGB32) const;

	QString name() const;

	bool open();

/**/

	quint8 *getImage(VAImage &image, VASurfaceID surfaceID, VAImageFormat &img_fmt) const;
	bool getNV12Image(VAImageFormat &img_fmt, VASurfaceID surfaceID, void *dest, ImgScaler *yv12ToRGB32) const;

	bool HWAccellInit(int W, int H, const char *codec_name);

	inline VADisplay getVADisplay() const
	{
		return VADisp;
	}
	inline VAContextID getVAContext() const
	{
		return context;
	}
	inline VAConfigID getVAConfig() const
	{
		return config;
	}

	QMPlay2SurfaceID getSurface();
	void putSurface(QMPlay2SurfaceID id);
private:
	void init_vpp();

	bool vaCreateConfigAndContext();
	bool vaCreateSurfaces(VASurfaceID *surfaces, int num_surfaces, bool useAttr);

	Q_SLOT void draw(VASurfaceID _id = -1, int _field = -1);

	void resizeEvent(QResizeEvent *);
	void paintEvent(QPaintEvent *);
	bool event(QEvent *);

	QPaintEngine *paintEngine() const;

	void clearRGBImage();
	void clr_vpp();
	void clr();

	bool ok, isXvBA, isVDPAU, allowVDPAU;

	VADisplay VADisp;
	VAContextID context;
	VAConfigID config;
	VAProfile profile;
	VAImageFormat *rgbImgFmt;
	_XDisplay *display;

	QList<VAProfile> profileList;

	static const int surfacesCount = 20;
	VASurfaceID surfaces[surfacesCount];
	QQueue<VASurfaceID> surfacesQueue;
	bool surfacesCreated, paused;

	static const int drawTimeout = 40;
	QList<const QMPlay2_OSD *> osd_list;
	bool subpict_dest_is_screen_coord;
	QList<QByteArray> osd_checksums;
	VASubpictureID vaSubpicID;
	QMutex osd_mutex;
	QTimer drawTim;
	QSize vaImgSize;
	VAImage vaImg;

	QRect dstQRect, srcQRect;
	double aspect_ratio, zoom;
	VASurfaceID id;
	int field, X, Y, W, H, outW, outH, deinterlace, Hue, Saturation, Brightness, Contrast;
	int minor, major;

#ifdef HAVE_VPP //Postprocessing
	VAContextID context_vpp;
	VAConfigID config_vpp;
	VABufferID vpp_buffers[VAProcFilterCount]; //TODO implement all filters
	VAProcDeinterlacingType vpp_deint_type;
	VASurfaceID id_vpp, forward_reference;
	bool use_vpp, vpp_second;
#endif
};

#endif //VAAPIWRITER_HPP
