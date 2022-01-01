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

#include <QMPlay2Extensions.hpp>

class GraphW final : public QWidget
{
public:
    GraphW();

    void setValue(int, float);
    inline void setValues(int vals)
    {
        values.resize(vals);
    }
private:
    void paintEvent(QPaintEvent *) override;

    QVector<float> values;
    float preamp;
};

/**/

class QScrollArea;
class QCheckBox;
class QSlider;
class QMenu;

class EqualizerGUI final : public QWidget, public QMPlay2Extensions
{
    Q_OBJECT
public:
    EqualizerGUI(Module &);

    bool set() override;

    DockWidget *getDockWidget() override;
private slots:
    void wallpaperChanged(bool hasWallpaper, double alpha);
    void enabled(bool);

    void valueChanged(int);
    void sliderChecked(bool);

    void setSliders();

    void addPreset();

    void showSettings();

    void deletePresetMenuRequest(const QPoint &p);
    void deletePreset();

    void setPresetValues();
private:
    inline QCheckBox *getSliderCheckBox(QSlider *slider);

    void sliderValueChanged(int idx, int v);
    void setSliderInfo(int idx, int v);

    void autoPreamp();

    void loadPresets();

    void showEvent(QShowEvent *event) override;

    QMap<int, int> getPresetValues(const QString &name);

    DockWidget *dw;
    GraphW graph;

    QCheckBox *enabledB;
    QScrollArea *slidersA;

    QMenu *presetsMenu, *deletePresetMenu;
    QWidget *autoCheckBoxSpacer;
    QList<QSlider *> sliders;

    bool canUpdateEqualizer;
};

#define EqualizerGUIName "Audio Equalizer Graphical Interface"
