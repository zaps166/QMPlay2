/*
	QMPlay2 is a video and audio player.
	Copyright (C) 2010-2016  Błażej Szczygieł

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

#include "OpenGL2Widget.hpp"

OpenGL2Widget::OpenGL2Widget()
{
	connect(&updateTimer, SIGNAL(timeout()), this, SLOT(update()));
}

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
