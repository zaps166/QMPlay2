// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QMPlay2Headers.hpp>

class QMPLAY2SHAREDLIB_EXPORT Slider final : public QSlider
{
    Q_OBJECT
public:
    Slider();
    ~Slider() final = default;

    inline bool ignoringValueChanged() const
    {
        return ignoreValueChanged;
    }
public slots:
    void setValue(int val);
    inline void setMaximum(int max)
    {
        QAbstractSlider::setMaximum(max);
    }
    inline void setWheelStep(int ws)
    {
        wheelStep = ws;
    }
    void drawRange(int first, int second);
protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void wheelEvent(QWheelEvent *) override;
    void enterEvent(Q_ENTER_EVENT *) override;
private:
    int getMousePos(const QPoint &pos);

    bool canSetValue, ignoreValueChanged;
    int lastMousePos, wheelStep, firstLine, secondLine;
    int cachedSliderValue;
signals:
    void mousePosition(int xPos);
};
