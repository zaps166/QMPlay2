#ifndef OPENGLWIDGET_HPP
#define OPENGLWIDGET_HPP

#include <OpenGL2CommonQt5.hpp>

#include <QOpenGLWindow>
#include <QWidget>

#if defined Q_OS_MAC || defined Q_OS_WIN //QTBUG-50505
	#define PASS_EVENTS_TO_PARENT
#endif

class OpenGL2Window : private QOpenGLWindow, public OpenGL2CommonQt5
{
	Q_OBJECT
public:
	OpenGL2Window();

	void deleteMe();

	QWidget *widget();

	bool setVSync(bool enable);
	void updateGL(bool requestDelayed);

	void initializeGL();
	void paintGL();

private slots:
	void resetClearCounter();
	void doUpdateGL(bool queued);
private:
	bool eventFilter(QObject *o, QEvent *e);

#ifdef PASS_EVENTS_TO_PARENT
	bool event(QEvent *e);
#endif

	QWidget *container;
};

#endif // OPENGLWIDGET_HPP
