#include <Functions.hpp>

#include <QMPlay2Extensions.hpp>
#include <QMPlay2_OSD.hpp>
#include <VideoFrame.hpp>
#include <Reader.hpp>

#include <QMimeData>
#include <QPainter>
#include <QDir>
#include <QUrl>

#include <math.h>

static inline void swapArray( char *a, char *b, int size )
{
	char t[ size ];
	memcpy( t, a, size );
	memcpy( a, b, size );
	memcpy( b, t, size );
}

/**/

QDate Functions::parseVersion( const QString &dateTxt )
{
	QStringList l = dateTxt.split( '.' );
	int y = 0, m = 0, d = 0;
	if ( l.count() == 3 )
	{
		y = l[ 0 ].toInt() + 2000;
		m = l[ 1 ].toInt();
		d = l[ 2 ].toInt();
	}
	if ( y < 2000 || m < 1 || m > 12 || d < 1 || d > 31 )
		y = m = d = 0;
	return QDate( y, m, d );
}

QString Functions::Url( QString url, const QString &pth )
{
#ifdef Q_OS_WIN
	url.replace( '\\', '/' );
#endif
	QString scheme = getUrlScheme( url );
#ifdef Q_OS_WIN
	if ( url.left( 8 ) == "file:///" ) //lokalnie na dysku
	{
		url.remove( 7, 1 );
		return url;
	}
	else if ( url.left( 7 ) == "file://" ) //adres sieciowy
	{
		url.replace( "file://", "file:////" );
		return url;
	}
	if ( url.left( 2 ) == "//" ) //adres sieciowy
	{
		url.prepend( "file://" );
		return url;
	}

	QStringList drives;
	QFileInfoList fIL = QDir::drives();
	foreach ( const QFileInfo &fI, fIL )
		drives += getUrlScheme( fI.path() );
	if ( drives.contains( scheme ) )
	{
		url = "file://" + url;
		return url;
	}
#endif
	if ( scheme.isEmpty() )
	{
#ifdef Q_OS_WIN
		if ( url.left( 1 ) == "/" )
			url.remove( 0, 1 );
#endif
#ifndef Q_OS_WIN
		if ( url.left( 1 ) != "/" )
#endif
		{
			QString addPth = pth.isEmpty() ? QDir::currentPath() : pth;
			if ( addPth.right( 1 ) != "/" )
				addPth += '/';
			url.prepend( addPth );
		}
		url.prepend( "file://" );
	}
	return url;
}
QString Functions::getUrlScheme( const QString &url )
{
	int idx = url.indexOf( ':' );
	if ( idx > -1 && url[ 0 ] != '/' )
		return url.left( idx );
	return QString();
}

QString Functions::timeToStr( int t, bool space )
{
	if ( t < 0 )
		return QString();

	QString separator;
	if ( space )
		separator = " : ";
	else
		separator = ":";

	int h, m, s;
	getHMS( t, h, m, s );

	QString timStr;
	if ( h )
		timStr = QString( "%1" + separator ).arg( h, 2, 10, QChar( '0' ) );
	timStr += QString( "%1" + separator + "%2" ).arg( m, 2, 10, QChar( '0' ) ).arg( s, 2, 10, QChar( '0' ) );

	return timStr;
}

QString Functions::filePath( const QString &f )
{
	return f.left( f.lastIndexOf( '/' ) + 1 );
}
QString Functions::fileName( QString f, bool extension )
{
	/* Tylko dla adresów do przekonwertowania */
	QString real_url;
	if ( splitPrefixAndUrlIfHasPluginPrefix( f, NULL, &real_url ) )
		return real_url;
	/**/
#ifndef Q_OS_WIN
	if ( f == "file:///" )
		return "/";
#endif
	while ( f.right( 1 ) == "/" )
		f.chop( 1 );
	QString n = f.right( f.length() - f.lastIndexOf( '/' ) - 1 );
	if ( extension || f.left( 5 ) != "file:" )
		return n;
	return n.mid( 0, n.lastIndexOf( '.' ) );
}
QString Functions::fileExt( const QString &f )
{
	int idx = f.lastIndexOf( '.' );
	if ( idx > -1 )
		return f.mid( idx+1 );
	return QString();
}

