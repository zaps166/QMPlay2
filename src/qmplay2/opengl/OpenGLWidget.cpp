// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "OpenGLWidget.hpp"

#include <QOpenGLContext>

OpenGLWidget::OpenGLWidget()
{
    if (m_canUse16bitTexture && m_glInstance->glVer >= 30)
    {
        setTextureFormat(GL_RGB10_A2);
    }

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
    glClear(GL_COLOR_BUFFER_BIT);
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
