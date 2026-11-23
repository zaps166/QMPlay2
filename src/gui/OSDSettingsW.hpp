// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <GUIHeaders.hpp>

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
