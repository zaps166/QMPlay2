#include "OpenGL2OldWidget.hpp"

#include <QMPlay2Core.hpp>

OpenGL2OldWidget::OpenGL2OldWidget()
{
#ifdef Q_OS_WIN
	preventFullscreen = true;
#endif
	connect(&QMPlay2Core, SIGNAL(videoDockMoved()), this, SLOT(resetClearCounter()));
}

QWidget *OpenGL2OldWidget::widget()
{
	return this;
}

bool OpenGL2OldWidget::testGL()
{
	makeCurrent();
	if ((isOK = isValid()))
		testGLInternal();
	doneCurrent();
	return isOK;
}
bool OpenGL2OldWidget::setVSync(bool enable)
{
#ifdef VSYNC_SETTINGS
	bool doDoneCurrent = false;
	if (QGLContext::currentContext() != context())
	{
		makeCurrent();
		doDoneCurrent = true;
	}
	typedef int (APIENTRY *SwapInterval)(int); //BOOL is just normal int in Windows, APIENTRY declares nothing on non-Windows platforms
	SwapInterval swapInterval = NULL;
#ifdef Q_OS_WIN
	swapInterval = (SwapInterval)context()->getProcAddress("wglSwapIntervalEXT");
#else
	swapInterval = (SwapInterval)context()->getProcAddress("glXSwapIntervalMESA");
	if (!swapInterval)
		swapInterval = (SwapInterval)context()->getProcAddress("glXSwapIntervalSGI");
#endif
	if (swapInterval)
		swapInterval(enable);
	if (doDoneCurrent)
		doneCurrent();
	vSync = enable;
#else
	Q_UNUSED(enable)
#endif
	return true;
}
void OpenGL2OldWidget::updateGL()
{
	QGLWidget::updateGL();
}

void OpenGL2OldWidget::initializeGL()
{
	OpenGL2Common::initializeGL();
}
void OpenGL2OldWidget::paintGL()
{
	if (doReset)
		resetClearCounter();
	if (doClear > 0)
	{
		glClear(GL_COLOR_BUFFER_BIT);
		--doClear;
	}
	OpenGL2Common::paintGL();
}

void OpenGL2OldWidget::resetClearCounter()
{
	OpenGL2Common::resetClearCounter();
}

void OpenGL2OldWidget::resizeGL(int w, int h)
{
	glViewport(0, 0, w, h);
}

bool OpenGL2OldWidget::event(QEvent *e)
{
	if (e->type() == QEvent::Paint)
		resetClearCounter();
	else
		dispatchEvent(e, parent());
	return QGLWidget::event(e);
}
