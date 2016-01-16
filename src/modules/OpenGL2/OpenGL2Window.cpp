#include "OpenGL2Window.hpp"

#include <QMPlay2Core.hpp>

OpenGL2Window::OpenGL2Window()
{
	setFlags(Qt::WindowTransparentForInput);

	container = QWidget::createWindowContainer(this);
	container->setAttribute(Qt::WA_NativeWindow);
	container->installEventFilter(this);

	connect(&QMPlay2Core, SIGNAL(videoDockMoved()), this, SLOT(resetClearCounter()));
}
OpenGL2Window::~OpenGL2Window()
{
	setParent(NULL); //Container must not take ownership of this
	delete container;
}

QWidget *OpenGL2Window::widget()
{
	return container;
}

bool OpenGL2Window::VSync(bool enable)
{
	QSurfaceFormat fmt = format();
	if (!handle())
	{
		fmt.setSwapBehavior(QSurfaceFormat::DoubleBuffer); //Probably it doesn't works
		fmt.setSwapInterval(enable);
		setFormat(fmt);
	}
	else if (enable != fmt.swapInterval())
	{
		fmt.setSwapInterval(enable);
		destroy();
		setFormat(fmt);
		create();
		show();
	}
	return true;
}
void OpenGL2Window::updateGL()
{
	//Sent event doesn't enqueue the event
	QEvent updateEvent(QEvent::UpdateRequest);
	QCoreApplication::sendEvent(this, &updateEvent);
}

void OpenGL2Window::initializeGL()
{
	OpenGL2Common::initializeGL();
	doClear = true;
}
void OpenGL2Window::paintGL()
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

void OpenGL2Window::resetClearCounter()
{
	doClear = NUM_BUFFERS_TO_CLEAR;
}

bool OpenGL2Window::eventFilter(QObject *o, QEvent *e)
{
	if (o == container)
		dispatchEvent(e, parent());
	return false;
}
