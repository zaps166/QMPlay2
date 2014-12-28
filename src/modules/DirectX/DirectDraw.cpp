#include <DirectDraw.hpp>

#include <QMPlay2_OSD.hpp>
#include <VideoFrame.hpp>
#include <Functions.hpp>
using Functions::getImageSize;

#include <QPainter>

#include <math.h>

#define ColorKEY 0x00000001

/**/

Drawable::Drawable( DirectDrawWriter &writer ) :
	writer( writer ),
	DDraw( NULL ), DDClipper( NULL ), DDSPrimary( NULL ), DDSSecondary( NULL ), DDSBackBuffer( NULL ), DDrawColorCtrl( NULL ),
	MemoryFlag( 0 ), mode( NONE )
{
	setMouseTracking( true );
	setAutoFillBackground( true );
	if ( DirectDrawCreate( NULL, &DDraw, NULL ) == DD_OK && DDraw->SetCooperativeLevel( NULL, DDSCL_NORMAL ) == DD_OK )
	{
		DDSURFACEDESC ddsd_primary = { sizeof ddsd_primary };
		ddsd_primary.dwFlags = DDSD_CAPS;
		ddsd_primary.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
		if ( DDraw->CreateSurface( &ddsd_primary, &DDSPrimary, NULL ) == DD_OK )
		{
			DDCAPS ddCaps = { sizeof ddCaps };
			LPDIRECTDRAWSURFACE DDrawTestSurface;
			DDSURFACEDESC ddsd_test = { sizeof ddsd_test };
			ddsd_test.dwWidth = ddsd_test.dwHeight = 1;
			ddsd_test.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;

			/* Prepare for Overlay and YV12 test */
			ddsd_test.ddpfPixelFormat.dwSize = sizeof ddsd_test.ddpfPixelFormat;
			ddsd_test.ddpfPixelFormat.dwFlags = DDPF_FOURCC;
			ddsd_test.ddpfPixelFormat.dwFourCC = MAKEFOURCC( 'Y', 'V', '1', '2' );

			/* Overlay test */
			DDraw->GetCaps( &ddCaps, NULL );
			if ( ddCaps.dwCaps & ( DDCAPS_OVERLAY | DDCAPS_OVERLAYFOURCC | DDCAPS_OVERLAYSTRETCH ) )
			{
				ddsd_test.ddsCaps.dwCaps = DDSCAPS_OVERLAY | DDSCAPS_VIDEOMEMORY;
				if ( DDraw->CreateSurface( &ddsd_test, &DDrawTestSurface, NULL ) == DD_OK )
				{
					RECT destRect = { 0, 0, 1, 1 };
					if ( DDrawTestSurface->UpdateOverlay( NULL, DDSPrimary, &destRect, DDOVER_SHOW, NULL ) == DD_OK )
					{
						DDrawTestSurface->UpdateOverlay( NULL, DDSPrimary, NULL, DDOVER_HIDE, NULL );
						mode = OVERLAY;
						MemoryFlag = DDSCAPS_VIDEOMEMORY;
					}
					DDrawTestSurface->Release();
				}
			}

			if ( mode == NONE && writer.onlyOverlay )
			{
				QMPlay2Core.logInfo( "DirectDraw :: " + tr( "Nakładka wyłączona, DirectDraw nie działa..." ), true );
				return;
			}

			/* YV12 test */
			if ( mode == NONE )
			{
				ddsd_test.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY;
				if ( DDraw->CreateSurface( &ddsd_test, &DDrawTestSurface, NULL ) == DD_OK )
				{
					mode = YV12;
					MemoryFlag = DDSCAPS_VIDEOMEMORY;
					QMPlay2Core.logInfo( "DirectDraw :: " + tr( "Sprzętowa konwersja YUV->RGB" ), true );
					DDrawTestSurface->Release();
				}
			}

			/* RGB32 test */
			if ( mode == NONE )
			{
				ddsd_test.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY;
				ddsd_test.dwFlags &= ~DDSD_PIXELFORMAT;
				ZeroMemory( &ddsd_test.ddpfPixelFormat, sizeof ddsd_test.ddpfPixelFormat );
				if ( DDraw->CreateSurface( &ddsd_test, &DDrawTestSurface, NULL ) == DD_OK )
				{
					mode = RGB32;
					MemoryFlag = DDSCAPS_VIDEOMEMORY;
					QMPlay2Core.logInfo( "DirectDraw :: " + tr( "Programowa konwersja YUV->RGB, pamięć karty grafiki" ), true );
				}
				else //last resort, RGB32 in system memory :D
				{
					ddsd_test.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
					if ( DDraw->CreateSurface( &ddsd_test, &DDrawTestSurface, NULL ) == DD_OK )
					{
						mode = RGB32;
						MemoryFlag = DDSCAPS_SYSTEMMEMORY;
						QMPlay2Core.logInfo( "DirectDraw :: " + tr( "Programowa konwersja YUV->RGB, pamięć systemowa" ), true );
						DDrawTestSurface->Release();
					}
				}
			}

			if ( mode != NONE )
			{
				if ( mode == OVERLAY )
				{
					setPalette( QColor( ColorKEY ) );
					connect( &QMPlay2Core, SIGNAL( mainWidgetMoved() ), this, SLOT( updateOverlay() ) );
					connect( &QMPlay2Core, SIGNAL( videoDockVisible( bool ) ), this, SLOT( overlayVisible( bool ) ) );
				}
				else
				{
					setPalette( Qt::black );
					if ( DDraw->CreateClipper( 0, &DDClipper, NULL ) == DD_OK )
						DDSPrimary->SetClipper( DDClipper );
				}
			}
		}
	}
}
Drawable::~Drawable()
{
	releaseSecondary();
	if ( DDSPrimary )
		DDSPrimary->Release();
	if ( DDClipper )
		DDClipper->Release();
	if ( DDraw )
		DDraw->Release();
}

