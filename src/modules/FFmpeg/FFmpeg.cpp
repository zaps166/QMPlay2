/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2019  Błażej Szczygieł

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

#include <FFmpeg.hpp>
#include <FFDemux.hpp>
#include <FFDecSW.hpp>
#ifdef QMPlay2_VAAPI
    #include <FFDecVAAPI.hpp>
    #include <VAAPIWriter.hpp>
#endif
#ifdef QMPlay2_VDPAU
    #include <FFDecVDPAU.hpp>
    #include <FFDecVDPAU_NW.hpp>
    #include <VDPAUWriter.hpp>
#endif
#ifdef QMPlay2_DXVA2
    #include <FFDecDXVA2.hpp>
#endif
#ifdef QMPlay2_VTB
    #include <FFDecVTB.hpp>
#endif
#include <FFReader.hpp>
#include <FFCommon.hpp>

extern "C"
{
    #include <libavformat/avformat.h>
#ifdef QMPlay2_libavdevice
    #include <libavdevice/avdevice.h>
#endif
}

#include <QComboBox>

FFmpeg::FFmpeg() :
    Module("FFmpeg"),
    demuxIcon(":/FFDemux.svgz")
{
    m_icon = QIcon(":/FFmpeg.svgz");

#ifdef QMPlay2_VDPAU
    vdpauIcon = QIcon(":/VDPAU.svgz");
#endif
#ifdef QMPlay2_VAAPI
    vaapiIcon = QIcon(":/VAAPI.svgz");
#endif
#ifdef QMPlay2_DXVA2
    dxva2Icon = QIcon(":/DXVA2.svgz");
    dxva2Loaded = FFDecDXVA2::loadLibraries();
#endif
#ifdef QMPlay2_VTB
    vtbIcon = QIcon(":/VAAPI.svgz");
#endif

    init("DemuxerEnabled", true);
    init("ReconnectStreammes", false);
    init("DecoderEnabled", true);
#ifdef QMPlay2_VDPAU
    init("DecoderVDPAUEnabled", true);
    init("DecoderVDPAU_NWEnabled", false);
    init("VDPAUDeintMethod", 1);
    if (getUInt("VDPAUDeintMethod") > 2)
        set("VDPAUDeintMethod", 1);
    init("VDPAUNoiseReductionEnabled", false);
    init("VDPAUNoiseReductionLvl", 0.0);
    init("VDPAUHQScaling", 0);
    if (getUInt("VDPAUHQScaling") > 9)
        set("VDPAUHQScaling", 0);
#endif
#ifdef QMPlay2_VAAPI
    init("DecoderVAAPIEnabled", true);
    init("UseOpenGLinVAAPI", true);
    init("CopyVideoVAAPI", Qt::Unchecked);
    init("VAAPIDeintMethod", 1);
    if (getUInt("VAAPIDeintMethod") > 2)
        set("VAAPIDeintMethod", 1);
#endif
#ifdef QMPlay2_DXVA2
    init("DecoderDXVA2Enabled", true);
    init("CopyVideoDXVA2", Qt::Unchecked);
#endif
#ifdef QMPlay2_VTB
    init("DecoderVTBEnabled", true);
    init("CopyVideoVTB", false);
#endif
    init("HurryUP", true);
    init("SkipFrames", true);
    init("ForceSkipFrames", false);
    init("Threads", 0);
    init("LowresValue", 0);
    init("ThreadTypeSlice", false);

    const QVariant self = QVariant::fromValue((void *)this);

#ifdef QMPlay2_VDPAU
    vdpauDeintMethodB = new QComboBox;
    vdpauDeintMethodB->addItems({tr("None"), "Temporal", "Temporal-spatial"});
    vdpauDeintMethodB->setCurrentIndex(getInt("VDPAUDeintMethod"));
    if (vdpauDeintMethodB->currentIndex() < 0)
        vdpauDeintMethodB->setCurrentIndex(1);
    vdpauDeintMethodB->setProperty("text", QString(tr("Deinterlacing method") + " (VDPAU): "));
    vdpauDeintMethodB->setProperty("module", self);
    QMPlay2Core.addVideoDeintMethod(vdpauDeintMethodB);
#endif
#if defined(QMPlay2_VAAPI)
    vaapiDeintMethodB = new QComboBox;
    vaapiDeintMethodB->addItems({tr("None"), "Motion adaptive", "Motion compensated"});
    vaapiDeintMethodB->setCurrentIndex(getInt("VAAPIDeintMethod"));
    if (vaapiDeintMethodB->currentIndex() < 0)
        vaapiDeintMethodB->setCurrentIndex(1);
    vaapiDeintMethodB->setProperty("text", QString(tr("Deinterlacing method") + " (VA-API): "));
    vaapiDeintMethodB->setProperty("module", self);
    QMPlay2Core.addVideoDeintMethod(vaapiDeintMethodB);
#endif

    static bool firstTime = true;
    if (firstTime)
    {
#ifdef QMPlay2_libavdevice
        avdevice_register_all();
#endif
        firstTime = false;
    }
}
FFmpeg::~FFmpeg()
{
#ifdef QMPlay2_VDPAU
    delete vdpauDeintMethodB;
#endif
#if defined(QMPlay2_VAAPI)
    delete vaapiDeintMethodB;
#endif
}

