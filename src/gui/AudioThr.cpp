#include <AudioThr.hpp>

#include <Main.hpp>
#include <Writer.hpp>
#include <Decoder.hpp>
#include <Settings.hpp>
#include <Functions.hpp>
#include <PlayClass.hpp>
#include <AudioFilter.hpp>
#include <QMPlay2Extensions.hpp>

#include <QCoreApplication>

#include <math.h>

AudioThr::AudioThr( PlayClass &playC, const QStringList &pluginsName ) :
	AVThread( playC, "audio:", NULL, pluginsName )
{
	foreach ( QMPlay2Extensions *QMPlay2Ext, QMPlay2Extensions::QMPlay2ExtensionsList() )
		if ( QMPlay2Ext->isVisualization() )
			visualizations += QMPlay2Ext;
	visualizations.squeeze();

	filters = AudioFilter::open();

	connect( this, SIGNAL( pauseVisSig( bool ) ), this, SLOT( pauseVis( bool ) ) );
#ifdef Q_OS_WIN
	startTimer( 1000 );
#endif
}

void AudioThr::stop( bool terminate )
{
	foreach ( QMPlay2Extensions *vis, visualizations )
		vis->visState( false );
	foreach ( AudioFilter *filter, filters )
		delete filter;
	AVThread::stop( terminate );
}

bool AudioThr::setParams( uchar realChn, uint realSRate, uchar chn, uint sRate )
{
	doSilence = -1.0;
	lastSpeed = playC.speed;

	realChannels    = realChn;
	realSample_rate = realSRate;

	writer->modParam( "chn",  ( channels = chn ? chn : realChannels ) );
	writer->modParam( "rate", ( sample_rate = sRate ? sRate : realSRate ) );

	bool paramsCorrected = false;
	if ( writer->processParams( ( sRate && chn ) ? NULL : &paramsCorrected ) ) //nie pozwala na korektę jeżeli są wymuszone parametry
	{
		if ( paramsCorrected )
		{
			const uchar lastChn = channels;
			const uint lastSRate = sample_rate;
			channels = writer->getParam( "chn" ).toUInt();
			sample_rate = writer->getParam( "rate" ).toUInt();
			if ( ( !chn || channels == lastChn ) && ( !sRate || sample_rate == lastSRate ) )
				QMPlay2Core.logInfo( tr( "Moduł" ) + " \"" + writer->name() + "\" " + tr( "ustawił parametry na" ) + ": " + QString::number( channels ) + " " + tr( "kanały" ) + ", " + QString::number( sample_rate ) + " " + tr( "Hz" ) );
			else
			{
				QMPlay2Core.logError( tr( "Moduł" ) + " \"" + writer->name() + "\" " + tr( "wymaga zmiany jednego z wymuszonych parametrów, dźwięk wyłączony..." ) );
				return false;
			}
		}

		if ( !resampler_create() )
		{
			if ( paramsCorrected )
				return false;
			channels    = realChannels;
			sample_rate = realSample_rate;
		}

		foreach ( QMPlay2Extensions *vis, visualizations )
			vis->visState( true, realChannels, realSample_rate );
		foreach ( AudioFilter *filter, filters )
			filter->setAudioParameters( realChannels, realSample_rate );

		return true;
	}

	return false;
}

void AudioThr::silence( bool invert )
{
	if ( QMPlay2Core.getSettings().getBool( "Silence" ) && doSilence == -1.0 && isRunning() && ( invert || !playC.paused ) )
	{
		playC.doSilenceBreak = false;
		silence_step = ( invert ? -4.0 : 4.0 ) / sample_rate;
		silenceChMutex.lock();
		doSilence = invert ? -silence_step : ( 1.0 - silence_step );
		silenceChMutex.unlock();
		while ( !invert && doSilence > 0.0 && doSilence < 1.0 && isRunning() )
		{
			double lastDoSilence = doSilence;
			for ( int i = 0 ; i < 7 ; ++i )
			{
				qApp->processEvents();
				if ( playC.doSilenceBreak )
				{
					playC.doSilenceBreak = false;
					break;
				}
				Functions::s_wait( 0.015 );
			}
			if ( lastDoSilence == doSilence )
			{
				silenceChMutex.lock();
				doSilence = -1.0;
				silenceChMutex.unlock();
				break;
			}
		}
	}
}

