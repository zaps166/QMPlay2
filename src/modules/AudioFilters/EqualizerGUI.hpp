#include <QMPlay2Extensions.hpp>

class GraphW : public QWidget
{
public:
	GraphW();

	void setValue(int, float);
	inline void setValues(int vals)
	{
		values.resize(vals);
	}
private:
	void paintEvent(QPaintEvent *);

	QVector<float> values;
	float preamp;
};

/**/

class QGridLayout;

class EqualizerGUI : public QWidget, public QMPlay2Extensions
{
	Q_OBJECT
public:
	EqualizerGUI(Module &);

	DockWidget *getDockWidget();
private slots:
	void wallpaperChanged(bool hasWallpaper, double alpha);
	void enabled(bool);
	void valueChanged(int);
	void setSliders();
private:
	bool set();

	DockWidget *dw;
	GraphW graph;

	QWidget *slidersW;
	QGridLayout *layout;
};

#define EqualizerGUIName "Audio Equalizer Graphical Interface"
