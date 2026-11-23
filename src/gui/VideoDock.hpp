// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <GUIHeaders.hpp>

#include <DockWidget.hpp>
#include <InDockW.hpp>

class QMenu;

class VideoDock final : public DockWidget
{
    Q_OBJECT
public:
    VideoDock();

    void scheduleEnterEventWorkaround();
    void maybeTriggerWidgetVisibility();

    void fullScreen(bool);
    inline void setLoseHeight(int lh)
    {
        iDW.setLoseHeight(lh);
    }

    inline void updateIDW()
    {
        iDW.updateSettings();
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
    void enterEvent(Q_ENTER_EVENT *) override;
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
private slots:
    void popup(const QPoint &);
    void hideCursor();
    void updateImage(const QImage &);
    void visibilityChanged(bool);
    void hasCoverImage(bool);
signals:
    void resized(const QSize &size);
    void itemDropped(const QString &, bool);
};
