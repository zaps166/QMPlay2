#include <Slider.hpp>

#include <QStyleOptionSlider>
#include <QMouseEvent>
#include <QPainter>
#include <QStyle>

Slider::Slider() :
	QSlider( Qt::Horizontal ),
	canSetValue( true ), _ignoreValueChanged( false ),
	wheelStep( 5 ), firstLine( -1 ), secondLine( -1 )
{
	setMouseTracking( true );
}

void Slider::setValue( int val )
{
	if ( canSetValue )
	{
		_ignoreValueChanged = true;
		QSlider::setValue( val );
		_ignoreValueChanged = false;
	}
}

void Slider::drawRange( int first, int second )
{
	firstLine  = first;
	secondLine = second;

	if ( secondLine > maximum() )
		secondLine = maximum();
	if ( firstLine > secondLine )
		firstLine = -1;

	repaint();
}

void Slider::paintEvent( QPaintEvent *e )
{
	int o, firstX, secondX;
	QSlider::paintEvent( e );
	if ( ( firstLine > -1 || secondLine > -1 ) && maximum() > 0 )
	{
		QPainter p( this );

		QStyleOptionSlider opt;
		initStyleOption( &opt );

		QRect handle = style()->subControlRect( QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, this );

		o = style()->pixelMetric( QStyle::PM_SliderLength ) - 1;
		if ( firstLine > -1 )
		{
			firstX  = QStyle::sliderPositionFromValue( minimum(), maximum(), firstLine,  width() - o, false ) + o / 2;
			p.drawLine( firstX, handle.y(), firstX, handle.height() );
		}
		if ( secondLine > -1 )
		{
			secondX = QStyle::sliderPositionFromValue( minimum(), maximum(), secondLine, width() - o, false ) + o / 2;
			p.drawLine( secondX, handle.y(), secondX, handle.height() );
		}
	}
}
void Slider::mousePressEvent( QMouseEvent *e )
{
	canSetValue = false;
	if ( e->buttons() != Qt::LeftButton )
		QSlider::mousePressEvent( e );
	else
	{
		QMouseEvent ev( e->type(), e->pos(), Qt::MidButton, Qt::MidButton, e->modifiers() );
		QSlider::mousePressEvent( &ev );
	}
}
void Slider::mouseReleaseEvent( QMouseEvent *e )
{
	canSetValue = true;
	QSlider::mouseReleaseEvent( e );
}
void Slider::mouseMoveEvent( QMouseEvent *e )
{
	if ( maximum() > 0 )
	{
		int pos = getMousePos( e->pos().x() );
		if ( pos != lastMousePos )
		{
			lastMousePos = pos;
			if ( pos < 0 )
				pos = 0;
			emit mousePosition( pos );
		}
	}
	QSlider::mouseMoveEvent( e );
}
void Slider::wheelEvent( QWheelEvent *e )
{
	int v;
	if ( e->delta() > 0 )
		v = value() + wheelStep;
	else
		v = value() - wheelStep;
	v -= v % wheelStep;
	QSlider::setValue( v );
}
void Slider::enterEvent( QEvent *e )
{
	lastMousePos = -1;
	QSlider::enterEvent( e );
}

int Slider::getMousePos( int X )
{
	const int o = style()->pixelMetric( QStyle::PM_SliderLength ) - 1;
	return QStyle::sliderValueFromPosition( minimum(), maximum(), X - o / 2, width() - o, false );
}
