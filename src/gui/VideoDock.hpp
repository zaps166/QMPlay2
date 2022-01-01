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

#include <QCommonStyle>
#include <QTimer>

#include <DockWidget.hpp>
#include <InDockW.hpp>

class QMenu;

class VideoDock final : public DockWidget
{
    Q_OBJECT
public:
    VideoDock();

    void fullScreen(bool);
    inline void setLoseHeight(int lh)
    {
        iDW.setLoseHeight(lh);
    }

    inline void updateIDW()
    {
        iDW.update();
    }

    bool isTouch, touchEnded;
private:
    QWidget *internalWidget();

    void unsetCursor(QWidget *w);

    void dragEnterEvent(QDragEnterEvent *) override;
    void dropEvent(QDropEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mouseDoubleClickEvent(QMouseEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void moveEvent(QMoveEvent *) override;
    void wheelEvent(QWheelEvent *) override;
    void leaveEvent(QEvent *) override;
    void enterEvent(QEvent *) override;
    bool event(QEvent *) override;

    QTimer hideCursorTim, leftButtonPlayTim;
    InDockW iDW;
    QMenu *popupMenu;
    QCommonStyle commonStyle;
    int pixels;
    Qt::KeyboardModifiers m_pressedKeyModifiers = Qt::NoModifier;
    bool canPopup, is_floating, canHideIDWCursor, doubleClicked;
    QMargins m_contentMarginsBackup;
    double touchZoom;
    bool m_workaround = false;
private slots:
    void popup(const QPoint &);
    void hideCursor();
    void resizedIDW(int, int);
    void updateImage(const QImage &);
    void visibilityChanged(bool);
    void hasCoverImage(bool);
signals:
    void resized(int, int);
    void itemDropped(const QString &, bool);
};