QList<FFmpeg::Info> FFmpeg::getModulesInfo(const bool showDisabled) const
{
    QList<Info> modulesInfo;
    if (showDisabled || getBool("DemuxerEnabled"))
        modulesInfo += Info(DemuxerName, DEMUXER, demuxIcon);
    if (showDisabled || getBool("DecoderEnabled"))
        modulesInfo += Info(DecoderName, DECODER, m_icon);
#ifdef QMPlay2_VDPAU
    if (showDisabled || getBool("DecoderVDPAUEnabled"))
    {
        modulesInfo += Info(DecoderVDPAUName, DECODER, vdpauIcon);
        modulesInfo += Info(VDPAUWriterName, WRITER | VIDEOHWFILTER, vdpauIcon);
    }
    if (showDisabled || getBool("DecoderVDPAU_NWEnabled"))
        modulesInfo += Info(DecoderVDPAU_NWName, DECODER, vdpauIcon);
#endif
#ifdef QMPlay2_VAAPI
    if (showDisabled || getBool("DecoderVAAPIEnabled"))
    {
        modulesInfo += Info(DecoderVAAPIName, DECODER, vaapiIcon);
        modulesInfo += Info(VAAPIWriterName, WRITER | VIDEOHWFILTER, vaapiIcon);
    }
#endif
#ifdef QMPlay2_DXVA2
    if (showDisabled || (dxva2Loaded && getBool("DecoderDXVA2Enabled")))
        modulesInfo += Info(DecoderDXVA2Name, DECODER, dxva2Icon);
#endif
#ifdef QMPlay2_VTB
    if (showDisabled || getBool("DecoderVTBEnabled"))
        modulesInfo += Info(DecoderVTBName, DECODER, vtbIcon);
#endif
    modulesInfo += Info(FFReaderName, READER, {"http", "https", "mms", "rtmp", "rtsp"}, m_icon);
    return modulesInfo;
}
void *FFmpeg::createInstance(const QString &name)
{
    if (name == DemuxerName && getBool("DemuxerEnabled"))
        return new FFDemux(*this);
    else if (name == DecoderName && getBool("DecoderEnabled"))
        return new FFDecSW(*this);
#ifdef QMPlay2_VDPAU
    else if (name == DecoderVDPAUName && getBool("DecoderVDPAUEnabled"))
        return new FFDecVDPAU(*this);
    else if (name == DecoderVDPAU_NWName && getBool("DecoderVDPAU_NWEnabled"))
        return new FFDecVDPAU_NW(*this);
#endif
#ifdef QMPlay2_VAAPI
    else if (name == DecoderVAAPIName && getBool("DecoderVAAPIEnabled"))
        return new FFDecVAAPI(*this);
#endif
#ifdef QMPlay2_DXVA2
    else if (name == DecoderDXVA2Name && (dxva2Loaded && getBool("DecoderDXVA2Enabled")))
        return new FFDecDXVA2(*this);
#endif
#ifdef QMPlay2_VTB
    else if (name == DecoderVTBName && getBool("DecoderVTBEnabled"))
        return new FFDecVTB(*this);
#endif
    else if (name == FFReaderName)
        return new FFReader;
    return nullptr;
}

