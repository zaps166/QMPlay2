#include <VAApiWriter.hpp>
#include <FFCommon.hpp>

#include <QMPlay2_OSD.hpp>
#include <VideoFrame.hpp>
#include <Functions.hpp>
#include <ImgScaler.hpp>

#include <QCoreApplication>
#include <QPainter>

#include <va/va_x11.h>

#ifdef HAVE_VPP
	#define VPP_TFF 0
	#define VPP_BFF ( VA_DEINTERLACING_BOTTOM_FIELD | VA_DEINTERLACING_BOTTOM_FIELD_FIRST )
#endif

VAApiWriter::VAApiWriter( Module &module ) :
	ok( false ),
	VADisp( NULL ),
	rgbImgFmt( NULL ),
	display( NULL ),
	aspect_ratio( 0.0 ), zoom( 0.0 ),
	outW( 0 ), outH( 0 ), Hue( 0 ), Saturation( 0 ), Brightness( 0 ), Contrast( 0 )
#ifdef HAVE_VPP
	, vpp_deint_type( VAProcDeinterlacingNone ),
	use_vpp( false )
#endif
{
	vaImg.image_id = vaSubpicID = 0;

	setAttribute( Qt::WA_OpaquePaintEvent );
	setAttribute( Qt::WA_PaintOnScreen );
	grabGesture( Qt::PinchGesture );
	setMouseTracking( true );

	SetModule( module );
}
VAApiWriter::~VAApiWriter()
{
	clr();
	if ( VADisp )
		vaTerminate( VADisp );
	if ( display )
		XCloseDisplay( display );
}

bool VAApiWriter::set()
{
#ifdef HAVE_VPP
	allowVDPAU = sets().getBool( "AllowVDPAUinVAAPI" );
	VAProcDeinterlacingType _vpp_deint_type;
	switch ( sets().getInt( "VAAPIDeintMethod" ) )
	{
		case 0:
			_vpp_deint_type = VAProcDeinterlacingNone;
			break;
		case 2:
			_vpp_deint_type = VAProcDeinterlacingMotionCompensated;
			break;
		default:
			_vpp_deint_type = VAProcDeinterlacingMotionAdaptive;
	}
	const bool reload_vpp = ok && use_vpp && vpp_deint_type != _vpp_deint_type;
	vpp_deint_type = _vpp_deint_type;
	if ( reload_vpp )
	{
		clr_vpp();
		init_vpp();
	}
#endif
	return true;
}

bool VAApiWriter::readyWrite() const
{
	return ok;
}

