#include <Equalizer.hpp>

extern "C"
{
	#include <libavutil/mem.h>
	#include <libavcodec/avfft.h>
}

static inline void fft_calc( FFTContext *fft_ctx, FFTComplex *cplx )
{
	av_fft_permute( fft_ctx, cplx );
	av_fft_calc( fft_ctx, cplx );
}

#include <math.h>

static inline float cosI( const float y1, const float y2, float p )
{
	p = ( 1.0f - cosf( p * M_PI ) ) / 2.0f;
	return y1 * ( 1.0f - p ) + y2 * p;
}

QVector< float > Equalizer::interpolate( const QVector< float > &src, const int len )
{
	QVector< float > dest( len );
	const int size = src.size();
	if ( size >= 2 )
	{
		const float mn = ( size - 1.0f ) / len;
		for ( int i = 0 ; i < len ; ++i )
		{
			int   x = i * mn;
			float p = i * mn - x;
			dest[ i ] = cosI( src[ x ], src[ x + 1 ], p );
		}
	}
	return dest;
}

QVector< float > Equalizer::freqs( Settings &sets )
{
	QVector< float > freqs( sets.getInt( "Equalizer/count" ) );
	const int minFreq = sets.getInt( "Equalizer/minFreq" ), maxFreq = sets.getInt( "Equalizer/maxFreq" );
	const float l = powf( maxFreq / minFreq, 1.0f / ( freqs.count() - 1 ) );
	for ( int i = 0 ; i < freqs.count() ; ++i )
		freqs[ i ] = minFreq * powf( l, i );
	return freqs;
}

Equalizer::Equalizer( Module &module ) :
	FFT_NBITS( 0 ), FFT_SIZE( 0 ), FFT_SIZE_2( 0 ),
	canFilter( false ), hasParameters( false ), enabled( false ),
	mutex( QMutex::Recursive ), fftIn( NULL ), fftOut( NULL )
{
	SetModule( module );
}
Equalizer::~Equalizer()
{
	alloc( false );
}

bool Equalizer::set()
{
	mutex.lock();
	enabled = sets().getBool( "Equalizer" );
	if ( FFT_NBITS && sets().getInt( "Equalizer/nbits" ) != FFT_NBITS )
		alloc( false );
	alloc( enabled && hasParameters );
	mutex.unlock();
	return true;
}

bool Equalizer::setAudioParameters( uchar chn, uint srate )
{
	hasParameters = chn && srate;
	if ( hasParameters )
	{
		this->chn = chn;
		this->srate = srate;
		clearBuffers();
	}
	alloc( enabled && hasParameters );
	return true;
}
int Equalizer::bufferedSamples() const
{
	if ( canFilter )
	{
		mutex.lock();
		int bufferedSamples = input.at( 0 ).size();
		mutex.unlock();
		return bufferedSamples;
	}
	return 0;
}
void Equalizer::clearBuffers()
{
	mutex.lock();
	if ( canFilter )
	{
		input.clear();
		input.resize( chn );
		last_samples.clear();
		last_samples.resize( chn );
	}
	mutex.unlock();
}
double Equalizer::filter( QByteArray &data, bool flush )
{
	if ( canFilter )
	{
		mutex.lock();

		if ( !flush )
		{
			float *samples = ( float * )data.data();
			const int size = data.size() / sizeof( float );
			for ( int c = 0 ; c < chn ; ++c ) //Buforowanie danych
				for ( int i = 0 ; i < size ; i += chn )
					input[ c ].append( samples[ c+i ] );
		}
		else for ( int c = 0 ; c < chn ; ++c ) //Dokładanie ciszy
			input[ c ].resize( FFT_SIZE );

		data.clear();
		const int chunks = input.at( 0 ).size() / FFT_SIZE_2 - 1;
		if ( chunks > 0 ) //Jeżeli jest wystarczająca ilość danych
		{
			data.resize( chn * sizeof( float ) * FFT_SIZE_2 * chunks );
			float *samples = ( float * )data.data();
			for ( int c = 0 ; c < chn ; ++c )
			{
				int pos = c;
				while ( input.at( c ).size() >= FFT_SIZE )
				{
					for ( int i = 0 ; i < FFT_SIZE ; ++i )
						complex[ i ] = ( FFTComplex ){ input.at( c ).at( i ), 0.0f };
					if ( !flush )
						input[ c ].remove( 0, FFT_SIZE_2 );
					else
						input[ c ].clear();

					fft_calc( fftIn, complex );
					for ( int i = 0 ; i < FFT_SIZE_2 ; ++i )
					{
						complex[            i ].re *= f.at( i ) * preamp;
						complex[            i ].im *= f.at( i ) * preamp;
						complex[ FFT_SIZE-1-i ].re *= f.at( i ) * preamp;
						complex[ FFT_SIZE-1-i ].im *= f.at( i ) * preamp;
					}
					fft_calc( fftOut, complex );

					if ( last_samples.at( c ).isEmpty() )
					{
						for ( int i = 0 ; i < FFT_SIZE_2 ; ++i, pos += chn )
							samples[ pos ] = complex[ i ].re / FFT_SIZE;
						last_samples[ c ].resize( FFT_SIZE_2 );
					}
					else for ( int i = 0 ; i < FFT_SIZE_2 ; ++i, pos += chn )
						samples[ pos ] = ( complex[ i ].re / FFT_SIZE ) * wind_f.at( i ) + last_samples.at( c ).at( i );

					for ( int i = FFT_SIZE_2 ; i < FFT_SIZE ; ++i )
						last_samples[ c ][ i-FFT_SIZE_2 ] = ( complex[ i ].re / FFT_SIZE ) * wind_f.at( i );
				}
			}
		}

		mutex.unlock();
		return FFT_SIZE / ( double )srate;
	}
	return 0.0;
}

