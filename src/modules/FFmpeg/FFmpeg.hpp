/*
	QMPlay2 is a video and audio player.
	Copyright (C) 2010-2017  Błażej Szczygieł

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

#include <QCoreApplication>

class QComboBox;

class FFmpeg : public Module
{
	Q_DECLARE_TR_FUNCTIONS(FFmpeg)
public:
	FFmpeg();
	~FFmpeg();
private:
	QList<Info> getModulesInfo(const bool) const override;
	void *createInstance(const QString &) override;

	SettingsWidget *getSettingsWidget() override;

	void videoDeintSave() override;

	/**/

	QImage demuxIcon;
#ifdef QMPlay2_VDPAU
	QImage vdpauIcon;
	QComboBox *vdpauDeintMethodB;
#endif
#ifdef QMPlay2_VAAPI
	QImage vaapiIcon;
	QComboBox *vaapiDeintMethodB;
#endif
#ifdef QMPlay2_DXVA2
	QImage dxva2Icon;
	bool dxva2Loaded;
#endif

	QMutex mutex;
};

/**/

class QCheckBox;
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
#ifdef QMPlay2_DXVA2
	ModuleSettingsWidget(Module &module, bool dxva2Loaded);
#else
	ModuleSettingsWidget(Module &module);
#endif
#ifdef QMPlay2_VDPAU
private slots:
	void setVDPAU();
	void checkEnables();
#endif
private:
	void saveSettings() override;

	QGroupBox *hurryUpB;
	QCheckBox *demuxerEB, *skipFramesB, *forceSkipFramesB;
	QGroupBox *decoderB;
#ifdef QMPlay2_VDPAU
	QGroupBox *decoderVDPAUB;
	QComboBox *vdpauHQScalingB;
	QCheckBox *noisereductionVDPAUB;
	Slider *noisereductionLvlVDPAUS;
	QCheckBox *decoderVDPAU_NWB;
#endif
#ifdef QMPlay2_VAAPI
	QGroupBox *decoderVAAPIEB;
	QCheckBox *useOpenGLinVAAPIB;
	QCheckBox *allowVDPAUinVAAPIB;
	QCheckBox *copyVideoVAAPIB;
#endif
#ifdef QMPlay2_DXVA2
	QGroupBox *decoderDXVA2EB;
	QCheckBox *copyVideoDXVA2;
#endif
	QSpinBox *threadsB;
	QComboBox *lowresB, *thrTypeB;
};
