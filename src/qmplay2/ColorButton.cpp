// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <ColorButton.hpp>

#include <QColorDialog>
#include <QPainter>

ColorButton::ColorButton(QWidget *parent) :
    QPushButton(parent)
{
    setCursor(Qt::PointingHandCursor);
    setAttribute(Qt::WA_OpaquePaintEvent);
    connect(this, &ColorButton::clicked, this, &ColorButton::openColorDialog);
}

void ColorButton::setAlphaAllowed(bool alpha)
{
    m_alphaChannel = alpha;
}

void ColorButton::setColor(const QColor &color)
{
    m_color = color;
    if (!m_alphaChannel)
        m_color.setAlpha(255);
    setToolTip(QString("#%1%2%3%4")
               .arg(m_color.red(), 2, 16, QChar('0'))
               .arg(m_color.green(), 2, 16, QChar('0'))
               .arg(m_color.blue(), 2, 16, QChar('0'))
               .arg(m_alphaChannel ? QStringLiteral("%1").arg(m_color.alpha(), 2, 16, QChar('0')) : QString())
               .toUpper()
    );
    update();
}

void ColorButton::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    if (m_alphaChannel)
        p.fillRect(rect(), Qt::white);
    p.fillRect(rect(), m_color);
}

void ColorButton::openColorDialog()
{
    const auto color = QColorDialog::getColor(
        m_color,
        this,
        QString(),
        m_alphaChannel ? QColorDialog::ColorDialogOption::ShowAlphaChannel : QColorDialog::ColorDialogOption()
    );
    if (color.isValid() && m_color != color)
    {
        setColor(color);
        emit colorChanged();
    }
}