void AudioThr::run()
{
	setPriority( QThread::HighestPriority );

	QMutex emptyBufferMutex;
	bool paused = false;
	tmp_br = tmp_time = 0;
#ifdef Q_OS_WIN
	canRefresh = false;
#endif
	while ( !br )
	{
		Packet packet;
		double delay = 0.0, audio_pts = 0.0; //"audio_pts" odporny na zerowanie przy przewijaniu
		Decoder *last_dec = dec;
		int bytes_consumed = -1;
		while ( !br && dec == last_dec )
		{
			playC.aPackets.lock();
			const bool hasAPackets = playC.aPackets.packetCount();
			bool hasBufferedSamples = false;
			if ( playC.endOfStream && !hasAPackets )
				foreach ( AudioFilter *filter, filters )
					if ( filter->bufferedSamples() )
					{
						hasBufferedSamples = true;
						break;
					}
			if ( playC.paused || ( !hasAPackets && !hasBufferedSamples ) || playC.waitForData )
			{
#ifdef Q_OS_WIN
				canRefresh = false;
#endif
				tmp_br = tmp_time = 0;
				if ( playC.paused && !paused )
				{
					doSilence = -1.0;
					writer->pause();
					paused = true;
					emit pauseVisSig( paused );
				}
				playC.aPackets.unlock();

				if ( !playC.paused )
					waiting = playC.fullBufferB = true;

				playC.emptyBufferCond.wait( &emptyBufferMutex, MUTEXWAIT_TIMEOUT );
				emptyBufferMutex.unlock();
				continue;
			}
			if ( paused )
			{
				paused = false;
				emit pauseVisSig( paused );
			}
			waiting = false;

			const bool flushAudio = playC.flushAudio;

			if ( !hasBufferedSamples && ( bytes_consumed < 0 || flushAudio ) )
				packet = playC.aPackets.dequeue();
			else if ( hasBufferedSamples )
				packet.ts = audio_pts + playC.audio_last_delay + delay; //szacowanie czasu
			playC.aPackets.unlock();
			playC.fullBufferB = true;

			mutex.lock();
			if ( br )
			{
				mutex.unlock();
				break;
			}

			QByteArray decoded;
			if ( !hasBufferedSamples )
			{
				bytes_consumed = dec->decode( packet, decoded, flushAudio );
				tmp_br += bytes_consumed;
			}

#ifndef Q_OS_WIN
			if ( tmp_time >= 1000.0 )
			{
				emit playC.updateBitrate( round( ( tmp_br << 3 ) / tmp_time ), -1, -1.0 );
				tmp_br = tmp_time = 0;
			}
#endif

			delay = writer->getParam( "delay" ).toDouble();
			foreach ( AudioFilter *filter, filters )
			{
				if ( flushAudio )
					filter->clearBuffers();
				delay += filter->filter( decoded, hasBufferedSamples );
			}

			if ( flushAudio )
				playC.flushAudio = false;

			int decodedSize = decoded.size();
			int decodedPos = 0;
			br2 = false;
			while ( decodedSize > 0 && !playC.paused && !br && !br2 )
			{
				const double max_len = 0.02; //TODO: zrobić opcje?
				const int chunk = qMin( decodedSize, ( int )( ceil( realSample_rate * max_len ) * realChannels * sizeof( float ) ) );
				const float vol = ( playC.muted || playC.vol == 0.0 ) ? 0.0 : playC.replayGain * ( playC.vol == 1.0 ? 1.0 : playC.vol * playC.vol );

				QByteArray decodedChunk;
				if ( vol == 0.0 )
					decodedChunk.fill( 0, chunk );
				else
					decodedChunk = QByteArray::fromRawData( decoded.data() + decodedPos, chunk );

				decodedPos += chunk;
				decodedSize -= chunk;

				playC.audio_last_delay = ( double )decodedChunk.size() / ( double )( sizeof( float ) * realChannels * realSample_rate );
				if ( packet.ts.isValid() )
				{
					audio_pts = playC.audio_current_pts = packet.ts - delay;
					if ( !playC.vThr )
					{
#ifdef Q_OS_WIN
						playC.chPos( playC.audio_current_pts, playC.flushAudio );
#else
						playC.chPos( playC.audio_current_pts );
#endif
					}
				}

				tmp_time += playC.audio_last_delay * 1000.0;
				packet.ts += playC.audio_last_delay;

#ifdef Q_OS_WIN
				canRefresh = true;
#endif

				if ( playC.skipAudioFrame <= 0.0 )
				{
					const double speed = playC.speed;
					if ( speed != lastSpeed )
					{
						resampler_create();
						lastSpeed = speed;
					}

					if ( vol != 1.0 && vol > 0.0 )
					{
						const int size = decodedChunk.size() / sizeof( float );
						float *data = ( float * )decodedChunk.data();
						for ( int i = 0 ; i < size ; ++i )
							data[ i ] *= vol;
					}

					foreach ( QMPlay2Extensions *vis, visualizations )
						vis->sendSoundData( decodedChunk );

					QByteArray dataToWrite;
					if ( sndResampler.isOpen() )
						sndResampler.convert( decodedChunk, dataToWrite );
					else
						dataToWrite = decodedChunk;

					if ( doSilence >= 0.0 && vol > 0.0 )
					{
						silenceChMutex.lock();
						if ( doSilence >= 0.0 )
						{
							float *data = ( float * )dataToWrite.data();
							const int s = dataToWrite.size() / sizeof( float );
							for ( int i = 0 ; i < s ; i += channels )
							{
								for ( int j = 0 ; j < channels ; ++j )
									data[ i+j ] *= doSilence;
								doSilence -= silence_step;
								if ( doSilence < 0.0 )
									doSilence = 0.0;
								else if ( doSilence > 1.0 )
								{
									doSilence = -1.0;
									break;
								}
							}
						}
						silenceChMutex.unlock();
					}

					writer->write( dataToWrite );
				}
				else
					playC.skipAudioFrame -= playC.audio_last_delay;
			}

			mutex.unlock();

			if ( playC.flushAudio || packet.data.size() == bytes_consumed || ( !bytes_consumed && !decoded.size() ) )
			{
				bytes_consumed = -1;
				packet = Packet();
			}
			else if ( bytes_consumed != packet.data.size() )
				packet.data.remove( 0, bytes_consumed );
		}
	}
}

bool AudioThr::resampler_create()
{
	double speed = playC.speed > 0.0 ? playC.speed : 1.0;
	if ( realSample_rate != sample_rate || realChannels != channels || speed != 1.0 )
	{
		const bool OK = sndResampler.create( realSample_rate, realChannels, sample_rate/speed, channels );
		if ( !OK )
			QMPlay2Core.logError( tr( "Błąd podczas inicjalizacji" ) + ": " + sndResampler.name() );
		return OK;
	}
	sndResampler.destroy();
	return true;
}

#ifdef Q_OS_WIN
void AudioThr::timerEvent( QTimerEvent * )
{
	if ( !canRefresh || br || !isRunning() )
		return;
	canRefresh = false;
	if ( !playC.vThr && playC.canUpdatePos )
		emit playC.updatePos( round( playC.pos ) );
	emit playC.updateBitrate( round( ( tmp_br << 3 ) / tmp_time ), -1, -1.0 );
	tmp_br = tmp_time = 0;
}
#endif

void AudioThr::pauseVis( bool b )
{
	foreach ( QMPlay2Extensions *vis, visualizations )
		vis->visState( !b, realChannels, realSample_rate );
}