QString Functions::cleanPath( QString p )
{
#ifndef Q_OS_WIN
	if ( p == "file:///" )
		return p;
#endif
	if ( p.right( 1 ) != "/" )
		return p + "/";
	while ( p.right( 2 ) == "//" )
		p.chop( 1 );
	return p;
}
QString Functions::cleanFileName( QString f )
{
	if ( f.length() > 200 )
		f.resize( 200 );
	f.replace( '/', '_' );
#ifdef Q_OS_WIN
	f.replace( '\\', '_' );
	f.replace( ':', '_' );
	f.replace( '*', '_' );
	f.replace( '?', '_' );
	f.replace( '"', '_' );
	f.replace( '<', '_' );
	f.replace( '>', '_' );
	f.replace( '|', '_' );
#endif
	return f;
}

QString Functions::sizeString( quint64 B )
{
	if ( B < 1024ULL )
		return QString::number( B ) + " B";
	else if ( B < 1048576ULL )
		return QString::number( B / 1024.0, 'f', 2 ) + " KiB";
	else if ( B < 1073741824ULL )
		return QString::number( B / 1048576.0, 'f', 2 ) + " MiB";
	else if ( B  < 1099511627776ULL )
		return QString::number( B / 1073741824.0, 'f', 2 ) + " GiB";
	return QString();
}

void Functions::getImageSize( const double aspect_ratio, const double zoom, const int winW, const int winH, int &W, int &H, int *X, int *Y, QRect *dstRect, const int *vidW, const int *vidH, QRect *srcRect )
{
	W = winW;
	H = winH;
	if ( aspect_ratio > 0.0 )
	{
		if ( W / aspect_ratio > H )
			W = H * aspect_ratio;
		else
			H = W / aspect_ratio;
	}
	if ( zoom != 1.0 && zoom > 0.0 )
	{
		W *= zoom;
		H *= zoom;
	}
	if ( X )
		*X = ( winW - W ) / 2;
	if ( Y )
		*Y = ( winH - H ) / 2;
	if ( X && Y && dstRect )
	{
		*dstRect = QRect( *X, *Y, W, H ) & QRect( 0, 0, winW, winH );
		if ( vidW && vidH && srcRect )
		{
			if ( W > 0 && H > 0 )
				srcRect->setCoords
				(
					( dstRect->x() - *X ) * *vidW / W,
					( dstRect->y() - *Y ) * *vidH / H,
					*vidW - ( *X + W - 1 - dstRect->right()  ) * *vidW / W - 1,
					*vidH - ( *Y + H - 1 - dstRect->bottom() ) * *vidH / H - 1
				);
			else
				srcRect->setCoords( 0, 0, 0, 0 );
		}
	}
}

