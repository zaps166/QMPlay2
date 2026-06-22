/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2026  Błażej Szczygieł

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

#include <QScrollArea>

class QVBoxLayout;
class QFontComboBox;
class QDoubleSpinBox;
class QCheckBox;
class QLabel;
class QSpinBox;
class QRadioButton;
class ColorButton;

class OSDSettingsW : public QScrollArea
{
    Q_OBJECT

public:
    static void init(const QString &, int, int, int, int, int, int, double, double, const QColor &, const QColor &, const QColor &, bool l, bool m);

    OSDSettingsW(const QString &);
    ~OSDSettingsW();

    void addWidget(QWidget *w);

    void writeSettings();

private:
    void setupUi(QWidget *w);

    void readSettings();

    inline void setShadowColorB();

private:
    QVBoxLayout *m_layout;
    QFontComboBox *m_fontCB;
    QSpinBox *m_fontSizeB;
    QSpinBox *m_spacingB;
    QCheckBox *m_boldCB;
    QSpinBox *m_leftMarginB;
    QSpinBox *m_rightMarginB;
    QSpinBox *m_vMarginB;
    QDoubleSpinBox *m_outlineB;
    QLabel *m_shadowL;
    QDoubleSpinBox *m_shadowB;
    QCheckBox *m_backgroundCB;
    ColorButton *m_textColorB;
    ColorButton *m_outlineColorB;
    QLabel *m_shadowColorL;
    ColorButton *m_shadowColorB;

    QRadioButton *alignB[9];
    QString prefix;
};
