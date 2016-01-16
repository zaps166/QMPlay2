#include "OpenGL2OldWidget.hpp"

#include <QMPlay2Core.hpp>

OpenGL2OldWidget::OpenGL2OldWidget()
{
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
bool OpenGL2OldWidget::VSync(bool enable)
{
#ifdef VSYNC_SETTINGS
	makeCurrent();
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
	doneCurrent();
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
		doClear = NUM_BUFFERS_TO_CLEAR;
	if (doClear > 0)
	{
		glClear(GL_COLOR_BUFFER_BIT);
		--doClear;
	}
	OpenGL2Common::paintGL();
}

void OpenGL2OldWidget::resetClearCounter()
{
	doClear = NUM_BUFFERS_TO_CLEAR;
}

void OpenGL2OldWidget::resizeGL(int w, int h)
{
	glViewport(0, 0, w, h);
}

bool OpenGL2OldWidget::event(QEvent *e)
{
	dispatchEvent(e, parent());
	return QGLWidget::event(e);
}