bool Functions::mustRepaintOSD( const QList< const QMPlay2_OSD * > &osd_list, const ChecksumList &osd_checksums, const qreal *scaleW, const qreal *scaleH, QRect *bounds )
{
	bool mustRepaint = false;
	foreach ( const QMPlay2_OSD *osd, osd_list )
	{
		osd->lock();
		if ( !mustRepaint )
			mustRepaint = !osd_checksums.contains( osd->getChecksum() );
		if ( scaleW && scaleH && bounds )
		{
			for ( int j = 0 ; j < osd->imageCount() ; j++ )
			{
				const QMPlay2_OSD::Image &img = osd->getImage( j );
				if ( !osd->needsRescale() )
					*bounds |= img.rect;
				else
				{
					const qreal scaledW = *scaleW;
					const qreal scaledH = *scaleH;
					*bounds |= QRect( img.rect.x() * scaledW, img.rect.y() * scaledH, img.rect.width() * scaledW, img.rect.height() * scaledH );
				}
			}
		}
		osd->unlock();
	}
	return mustRepaint;
}
void Functions::paintOSD( bool rgbSwapped, const QList< const QMPlay2_OSD * > &osd_list, const qreal scaleW, const qreal scaleH, QPainter &painter, ChecksumList *osd_checksums )
{
	if ( osd_checksums )
		osd_checksums->clear();
	foreach ( const QMPlay2_OSD *osd, osd_list )
	{
		osd->lock();
		if ( osd_checksums )
			osd_checksums->append( osd->getChecksum() );
		if ( osd->needsRescale() )
		{
			painter.save();
			painter.setRenderHint( QPainter::SmoothPixmapTransform );
			painter.scale( scaleW, scaleH );
		}
		for ( int j = 0 ; j < osd->imageCount() ; j++ )
		{
			const QMPlay2_OSD::Image &img = osd->getImage( j );
			const QImage qImg = QImage( ( uchar * )img.data.data(), img.rect.width(), img.rect.height(), QImage::Format_ARGB32 );
			painter.drawImage( img.rect.topLeft(), rgbSwapped ? qImg.rgbSwapped() : qImg );
		}
		if ( osd->needsRescale() )
			painter.restore();
		osd->unlock();
	}
}
void Functions::paintOSDtoYV12( quint8 *imageData, const QByteArray &videoFrameData, QImage &osdImg, int W, int H, int linesizeLuma, int linesizeChroma, const QList< const QMPlay2_OSD * > &osd_list, ChecksumList &osd_checksums )
{
	QRect bounds;
	const int osdW = osdImg.width();
	const int imgH = osdImg.height();
	const qreal iScaleW = ( qreal )osdW / W, iScaleH = ( qreal )imgH / H;
	const qreal scaleW = ( qreal )W / osdW, scaleH = ( qreal )H / imgH;
	const bool mustRepaint = Functions::mustRepaintOSD( osd_list, osd_checksums, &scaleW, &scaleH, &bounds );
	bounds = QRect( floor( bounds.x() * iScaleW ), floor( bounds.y() * iScaleH ), ceil( bounds.width() * iScaleW ), ceil( bounds.height() * iScaleH ) ) & QRect( 0, 0, osdW, imgH );
	quint32 *osdImgData = ( quint32 * )osdImg.bits();
	if ( mustRepaint )
	{
		for ( int h = bounds.top() ; h <= bounds.bottom() ; ++h )
			memset( osdImgData + ( h * osdW + bounds.left() ), 0, bounds.width() << 2 );
		QPainter p( &osdImg );
		p.setRenderHint( QPainter::SmoothPixmapTransform );
		p.scale( iScaleW, iScaleH );
		Functions::paintOSD( false, osd_list, scaleW, scaleH, p, &osd_checksums );
	}

	quint8 *data[ 3 ] = { ( quint8 * )imageData };
	data[ 2 ] = data[ 0 ] + linesizeLuma * imgH;
	data[ 1 ] = data[ 2 ] + linesizeChroma * ( imgH >> 1 );

	const VideoFrame *videoFrame = VideoFrame::fromData( videoFrameData );
	const int alphaLinesizeLuma = videoFrame->linesize[ 0 ], alphaLinesizeChroma = videoFrame->linesize[ 1 ];
	const quint8 *dataForAlpha[ 3 ] = { videoFrame->data[ 0 ], videoFrame->data[ 1 ], videoFrame->data[ 2 ] };

	for ( int h = bounds.top() ; h <= bounds.bottom() ; ++h )
	{
		const int fullLine = h * linesizeLuma;
		const int halfLine = ( h >> 1 ) * linesizeChroma;
		const int alphaFullLine = h * alphaLinesizeLuma;
		const int alphaHalfLine = ( h >> 1 ) * alphaLinesizeChroma;
		const int line = h * osdW;

		for ( int w = bounds.left() ; w <= bounds.right() ; ++w )
		{
			const int pixelPos = fullLine + w;
			const quint32 pixel = osdImgData[ line + w ];
			const quint8 A = ( pixel >> 24 ) & 0xFF;
			if ( A )
			{
				const quint8 iA = ~A & 0xFF;
				const quint8  B = ( pixel >> 16 ) & 0xFF;
				const quint8  G = ( pixel >> 8 ) & 0xFF;
				const quint8  R = pixel & 0xFF;

				const quint8 Y = ( ( R * 66 ) >> 8 ) + ( G >> 1 ) + ( ( B * 25 ) >> 8 ) + 16;
				if ( A == 0xFF )
					data[ 0 ][ pixelPos ] = Y;
				else
					data[ 0 ][ pixelPos ] = dataForAlpha[ 0 ][ alphaFullLine + w ] * iA / 255 + Y * A / 255;

				if ( !( w & 1 ) && !( h & 1 ) )
				{
					const int pixelPos = halfLine + ( w >> 1 );
					const quint8 cB = -( ( R * 38  ) >> 8 ) - ( ( G * 74 ) >> 8 ) + ( ( B * 112 ) >> 8 ) + 128;
					const quint8 cR =  ( ( R * 112 ) >> 8 ) - ( ( G * 94 ) >> 8 ) - ( ( B * 18  ) >> 8 ) + 128;
					if ( A == 0xFF )
					{
						data[ 1 ][ pixelPos ] = cB;
						data[ 2 ][ pixelPos ] = cR;
					}
					else
					{
						const int pixelPosAlpha = alphaHalfLine + ( w >> 1 );
						data[ 1 ][ pixelPos ] = dataForAlpha[ 1 ][ pixelPosAlpha ] * iA / 255 + cB * A / 255;
						data[ 2 ][ pixelPos ] = dataForAlpha[ 2 ][ pixelPosAlpha ] * iA / 255 + cR * A / 255;
					}
				}
			}
		}
	}
}

