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

#include <QAbstractNativeEventFilter>
#include <QAbstractItemView>
#include <QApplication>
#include <QTouchDevice>
#include <QScrollBar>
#include <QLibrary>
#include <QPointer>
#include <QEvent>
#include <QTimer>

#ifdef WINVER
    #undef WINVER
#endif
#define WINVER 0x0601
#include <windows.h>
#ifndef WM_GESTURE
    #define WM_GESTURE 0x0119
#endif
#ifndef WM_GESTURENOTIFY
    #define WM_GESTURENOTIFY 0x011A
#endif

namespace PanGestureEventFilter {

namespace User32 {
    using UnregisterTouchWindowFunc = BOOL (WINAPI *)(HWND hwnd);
    static UnregisterTouchWindowFunc UnregisterTouchWindow;

    using SetGestureConfigFunc = BOOL (WINAPI *)(HWND hwnd, DWORD dwReserved, UINT cIDs, PGESTURECONFIG pGestureConfig, UINT cbSize);
    static SetGestureConfigFunc SetGestureConfig;

    using GetGestureInfoFunc = BOOL (WINAPI *)(HGESTUREINFO hGestureInfo, PGESTUREINFO pGestureInfo);
    static GetGestureInfoFunc GetGestureInfo;

    using CloseGestureInfoHandleFunc = BOOL (WINAPI *)(HGESTUREINFO hGestureInfo);
    static CloseGestureInfoHandleFunc CloseGestureInfoHandle;
}

namespace UxTheme {
    using BeginPanningFeedbackFunc = BOOL (WINAPI *)(HWND hwnd);
    static BeginPanningFeedbackFunc BeginPanningFeedback;

    using UpdatePanningFeedbackFunc = BOOL (WINAPI *)(HWND hwnd, LONG lTotalOverpanOffsetX, LONG lTotalOverpanOffsetY, BOOL fInInertia);
    static UpdatePanningFeedbackFunc UpdatePanningFeedback;

    using EndPanningFeedbackFunc = BOOL (WINAPI *)(HWND hwnd, BOOL fAnimateBack);
    static EndPanningFeedbackFunc EndPanningFeedback;
}

class PanGestureEventFilterPriv final : public QObject, public QAbstractNativeEventFilter
{
    bool eventFilter(QObject *watched, QEvent *event)
    {
        if (event->type() == QEvent::Show)
        {
            QAbstractScrollArea *scrollArea = qobject_cast<QAbstractScrollArea *>(watched);
            if (scrollArea && !m_installedGestures.contains(scrollArea))
            {
                QTimer *timer = new QTimer(scrollArea);
                timer->setSingleShot(true);
                timer->setInterval(10);

                connect(timer, &QTimer::timeout, scrollArea, [=] {
                    if (HWND winId = (HWND)scrollArea->viewport()->winId())
                    {
                        GESTURECONFIG gc = {
                            GID_PAN,
                            0,
                            0,
                        };

                        if (scrollArea->horizontalScrollBar()->maximum() > 0)
                            gc.dwWant |= GC_PAN_WITH_SINGLE_FINGER_HORIZONTALLY;
                        else
                            gc.dwBlock |= GC_PAN_WITH_SINGLE_FINGER_HORIZONTALLY;

                        if (scrollArea->verticalScrollBar()->maximum() > 0)
                            gc.dwWant |= GC_PAN_WITH_SINGLE_FINGER_VERTICALLY;
                        else
                            gc.dwBlock |= GC_PAN_WITH_SINGLE_FINGER_VERTICALLY;

                        User32::UnregisterTouchWindow(winId);
                        if (User32::SetGestureConfig(winId, 0, 1, &gc, sizeof gc))
                        {
                            if (QAbstractItemView *itemView = qobject_cast<QAbstractItemView *>(scrollArea))
                            {
                                itemView->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
                                itemView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
                            }
                        }
                    }
                });

                connect(scrollArea->horizontalScrollBar(), &QScrollBar::rangeChanged, scrollArea, [=] {
                    timer->start();
                });
                connect(scrollArea->verticalScrollBar(), &QScrollBar::rangeChanged, scrollArea, [=] {
                    timer->start();
                });
                timer->start();

                m_installedGestures.insert(scrollArea);
                connect(scrollArea, &QAbstractScrollArea::destroyed, this, [=] {
                    m_installedGestures.remove(scrollArea);
                });
            }
        }
        return QObject::eventFilter(watched, event);
    }

