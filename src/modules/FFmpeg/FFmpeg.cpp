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
	demuxIcon(":/FFDemux")
{
	moduleImg = QImage(":/FFmpeg");
	moduleImg.setText("Path", ":/FFmpeg");

	demuxIcon.setText("Path", ":/FFDemux");
#ifdef QMPlay2_VDPAU
	vdpauIcon = QImage(":/VDPAU");
	vdpauIcon.setText("Path", ":/VDPAU");
#endif
#ifdef QMPlay2_VAAPI
	vaapiIcon = QImage(":/VAAPI");
	vaapiIcon.setText("Path", ":/VAAPI");
#endif

	init("DemuxerEnabled", true);
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
	init("AllowVDPAUinVAAPI", false);
	init("DecoderVAAPIEnabled", true);
	init("VAAPIDeintMethod", 1);
	if (getUInt("VAAPIDeintMethod") > 2)
		set("VAAPIDeintMethod", 1);
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
	vdpauDeintMethodB->addItems(QStringList() << tr("None") << "Temporal" << "Temporal-spatial");
	vdpauDeintMethodB->setCurrentIndex(getInt("VDPAUDeintMethod"));
	if (vdpauDeintMethodB->currentIndex() < 0)
		vdpauDeintMethodB->setCurrentIndex(1);
	vdpauDeintMethodB->setProperty("text", tr("Deinterlacing method") + " (VDPAU): ");
	vdpauDeintMethodB->setProperty("module", self);
	QMPlay2Core.addVideoDeintMethod(vdpauDeintMethodB);
#endif
#if defined(QMPlay2_VAAPI) && defined(HAVE_VPP)
	vaapiDeintMethodB = new QComboBox;
	vaapiDeintMethodB->addItems(QStringList() << tr("None") << "Motion adaptive" << "Motion compensated");
	vaapiDeintMethodB->setCurrentIndex(getInt("VAAPIDeintMethod"));
	if (vaapiDeintMethodB->currentIndex() < 0)
		vaapiDeintMethodB->setCurrentIndex(1);
	vaapiDeintMethodB->setProperty("text", tr("Deinterlacing method") + " (VA-API): ");
	vaapiDeintMethodB->setProperty("module", self);
	QMPlay2Core.addVideoDeintMethod(vaapiDeintMethodB);
#endif

	static bool firstTime = true;
	if (firstTime)
	{
#ifndef QT_DEBUG
		av_log_set_level(AV_LOG_FATAL);
#endif
		av_register_all();
#ifdef QMPlay2_libavdevice
		avdevice_register_all();
#endif
		firstTime = false;
	}
	avformat_network_init();
}
FFmpeg::~FFmpeg()
{
	avformat_network_deinit();
}

QList<FFmpeg::Info> FFmpeg::getModulesInfo(const bool showDisabled) const
{
	QList<Info> modulesInfo;
	if (showDisabled || getBool("DemuxerEnabled"))
		modulesInfo += Info(DemuxerName, DEMUXER, demuxIcon);
	if (showDisabled || getBool("DecoderEnabled"))
		modulesInfo += Info(DecoderName, DECODER, moduleImg);
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
	modulesInfo += Info(FFReaderName, READER, QStringList() << "http" << "https" << "mms" << "rtmp" << "rtsp", moduleImg);
	return modulesInfo;
}
void *FFmpeg::createInstance(const QString &name)
{
	if (name == DemuxerName && getBool("DemuxerEnabled"))
		return new FFDemux(mutex, *this);
	else if (name == DecoderName && getBool("DecoderEnabled"))
		return new FFDecSW(mutex, *this);
#ifdef QMPlay2_VDPAU
	else if (name == DecoderVDPAUName && getBool("DecoderVDPAUEnabled"))
		return new FFDecVDPAU(mutex, *this);
	else if (name == DecoderVDPAU_NWName && getBool("DecoderVDPAU_NWEnabled"))
		return new FFDecVDPAU_NW(mutex, *this);
#endif
#ifdef QMPlay2_VAAPI
	else if (name == DecoderVAAPIName && getBool("DecoderVAAPIEnabled"))
		return new FFDecVAAPI(mutex, *this);
#endif
	else if (name == FFReaderName)
		return new FFReader;
	return NULL;
}

