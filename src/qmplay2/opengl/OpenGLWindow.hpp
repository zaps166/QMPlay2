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

#pragma once

#include "OpenGLCommon.hpp"

#include <QOpenGLWindow>

#if defined Q_OS_MACOS || defined Q_OS_WIN //QTBUG-50505
    #define PASS_EVENTS_TO_PARENT
#endif

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

#ifdef PASS_EVENTS_TO_PARENT
    bool event(QEvent *e) override;
#endif

    bool visible;
};
