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

#pragma once

#include <OpenGL2Common.hpp>

#include <QOpenGLWindow>
#include <QWidget>

#if defined Q_OS_MACOS || defined Q_OS_WIN //QTBUG-50505
    #define PASS_EVENTS_TO_PARENT
#endif

class OpenGL2Window final : private QOpenGLWindow, public OpenGL2Common
{
    Q_OBJECT
public:
    OpenGL2Window();
    ~OpenGL2Window();

    void deleteMe() override;

    QWidget *widget() override;

    bool setVSync(bool enable) override;
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

    QWidget *container;
    bool visible;
};
