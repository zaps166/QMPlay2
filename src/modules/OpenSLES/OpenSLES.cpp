#include <OpenSLES.hpp>
#include <OpenSLESWriter.hpp>

OpenSLES::OpenSLES() :
	Module("OpenSLES")
{
	moduleImg = QImage(":/OpenSLES");

	init("WriterEnabled", true);
}

QList< OpenSLES::Info > OpenSLES::getModulesInfo(const bool showDisabled) const
{
	QList< Info > modulesInfo;
	if (showDisabled || getBool("WriterEnabled"))
		modulesInfo += Info(OpenSLESWriterName, WRITER, QStringList("audio"));
	return modulesInfo;
}
void *OpenSLES::createInstance(const QString &name)
{
	if (name == OpenSLESWriterName && getBool("WriterEnabled"))
		return new OpenSLESWriter(*this);
	return NULL;
}

OpenSLES::SettingsWidget *OpenSLES::getSettingsWidget()
{
	return new ModuleSettingsWidget(*this);
}

QMPLAY2_EXPORT_PLUGIN(OpenSLES)

/**/

#include <QGridLayout>
#include <QCheckBox>

ModuleSettingsWidget::ModuleSettingsWidget(Module &module) :
	Module::SettingsWidget(module)
{
	openslesB = new QCheckBox(tr("ON"));
	openslesB->setChecked(sets().getBool("WriterEnabled"));

	QGridLayout *layout = new QGridLayout(this);
	layout->addWidget(openslesB);
}

void ModuleSettingsWidget::saveSettings()
{
	sets().set("WriterEnabled", openslesB->isChecked());
}
