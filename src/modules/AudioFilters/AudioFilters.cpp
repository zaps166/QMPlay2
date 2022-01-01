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

#include <AudioFilters.hpp>
#include <BS2B.hpp>
#include <Equalizer.hpp>
#include <EqualizerGUI.hpp>
#include <VoiceRemoval.hpp>
#include <PhaseReverse.hpp>
#include <SwapStereo.hpp>
#include <Echo.hpp>
#include <DysonCompressor.hpp>

enum Defaults
{
    BS2B_FCUT = 700,
    BS2B_FEED = 45,

    EQUALIZER_FFT_BITS = 10,
    EQUALIZER_COUNT = 8,
    EQUALIZER_MIN_FREQ = 200,
    EQUALIZER_MAX_FREQ = 18000,

    ECHO_DELAY = 500,
    ECHO_VOLUME = 50,
    ECHO_FEEDBACK = 50,
    ECHO_SURROUND = 0,

    COMPRESSOR_PEEK_PERCENT = 90,
    COMPRESSOR_RELEASE_TIME = 2,
    COMPRESSOR_FAST = 9,
    COMPRESSOR_OVERALL = 6
};

AudioFilters::AudioFilters() :
    Module("AudioFilters")
{
    m_icon = QIcon(":/AudioFilters.svgz");

    init("BS2B", false);
    init("BS2B/Fcut", BS2B_FCUT);
    init("BS2B/Feed", BS2B_FEED / 10.0);

    init("Equalizer", false);
    int nbits = getInt("Equalizer/nbits");
    if (nbits < 8 || nbits > 16)
        set("Equalizer/nbits", EQUALIZER_FFT_BITS);
    int count = getInt("Equalizer/count");
    if (count < 2 || count > 20)
        set("Equalizer/count", (count = EQUALIZER_COUNT));
    int minFreq = getInt("Equalizer/minFreq");
    if (minFreq < 10 || minFreq > 300)
        set("Equalizer/minFreq", (minFreq = EQUALIZER_MIN_FREQ));
    int maxFreq = getInt("Equalizer/maxFreq");
    if (maxFreq < 10000 || maxFreq > 96000)
        set("Equalizer/maxFreq", (maxFreq = EQUALIZER_MAX_FREQ));
    init("Equalizer/-1", 50);
    for (int i = 0; i < count; ++i)
        init("Equalizer/" + QString::number(i), 50);

    init("VoiceRemoval", false);

    init("PhaseReverse", false);
    init("PhaseReverse/ReverseRight", false);

    init("SwapStereo", false);

    init("Echo", false);
    init("Echo/Delay", ECHO_DELAY);
    init("Echo/Volume", ECHO_VOLUME);
    init("Echo/Feedback", ECHO_FEEDBACK);
    init("Echo/Surround", (bool)ECHO_SURROUND);

    init("Compressor", false);
    init("Compressor/PeakPercent", COMPRESSOR_PEEK_PERCENT);
    init("Compressor/ReleaseTime", COMPRESSOR_RELEASE_TIME / 10.0);
    init("Compressor/FastGainCompressionRatio", COMPRESSOR_FAST / 10.0);
    init("Compressor/OverallCompressionRatio", COMPRESSOR_OVERALL / 10.0);

    if (getBool("Equalizer"))
    {
        bool disableEQ = true;
        for (int i = -1; i < count; ++i)
        {
            const int val = getInt(QString("Equalizer/%1").arg(i));
            disableEQ &= (((val < 0 && i < 0) ? ~val : val) == 50);
        }
        if (disableEQ)
            set("Equalizer", false);
    }
}

QList<AudioFilters::Info> AudioFilters::getModulesInfo(const bool) const
{
    QList<Info> modulesInfo;
    modulesInfo += Info(BS2BName, AUDIOFILTER);
    modulesInfo += Info(EqualizerName, AUDIOFILTER);
    modulesInfo += Info(EqualizerGUIName, QMPLAY2EXTENSION);
    modulesInfo += Info(VoiceRemovalName, AUDIOFILTER);
    modulesInfo += Info(PhaseReverseName, AUDIOFILTER);
    modulesInfo += Info(SwapStereoName, AUDIOFILTER);
    modulesInfo += Info(EchoName, AUDIOFILTER);
    modulesInfo += Info(DysonCompressorName, AUDIOFILTER);
    return modulesInfo;
}
void *AudioFilters::createInstance(const QString &name)
{
    if (name == BS2BName)
        return new BS2B(*this);
    else if (name == EqualizerName)
        return new Equalizer(*this);
    else if (name == EqualizerGUIName)
        return static_cast<QMPlay2Extensions *>(new EqualizerGUI(*this));
    else if (name == VoiceRemovalName)
        return new VoiceRemoval(*this);
    else if (name == PhaseReverseName)
        return new PhaseReverse(*this);
    else if (name == SwapStereoName)
        return new SwapStereo(*this);
    else if (name == EchoName)
        return new Echo(*this);
    else if (name == DysonCompressorName)
        return new DysonCompressor(*this);
    return nullptr;
}

