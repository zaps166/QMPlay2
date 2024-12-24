/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2024  Błażej Szczygieł

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

#include <QMPlay2Lib.hpp>

#include <QDockWidget>

class QTimer;

class QMPLAY2SHAREDLIB_EXPORT DockWidget : public QDockWidget
{
    Q_OBJECT

public:
    DockWidget();
    ~DockWidget();

    void setTitleBarVisible(bool v);
    void setGlobalTitleBarVisible(bool v);

    void setResizingByMainWindow(bool r);

protected:
    void resizeEvent(QResizeEvent *e) override;
    void showEvent(QShowEvent *e) override;
    void closeEvent(QCloseEvent *e) override;

signals:
    void dockVisibilityChanged(bool visible);
    void shouldStoreSizes();

private:
    QWidget *const m_emptyW;
    QTimer *const m_visibilityTimer;
    bool m_titleBarVisible = true;
    bool m_globalTitleBarVisible = true;
    bool m_visible = false;
    bool m_closed = true;
    bool m_resizingByMainWindow = false;
    int m_lastVisible = -1;
};
