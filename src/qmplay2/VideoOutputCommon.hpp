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

#include <X11BypassCompositor.hpp>

#include <QVariantAnimation>
#include <QMatrix4x4>
#include <QRect>

#include <functional>

class QMouseEvent;
class QVariant;
class QWidget;

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

    QSizeF m_scaledSize;
    QRectF m_subsRect;

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