AudioFilters::SettingsWidget *AudioFilters::getSettingsWidget()
{
    return new ModuleSettingsWidget(*this);
}

QMPLAY2_EXPORT_MODULE(AudioFilters)

/**/

#include <Slider.hpp>

#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QGroupBox>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QLabel>

ModuleSettingsWidget::ModuleSettingsWidget(Module &module) :
    Module::SettingsWidget(module),
    restoringDefault(false)
{
    bs2bB = new QGroupBox(tr(BS2BName));
    bs2bB->setCheckable(true);
    bs2bB->setChecked(sets().getBool("BS2B"));
    connect(bs2bB, SIGNAL(clicked()), this, SLOT(bs2b()));

    bs2bFcutB = new QSpinBox;
    bs2bFcutB->setRange(BS2B_MINFCUT, BS2B_MAXFCUT);
    bs2bFcutB->setValue(sets().getInt("BS2B/Fcut"));
    bs2bFcutB->setPrefix(tr("Cut frequency") + ": ");
    bs2bFcutB->setSuffix(" Hz");
    connect(bs2bFcutB, SIGNAL(valueChanged(int)), this, SLOT(bs2b()));

    bs2bFeedB = new QDoubleSpinBox;
    bs2bFeedB->setRange(BS2B_MINFEED / 10.0, BS2B_MAXFEED / 10.0);
    bs2bFeedB->setValue(sets().getDouble("BS2B/Feed"));
    bs2bFeedB->setSingleStep(0.1);
    bs2bFeedB->setDecimals(1);
    bs2bFeedB->setPrefix(tr("Feed level") + ": ");
    bs2bFeedB->setSuffix(" dB");
    connect(bs2bFeedB, SIGNAL(valueChanged(double)), this, SLOT(bs2b()));

    QGridLayout *bs2bLayoutB = new QGridLayout(bs2bB);
    bs2bLayoutB->addWidget(bs2bFcutB);
    bs2bLayoutB->addWidget(bs2bFeedB);
    bs2bLayoutB->setSpacing(3);
    bs2bLayoutB->setMargin(3);


    voiceRemovalB = new QCheckBox(tr("Voice removal"));
    voiceRemovalB->setChecked(sets().getBool("VoiceRemoval"));
    connect(voiceRemovalB, SIGNAL(clicked()), this, SLOT(voiceRemovalToggle()));


    phaseReverseB = new QGroupBox(tr("Phase reverse"));
    phaseReverseB->setCheckable(true);
    phaseReverseB->setChecked(sets().getBool("PhaseReverse"));
    connect(phaseReverseB, SIGNAL(clicked()), this, SLOT(phaseReverse()));

    phaseReverseRightB = new QCheckBox(tr("Reverse the right channel phase"));
    phaseReverseRightB->setChecked(sets().getBool("PhaseReverse/ReverseRight"));
    connect(phaseReverseRightB, SIGNAL(clicked()), this, SLOT(phaseReverse()));

    QGridLayout *phaseReverseLayout = new QGridLayout(phaseReverseB);
    phaseReverseLayout->addWidget(phaseReverseRightB);
    phaseReverseLayout->setMargin(3);


    swapStereoB = new QCheckBox(tr("Swap stereo channels"));
    swapStereoB->setCheckable(true);
    swapStereoB->setChecked(sets().getBool("SwapStereo"));
    connect(swapStereoB, SIGNAL(clicked()), this, SLOT(swapStereo()));


    echoB = new QGroupBox(tr("Echo"));
    echoB->setCheckable(true);
    echoB->setChecked(sets().getBool("Echo"));
    connect(echoB, SIGNAL(clicked()), this, SLOT(echo()));

    echoDelayS = new Slider;
    echoDelayS->setRange(1, 1000);
    echoDelayS->setValue(sets().getUInt("Echo/Delay"));
    connect(echoDelayS, SIGNAL(valueChanged(int)), this, SLOT(echo()));

    echoVolumeS = new Slider;
    echoVolumeS->setRange(1, 100);
    echoVolumeS->setValue(sets().getUInt("Echo/Volume"));
    connect(echoVolumeS, SIGNAL(valueChanged(int)), this, SLOT(echo()));

    echoFeedbackS = new Slider;
    echoFeedbackS->setRange(1, 100);
    echoFeedbackS->setValue(sets().getUInt("Echo/Feedback"));
    connect(echoFeedbackS, SIGNAL(valueChanged(int)), this, SLOT(echo()));

    echoSurroundB = new QCheckBox(tr("Echo surround"));
    echoSurroundB->setChecked(sets().getBool("Echo/Surround"));
    connect(echoSurroundB, SIGNAL(clicked()), this, SLOT(echo()));

    QFormLayout *echoBLayout = new QFormLayout(echoB);
    echoBLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    echoBLayout->addRow(tr("Echo delay") + ": ", echoDelayS);
    echoBLayout->addRow(tr("Echo volume") + ": ", echoVolumeS);
    echoBLayout->addRow(tr("Echo repeat") + ": ", echoFeedbackS);
    echoBLayout->addRow(echoSurroundB);


    compressorB = new QGroupBox(tr("Dynamic range compression"));
    compressorB->setCheckable(true);
    compressorB->setChecked(sets().getBool("Compressor"));
    connect(compressorB, SIGNAL(clicked()), this, SLOT(compressor()));

    compressorPeakS = new Slider;
    compressorPeakS->setRange(1, 20);
    compressorPeakS->setValue(sets().getInt("Compressor/PeakPercent") / 5);
    connect(compressorPeakS, SIGNAL(valueChanged(int)), this, SLOT(compressor()));

    compressorReleaseTimeS = new Slider;
    compressorReleaseTimeS->setRange(1, 20);
    compressorReleaseTimeS->setValue(sets().getDouble("Compressor/ReleaseTime") * 20);
    connect(compressorReleaseTimeS, SIGNAL(valueChanged(int)), this, SLOT(compressor()));

    compressorFastRatioS = new Slider;
    compressorFastRatioS->setRange(1, 20);
    compressorFastRatioS->setValue(sets().getDouble("Compressor/FastGainCompressionRatio") * 20);
    connect(compressorFastRatioS, SIGNAL(valueChanged(int)), this, SLOT(compressor()));

    compressorRatioS = new Slider;
    compressorRatioS->setRange(1, 20);
    compressorRatioS->setValue(sets().getDouble("Compressor/OverallCompressionRatio") * 20);
    connect(compressorRatioS, SIGNAL(valueChanged(int)), this, SLOT(compressor()));

    QFormLayout *compressorBLayout = new QFormLayout(compressorB);
    compressorBLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    compressorBLayout->addRow(tr("Peak limit") + ": ", compressorPeakS); //[%]
    compressorBLayout->addRow(tr("Release time") + ": ", compressorReleaseTimeS); //[s]
    compressorBLayout->addRow(tr("Fast compression ratio") + ": ", compressorFastRatioS);
    compressorBLayout->addRow(tr("Overall compression ratio") + ": ", compressorRatioS);


    QGroupBox *eqGroupB = new QGroupBox(tr("Equalizer"));

    QLabel *eqQualityL = new QLabel(tr("Sound equalizer quality") + ": ");

    eqQualityB = new QComboBox;
    eqQualityB->addItems({
        tr("Low") + ", " + tr("filter size") + ": 256",
        tr("Low") + ", " + tr("filter size") + ": 512",
        tr("Medium") + ", " + tr("filter size") + ": 1024",
        tr("Medium") + ", " + tr("filter size") + ": 2048",
        tr("High") + ", " + tr("filter size") + ": 4096",
        tr("Very high") + ", " + tr("filter size") + ": 8192",
        tr("Very high") + ", " + tr("filter size") + ": 16384",
        tr("Very high") + ", " + tr("filter size") + ": 32768",
        tr("Very high") + ", " + tr("filter size") + ": 65536"
    });
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

    QGridLayout *eqLayout = new QGridLayout(eqGroupB);
    eqLayout->addWidget(eqQualityL, 1, 0);
    eqLayout->addWidget(eqQualityB, 1, 1);
    eqLayout->addWidget(eqSlidersL, 2, 0);
    eqLayout->addWidget(eqSlidersB, 2, 1);
    eqLayout->addWidget(eqMinFreqB, 3, 0);
    eqLayout->addWidget(eqMaxFreqB, 3, 1);
    eqLayout->setMargin(3);


    defaultB = new QPushButton(tr("Default settings"));
    connect(defaultB, SIGNAL(clicked(bool)), this, SLOT(defaultSettings()));


    QGridLayout *layout = new QGridLayout(this);
    layout->addWidget(bs2bB);
    layout->addWidget(voiceRemovalB);
    layout->addWidget(phaseReverseB);
    layout->addWidget(swapStereoB);
    layout->addWidget(echoB);
    layout->addWidget(compressorB);
    layout->addWidget(eqGroupB);
    layout->addWidget(defaultB);
}

