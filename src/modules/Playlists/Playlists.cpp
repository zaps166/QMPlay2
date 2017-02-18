/*
	QMPlay2 is a video and audio player.
	Copyright (C) 2010-2017  Błażej Szczygieł

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

#include <Playlists.hpp>
#include <PLS.hpp>
#include <M3U.hpp>
#include <XSPF.hpp>

Playlists::Playlists() :
	Module("Playlists")
{
	moduleImg = QImage(":/Playlists");

	init("PLS_enabled", true);
	init("M3U_enabled", true);
	init("XSPF_enabled", true);
	init("Relative", true);
}

QList<Playlists::Info> Playlists::getModulesInfo(const bool showDisabled) const
{
	QList<Info> modulesInfo;
	if (showDisabled || getBool("PLS_enabled"))
		modulesInfo += Info(PLSName, PLAYLIST, QStringList("pls"));
	if (showDisabled || getBool("M3U_enabled"))
		modulesInfo += Info(M3UName, PLAYLIST, QStringList("m3u"));
	if (showDisabled || getBool("XSPF_enabled"))
		modulesInfo += Info(XSPFName, PLAYLIST, QStringList("xspf"));
	return modulesInfo;
}
void *Playlists::createInstance(const QString &name)
{
	const bool useRalative = getBool("Relative");
	if (name == PLSName && getBool("PLS_enabled"))
		return new PLS(useRalative);
	else if (name == M3UName && getBool("M3U_enabled"))
		return new M3U(useRalative);
	else if (name == XSPFName && getBool("XSPF_enabled"))
		return new XSPF(useRalative);
	return NULL;
}

Playlists::SettingsWidget *Playlists::getSettingsWidget()
{
	return new ModuleSettingsWidget(*this);
}

QMPLAY2_EXPORT_PLUGIN(Playlists)

/**/

#include <QGridLayout>
#include <QBoxLayout>
#include <QCheckBox>

ModuleSettingsWidget::ModuleSettingsWidget(Module &module) :
	Module::SettingsWidget(module)
{
	m_plsEnabledB = new QCheckBox(tr("PLS support"));
	m_plsEnabledB->setChecked(sets().getBool("PLS_enabled"));

	m_m3uEnabledB = new QCheckBox(tr("M3U support"));
	m_m3uEnabledB->setChecked(sets().getBool("M3U_enabled"));

	m_xspfEnabledB = new QCheckBox(tr("XSPF support"));
	m_xspfEnabledB->setChecked(sets().getBool("XSPF_enabled"));

	m_relativeB = new QCheckBox(tr("Try to use relative path when possible"));
	m_relativeB->setChecked(sets().getBool("Relative"));

	QGridLayout *layout = new QGridLayout(this);
	layout->addWidget(m_plsEnabledB);
	layout->addWidget(m_m3uEnabledB);
	layout->addWidget(m_xspfEnabledB);
	layout->addWidget(m_relativeB);
}

void ModuleSettingsWidget::saveSettings()
{
	sets().set("PLS_enabled", m_plsEnabledB->isChecked());
	sets().set("M3U_enabled", m_m3uEnabledB->isChecked());
	sets().set("XSPF_enabled", m_xspfEnabledB->isChecked());
	sets().set("Relative", m_relativeB->isChecked());
}