bool VAApiWriter::processParams( bool * )
{
	zoom = getParam( "Zoom" ).toDouble();
	deinterlace = getParam( "Deinterlace" ).toInt();
	aspect_ratio = getParam( "AspectRatio" ).toDouble();

	const int _Hue = getParam( "Hue" ).toInt();
	const int _Saturation = getParam( "Saturation" ).toInt();
	const int _Brightness = getParam( "Brightness" ).toInt();
	const int _Contrast = getParam( "Contrast" ).toInt();
	if ( _Hue != Hue || _Saturation != Saturation || _Brightness != Brightness || _Contrast != Contrast )
	{
		Hue = _Hue;
		Saturation = _Saturation;
		Brightness = _Brightness;
		Contrast = _Contrast;

		int num_attribs = vaMaxNumDisplayAttributes( VADisp );
		VADisplayAttribute attribs[ num_attribs ];
		if ( !vaQueryDisplayAttributes( VADisp, attribs, &num_attribs ) )
		{
			for ( int i = 0 ; i < num_attribs ; ++i )
			{
				switch ( attribs[ i ].type )
				{
					case VADisplayAttribHue:
						attribs[ i ].value = Functions::scaleEQValue( Hue, attribs[ i ].min_value, attribs[ i ].max_value );
						break;
					case VADisplayAttribSaturation:
						attribs[ i ].value = Functions::scaleEQValue( Saturation, attribs[ i ].min_value, attribs[ i ].max_value );
						break;
					case VADisplayAttribBrightness:
						attribs[ i ].value = Functions::scaleEQValue( Brightness, attribs[ i ].min_value, attribs[ i ].max_value );
						break;
					case VADisplayAttribContrast:
						attribs[ i ].value = Functions::scaleEQValue( Contrast, attribs[ i ].min_value, attribs[ i ].max_value );
						break;
					default:
						break;
				}
			}
			vaSetDisplayAttributes( VADisp, attribs, num_attribs );
		}
	}

	if ( !isVisible() )
		emit QMPlay2Core.dockVideo( this );
	else
	{
		resizeEvent( NULL );
		if ( paused )
			draw();
	}

	return readyWrite();
}
qint64 VAApiWriter::write( const QByteArray &data )
{
	VideoFrame *videoFrame = ( VideoFrame * )data.data();
	const VASurfaceID curr_id = ( unsigned long )videoFrame->data[ 3 ];
	const int field = FFCommon::getField( videoFrame, deinterlace, 0, VA_TOP_FIELD, VA_BOTTOM_FIELD );
#ifdef HAVE_VPP
	if ( use_vpp )
	{
		const bool do_vpp_deint = field != 0 && vpp_buffers[ VAProcFilterDeinterlacing ] != VA_INVALID_ID;
		bool vpp_ok = false;

		if ( !do_vpp_deint )
		{
			forward_reference = VA_INVALID_SURFACE;
			vpp_second = false;
		}

		if ( do_vpp_deint && forward_reference == VA_INVALID_SURFACE )
			forward_reference = curr_id;
		if ( !vpp_second && forward_reference == curr_id )
			return data.size();

		if ( do_vpp_deint && !vpp_second )
		{
			VAProcFilterParameterBufferDeinterlacing *deint_params = NULL;
			if ( vaMapBuffer( VADisp, vpp_buffers[ VAProcFilterDeinterlacing ], ( void ** )&deint_params ) == VA_STATUS_SUCCESS )
			{
				deint_params->flags = field == VA_TOP_FIELD ? VPP_TFF : VPP_BFF;
				vaUnmapBuffer( VADisp, vpp_buffers[ VAProcFilterDeinterlacing ] );
			}
		}

		VABufferID pipeline_buf;
		if ( vaCreateBuffer( VADisp, context_vpp, VAProcPipelineParameterBufferType, sizeof( VAProcPipelineParameterBuffer ), 1, NULL, &pipeline_buf ) == VA_STATUS_SUCCESS )
		{
			VAProcPipelineParameterBuffer *pipeline_param = NULL;
			if ( vaMapBuffer( VADisp, pipeline_buf, ( void ** )&pipeline_param ) == VA_STATUS_SUCCESS )
			{
				memset( pipeline_param, 0, sizeof *pipeline_param );
				pipeline_param->surface = curr_id;
				pipeline_param->output_background_color = 0xFF000000;

				pipeline_param->num_filters = 1;
				if ( !do_vpp_deint )
					pipeline_param->filters = &vpp_buffers[ VAProcFilterNone ];
				else
				{
					pipeline_param->filters = &vpp_buffers[ VAProcFilterDeinterlacing ];
					pipeline_param->num_forward_references = 1;
					pipeline_param->forward_references = &forward_reference;
				}

				vaUnmapBuffer( VADisp, pipeline_buf );
				if ( vaBeginPicture( VADisp, context_vpp, id_vpp ) == VA_STATUS_SUCCESS )
				{
					vpp_ok = vaRenderPicture( VADisp, context_vpp, &pipeline_buf, 1 ) == VA_STATUS_SUCCESS;
					vaEndPicture( VADisp, context_vpp );
				}
			}
			if ( !vpp_ok )
				vaDestroyBuffer( VADisp, pipeline_buf );
		}

		if ( vpp_second )
			forward_reference = curr_id;
		if ( do_vpp_deint )
			vpp_second = !vpp_second;

		if ( ( ok = vpp_ok ) )
			draw( id_vpp, do_vpp_deint ? 0 : field );
	}
	else
#endif
		draw( curr_id, field );
	paused = false;
	return data.size();
}
void VAApiWriter::pause()
{
	paused = true;
}
void VAApiWriter::writeOSD( const QList< const QMPlay2_OSD * > &osds )
{
	if ( rgbImgFmt )
	{
		osd_mutex.lock();
		osd_list = osds;
		osd_mutex.unlock();
	}
}

