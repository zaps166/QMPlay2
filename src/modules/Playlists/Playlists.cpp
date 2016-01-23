#include <Playlists.hpp>
#include <PLS.hpp>
#include <M3U.hpp>

Playlists::Playlists() :
	Module("Playlists")
{
	init("PLS_enabled", true);
	init("M3U_enabled", true);
}

QList< Playlists::Info > Playlists::getModulesInfo(const bool showDisabled) const
{
	QList< Info > modulesInfo;
	if (showDisabled || getBool("PLS_enabled"))
		modulesInfo += Info(PLSName, PLAYLIST, QStringList("pls"));
	if (showDisabled || getBool("M3U_enabled"))
		modulesInfo += Info(M3UName, PLAYLIST, QStringList("m3u"));
	return modulesInfo;
}
void *Playlists::createInstance(const QString &name)
{
	if (name == PLSName && getBool("PLS_enabled"))
		return new PLS;
	else if (name == M3UName && getBool("M3U_enabled"))
		return new M3U;
	return NULL;
}

Playlists::SettingsWidget *Playlists::getSettingsWidget()
{
	return new ModuleSettingsWidget(*this);
}

QMPLAY2_EXPORT_PLUGIN(Playlists)

/**/

#include <QGridLayout>
#include <QCheckBox>

ModuleSettingsWidget::ModuleSettingsWidget(Module &module) :
	Module::SettingsWidget(module)
{
	plsEnabledB = new QCheckBox(tr("PLS support"));
	plsEnabledB->setChecked(sets().getBool("PLS_enabled"));

	m3uEnabledB = new QCheckBox(tr("M3U support"));
	m3uEnabledB->setChecked(sets().getBool("M3U_enabled"));

	QGridLayout *layout = new QGridLayout(this);
	layout->addWidget(plsEnabledB);
	layout->addWidget(m3uEnabledB);
}

void ModuleSettingsWidget::saveSettings()
{
	sets().set("PLS_enabled", plsEnabledB->isChecked());
	sets().set("M3U_enabled", m3uEnabledB->isChecked());
}
