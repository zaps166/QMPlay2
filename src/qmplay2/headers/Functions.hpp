#ifndef FUNCTIONS_HPP
#define FUNCTIONS_HPP

#include <IOController.hpp>

#include <QStringList>
#include <QImage>
#include <QDate>

class QMPlay2_OSD;
class QMimeData;
class QPainter;
class QRect;

#ifdef Q_OS_WIN
	#include <windows.h>
#else
	#ifdef Q_OS_MAC
		#include <mach/mach_time.h>
	#else
		#include <time.h>
	#endif
	#include <unistd.h>
#endif

namespace Functions
{
	struct DemuxerInfo
	{
		QString name;
		QImage img;
		QStringList extensions;
	};
	typedef QVector< DemuxerInfo > DemuxersInfo;
	typedef QList< QByteArray > ChecksumList;

	QDate parseVersion( const QString &dateTxt );

	QString Url( QString, const QString &pth = QString() );
	QString getUrlScheme( const QString &url );

	static inline void getHMS( int t, int &H, int &M, int &S )
	{
		H = t / 3600;
		M = t % 3600 / 60;
		S = t % 60;
	}
	QString timeToStr( int, bool space = false );

	QString filePath( const QString & );
	QString fileName( QString, bool extension = true );
	QString fileExt( const QString & );

	QString cleanPath( QString );
	QString cleanFileName( QString );

	QString sizeString( quint64 );

	static inline double gettime()
	{
#if defined Q_OS_WIN
		LARGE_INTEGER Frequency, Counter;
		QueryPerformanceFrequency( &Frequency );
		QueryPerformanceCounter( &Counter );
		return ( double )Counter.QuadPart / ( double )Frequency.QuadPart;
#elif defined Q_OS_MAC
		mach_timebase_info_data_t mach_base_info;
		mach_timebase_info( &mach_base_info );
		return ( ( double )mach_absolute_time() * ( double )mach_base_info.numer ) / ( 1000000000.0 * ( double )mach_base_info.denom );
#else
		timespec now;
		clock_gettime(
		#ifdef CLOCK_MONOTONIC_RAW
			CLOCK_MONOTONIC_RAW,
		#else
			CLOCK_MONOTONIC,
		#endif
			&now
		);
		return now.tv_sec + ( now.tv_nsec / 1000000000.0 );
#endif
	}
	static inline void s_wait( const double s )
	{
		if ( s > 0.0 )
		{
#ifndef Q_OS_WIN
			usleep( s * 1000000UL );
#else
			Sleep( s * 1000 );
#endif
		}
	}

	template < typename T > static inline T aligned( const T val, const T alignment )
	{
		return ( val + alignment - 1 ) & ~( alignment - 1 );
	}
	static inline quint8 clip8( int val )
	{
		return val > 255 ? ( quint8 )255 : ( val < 0 ? ( quint8 )0 : val );
	}

	void getImageSize( const double aspect_ratio, const double zoom, const int winW, const int winH, int &W, int &H, int *X = NULL, int *Y = NULL, QRect *dstRect = NULL, const int *vidW = NULL, const int *vidH = NULL, QRect *srcRect = NULL );

	bool mustRepaintOSD( const QList< const QMPlay2_OSD * > &osd_list, const ChecksumList &osd_checksums, const qreal *scaleW = NULL, const qreal *scaleH = NULL, QRect *bounds = NULL );
	void paintOSD( bool rgbSwapped, const QList< const QMPlay2_OSD * > &osd_list, const qreal scaleW, const qreal scaleH, QPainter &painter, ChecksumList *osd_checksums = NULL );
	void paintOSDtoYV12( quint8 *imageData, const QByteArray &videoFrameData, QImage &osdImg, int W, int H, int linesizeLuma, int linesizeChroma, const QList< const QMPlay2_OSD * > &osd_list, ChecksumList &osd_checksums );

	void ImageEQ( int Contrast, int Brightness, quint8 *imageBits, unsigned bitsCount );
	int scaleEQValue( int val, int min, int max );

	QByteArray convertToASS( const QByteArray & );

	bool chkMimeData( const QMimeData * );
	QStringList getUrlsFromMimeData( const QMimeData * );

	bool splitPrefixAndUrlIfHasPluginPrefix( const QString &, QString *, QString *, QString *param = NULL );
	void getDataIfHasPluginPrefix( const QString &wholeUrl, QString *url = NULL, QString *name = NULL, QImage *img = NULL, IOController<> *ioCtrl = NULL, const DemuxersInfo &demuxersInfo = DemuxersInfo() );

	void hFlip( char *data, int linesize, int height, int width );
	void vFlip( char *data, int linesize, int height );

	QString dBStr( double a );
}

#endif
