#include <SndResampler.hpp>

#include <QByteArray>

#include <math.h>

extern "C"
{
#ifdef QMPLAY2_AVRESAMPLE
	#include <libavresample/avresample.h>
	#include <libavutil/samplefmt.h>
#else
	#include <libswresample/swresample.h>
	#include <libavutil/audioconvert.h>
#endif
	#include <libavutil/opt.h>
}

bool SndResampler::create( int _src_samplerate, int _src_channels, int _dst_samplerate, int _dst_channels )
{
	destroy();

	src_samplerate = _src_samplerate;
	src_channels  = _src_channels;
	dst_samplerate = _dst_samplerate;
	dst_channels = _dst_channels;
	int src_chn_layout = av_get_default_channel_layout( src_channels );
	int dst_chn_layout = av_get_default_channel_layout( dst_channels );
	if ( !src_samplerate || !dst_samplerate || !src_chn_layout || !dst_chn_layout )
		return false;

#ifdef QMPLAY2_AVRESAMPLE
	snd_convert_ctx = avresample_alloc_context();
	av_opt_set_int( snd_convert_ctx, "in_channel_layout",  src_chn_layout,    0 );
	av_opt_set_int( snd_convert_ctx, "out_channel_layout", dst_chn_layout,    0 );
	av_opt_set_int( snd_convert_ctx, "in_sample_rate",     src_samplerate,    0 );
	av_opt_set_int( snd_convert_ctx, "out_sample_rate",    dst_samplerate,    0 );
	av_opt_set_int( snd_convert_ctx, "in_sample_fmt",      AV_SAMPLE_FMT_FLT, 0 );
	av_opt_set_int( snd_convert_ctx, "out_sample_fmt",     AV_SAMPLE_FMT_FLT, 0 );
	av_opt_set_int( snd_convert_ctx, "linear_interp",      true,              0 );
	if ( dst_channels > src_channels )
	{
		double matrix[ dst_channels ][ src_channels ];
		memset( matrix, 0, sizeof matrix );
		for ( int i = 0, c = 0 ; i < dst_channels ; i++ )
		{
			matrix[ i ][ c ] = 1.0;
			c = ( c + 1 ) % src_channels;
		}
		avresample_set_matrix( snd_convert_ctx, ( const double * )matrix, src_channels );
	}
	if ( avresample_open( snd_convert_ctx ) )
		destroy();
#else
	snd_convert_ctx = swr_alloc_set_opts( NULL, dst_chn_layout, AV_SAMPLE_FMT_FLT, dst_samplerate, src_chn_layout, AV_SAMPLE_FMT_FLT, src_samplerate, 0, NULL );
	if ( snd_convert_ctx )
	{
		if ( dst_channels > src_channels )
		{
			channel_map.fill( -1, dst_channels /*SWR_CH_MAX*/ );
			for ( int i = 0, c = 0 ; i < dst_channels ; ++i )
			{
				channel_map[ i ] = c;
				c = ( c + 1 ) % src_channels;
			}
			av_opt_set_int( snd_convert_ctx, "icl", dst_chn_layout, 0 ); //Input Channel Layout na takie samo jak Output, ale ilość jest różna
			av_opt_set_int( snd_convert_ctx, "uch", dst_channels,  0 ); //Used Channel Count
			swr_set_channel_mapping( snd_convert_ctx, channel_map.data() );
		}
		av_opt_set_int( snd_convert_ctx, "linear_interp", true, 0 );
		if ( swr_init( snd_convert_ctx ) )
			destroy();
	}
#endif

	return isOpen();
}
void SndResampler::convert( const QByteArray &src, QByteArray &dst )
{
	const int in_size = src.size() / src_channels / sizeof( float );
	const int out_size = ceil( in_size * ( double )dst_samplerate / ( double )src_samplerate );

	dst.reserve( out_size * sizeof( float ) * dst_channels );

	uint8_t *in[]  = { ( uint8_t * )src.data() };
	uint8_t *out[] = { ( uint8_t * )dst.data() };

#ifdef QMPLAY2_AVRESAMPLE
	const int converted = avresample_convert( snd_convert_ctx, out, 1, out_size, in, 1, in_size );
#else
	const int converted = swr_convert( snd_convert_ctx, out, out_size, ( const uint8_t ** )in, in_size );
#endif
	if ( converted > 0 )
		dst.resize( converted * sizeof( float ) * dst_channels );
	else
		dst.clear();
}
void SndResampler::destroy()
{
#ifdef QMPLAY2_AVRESAMPLE
	avresample_free( &snd_convert_ctx );
#else
	swr_free( &snd_convert_ctx );
#endif
}