void Drawable::dock()
{
	QMPlay2Core.dockVideo( this );
	if ( DDClipper )
		DDClipper->SetHWnd( 0, winId() );
}
bool Drawable::draw( const QByteArray &videoFrameData )
{
	DDSURFACEDESC ddsd = { sizeof ddsd };
	if ( restoreLostSurface() && mode == OVERLAY )
		updateOverlay();
	if ( DDSBackBuffer->Lock( NULL, &ddsd, DDLOCK_WAIT | DDLOCK_NOSYSLOCK, NULL ) == DD_OK )
	{
		BYTE *surface = ( BYTE * )ddsd.lpSurface;
		if ( mode != RGB32 ) //Overlay and YV12
		{
			VideoFrame::copyYV12( surface, videoFrameData, ddsd.lPitch, ddsd.lPitch >> 1, ddsd.dwHeight );
			osd_mutex.lock();
			if ( !osd_list.isEmpty() )
			{
				if ( osdImg.size() != QSize( ddsd.dwWidth, ddsd.dwHeight ) )
				{
					osdImg = QImage( ddsd.dwWidth, ddsd.dwHeight, QImage::Format_ARGB32 );
					osdImg.fill( 0 );
				}
				Functions::paintOSDtoYV12( surface, videoFrameData, osdImg, W, H, ddsd.lPitch, ddsd.lPitch >> 1, osd_list, osd_checksums );
			}
			osd_mutex.unlock();
		}
		else if ( imgScaler.create( writer.outW, writer.outH, ddsd.lPitch >> 2, writer.outH ) )
		{
			imgScaler.scale( VideoFrame::fromData( videoFrameData ), surface );
			osd_mutex.lock();
			if ( !osd_list.isEmpty() )
			{
				QImage osdImg( surface, ddsd.lPitch >> 2, ddsd.dwHeight, QImage::Format_ARGB32 );
				QPainter p( &osdImg );
				p.setRenderHint( QPainter::SmoothPixmapTransform );
				p.scale( ( qreal )( ddsd.lPitch >> 2 ) / W, ( qreal )ddsd.dwHeight / H );
				Functions::paintOSD( osd_list, ( qreal )W / ( ddsd.lPitch >> 2 ), ( qreal )H / ddsd.dwHeight, p );
			}
			osd_mutex.unlock();
		}
		if ( DDSBackBuffer->Unlock( NULL ) == DD_OK )
		{
			if ( mode == OVERLAY )
				DDSSecondary->Flip( NULL, DDFLIP_WAIT );
			else
				blit();
			return true;
		}
	}
	return false;
}
bool Drawable::createSecondary()
{
	releaseSecondary();

	DDSURFACEDESC ddsd = { sizeof ddsd };
	ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
	ddsd.dwWidth = writer.outW;
	ddsd.dwHeight = writer.outH;
	ddsd.ddsCaps.dwCaps |= MemoryFlag;

	if ( mode == RGB32 || mode == YV12 )
		ddsd.ddsCaps.dwCaps |= DDSCAPS_OFFSCREENPLAIN;
	if ( mode == OVERLAY )
	{
		ddsd.ddsCaps.dwCaps |= DDSCAPS_OVERLAY | DDSCAPS_COMPLEX | DDSCAPS_FLIP;
		ddsd.dwFlags |= DDSD_BACKBUFFERCOUNT;
		ddsd.dwBackBufferCount = 1;
	}
	if ( mode == OVERLAY || mode == YV12 )
	{
		ddsd.dwFlags |= DDSD_PIXELFORMAT;
		ddsd.ddpfPixelFormat.dwSize = sizeof ddsd.ddpfPixelFormat;
		ddsd.ddpfPixelFormat.dwFlags = DDPF_FOURCC;
		ddsd.ddpfPixelFormat.dwFourCC = MAKEFOURCC( 'Y', 'V', '1', '2' );
	}

	if ( DDraw->CreateSurface( &ddsd, &DDSSecondary, NULL ) == DD_OK )
	{
		DDSSecondary->QueryInterface( IID_IDirectDrawColorControl, ( LPVOID * )&DDrawColorCtrl );
		if ( mode == OVERLAY )
		{
			DDSCAPS ddsCaps = { sizeof ddsCaps };
			ddsCaps.dwCaps = DDSCAPS_BACKBUFFER;
			DDSSecondary->GetAttachedSurface( &ddsCaps, &DDSBackBuffer );
		}
		if ( !DDSBackBuffer )
			DDSBackBuffer = DDSSecondary;
		if ( mode != RGB32 && DDSBackBuffer->Lock( NULL, &ddsd, DDLOCK_WAIT, NULL ) == DD_OK ) //Black on start
		{
			const DWORD size = ddsd.lPitch * ddsd.dwHeight;
			memset( ( BYTE * )ddsd.lpSurface, 0x00, size );
			memset( ( BYTE * )ddsd.lpSurface + size, 0x7F, size / 2 );
			if ( DDSBackBuffer->Unlock( NULL ) == DD_OK )
			{
				if ( mode == OVERLAY )
					DDSSecondary->Flip( NULL, DDFLIP_WAIT );
				return true;
			}
		}
		else if ( mode == RGB32 )
			return true;
	}

	return false;
}
void Drawable::colorSet( int Hue, int Saturation, int Brightness, int Contrast )
{
	if ( DDrawColorCtrl )
	{
		DDCOLORCONTROL DDCC =
		{
			sizeof DDCC,
			DDCOLOR_HUE | DDCOLOR_SATURATION | DDCOLOR_BRIGHTNESS | DDCOLOR_CONTRAST,
			Brightness <= 0 ? ( Brightness + 100 ) * 15 / 2 : Brightness * 100,
			( Contrast + 100 ) * 100,
			Hue * 18 / 10,
			( Saturation + 100 ) * 100,
		};
		DDrawColorCtrl->SetColorControls( &DDCC );
	}
}

