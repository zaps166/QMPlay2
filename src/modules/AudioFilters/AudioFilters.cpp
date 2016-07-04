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

#include <AudioFilters.hpp>
#include <Equalizer.hpp>
#include <EqualizerGUI.hpp>
#include <VoiceRemoval.hpp>
#include <PhaseReverse.hpp>
#include <Echo.hpp>

AudioFilters::AudioFilters() :
	Module("AudioFilters")
{
	moduleImg = QImage(":/AudioFilters");

	init("Equalizer", false);
	int nbits = getInt("Equalizer/nbits");
	if (nbits < 8 || nbits > 16)
		set("Equalizer/nbits", 10);
	int count = getInt("Equalizer/count");
	if (count < 2 || count > 20)
		set("Equalizer/count", (count = 8));
	int minFreq = getInt("Equalizer/minFreq");
	if (minFreq < 10 || minFreq > 300)
		set("Equalizer/minFreq", (minFreq = 200));
	int maxFreq = getInt("Equalizer/maxFreq");
	if (maxFreq < 10000 || maxFreq > 96000)
		set("Equalizer/maxFreq", (maxFreq = 18000));
	init("Equalizer/-1", 50);
	for (int i = 0; i < count; ++i)
		init("Equalizer/" + QString::number(i), 50);
	init("VoiceRemoval", false);
	init("PhaseReverse", false);
	init("PhaseReverse/ReverseRight", false);
	init("Echo", false);
	init("Echo/Delay", 500);
	init("Echo/Volume", 50);
	init("Echo/Feedback", 50);
	init("Echo/Surround", false);
	if (getBool("Equalizer"))
	{
		bool disableEQ = true;
		for (int i = -1; i < count; ++i)
			disableEQ &= getInt("Equalizer/" + QString::number(i)) == 50;
		if (disableEQ)
			set("Equalizer", false);
	}
}

QList<AudioFilters::Info> AudioFilters::getModulesInfo(const bool) const
{
	QList<Info> modulesInfo;
	modulesInfo += Info(EqualizerName, AUDIOFILTER);
	modulesInfo += Info(EqualizerGUIName, QMPLAY2EXTENSION);
	modulesInfo += Info(VoiceRemovalName, AUDIOFILTER);
	modulesInfo += Info(PhaseReverseName, AUDIOFILTER);
	modulesInfo += Info(EchoName, AUDIOFILTER);
	return modulesInfo;
}
void *AudioFilters::createInstance(const QString &name)
{
	if (name == EqualizerName)
		return new Equalizer(*this);
	else if (name == EqualizerGUIName)
		return static_cast<QMPlay2Extensions *>(new EqualizerGUI(*this));
	else if (name == VoiceRemovalName)
		return new VoiceRemoval(*this);
	else if (name == PhaseReverseName)
		return new PhaseReverse(*this);
	else if (name == EchoName)
		return new Echo(*this);
	return NULL;
}

AudioFilters::SettingsWidget *AudioFilters::getSettingsWidget()
{
	return new ModuleSettingsWidget(*this);
}

QMPLAY2_EXPORT_PLUGIN(AudioFilters)

/**/

#include <Slider.hpp>

#include <QGridLayout>
#include <QGroupBox>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QLabel>

