/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2024  Błażej Szczygieł

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
#ifdef QMPlay2_VKVIDEO
#   include <FFDecVkVideo.hpp>
#endif
#ifdef QMPlay2_VAAPI
    #include <FFDecVAAPI.hpp>
#endif
#ifdef QMPlay2_DXVA2
    #include <FFDecDXVA2.hpp>
#endif
#ifdef QMPlay2_D3D11VA
    #include <FFDecD3D11VA.hpp>
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

#include <QOperatingSystemVersion>
#include <QGuiApplication>
#include <QComboBox>

FFmpeg::FFmpeg() :
    Module("FFmpeg"),
    demuxIcon(":/FFDemux.svgz")
{
    m_icon = QIcon(":/FFmpeg.svgz");

#ifdef QMPlay2_VKVIDEO
    vkVideoIcon = QIcon(":/VkVideo.svgz");
    if (QMPlay2Core.isVulkanRenderer())
        vkVideoSupported = true;
#endif
#ifdef QMPlay2_VAAPI
    vaapiIcon = QIcon(":/VAAPI.svgz");
#endif
#if defined(QMPlay2_DXVA2) || defined(QMPlay2_D3D11VA)
    dxIcon = QIcon(":/DXVA2.svgz");
    if (QMPlay2Core.isVulkanRenderer())
        d3d11vaSupported = (QOperatingSystemVersion::current() >= QOperatingSystemVersion::Windows8);
    else
        dxva2Supported = true;
#endif
#ifdef QMPlay2_VTB
    vtbIcon = QIcon(":/VAAPI.svgz");
#endif

    init("DemuxerEnabled", true);
    init("ReconnectNetwork", true);
    init("DecoderEnabled", true);
#ifdef QMPlay2_VKVIDEO
    switch (QOperatingSystemVersion::currentType())
    {
        case QOperatingSystemVersion::Windows:
            init("DecoderVkVideoEnabled", QOperatingSystemVersion::current() >= QOperatingSystemVersion::Windows10);
            break;
        default:
            init("DecoderVkVideoEnabled", true);
            break;
    }
#endif
#ifdef QMPlay2_VAAPI
    init("DecoderVAAPIEnabled", true);
    init("VAAPIDeintMethod", 1);
    if (getUInt("VAAPIDeintMethod") > 2)
        set("VAAPIDeintMethod", 1);
#endif
#ifdef QMPlay2_DXVA2
    init("DecoderDXVA2Enabled", true);
#endif
#ifdef QMPlay2_D3D11VA
    init("DecoderD3D11VAEnabled", true);
    init("DecoderD3D11VAZeroCopy", false);
#endif
#ifdef QMPlay2_VTB
    init("DecoderVTBEnabled", true);
#endif
    init("HurryUP", true);
    init("SkipFrames", true);
    init("ForceSkipFrames", false);
    init("Threads", 0);
    init("LowresValue", 0);
    init("ThreadTypeSlice", false);

#if defined(USE_OPENGL)

#   if defined(QMPlay2_VAAPI)
    const QVariant self = QVariant::fromValue((void *)this);
#   endif

#   if defined(QMPlay2_VAAPI)
    m_vaapiDeintMethodB = new QComboBox;
    m_vaapiDeintMethodB->addItems({tr("None"), "Motion adaptive", "Motion compensated"});
    m_vaapiDeintMethodB->setCurrentIndex(getInt("VAAPIDeintMethod"));
    if (m_vaapiDeintMethodB->currentIndex() < 0)
        m_vaapiDeintMethodB->setCurrentIndex(1);
    if (QMPlay2Core.isVulkanRenderer())
        m_vaapiDeintMethodB->setProperty("text", QString(tr("Deinterlacing method") + " (VA-API, Intel only): "));
    else
        m_vaapiDeintMethodB->setProperty("text", QString(tr("Deinterlacing method") + " (VA-API): "));
    m_vaapiDeintMethodB->setProperty("module", self);
    QMPlay2Core.addVideoDeintMethod(m_vaapiDeintMethodB);
#   endif

#endif

#ifdef QMPlay2_libavdevice
    static bool firstTime = true;
    if (firstTime)
    {
        avdevice_register_all();
        firstTime = false;
    }
#endif
}
FFmpeg::~FFmpeg()
{
#ifdef QMPlay2_VAAPI
    delete m_vaapiDeintMethodB;
#endif
}

