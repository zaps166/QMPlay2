#include <FFDecVDPAU_NW.hpp>
#include <FFCommon.hpp>

#include <HWAccelHelper.hpp>
#include <StreamInfo.hpp>
#include <VideoFrame.hpp>

#include <QQueue>

extern "C"
{
	#include <libavformat/avformat.h>
	#include <libavcodec/vdpau.h>
}

#include <vdpau/vdpau_x11.h>

class VDPAU : public HWAccelHelper
{
public:
	int id;

	VDPAU( int w, int h, const char *codec_name ) :
		mustDelete( false ), mustntDelete( false ),
		display( NULL ),
		device( 0 ),
		decoder( 0 ),
		vpd_decoder_render( NULL ),
		vdp_device_destroy( NULL )
	{
		memset( surfaces, 0, sizeof surfaces );
		if ( vdp_device_create_x11( ( display = XOpenDisplay( NULL ) ), 0, &device, &vdp_get_proc_address ) == VDP_STATUS_OK )
		{
			if
			(
				vdp_get_proc_address( device, VDP_FUNC_ID_DECODER_CREATE, ( void ** )&vdp_decoder_create ) == VDP_STATUS_OK &&
				vdp_get_proc_address( device, VDP_FUNC_ID_VIDEO_SURFACE_GET_BITS_Y_CB_CR, ( void ** )&vdp_surface_get_bits ) == VDP_STATUS_OK &&
				vdp_get_proc_address( device, VDP_FUNC_ID_VIDEO_SURFACE_CREATE, ( void ** )&vdp_video_surface_create ) == VDP_STATUS_OK &&
				vdp_get_proc_address( device, VDP_FUNC_ID_VIDEO_SURFACE_DESTROY, ( void ** )&vdp_video_surface_destroy ) == VDP_STATUS_OK &&
				vdp_get_proc_address( device, VDP_FUNC_ID_DEVICE_DESTROY, ( void ** )&vdp_device_destroy ) == VDP_STATUS_OK &&
				vdp_get_proc_address( device, VDP_FUNC_ID_DECODER_DESTROY, ( void ** )&vdp_decoder_destroy ) == VDP_STATUS_OK &&
				vdp_get_proc_address( device, VDP_FUNC_ID_DECODER_RENDER, ( void ** )&vpd_decoder_render ) == VDP_STATUS_OK &&
				vdp_get_proc_address( device, VDP_FUNC_ID_DECODER_QUERY_CAPABILITIES, ( void ** )&vdp_decoder_query_capabilities ) == VDP_STATUS_OK
			)
			{
				quint32 out[ 4 ];
				VdpBool isSupported;
				QList< VdpDecoderProfile > profileList;
				for ( VdpDecoderProfile p = VDP_DECODER_PROFILE_MPEG1 ; p <= VDP_DECODER_PROFILE_MPEG4_PART2_ASP ; ++p )
					if ( vdp_decoder_query_capabilities( device, p, &isSupported, out + 0, out + 1, out + 2, out + 3 ) == VDP_STATUS_OK && isSupported )
						profileList.push_back( p );
				if ( !profileList.isEmpty() )
				{
					VdpDecoderProfile p = -1;
					if ( !qstrcmp( codec_name, "h264" ) )
					{
						if ( profileList.contains( VDP_DECODER_PROFILE_H264_HIGH ) )
							p = VDP_DECODER_PROFILE_H264_HIGH;
						else if ( profileList.contains( VDP_DECODER_PROFILE_H264_MAIN ) )
							p = VDP_DECODER_PROFILE_H264_MAIN;
						else if ( profileList.contains( VDP_DECODER_PROFILE_H264_BASELINE ) )
							p = VDP_DECODER_PROFILE_H264_BASELINE;
					}
					else if ( !qstrcmp( codec_name, "mpeg2video" ) )
					{
						if ( profileList.contains( VDP_DECODER_PROFILE_MPEG2_MAIN ) )
							p = VDP_DECODER_PROFILE_MPEG2_MAIN;
						else if ( profileList.contains( VDP_DECODER_PROFILE_MPEG2_SIMPLE ) )
							p = VDP_DECODER_PROFILE_MPEG2_SIMPLE;
					}
					else if ( !qstrcmp( codec_name, "mpeg4" ) )
					{
						if ( profileList.contains( VDP_DECODER_PROFILE_MPEG4_PART2_ASP ) )
							p = VDP_DECODER_PROFILE_MPEG4_PART2_ASP;
						else if ( profileList.contains( VDP_DECODER_PROFILE_MPEG4_PART2_SP ) )
							p = VDP_DECODER_PROFILE_MPEG4_PART2_SP;
					}
					else if ( !qstrcmp( codec_name, "vc1" ) )
					{
						if ( profileList.contains( VDP_DECODER_PROFILE_VC1_ADVANCED ) )
							p = VDP_DECODER_PROFILE_VC1_ADVANCED;
						else if ( profileList.contains( VDP_DECODER_PROFILE_VC1_MAIN ) )
							p = VDP_DECODER_PROFILE_VC1_MAIN;
						else if ( profileList.contains( VDP_DECODER_PROFILE_VC1_SIMPLE ) )
							p = VDP_DECODER_PROFILE_VC1_SIMPLE;
					}
					else if ( !qstrcmp( codec_name, "mpeg1video" ) && profileList.contains( VDP_DECODER_PROFILE_MPEG1 ) )
						p = VDP_DECODER_PROFILE_MPEG1;
					if ( vdp_decoder_create( device, p, w, h, 16, &decoder ) == VDP_STATUS_OK )
					{
						for ( int i = 0 ; i < surfacesCount ; ++i )
						{
							if ( vdp_video_surface_create( device, VDP_CHROMA_TYPE_420, w, h, &surfaces[ i ] ) == VDP_STATUS_OK )
								surfacesQueue.enqueue( surfaces[ i ] );
						}
					}
				}
			}
		}
	}
	~VDPAU()
	{
		if ( device )
		{
			if ( decoder )
			{
				for ( int i = 0 ; i < surfacesCount ; ++i )
					vdp_video_surface_destroy( surfaces[ i ] );
				vdp_decoder_destroy( decoder );
			}
			if ( vdp_device_destroy )
				vdp_device_destroy( device );
		}
		if ( display )
			XCloseDisplay( display );
	}

