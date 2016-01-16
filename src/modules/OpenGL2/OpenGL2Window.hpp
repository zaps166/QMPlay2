#ifndef OPENGLWIDGET_HPP
#define OPENGLWIDGET_HPP

#include <OpenGL2CommonQt5.hpp>

#include <QOpenGLWindow>
#include <QWidget>

class OpenGL2Window : private QOpenGLWindow, public OpenGL2CommonQt5
{
	Q_OBJECT
public:
	OpenGL2Window();
	~OpenGL2Window();

	QWidget *widget();

	bool VSync(bool enable);
	void updateGL();

	void initializeGL();
	void paintGL();
private slots:
	void resetClearCounter();
private:
	bool eventFilter(QObject *o, QEvent *e);

	QWidget *container;
	int doClear;
};

#endif // OPENGLWIDGET_HPP