bool VAApiWriter::HWAccellGetImg( const VideoFrame *videoFrame, void *dest, ImgScaler *yv12ToRGB32 ) const
{
	if ( dest && !( outH & 1 ) && !( outW % 4 ) )
	{
		int fmt_count = vaMaxNumImageFormats( VADisp );
		VAImageFormat img_fmt[ fmt_count ];
		if ( vaQueryImageFormats( VADisp, img_fmt, &fmt_count ) == VA_STATUS_SUCCESS )
		{
			const VASurfaceID surfaceID = ( unsigned long )videoFrame->data[ 3 ];
			int img_fmt_idx[ 3 ] = { -1, -1, -1 };
			for ( int i = 0 ; i < fmt_count ; ++i )
			{
				if ( !qstrncmp( ( const char * )&img_fmt[ i ].fourcc, "BGR", 3 ) )
					img_fmt_idx[ 0 ] = i;
				else if ( !qstrncmp( ( const char * )&img_fmt[ i ].fourcc, "YV12", 4 ) )
					img_fmt_idx[ 1 ] = i;
				else if ( !qstrncmp( ( const char * )&img_fmt[ i ].fourcc, "NV12", 4 ) )
					img_fmt_idx[ 2 ] = i;
			}
			return
			(
				( img_fmt_idx[ 0 ] > -1 && getRGB32Image( &img_fmt[ img_fmt_idx[ 0 ] ], surfaceID, dest ) ) ||
				( img_fmt_idx[ 1 ] > -1 && getYV12Image( &img_fmt[ img_fmt_idx[ 1 ] ], surfaceID, dest, yv12ToRGB32 ) ) ||
				( img_fmt_idx[ 2 ] > -1 && getNV12Image( &img_fmt[ img_fmt_idx[ 2 ] ], surfaceID, dest, yv12ToRGB32 ) )
			);
		}
	}
	return false;
}

QString VAApiWriter::name() const
{
	return VAApiWriterName;
}

bool VAApiWriter::open()
{
	addParam( "Zoom" );
	addParam( "AspectRatio" );
	addParam( "Deinterlace" );
	addParam( "PrepareForHWBobDeint", true );
	addParam( "Hue" );
	addParam( "Saturation" );
	addParam( "Brightness" );
	addParam( "Contrast" );

	clr();

	VADisp = vaGetDisplay( ( display = XOpenDisplay( NULL ) ) );
	int minor, major;
	if ( vaInitialize( VADisp, &minor, &major ) == VA_STATUS_SUCCESS )
	{
		const QString vendor = vaQueryVendorString( VADisp );
		isVDPAU = vendor.contains( "VDPAU" );
		if ( isVDPAU && !allowVDPAU )
			return false;
		isXvBA = vendor.contains( "XvBA" );

		int numProfiles = vaMaxNumProfiles( VADisp );
		VAProfile profiles[ numProfiles ];
		if ( vaQueryConfigProfiles( VADisp, profiles, &numProfiles ) == VA_STATUS_SUCCESS )
		{
			for ( int i = 0 ; i < numProfiles ; ++i )
				profileList.push_back( profiles[ i ] );
			return true;
		}
	}
	return false;
}

