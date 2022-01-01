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

#include <PanGestureEventFilter.hpp>

#include <QAbstractItemView>
#include <QCoreApplication>
#include <QTouchDevice>
#include <QScrollBar>
#include <QScroller>
#include <qevent.h>

class PanGestureEventFilterPriv : public QObject
{
    bool eventFilter(QObject *watched, QEvent *event) override final
    {
        switch (event->type())
        {
            case QEvent::Show:
            {
                QAbstractScrollArea *scrollArea = qobject_cast<QAbstractScrollArea *>(watched);
                if (scrollArea && !QScroller::hasScroller(scrollArea))
                {
                    if (QAbstractItemView *itemView = qobject_cast<QAbstractItemView *>(watched))
                    {
                        itemView->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
                        itemView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
                    }
                    QScroller::grabGesture(scrollArea->viewport(), m_gestureType);
                }
                break;
            }
            case QEvent::MouseButtonPress:
            case QEvent::MouseButtonRelease:
            case QEvent::MouseMove:
            {
                QMouseEvent *me = static_cast<QMouseEvent *>(event);
                const bool hasScroller = QScroller::hasScroller(watched);
                if (!hasScroller && (event->type() != QEvent::MouseMove || me->buttons() != Qt::NoButton))
                {
                    // Stop scrolling when using scroll bar
                    if (QScrollBar *scrollBar = qobject_cast<QScrollBar *>(watched))
                    {
                        if (QWidget *parent = scrollBar->parentWidget())
                        {
                            if (QAbstractScrollArea *scrollArea = qobject_cast<QAbstractScrollArea *>(parent->parentWidget()))
                            {
                                QWidget *viewport = scrollArea->viewport();
                                if (QScroller::hasScroller(viewport))
                                    QScroller::scroller(viewport)->stop();
                            }
                        }
                    }
                }
                break;
            }
            default:
                break;
        }
        return QObject::eventFilter(watched, event);
    }

#ifdef Q_OS_ANDROID
    const QScroller::ScrollerGestureType m_gestureType = QScroller::LeftMouseButtonGesture;
#else
    const QScroller::ScrollerGestureType m_gestureType = QScroller::TouchGesture;
#endif
};

/**/

void PanGestureEventFilter::install()
{
    static bool installed;

#ifndef Q_OS_ANDROID
    return; // TODO
#endif

    if (installed)
        return;

    if (QTouchDevice::devices().isEmpty())
        return;

    QScrollerProperties defaultScrollerProps;

    // Disable Overshoot, QScrollerProperties::OvershootAlwaysOff can cause jumps in scrolling
    defaultScrollerProps.setScrollMetric(QScrollerProperties::OvershootDragResistanceFactor, 0.0);
    defaultScrollerProps.setScrollMetric(QScrollerProperties::OvershootDragDistanceFactor, 0.0);
    defaultScrollerProps.setScrollMetric(QScrollerProperties::OvershootScrollTime, 0);

    defaultScrollerProps.setScrollMetric(QScrollerProperties::DragStartDistance, 0.0015);
    defaultScrollerProps.setScrollMetric(QScrollerProperties::DecelerationFactor, 0.25);

    QScrollerProperties::setDefaultScrollerProperties(defaultScrollerProps);

    qApp->installEventFilter(new PanGestureEventFilterPriv);

    installed = true;
}