    bool nativeEventFilter(const QByteArray &eventType, void *message, long *result)
    {
        if (eventType == "windows_generic_MSG")
        {
            const MSG *winMsg = (const MSG *)message;
            if (winMsg->message == WM_GESTURENOTIFY)
            {
                return true;
            }
            else if (winMsg->message == WM_GESTURE)
            {
                GESTUREINFO gi;
                memset(&gi, 0, sizeof gi);
                gi.cbSize = sizeof gi;
                if (User32::GetGestureInfo((HGESTUREINFO)winMsg->lParam, &gi))
                {
                    const QPoint pos(gi.ptsLocation.x, gi.ptsLocation.y);
                    if (gi.dwID == GID_BEGIN)
                    {
                        if (QWidget *widget = QWidget::find((WId)winMsg->hwnd))
                        {
                            m_scrollArea = qobject_cast<QAbstractScrollArea *>(widget->parentWidget());
                            m_lastPos = pos;
                            if (UxTheme::BeginPanningFeedback)
                            {
                                UxTheme::BeginPanningFeedback(winMsg->hwnd);
                                m_xOverPan = m_yOverPan = 0;
                            }
                        }
                    }
                    else if (gi.dwID == GID_END)
                    {
                        m_scrollArea.clear();
                        m_lastPos = QPoint();
                        if (UxTheme::EndPanningFeedback)
                        {
                            UxTheme::EndPanningFeedback(winMsg->hwnd, true);
                            m_xOverPan = m_yOverPan = 0;
                        }
                    }
                    else if (m_scrollArea)
                    {
                        const QPoint diff = m_lastPos - pos;

                        QScrollBar *verticalScrollBar = m_scrollArea->verticalScrollBar();
                        QScrollBar *horizontalScrollBar = m_scrollArea->horizontalScrollBar();

                        if (m_xOverPan == 0)
                            horizontalScrollBar->setValue(horizontalScrollBar->value() + diff.x());
                        if (m_yOverPan == 0)
                            verticalScrollBar->setValue(verticalScrollBar->value() + diff.y());

                        if (UxTheme::UpdatePanningFeedback)
                        {
                            const auto calcOverPan = [](const QScrollBar *scrollBar, int &overPan, const int diff) {
                                if (scrollBar->minimum() != scrollBar->maximum() && (scrollBar->value() == scrollBar->minimum() || scrollBar->value() == scrollBar->maximum()))
                                {
                                    const bool isPositive = (overPan > 0);
                                    const bool isNegative = (overPan < 0);
                                    overPan -= diff;
                                    if ((isPositive && overPan < 0) || (isNegative && overPan > 0))
                                        overPan = 0;
                                }
                                else
                                {
                                    overPan = 0;
                                }
                            };
                            if (horizontalScrollBar->minimum() != horizontalScrollBar->maximum())
                                calcOverPan(horizontalScrollBar, m_xOverPan, diff.x());
                            if (verticalScrollBar->minimum() != verticalScrollBar->maximum())
                                calcOverPan(verticalScrollBar, m_yOverPan, diff.y());
                            UxTheme::UpdatePanningFeedback(winMsg->hwnd, m_xOverPan, m_yOverPan, gi.dwFlags & GF_INERTIA);
                        }

                        m_lastPos = pos;
                    }
                    User32::CloseGestureInfoHandle((HGESTUREINFO)winMsg->lParam);
                }
                return true;
            }
        }
        return false;
    }

    QSet<QAbstractScrollArea *> m_installedGestures;
    QPointer<QAbstractScrollArea> m_scrollArea;
    int m_xOverPan = 0, m_yOverPan = 0;
    QPoint m_lastPos;
};

/**/

void install()
{
    static bool installed;

    if (QTouchDevice::devices().isEmpty())
        return;

    if (installed)
        return;

    QLibrary user32("user32");
    if (user32.load())
    {
        User32::UnregisterTouchWindow = (User32::UnregisterTouchWindowFunc)user32.resolve("UnregisterTouchWindow");
        User32::SetGestureConfig = (User32::SetGestureConfigFunc)user32.resolve("SetGestureConfig");
        User32::GetGestureInfo = (User32::GetGestureInfoFunc)user32.resolve("GetGestureInfo");
        User32::CloseGestureInfoHandle = (User32::CloseGestureInfoHandleFunc)user32.resolve("CloseGestureInfoHandle");
        if (User32::UnregisterTouchWindow && User32::SetGestureConfig && User32::GetGestureInfo && User32::CloseGestureInfoHandle)
        {
            QLibrary uxtheme("uxtheme");
            if (uxtheme.load())
            {
                UxTheme::BeginPanningFeedback = (UxTheme::BeginPanningFeedbackFunc)uxtheme.resolve("BeginPanningFeedback");
                UxTheme::UpdatePanningFeedback = (UxTheme::UpdatePanningFeedbackFunc)uxtheme.resolve("UpdatePanningFeedback");
                UxTheme::EndPanningFeedback = (UxTheme::EndPanningFeedbackFunc)uxtheme.resolve("EndPanningFeedback");
            }
            PanGestureEventFilterPriv *panGestureEventFilter = new PanGestureEventFilterPriv;
            qApp->installNativeEventFilter(panGestureEventFilter);
            qApp->installEventFilter(panGestureEventFilter);
            installed = true;
        }
    }
}

}
