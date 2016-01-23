#include <Modplug.hpp>
#include <MPDemux.hpp>

Modplug::Modplug() :
	Module("Modplug")
{
	init("ModplugEnabled", true);
	init("ModplugResamplingMethod", 3);
}

QList< Modplug::Info > Modplug::getModulesInfo(const bool showDisabled) const
{
	QList< Info > modulesInfo;
	if (showDisabled || getBool("ModplugEnabled"))
		modulesInfo += Info(DemuxerName, DEMUXER, QStringList() << "669" << "amf" << "ams" << "dbm" << "dmf" << "dsm" << "far" << "it" << "j2b" << "mdl" << "med" << "mod" << "mt2" << "mtm" << "okt" << "psm" << "ptm" << "s3m" << "stm" << "ult" << "umx" << "xm" << "sfx");
	return modulesInfo;
}
void *Modplug::createInstance(const QString &name)
{
	if (name == DemuxerName && getBool("ModplugEnabled"))
		return new MPDemux(*this);
	return NULL;
}

Modplug::SettingsWidget *Modplug::getSettingsWidget()
{
	return new ModuleSettingsWidget(*this);
}

QMPLAY2_EXPORT_PLUGIN(Modplug)

/**/

#include <QGridLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>

ModuleSettingsWidget::ModuleSettingsWidget(Module &module) :
	Module::SettingsWidget(module)
{
	enabledB = new QCheckBox("Modplug " + tr("enabled"));
	enabledB->setChecked(sets().getBool("ModplugEnabled"));

	QLabel *resamplingL = new QLabel(tr("Resampling method") + ": ");

	resamplingB = new QComboBox;
	resamplingB->addItems(QStringList() << "Nearest" << "Linear" << "Spline" << "FIR");
	resamplingB->setCurrentIndex(sets().getInt("ModplugResamplingMethod"));
	if (resamplingB->currentIndex() < 0)
	{
		resamplingB->setCurrentIndex(3);
		sets().set("ModplugResamplingMethod", 3);
	}

	QGridLayout *layout = new QGridLayout(this);
	layout->addWidget(enabledB, 0, 0, 1, 2);
	layout->addWidget(resamplingL, 1, 0, 1, 1);
	layout->addWidget(resamplingB, 1, 1, 1, 1);
}

void ModuleSettingsWidget::saveSettings()
{
	sets().set("ModplugEnabled", enabledB->isChecked());
	sets().set("ModplugResamplingMethod", resamplingB->currentIndex());
}
