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

#include <Module.hpp>

class AudioFilters : public Module
{
public:
	AudioFilters();
private:
	QList<Info> getModulesInfo(const bool) const;
	void *createInstance(const QString &);

	SettingsWidget *getSettingsWidget();
};

/**/

class QGroupBox;
class QCheckBox;
class QComboBox;
class QSpinBox;
class Slider;

class ModuleSettingsWidget : public Module::SettingsWidget
{
	Q_OBJECT
public:
	ModuleSettingsWidget(Module &);
private slots:
	void voiceRemovalToggle();
	void phaseReverse();
	void echo();
private:
	void saveSettings();

	QCheckBox *voiceRemovalEB, *phaseReverseEB, *phaseReverseRightB;

	QGroupBox *echoB;
	Slider *echoDelayB, *echoVolumeB, *echoFeedbackB;
	QCheckBox *echoSurroundB;

	QComboBox *eqQualityB;
	QSpinBox *eqSlidersB, *eqMinFreqB, *eqMaxFreqB;
};
