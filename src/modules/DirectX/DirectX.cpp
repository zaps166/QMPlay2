#include <DirectX.hpp>
#include <DirectDraw.hpp>

DirectX::DirectX() :
	Module("DirectX")
{
	moduleImg = QImage(":/DirectX");

	init("DirectDrawEnabled", true);
}

QList<DirectX::Info> DirectX::getModulesInfo(const bool showDisabled) const
{
	QList<Info> modulesInfo;
	if (showDisabled || getBool("DirectDrawEnabled"))
		modulesInfo += Info(DirectDrawWriterName, WRITER, QStringList("video"));
	return modulesInfo;
}
void *DirectX::createInstance(const QString &name)
{
	if (name == DirectDrawWriterName && getBool("DirectDrawEnabled"))
		return new DirectDrawWriter(*this);
	return NULL;
}

DirectX::SettingsWidget *DirectX::getSettingsWidget()
{
	return new ModuleSettingsWidget(*this);
}

QMPLAY2_EXPORT_PLUGIN(DirectX)

/**/

#include <QGridLayout>
#include <QCheckBox>

ModuleSettingsWidget::ModuleSettingsWidget(Module &module) :
	Module::SettingsWidget(module)
{
	ddrawB = new QCheckBox(tr("Enabled"));
	ddrawB->setChecked(sets().getBool("DirectDrawEnabled"));

	QGridLayout *layout = new QGridLayout(this);
	layout->addWidget(ddrawB);
}

void ModuleSettingsWidget::saveSettings()
{
	sets().set("DirectDrawEnabled", ddrawB->isChecked());
}
