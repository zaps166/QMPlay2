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

#ifndef VDPAUWRITER_HPP
#define VDPAUWRITER_HPP

#include <VideoWriter.hpp>
#include <HWAccelHelper.hpp>

#include <QWidget>
#include <QTimer>

#include <vdpau/vdpau.h>

struct _XDisplay;

class VDPAUWriter : public QWidget, public VideoWriter
{
	Q_OBJECT
public:
	VDPAUWriter(Module &module);
	~VDPAUWriter();

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

	SurfacesQueue getSurfacesQueue() const;

	bool hwAccelInit(int W, int H, const char *codec_name);

	inline VdpDecoder getVdpDecoder() const
	{
		return decoder;
	}
	inline VdpDecoderRender *getVdpDecoderRender() const
	{
		return vpd_decoder_render;
	}

private:
	static void preemption_callback(VdpDevice device, void *context);

	void setFeatures();

	Q_SLOT void videoVisible1(bool);
#if QT_VERSION >= 0x050000
	Q_SLOT void videoVisible2(bool);
#endif
	Q_SLOT void doVideoVisible();

	void presentationQueueCreate(WId winId);

	Q_SLOT void draw(VdpVideoSurface surface_id = VDP_INVALID_HANDLE);
	void vdpau_display();

	void resizeEvent(QResizeEvent *);
	void paintEvent(QPaintEvent *);
	bool event(QEvent *);

	QPaintEngine *paintEngine() const;

	void destroyOutputSurfaces();
	void clr();

	bool ok, mustRestartPlaying, canDrawOSD;

	QList<VdpDecoderProfile> profileList;

	VdpPresentationQueueTarget queueTarget;
	VdpPresentationQueue presentationQueue;
	VdpDecoderProfile profile;
	VdpVideoMixer mixer;
	VdpDecoder decoder;
	VdpDevice device;
	_XDisplay *display;

	VdpGetProcAddress *vdp_get_proc_address;
	VdpDecoderCreate *vdp_decoder_create;
	VdpDecoderRender *vpd_decoder_render;
	VdpVideoSurfaceCreate *vdp_video_surface_create;
	VdpVideoSurfaceDestroy *vdp_video_surface_destroy;
	VdpOutputSurfaceCreate *vdp_output_surface_create;
	VdpOutputSurfaceDestroy *vdp_output_surface_destroy;
	VdpBitmapSurfaceQueryCapabilities *vdp_bitmap_surface_query_capabilities;
	VdpBitmapSurfaceCreate *vdp_bitmap_surface_create;
	VdpBitmapSurfaceDestroy *vdp_bitmap_surface_destroy;
	VdpBitmapSurfacePutBitsNative *vdp_bitmap_surface_put_bits_native;
	VdpOutputSurfaceRenderBitmapSurface *vdp_output_surface_render_bitmap_surface;
	VdpPresentationQueueCreate *vdp_presentation_queue_create;
	VdpPresentationQueueSetBackgroundColor *vdp_presentation_queue_set_background_color;
	VdpPresentationQueueDestroy *vdp_presentation_queue_destroy;
	VdpPresentationQueueTargetDestroy *vdp_presentation_queue_target_destroy;
	VdpVideoMixerCreate *vdp_video_mixer_create;
	VdpVideoMixerSetFeatureEnables *vdp_video_mixer_set_feature_enables;
	VdpVideoMixerDestroy *vdp_video_mixer_destroy;
	VdpPresentationQueueBlockUntilSurfaceIdle *vdp_presentation_queue_block_until_surface_idle;
	VdpVideoMixerRender *vdp_video_mixer_render;
	VdpVideoMixerSetAttributeValues *vdp_video_mixer_set_attribute_values;
	VdpVideoSurfaceGetBitsYCbCr *vdp_surface_get_bits;
	VdpPresentationQueueDisplay *vdp_presentation_queue_display;
	VdpDeviceDestroy *vdp_device_destroy;
	VdpDecoderDestroy *vdp_decoder_destroy;
	VdpGenerateCSCMatrix *vdp_generate_csc_matrix;
	VdpDecoderQueryCapabilities *vdp_decoder_query_capabilities;
	VdpPreemptionCallbackRegister *vdp_preemption_callback_register;
	VdpVideoMixerQueryFeatureSupport *vdp_video_mixer_query_feature_support;

	static const int surfacesCount = 20, outputSurfacesCount = 2;
	VdpOutputSurface outputSurfaces[outputSurfacesCount];
	VdpVideoSurface surfaces[surfacesCount], renderSurfaces[3];
	QSize outputSurfacesSize;

	static const int featuresCount = 13, scalingLevelsIdx = 4, scalingLevelsCount = 9;
	VdpVideoMixerFeature features[featuresCount];
	VdpBool featureEnables[featuresCount];
	float noisereduction_lvl, sharpness_lvl;
	int featuresCountCreated;

	VdpVideoMixerPictureStructure field;
	VdpRect srcRect, dstRect;
	WId lastWinId;

	static const int drawTimeout = 40;
	QList<const QMPlay2OSD *> osd_list;
	QList<QByteArray> osd_checksums;
	VdpBitmapSurface bitmapSurface;
	QTimer visibleTim, drawTim;
	QSize bitmapSurfaceSize;
	QMutex osd_mutex;
	QImage osdImg;

	bool surfacesCreated, outputSurfacesCreated, paused, hasImage;
	unsigned outputSurfaceIdx;

	double aspect_ratio, zoom;
	int X, Y, W, H, flip, outW, outH, deinterlace, Hue, Saturation, Brightness, Contrast;
};

#endif //VDPAUWRITER_HPP