QList<FFmpeg::Info> FFmpeg::getModulesInfo(const bool showDisabled) const
{
    QList<Info> modulesInfo;
    if (showDisabled || getBool("DemuxerEnabled"))
        modulesInfo += Info(DemuxerName, DEMUXER, demuxIcon);
    if (showDisabled || getBool("DecoderEnabled"))
        modulesInfo += Info(DecoderName, DECODER, m_icon);
#ifdef QMPlay2_VKVIDEO
    if (showDisabled || (vkVideoSupported && getBool("DecoderVkVideoEnabled")))
        modulesInfo += Info(DecoderVkVideoName, DECODER, vkVideoIcon);
#endif
#ifdef QMPlay2_VAAPI
    if (showDisabled || getBool("DecoderVAAPIEnabled"))
    {
        modulesInfo += Info(DecoderVAAPIName, DECODER, vaapiIcon);
        modulesInfo += Info(VAAPIWriterName, WRITER | VIDEOHWFILTER, vaapiIcon);
    }
#endif
#ifdef QMPlay2_DXVA2
    if (showDisabled || (dxva2Supported && getBool("DecoderDXVA2Enabled")))
        modulesInfo += Info(DecoderDXVA2Name, DECODER, dxIcon);
#endif
#ifdef QMPlay2_D3D11VA
    if (showDisabled || (d3d11vaSupported && getBool("DecoderD3D11VAEnabled")))
        modulesInfo += Info(DecoderD3D11VAName, DECODER, dxIcon);
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
#ifdef QMPlay2_VKVIDEO
    else if (name == DecoderVkVideoName && vkVideoSupported && getBool("DecoderVkVideoEnabled"))
        return new FFDecVkVideo(*this);
#endif
#ifdef QMPlay2_VAAPI
    else if (name == DecoderVAAPIName && getBool("DecoderVAAPIEnabled"))
        return new FFDecVAAPI(*this);
#endif
#ifdef QMPlay2_DXVA2
    else if (name == DecoderDXVA2Name && (dxva2Supported && getBool("DecoderDXVA2Enabled")))
        return new FFDecDXVA2(*this);
#endif
#ifdef QMPlay2_D3D11VA
    else if (name == DecoderD3D11VAName && (d3d11vaSupported && getBool("DecoderD3D11VAEnabled")))
        return new FFDecD3D11VA(*this);
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
    return new ModuleSettingsWidget(*this, vkVideoSupported, dxva2Supported, d3d11vaSupported);
}