quint8 *VAApiWriter::getImage( VAImage &image, VASurfaceID surfaceID, VAImageFormat *img_fmt ) const
{
	if ( vaCreateImage( VADisp, img_fmt, outW, outH, &image ) == VA_STATUS_SUCCESS )
	{
		quint8 *data;
		if
		(
			vaSyncSurface( VADisp, surfaceID ) == VA_STATUS_SUCCESS &&
			vaGetImage( VADisp, surfaceID, 0, 0, outW, outH, image.image_id ) == VA_STATUS_SUCCESS &&
			vaMapBuffer( VADisp, image.buf, ( void ** )&data ) == VA_STATUS_SUCCESS
		) return data;
		vaDestroyImage( VADisp, image.image_id );
	}
	return NULL;
}
bool VAApiWriter::getRGB32Image( VAImageFormat *img_fmt, VASurfaceID surfaceID, void *dest ) const
{
	VAImage image;
	quint8 *data = getImage( image, surfaceID, img_fmt );
	if ( data )
	{
		memcpy( dest, data + image.offsets[ 0 ], outW * outH << 2 );
		vaUnmapBuffer( VADisp, image.buf );
		vaDestroyImage( VADisp, image.image_id );
		return true;
	}
	return false;
}
bool VAApiWriter::getYV12Image( VAImageFormat *img_fmt, VASurfaceID surfaceID, void *dest, ImgScaler *yv12ToRGB32 ) const
{
	VAImage image;
	quint8 *data = getImage( image, surfaceID, img_fmt );
	if ( data )
	{
		QByteArray yv12;
		yv12.resize( outW * outH * 3 << 1 );
		char *yv12Data = yv12.data();
		memcpy( yv12Data, data + image.offsets[ 0 ], outW * outH );
		memcpy( yv12Data + outW * outH, data + image.offsets[ 1 ], outW/2 * outH/2 );
		memcpy( yv12Data + outW * outH + outW/2 * outH/2, data + image.offsets[ 2 ], outW/2 * outH/2 );
		vaUnmapBuffer( VADisp, image.buf );
		yv12ToRGB32->scale( yv12Data, dest );
		vaDestroyImage( VADisp, image.image_id );
		return true;
	}
	return false;
}
bool VAApiWriter::getNV12Image( VAImageFormat *img_fmt, VASurfaceID surfaceID, void *dest, ImgScaler *yv12ToRGB32 ) const
{
	VAImage image;
	quint8 *data = getImage( image, surfaceID, img_fmt );
	if ( data )
	{
		QByteArray yv12;
		yv12.resize( outW * outH * 3 << 1 );
		char *yv12Data = yv12.data();
		memcpy( yv12Data, data + image.offsets[ 0 ], outW * outH );
		quint8 *yv12data_cb = ( quint8 * )yv12Data + outW * outH;
		quint8 *yv12data_cr = yv12data_cb + outW/2 * outH/2;
		const unsigned second_plane_size = outW * outH / 2;
		data += image.offsets[ 1 ];
		for ( unsigned i = 0 ; i < second_plane_size ; i += 2 )
		{
			*( yv12data_cr++ ) = *( data++ );
			*( yv12data_cb++ ) = *( data++ );
		}
		vaUnmapBuffer( VADisp, image.buf );
		yv12ToRGB32->scale( yv12.data(), dest );
		vaDestroyImage( VADisp, image.image_id );
		return true;
	}
	return false;
}