ModuleSettingsWidget::ModuleSettingsWidget(Module &module) :
	Module::SettingsWidget(module)
{
	voiceRemovalEB = new QCheckBox(tr("Voice removal"));
	voiceRemovalEB->setChecked(sets().getBool("VoiceRemoval"));
	connect(voiceRemovalEB, SIGNAL(clicked()), this, SLOT(voiceRemovalToggle()));

	phaseReverseEB = new QCheckBox(tr("Phase reverse"));
	phaseReverseEB->setChecked(sets().getBool("PhaseReverse"));
	connect(phaseReverseEB, SIGNAL(clicked()), this, SLOT(phaseReverse()));

	phaseReverseRightB = new QCheckBox(tr("Reverse the right channel phase"));
	phaseReverseRightB->setChecked(sets().getBool("PhaseReverse/ReverseRight"));
	connect(phaseReverseRightB, SIGNAL(clicked()), this, SLOT(phaseReverse()));

	phaseReverseRightB->setEnabled(phaseReverseEB->isChecked());

	echoB = new QGroupBox(tr("Echo"));
	echoB->setCheckable(true);
	echoB->setChecked(sets().getBool("Echo"));
	connect(echoB, SIGNAL(clicked()), this, SLOT(echo()));

	QLabel *echoDelayL = new QLabel(tr("Echo delay") + ": ");

	echoDelayB = new Slider;
	echoDelayB->setRange(1, 1000);
	echoDelayB->setValue(sets().getUInt("Echo/Delay"));
	connect(echoDelayB, SIGNAL(valueChanged(int)), this, SLOT(echo()));

	QLabel *echoVolumeL = new QLabel(tr("Echo volume") + ": ");

	echoVolumeB = new Slider;
	echoVolumeB->setRange(1, 100);
	echoVolumeB->setValue(sets().getUInt("Echo/Volume"));
	connect(echoVolumeB, SIGNAL(valueChanged(int)), this, SLOT(echo()));

	QLabel *echoFeedbackL = new QLabel(tr("Echo repeat") + ": ");

	echoFeedbackB = new Slider;
	echoFeedbackB->setRange(1, 100);
	echoFeedbackB->setValue(sets().getUInt("Echo/Feedback"));
	connect(echoFeedbackB, SIGNAL(valueChanged(int)), this, SLOT(echo()));

	echoSurroundB = new QCheckBox(tr("Echo surround"));
	connect(echoSurroundB, SIGNAL(clicked()), this, SLOT(echo()));

	QGridLayout *echoBLayout = new QGridLayout(echoB);
	echoBLayout->addWidget(echoDelayL, 0, 0, 1, 1);
	echoBLayout->addWidget(echoDelayB, 0, 1, 1, 1);
	echoBLayout->addWidget(echoVolumeL, 1, 0, 1, 1);
	echoBLayout->addWidget(echoVolumeB, 1, 1, 1, 1);
	echoBLayout->addWidget(echoFeedbackL, 2, 0, 1, 1);
	echoBLayout->addWidget(echoFeedbackB, 2, 1, 1, 1);
	echoBLayout->addWidget(echoSurroundB, 3, 0, 1, 2);

	QLabel *eqQualityL = new QLabel(tr("Sound equalizer quality") + ": ");

	eqQualityB = new QComboBox;
	eqQualityB->addItems(QStringList()
		<< tr("Low") + ", " + tr("filter size") + ": 256"
		<< tr("Low") + ", " + tr("filter size") + ": 512"
		<< tr("Medium") + ", " + tr("filter size") + ": 1024"
		<< tr("Medium") + ", " + tr("filter size") + ": 2048"
		<< tr("High") + ", " + tr("filter size") + ": 4096"
		<< tr("Very high") + ", " + tr("filter size") + ": 8192"
		<< tr("Very high") + ", " + tr("filter size") + ": 16384"
		<< tr("Very high") + ", " + tr("filter size") + ": 32768"
		<< tr("Very high") + ", " + tr("filter size") + ": 65536"
	);
	eqQualityB->setCurrentIndex(sets().getInt("Equalizer/nbits") - 8);

	QLabel *eqSlidersL = new QLabel(tr("Slider count in sound equalizer") + ": ");

	eqSlidersB = new QSpinBox;
	eqSlidersB->setRange(2, 20);
	eqSlidersB->setValue(sets().getInt("Equalizer/count"));

	eqMinFreqB = new QSpinBox;
	eqMinFreqB->setPrefix(tr("Minimum frequency") + ": ");
	eqMinFreqB->setSuffix(" Hz");
	eqMinFreqB->setRange(10, 300);
	eqMinFreqB->setValue(sets().getInt("Equalizer/minFreq"));

	eqMaxFreqB = new QSpinBox;
	eqMaxFreqB->setPrefix(tr("Maximum frequency") + ": ");
	eqMaxFreqB->setSuffix(" Hz");
	eqMaxFreqB->setRange(10000, 96000);
	eqMaxFreqB->setValue(sets().getInt("Equalizer/maxFreq"));

	QGridLayout *layout = new QGridLayout(this);
	layout->addWidget(voiceRemovalEB, 0, 0, 1, 2);
	layout->addWidget(phaseReverseEB, 1, 0, 1, 2);
	layout->addWidget(phaseReverseRightB, 2, 0, 1, 2);
	layout->addWidget(echoB, 3, 0, 1, 2);
	layout->addWidget(eqQualityL, 4, 0, 1, 1);
	layout->addWidget(eqQualityB, 4, 1, 1, 1);
	layout->addWidget(eqSlidersL, 5, 0, 1, 1);
	layout->addWidget(eqSlidersB, 5, 1, 1, 1);
	layout->addWidget(eqMinFreqB, 6, 0, 1, 1);
	layout->addWidget(eqMaxFreqB, 6, 1, 1, 1);
}

void ModuleSettingsWidget::voiceRemovalToggle()
{
	sets().set("VoiceRemoval", voiceRemovalEB->isChecked());
	SetInstance<VoiceRemoval>();
}
void ModuleSettingsWidget::phaseReverse()
{
	sets().set("PhaseReverse", phaseReverseEB->isChecked());
	sets().set("PhaseReverse/ReverseRight", phaseReverseRightB->isChecked());
	phaseReverseRightB->setEnabled(phaseReverseEB->isChecked());
	SetInstance<PhaseReverse>();
}
void ModuleSettingsWidget::echo()
{
	sets().set("Echo", echoB->isChecked());
	sets().set("Echo/Delay", echoDelayB->value());
	sets().set("Echo/Volume", echoVolumeB->value());
	sets().set("Echo/Feedback", echoFeedbackB->value());
	sets().set("Echo/Surround", echoSurroundB->isChecked());
	SetInstance<Echo>();
}

void ModuleSettingsWidget::saveSettings()
{
	sets().set("Equalizer/nbits", eqQualityB->currentIndex() + 8);
	sets().set("Equalizer/count", eqSlidersB->value());
	sets().set("Equalizer/minFreq", eqMinFreqB->value());
	sets().set("Equalizer/maxFreq", eqMaxFreqB->value());
}
