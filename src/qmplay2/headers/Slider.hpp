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
	void setValue(int val);
	inline void setMaximum(int max)
	{
		QAbstractSlider::setMaximum(max);
	}
	inline void setWheelStep(int ws)
	{
		wheelStep = ws;
	}
	void drawRange(int first, int second);
protected:
	void paintEvent(QPaintEvent *);
	void mousePressEvent(QMouseEvent *);
	void mouseReleaseEvent(QMouseEvent *);
	void mouseMoveEvent(QMouseEvent *);
	void wheelEvent(QWheelEvent *);
	void enterEvent(QEvent *);
private:
	int getMousePos(int X);

	bool canSetValue, _ignoreValueChanged;
	int lastMousePos, wheelStep, firstLine, secondLine;
signals:
	void mousePosition(int xPos);
};

#endif