bool VAApiWriter::HWAccellInit( int W, int H, const char *codec_name )
{
	VAProfile p = ( VAProfile )-1; //VAProfileNone
	if ( !qstrcmp( codec_name, "h264" ) )
	{
		if ( profileList.contains( VAProfileH264High ) )
			p = VAProfileH264High;
		else if ( profileList.contains( VAProfileH264Main ) )
			p = VAProfileH264Main;
		else if ( profileList.contains( VAProfileH264Baseline ) )
			p = VAProfileH264Baseline;
	}
	else if ( !qstrcmp( codec_name, "mpeg2video" ) )
	{
		if ( profileList.contains( VAProfileMPEG2Main ) )
			p = VAProfileMPEG2Main;
		else if ( profileList.contains( VAProfileMPEG2Simple ) )
			p = VAProfileMPEG2Simple;
	}
	else if ( !qstrcmp( codec_name, "mpeg4" ) )
	{
		if ( profileList.contains( VAProfileMPEG4Main ) )
			p = VAProfileMPEG4Main;
		else if ( profileList.contains( VAProfileMPEG4Simple ) )
			p = VAProfileMPEG4Simple;
	}
	else if ( !qstrcmp( codec_name, "vc1" ) )
	{
		if ( profileList.contains( VAProfileVC1Advanced ) )
			p = VAProfileVC1Advanced;
		else if ( profileList.contains( VAProfileVC1Main ) )
			p = VAProfileVC1Main;
		else if ( profileList.contains( VAProfileVC1Simple ) )
			p = VAProfileVC1Simple;
	}
	else if ( !qstrcmp( codec_name, "h263" ) && profileList.contains( VAProfileH263Baseline ) )
		p = VAProfileH263Baseline;

	if ( !ok || profile != p || outW != W || outH != H )
	{
		clr();

		profile = p;
		outW = W;
		outH = H;

		if ( !vaCreateSurfaces( surfaces, surfacesCount ) )
			return false;
		surfacesCreated = true;

		if ( !vaCreateConfigAndContext() )
			return false;

		for ( int i = 0 ; i < surfacesCount ; i++ )
			surfacesQueue.enqueue( surfaces[ i ] );


		unsigned numSubpicFmts = vaMaxNumSubpictureFormats( VADisp );
		VAImageFormat subpicFmtList[ numSubpicFmts ];
		unsigned subpic_flags[ numSubpicFmts ];
		if ( vaQuerySubpictureFormats( VADisp, subpicFmtList, subpic_flags, &numSubpicFmts ) == VA_STATUS_SUCCESS )
		{
			for ( unsigned i = 0 ; i < numSubpicFmts ; ++i )
				if ( !qstrncmp( ( const char * )&subpicFmtList[ i ].fourcc, "BGRA", 4 ) )
				{
					subpict_dest_is_screen_coord = subpic_flags[ i ] & VA_SUBPICTURE_DESTINATION_IS_SCREEN_COORD;
					rgbImgFmt = new VAImageFormat( subpicFmtList[ i ] );
					break;
				}
		}

		init_vpp();

		if ( isXvBA )
		{
			QWidget::destroy();
			QWidget::create();
		}

		ok = true;
	}
	else
	{
#ifdef HAVE_VPP
		forward_reference = VA_INVALID_SURFACE;
		vpp_second = false;
#endif
		if ( isVDPAU )
		{
			if ( context )
			{
				vaDestroyContext( VADisp, context );
				context = 0;
			}
			if ( config )
			{
				vaDestroyConfig( VADisp, config );
				config = 0;
			}
			if ( !vaCreateConfigAndContext() )
				return false;
		}
	}

	return ok;
}

QMPlay2SurfaceID VAApiWriter::getSurface()
{
	return surfacesQueue.isEmpty() ? QMPlay2InvalidSurfaceID : surfacesQueue.dequeue();
}
void VAApiWriter::putSurface( QMPlay2SurfaceID id )
{
	surfacesQueue.enqueue( id );
}