void Drawable::resizeEvent( QResizeEvent *e )
{
	getImageSize( writer.aspect_ratio, writer.zoom, width(), height(), W, H, &X, &Y );

	if ( mode == OVERLAY )
		updateOverlay();
	else
		blit();

	if ( e )
		QWidget::resizeEvent( e );
}

void Drawable::getRects( RECT &srcRect, RECT &destRect )
{
	const QPoint point = mapToGlobal( QPoint( X, Y ) );
	const RECT dR = { point.x(), point.y(), point.x() + W, point.y() + H };
	const RECT screenRect = { 0, 0, GetSystemMetrics( SM_CXSCREEN ), GetSystemMetrics( SM_CYSCREEN ) };

	IntersectRect( &destRect, &dR, &screenRect );

	if ( dR.right - dR.left && dR.bottom - dR.top )
	{
		srcRect.left   = ( destRect.left - dR.left ) * writer.outW / ( dR.right - dR.left );
		srcRect.top    = ( destRect.top - dR.top )   * writer.outH / ( dR.bottom - dR.top );
		srcRect.right  = writer.outW - ( dR.right - destRect.right   ) * writer.outW / ( dR.right - dR.left );
		srcRect.bottom = writer.outH - ( dR.bottom - destRect.bottom ) * writer.outH / ( dR.bottom - dR.top );
	}
	else
		memset( &srcRect, 0, sizeof srcRect );
}

void Drawable::updateOverlay()
{
	RECT srcRect, destRect;
	getRects( srcRect, destRect );

	restoreLostSurface();

	DDOVERLAYFX ovfx = { sizeof ovfx };
	ovfx.dckDestColorkey.dwColorSpaceHighValue = ovfx.dckDestColorkey.dwColorSpaceLowValue = ColorKEY;
	if ( writer.flip & Qt::Horizontal )
		ovfx.dwDDFX |= DDOVERFX_MIRRORLEFTRIGHT;
	if ( writer.flip & Qt::Vertical )
		ovfx.dwDDFX |= DDOVERFX_MIRRORUPDOWN;
	DDSSecondary->UpdateOverlay( &srcRect, DDSPrimary, &destRect, DDOVER_SHOW | DDOVER_KEYDESTOVERRIDE | DDOVER_DDFX, &ovfx );
}
void Drawable::overlayVisible( bool v )
{
	if ( v )
		updateOverlay();
	else
		DDSSecondary->UpdateOverlay( NULL, DDSPrimary, NULL, DDOVER_HIDE, NULL );
}
void Drawable::blit()
{
	RECT srcRect, destRect;
	getRects( srcRect, destRect );

	restoreLostSurface();

	DDBLTFX bltfx = { sizeof bltfx };
	bltfx.dwDDFX = DDBLTFX_NOTEARING;
	if ( writer.flip & Qt::Horizontal )
		bltfx.dwDDFX |= DDBLTFX_MIRRORLEFTRIGHT;
	if ( writer.flip & Qt::Vertical )
		bltfx.dwDDFX |= DDBLTFX_MIRRORUPDOWN;
	DDSPrimary->Blt( &destRect, DDSSecondary, &srcRect, DDBLT_WAIT | DDBLT_DDFX, &bltfx );
}

