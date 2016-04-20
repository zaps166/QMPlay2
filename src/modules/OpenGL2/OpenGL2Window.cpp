#include "OpenGL2Window.hpp"

#include <QMPlay2Core.hpp>

OpenGL2Window::OpenGL2Window()
{
#ifndef PASS_EVENTS_TO_PARENT
	setFlags(Qt::WindowTransparentForInput);
#endif
#ifdef Q_OS_WIN
	preventFullscreen = true;
#endif

	container = QWidget::createWindowContainer(this);
	container->setAttribute(Qt::WA_NativeWindow);
	container->installEventFilter(this);
	container->setAcceptDrops(false);

	connect(&QMPlay2Core, SIGNAL(videoDockMoved()), this, SLOT(resetClearCounter()));
}
void OpenGL2Window::deleteMe()
{
	delete container;
}

QWidget *OpenGL2Window::widget()
{
	return container;
}

bool OpenGL2Window::setVSync(bool enable)
{
	QSurfaceFormat fmt = format();
	if (!handle())
	{
		fmt.setSwapBehavior(QSurfaceFormat::DoubleBuffer); //Probably it doesn't work
		fmt.setSwapInterval(enable);
		setFormat(fmt);
	}
	else if (enable != fmt.swapInterval())
	{
		fmt.setSwapInterval(enable);
		destroy();
		setFormat(fmt);
		create();
		setVisible(true);
		OpenGL2Common::resetClearCounter();
	}
	vSync = enable;
	return true;
}
void OpenGL2Window::updateGL(bool requestDelayed)
{
	QMetaObject::invokeMethod(this, "doUpdateGL", Qt::QueuedConnection, Q_ARG(bool, requestDelayed));
}

void OpenGL2Window::initializeGL()
{
	OpenGL2Common::initializeGL();
}
void OpenGL2Window::paintGL()
{
	if (doReset)
		OpenGL2Common::resetClearCounter();
	if (doClear > 0)
	{
		glClear(GL_COLOR_BUFFER_BIT);
		--doClear;
	}
	OpenGL2Common::paintGL();
}

void OpenGL2Window::resetClearCounter()
{
	OpenGL2Common::resetClearCounter();
}
void OpenGL2Window::doUpdateGL(bool queued)
{
	if (queued)
		QCoreApplication::postEvent(this, new QEvent(QEvent::UpdateRequest), Qt::LowEventPriority);
	else
	{
		//sendEvent() doesn't enqueue the event here
		QEvent updateEvent(QEvent::UpdateRequest);
		QCoreApplication::sendEvent(this, &updateEvent);
	}
}

bool OpenGL2Window::eventFilter(QObject *o, QEvent *e)
{
	if (o == container)
	{
		if (e->type() == QEvent::Paint)
			OpenGL2Common::resetClearCounter();
		else
			dispatchEvent(e, container->parent());
	}
	return false;
}

#ifdef PASS_EVENTS_TO_PARENT
bool OpenGL2Window::event(QEvent *e)
{
	switch (e->type())
	{
		case QEvent::MouseButtonPress:
		case QEvent::MouseButtonRelease:
		case QEvent::MouseButtonDblClick:
		case QEvent::MouseMove:
		case QEvent::FocusIn:
		case QEvent::FocusOut:
		case QEvent::FocusAboutToChange:
		case QEvent::Enter:
		case QEvent::Leave:
		case QEvent::Wheel:
		case QEvent::TabletMove:
		case QEvent::TabletPress:
		case QEvent::TabletRelease:
		case QEvent::TabletEnterProximity:
		case QEvent::TabletLeaveProximity:
		case QEvent::TouchBegin:
		case QEvent::TouchUpdate:
		case QEvent::TouchEnd:
		case QEvent::InputMethodQuery:
		case QEvent::TouchCancel:
			return QCoreApplication::sendEvent(parent(), e);
		default:
			break;
	}
	return QOpenGLWindow::event(e);
}
#endif
