// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

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
