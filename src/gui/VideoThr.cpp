#include <VideoThr.hpp>

#include <PlayClass.hpp>
#include <Main.hpp>

#include <VideoWriter.hpp>
#include <QMPlay2_OSD.hpp>
#include <VideoFrame.hpp>
#include <Settings.hpp>
#include <Decoder.hpp>
#include <LibASS.hpp>

#include <ImgScaler.hpp>
#include <Functions.hpp>
using Functions::convertToASS;
using Functions::gettime;
using Functions::s_wait;
using Functions::aligned;

#include <QDir>
#include <QImage>

#include <math.h>

VideoThr::VideoThr( PlayClass &playC, Writer *HWAccelWriter, const QStringList &pluginsName ) :
	AVThread( playC, "video:", HWAccelWriter, pluginsName ),
	sDec( NULL ),
	do_screenshot( false ),
	HWAccel( HWAccelWriter ),
	deleteOSD( false ), deleteFrame( false ),
	W( 0 ), H( 0 ),
	napisy( NULL )
{
	connect( this, SIGNAL( write( const QByteArray & ) ), this, SLOT( write_slot( const QByteArray & ) ) );
	connect( this, SIGNAL( screenshot( const QByteArray & ) ), this, SLOT( screenshot_slot( const QByteArray & ) ) );
	connect( this, SIGNAL( pause() ), this, SLOT( pause_slot() ) );
}
VideoThr::~VideoThr()
{
	delete playC.osd;
	playC.osd = NULL;
	delete napisy;
	delete sDec;
}

void VideoThr::stop( bool terminate )
{
#ifdef Q_OS_WIN
	QMPlay2GUI.forbid_screensaver = false;
#endif
	AVThread::stop( terminate );
}

bool VideoThr::setFlip()
{
	return writer ? writer->modParam( "Flip", playC.flip ) : false;
}
void VideoThr::setVideoEqualizer()
{
	if ( writer )
	{
		writer->modParam( "Brightness", playC.Brightness );
		writer->modParam( "Saturation", playC.Saturation );
		writer->modParam( "Contrast", playC.Contrast );
		writer->modParam( "Hue", playC.Hue );
	}
}
void VideoThr::setFrameSize( int w, int h )
{
	W = w;
	H = h;
	if ( writer )
	{
		writer->modParam( "W", W );
		writer->modParam( "H", H );
	}
	deleteSubs = deleteOSD = deleteFrame = true;
}
void VideoThr::setARatio( double aRatio )
{
	updateSubs();
	if ( writer )
		writer->modParam( "AspectRatio", aRatio );
}
void VideoThr::setZoom()
{
	updateSubs();
	if ( writer )
		writer->modParam( "Zoom", playC.zoom );
}

void VideoThr::initFilters( bool processParams )
{
	Settings &QMPSettings = QMPlay2Core.getSettings();

	if ( processParams )
		lock();

	filters.clear();

	if ( QMPSettings.getBool( "Deinterlace/ON" ) ) //Deinterlacing filters as first
	{
		const bool autoDeint = QMPSettings.getBool( "Deinterlace/Auto" );
		const bool doubleFramerate = QMPSettings.getBool( "Deinterlace/Doubler" );
		const bool autoParity = QMPSettings.getBool( "Deinterlace/AutoParity" );
		const bool topFieldFirst = QMPSettings.getBool( "Deinterlace/TFF" );
		const quint8 deintFlags = autoDeint | doubleFramerate << 1 | autoParity << 2 | topFieldFirst << 3;
		bool HWDeint = false, PrepareForHWBobDeint = false;
		if ( writer && ( ( HWDeint = writer->modParam( "Deinterlace", 1 | deintFlags << 1 ) ) ) )
			PrepareForHWBobDeint = doubleFramerate && writer->getParam( "PrepareForHWBobDeint" ).toBool();
		if ( !HWDeint || PrepareForHWBobDeint )
		{
			const QString deintFilterName = PrepareForHWBobDeint ? "PrepareForHWBobDeint" : QMPSettings.getString( "Deinterlace/SoftwareMethod" );
			VideoFilter *deintFilter = filters.on( deintFilterName );
			bool ok = false;
			if ( deintFilter && deintFilter->modParam( "DeinterlaceFlags", deintFlags ) )
			{
				deintFilter->modParam( "W", W );
				deintFilter->modParam( "H", H );
				ok = deintFilter->processParams();
			}
			if ( !ok )
			{
				if ( deintFilter )
					filters.off( deintFilter );
				QMPlay2Core.logError( tr( "Błąd inicjalizacji filtru usuwania przeplotu" ) + " \"" + deintFilterName + '"' );
			}
		}
	}
	else if ( writer )
		writer->modParam( "Deinterlace", 0 );

	if ( !isHWAccel() )
		foreach ( QString filterName, QMPSettings.get( "VideoFilters" ).toStringList() )
			if ( filterName.left( 1 ).toInt() ) //if filter is enabled
			{
				VideoFilter *filter = filters.on( ( filterName = filterName.mid( 1 ) ) );
				bool ok = false;
				if ( filter )
				{
					filter->modParam( "W", W );
					filter->modParam( "H", H );
					if ( !( ok = filter->processParams() ) )
						filters.off( filter );
				}
				if ( !ok )
					QMPlay2Core.logError( tr( "Błąd inicjalizacji filtru" ) + " \"" + filterName + '"' );
			}

	if ( processParams )
	{
		unlock();
		if ( writer && writer->hasParam( "Deinterlace" ) )
			writer->processParams();
	}
}