FFmpeg::SettingsWidget *FFmpeg::getSettingsWidget()
{
	return new ModuleSettingsWidget(*this);
}

void FFmpeg::videoDeintSave()
{
#ifdef QMPlay2_VDPAU
	set("VDPAUDeintMethod", vdpauDeintMethodB->currentIndex());
	setInstance<VDPAUWriter>();
#endif
#if defined(QMPlay2_VAAPI) && defined(HAVE_VPP)
	set("VAAPIDeintMethod", vaapiDeintMethodB->currentIndex());
	setInstance<VAAPIWriter>();
#endif
}

QMPLAY2_EXPORT_PLUGIN(FFmpeg)

/**/

#include <Slider.hpp>

#include <QGridLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QLabel>

ModuleSettingsWidget::ModuleSettingsWidget(Module &module) :
	Module::SettingsWidget(module)
{
	demuxerEB = new QCheckBox(tr("Demuxer"));
	demuxerEB->setChecked(sets().getBool("DemuxerEnabled"));

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

	allowVDPAUinVAAPIB = new QCheckBox(tr("Allow VDPAU"));
	allowVDPAUinVAAPIB->setChecked(sets().getBool("AllowVDPAUinVAAPI"));

	QFormLayout *vaapiLayout = new QFormLayout(decoderVAAPIEB);
	vaapiLayout->addRow(allowVDPAUinVAAPIB);
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
	lowresB->addItems(QStringList() << tr("Normal size") << tr("4x smaller") << tr("16x smaller"));
	lowresB->setCurrentIndex(sets().getInt("LowresValue"));
	if (lowresB->currentIndex() < 0)
	{
		lowresB->setCurrentIndex(0);
		sets().set("LowresValue", 0);
	}

	thrTypeB = new QComboBox;
	thrTypeB->addItems(QStringList() << tr("Frames") << tr("Slices"));
	thrTypeB->setCurrentIndex(sets().getInt("ThreadTypeSlice"));
	if (thrTypeB->currentIndex() < 0)
	{
		thrTypeB->setCurrentIndex(0);
		sets().set("ThreadTypeSlice", 0);
	}

	QFormLayout *decoderLayout = new QFormLayout(decoderB);
	decoderLayout->addRow(tr("Number of threads used to decode video") + ": ", threadsB);
	decoderLayout->addRow(tr("Low resolution decoding (only some codecs)") + ": ", lowresB);
	decoderLayout->addRow(tr("Method of multithreaded decoding") + ": ", thrTypeB);
	decoderLayout->addRow(hurryUpB);

	connect(skipFramesB, SIGNAL(clicked(bool)), forceSkipFramesB, SLOT(setEnabled(bool)));
	if (hurryUpB->isChecked() || !skipFramesB->isChecked())
		forceSkipFramesB->setEnabled(skipFramesB->isChecked());

	QGridLayout *layout = new QGridLayout(this);
	layout->addWidget(demuxerEB);
#ifdef QMPlay2_VDPAU
	layout->addWidget(decoderVDPAUB);
	layout->addWidget(decoderVDPAU_NWB);
#endif
#ifdef QMPlay2_VAAPI
	layout->addWidget(decoderVAAPIEB);
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
	sets().set("DemuxerEnabled", demuxerEB->isChecked());
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
	sets().set("AllowVDPAUinVAAPI", allowVDPAUinVAAPIB->isChecked());
	sets().set("DecoderVAAPIEnabled", decoderVAAPIEB->isChecked());
#endif
}
