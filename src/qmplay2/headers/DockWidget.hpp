#ifndef DOCKWIDGET_HPP
#define DOCKWIDGET_HPP

#include <QDockWidget>

class DockWidget : public QDockWidget
{
public:
	inline DockWidget() :
		titleBarVisible(true), globalTitleBarVisible(true)
	{}
	inline void setTitleBarVisible(bool v = true)
	{
		setTitleBarWidget((titleBarVisible = v) && globalTitleBarVisible ? NULL : &emptyW);
	}
	inline void setGlobalTitleBarVisible(bool v)
	{
		globalTitleBarVisible = v;
		setTitleBarVisible(titleBarVisible);
	}
private:
	class EmptyW : public QWidget
	{
#if QT_VERSION >= 0x050000 && QT_VERSION < 0x050600
		void showEvent(QShowEvent *);
#endif

		QSize sizeHint() const;
	} emptyW;
	bool titleBarVisible, globalTitleBarVisible;
};

#endif // DOCKWIDGET_HPP
