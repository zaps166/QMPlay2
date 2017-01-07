/*
	QMPlay2 is a video and audio player.
	Copyright (C) 2010-2016  Błażej Szczygieł

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

#include <QMPlay2Extensions.hpp>

class GraphW : public QWidget
{
public:
	GraphW();

	void setValue(int, float);
	inline void setValues(int vals)
	{
		values.resize(vals);
	}
private:
	void paintEvent(QPaintEvent *);

	QVector<float> values;
	float preamp;
};

/**/

class QScrollArea;
class QCheckBox;
class QSlider;
class QMenu;

class EqualizerGUI : public QWidget, public QMPlay2Extensions
{
	Q_OBJECT
public:
	EqualizerGUI(Module &);

	bool set();

	DockWidget *getDockWidget();
private slots:
	void wallpaperChanged(bool hasWallpaper, double alpha);
	void enabled(bool);
	void valueChanged(int);
	void setSliders();

	void addPreset();

	void showSettings();

	void deletePresetMenuRequest(const QPoint &p);
	void deletePreset();

	void setPresetValues();
private:
	void loadPresets();

	QMap<int, int> getPresetValues(const QString &name);

	DockWidget *dw;
	GraphW graph;

	QCheckBox *enabledB;
	QScrollArea *slidersA;

	QMenu *presetsMenu, *deletePresetMenu;
	QList<QSlider *> sliders;
};

#define EqualizerGUIName "Audio Equalizer Graphical Interface"
