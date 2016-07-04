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

class FFmpeg : public Module
{
public:
	FFmpeg();
	~FFmpeg();
private:
	QList<Info> getModulesInfo(const bool) const;
	void *createInstance(const QString &);

	SettingsWidget *getSettingsWidget();

	/**/

	QImage demuxIcon;
#ifdef QMPlay2_VDPAU
	QImage vdpauIcon;
#endif
#ifdef QMPlay2_VAAPI
	QImage vaapiIcon;
#endif

	QMutex mutex;
};

/**/

#include <QCoreApplication>

class QCheckBox;
class QComboBox;
class QGroupBox;
class QSpinBox;
class Slider;

class ModuleSettingsWidget : public Module::SettingsWidget
{
#ifdef QMPlay2_VDPAU
	Q_OBJECT
#else
	Q_DECLARE_TR_FUNCTIONS(ModuleSettingsWidget)
#endif
public:
	ModuleSettingsWidget(Module &);
#ifdef QMPlay2_VDPAU
private slots:
	void setVDPAU();
	void checkEnables();
#endif
private:
	void saveSettings();

	QGroupBox *hurryUpB;
	QCheckBox *demuxerEB, *skipFramesB, *forceSkipFramesB;
	QGroupBox *decoderB;
#ifdef QMPlay2_VDPAU
	QGroupBox *decoderVDPAUB;
	QComboBox *vdpauDeintMethodB, *vdpauHQScalingB;
	QCheckBox *noisereductionVDPAUB, *sharpnessVDPAUB;
	Slider *noisereductionLvlVDPAUS, *sharpnessLvlVDPAUS;
	QCheckBox *decoderVDPAU_NWB;
#endif
#ifdef QMPlay2_VAAPI
	QCheckBox *allowVDPAUinVAAPIB;
	QGroupBox *decoderVAAPIEB;
	QComboBox *vaapiDeintMethodB;
#endif
	QSpinBox *threadsB;
	QComboBox *lowresB, *thrTypeB;
};
