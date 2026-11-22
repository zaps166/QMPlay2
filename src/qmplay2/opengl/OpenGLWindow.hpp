// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include "OpenGLCommon.hpp"

#include <QOpenGLWindow>

class OpenGLWindow final : private QOpenGLWindow, public OpenGLCommon
{
    Q_OBJECT
public:
    OpenGLWindow();
    ~OpenGLWindow();

    void deleteMe() override;

    bool makeContextCurrent() override;
    void doneContextCurrent() override;

    void setVSync(bool enable) override;
    void updateGL(bool requestDelayed) override;

    void initializeGL() override;
    void paintGL() override;

private slots:
    void doUpdateGL(bool queued = false);
    void aboutToBeDestroyed();
    void videoVisible(bool v);
private:
    bool eventFilter(QObject *o, QEvent *e) override;

    bool event(QEvent *e) override;

private:
    const QString m_platformName;

    const bool m_passEventsToParent;

    bool m_visible = true;
};
