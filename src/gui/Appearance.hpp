// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <GUIHeaders.hpp>

class ColorButton;
class WallpaperW;

class Appearance : public QDialog
{
    Q_OBJECT
public:
    static void init();
    static void applyPalette(const QPalette &pal, const QPalette &sliderButton_pal, const QPalette &mainW_pal);

    Appearance(QWidget *p);
private slots:
    void schemesIndexChanged(int idx);
    void newScheme();
    void saveScheme();
    void deleteScheme();
    void chooseWallpaper();
    void buttonBoxClicked(QAbstractButton *b);
private:
    void showReadOnlyWarning();
    void saveScheme(QSettings &colorScheme);
    void reloadSchemes();
    void loadCurrentPalette();
    void loadDefaultPalette();
    void apply();

    int rwSchemesIdx;
    bool warned;

    QGroupBox *colorSchemesB;
    QComboBox *schemesB;
    QPushButton *newB, *saveB, *deleteB;

    QGroupBox *useColorsB;
    QHash<QPalette::ColorRole, ColorButton *> m_colorButtons;
    ColorButton *m_colorSliderButton, *m_colorSliderHighlight;

    QGroupBox *useWallpaperB;
    WallpaperW *wallpaperW;
    QPushButton *wallpaperB;
    QDoubleSpinBox *alphaB;

    QGroupBox *gradientB;
    ColorButton *grad1C, *grad2C, *qmpTxtC;

    QDialogButtonBox *buttonBox;
};
