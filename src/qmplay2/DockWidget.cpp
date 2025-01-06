/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2025  Błażej Szczygieł

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

#include <qevent.h>
#include <QTimer>

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
    , m_visibilityTimer(new QTimer(this))
{
    m_visibilityTimer->setSingleShot(true);
    m_visibilityTimer->setInterval(0);
    connect(m_visibilityTimer, &QTimer::timeout, this, [this] {
        if (m_lastVisible != m_visible)
        {
            emit dockVisibilityChanged(m_visible);
            m_lastVisible = m_visible;
        }
    });

    connect(this, &QDockWidget::visibilityChanged, this, [this](bool visible) {
        m_visible = visible;
        if (!m_visibilityTimer->isActive())
            m_visibilityTimer->start();
    });

    connect(this, &QDockWidget::dockLocationChanged, this, [this](Qt::DockWidgetArea area) {
        Q_UNUSED(area)
        emit shouldStoreSizes();
    });
}
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

void DockWidget::setResizingByMainWindow(bool r)
{
    m_resizingByMainWindow = true;
}

void DockWidget::resizeEvent(QResizeEvent *e)
{
    QDockWidget::resizeEvent(e);
    if (m_resizingByMainWindow)
    {
        m_resizingByMainWindow = false;
    }
    else if (!isFloating())
    {
        emit shouldStoreSizes();
    }
}
void DockWidget::showEvent(QShowEvent *e)
{
    QDockWidget::showEvent(e);
    if (m_closed)
    {
        if (!isFloating())
        {
            emit shouldStoreSizes();
        }
        m_closed = false;
    }
}
void DockWidget::closeEvent(QCloseEvent *e)
{
    QDockWidget::closeEvent(e);
    if (!isFloating())
    {
        emit shouldStoreSizes();
    }
    m_closed = true;
}
