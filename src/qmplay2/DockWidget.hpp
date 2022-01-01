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

#include <QMPlay2Lib.hpp>

#include <QDockWidget>

class QMPLAY2SHAREDLIB_EXPORT DockWidget : public QDockWidget
{
    Q_OBJECT

public:
    DockWidget();
    ~DockWidget();

    void setTitleBarVisible(bool v);
    void setGlobalTitleBarVisible(bool v);

private:
    QWidget *const m_emptyW;
    bool m_titleBarVisible = true;
    bool m_globalTitleBarVisible = true;
};
