#include <VideoDock.hpp>

#include <Settings.hpp>
#include <MenuBar.hpp>
#include <Main.hpp>

#include <Functions.hpp>
#include <SubsDec.hpp>

#include <QMouseEvent>
#include <QFileInfo>
#include <QMimeData>
#include <QPainter>
#include <QGesture>
#include <QMenu>

#include <math.h>

VideoDock::VideoDock() :
	iDW( QMPlay2Core.getQMPlay2Pixmap(), QMPlay2GUI.grad1, QMPlay2GUI.grad2, QMPlay2GUI.qmpTxt ),
	pixels( 0 ),
	canPopup( true ), is_floating( false ),
	touchZoom( 0.0 )
{
	setWindowTitle( tr( "Wideo" ) );

	popupMenu = new QMenu( this );
	popupMenu->addMenu( QMPlay2GUI.menuBar->window );
	popupMenu->addMenu( QMPlay2GUI.menuBar->widgets );
	popupMenu->addMenu( QMPlay2GUI.menuBar->playlist );
	popupMenu->addMenu( QMPlay2GUI.menuBar->player );
	popupMenu->addMenu( QMPlay2GUI.menuBar->playing );
	popupMenu->addMenu( QMPlay2GUI.menuBar->options );
	popupMenu->addMenu( QMPlay2GUI.menuBar->help );

	/* Menu actions which will be available in fullscreen or compact mode */
	foreach ( QAction *act, QMPlay2GUI.menuBar->window->actions() )
		addAction( act );
	foreach ( QAction *act, QMPlay2GUI.menuBar->playlist->actions() )
		addAction( act );
	foreach ( QAction *act, QMPlay2GUI.menuBar->player->actions() )
		addAction( act );
	foreach ( QAction *act, QMPlay2GUI.menuBar->playing->actions() )
		addAction( act );
	foreach ( QAction *act, QMPlay2GUI.menuBar->options->actions() )
		addAction( act );
	foreach ( QAction *act, QMPlay2GUI.menuBar->help->actions() )
		addAction( act );
	/**/

	setMouseTracking( true );
	setAcceptDrops( true );
	grabGesture( Qt::PinchGesture );
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
		/* Visualizations on full screen */
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
		if ( Functions::chkMimeData( mimeData ) )
		{
			QStringList urls = Functions::getUrlsFromMimeData( mimeData );
			if ( urls.size() == 1 )
			{
				QString url = Functions::Url( urls[ 0 ] );
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
		if ( ++pixels == 25 )
			internalWidget()->unsetCursor();
		hideCursorTim.start( 750 );
	}
	if ( e )
		DockWidget::mouseMoveEvent( e );
}
#ifndef Q_OS_MAC
void VideoDock::mouseDoubleClickEvent( QMouseEvent *e )
{
	if ( e->buttons() == Qt::LeftButton )
		QMPlay2GUI.menuBar->window->toggleFullScreen->trigger();
	DockWidget::mouseDoubleClickEvent( e );
}
#endif
void VideoDock::mousePressEvent( QMouseEvent *e )
{
	if ( e->button() == Qt::LeftButton )
		canPopup = false;
	if ( e->buttons() == Qt::MiddleButton )
		QMPlay2GUI.menuBar->player->togglePlay->trigger();
	else if ( ( e->buttons() & ( Qt::LeftButton | Qt::MiddleButton ) ) == ( Qt::LeftButton | Qt::MiddleButton ) )
		QMPlay2GUI.menuBar->player->reset->trigger();
	else if ( ( e->buttons() & ( Qt::LeftButton | Qt::RightButton ) ) == ( Qt::LeftButton | Qt::RightButton ) )
		QMPlay2GUI.menuBar->player->switchARatio->trigger();
	DockWidget::mousePressEvent( e );
}
void VideoDock::mouseReleaseEvent( QMouseEvent *e )
{
	if ( e->button() == Qt::LeftButton )
		canPopup = true;
	DockWidget::mouseReleaseEvent( e );
}
void VideoDock::moveEvent( QMoveEvent *e )
{
	if ( isFloating() )
		emit QMPlay2Core.videoDockMoved();
	DockWidget::moveEvent( e );
}
void VideoDock::wheelEvent( QWheelEvent *e )
{
	if ( e->orientation() == Qt::Vertical )
	{
		if ( e->buttons() & Qt::LeftButton )
			e->delta() > 0 ? QMPlay2GUI.menuBar->player->zoomIn->trigger() : QMPlay2GUI.menuBar->player->zoomOut->trigger();
		else if ( e->buttons() == Qt::NoButton && QMPlay2Core.getSettings().getBool( "ScrollSeek" ) )
			e->delta() > 0 ? QMPlay2GUI.menuBar->player->seekF->trigger() : QMPlay2GUI.menuBar->player->seekB->trigger();
	}
	DockWidget::wheelEvent( e );
}
void VideoDock::leaveEvent( QEvent *e )
{
	hideCursorTim.stop();
	if ( internalWidget() )
		internalWidget()->unsetCursor();
	pixels = 0;
	DockWidget::leaveEvent( e );
}
void VideoDock::enterEvent( QEvent *e )
{
	mouseMoveEvent( NULL );
	DockWidget::enterEvent( e );
}
bool VideoDock::event( QEvent *e )
{
	if ( e->type() == QEvent::Gesture )
	{
		QPinchGesture *pinch = ( QPinchGesture * )( ( QGestureEvent * )e )->gesture( Qt::PinchGesture );
		if ( pinch )
		{
			if ( pinch->state() == Qt::GestureStarted )
				touchZoom = 0.0;
			else if ( pinch->state() == Qt::GestureUpdated )
			{
				touchZoom += pinch->scaleFactor() - 1.0;
				if ( fabs( touchZoom ) >= 0.05 )
				{
					if ( touchZoom > 0.00 )
						QMPlay2GUI.menuBar->player->zoomIn->trigger();
					else if ( touchZoom < 0.00 )
						QMPlay2GUI.menuBar->player->zoomOut->trigger();
					touchZoom = 0.0;
				}
			}
		}
	}
	return DockWidget::event( e );
}

void VideoDock::popup( const QPoint &p )
{
	if ( canPopup )
		popupMenu->popup( mapToGlobal( p ) );
}
void VideoDock::hideCursor()
{
	hideCursorTim.stop();
	if ( internalWidget() )
		internalWidget()->setCursor( Qt::BlankCursor );
	pixels = 0;
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
