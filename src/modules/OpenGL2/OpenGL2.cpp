#include <OpenGL2.hpp>
#include <OpenGL2Writer.hpp>

OpenGL2::OpenGL2() :
	Module("OpenGL2")
{
	moduleImg = QImage(":/OpenGL2");

	init("Enabled", true);
#ifdef OPENGL_NEW_API
	init("ForceRtt", false);
#endif
#ifdef VSYNC_SETTINGS
	init("VSync", true);
#endif
}

QList<OpenGL2::Info> OpenGL2::getModulesInfo(const bool showDisabled) const
{
	QList<Info> modulesInfo;
	if (showDisabled || getBool("Enabled"))
		modulesInfo += Info(OpenGL2WriterName, WRITER, QStringList("video"));
	return modulesInfo;
}
void *OpenGL2::createInstance(const QString &name)
{
	if (name == OpenGL2WriterName && getBool("Enabled"))
		return new OpenGL2Writer(*this);
	return NULL;
}

OpenGL2::SettingsWidget *OpenGL2::getSettingsWidget()
{
	return new ModuleSettingsWidget(*this);
}

QMPLAY2_EXPORT_PLUGIN(OpenGL2)

/**/

#include <QGridLayout>
#include <QCheckBox>

ModuleSettingsWidget::ModuleSettingsWidget(Module &module) :
	Module::SettingsWidget(module)
{
	enabledB = new QCheckBox(tr("Enabled"));
	enabledB->setChecked(sets().getBool("Enabled"));

#ifdef OPENGL_NEW_API
	forceRttB = new QCheckBox(tr("Force render to texture if possible (not recommended)"));
	forceRttB->setChecked(sets().getBool("ForceRtt"));
#endif

#ifdef VSYNC_SETTINGS
	vsyncB = new QCheckBox(tr("Vertical sync") +  " (VSync)");
	vsyncB->setChecked(sets().getBool("VSync"));
#endif

	QGridLayout *layout = new QGridLayout(this);
	layout->addWidget(enabledB);
#ifdef OPENGL_NEW_API
	layout->addWidget(forceRttB);
#endif
#ifdef VSYNC_SETTINGS
	layout->addWidget(vsyncB);
#endif
}

void ModuleSettingsWidget::saveSettings()
{
	sets().set("Enabled", enabledB->isChecked());
#ifdef OPENGL_NEW_API
	sets().set("ForceRtt", forceRttB->isChecked());
#endif
#ifdef VSYNC_SETTINGS
	sets().set("VSync", vsyncB->isChecked());
#endif
}