	QMPlay2SurfaceID getSurface()
	{
		mustntDelete = true;
		return surfacesQueue.isEmpty() ? QMPlay2InvalidSurfaceID : surfacesQueue.dequeue();
	}
	void putSurface( QMPlay2SurfaceID id )
	{
		surfacesQueue.enqueue( id );
		if ( mustDelete && surfacesQueue.count() == surfacesCount )
			delete this;
	}

	bool mustDelete, mustntDelete;

	static const int surfacesCount = 18;
	QQueue< VdpVideoSurface > surfacesQueue;
	VdpVideoSurface surfaces[ surfacesCount ];

	Display *display;
	VdpDevice device;
	VdpDecoder decoder;
	VdpDecoderRender *vpd_decoder_render;
	VdpDecoderCreate *vdp_decoder_create;
	VdpDeviceDestroy *vdp_device_destroy;
	VdpDecoderDestroy *vdp_decoder_destroy;
	VdpGetProcAddress *vdp_get_proc_address;
	VdpVideoSurfaceCreate *vdp_video_surface_create;
	VdpVideoSurfaceGetBitsYCbCr *vdp_surface_get_bits;
	VdpVideoSurfaceDestroy *vdp_video_surface_destroy;
	VdpDecoderQueryCapabilities *vdp_decoder_query_capabilities;
};

static AVPixelFormat get_format( AVCodecContext *, const AVPixelFormat * )
{
	return AV_PIX_FMT_VDPAU;
}

/**/

FFDecVDPAU_NW::FFDecVDPAU_NW( QMutex &avcodec_mutex, Module &module ) :
	FFDecHWAccel( avcodec_mutex )
{
	SetModule( module );
}

FFDecVDPAU_NW::~FFDecVDPAU_NW()
{
	if ( codec_ctx && codec_ctx->opaque )
	{
		VDPAU *vdpau = ( VDPAU * )codec_ctx->opaque;
		if ( vdpau->mustntDelete )
			vdpau->mustDelete = true;
		else
			delete vdpau;
	}
}

bool FFDecVDPAU_NW::set()
{
	return sets().getBool( "DecoderVDPAUEnabled" );
}

QString FFDecVDPAU_NW::name() const
{
	return "FFMpeg/VDPAU";
}

int FFDecVDPAU_NW::decode( Packet &encodedPacket, QByteArray &decoded, bool flush, unsigned hurry_up )
{
	AVPacket packet;
	int frameFinished = 0;
	decodeFirstStep( packet, encodedPacket, flush );
	const int bytes_consumed = avcodec_decode_video2( codec_ctx, frame, &frameFinished, &packet );
	if ( frameFinished && ~hurry_up )
	{
		VideoFrame *videoFrame = VideoFrame::create( decoded, streamInfo->W, streamInfo->H, frame->interlaced_frame, frame->top_field_first );
		if ( ( ( VDPAU * )codec_ctx->opaque )->vdp_surface_get_bits( ( unsigned long )frame->data[ 3 ], VDP_YCBCR_FORMAT_YV12, ( void * const * )videoFrame->data, ( quint32 * )videoFrame->linesize ) == VDP_STATUS_OK )
		{
			quint8 *tmp = videoFrame->data[ 1 ];
			videoFrame->data[ 1 ] = videoFrame->data[ 2 ];
			videoFrame->data[ 2 ] = tmp;
		}
		else
			VideoFrame::unref( decoded );
	}
	if ( frameFinished )
	{
		if ( frame->best_effort_timestamp != QMPLAY2_NOPTS_VALUE )
			encodedPacket.ts = frame->best_effort_timestamp * time_base;
	}
	else
		encodedPacket.ts.setInvalid();
	return bytes_consumed > 0 ? bytes_consumed : 0;
}

bool FFDecVDPAU_NW::open( StreamInfo *streamInfo, Writer * )
{
	if ( canUseHWAccel( streamInfo ) )
	{
		AVCodec *codec = init( streamInfo );
		if ( codec && hasHWAccel( "vdpau" ) )
		{
			VDPAU *vdpau = new VDPAU( codec_ctx->width, codec_ctx->height, avcodec_get_name( codec_ctx->codec_id ) );
			if ( vdpau->surfacesQueue.count() == VDPAU::surfacesCount )
			{
				codec_ctx->hwaccel_context = av_mallocz( sizeof( AVVDPAUContext ) );
				( ( AVVDPAUContext * )codec_ctx->hwaccel_context )->decoder = vdpau->decoder;
				( ( AVVDPAUContext * )codec_ctx->hwaccel_context )->render  = vdpau->vpd_decoder_render;
				codec_ctx->thread_count   = 1;
				codec_ctx->get_buffer2    = HWAccelHelper::get_buffer;
				codec_ctx->get_format     = get_format;
				codec_ctx->slice_flags    = SLICE_FLAG_CODED_ORDER | SLICE_FLAG_ALLOW_FIELD;
				codec_ctx->opaque         = dynamic_cast< HWAccelHelper * >( vdpau );
				if ( openCodec( codec ) )
					return true;
			}
			else
				delete vdpau;
		}
	}
	return false;
}
