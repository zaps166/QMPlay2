#ifndef VDPAUWRITER_HPP
#define VDPAUWRITER_HPP

#include <HWAccelHelper.hpp>
#include <VideoWriter.hpp>

#include <QWidget>
#include <QTimer>
#include <QQueue>

#include <vdpau/vdpau.h>

struct _XDisplay;

class VDPAUWriter : public QWidget, public HWAccelHelper, public VideoWriter
{
	Q_OBJECT
public:
	VDPAUWriter(Module &module);
	~VDPAUWriter();

	bool set();

	bool readyWrite() const;

	bool processParams(bool *paramsCorrected);
	void writeVideo(const VideoFrame &videoFrame);
	void writeOSD(const QList< const QMPlay2_OSD * > &osd);
	void pause();

	bool HWAccellGetImg(const VideoFrame &videoFrame, void *dest, ImgScaler *yv12ToRGB32) const;

	QString name() const;

	bool open();

/**/

	bool HWAccellInit(int W, int H, const char *codec_name);

	inline VdpDecoder getVdpDecoder() const
	{
		return decoder;
	}
	inline VdpDecoderRender *getVdpDecoderRender() const
	{
		return vpd_decoder_render;
	}

	QMPlay2SurfaceID getSurface();
	void putSurface(QMPlay2SurfaceID id);
private:
	static void preemption_callback(VdpDevice device, void *context);

	void setFeatures();

	Q_SLOT void videoVisible(bool);
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

	QList< VdpDecoderProfile > profileList;

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
	QQueue< VdpVideoSurface > surfacesQueue;
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
	QList< const QMPlay2_OSD * > osd_list;
	QList< QByteArray > osd_checksums;
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