FFmpeg::SettingsWidget *FFmpeg::getSettingsWidget()
{
#ifdef QMPlay2_DXVA2
    return new ModuleSettingsWidget(*this, dxva2Loaded);
#else
    return new ModuleSettingsWidget(*this);
#endif
}

void FFmpeg::videoDeintSave()
{
#ifdef QMPlay2_VDPAU
    set("VDPAUDeintMethod", vdpauDeintMethodB->currentIndex());
    setInstance<VDPAUWriter>();
#endif
#if defined(QMPlay2_VAAPI)
    set("VAAPIDeintMethod", vaapiDeintMethodB->currentIndex());
    setInstance<FFDecVAAPI>();
#endif
}

QMPLAY2_EXPORT_MODULE(FFmpeg)

/**/

#include <Slider.hpp>

#include <QGridLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QLabel>

#ifdef QMPlay2_DXVA2
ModuleSettingsWidget::ModuleSettingsWidget(Module &module, bool dxva2Loaded) :
#else
ModuleSettingsWidget::ModuleSettingsWidget(Module &module) :
#endif
    Module::SettingsWidget(module)
{
    demuxerB = new QGroupBox(tr("Demuxer"));
    demuxerB->setCheckable(true);
    demuxerB->setChecked(sets().getBool("DemuxerEnabled"));

    reconnectStreamedB = new QCheckBox(tr("Try to automatically reconnect live streams on error"));
    reconnectStreamedB->setChecked(sets().getBool("ReconnectStreamed"));

    decoderB = new QGroupBox(tr("Software decoder"));
    decoderB->setCheckable(true);
    decoderB->setChecked(sets().getBool("DecoderEnabled"));

#ifdef QMPlay2_VDPAU
    decoderVDPAUB = new QGroupBox(tr("Decoder") + " VDPAU - " + tr("hardware decoding"));
    decoderVDPAUB->setCheckable(true);
    decoderVDPAUB->setChecked(sets().getBool("DecoderVDPAUEnabled"));

    vdpauHQScalingB = new QComboBox;
    for (int i = 0; i <= 9; ++i)
        vdpauHQScalingB->addItem(QString("Level %1").arg(i));
    vdpauHQScalingB->setCurrentIndex(sets().getUInt("VDPAUHQScaling"));

    noisereductionVDPAUB = new QCheckBox(tr("Noise reduction"));
    noisereductionVDPAUB->setChecked(sets().getBool("VDPAUNoiseReductionEnabled"));
    connect(noisereductionVDPAUB, SIGNAL(clicked()), this, SLOT(checkEnables()));
    connect(noisereductionVDPAUB, SIGNAL(clicked()), this, SLOT(setVDPAU()));
    noisereductionLvlVDPAUS = new Slider;
    noisereductionLvlVDPAUS->setRange(0, 50);
    noisereductionLvlVDPAUS->setTickInterval(50);
    noisereductionLvlVDPAUS->setTickPosition(QSlider::TicksBelow);
    noisereductionLvlVDPAUS->setValue(sets().getDouble("VDPAUNoiseReductionLvl") * 50);
    connect(noisereductionLvlVDPAUS, SIGNAL(valueChanged(int)), this, SLOT(setVDPAU()));

    checkEnables();

    QFormLayout *vdpauLayout = new QFormLayout(decoderVDPAUB);
    vdpauLayout->addRow(tr("Image scaling level") + ": ", vdpauHQScalingB);
    vdpauLayout->addRow(noisereductionVDPAUB, noisereductionLvlVDPAUS);

    decoderVDPAU_NWB = new QCheckBox(tr("Decoder") + " VDPAU (no output) - " + tr("hardware decoding"));
    decoderVDPAU_NWB->setToolTip(tr("This decoder doesn't have its own video output, so it can be used with any video output.\nIt copies decoded video frame to system RAM, so it can be slow!"));
    decoderVDPAU_NWB->setChecked(sets().getBool("DecoderVDPAU_NWEnabled"));
#endif

#ifdef QMPlay2_VAAPI
    decoderVAAPIEB = new QGroupBox(tr("Decoder") + " VAAPI - " + tr("hardware decoding"));
    decoderVAAPIEB->setCheckable(true);
    decoderVAAPIEB->setChecked(sets().getBool("DecoderVAAPIEnabled"));

    useOpenGLinVAAPIB = new QCheckBox(tr("Use OpenGL"));
    useOpenGLinVAAPIB->setChecked(sets().getBool("UseOpenGLinVAAPI"));

    copyVideoVAAPIB = new QCheckBox(tr("Copy decoded video to CPU memory (not recommended)"));
    copyVideoVAAPIB->setTristate(true);
    copyVideoVAAPIB->setCheckState((Qt::CheckState)sets().getInt("CopyVideoVAAPI"));
    copyVideoVAAPIB->setToolTip(tr("Partially checked means that it will copy a video data only if the fast method fails"));

    QFormLayout *vaapiLayout = new QFormLayout(decoderVAAPIEB);
    vaapiLayout->addRow(useOpenGLinVAAPIB);
    vaapiLayout->addRow(copyVideoVAAPIB);
#endif

#ifdef QMPlay2_DXVA2
    if (dxva2Loaded)
    {
        decoderDXVA2EB = new QGroupBox(tr("Decoder") + " DXVA2 - " + tr("hardware decoding"));
        decoderDXVA2EB->setCheckable(true);
        decoderDXVA2EB->setChecked(sets().getBool("DecoderDXVA2Enabled"));

        copyVideoDXVA2 = new QCheckBox(tr("Copy decoded video to CPU memory (not recommended, very slow on Intel)"));
        copyVideoDXVA2->setTristate(true);
        copyVideoDXVA2->setCheckState((Qt::CheckState)sets().getInt("CopyVideoDXVA2"));
        copyVideoDXVA2->setToolTip(tr("Partially checked means that it will copy a video data only if the fast method fails"));

        QFormLayout *dxva2Layout = new QFormLayout(decoderDXVA2EB);
        dxva2Layout->addRow(copyVideoDXVA2);
    }
    else
    {
        decoderDXVA2EB = nullptr;
        copyVideoDXVA2 = nullptr;
    }
#endif

#ifdef QMPlay2_VTB
    decoderVTBEB = new QGroupBox(tr("Decoder") + " VideoToolBox - " + tr("hardware decoding"));
    decoderVTBEB->setCheckable(true);
    decoderVTBEB->setChecked(sets().getBool("DecoderVTBEnabled"));

    copyVideoVTB = new QCheckBox("Copy decoded video to CPU memory (not recommended)");
    copyVideoVTB->setChecked(sets().getBool("CopyVideoVTB"));

    QFormLayout *vtbLayout = new QFormLayout(decoderVTBEB);
    vtbLayout->addRow(copyVideoVTB);
#endif

    /* Hurry up */
    hurryUpB = new QGroupBox(tr("Hurry up"));
    hurryUpB->setCheckable(true);
    hurryUpB->setChecked(sets().getBool("HurryUP"));

    skipFramesB = new QCheckBox(tr("Skip some frames"));
    skipFramesB->setChecked(sets().getBool("SkipFrames"));

    forceSkipFramesB = new QCheckBox(tr("Force frame skipping"));
    forceSkipFramesB->setChecked(sets().getBool("ForceSkipFrames"));

    QHBoxLayout *hurryUpLayout = new QHBoxLayout(hurryUpB);
    hurryUpLayout->addWidget(skipFramesB);
    hurryUpLayout->addWidget(forceSkipFramesB);
    /**/

    threadsB = new QSpinBox;
    threadsB->setRange(0, 16);
    threadsB->setSpecialValueText(tr("Autodetect"));
    threadsB->setValue(sets().getInt("Threads"));

    lowresB = new QComboBox;
    lowresB->addItems({tr("Normal size"), tr("4x smaller"), tr("16x smaller")});
    lowresB->setCurrentIndex(sets().getInt("LowresValue"));
    if (lowresB->currentIndex() < 0)
    {
        lowresB->setCurrentIndex(0);
        sets().set("LowresValue", 0);
    }

    thrTypeB = new QComboBox;
    thrTypeB->addItems({tr("Frames"), tr("Slices")});
    thrTypeB->setCurrentIndex(sets().getInt("ThreadTypeSlice"));
    if (thrTypeB->currentIndex() < 0)
    {
        thrTypeB->setCurrentIndex(0);
        sets().set("ThreadTypeSlice", 0);
    }

    QFormLayout *demuxerLayout = new QFormLayout(demuxerB);
    demuxerLayout->addRow(nullptr, reconnectStreamedB);

    QFormLayout *decoderLayout = new QFormLayout(decoderB);
    decoderLayout->addRow(tr("Number of threads used to decode video") + ": ", threadsB);
    decoderLayout->addRow(tr("Low resolution decoding (only some codecs)") + ": ", lowresB);
    decoderLayout->addRow(tr("Method of multithreaded decoding") + ": ", thrTypeB);
    decoderLayout->addRow(hurryUpB);

    connect(skipFramesB, SIGNAL(clicked(bool)), forceSkipFramesB, SLOT(setEnabled(bool)));
    if (hurryUpB->isChecked() || !skipFramesB->isChecked())
        forceSkipFramesB->setEnabled(skipFramesB->isChecked());

    QGridLayout *layout = new QGridLayout(this);
    layout->addWidget(demuxerB);
#ifdef QMPlay2_VDPAU
    layout->addWidget(decoderVDPAUB);
    layout->addWidget(decoderVDPAU_NWB);
#endif
#ifdef QMPlay2_VAAPI
    layout->addWidget(decoderVAAPIEB);
#endif
#ifdef QMPlay2_DXVA2
    if (decoderDXVA2EB)
        layout->addWidget(decoderDXVA2EB);
#endif
#ifdef QMPlay2_VTB
    layout->addWidget(decoderVTBEB);
#endif
    layout->addWidget(decoderB);
}