void VAApiWriter::init_vpp()
{
#ifdef HAVE_VPP
	use_vpp = true;
	if
	(
		vaCreateConfig( VADisp, ( VAProfile )-1, VAEntrypointVideoProc, NULL, 0, &config_vpp ) == VA_STATUS_SUCCESS &&
		vaCreateContext( VADisp, config_vpp, 0, 0, 0, NULL, 0, &context_vpp ) == VA_STATUS_SUCCESS &&
		vaCreateSurfaces( &id_vpp, 1 )
	)
	{
		unsigned num_filters = VAProcFilterCount;
		VAProcFilterType filters[ VAProcFilterCount ];
		if ( vaQueryVideoProcFilters( VADisp, context_vpp, filters, &num_filters ) != VA_STATUS_SUCCESS )
			num_filters = 0;
		if ( num_filters )
		{
			/* Creating dummy filter (some drivers/api versions crashes without any filter) */
			VAProcFilterParameterBufferBase none_params = { VAProcFilterNone };
			if ( vaCreateBuffer( VADisp, context_vpp, VAProcFilterParameterBufferType, sizeof none_params, 1, &none_params, &vpp_buffers[ VAProcFilterNone ] ) != VA_STATUS_SUCCESS )
				vpp_buffers[ VAProcFilterNone ] = VA_INVALID_ID;
			/* Searching deinterlacing filter */
			if ( vpp_deint_type != VAProcDeinterlacingNone )
				for ( unsigned i = 0 ; i < num_filters ; ++i )
					if ( filters[ i ] == VAProcFilterDeinterlacing )
					{
						VAProcFilterCapDeinterlacing deinterlacing_caps[ VAProcDeinterlacingCount ];
						unsigned num_deinterlacing_caps = VAProcDeinterlacingCount;
						if ( vaQueryVideoProcFilterCaps( VADisp, context_vpp, VAProcFilterDeinterlacing, &deinterlacing_caps, &num_deinterlacing_caps ) != VA_STATUS_SUCCESS )
							num_deinterlacing_caps = 0;
						bool vpp_deint_types[ 2 ] = { false };
						for ( unsigned j = 0 ; j < num_deinterlacing_caps ; ++j )
						{
							switch ( deinterlacing_caps[ j ].type )
							{
								case VAProcDeinterlacingMotionAdaptive:
									vpp_deint_types[ 0 ] = true;
									break;
								case VAProcDeinterlacingMotionCompensated:
									vpp_deint_types[ 1 ] = true;
									break;
								default:
									break;
							}
						}
						if ( vpp_deint_type == VAProcDeinterlacingMotionCompensated && !vpp_deint_types[ 1 ] )
						{
							QMPlay2Core.log( tr( "Nie obsługiwany algorytm usuwania przeplotu" ) + " - Motion compensated", ErrorLog | LogOnce );
							vpp_deint_type = VAProcDeinterlacingMotionAdaptive;
						}
						if ( vpp_deint_type == VAProcDeinterlacingMotionAdaptive && !vpp_deint_types[ 0 ] )
						{
							QMPlay2Core.log( tr( "Nie obsługiwany algorytm usuwania przeplotu" ) + " - Motion adaptive", ErrorLog | LogOnce );
							vpp_deint_type = VAProcDeinterlacingNone;
						}
						if ( vpp_deint_type != VAProcDeinterlacingNone )
						{
							VAProcFilterParameterBufferDeinterlacing deint_params = { VAProcFilterDeinterlacing, vpp_deint_type, VPP_TFF };
							if ( vaCreateBuffer( VADisp, context_vpp, VAProcFilterParameterBufferType, sizeof deint_params, 1, &deint_params, &vpp_buffers[ VAProcFilterDeinterlacing ] ) != VA_STATUS_SUCCESS )
								vpp_buffers[ VAProcFilterDeinterlacing ] = VA_INVALID_ID;
						}
						break;
					}
			return;
		}
	}
	if ( vpp_deint_type != VAProcDeinterlacingNone ) //Show error only when filter is required
		QMPlay2Core.log( "VA-API :: " + tr( "Nie można otworzyć filtrów obrazu" ), ErrorLog | LogOnce );
	clr_vpp();
#endif
}