bool VideoThr::processParams()
{
	return writer ? writer->processParams() : false;
}

void VideoThr::updateSubs()
{
	if ( writer && playC.ass )
	{
		playC.sPackets.lock();
		if ( napisy )
			playC.ass->getASS( napisy, playC.frame_last_pts + playC.frame_last_delay  - playC.subtitlesSync );
		playC.sPackets.unlock();
	}
}

void VideoThr::run()
{
#ifdef Q_OS_WIN
	QMPlay2GUI.forbid_screensaver = true;
#endif

	bool skip = false, paused = false, oneFrame = false, useLastDelay = false, lastOSDListEmpty = true;
	double tmp_time = 0.0, sync_last_pts = 0.0, frame_timer = 0.0, sync_timer = 0.0;
	QMutex emptyBufferMutex;
	QByteArray frame;
	unsigned fast = 0;
	int tmp_br = 0, frames = 0;
	canWrite = true;

	while ( !br )
	{
		if ( deleteFrame )
		{
			VideoFrame::unref( frame );
			deleteFrame = false;
		}

		if ( do_screenshot && !frame.isEmpty() )
		{
			VideoFrame::ref( frame );
			emit screenshot( frame );
			do_screenshot = false;
		}

		const bool getNewPacket = !filters.readyToRead();
		playC.vPackets.lock();
		const bool hasVPackets = playC.vPackets.canFetch();
		if ( ( playC.paused && !oneFrame ) || ( !hasVPackets && getNewPacket ) || playC.waitForData )
		{
			if ( playC.paused && !paused )
			{
				emit pause();
#ifdef Q_OS_WIN
				QMPlay2GUI.forbid_screensaver = false;
#endif
				paused = true;
			}
			playC.vPackets.unlock();

			tmp_br = tmp_time = frames = 0;
			skip = false;
			fast = 0;

			if ( !playC.paused )
				waiting = playC.fillBufferB = true;

			playC.emptyBufferCond.wait( &emptyBufferMutex, MUTEXWAIT_TIMEOUT );
			emptyBufferMutex.unlock();
			frame_timer = gettime();
			continue;
		}
		if ( paused )
		{
#ifdef Q_OS_WIN
			QMPlay2GUI.forbid_screensaver = true;
#endif
			paused = false;
		}
		waiting = false;
		Packet packet;
		if ( hasVPackets && getNewPacket )
			packet = playC.vPackets.fetch();
		else
			packet.ts.setInvalid();
		playC.vPackets.unlock();
		if ( playC.nextFrameB )
		{
			skip = playC.nextFrameB = false;
			oneFrame = playC.paused = true;
			fast = 0;
		}
		playC.fillBufferB = true;

		mutex.lock();
		if ( br )
		{
			mutex.unlock();
			break;
		}

		/* napisy */
		const double pts = playC.frame_last_pts + playC.frame_last_delay  - playC.subtitlesSync;
		QList< const QMPlay2_OSD * > osd_list, osd_list_to_delete;
		playC.sPackets.lock();
		if ( sDec ) //image subs (pgssub, dvdsub)
		{
			Packet sPacket;
			if ( playC.sPackets.canFetch() )
				sPacket = playC.sPackets.fetch();
			if ( !sDec->decodeSubtitle( sPacket, sPacket.ts, pts, napisy, W, H ) )
			{
				osd_list_to_delete += napisy;
				napisy = NULL;
			}
		}
		else
		{
			if ( playC.sPackets.canFetch() )
			{
				Packet sPacket = playC.sPackets.fetch();
				if ( playC.ass->isASS() )
					playC.ass->addASSEvent( sPacket );
				else
					playC.ass->addASSEvent( convertToASS( sPacket ), sPacket.ts, sPacket.duration );
			}
			if ( !playC.ass->getASS( napisy, pts ) )
			{
				osd_list_to_delete += napisy;
				napisy = NULL;
			}
		}
		if ( napisy )
		{
			bool hasDuration = napisy->duration() >= 0.0;
			if ( deleteSubs || ( napisy->isStarted() && pts < napisy->pts() ) || ( hasDuration && pts > napisy->pts() + napisy->duration() ) )
			{
				osd_list_to_delete += napisy;
				napisy = NULL;
			}
			else if ( pts >= napisy->pts() )
			{
				napisy->start();
				osd_list += napisy;
			}
		}
		playC.sPackets.unlock();
		playC.osd_mutex.lock();
		if ( playC.osd )
		{
			if ( deleteOSD || playC.osd->left_duration() < 0 )
			{
				osd_list_to_delete += playC.osd;
				playC.osd = NULL;
			}
			else
				osd_list += playC.osd;
		}
		playC.osd_mutex.unlock();

		if ( ( !lastOSDListEmpty || !osd_list.isEmpty() ) && writer && writer->readyWrite() )
		{
			( ( VideoWriter * )writer )->writeOSD( osd_list );
			lastOSDListEmpty = osd_list.isEmpty();
		}
		while ( osd_list_to_delete.size() )
			delete osd_list_to_delete.takeFirst();
		deleteSubs = deleteOSD = false;
		/**/

		if ( playC.flushVideo )
			filters.clearBuffers();

		if ( !packet.isEmpty() )
		{
			QByteArray decoded;
			const int bytes_consumed = dec->decode( packet, decoded, playC.flushVideo, skip ? ~0 : ( fast > 1 ? fast - 1 : 0 ) );
			if ( playC.flushVideo )
			{
				useLastDelay = true; //if seeking
				playC.flushVideo = false;
			}
			if ( !decoded.isEmpty() )
				filters.addFrame( decoded, packet.ts );
			else if ( skip )
				filters.removeLastFromInputBuffer();
			tmp_br += bytes_consumed;
		}

		const bool ptsIsValid = filters.getFrame( frame, packet.ts );
		if ( packet.ts.isValid() )
		{
			if ( dec->aspect_ratio_changed() )
				emit playC.aRatioUpdate();
			if ( ptsIsValid || packet.ts > playC.pos )
				playC.chPos( packet.ts );

			double delay = packet.ts - playC.frame_last_pts;
			if ( useLastDelay || delay <= 0.0 || ( playC.frame_last_pts <= 0.0 && delay > playC.frame_last_delay ) )
			{
				delay = playC.frame_last_delay;
				useLastDelay = false;
			}

			tmp_time += delay * 1000.0;
			frames += 1000;
			if ( tmp_time >= 1000.0 )
			{
				emit playC.updateBitrate( -1, round( ( tmp_br << 3 ) / tmp_time ), frames / tmp_time );
				frames = tmp_br = tmp_time = 0;
			}

			delay /= playC.speed;

			playC.frame_last_delay = delay;
			playC.frame_last_pts = packet.ts;

			if ( playC.skipAudioFrame < 0.0 )
				playC.skipAudioFrame = 0.0;

			const double true_delay = delay;

			if ( syncVtoA && playC.audio_current_pts > 0.0 && !oneFrame )
			{
				double sync_pts = playC.audio_current_pts;
				if ( sync_last_pts == sync_pts )
					sync_pts += gettime() - sync_timer;
				else
				{
					sync_last_pts = sync_pts;
					sync_timer = gettime();
				}

				const double diff = packet.ts - ( delay + sync_pts - playC.videoSync );
				const double sync_threshold = qMax( delay, playC.audio_last_delay );
				const double max_threshold = sync_threshold < 0.1 ? 0.125 : sync_threshold * 1.5;
				const double fDiff = qAbs( diff );

				if ( fast && !skip && diff > -sync_threshold / 2.0 )
					fast = 0;
				skip = false;

//				qDebug() << "diff" << diff << "sync_threshold" << sync_threshold << "max_threshold" << max_threshold;
				if ( fDiff > sync_threshold && fDiff < max_threshold )
				{
					if ( diff < 0.0 ) //obraz się spóźnia
					{
						delay -= sync_threshold / 10.0;
// 						qDebug() << "speed up" << diff << delay << sync_threshold;
					}
					else if ( diff > 0.0 ) //obraz idzie za szybko
					{
						delay += sync_threshold / 10.0;
// 						qDebug() << "slow down" << diff << delay << sync_threshold;
					}
				}
				else if ( fDiff >= max_threshold )
				{
					if ( diff < 0.0 ) //obraz się spóźnia
					{
						delay = 0.0;
						if ( fast > 3 )
							skip = true;
					}
					else if ( diff > 0.0 ) //obraz idzie za szybko
					{
						if ( diff <= 0.5 )
							delay *= 2.0;
						else if ( !playC.skipAudioFrame )
							playC.skipAudioFrame = diff;
					}
//					qDebug() << "Skipping" << diff << skip << fast << delay;
				}
			}
			else if ( playC.audio_current_pts <= 0.0 || oneFrame )
			{
				skip = false;
				fast = 0;
			}

			mutex.unlock();

			if ( !frame.isEmpty() )
			{
				if ( !skip && canWrite )
				{
					oneFrame = canWrite = false;
					VideoFrame::ref( frame );
					emit write( frame );
				}

				const double delay_diff = gettime() - frame_timer;
				if ( syncVtoA && true_delay > 0.0 && delay_diff > true_delay )
					++fast;

				delay -= delay_diff;
				while ( delay > 0.25 )
				{
					s_wait( 0.25 );
					if ( br || playC.flushVideo || br2 )
						delay = 0.0;
					else
						delay -= 0.25;
				}
				s_wait( delay );
			}
			frame_timer = gettime();
		}
		else
			mutex.unlock();
	}

	VideoFrame::unref( frame );
}