#ifdef QMPlay2_VDPAU
void ModuleSettingsWidget::setVDPAU()
{
    sets().set("VDPAUNoiseReductionEnabled", noisereductionVDPAUB->isChecked());
    sets().set("VDPAUNoiseReductionLvl", noisereductionLvlVDPAUS->value() / 50.0);
    SetInstance<VDPAUWriter>();
}
void ModuleSettingsWidget::checkEnables()
{
    noisereductionLvlVDPAUS->setEnabled(noisereductionVDPAUB->isChecked());
}
#endif

void ModuleSettingsWidget::saveSettings()
{
    sets().set("DemuxerEnabled", demuxerB->isChecked());
    sets().set("ReconnectStreamed", reconnectStreamedB->isChecked());
    sets().set("DecoderEnabled", decoderB->isChecked());
    sets().set("HurryUP", hurryUpB ->isChecked());
    sets().set("SkipFrames", skipFramesB->isChecked());
    sets().set("ForceSkipFrames", forceSkipFramesB->isChecked());
    sets().set("Threads", threadsB->value());
    sets().set("LowresValue", lowresB->currentIndex());
    sets().set("ThreadTypeSlice", thrTypeB->currentIndex());
#ifdef QMPlay2_VDPAU
    sets().set("DecoderVDPAUEnabled", decoderVDPAUB->isChecked());
    sets().set("VDPAUHQScaling", vdpauHQScalingB->currentIndex());
    sets().set("DecoderVDPAU_NWEnabled", decoderVDPAU_NWB->isChecked());
#endif
#ifdef QMPlay2_VAAPI
    sets().set("DecoderVAAPIEnabled", decoderVAAPIEB->isChecked());
    sets().set("UseOpenGLinVAAPI", useOpenGLinVAAPIB->isChecked());
    sets().set("CopyVideoVAAPI", copyVideoVAAPIB->checkState());
#endif
#ifdef QMPlay2_DXVA2
    if (decoderDXVA2EB)
    {
        sets().set("DecoderDXVA2Enabled", decoderDXVA2EB->isChecked());
        sets().set("CopyVideoDXVA2", copyVideoDXVA2->checkState());
    }
#endif
#ifdef QMPlay2_VTB
    sets().set("DecoderVTBEnabled", decoderVTBEB->isChecked());
    sets().set("CopyVideoVTB", copyVideoVTB->checkState());
#endif
}
