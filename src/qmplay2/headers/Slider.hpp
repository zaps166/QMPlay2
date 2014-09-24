#ifndef SLIDER_HPP
#define SLIDER_HPP

#include <QSlider>

class Slider : public QSlider
{
	Q_OBJECT
public:
	Slider();

	inline bool ignoreValueChanged() const
	{
		return _ignoreValueChanged;
	}
public slots:
	void setValue( int );
	inline void setMaximum( int m )
	{
		QSlider::setMaximum( m );
	}
	inline void setWheelStep( int ws )
	{
		wheelStep = ws;
	}
	void drawLineTo( int );
protected:
	void paintEvent( QPaintEvent * );
	void mousePressEvent( QMouseEvent * );
	void mouseReleaseEvent( QMouseEvent * );
	void mouseMoveEvent( QMouseEvent * );
	void wheelEvent( QWheelEvent * );
	void enterEvent( QEvent * );
private:
	int getMousePos( int );

	bool canSetValue, _ignoreValueChanged;
	int lastMousePos, wheelStep, _drawLineTo;
signals:
	void mousePosition( int );
};

#endif
