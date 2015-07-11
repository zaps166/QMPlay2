#ifndef VAAPIWRITER_HPP
#define VAAPIWRITER_HPP

#include <HWAccelHelper.hpp>
#include <VideoWriter.hpp>

#include <QWidget>
#include <QQueue>

#include <va/va.h>

#if VA_VERSION_HEX >= 0x220000
	#include <va/va_vpp.h>

	#define NEW_CREATESURFACES
	#define HAVE_VPP
#endif

struct _XDisplay;

class VAApiWriter : public HWAccelHelper, public VideoWriter, public QWidget
{
public:
	VAApiWriter( Module &module );
	~VAApiWriter();

	bool set();

	bool readyWrite() const;

	bool processParams( bool *paramsCorrected );
	qint64 write( const QByteArray & );
	void pause();
	void writeOSD( const QList< const QMPlay2_OSD * > &osd );

	bool HWAccellGetImg( const VideoFrame *videoFrame, void *dest, ImgScaler *yv12ToRGB32 ) const;

	QString name() const;

	bool open();

/**/

	quint8 *getImage( VAImage &image, VASurfaceID surfaceID, VAImageFormat *img_fmt ) const;
	bool getRGB32Image( VAImageFormat *img_fmt, VASurfaceID surfaceID, void *dest ) const;
	bool getYV12Image( VAImageFormat *img_fmt, VASurfaceID surfaceID, void *dest, ImgScaler *yv12ToRGB32 ) const;
	bool getNV12Image( VAImageFormat *img_fmt, VASurfaceID surfaceID, void *dest, ImgScaler *yv12ToRGB32 ) const;

	bool HWAccellInit( int W, int H, const char *codec_name );

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
	void putSurface( QMPlay2SurfaceID id );
private:
	void init_vpp();

	bool vaCreateConfigAndContext();
	bool vaCreateSurfaces( VASurfaceID *surfaces, int num_surfaces );

	void draw( VASurfaceID _id = -1, int _field = -1 );

	void resizeEvent( QResizeEvent * );
	void paintEvent( QPaintEvent * );
	bool event( QEvent * );

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

	QList< VAProfile > profileList;

	static const int surfacesCount = 20;
	VASurfaceID surfaces[ surfacesCount ];
	QQueue< VASurfaceID > surfacesQueue;
	bool surfacesCreated, paused;

	QList< const QMPlay2_OSD * > osd_list;
	bool subpict_dest_is_screen_coord;
	QList< QByteArray > osd_checksums;
	VASubpictureID vaSubpicID;
	QMutex osd_mutex;
	QSize vaImgSize;
	VAImage vaImg;

	QRect dstQRect, srcQRect;
	double aspect_ratio, zoom;
	VASurfaceID id;
	int field, X, Y, W, H, outW, outH, deinterlace, Hue, Saturation, Brightness, Contrast;

#ifdef HAVE_VPP //Postprocessing
	VAContextID context_vpp;
	VAConfigID config_vpp;
	VABufferID vpp_buffers[ VAProcFilterCount ]; //TODO implement all filters
	VAProcDeinterlacingType vpp_deint_type;
	VASurfaceID id_vpp, forward_reference;
	bool use_vpp, vpp_second;
#endif
};

#endif //VAAPIWRITER_HPP