void Drawable::releaseSecondary()
{
	if ( DDSSecondary )
	{
		if ( DDrawColorCtrl )
		{
			DDrawColorCtrl->Release();
			DDrawColorCtrl = NULL;
		}
		if ( mode == OVERLAY )
			DDSSecondary->UpdateOverlay( NULL, DDSPrimary, NULL, DDOVER_HIDE, NULL );
		else if ( mode == RGB32 )
			imgScaler.destroy();
		DDSSecondary->Release();
		DDSSecondary = NULL;
		DDSBackBuffer = NULL;
	}
}

/**/

DirectDrawWriter::DirectDrawWriter( Module &module ) :
	outW( -1 ), outH( -1 ), flip( 0 ), Hue( 0 ), Saturation( 0 ), Brightness( 0 ), Contrast( 0 ),
	aspect_ratio( 0.0 ), zoom( 0.0 ),
	onlyOverlay( true ),
	drawable( NULL )
{
	addParam( "W" );
	addParam( "H" );
	addParam( "AspectRatio" );
	addParam( "Zoom" );
	addParam( "Hue" );
	addParam( "Saturation" );
	addParam( "Brightness" );
	addParam( "Contrast" );
	addParam( "Flip" );

	SetModule( module );
}
DirectDrawWriter::~DirectDrawWriter()
{
	delete drawable;
}

bool DirectDrawWriter::set()
{
	if ( onlyOverlay != sets().getBool( "DirectDraw/OnlyOverlay" ) )
	{
		onlyOverlay = sets().getBool( "DirectDraw/OnlyOverlay" );
		return false;
	}
	return sets().getBool( "DirectDraw" );
}

bool DirectDrawWriter::readyWrite() const
{
	return drawable && drawable->canDraw();
}

bool DirectDrawWriter::processParams( bool * )
{
	bool doResizeEvent = drawable->isVisible();

	const double _aspect_ratio = getParam( "AspectRatio" ).toDouble();
	const double _zoom = getParam( "Zoom" ).toDouble();
	const int _flip = getParam( "Flip" ).toInt();
	if ( _aspect_ratio != aspect_ratio || _zoom != zoom || _flip != flip )
	{
		zoom = _zoom;
		aspect_ratio = _aspect_ratio;
		flip = _flip;
	}

	const int _outW = getParam( "W" ).toInt();
	const int _outH = getParam( "H" ).toInt();
	if ( _outW > 0 && _outH > 0 && ( _outW != outW || _outH != outH ) )
	{
		outW = _outW;
		outH = _outH;

		if ( drawable->createSecondary() )
			drawable->dock();
	}

	const int _Hue = getParam( "Hue" ).toInt();
	const int _Saturation = getParam( "Saturation" ).toInt();
	const int _Brightness = getParam( "Brightness" ).toInt();
	const int _Contrast = getParam( "Contrast" ).toInt();
	if ( _Hue != Hue || _Saturation != Saturation || _Brightness != Brightness || _Contrast != Contrast )
		drawable->colorSet( Hue = _Hue, Saturation = _Saturation, Brightness = _Brightness, Contrast = _Contrast );

	if ( doResizeEvent )
		drawable->resizeEvent( NULL );

	return readyWrite();
}

qint64 DirectDrawWriter::write( const QByteArray &arr )
{
	int ret = 0;
	if ( drawable->draw( arr ) )
		ret = arr.size();
	VideoFrame::unref( arr );
	return ret;
}
void DirectDrawWriter::writeOSD( const QList< const QMPlay2_OSD * > &osds )
{
	drawable->osd_mutex.lock();
	drawable->osd_list = osds;
	drawable->osd_mutex.unlock();
	if ( drawable->isOverlay() )
		drawable->update();
}

QString DirectDrawWriter::name() const
{
	return DirectDrawWriterName;
}

bool DirectDrawWriter::open()
{
	drawable = new Drawable( *this );
	return drawable->isOK();
}
