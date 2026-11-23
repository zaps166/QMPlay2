// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <Playlists.hpp>
#include <PLS.hpp>
#include <M3U.hpp>
#include <XSPF.hpp>

Playlists::Playlists() :
    Module("Playlists")
{
    m_icon = QIcon(":/Playlists.svgz");

    init("M3U_enabled", true);
    init("XSPF_enabled", true);
}

QList<Playlists::Info> Playlists::getModulesInfo(const bool showDisabled) const
{
    QList<Info> modulesInfo;
    modulesInfo += Info(PLSName, PLAYLIST, QStringList{"pls"});
    if (showDisabled || getBool("M3U_enabled"))
        modulesInfo += Info(M3UName, PLAYLIST, QStringList{"m3u", "m3u8"});
    if (showDisabled || getBool("XSPF_enabled"))
        modulesInfo += Info(XSPFName, PLAYLIST, QStringList{"xspf"});
    return modulesInfo;
}
void *Playlists::createInstance(const QString &name)
{
    if (name == PLSName)
        return new PLS;
    else if (name == M3UName && getBool("M3U_enabled"))
        return new M3U;
    else if (name == XSPFName && getBool("XSPF_enabled"))
        return new XSPF;
    return nullptr;
}

Playlists::SettingsWidget *Playlists::getSettingsWidget()
{
    return new ModuleSettingsWidget(*this);
}

QMPLAY2_EXPORT_MODULE(Playlists)

/**/

ModuleSettingsWidget::ModuleSettingsWidget(Module &module) :
    Module::SettingsWidget(module)
{
    m3uEnabledB = new QCheckBox(tr("M3U support"));
    m3uEnabledB->setChecked(sets().getBool("M3U_enabled"));

    xspfEnabledB = new QCheckBox(tr("XSPF support"));
    xspfEnabledB->setChecked(sets().getBool("XSPF_enabled"));

    QGridLayout *layout = new QGridLayout(this);
    layout->addWidget(m3uEnabledB);
    layout->addWidget(xspfEnabledB);
}

void ModuleSettingsWidget::saveSettings()
{
    sets().set("M3U_enabled", m3uEnabledB->isChecked());
    sets().set("XSPF_enabled", xspfEnabledB->isChecked());
}
