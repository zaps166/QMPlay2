#include <OpenGL2CommonQt5.hpp>

#include <QOffscreenSurface>
#include <QOpenGLContext>

bool OpenGL2CommonQt5::testGL()
{
	QOpenGLContext glCtx;
	if ((isOK = glCtx.create()))
	{
		QOffscreenSurface offscreenSurface;
		offscreenSurface.create();
		glCtx.makeCurrent(&offscreenSurface);
		testGLInternal();
	}
	return isOK;
}
