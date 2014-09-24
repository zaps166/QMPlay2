#include <LineEdit.hpp>
#include <QMPlay2Core.hpp>

#include <QResizeEvent>

LineEditButton::LineEditButton()
{
	setToolTip( tr( "Wyczyść" ) );
	setPixmap( QMPlay2Core.getIconFromTheme( "edit-clear" ).pixmap( 16, 16 ) );
	resize( pixmap()->size() );
	setCursor( Qt::ArrowCursor );
}

void LineEditButton::mousePressEvent( QMouseEvent *e )
{
	if ( e->button() & Qt::LeftButton )
		emit clicked();
}

/**/

LineEdit::LineEdit( QWidget *parent ) : QLineEdit( parent )
{
	connect( this, SIGNAL( textChanged( const QString & ) ), this, SLOT( textChangedSlot( const QString & ) ) );
	connect( &b, SIGNAL( clicked() ), this, SLOT( clear_text() ) );
	setMinimumWidth( b.width() * 2.5 );
	setTextMargins( 0, 0, b.width() * 1.5, 0 );
	b.setParent( this );
	b.hide();
}

void LineEdit::resizeEvent( QResizeEvent *e )
{
	b.move( e->size().width() - b.width() * 1.5, e->size().height() / 2 - b.height() / 2 );
}
void LineEdit::mousePressEvent( QMouseEvent *e )
{
	if ( !b.underMouse() )
		QLineEdit::mousePressEvent( e );
}
void LineEdit::mouseMoveEvent( QMouseEvent *e )
{
	if ( !b.underMouse() )
		QLineEdit::mouseMoveEvent( e );
}

void LineEdit::textChangedSlot( const QString &str )
{
	b.setVisible( !str.isEmpty() );
}
void LineEdit::clear_text()
{
	clear();
	emit clearButtonClicked();
}