void ModuleSettingsWidget::bs2b()
{
    if (restoringDefault)
        return;
    sets().set("BS2B", bs2bB->isChecked());
    sets().set("BS2B/Fcut", bs2bFcutB->value());
    sets().set("BS2B/Feed", bs2bFeedB->value());
    SetInstance<BS2B>();
}
void ModuleSettingsWidget::voiceRemovalToggle()
{
    if (restoringDefault)
        return;
    sets().set("VoiceRemoval", voiceRemovalB->isChecked());
    SetInstance<VoiceRemoval>();
}
void ModuleSettingsWidget::phaseReverse()
{
    if (restoringDefault)
        return;
    sets().set("PhaseReverse", phaseReverseB->isChecked());
    sets().set("PhaseReverse/ReverseRight", phaseReverseRightB->isChecked());
    SetInstance<PhaseReverse>();
}
void ModuleSettingsWidget::swapStereo()
{
    if (restoringDefault)
        return;
    sets().set("SwapStereo", swapStereoB->isChecked());
    SetInstance<SwapStereo>();
}
void ModuleSettingsWidget::echo()
{
    if (restoringDefault)
        return;
    sets().set("Echo", echoB->isChecked());
    sets().set("Echo/Delay", echoDelayS->value());
    sets().set("Echo/Volume", echoVolumeS->value());
    sets().set("Echo/Feedback", echoFeedbackS->value());
    sets().set("Echo/Surround", echoSurroundB->isChecked());
    SetInstance<Echo>();
}
void ModuleSettingsWidget::compressor()
{
    if (restoringDefault)
        return;
    sets().set("Compressor", compressorB->isChecked());
    sets().set("Compressor/PeakPercent", compressorPeakS->value() * 5);
    sets().set("Compressor/ReleaseTime", compressorReleaseTimeS->value() / 20.0);
    sets().set("Compressor/FastGainCompressionRatio", compressorFastRatioS->value() / 20.0);
    sets().set("Compressor/OverallCompressionRatio", compressorRatioS->value() / 20.0);
    SetInstance<DysonCompressor>();
}
void ModuleSettingsWidget::defaultSettings()
{
    restoringDefault = true;

    bs2bB->setChecked(false);
    bs2bFcutB->setValue(BS2B_FCUT);
    bs2bFeedB->setValue(BS2B_FEED / 10.0);

    eqQualityB->setCurrentIndex(EQUALIZER_FFT_BITS - 8);
    eqSlidersB->setValue(EQUALIZER_COUNT);
    eqMinFreqB->setValue(EQUALIZER_MIN_FREQ);
    eqMaxFreqB->setValue(EQUALIZER_MAX_FREQ);

    voiceRemovalB->setChecked(false);

    phaseReverseB->setChecked(false);
    phaseReverseRightB->setChecked(false);

    swapStereoB->setChecked(false);

    echoB->setChecked(false);
    echoDelayS->setValue(ECHO_DELAY);
    echoVolumeS->setValue(ECHO_VOLUME);
    echoFeedbackS->setValue(ECHO_FEEDBACK);
    echoSurroundB->setChecked(ECHO_SURROUND);

    compressorB->setChecked(false);
    compressorPeakS->setValue(COMPRESSOR_PEEK_PERCENT / 5);
    compressorReleaseTimeS->setValue(COMPRESSOR_RELEASE_TIME * 2);
    compressorFastRatioS->setValue(COMPRESSOR_FAST * 2);
    compressorRatioS->setValue(COMPRESSOR_OVERALL * 2);

    restoringDefault = false;

    bs2b();
    saveSettings(); //For equalizer
    SetInstance<EqualizerGUI>();
    SetInstance<Equalizer>();
    voiceRemovalToggle();
    phaseReverse();
    swapStereo();
    echo();
    compressor();
}

void ModuleSettingsWidget::saveSettings()
{
    sets().set("Equalizer/nbits", eqQualityB->currentIndex() + 8);
    sets().set("Equalizer/count", eqSlidersB->value());
    sets().set("Equalizer/minFreq", eqMinFreqB->value());
    sets().set("Equalizer/maxFreq", eqMaxFreqB->value());
}
