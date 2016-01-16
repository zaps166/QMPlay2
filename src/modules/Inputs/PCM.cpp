#include <PCM.hpp>

#include <ByteArray.hpp>
#include <Packet.hpp>
#include <Reader.hpp>

/**/

static const unsigned char bytes[ PCM::FORMAT_COUNT ] =
{
	1, 1, 2, 3, 4, 4
};

/**/

PCM::PCM( Module &module )
{
	SetModule( module );
}

bool PCM::set()
{
	FORMAT format = ( FORMAT )sets().getInt( "PCM/format" );
	int channels = sets().getInt( "PCM/chn" );
	int samplerate = sets().getInt( "PCM/srate" );
	int fileoffset = sets().getInt( "PCM/offset" );

	if ( reader && ( fmt != format || chn != channels || srate != samplerate || offset != fileoffset ) )
		return false;

	bigEndian = sets().getBool( "PCM/BE" );
	if ( !reader )
	{
		fmt = format;
		chn = channels;
		srate = samplerate;
		offset = fileoffset;
	}

	return sets().getBool( "PCM" );
}

QString PCM::name() const
{
	return PCMName;
}
QString PCM::title() const
{
	return QString();
}
double PCM::length() const
{
	return len;
}
int PCM::bitrate() const
{
	return 8 * ( srate * chn * bytes[ fmt ] ) / 1000;
}

bool PCM::seek( int s )
{
	int filePos = offset + s * srate * chn * bytes[ fmt ];
	return reader->seek( filePos );
}
bool PCM::read( Packet &decoded, int &idx )
{
	if ( reader.isAborted() )
		return false;

	decoded.ts = ( reader->pos() - offset ) / ( double )bytes[ fmt ] / chn / srate;

	QByteArray dataBA = reader->read( chn * bytes[ fmt ] * 256 );
	const int samples_with_channels = dataBA.size() / bytes[ fmt ];
	decoded.resize( samples_with_channels * sizeof( float ) );
	float *decoded_data = ( float * )decoded.data();
	ByteArray data( dataBA.data(), dataBA.size(), bigEndian );
	switch ( fmt )
	{
		case PCM_U8:
			for ( int i = 0; i < samples_with_channels; i++ )
				decoded_data[ i ] = ( data.getBYTE() - 0x7F ) / 128.0f;
			break;
		case PCM_S8:
			for ( int i = 0; i < samples_with_channels; i++ )
				decoded_data[ i ] = ( qint8 )data.getBYTE() / 128.0f;
			break;
		case PCM_S16:
			for ( int i = 0; i < samples_with_channels; i++ )
				decoded_data[ i ] = ( qint16 )data.getWORD() / 32768.0f;
			break;
		case PCM_S24:
			for ( int i = 0; i < samples_with_channels; i++ )
				decoded_data[ i ] = ( qint32 )data.get24bAs32b() / 2147483648.0f;
			break;
		case PCM_S32:
			for ( int i = 0; i < samples_with_channels; i++ )
				decoded_data[ i ] = ( qint32 )data.getDWORD() / 2147483648.0f;
			break;
		case PCM_FLT:
			for ( int i = 0; i < samples_with_channels; i++ )
				decoded_data[ i ] = data.getFloat();
			break;
		default:
			break;
	}

	idx = 0;
	decoded.duration = decoded.size() / chn / sizeof( float ) / ( double )srate;

	return decoded.size();
}
void PCM::abort()
{
	reader.abort();
}

bool PCM::open( const QString &url )
{
	if ( Reader::create( url, reader ) && ( !offset || reader->seek( offset ) ) )
	{
		if ( reader->size() >= 0 )
			len = ( double )reader->size() / srate / chn / bytes[ fmt ];
		else
			len = -1.0;

		streams_info += new StreamInfo( srate, chn );
		return true;
	}
	return false;
}
