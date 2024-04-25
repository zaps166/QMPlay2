/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2024  Błażej Szczygieł

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

#include <ColorButton.hpp>

#include <QColorDialog>
#include <QPainter>

ColorButton::ColorButton(QWidget *parent) :
    QPushButton(parent)
{
    setCursor(Qt::PointingHandCursor);
    setAttribute(Qt::WA_OpaquePaintEvent);
    connect(this, SIGNAL(clicked()), this, SLOT(openColorDialog()));
}

void ColorButton::setColor(const QColor &color)
{
    m_color = color;
    m_color.setAlpha(255);
    setToolTip(QString("#%1%2%3")
               .arg(m_color.red(), 2, 16, QChar('0'))
               .arg(m_color.green(), 2, 16, QChar('0'))
               .arg(m_color.blue(), 2, 16, QChar('0'))
               .toUpper()
    );
    update();
}

void ColorButton::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.fillRect(rect(), m_color);
}

void ColorButton::openColorDialog()
{
    const QColor color = QColorDialog::getColor(getColor(), this);
    if (color.isValid() && m_color != color)
    {
        setColor(color);
        emit colorChanged();
    }
}
