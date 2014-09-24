#include <Slider.hpp>

#include <QStyleOptionSlider>
#include <QMouseEvent>
#include <QPainter>
#include <QStyle>

Slider::Slider() :
	QSlider( Qt::Horizontal )
{
	wheelStep = 5;
	_drawLineTo = 0;
	canSetValue = true;
	_ignoreValueChanged = false;
	setMouseTracking( true );
}

void Slider::setValue( int v )
{
	if ( canSetValue )
	{
		_ignoreValueChanged = true;
		QSlider::setValue( v );
		_ignoreValueChanged = false;
	}
}

void Slider::drawLineTo( int i )
{
	_drawLineTo = i;
	repaint();
}

void Slider::paintEvent( QPaintEvent *e )
{
	QSlider::paintEvent( e );
	if ( _drawLineTo > 0 && maximum() )
	{
		if ( _drawLineTo + value() > maximum() )
			_drawLineTo = maximum() - value();

		QPainter p( this );

		QStyleOptionSlider opt;
		initStyleOption( &opt );

		const int o = style()->pixelMetric( QStyle::PM_SliderLength ) - 1;
		const int X = QStyle::sliderPositionFromValue( minimum(), maximum(), value()+_drawLineTo, width()-o, false ) + o/2;
		QRect handle = style()->subControlRect( QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, this );

		p.drawLine( X, handle.y(), X, handle.height() );
	}
}
void Slider::mousePressEvent( QMouseEvent *e )
{
	canSetValue = false;
	int v = value();
	if ( e->buttons() == Qt::LeftButton )
	{
		QMouseEvent ev( e->type(), e->pos(), Qt::MidButton, Qt::MidButton, e->modifiers() );
		QSlider::mousePressEvent( &ev );
	}
	else
		QSlider::mousePressEvent( e );
	if ( v != value() )
		_drawLineTo = 0;
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
	return QStyle::sliderValueFromPosition( minimum(), maximum(), X - o/2, width() - o, false );
}
