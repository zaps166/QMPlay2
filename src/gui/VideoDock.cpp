#include <VideoDock.hpp>

#include <Settings.hpp>
#include <MenuBar.hpp>
#include <Main.hpp>

#include <Functions.hpp>
#include <SubsDec.hpp>

using Functions::chkMimeData;
using Functions::getUrlsFromMimeData;
using Functions::Url;

#include <QMouseEvent>
#include <QFileInfo>
#include <QMimeData>
#include <QPainter>
#include <QMenu>

VideoDock::VideoDock() :
	iDW( QMPlay2Core.getQMPlay2Pixmap(), QMPlay2GUI.grad1, QMPlay2GUI.grad2, QMPlay2GUI.qmpTxt ),
#ifndef Q_OS_MAC
	pixels( 0 ),
#endif
	cantpopup( false ), is_floating( false )
{
	setWindowTitle( tr( "Wideo" ) );

	popupMenu = new QMenu( this );
	popupMenu->addMenu( QMPlay2GUI.menubar->window );
	popupMenu->addMenu( QMPlay2GUI.menubar->widgets );
	popupMenu->addMenu( QMPlay2GUI.menubar->playlist );
	popupMenu->addMenu( QMPlay2GUI.menubar->player );
	popupMenu->addMenu( QMPlay2GUI.menubar->playing );
	popupMenu->addMenu( QMPlay2GUI.menubar->options );
	popupMenu->addMenu( QMPlay2GUI.menubar->help );

	setMouseTracking( true );
	setAcceptDrops( true );
	setContextMenuPolicy( Qt::CustomContextMenu );

	setWidget( &iDW );

	connect( &hideCursorTim, SIGNAL( timeout() ), this, SLOT( hideCursor() ) );
	connect( this, SIGNAL( customContextMenuRequested( const QPoint & ) ), this, SLOT( popup( const QPoint & ) ) );
	connect( &iDW, SIGNAL( resized( int, int ) ), this, SLOT( resizedIDW( int, int ) ) );
	connect( this, SIGNAL( visibilityChanged( bool ) ), this, SLOT( visibilityChanged( bool ) ) );
	connect( &QMPlay2Core, SIGNAL( dockVideo( QWidget * ) ), &iDW, SLOT( setWidget( QWidget * ) ) );
}

void VideoDock::fullScreen( bool b )
{
	if ( b )
	{
		is_floating = isFloating();

		setTitleBarVisible( false );
		setFeatures( DockWidget::NoDockWidgetFeatures );
		setFloating( false );

		setStyle( &commonStyle );
	}
	else
	{
		if ( widget() != &iDW )
		{
			if ( widget() )
			{
				widget()->unsetCursor();
				widget()->setParent( NULL );
			}
			setWidget( &iDW );
		}

		setTitleBarVisible();
		setFeatures( DockWidget::AllDockWidgetFeatures );
		setFloating( is_floating );

		setStyle( NULL );
	}
}

void VideoDock::dragEnterEvent( QDragEnterEvent *e )
{
	if ( e )
	{
		e->accept();
		QWidget::dragEnterEvent( e );
	}
}
void VideoDock::dropEvent( QDropEvent *e )
{
	if ( e )
	{
		const QMimeData *mimeData = e->mimeData();
		if ( chkMimeData( mimeData ) )
		{
			QStringList urls = getUrlsFromMimeData( mimeData );
			if ( urls.size() == 1 )
			{
				QString url = Url( urls[ 0 ] );
#ifdef Q_OS_WIN
				if ( url.left( 7 ) == "file://" )
					url.remove( 0, 7 );
#endif
				if ( !QFileInfo( url.mid( 7 ) ).isDir() )
				{
					bool subtitles = false;
					QString fileExt = Functions::fileExt( url ).toLower();
					if ( !fileExt.isEmpty() && ( fileExt == "ass" || fileExt == "ssa" || SubsDec::extensions().contains( fileExt ) ) )
						subtitles = true;
#ifdef Q_OS_WIN
					if ( subtitles && !url.contains( "://" ) )
						url.prepend( "file://" );
#endif
					emit itemDropped( url, subtitles );
				}
			}
		}
		QWidget::dropEvent( e );
	}
}
void VideoDock::mouseMoveEvent( QMouseEvent *e )
{
	if ( internalWidget() )
	{
#ifndef Q_OS_MAC
		if ( ++pixels == 25 )
#endif
			internalWidget()->unsetCursor();
		hideCursorTim.start( 750 );
	}
	if ( e )
		DockWidget::mouseMoveEvent( e );
}
void VideoDock::mouseDoubleClickEvent( QMouseEvent *e )
{
	if ( e->buttons() == Qt::LeftButton )
		QMPlay2GUI.menubar->window->toggleFullScreen->trigger();
	DockWidget::mouseDoubleClickEvent( e );
}
void VideoDock::mousePressEvent( QMouseEvent *e )
{
	if ( e->buttons() == Qt::MiddleButton )
		QMPlay2GUI.menubar->player->togglePlay->trigger();
	else if ( ( e->buttons() & ( Qt::LeftButton | Qt::MiddleButton ) ) == ( Qt::LeftButton | Qt::MiddleButton ) )
		QMPlay2GUI.menubar->player->reset->trigger();
	else if ( ( e->buttons() & ( Qt::LeftButton | Qt::RightButton ) ) == ( Qt::LeftButton | Qt::RightButton ) )
	{
		QMPlay2GUI.menubar->player->switchARatio->trigger();
		cantpopup = true;
	}
	DockWidget::mousePressEvent( e );
}
void VideoDock::wheelEvent( QWheelEvent *e )
{
	if ( e->orientation() == Qt::Vertical )
	{
		if ( e->buttons() & Qt::LeftButton )
			e->delta() > 0 ? QMPlay2GUI.menubar->player->zoomIn->trigger() : QMPlay2GUI.menubar->player->zoomOut->trigger();
		else if ( e->buttons() == Qt::NoButton && QMPlay2Core.getSettings().getBool( "ScrollSeek" ) )
			e->delta() > 0 ? QMPlay2GUI.menubar->player->seekF->trigger() : QMPlay2GUI.menubar->player->seekB->trigger();
	}
}
void VideoDock::leaveEvent( QEvent *e )
{
	hideCursorTim.stop();
	if ( internalWidget() )
		internalWidget()->unsetCursor();
#ifndef Q_OS_MAC
	pixels = 0;
#endif
	DockWidget::leaveEvent( e );
}
void VideoDock::enterEvent( QEvent *e )
{
	mouseMoveEvent( NULL );
	DockWidget::enterEvent( e );
}

void VideoDock::popup( const QPoint &p )
{
	if ( cantpopup )
		cantpopup = false;
	else
		popupMenu->popup( mapToGlobal( p ) );
}
void VideoDock::hideCursor()
{
	hideCursorTim.stop();
	if ( internalWidget() )
		internalWidget()->setCursor( Qt::BlankCursor );
#ifndef Q_OS_MAC
	pixels = 0;
#endif
}
void VideoDock::resizedIDW( int w, int h )
{
	emit resized( w, h );
}
void VideoDock::updateImage( const QImage &img )
{
	iDW.setCustomPixmap( QPixmap::fromImage( img ) );
}
void VideoDock::visibilityChanged( bool v )
{
	emit QMPlay2Core.videoDockVisible( v );
}
