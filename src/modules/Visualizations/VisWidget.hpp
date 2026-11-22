// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QWidget>
#include <QTimer>

#ifdef USE_OPENGL
#   include <QOpenGLWidget>
#endif

class DockWidget;

class VisWidget : public QWidget
{
    Q_OBJECT
protected:
    static void setValue(qreal &out, qreal in, qreal tDiffScaled);
    static void setValue(QPair<qreal, double> &out, qreal in, qreal tDiffScaled);

    VisWidget();

    bool canStart() const;

    virtual void paint(QPainter &p) = 0;

    virtual void start() = 0;
    virtual void stop();

    void setUseOpenGL(bool b);

    void resizeEvent(QResizeEvent *e) override;

    QTimer tim;
    bool stopped;
    DockWidget *dw;
    double time;
private:
    void mouseDoubleClickEvent(QMouseEvent *) override final;
    void paintEvent(QPaintEvent *) override final;
    void changeEvent(QEvent *) override final;
    void wheelEvent(QWheelEvent *e) override final;

    bool eventFilter(QObject *watched, QEvent *event) override final;

#ifdef USE_OPENGL
    QOpenGLWidget *glW = nullptr;
    bool m_pendingUpdate = false;
#endif
    bool dockWidgetVisible = false;

    QPixmap m_wallpaper;
private slots:
    void wallpaperChanged(const QPixmap &wallpaper);
    void contextMenu(const QPoint &point);
    void visibilityChanged(bool v);
    void updateVisualization();
    void showSettings();
signals:
    void doubleClicked();
};