void FFmpeg::videoDeintSave()
{
#if defined(QMPlay2_VAAPI)
    if (m_vaapiDeintMethodB)
    {
        set("VAAPIDeintMethod", m_vaapiDeintMethodB->currentIndex());
        setInstance<FFDecVAAPI>();
    }
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

ModuleSettingsWidget::ModuleSettingsWidget(Module &module, bool vkVideo, bool dxva2, bool d3d11va)
    : Module::SettingsWidget(module)
{
    demuxerB = new QGroupBox(tr("Demuxer"));
    demuxerB->setCheckable(true);
    demuxerB->setChecked(sets().getBool("DemuxerEnabled"));

    reconnectNetworkB = new QCheckBox(tr("Try to automatically reconnect the network connection"));
    reconnectNetworkB->setChecked(sets().getBool("ReconnectNetwork"));

    decoderB = new QGroupBox(tr("Software decoder"));
    decoderB->setCheckable(true);
    decoderB->setChecked(sets().getBool("DecoderEnabled"));

#ifdef QMPlay2_VKVIDEO
    if (vkVideo)
    {
        decoderVKVIDEO = new QCheckBox(tr("Decoder") + " VkVideo - " + tr("hardware decoding"));
        decoderVKVIDEO->setChecked(sets().getBool("DecoderVkVideoEnabled"));
    }
#else
    Q_UNUSED(vkVideo)
#endif

#ifdef QMPlay2_VAAPI
    decoderVAAPIEB = new QCheckBox(tr("Decoder") + " VAAPI - " + tr("hardware decoding"));
    decoderVAAPIEB->setChecked(sets().getBool("DecoderVAAPIEnabled"));
#endif

#ifdef QMPlay2_DXVA2
    if (dxva2)
    {
        decoderDXVA2EB = new QCheckBox(tr("Decoder") + " DXVA2 - " + tr("hardware decoding"));
        decoderDXVA2EB->setChecked(sets().getBool("DecoderDXVA2Enabled"));
    }
#else
    Q_UNUSED(dxva2)
#endif
#ifdef QMPlay2_D3D11VA
    if (d3d11va)
    {
        decoderD3D11VA = new QGroupBox(tr("Decoder") + " D3D11VA - " + tr("hardware decoding"));
        decoderD3D11VA->setCheckable(true);
        decoderD3D11VA->setChecked(sets().getBool("DecoderD3D11VAEnabled"));

        d3d11vaZeroCopy = new QCheckBox(tr("Zero-copy decoding on Intel hardware (experimental)"));
        d3d11vaZeroCopy->setToolTip(tr("Better performance, but can cause garbage or might not work at all."));
        d3d11vaZeroCopy->setChecked(sets().getBool("DecoderD3D11VAZeroCopy"));

        auto d3d11vaLayout = new QFormLayout;
        d3d11vaLayout->addWidget(d3d11vaZeroCopy);

        decoderD3D11VA->setLayout(d3d11vaLayout);
    }
#else
    Q_UNUSED(d3d11va)
#endif

#ifdef QMPlay2_VTB
    decoderVTBEB = new QCheckBox(tr("Decoder") + " VideoToolBox - " + tr("hardware decoding"));
    decoderVTBEB->setChecked(sets().getBool("DecoderVTBEnabled"));
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
    demuxerLayout->addRow(nullptr, reconnectNetworkB);

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
#ifdef QMPlay2_VKVIDEO
    if (decoderVKVIDEO)
        layout->addWidget(decoderVKVIDEO);
#endif
#ifdef QMPlay2_VAAPI
    layout->addWidget(decoderVAAPIEB);
#endif
#ifdef QMPlay2_DXVA2
    if (decoderDXVA2EB)
        layout->addWidget(decoderDXVA2EB);
#endif
#ifdef QMPlay2_D3D11VA
    if (decoderD3D11VA)
        layout->addWidget(decoderD3D11VA);
#endif
#ifdef QMPlay2_VTB
    layout->addWidget(decoderVTBEB);
#endif
    layout->addWidget(decoderB);
}

void ModuleSettingsWidget::saveSettings()
{
    sets().set("DemuxerEnabled", demuxerB->isChecked());
    sets().set("ReconnectNetwork", reconnectNetworkB->isChecked());
    sets().set("DecoderEnabled", decoderB->isChecked());
    sets().set("HurryUP", hurryUpB ->isChecked());
    sets().set("SkipFrames", skipFramesB->isChecked());
    sets().set("ForceSkipFrames", forceSkipFramesB->isChecked());
    sets().set("Threads", threadsB->value());
    sets().set("LowresValue", lowresB->currentIndex());
    sets().set("ThreadTypeSlice", thrTypeB->currentIndex());
#ifdef QMPlay2_VKVIDEO
    if (decoderVKVIDEO)
        sets().set("DecoderVkVideoEnabled", decoderVKVIDEO->isChecked());
#endif
#ifdef QMPlay2_VAAPI
    sets().set("DecoderVAAPIEnabled", decoderVAAPIEB->isChecked());
#endif
#ifdef QMPlay2_DXVA2
    if (decoderDXVA2EB)
    {
        sets().set("DecoderDXVA2Enabled", decoderDXVA2EB->isChecked());
    }
#endif
#ifdef QMPlay2_D3D11VA
    if (decoderD3D11VA)
    {
        sets().set("DecoderD3D11VAEnabled", decoderD3D11VA->isChecked());
        sets().set("DecoderD3D11VAZeroCopy", d3d11vaZeroCopy->isChecked());
    }
#endif
#ifdef QMPlay2_VTB
    sets().set("DecoderVTBEnabled", decoderVTBEB->isChecked());
#endif
}
