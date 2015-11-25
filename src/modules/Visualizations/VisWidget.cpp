#include <VisWidget.hpp>
#include <QMPlay2Core.hpp>
#include <DockWidget.hpp>

#include <QMouseEvent>
#include <QMenu>

VisWidget::VisWidget() :
	stopped( true ),
	dw( new DockWidget )
{
	setContextMenuPolicy( Qt::CustomContextMenu );
	setFocusPolicy( Qt::StrongFocus );
	setAutoFillBackground( true );
	setMouseTracking( true );
	setPalette( Qt::black );

	connect( &tim, SIGNAL( timeout() ), this, SLOT( update() ) );
	connect( dw, SIGNAL( visibilityChanged( bool ) ), this, SLOT( visibilityChanged( bool ) ) );
	connect( &QMPlay2Core, SIGNAL( wallpaperChanged( bool, double ) ), this, SLOT( wallpaperChanged( bool, double ) ) );
	connect( this, SIGNAL( customContextMenuRequested( const QPoint & ) ), this, SLOT( contextMenu( const QPoint & ) ) );
}

void VisWidget::mouseDoubleClickEvent( QMouseEvent *e )
{
	if ( parent() == dw && ( e->buttons() & Qt::LeftButton ) )
		emit doubleClicked();
	else
		QWidget::mouseDoubleClickEvent( e );
}
void VisWidget::changeEvent( QEvent *event )
{
	if ( event->type() == QEvent::ParentChange && !parent() )
		dw->setWidget( this );
	QWidget::changeEvent( event );
}

void VisWidget::wallpaperChanged( bool hasWallpaper, double alpha )
{
	QColor c = Qt::black;
	if ( hasWallpaper )
		c.setAlphaF( alpha );
	setPalette( c );
}
void VisWidget::contextMenu( const QPoint &point )
{
	QMenu *menu = new QMenu( this );
	connect( menu, SIGNAL( aboutToHide() ), menu, SLOT( deleteLater() ) );
	connect( menu->addAction( tr( "Ustawienia" ) ), SIGNAL( triggered() ), this, SLOT( showSettings() ) );
	menu->popup( mapToGlobal( point ) );
}
void VisWidget::visibilityChanged( bool v )
{
	if ( !v && parent() == dw )
		stop();
	else if ( !stopped )
		start( v );
}
void VisWidget::showSettings()
{
	emit QMPlay2Core.showSettings( "Visualizations" );
}
