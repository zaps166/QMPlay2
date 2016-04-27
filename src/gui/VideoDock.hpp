#include <QCommonStyle>
#include <QTimer>

#include <DockWidget.hpp>
#include <InDockW.hpp>

class QMenu;

class VideoDock : public DockWidget
{
	Q_OBJECT
public:
	VideoDock();

	void fullScreen(bool);
	inline void setLoseHeight(int lh)
	{
		iDW.setLoseHeight(lh);
	}

	inline void updateIDW()
	{
		iDW.update();
	}

	bool isTouch;
private:
	inline QWidget *internalWidget();

	void unsetCursor(QWidget *w);

	void dragEnterEvent(QDragEnterEvent *);
	void dropEvent(QDropEvent *);
	void mouseMoveEvent(QMouseEvent *);
#ifndef Q_OS_MAC
	void mouseDoubleClickEvent(QMouseEvent *);
#endif
	void mousePressEvent(QMouseEvent *);
	void mouseReleaseEvent(QMouseEvent *);
	void moveEvent(QMoveEvent *);
	void wheelEvent(QWheelEvent *);
	void leaveEvent(QEvent *);
	void enterEvent(QEvent *);
	bool event(QEvent *);

	QTimer hideCursorTim;
	InDockW iDW;
	QMenu *popupMenu;
	QCommonStyle commonStyle;
	int pixels;
	bool canPopup, is_floating, isBreeze;
	double touchZoom;
private slots:
	void popup(const QPoint &);
	void hideCursor();
	void resizedIDW(int, int);
	void updateImage(const QImage &);
	void visibilityChanged(bool);
signals:
	void resized(int, int);
	void itemDropped(const QString &, bool);
};
