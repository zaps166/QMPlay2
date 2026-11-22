// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

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