bool VAApiWriter::vaCreateConfigAndContext()
{
	return vaCreateConfig( VADisp, profile, VAEntrypointVLD, NULL, 0, &config ) == VA_STATUS_SUCCESS && vaCreateContext( VADisp, config, outW, outH, VA_PROGRESSIVE, surfaces, surfacesCount, &context ) == VA_STATUS_SUCCESS;
}
bool VAApiWriter::vaCreateSurfaces( VASurfaceID *surfaces, int num_surfaces )
{
#ifdef NEW_CREATESURFACES
	return ::vaCreateSurfaces( VADisp, VA_RT_FORMAT_YUV420, outW, outH, surfaces, num_surfaces, NULL, 0 ) == VA_STATUS_SUCCESS;
#else
	return ::vaCreateSurfaces( VADisp, outW, outH, VA_RT_FORMAT_YUV420, num_surfaces, surfaces ) == VA_STATUS_SUCCESS;
#endif
}

void VAApiWriter::draw( VASurfaceID _id, int _field )
{
	if ( _id != VA_INVALID_SURFACE && _field > -1 )
	{
		if ( id != _id || _field == field )
			vaSyncSurface( VADisp, _id );
		id = _id;
		field = _field;
	}
	if ( id == VA_INVALID_SURFACE )
		return;

	bool associated = false;

	osd_mutex.lock();
	if ( !osd_list.isEmpty() )
	{
		QRect bounds;
		const qreal scaleW = ( qreal )W / outW, scaleH = ( qreal )H / outH;
		bool mustRepaint = Functions::mustRepaintOSD( osd_list, osd_checksums, &scaleW, &scaleH, &bounds );
		if ( !mustRepaint )
			mustRepaint = vaImgSize != bounds.size();
		bool canAssociate = !mustRepaint;
		if ( mustRepaint )
		{
			if ( vaImgSize != bounds.size() )
			{
				clearRGBImage();
				vaImgSize = QSize();
				if ( vaCreateImage( VADisp, rgbImgFmt, bounds.width(), bounds.height(), &vaImg ) == VA_STATUS_SUCCESS )
				{
					if ( vaCreateSubpicture( VADisp, vaImg.image_id, &vaSubpicID ) == VA_STATUS_SUCCESS )
						vaImgSize = bounds.size();
					else
						clearRGBImage();
				}
			}
			if ( vaSubpicID )
			{
				quint8 *buff;
				if ( vaMapBuffer( VADisp, vaImg.buf, ( void ** )&buff ) == VA_STATUS_SUCCESS )
				{
					QImage osdImg( buff += vaImg.offsets[ 0 ], vaImg.pitches[ 0 ] >> 2, bounds.height(), QImage::Format_ARGB32 );
					osdImg.fill( 0 );
					QPainter p( &osdImg );
					p.translate( -bounds.topLeft() );
					Functions::paintOSD( osd_list, scaleW, scaleH, p, &osd_checksums );
					vaUnmapBuffer( VADisp, vaImg.buf );
					canAssociate = true;
				}
			}
		}
		if ( vaSubpicID && canAssociate )
		{
			if ( subpict_dest_is_screen_coord )
			{
				associated = vaAssociateSubpicture
				(
					VADisp, vaSubpicID, &id, 1,
					0,              0,              bounds.width(), bounds.height(),
					bounds.x() + X, bounds.y() + Y, bounds.width(), bounds.height(),
					VA_SUBPICTURE_DESTINATION_IS_SCREEN_COORD
				) == VA_STATUS_SUCCESS;
			}
			else
			{
				const double sW = ( double )outW / dstQRect.width(), sH = ( double )outH / dstQRect.height();
				const int Xoffset = dstQRect.width() == width() ? X : 0;
				const int Yoffset = dstQRect.height() == height() ? Y : 0;
				associated = vaAssociateSubpicture
				(
					VADisp, vaSubpicID, &id, 1,
					0,                             0,                             bounds.width(),      bounds.height(),
					( bounds.x() + Xoffset ) * sW, ( bounds.y() + Yoffset ) * sH, bounds.width() * sW, bounds.height() * sH,
					0
				) == VA_STATUS_SUCCESS;
			}
		}
	}
	osd_mutex.unlock();

	for ( int i = 0 ; i <= 1 ; ++i )
	{
		const int err = vaPutSurface
		(
			VADisp, id, winId(),
			srcQRect.x(), srcQRect.y(), srcQRect.width(), srcQRect.height(),
			dstQRect.x(), dstQRect.y(), dstQRect.width(), dstQRect.height(),
			NULL, 0, field | VA_CLEAR_DRAWABLE
		);
		if ( err != VA_STATUS_SUCCESS )
			QMPlay2Core.log( QString( "vaPutSurface() - " ) + vaErrorStr( err ) );
		VASurfaceStatus status;
		if ( !isVDPAU || !ok || i || vaQuerySurfaceStatus( VADisp, id, &status ) != VA_STATUS_SUCCESS || status != VASurfaceReady )
			break;
	}

	if ( associated )
		vaDeassociateSubpicture( VADisp, vaSubpicID, &id, 1 );
}

