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

#include <Slider.hpp>

#include <QStyleOptionSlider>
#include <QMouseEvent>
#include <QPainter>
#include <QStyle>

Slider::Slider() :
    QSlider(Qt::Horizontal),
    canSetValue(true), ignoreValueChanged(false),
    wheelStep(5), firstLine(-1), secondLine(-1),
    cachedSliderValue(-1)
{
    setMouseTracking(true);
}

void Slider::setValue(int val)
{
    if (!canSetValue)
        cachedSliderValue = val;
    else
    {
        ignoreValueChanged = true;
        QSlider::setValue(val);
        ignoreValueChanged = false;
    }
}

void Slider::drawRange(int first, int second)
{
    if (second > maximum())
        second = maximum();
    if (first > second)
        first = second;
    if (first != firstLine || second != secondLine)
    {
        firstLine  = first;
        secondLine = second;
        update();
    }
}

void Slider::paintEvent(QPaintEvent *e)
{
    QSlider::paintEvent(e);
    if ((firstLine > -1 || secondLine > -1) && maximum() > 0)
    {
        QPainter p(this);

        QStyleOptionSlider opt;
        initStyleOption(&opt);

        const int handleW_2 = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, this).width() / 2;

        const int o = style()->pixelMetric(QStyle::PM_SliderLength) - 1;
        if (firstLine > -1)
        {
            int X  = QStyle::sliderPositionFromValue(minimum(), maximum(), firstLine,  width() - o, false) + o / 2 - handleW_2;
            if (X < 0)
                X = 0;
            p.drawLine(X, 0, X + handleW_2, 0);
            p.drawLine(X, 0, X, height()-1);
            p.drawLine(X, height()-1, X + handleW_2, height()-1);
        }
        if (secondLine > -1)
        {
            int X = QStyle::sliderPositionFromValue(minimum(), maximum(), secondLine, width() - o, false) + o / 2 + handleW_2 - 1;
            if (X >= width())
                X = width()-1;
            p.drawLine(X, 0, X - handleW_2, 0);
            p.drawLine(X, 0, X, height()-1);
            p.drawLine(X, height()-1, X - handleW_2, height()-1);
        }
    }
}
void Slider::mousePressEvent(QMouseEvent *e)
{
    if (e->buttons() != Qt::RightButton) //Usually context menu or nothing
    {
        cachedSliderValue = -1;
        canSetValue = false;
    }

    const auto style = this->style();
    if (style->styleHint(QStyle::SH_Slider_PageSetButtons) & e->button())
    {
        const auto sliderAbsoluteSetButtons = static_cast<Qt::MouseButtons>(style->styleHint(QStyle::SH_Slider_AbsoluteSetButtons));
        if (sliderAbsoluteSetButtons & (Qt::LeftButton | Qt::MidButton))
        {
            Qt::MouseButton sliderAbsoluteSetButton = Qt::NoButton;
            if (sliderAbsoluteSetButtons & Qt::LeftButton)
                sliderAbsoluteSetButton = Qt::LeftButton;
            else if (sliderAbsoluteSetButtons & Qt::MidButton)
                sliderAbsoluteSetButton = Qt::MidButton;

            QMouseEvent ev(e->type(), e->pos(), sliderAbsoluteSetButton, sliderAbsoluteSetButtons, e->modifiers());
            QSlider::mousePressEvent(&ev);
            e->setAccepted(ev.isAccepted());

            return;
        }
    }

    QSlider::mousePressEvent(e);
}
void Slider::mouseReleaseEvent(QMouseEvent *e)
{
    if (!canSetValue)
    {
        canSetValue = true;
        if (cachedSliderValue > -1)
        {
            setValue(cachedSliderValue);
            cachedSliderValue = -1;
        }
    }
    QSlider::mouseReleaseEvent(e);
}
void Slider::mouseMoveEvent(QMouseEvent *e)
{
    if (maximum() > 0)
    {
        int pos = getMousePos(e->pos());
        if (pos != lastMousePos)
        {
            lastMousePos = pos;
            if (pos < 0)
                pos = 0;
            emit mousePosition(pos);
        }
    }
    QSlider::mouseMoveEvent(e);
}
void Slider::wheelEvent(QWheelEvent *e)
{
    int v;
    if (e->delta() > 0)
        v = value() + wheelStep;
    else
        v = value() - wheelStep;
    v -= v % wheelStep;
    QSlider::setValue(v);
}
void Slider::enterEvent(QEvent *e)
{
    lastMousePos = -1;
    QSlider::enterEvent(e);
}

int Slider::getMousePos(const QPoint &pos)
{
    QStyleOptionSlider opt;
    initStyleOption(&opt);

    const QRect gr = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderGroove, this);
    const QRect sr = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, this);
    const QPoint center = sr.center() - sr.topLeft();

    int sliderMin, sliderMax, sliderLength, p;
    if (orientation() == Qt::Horizontal)
    {
        sliderLength = sr.width();
        sliderMin = gr.x();
        sliderMax = gr.right() - sliderLength + 1;
        p = pos.x() - center.x() - sliderMin;
    }
    else
    {
        sliderLength = sr.height();
        sliderMin = gr.y();
        sliderMax = gr.bottom() - sliderLength + 1;
        p = pos.y() - center.y() - sliderMin;
    }

    return QStyle::sliderValueFromPosition(minimum(), maximum(), p, sliderMax - sliderMin, opt.upsideDown);
}
