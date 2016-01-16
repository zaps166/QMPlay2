#ifndef OPENGL2WIDGET_HPP
#define OPENGL2WIDGET_HPP

#include <OpenGL2CommonQt5.hpp>

#include <QSurfaceFormat>
#include <QOpenGLWidget>

class OpenGL2Widget : public QOpenGLWidget, public OpenGL2CommonQt5
{
public:
	QWidget *widget();

	bool VSync(bool enable);
	void updateGL();

	void initializeGL();
	void paintGL();
private:
	bool event(QEvent *e);
};

#endif // OPENGL2WIDGET_HPP