void VAApiWriter::resizeEvent( QResizeEvent * )
{
	Functions::getImageSize( aspect_ratio, zoom, width(), height(), W, H, &X, &Y, &dstQRect, &outW, &outH, &srcQRect );
}
void VAApiWriter::paintEvent( QPaintEvent * )
{
	draw();
}
bool VAApiWriter::event( QEvent *e )
{
	/* Pass gesture event to the parent */
	if ( e->type() == QEvent::Gesture )
		return qApp->notify( parent(), e );
	return QWidget::event( e );
}

QPaintEngine *VAApiWriter::paintEngine() const
{
	return NULL;
}

void VAApiWriter::clearRGBImage()
{
	if ( vaSubpicID )
		vaDestroySubpicture( VADisp, vaSubpicID );
	if ( vaImg.image_id )
		vaDestroyImage( VADisp, vaImg.image_id );
	vaImg.image_id = vaSubpicID = 0;
}
void VAApiWriter::clr_vpp()
{
#ifdef HAVE_VPP
	if ( use_vpp )
	{
		for ( int i = 0 ; i < VAProcFilterCount ; ++i )
			if ( vpp_buffers[ i ] != VA_INVALID_ID )
				vaDestroyBuffer( VADisp, vpp_buffers[ i ] );
		if ( id_vpp != VA_INVALID_SURFACE )
			vaDestroySurfaces( VADisp, &id_vpp, 1 );
		if ( context_vpp )
			vaDestroyContext( VADisp, context_vpp );
		if ( config_vpp )
			vaDestroyConfig( VADisp, config_vpp );
		use_vpp = false;
	}
	id_vpp = forward_reference = VA_INVALID_SURFACE;
	for ( int i = 0 ; i < VAProcFilterCount ; ++i )
		vpp_buffers[ i ] = VA_INVALID_ID;
	vpp_second = false;
	context_vpp = 0;
	config_vpp = 0;
#endif
}
void VAApiWriter::clr()
{
	clearRGBImage();
	clr_vpp();
	if ( VADisp )
	{
		if ( surfacesCreated )
			vaDestroySurfaces( VADisp, surfaces, surfacesCount );
		if ( context )
			vaDestroyContext( VADisp, context );
		if ( config )
			vaDestroyConfig( VADisp, config );
	}
	surfacesCreated = ok = paused = false;
	surfacesQueue.clear();
	profile = ( VAProfile )-1; //VAProfileNone
	delete rgbImgFmt;
	rgbImgFmt = NULL;
	id = VA_INVALID_SURFACE;
	field = -1;
	context = 0;
	config = 0;
}
