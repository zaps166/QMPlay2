/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2021  Błażej Szczygieł

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

#include <Subtitles.hpp>
#include <SRT.hpp>
#include <Classic.hpp>
#include <Settings.hpp>

Subtitles::Subtitles() :
    Module("Subtitles")
{
    m_icon = QIcon(":/Subtitles.svgz");

    init("SRT_enabled", true);
    init("Classic_enabled", true);
    init("Use_mDVD_FPS", true);
    init("Sub_max_s", 5.0);
}

QList<Subtitles::Info> Subtitles::getModulesInfo(const bool showDisabled) const
{
    QList<Info> modulesInfo;
    if (showDisabled || getBool("SRT_enabled"))
        modulesInfo += Info(SRTSubsName, SUBSDEC, QStringList{"srt", "vtt", "webvtt"});
    if (showDisabled || getBool("Classic_enabled"))
        modulesInfo += Info(ClassicSubsName, SUBSDEC, QStringList{"sub", "txt", "tmp"});
    return modulesInfo;
}
void *Subtitles::createInstance(const QString &name)
{
    if (name == SRTSubsName && getBool("SRT_enabled"))
        return new SRT;
    else if (name == ClassicSubsName && getBool("Classic_enabled"))
        return new Classic(getBool("Use_mDVD_FPS"), getDouble("Sub_max_s"));
    return nullptr;
}

Subtitles::SettingsWidget *Subtitles::getSettingsWidget()
{
    return new ModuleSettingsWidget(*this);
}

QMPLAY2_EXPORT_MODULE(Subtitles)

/**/

#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QCheckBox>
#include <QLabel>

ModuleSettingsWidget::ModuleSettingsWidget(Module &module) :
    Module::SettingsWidget(module)
{
    srtEB = new QCheckBox(tr("SRT reading"));
    srtEB->setChecked(sets().getBool("SRT_enabled"));

    classicEB = new QCheckBox(tr("Classic subtitles reading"));
    classicEB->setChecked(sets().getBool("Classic_enabled"));

    mEB = new QCheckBox(tr("Use the specified FPS in MicroDVD subtitles (if exists)"));
    mEB->setChecked(sets().getBool("Use_mDVD_FPS"));

    QLabel *maxLenL = new QLabel(tr("The maximum duration of subtitles without a specified length") + ": ");

    maxLenB = new QDoubleSpinBox;
    maxLenB->setRange(0.5, 9.5);
    maxLenB->setDecimals(1);
    maxLenB->setSingleStep(0.1);
    maxLenB->setSuffix(" " + tr("sec"));
    maxLenB->setValue(sets().getDouble("Sub_max_s"));

    QGridLayout *layout = new QGridLayout(this);
    layout->addWidget(srtEB, 0, 0, 1, 2);
    layout->addWidget(classicEB, 1, 0, 1, 2);
    layout->addWidget(mEB, 2, 0, 1, 2);
    layout->addWidget(maxLenL, 3, 0, 1, 1);
    layout->addWidget(maxLenB, 3, 1, 1, 1);
}

void ModuleSettingsWidget::saveSettings()
{
    sets().set("SRT_enabled", srtEB->isChecked());
    sets().set("Classic_enabled", classicEB->isChecked());
    sets().set("Use_mDVD_FPS", mEB->isChecked());
    sets().set("Sub_max_s", maxLenB->value());
}
