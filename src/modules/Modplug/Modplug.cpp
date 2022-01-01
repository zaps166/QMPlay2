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

#include <Modplug.hpp>
#include <MPDemux.hpp>

Modplug::Modplug() :
    Module("Modplug"),
    modIcon(":/MOD.svgz")
{
    m_icon = QIcon(":/Modplug.svgz");

    init("ModplugEnabled", true);
    init("ModplugResamplingMethod", 3);
}

QList<Modplug::Info> Modplug::getModulesInfo(const bool showDisabled) const
{
    QList<Info> modulesInfo;
    if (showDisabled || getBool("ModplugEnabled"))
        modulesInfo += Info(DemuxerName, DEMUXER, QStringList{"669", "amf", "ams", "dbm", "dmf", "dsm", "far", "it", "j2b", "mdl", "med", "mod", "mt2", "mtm", "okt", "psm", "ptm", "s3m", "stm", "ult", "umx", "xm", "sfx"}, modIcon);
    return modulesInfo;
}
void *Modplug::createInstance(const QString &name)
{
    if (name == DemuxerName && getBool("ModplugEnabled"))
        return new MPDemux(*this);
    return nullptr;
}

Modplug::SettingsWidget *Modplug::getSettingsWidget()
{
    return new ModuleSettingsWidget(*this);
}

QMPLAY2_EXPORT_MODULE(Modplug)

/**/

#include <QFormLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>

ModuleSettingsWidget::ModuleSettingsWidget(Module &module) :
    Module::SettingsWidget(module)
{
    enabledB = new QCheckBox("Modplug " + tr("enabled"));
    enabledB->setChecked(sets().getBool("ModplugEnabled"));

    resamplingB = new QComboBox;
    resamplingB->addItems({"Nearest", "Linear", "Spline", "FIR"});
    resamplingB->setCurrentIndex(sets().getInt("ModplugResamplingMethod"));
    if (resamplingB->currentIndex() < 0)
    {
        resamplingB->setCurrentIndex(3);
        sets().set("ModplugResamplingMethod", 3);
    }

    QFormLayout *layout = new QFormLayout(this);
    layout->addRow(enabledB);
    layout->addRow(tr("Resampling method") + ": ", resamplingB);
}

void ModuleSettingsWidget::saveSettings()
{
    sets().set("ModplugEnabled", enabledB->isChecked());
    sets().set("ModplugResamplingMethod", resamplingB->currentIndex());
}
