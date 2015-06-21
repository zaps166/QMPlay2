#include <Rayman2.hpp>

#include <ByteArray.hpp>
#include <Reader.hpp>

/**/

static float decode( unsigned char nibble, short &stepIndex, int &predictor )
{
	static const unsigned short ima_step_table[ 89 ] =
	{
		7,     8,     9,     10,    11,    12,    13,    14,    16,    17,
		19,    21,    23,    25,    28,    31,    34,    37,    41,    45,
		50,    55,    60,    66,    73,    80,    88,    97,    107,   118,
		130,   143,   157,   173,   190,   209,   230,   253,   279,   307,
		337,   371,   408,   449,   494,   544,   598,   658,   724,   796,
		876,   963,   1060,  1166,  1282,  1411,  1552,  1707,  1878,  2066,
		2272,  2499,  2749,  3024,  3327,  3660,  4026,  4428,  4871,  5358,
		5894,  6484,  7132,  7845,  8630,  9493,  10442, 11487, 12635, 13899,
		15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
	};
	static const char ima_index_table[ 8 ] = { -1, -1, -1, -1, 2, 4, 6, 8 };

	int step = ima_step_table[ stepIndex ];
	int diff = step >> 3;

	if ( nibble & 1 )
		diff += step >> 2;
	if ( nibble & 2 )
		diff += step >> 1;
	if ( nibble & 4 )
		diff += step;
	if ( nibble & 8 )
		diff = -diff;

	predictor += diff;
	if ( predictor > 32767 )
		predictor = 32767;
	else if ( predictor < -32768 )
		predictor = -32768;

	stepIndex += ima_index_table[ nibble & 7 ];
	if ( stepIndex > 88 )
		stepIndex = 88;
	else if ( stepIndex < 0 )
		stepIndex = 0;

	return predictor / 32768.0f;
}

static inline void writeSample( QByteArray &decoded, float sample )
{
	decoded.append( ( const char * )&sample, sizeof sample );
}

/**/

Rayman2::Rayman2( Module &module )
{
	SetModule( module );
}

bool Rayman2::set()
{
	return sets().getBool( "Rayman2" );
}

QString Rayman2::name() const
{
	return Rayman2Name" - IMA ADPCM";
}
QString Rayman2::title() const
{
	return QString();
}
double Rayman2::length() const
{
	return len;
}
int Rayman2::bitrate() const
{
	return 8 * ( srate * chn / 2 ) / 1000;
}

bool Rayman2::seek( int s, bool )
{
	int filePos = 0x64 + s * srate * chn / 2;
	if ( reader->pos() > filePos ) //backward
	{
		if ( !reader->seek( 0 ) )
			return false;
		readHeader( reader->read( 0x64 ) );
	}
	QByteArray sampleCodes = reader->read( filePos - reader->pos() );
	if ( filePos - reader->pos() != 0 )
		return false;
	for ( int i = 0 ; !reader.isAborted() && i < sampleCodes.size() ; i += chn )
	{
		for ( int c = 0 ; c < chn ; ++c )
		{
			decode( sampleCodes[ i+c ] >> 4, stepIndex[ c ], predictor[ c ] );
			decode( sampleCodes[ i+c ], stepIndex[ c ], predictor[ c ] );
		}
	}
	return true;
}
bool Rayman2::read( QByteArray &decoded, int &idx, TimeStamp &ts, double &duration )
{
	if ( reader.isAborted() )
		return false;

	ts = ( reader->pos() - 0x64 ) * 2.0 / chn / srate;

	QByteArray sampleCodes = reader->read( chn * 256 );
	for ( int i = 0 ; !reader.isAborted() && i + chn <= sampleCodes.size() ; i += chn )
	{
		for ( int c = 0 ; c < chn ; ++c )
			writeSample( decoded, decode( sampleCodes[ i+c ] >> 4, stepIndex[ c ], predictor[ c ] ) );
		for ( int c = 0 ; c < chn ; ++c )
			writeSample( decoded, decode( sampleCodes[ i+c ], stepIndex[ c ], predictor[ c ] ) );
	}

	if ( !decoded.size() )
		return false;

	idx = 0;
	duration = decoded.size() / chn / sizeof( float ) / ( double )srate;

	return !reader.isAborted();
}
void Rayman2::abort()
{
	reader.abort();
}

bool Rayman2::open( const QString &url )
{
	if ( Reader::create( url, reader ) )
	{
		QByteArray header = reader->read( 0x64 );
		if ( header.size() == 0x64 )
		{
			const char *data = header.constData();
			readHeader( data );
			if ( srate && ( chn == 1 || chn == 2 ) && !strncmp( data + 0x14, "vs12", 4 ) && !strncmp( data + 0x60, "DATA", 4 ) )
			{
				StreamInfo *streamInfo = new StreamInfo;
				streamInfo->type = QMPLAY2_TYPE_AUDIO;
				streamInfo->is_default = true;
				streamInfo->sample_rate = srate;
				streamInfo->channels = chn;
				streams_info += streamInfo;
				return true;
			}
		}
	}
	return false;
}

void Rayman2::readHeader( const char *_data )
{
	ByteArray data( _data, 0x64 );
	data = 0x02;
	chn = data.getWORD();
	srate = data.getDWORD();
	data = 0x1C;
	len = data.getDWORD() / ( double )srate;
	data = 0x2C;
	if ( chn == 2 )
	{
		predictor[ 1 ] = data.getDWORD();
		stepIndex[ 1 ] = data.getWORD();
		data += 0x06;
	}
	predictor[ 0 ] = data.getDWORD();
	stepIndex[ 0 ] = data.getWORD();
}
