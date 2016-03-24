#include <XVideo.hpp>
#include <XVideoWriter.hpp>

XVideo::XVideo() :
	Module("XVideo")
{
	moduleImg = QImage(":/Xorg");

	init("Enabled", true);
	init("UseSHM", true);
}

QList< XVideo::Info > XVideo::getModulesInfo(const bool showDisabled) const
{
	QList< Info > modulesInfo;
	if (showDisabled || getBool("Enabled"))
		modulesInfo += Info(XVideoWriterName, WRITER, QStringList("video"));
	return modulesInfo;
}
void *XVideo::createInstance(const QString &name)
{
	if (name == XVideoWriterName && getBool("Enabled"))
		return new XVideoWriter(*this);
	return NULL;
}

XVideo::SettingsWidget *XVideo::getSettingsWidget()
{
	return new ModuleSettingsWidget(*this);
}

QMPLAY2_EXPORT_PLUGIN(XVideo)

/**/

#include <QGridLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>

ModuleSettingsWidget::ModuleSettingsWidget(Module &module) :
	Module::SettingsWidget(module)
{
	enabledB = new QCheckBox(tr("Enabled"));
	enabledB->setChecked(sets().getBool("Enabled"));

	useSHMB = new QCheckBox(tr("Use shared memory"));
	useSHMB->setChecked(sets().getBool("UseSHM"));

	QLabel *adaptorsL = new QLabel(tr("XVideo outputs") + ": ");

	adaptorsB = new QComboBox;
	adaptorsB->addItem(tr("Default"));
	adaptorsB->addItems(XVIDEO::adaptorsList());
	int idx = adaptorsB->findText(sets().getString("Adaptor"));
	adaptorsB->setCurrentIndex(idx < 0 ? 0 : idx);

	QGridLayout *layout = new QGridLayout(this);
	layout->addWidget(enabledB, 0, 0, 1, 2);
	layout->addWidget(useSHMB, 1, 0, 1, 2);
	layout->addWidget(adaptorsL, 2, 0, 1, 1);
	layout->addWidget(adaptorsB, 2, 1, 1, 1);
}

void ModuleSettingsWidget::saveSettings()
{
	sets().set("Enabled", enabledB->isChecked());
	sets().set("UseSHM", useSHMB->isChecked());
	sets().set("Adaptor", adaptorsB->currentIndex() > 0 ? adaptorsB->currentText() : QString());
}
