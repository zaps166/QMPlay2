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

#include <EventFilterWorkarounds.hpp>

#include <QCoreApplication>
#include <qevent.h>
#include <QWindow>

class FloorMouseEvent : public QMouseEvent
{
public:
    inline void maybeFloorValues()
    {
        if (l == w)
        {
            l = QPoint(l.x(), l.y());
            w = QPoint(w.x(), w.y());
            s = QPoint(s.x(), s.y());
        }
    }
};
class FloorEnterEvent : public QEnterEvent
{
public:
    inline void maybeFloorValues()
    {
        if (l == w)
        {
            l = QPoint(l.x(), l.y());
            w = QPoint(w.x(), w.y());
            s = QPoint(s.x(), s.y());
        }
    }
};

/**/

EventFilterWorkarounds::EventFilterWorkarounds(QObject *parent)
    : QObject(parent)
{}
EventFilterWorkarounds::~EventFilterWorkarounds()
{}

bool EventFilterWorkarounds::eventFilter(QObject *watched, QEvent *event)
{
    if (watched->isWindowType())
    {
        QWindow *win = (QWindow *)watched;
        if (win->devicePixelRatio() > 1.0)
        {
            switch (event->type())
            {
                case QEvent::MouseButtonPress:
                case QEvent::MouseButtonRelease:
                case QEvent::MouseButtonDblClick:
                case QEvent::MouseMove:
                    static_cast<FloorMouseEvent *>(event)->maybeFloorValues();
                    break;
                case QEvent::Enter:
                    static_cast<FloorEnterEvent *>(event)->maybeFloorValues();
                    break;
                default:
                    break;
            }
        }
    }
    return QObject::eventFilter(watched, event);
}
