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

#include <DockWidget.hpp>

class EmptyW final : public QWidget
{
    QSize sizeHint() const override
    {
        return QSize(0, 0);
    }
};

/**/

DockWidget::DockWidget()
    : m_emptyW(new EmptyW)
{}
DockWidget::~DockWidget()
{
    delete m_emptyW;
}

void DockWidget::setTitleBarVisible(bool v)
{
    m_titleBarVisible = v;
    setTitleBarWidget((m_titleBarVisible && m_globalTitleBarVisible) ? nullptr : m_emptyW);
}
void DockWidget::setGlobalTitleBarVisible(bool v)
{
    m_globalTitleBarVisible = v;
    setTitleBarVisible(m_titleBarVisible);
}
