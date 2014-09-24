#ifndef VIDEOEQUALIZER_HPP
#define VIDEOEQUALIZER_HPP

#include <QWidget>

class QGridLayout;
class QPushButton;
class QLabel;
class Slider;

class VideoEqualizer : public QWidget
{
	Q_OBJECT
public:
	VideoEqualizer();
signals:
	void valuesChanged( int, int, int, int );
private slots:
	void setValue( int );
	void reset();
private:
	QGridLayout *layout;
	enum CONTROLS { BRIGHTNESS, SATURATION, CONTRAST, HUE, CONTROLS_COUNT };
	struct
	{
		QLabel *titleL, *valueL;
		Slider *slider;
	} controls[ CONTROLS_COUNT ];
	QPushButton *resetB;
};

#endif //VIDEOEQUALIZER_HPP
