// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <Notify.hpp>

#include <NotifyExtension.hpp>

Notify::Notify() :
    Module("Notify")
{
    m_icon = QIcon(":/Notify.svgz");

    init("Enabled", false);

    init("Timeout", 5000);

    init("ShowVolume", true);
    init("ShowTitle", true);
    init("ShowPlayState", true);

    init("CustomMsg", false);
    init("CustomSummary", QString());
    init("CustomBody", QString());
}

QList<Notify::Info> Notify::getModulesInfo(const bool) const
{
    return {Info(NotifyExtensionName, QMPLAY2EXTENSION)};
}
void *Notify::createInstance(const QString &name)
{
    if (name == NotifyExtensionName)
        return new NotifyExtension(*this);
    return nullptr;
}

Notify::SettingsWidget *Notify::getSettingsWidget()
{
    return new ModuleSettingsWidget(*this);
}

QMPLAY2_EXPORT_MODULE(Notify)

/**/

ModuleSettingsWidget::ModuleSettingsWidget(Module &module) :
    Module::SettingsWidget(module)
{
    m_notify = new QGroupBox(tr("Additional notifications"));
    m_notify->setCheckable(true);
    m_notify->setChecked(sets().getBool("Enabled"));

    /**/

    m_timeoutSB = new QDoubleSpinBox;
    m_timeoutSB->setDecimals(1);
    m_timeoutSB->setRange(0.0, 3600.0);
    m_timeoutSB->setSingleStep(0.1);
    m_timeoutSB->setSpecialValueText(tr("Infinite"));
    m_timeoutSB->setSuffix(" " + tr("sec"));
    m_timeoutSB->setValue(sets().getInt("Timeout") / 1000.0);

    m_notifyVolumeB = new QCheckBox(tr("Show notification when volume changes"));
    m_notifyVolumeB->setChecked(sets().getBool("ShowVolume"));

    m_notifyTitleB = new QCheckBox(tr("Show notification when media changes"));
    m_notifyTitleB->setChecked(sets().getBool("ShowTitle"));

    m_notifyPlayStateB = new QCheckBox(tr("Show notification when play state changes"));
    m_notifyPlayStateB->setChecked(sets().getBool("ShowPlayState"));

    QFormLayout *generalL = new QFormLayout;
    generalL->addRow(tr("Notification timeout") + ":", m_timeoutSB);
    generalL->addRow(m_notifyVolumeB);
    generalL->addRow(m_notifyTitleB);
    generalL->addRow(m_notifyPlayStateB);

    /**/

    m_customMsgG = new QGroupBox(tr("Use a custom message for media change notifications"));
    m_customMsgG->setCheckable(true);
    m_customMsgG->setChecked(sets().getBool("CustomMsg"));

    m_customSummary = new QLineEdit(sets().getString("CustomSummary"));
    m_customSummary->setPlaceholderText("%title% - %artist%");

    m_customBody = new QLineEdit(sets().getString("CustomBody"));
    m_customBody->setPlaceholderText("%album%");

    QFormLayout *customMsgL = new QFormLayout(m_customMsgG);
    customMsgL->addRow(tr("Summary") + ":", m_customSummary);
    customMsgL->addRow(tr("Body") + ":", m_customBody);

    /**/

    QVBoxLayout *layout = new QVBoxLayout(m_notify);
    layout->addLayout(generalL);
    layout->addWidget(m_customMsgG);
    layout->setContentsMargins(3, 3, 3, 3);

    /**/

    QGridLayout *mainLayout = new QGridLayout(this);
    mainLayout->addWidget(m_notify);
}

void ModuleSettingsWidget::saveSettings()
{
    sets().set("Enabled", m_notify->isChecked());

    sets().set("Timeout", (int)(m_timeoutSB->value() * 1000));
    sets().set("ShowVolume", m_notifyVolumeB->isChecked());
    sets().set("ShowTitle", m_notifyTitleB->isChecked());
    sets().set("ShowPlayState", m_notifyPlayStateB->isChecked());

    sets().set("CustomMsg", m_customMsgG->isChecked());
    sets().set("CustomSummary", m_customSummary->text());
    sets().set("CustomBody", m_customBody->text());
}
