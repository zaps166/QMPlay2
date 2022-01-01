/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2022  Błażej Szczygieł

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

#include "OpenGLWidget.hpp"

#include <QOpenGLContext>

OpenGLWidget::OpenGLWidget()
{
    m_widget = this;
    connect(&updateTimer, SIGNAL(timeout()), this, SLOT(update()));
}
OpenGLWidget::~OpenGLWidget()
{
    makeCurrent();
}

bool OpenGLWidget::makeContextCurrent()
{
    if (!context())
        return false;

    makeCurrent();
    return true;
}
void OpenGLWidget::doneContextCurrent()
{
    doneCurrent();
}

void OpenGLWidget::setVSync(bool enable)
{
    Q_UNUSED(enable)
    // Not supported
}
void OpenGLWidget::updateGL(bool requestDelayed)
{
    if (requestDelayed)
        QMetaObject::invokeMethod(this, "update", Qt::QueuedConnection);
    else
        update();
}

void OpenGLWidget::initializeGL()
{
    connect(context(), SIGNAL(aboutToBeDestroyed()), this, SLOT(aboutToBeDestroyed()), Qt::DirectConnection);
    OpenGLCommon::initializeGL();
}
void OpenGLWidget::paintGL()
{
    OpenGLCommon::paintGL();
}

void OpenGLWidget::aboutToBeDestroyed()
{
    makeCurrent();
    contextAboutToBeDestroyed();
    doneCurrent();
}

bool OpenGLWidget::event(QEvent *e)
{
    dispatchEvent(e, parent());
    return QOpenGLWidget::event(e);
}
