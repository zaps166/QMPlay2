// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QMPlay2Headers.hpp>

class QMPLAY2SHAREDLIB_EXPORT ColorButton final : public QPushButton
{
    Q_OBJECT
public:
    ColorButton(QWidget *parent = nullptr);

    void setAlphaAllowed(bool alpha);

    void setColor(const QColor &color);
    inline QColor getColor() const
    {
        return m_color;
    }

protected:
    void paintEvent(QPaintEvent *) override;

private:
    void openColorDialog();

signals:
    void colorChanged();

private:
    QColor m_color;
    bool m_alphaChannel = false;
};