#ifdef Q_WS_X11
	#include <QX11Info>
	#include <X11/Xlib.h>
#endif

void VideoThr::write_slot( const QByteArray &frame )
{
#ifdef Q_WS_X11
	XResetScreenSaver( QX11Info::display() );
#endif
	canWrite = true;
	if ( writer && writer->readyWrite() )
		writer->write( frame );
	else
		VideoFrame::unref( frame );
}
void VideoThr::screenshot_slot( const QByteArray &frame )
{
	ImgScaler imgScaler;
	const int aligned8W = aligned( W, 8 );
	if ( writer && imgScaler.create( W, H, aligned8W, H ) )
	{
		QImage img( aligned8W, H, QImage::Format_RGB32 );
		VideoFrame *videoFrame = ( VideoFrame * )frame.data();
		bool ok = true;
		if ( !isHWAccel() )
			imgScaler.scale( videoFrame, img.bits() );
		else
			ok = ( ( VideoWriter * )writer )->HWAccellGetImg( videoFrame, img.bits(), &imgScaler );
		if ( !ok )
			QMPlay2Core.logError( tr( "Błąd podczas tworzenia zrzutu ekranu" ) );
		else
		{
			const QString ext = QMPlay2Core.getSettings().getString( "screenshotFormat" );
			const QString dir = QMPlay2Core.getSettings().getString( "screenshotPth" );
			quint16 num = 0;
			foreach ( QString f, QDir( dir ).entryList( QStringList( "QMPlay2_snap_?????" + ext ), QDir::Files, QDir::Name ) )
			{
				const quint16 n = f.mid( 13, 5 ).toUShort();
				if ( n > num )
					num = n;
			}
			img.save( dir + "/QMPlay2_snap_" + QString( "%1" ).arg( ++num, 5, 10, QChar( '0' ) ) + ext );
		}
	}
	VideoFrame::unref( frame );
}
void VideoThr::pause_slot()
{
	if ( writer )
		writer->pause();
}
