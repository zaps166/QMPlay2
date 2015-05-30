#ifndef FUNCTIONS_HPP
#define FUNCTIONS_HPP

#include <IOController.hpp>

#include <QStringList>
#include <QImage>

class QMPlay2_OSD;
class QMimeData;
class QPainter;
class QRect;

#ifndef Q_OS_WIN
	#include <unistd.h>
	#include <time.h>
#else
	#include <windows.h>
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
#ifndef Q_OS_WIN
		timespec now;
		clock_gettime( CLOCK_MONOTONIC, &now );
		return now.tv_sec + ( now.tv_nsec / 1000000000.0 );
#else
		LARGE_INTEGER Frequency, Counter;
		QueryPerformanceFrequency( &Frequency );
		QueryPerformanceCounter( &Counter );
		return ( double )Counter.QuadPart / ( double )Frequency.QuadPart;
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
	void paintOSD( const QList< const QMPlay2_OSD * > &osd_list, const qreal scaleW, const qreal scaleH, QPainter &painter, ChecksumList *osd_checksums = NULL );
	void paintOSDtoYV12( quint8 *imageData, const QByteArray &videoFrameData, QImage &osdImg, int W, int H, int linesizeLuma, int linesizeChroma, const QList< const QMPlay2_OSD * > &osd_list, ChecksumList &osd_checksums );

	void ImageEQ( int Contrast, int Brightness, quint8 *imageBits, unsigned bitsCount );
	int scaleEQValue( int val, int min, int max );

	QByteArray convertToASS( const QByteArray & );

	bool chkMimeData( const QMimeData * );
	QStringList getUrlsFromMimeData( const QMimeData * );

	bool splitPrefixAndUrlIfHasPluginPrefix( const QString &, QString *, QString *, QString *param = NULL );
	void getDataIfHasPluginPrefix( const QString &wholeUrl, QString *url = NULL, QString *name = NULL, QImage *img = NULL, IOController<> *ioCtrl = NULL, const DemuxersInfo &demuxersInfo = DemuxersInfo() );
}

#endif
