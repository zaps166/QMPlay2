#include <XVideoWriter.hpp>

#include <QMPlay2_OSD.hpp>
#include <Functions.hpp>
using Functions::getImageSize;

#include <QDebug>

/**/

Drawable::Drawable( XVideoWriter &writer ) :
	writer( writer )
{
	setAttribute( Qt::WA_OpaquePaintEvent );
	setAttribute( Qt::WA_PaintOnScreen );
	setMouseTracking( true );
}

void Drawable::resizeEvent( QResizeEvent * )
{
	getImageSize( writer.aspect_ratio, writer.zoom, width(), height(), W, H, &X, &Y, &dstRect, &writer.outW, &writer.outH, &srcRect );
	repaint();
}
void Drawable::paintEvent( QPaintEvent * )
{
	writer.xv->redraw( srcRect, dstRect, X, Y, W, H, width(), height() );
}

/**/

XVideoWriter::XVideoWriter( Module &module )
{
	addParam( "W" );
	addParam( "H" );
	addParam( "AspectRatio" );
	addParam( "Zoom" );
	addParam( "Flip" );
	addParam( "Saturation" );
	addParam( "Brightness" );
	addParam( "Contrast" );
	addParam( "Hue" );

	xv = NULL;
	drawable = NULL;
	outW = outH = -1;
	Hue = Saturation = Brightness = Contrast = 0;

	SetModule( module );
}

XVideoWriter::~XVideoWriter()
{
	delete drawable;
	delete xv;
}

bool XVideoWriter::set()
{
	bool restartPlaying = false;

	QString _adaptorName = sets().getString( "Adaptor" );
	if ( !XVIDEO::adaptorsList().contains( _adaptorName ) )
		_adaptorName.clear();
	bool _useSHM = sets().getBool( "UseSHM" );

	if ( _adaptorName != adaptorName )
	{
		restartPlaying = true;
		adaptorName = _adaptorName;
	}
	if ( _useSHM != useSHM )
	{
		restartPlaying = true;
		useSHM = _useSHM;
	}

	return !restartPlaying && sets().getBool( "Enabled" );
}

bool XVideoWriter::readyWrite() const
{
	return xv && xv->isOpen();
}

bool XVideoWriter::processParams( bool * )
{
	bool doResizeEvent = false;

	double _aspect_ratio = getParam( "AspectRatio" ).toDouble();
	double _zoom = getParam( "Zoom" ).toDouble();
	int _flip = getParam( "Flip" ).toInt();
	int _Hue = getParam( "Hue" ).toInt();
	int _Saturation = getParam( "Saturation" ).toInt();
	int _Brightness = getParam( "Brightness" ).toInt();
	int _Contrast = getParam( "Contrast" ).toInt();
	bool setEQ = _Hue != Hue || _Saturation != Saturation || _Brightness != Brightness || _Contrast != Contrast;
	int _outW = getParam( "W" ).toInt();
	int _outH = getParam( "H" ).toInt();
	bool videoSizeChanged = _outW != outW || _outH != outH;
	if ( _aspect_ratio != aspect_ratio || _zoom != zoom || _flip != xv->flip() || setEQ || videoSizeChanged )
	{
		zoom = _zoom;
		aspect_ratio = _aspect_ratio;
		xv->setFlip( _flip );
		Hue = _Hue;
		Saturation = _Saturation;
		Brightness = _Brightness;
		Contrast = _Contrast;
		doResizeEvent = drawable->isVisible();
	}

	if ( _outW > 0 && _outH > 0 && videoSizeChanged )
	{
		outW = _outW;
		outH = _outH;
		setEQ = true;

		QMPlay2Core.dockVideo( drawable );
		xv->open( outW, outH, drawable->winId(), adaptorName, useSHM );
	}

	if ( setEQ )
		xv->setVideoEqualizer( Hue, Saturation, Brightness, Contrast );

	if ( doResizeEvent )
		drawable->resizeEvent( NULL );

	return readyWrite();
}

qint64 XVideoWriter::write( const QByteArray &arr )
{
	xv->draw( arr, drawable->srcRect, drawable->dstRect, drawable->W, drawable->H, osd_list, osd_mutex );
	return arr.size();
}
void XVideoWriter::writeOSD( const QList< const QMPlay2_OSD * > &osds )
{
	osd_mutex.lock();
	osd_list = osds;
	osd_mutex.unlock();
}

QString XVideoWriter::name() const
{
	return XVideoWriterName;
}

bool XVideoWriter::open()
{
	xv = new XVIDEO;
	if ( xv->isOK() )
	{
		drawable = new Drawable( *this );
		return true;
	}
	delete xv;
	xv = NULL;
	return false;
}
