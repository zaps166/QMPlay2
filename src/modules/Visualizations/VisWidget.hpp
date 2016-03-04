#ifndef VISWIDGET_HPP
#define VISWIDGET_HPP

#include <QWidget>
#include <QTimer>

class DockWidget;

class VisWidget : public QWidget
{
	Q_OBJECT
protected:
	VisWidget();

	static void setValue(qreal &out, qreal in, qreal tDiffScaled);
	static void setValue(QPair< qreal, double > &out, qreal in, qreal tDiffScaled);

	bool regionIsVisible() const;

	virtual void start(bool v, bool dontCheckRegion) = 0;
	virtual void stop() = 0;

	QTimer tim;
	bool stopped;
	DockWidget *dw;
	double time;
private:
	void changeEvent(QEvent *);
	void mouseDoubleClickEvent(QMouseEvent *);
private slots:
	void wallpaperChanged(bool hasWallpaper, double alpha);
	void contextMenu(const QPoint &point);
	void visibilityChanged(bool v);
	void showSettings();
signals:
	void doubleClicked();
};

#endif
