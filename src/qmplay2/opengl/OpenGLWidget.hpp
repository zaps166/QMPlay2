// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include "OpenGLCommon.hpp"

#include <QSurfaceFormat>
#include <QOpenGLWidget>

class OpenGLWidget final : public QOpenGLWidget, public OpenGLCommon
{
    Q_OBJECT
public:
    OpenGLWidget();
    ~OpenGLWidget();

    bool makeContextCurrent() override;
    void doneContextCurrent() override;

    void setVSync(bool enable) override;
    void updateGL(bool requestDelayed) override;

    void initializeGL() override;
    void paintGL() override;
private slots:
    void aboutToBeDestroyed();
private:
    bool event(QEvent *e) override;
};