void Functions::ImageEQ( int Contrast, int Brightness, quint8 *imageBits, unsigned bitsCount )
{
	for ( unsigned i = 0 ; i < bitsCount ; i += 4 )
	{
		imageBits[ i+0 ] = clip8( imageBits[ i+0 ] * Contrast / 100 + Brightness );
		imageBits[ i+1 ] = clip8( imageBits[ i+1 ] * Contrast / 100 + Brightness );
		imageBits[ i+2 ] = clip8( imageBits[ i+2 ] * Contrast / 100 + Brightness );
	}
}
int Functions::scaleEQValue( int val, int min, int max )
{
	return ( val + 100 ) * ( ( abs( min ) + abs( max ) ) ) / 200 - abs( min );
}

QByteArray Functions::convertToASS( const QByteArray &arr ) //TODO: Colors?
{
	QString txt = arr;
	txt.replace( "<i>", "{\\i1}", Qt::CaseInsensitive );
	txt.replace( "</i>", "{\\i0}", Qt::CaseInsensitive );
	txt.replace( "<b>", "{\\b1}", Qt::CaseInsensitive );
	txt.replace( "</b>", "{\\b0}", Qt::CaseInsensitive );
	txt.replace( "<u>", "{\\u1}", Qt::CaseInsensitive );
	txt.replace( "</u>", "{\\u0}", Qt::CaseInsensitive );
	txt.replace( "<s>", "{\\s1}", Qt::CaseInsensitive );
	txt.replace( "</s>", "{\\s0}", Qt::CaseInsensitive );
	txt.remove( '\r' );
	txt.replace( '\n', "\\n", Qt::CaseInsensitive );
	return txt.toUtf8();
}

bool Functions::chkMimeData( const QMimeData *mimeData )
{
	return mimeData && ( ( mimeData->hasUrls() && !mimeData->urls().isEmpty() ) || ( mimeData->hasText() && !mimeData->text().isEmpty() ) );
}
QStringList Functions::getUrlsFromMimeData( const QMimeData *mimeData )
{
	QStringList urls;
	if ( mimeData->hasUrls() )
	{
		foreach ( const QUrl &url, mimeData->urls() )
		{
			QString u = url.toLocalFile();
			if ( u.length() > 1 && u.right( 1 ) == "/" )
				u.chop( 1 );
			if ( !u.isEmpty() )
				urls += u;
		}
	}
	else if ( mimeData->hasText() )
		urls = mimeData->text().remove( '\r' ).split( '\n', QString::SkipEmptyParts );
	return urls;
}

