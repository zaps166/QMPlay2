#include "OpenGL2Widget.hpp"

QWidget *OpenGL2Widget::widget()
{
	return this;
}

bool OpenGL2Widget::setVSync(bool enable)
{
	QSurfaceFormat fmt = format();
	vSync = enable;
	if (!isValid())
	{
		fmt.setSwapBehavior(QSurfaceFormat::DoubleBuffer); //Probably it doesn't work
		fmt.setSwapInterval(enable); //Does it work on QOpenGLWidget?
		setFormat(fmt);
		return true;
	}
	return (fmt.swapInterval() == enable);
}
void OpenGL2Widget::updateGL(bool requestDelayed)
{
	if (requestDelayed)
		QMetaObject::invokeMethod(this, "update", Qt::QueuedConnection);
	else
		update();
}

void OpenGL2Widget::initializeGL()
{
	OpenGL2Common::initializeGL();
}
void OpenGL2Widget::paintGL()
{
	OpenGL2Common::paintGL();
}

bool OpenGL2Widget::event(QEvent *e)
{
	dispatchEvent(e, parent());
	return QOpenGLWidget::event(e);
}
