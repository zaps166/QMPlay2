#ifndef OPENGL2WIDGET_HPP
#define OPENGL2WIDGET_HPP

#include <OpenGL2CommonQt5.hpp>

#include <QSurfaceFormat>
#include <QOpenGLWidget>

class OpenGL2Widget : public QOpenGLWidget, public OpenGL2CommonQt5
{
public:
	OpenGL2Widget();

	QWidget *widget();

	bool setVSync(bool enable);
	void updateGL(bool requestDelayed);

	void initializeGL();
	void paintGL();
private:
	bool event(QEvent *e);
};

#endif // OPENGL2WIDGET_HPP
