#include <PortAudioWriter.hpp>
#include <QMPlay2Core.hpp>

PortAudioWriter::PortAudioWriter( Module &module ) :
	stream( NULL ),
	sample_rate( 0 ),
	err( false )
{
	addParam( "delay" );
	addParam( "chn" );
	addParam( "rate" );

	memset( &outputParameters, 0, sizeof( outputParameters ) );
	outputParameters.sampleFormat = paFloat32;
	outputParameters.hostApiSpecificStreamInfo = NULL;

	SetModule( module );
}
PortAudioWriter::~PortAudioWriter()
{
	close();
}

bool PortAudioWriter::set()
{
	bool restartPlaying = false;
	int devIdx = PortAudioCommon::getDeviceIndexForOutput( sets().getString( "OutputDevice" ) );
	double delay = sets().getDouble( "Delay" );
	if ( outputParameters.device != devIdx )
	{
		outputParameters.device = devIdx;
		restartPlaying = true;
	}
	if ( outputParameters.suggestedLatency != delay )
	{
		outputParameters.suggestedLatency = delay;
		restartPlaying = true;
	}
	return !restartPlaying && sets().getBool( "WriterEnabled" );
}

bool PortAudioWriter::readyWrite() const
{
	return stream && !err;
}

bool PortAudioWriter::processParams( bool *paramsCorrected )
{
	bool resetAudio = false;

	int chn = getParam( "chn" ).toInt();
	if ( paramsCorrected )
	{
		const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo( outputParameters.device );
		if ( deviceInfo && deviceInfo->maxOutputChannels < chn )
		{
			modParam( "chn", ( chn = deviceInfo->maxOutputChannels ) );
			*paramsCorrected = true;
		}
	}
	if ( outputParameters.channelCount != chn )
	{
		resetAudio = true;
		outputParameters.channelCount = chn;
	}

	int rate = getParam( "rate" ).toInt();
	if ( sample_rate != rate )
	{
		resetAudio = true;
		sample_rate = rate;
	}

	if ( resetAudio || err )
	{
		close();
		err = Pa_OpenStream( &stream, NULL, &outputParameters, sample_rate, 0, 0, NULL, NULL );
		if ( !err )
		{
			outputLatency = Pa_GetStreamInfo( stream )->outputLatency;
			modParam( "delay", outputLatency );
		}
		else
			QMPlay2Core.logError( "PortAudio :: " + tr( "Nie można otworzyć strumienia wyjścia dźwięku" ) );
	}
#ifdef Q_OS_WIN
	else
		Pa_AbortStream( stream );
#endif

	return readyWrite();
}
qint64 PortAudioWriter::write( const QByteArray &arr )
{
	if ( !readyWrite() )
		return 0;

	if ( Pa_IsStreamStopped( stream ) )
		Pa_StartStream( stream );

#ifndef Q_OS_MAC //?
	int diff = Pa_GetStreamWriteAvailable( stream ) - outputLatency * sample_rate;
	if ( diff > 0 )
		Pa_WriteStream( stream, QByteArray( diff * outputParameters.channelCount * sizeof( float ), 0 ).constData(), diff );
#endif

#ifdef Q_OS_LINUX
	int chn = outputParameters.channelCount;
	if ( chn == 6 || chn == 8 )
	{
		float *audio_buffer = ( float * )arr.data();
		for ( int i = 0 ; i < arr.size() / 4 ; i += chn )
		{
			float tmp = audio_buffer[i+2];
			audio_buffer[i+2] = audio_buffer[i+4];
			audio_buffer[i+4] = tmp;
			tmp = audio_buffer[i+3];
			audio_buffer[i+3] = audio_buffer[i+5];
			audio_buffer[i+5] = tmp;
		}
	}
#endif

	int e = Pa_WriteStream( stream, arr.constData(), arr.size() / outputParameters.channelCount / sizeof( float ) );
	if ( e == paUnanticipatedHostError )
	{
		QMPlay2Core.logError( "PortAudio :: " + tr( "Błąd podczas odtwarzania" ) );
		err = true;
		return 0;
	}

	return arr.size();
}
void PortAudioWriter::pause()
{
	if ( stream )
		Pa_StopStream( stream );
}

QString PortAudioWriter::name() const
{
	return PortAudioWriterName;
}

bool PortAudioWriter::open()
{
	return true;
}

/**/

void PortAudioWriter::close()
{
	if ( stream )
	{
		Pa_CloseStream( stream );
		stream = NULL;
	}
	err = false;
}
