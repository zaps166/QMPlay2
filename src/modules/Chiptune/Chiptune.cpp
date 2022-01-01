/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2022  Błażej Szczygieł

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

#include <Chiptune.hpp>
#ifdef USE_SIDPLAY
    #include <SIDPlay.hpp>
#endif
#ifdef USE_GME
    #include <GME.hpp>
#endif

Chiptune::Chiptune() :
    Module("Chiptune"),
    GMEIcon(":/GME.svgz"), SIDIcon(":/SID.svgz")
{
    m_icon = QIcon(":/Chiptune.svgz");

#ifdef USE_GME
    init("GME", true);
#endif
#ifdef USE_SIDPLAY
    init("SIDPlay", true);
#endif
    init("DefaultLength", 180);
}

QList<Chiptune::Info> Chiptune::getModulesInfo(const bool showDisabled) const
{
    QList<Info> modulesInfo;
#ifdef USE_GME
    if (showDisabled || getBool("GME"))
        modulesInfo += Info(GMEName, DEMUXER, {"ay", "gbs", "gym", "hes", "kss", "nsf", "nsfe", "sap", "spc", "vgm", "vgz"}, GMEIcon);
#endif
#ifdef USE_SIDPLAY
    if (showDisabled || getBool("SIDPlay"))
        modulesInfo += Info(SIDPlayName, DEMUXER, {"sid", "c64", "prg"}, SIDIcon);
#endif
    return modulesInfo;
}
void *Chiptune::createInstance(const QString &name)
{
#ifdef USE_GME
    if (name == GMEName)
        return new GME(*this);
#endif
#ifdef USE_SIDPLAY
    if (name == SIDPlayName)
        return new SIDPlay(*this);
#endif
    return nullptr;
}

Chiptune::SettingsWidget *Chiptune::getSettingsWidget()
{
    return new ModuleSettingsWidget(*this);
}

QMPLAY2_EXPORT_MODULE(Chiptune)

/**/

#include <QGridLayout>
#include <QCheckBox>
#include <QSpinBox>

ModuleSettingsWidget::ModuleSettingsWidget(Module &module) :
    Module::SettingsWidget(module)
{
#ifdef USE_GME
    gmeB = new QCheckBox("Game-Music-Emu " + tr("enabled"));
    gmeB->setChecked(sets().getBool("GME"));
#endif

#ifdef USE_SIDPLAY
    sidB = new QCheckBox("SIDPlay " + tr("enabled"));
    sidB->setChecked(sets().getBool("SIDPlay"));
#endif

    lengthB = new QSpinBox;
    lengthB->setRange(30, 600);
    lengthB->setPrefix(tr("Default length") + ": ");
    lengthB->setSuffix(" " + tr("sec"));
    lengthB->setValue(sets().getInt("DefaultLength"));

    QGridLayout *layout = new QGridLayout(this);
#ifdef USE_GME
    layout->addWidget(gmeB);
#endif
#ifdef USE_SIDPLAY
    layout->addWidget(sidB);
#endif
    layout->addWidget(lengthB);
}

void ModuleSettingsWidget::saveSettings()
{
#ifdef USE_GME
    sets().set("GME", gmeB->isChecked());
#endif
#ifdef USE_SIDPLAY
    sets().set("SIDPlay", sidB->isChecked());
#endif
    sets().set("DefaultLength", lengthB->value());
}
