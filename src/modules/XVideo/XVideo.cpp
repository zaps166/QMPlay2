// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <XVideo.hpp>
#include <XVideoWriter.hpp>

#include <QGuiApplication>

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
    if (showDisabled || (QGuiApplication::platformName() == "xcb" && getBool("Enabled")))
        modulesInfo += Info(XVideoWriterName, WRITER, QStringList{"video"});
    return modulesInfo;
}
void *XVideo::createInstance(const QString &name)
{
    if (name == XVideoWriterName && QGuiApplication::platformName() == "xcb" && getBool("Enabled"))
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
