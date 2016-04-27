#ifndef VOLWIDGET_HPP
#define VOLWIDGET_HPP

#include <Slider.hpp>

#include <QWidget>
#include <QMenu>

class VolWidget : public QWidget
{
	Q_OBJECT
public:
	VolWidget(int max);

	inline int volumeL() const
	{
		return vol[0].value();
	}
	inline int volumeR() const
	{
		return vol[1].value();
	}

	void setVolume(int volL, int volR, bool first = false);
	void changeVolume(int deltaVol);
private slots:
	void customContextMenuRequested(const QPoint &pos);
	void splitTriggered(bool splitted);
	void setMaximumVolume(int max);

	void sliderValueChanged(int v);
signals:
	void volumeChanged(int l, int r);
private:
	void setSlidersToolTip();

	QAction *splitA;
	Slider vol[2];
	QMenu menu;
};

#endif // VOLWIDGET_HPP
