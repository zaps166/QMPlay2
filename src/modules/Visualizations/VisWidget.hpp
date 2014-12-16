#ifndef VISWIDGET_HPP
#define VISWIDGET_HPP

#include <QWidget>
#include <QTimer>
#include <QTime>

class DockWidget;

class VisWidget : public QWidget
{
	Q_OBJECT
protected:
	VisWidget();

	virtual void start( bool v = false ) = 0;
	virtual void stop() = 0;

	QTimer tim;
	bool stopped;
	DockWidget *dw;
	QTime timInterval;
private:
	void changeEvent( QEvent * );
	void mouseDoubleClickEvent( QMouseEvent * );
private slots:
	void wallpaperChanged( bool hasWallpaper, double alpha );
	void contextMenu( const QPoint &point );
	void visibilityChanged( bool v );
	void showSettings();
signals:
	void doubleClicked();
};

#endif
