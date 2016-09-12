/*
	QMPlay2 is a video and audio player.
	Copyright (C) 2010-2017  Błażej Szczygieł

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU Lesser General Public License as published
	by the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "OpenGL2Window.hpp"

#include <QMPlay2Core.hpp>

#include <QOpenGLContext>
#include <QDockWidget>

OpenGL2Window::OpenGL2Window() :
	visible(true)
{
	connect(&updateTimer, SIGNAL(timeout()), this, SLOT(doUpdateGL()));

#ifndef PASS_EVENTS_TO_PARENT
	setFlags(Qt::WindowTransparentForInput);
#endif

	container = QWidget::createWindowContainer(this);
	container->setAttribute(Qt::WA_NativeWindow);
	container->installEventFilter(this);
	container->setAcceptDrops(false);

	connect(&QMPlay2Core, SIGNAL(videoDockVisible(bool)), this, SLOT(videoVisible(bool)));
}
OpenGL2Window::~OpenGL2Window()
{
	makeCurrent();
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
	}
	vSync = enable;
	return true;
}
void OpenGL2Window::updateGL(bool requestDelayed)
{
	if (visible && isExposed())
		QMetaObject::invokeMethod(this, "doUpdateGL", Qt::QueuedConnection, Q_ARG(bool, requestDelayed));
}

void OpenGL2Window::initializeGL()
{
	connect(context(), SIGNAL(aboutToBeDestroyed()), this, SLOT(aboutToBeDestroyed()), Qt::DirectConnection);
	OpenGL2Common::initializeGL();
}
void OpenGL2Window::paintGL()
{
	if (isExposed())
	{
		glClear(GL_COLOR_BUFFER_BIT);
		OpenGL2Common::paintGL();
	}
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
void OpenGL2Window::aboutToBeDestroyed()
{
	makeCurrent();
	contextAboutToBeDestroyed();
	doneCurrent();
}
void OpenGL2Window::videoVisible(bool v)
{
	visible = v && (container->visibleRegion() != QRegion() || QMPlay2Core.getVideoDock()->visibleRegion() != QRegion());
}

bool OpenGL2Window::eventFilter(QObject *o, QEvent *e)
{
	if (o == container)
		dispatchEvent(e, container->parent());
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