void Equalizer::alloc( bool b )
{
	mutex.lock();
	if ( !b && ( fftIn || fftOut ) )
	{
		canFilter = false;
		FFT_NBITS = FFT_SIZE = FFT_SIZE_2 = 0;
		av_fft_end( fftIn );
		av_fft_end( fftOut );
		fftIn = NULL;
		fftOut = NULL;
		av_free( complex );
		complex = NULL;
		input.clear();
		last_samples.clear();
		wind_f.clear();
		f.clear();
	}
	else if ( b )
	{
		if ( !fftIn || !fftOut )
		{
			FFT_NBITS  = sets().getInt( "Equalizer/nbits" );
			FFT_SIZE   = 1 << FFT_NBITS;
			FFT_SIZE_2 = FFT_SIZE / 2;
			fftIn  = av_fft_init( FFT_NBITS, false );
			fftOut = av_fft_init( FFT_NBITS, true );
			complex = ( FFTComplex * )av_malloc( FFT_SIZE * sizeof( FFTComplex ) );
			input.resize( chn );
			last_samples.resize( chn );
			wind_f.resize( FFT_SIZE );
			for ( int i = 0 ; i < FFT_SIZE ; ++i )
				wind_f[ i ] = 0.5f - 0.5f * cos( 2.0f * M_PI * i / ( FFT_SIZE - 1 ) );
		}
		interpolateFilterCurve();
		canFilter = true;
	}
	mutex.unlock();
}
void Equalizer::interpolateFilterCurve()
{
	const int size = sets().getInt( "Equalizer/count" );
	preamp = sets().getInt( "Equalizer/-1" ) / 50.0f;
	QVector< float > src( size );
	for ( int i = 0 ; i < size ; ++i )
		src[ i ] = sets().getInt( "Equalizer/" + QString::number( i ) ) / 50.0f;
	const int len = FFT_SIZE_2;
	if ( f.size() != len )
		f.resize( len );
	if ( srate && size >= 2 )
	{
		QVector< float > freqs = Equalizer::freqs( sets() );
		const int maxHz = srate / 2;
		int x = 0, start = 0;
		for ( int i = 0 ; i < len ; ++i )
		{
			const int hz = ( i+1 ) * maxHz / len;
			for ( int j = x ; j < size ; ++j )
			{
				if ( freqs[ j ] >= hz )
					break;
				if ( x != j )
				{
					x = j;
					start = i;
				}
			}
			if ( x+1 < size )
				f[ i ] = cosI( src[ x ], src[ x + 1 ], ( i - start ) / ( len * freqs[ x + 1 ] / maxHz - 1 - start ) ); /* start / end */
			else
				f[ i ] = src[ x ];
		}
	}
}
