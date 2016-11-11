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

#ifndef VAAPI_HPP
#define VAAPI_HPP

#include <HWAccelHelper.hpp>

#include <QCoreApplication>

#include <va/va.h>

#if VA_VERSION_HEX >= 0x220000 // 1.2.0
	#include <va/va_vpp.h>

	#define NEW_CREATESURFACES
	#define HAVE_VPP
#endif

struct _XDisplay;
class VideoFrame;
class ImgScaler;

class VAAPI
{
	Q_DECLARE_TR_FUNCTIONS(VAAPI)
public:
	VAAPI();
	~VAAPI();

	bool open(bool allowVDPAU, bool openGL);

	bool init(int width, int height, const char *codecName);

	SurfacesQueue getSurfacesQueue() const;

	inline bool vaCreateConfigAndContext();
	bool vaapiCreateSurfaces(VASurfaceID *surfaces, int surfacesCount, bool useAttr);

	void init_vpp();

	void clr_vpp();
	void clr();

	bool writeVideo(const VideoFrame &videoFrame, int deinterlace, VASurfaceID &id, int &field);

	quint8 *getNV12Image(VAImage &image, VASurfaceID surfaceID) const;
	bool getImage(const VideoFrame &videoFrame, void *dest, ImgScaler *nv12ToRGB32) const;

	/**/

	bool ok, isXvBA, isVDPAU;

	VADisplay VADisp;
	VAContextID context;
	VAConfigID config;

	VAImageFormat nv12ImageFmt;

	int outW, outH;

#ifdef HAVE_VPP //Postprocessing
	VAProcDeinterlacingType vpp_deint_type;
	bool use_vpp;
#endif

private:
	int version;

	_XDisplay *display;
	VAProfile profile;

	QList<VAProfile> profileList;

	static const int surfacesCount = 20;
	VASurfaceID surfaces[surfacesCount];
	bool surfacesCreated;


#ifdef HAVE_VPP //Postprocessing
	VAContextID context_vpp;
	VAConfigID config_vpp;
	VABufferID vpp_buffers[VAProcFilterCount]; //TODO implement all filters
	VASurfaceID id_vpp, forward_reference;
	bool vpp_second;
#endif
};

#endif // VAAPI_HPP
