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

#include <QDialog>

class QDialogButtonBox;
class QAbstractButton;
class QDoubleSpinBox;
class ColorButton;
class WallpaperW;
class QGroupBox;
class QComboBox;
class QSettings;

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
    void showReadOnlyWarning();
private:
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
    ColorButton *buttonC, *windowC, *shadowC, *highlightC, *baseC, *textC, *highlightedTextC, *sliderButtonC;

    QGroupBox *useWallpaperB;
    WallpaperW *wallpaperW;
    QPushButton *wallpaperB;
    QDoubleSpinBox *alphaB;

    QGroupBox *gradientB;
    ColorButton *grad1C, *grad2C, *qmpTxtC;

    QDialogButtonBox *buttonBox;
};