bool Functions::splitPrefixAndUrlIfHasPluginPrefix( const QString &whole_url, QString *addressPrefixName, QString *url, QString *param )
{
	int idx = whole_url.indexOf( "://{" );
	if ( idx > -1 )
	{
		if ( addressPrefixName )
			*addressPrefixName = whole_url.mid( 0, idx );
		if ( url || param )
		{
			idx += 4;
			int idx2 = whole_url.indexOf( "}", idx );
			if ( idx2 > -1 )
			{
				if ( param )
					*param = whole_url.mid( idx2+1, whole_url.length()-(idx2+1) );
				if ( url )
					*url = whole_url.mid( idx, idx2-idx );
			}
		}
		return ( !addressPrefixName || !addressPrefixName->isEmpty() ) && ( !url || !url->isNull() );
	}
	return false;
}
void Functions::getDataIfHasPluginPrefix(const QString &wholeUrl, QString *url, QString *name, QImage *img, IOController<> *ioCtrl, const DemuxersInfo &demuxersInfo )
{
	QString addressPrefixName, realUrl, param;
	if ( ( url || img ) && splitPrefixAndUrlIfHasPluginPrefix( wholeUrl, &addressPrefixName, &realUrl, &param ) )
	{
		foreach ( QMPlay2Extensions *QMPlay2Ext, QMPlay2Extensions::QMPlay2ExtensionsList() )
			if ( QMPlay2Ext->addressPrefixList( false ).contains( addressPrefixName ) )
			{
				QMPlay2Ext->convertAddress( addressPrefixName, realUrl, param, url, name, img, NULL, ioCtrl );
				break;
			}
	}
	else if ( img )
	{
		QString scheme = getUrlScheme( wholeUrl );
		QString extension = fileExt( wholeUrl ).toLower();
		if ( demuxersInfo.isEmpty() )
		{
			foreach ( Module *module, QMPlay2Core.getPluginsInstance() )
				foreach ( const Module::Info &mod, module->getModulesInfo() )
					if ( mod.type == Module::DEMUXER && ( mod.extensions.contains( extension ) || mod.name == scheme ) )
					{
						*img = !mod.img.isNull() ? mod.img : module->image();
						return;
					}
		}
		else
		{
			foreach ( const DemuxerInfo &demuxerInfo, demuxersInfo )
				if ( demuxerInfo.extensions.contains( extension ) || demuxerInfo.name == scheme )
				{
					*img = demuxerInfo.img;
					return;
				}
		}
	}
}

void Functions::hFlip( char *data, int linesize, int height, int width )
{
	int h, w, width_div_2 = width/2, linesize_div_2 = linesize/2, width_div_4 = width/4, offset = 0;
	for ( h = 0 ; h < height ; ++h )
	{
		for ( w = 0 ; w < width_div_2 ; ++w )
			qSwap( data[ offset + w ], data[ offset + width-1-w ] );
		offset += linesize;
	}
	for ( h = 0 ; h < height ; ++h )
	{
		for ( w = 0 ; w < width_div_4 ; ++w )
			qSwap( data[ offset + w ], data[ offset + width_div_2-1-w ] );
		offset += linesize_div_2;
	}
}
void Functions::vFlip( char *data, int linesize, int height )
{
	int size = linesize*height, chroma_size = linesize*height/4, chroma_width = linesize/2;
	char *data_s;

	for ( data_s = data + size ; data_s > data ; )
	{
		swapArray( data, data_s -= linesize, linesize );
		data += linesize;
	}
	data += size/2;

	for ( data_s = data + chroma_size ; data_s > data ; )
	{
		swapArray( data, data_s -= chroma_width, chroma_width );
		data += chroma_width;
	}
	data += chroma_size/2;

	for ( data_s = data + chroma_size ; data_s > data ; )
	{
		swapArray( data, data_s -= chroma_width, chroma_width );
		data += chroma_width;
	}
}

QString Functions::dBStr( double a )
{
	return ( !a ? "-∞" : QString::number( 20.0 * log10( a ), 'g', 3 ) ) + " dB";
}

quint32 Functions::getBestSampleRate()
{
	quint32 srate = 48000; //Use 48kHz as default
	if ( QMPlay2Core.getSettings().getBool( "ForceSamplerate" ) )
	{
		const quint32 choosenSrate = QMPlay2Core.getSettings().getUInt( "Samplerate" );
		if ( ( choosenSrate % 11025 ) == 0 )
			srate = 44100;
	}
	return srate;
}
