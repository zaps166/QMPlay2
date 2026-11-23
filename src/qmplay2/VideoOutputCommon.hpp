// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <X11BypassCompositor.hpp>

class VideoOutputCommon : public X11BypassCompositor
{
protected:
    VideoOutputCommon(bool yInverted);
    ~VideoOutputCommon();

public:
    QWidget *widget() const;

    void resetOffsets();

    virtual bool setSphericalView(bool sphericalView);

protected:
    QSize getRealWidgetSize(bool lh = false) const;

    QPointF getOsdOffset() const;

    void updateSizes(bool transpose);
    void updateMatrix();

protected:
    virtual void dispatchEvent(QEvent *e, QObject *p);

    void mousePress(QMouseEvent *e);
    void mouseMove(QMouseEvent *e);
    void mouseRelease(QMouseEvent *e);

    void mousePress360(QMouseEvent *e);
    void mouseMove360(QMouseEvent *e);
    void mouseRelease360(QMouseEvent *e);

private:
    void rotValueUpdated(const QVariant &value);

protected:
    const float m_yMultiplier;

    QWidget *m_widget = nullptr;
    std::function<void()> m_matrixChangeFn;

    double m_aRatio = 0.0;
    double m_zoom = 0.0;

    QSize m_scaledSize;
    QRect m_subsRect;

    QMatrix4x4 m_matrix;

    QVariantAnimation m_rotAnimation;

    QPointF m_videoOffset;
    QPointF m_osdOffset;

    bool m_moveVideo = false;
    bool m_moveOSD = false;

    bool m_sphericalView = false;

    bool m_buttonPressed = false;
    bool m_mouseWrapped = false;
    bool m_canWrapMouse = false;

    double m_mouseTime = 0.0;
    QPoint m_mousePos;

    QPointF m_rot;
};
