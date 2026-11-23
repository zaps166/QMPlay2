// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <AudioFiltersHeaders.hpp>

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
