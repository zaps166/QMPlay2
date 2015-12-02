#include <ToneGenerator.hpp>

#include <Packet.hpp>

#include <QUrl>
#if QT_VERSION < 0x050000
	#define QUrlQuery( url ) url
#else
	#include <QUrlQuery>
#endif

#include <math.h>

ToneGenerator::ToneGenerator( Module &module ) :
	aborted( false ), metadata_changed( false ), fromUrl( false ), pos( 0.0 ), srate( 0 )
{
	SetModule( module );
}

bool ToneGenerator::set()
{
	bool restartPlaying = false;
	if ( !fromUrl )
	{
		const QStringList newFreqs = sets().getString( "ToneGenerator/freqs" ).split( ',' );
		restartPlaying = freqs.size() && ( srate != sets().getUInt( "ToneGenerator/srate" ) || freqs.size() != newFreqs.size() );
		if ( !restartPlaying )
		{
			srate = sets().getUInt( "ToneGenerator/srate" );
			if ( !freqs.size() )
				freqs.resize( qMin( 8, newFreqs.size() ) );
			else
				metadata_changed = true;
			for ( int i = 0 ; i < freqs.size() ; ++i )
				freqs[ i ] = newFreqs[ i ].toInt();
		}
	}
	return !restartPlaying;
}

bool ToneGenerator::metadataChanged() const
{
	if ( metadata_changed )
	{
		metadata_changed = false;
		return true;
	}
	return false;
}

QString ToneGenerator::name() const
{
	return tr( "Generator częstotliwości" );
}
QString ToneGenerator::title() const
{
	QString t;
	foreach ( uint hz, freqs )
		t += "   - " + QString::number( hz ) + tr( "Hz" ) + "\n";
	t.chop( 1 );
	return tr( "Generator częstotliwości" ) + " (" + QString::number( srate ) + tr( "Hz" ) + "):\n" + t;
}
double ToneGenerator::length() const
{
	return -1;
}
int ToneGenerator::bitrate() const
{
	return 8 * ( srate * freqs.size() * sizeof( float ) ) / 1000;
}

bool ToneGenerator::dontUseBuffer() const
{
	return true;
}

bool ToneGenerator::seek( int, bool )
{
	return false;
}
bool ToneGenerator::read( Packet &decoded, int &idx )
{
	if ( aborted )
		return false;

	int chn = freqs.size();

	decoded.resize( sizeof( float ) * chn * srate );
	float *samples = ( float * )decoded.data();

	for ( uint i = 0 ; i < srate * chn ; i += chn )
		for ( int c = 0 ; c < chn ; ++c )
			samples[ i+c ] = sin( 2.0 * M_PI * freqs[ c ] * i / srate / chn ); //don't use sinf()!

	idx = 0;
	decoded.ts = pos;
	decoded.duration = 1.0;
	pos += decoded.duration;

	return true;
}
void ToneGenerator::abort()
{
	aborted = true;
}

bool ToneGenerator::open( const QString &_url )
{
	if ( _url.indexOf( ToneGeneratorName"://" ) != 0 )
		return false;
	QUrl url( "?" + QString( _url ).remove( 0, QString( ToneGeneratorName"://" ).length() ) );

	if ( !( fromUrl = url.toString() != "?" ) )
	{
		streams_info += new StreamInfo( srate, freqs.size() );
		return true;
	}

	srate = QUrlQuery( url ).queryItemValue( "samplerate" ).toUInt();
	if ( !srate )
		srate = 44100;

	freqs.clear();
	foreach ( const QString &freq, QUrlQuery( url ).queryItemValue( "freqs" ).split( ',', QString::SkipEmptyParts ) )
		freqs += freq.toInt();
	if ( freqs.isEmpty() )
	{
		bool ok;
		uint freq = url.toString().remove( '?' ).toUInt( &ok );
		if ( ok )
			freqs += freq;
		else
			freqs += 440;
	}

	if ( freqs.size() <= 8 )
	{
		streams_info += new StreamInfo( srate, freqs.size() );
		return true;
	}
	return false;
}
