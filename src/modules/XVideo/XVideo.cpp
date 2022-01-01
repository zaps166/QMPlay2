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

#include <XVideo.hpp>
#include <XVideoWriter.hpp>

XVideo::XVideo() :
    Module("XVideo")
{
    m_icon = QIcon(":/XVideo.svgz");

    init("Enabled", true);
    init("UseSHM", true);
}

QList<XVideo::Info> XVideo::getModulesInfo(const bool showDisabled) const
{
    QList<Info> modulesInfo;
    if (showDisabled || getBool("Enabled"))
        modulesInfo += Info(XVideoWriterName, WRITER, QStringList{"video"});
    return modulesInfo;
}
void *XVideo::createInstance(const QString &name)
{
    if (name == XVideoWriterName && getBool("Enabled"))
        return new XVideoWriter(*this);
    return nullptr;
}

XVideo::SettingsWidget *XVideo::getSettingsWidget()
{
    return new ModuleSettingsWidget(*this);
}

QMPLAY2_EXPORT_MODULE(XVideo)

/**/

#include <QFormLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>

ModuleSettingsWidget::ModuleSettingsWidget(Module &module) :
    Module::SettingsWidget(module)
{
    enabledB = new QCheckBox(tr("Enabled"));
    enabledB->setChecked(sets().getBool("Enabled"));

    useSHMB = new QCheckBox(tr("Use shared memory"));
    useSHMB->setChecked(sets().getBool("UseSHM"));

    adaptorsB = new QComboBox;
    adaptorsB->addItem(tr("Default"));
    adaptorsB->addItems(XVIDEO::adaptorsList());
    int idx = adaptorsB->findText(sets().getString("Adaptor"));
    adaptorsB->setCurrentIndex(idx < 0 ? 0 : idx);

    QFormLayout *layout = new QFormLayout(this);
    layout->addRow(enabledB);
    layout->addRow(useSHMB);
    layout->addRow(tr("XVideo outputs") + ": ", adaptorsB);
}

void ModuleSettingsWidget::saveSettings()
{
    sets().set("Enabled", enabledB->isChecked());
    sets().set("UseSHM", useSHMB->isChecked());
    sets().set("Adaptor", adaptorsB->currentIndex() > 0 ? adaptorsB->currentText() : QString());
}
