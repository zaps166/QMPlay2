#ifndef DOCKWIDGET_HPP
#define DOCKWIDGET_HPP

#include <QDockWidget>

class DockWidget : public QDockWidget
{
public:
	inline DockWidget() :
		titleBarVisible( true ), globalTitleBarVisible( true )
	{}
	inline void setTitleBarVisible( bool v = true )
	{
		setTitleBarWidget( ( titleBarVisible = v ) && globalTitleBarVisible ? NULL : &emptyW );
	}
	inline void setGlobalTitleBarVisible( bool v )
	{
		globalTitleBarVisible = v;
		setTitleBarVisible( titleBarVisible );
	}
private:
	class EmptyW : public QWidget
	{
		void showEvent( QShowEvent * );

		QSize sizeHint() const;
	} emptyW;
	bool titleBarVisible, globalTitleBarVisible;
};

#endif // DOCKWIDGET_HPP
