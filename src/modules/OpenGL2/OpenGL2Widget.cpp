/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2019  Błażej Szczygieł

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

#include <QOpenGLContext>

OpenGL2Widget::OpenGL2Widget()
{
    connect(&updateTimer, SIGNAL(timeout()), this, SLOT(update()));
}
OpenGL2Widget::~OpenGL2Widget()
{
    makeCurrent();
}

QWidget *OpenGL2Widget::widget()
{
    return this;
}

void OpenGL2Widget::setVSync(bool enable)
{
    Q_UNUSED(enable)
    // Not supported
}
void OpenGL2Widget::updateGL()
{
    update();
}

void OpenGL2Widget::initializeGL()
{
    connect(context(), SIGNAL(aboutToBeDestroyed()), this, SLOT(aboutToBeDestroyed()), Qt::DirectConnection);
    OpenGL2Common::initializeGL();
}
void OpenGL2Widget::paintGL()
{
    OpenGL2Common::paintGL();
}

void OpenGL2Widget::aboutToBeDestroyed()
{
    makeCurrent();
    contextAboutToBeDestroyed();
    doneCurrent();
}

bool OpenGL2Widget::event(QEvent *e)
{
    dispatchEvent(e, parent());
    return QOpenGLWidget::event(e);
}
