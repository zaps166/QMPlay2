/*
	QMPlay2 is a video and audio player.
	Copyright (C) 2010-2016  Błażej Szczygieł

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

#include <Notifies.hpp>
#include <Settings.hpp>
#include <NotifyExtension.hpp>

Notifies::Notifies() :
	Module("Notifies")
{
//	moduleImg = QImage(":/???"); // TODO: add icon

	init("TypeDisabled", false);
#ifdef Q_OS_LINUX
	init("TypeNative", true);
	init("TypeTray", false);
#else
	init("TypeTray", true);
#endif

	init("timeout", 5000);

	init("ShowVolume", true);
	init("ShowTitle", true);
	init("ShowPlayState", true);

	init("CustomMsg", false);
	init("CustomSummary", QString());
	init("CustomBody", QString());
}

QList<Notifies::Info> Notifies::getModulesInfo(const bool) const
{
	return QList<Info>()
			<< Info(NotifyExtensionName, QMPLAY2EXTENSION);
}

void *Notifies::createInstance(const QString &name)
{
	if (name == NotifyExtensionName)
		return static_cast<QMPlay2Extensions *>(new NotifyExtension(*this));
	return NULL;
}

Notifies::SettingsWidget *Notifies::getSettingsWidget()
{
	return new ModuleSettingsWidget(*this);
}

QMPLAY2_EXPORT_PLUGIN(Notifies)

/**/

#include <QBoxLayout>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QRadioButton>

ModuleSettingsWidget::ModuleSettingsWidget(Module &module) :
	Module::SettingsWidget(module)
{
	QGroupBox *notifyTypeG = new QGroupBox(tr("Notification Type"));
	QVBoxLayout *notifyTypeL = new QVBoxLayout(notifyTypeG);

	m_disabledR = new QRadioButton(tr("Disabled"));
	m_disabledR->setChecked(sets().getBool("TypeDisabled"));
	notifyTypeL->addWidget(m_disabledR);

#ifdef Q_OS_LINUX
	m_nativeR = new QRadioButton(tr("Show a Native Desktop Notification"));
	m_nativeR->setChecked(sets().getBool("TypeNative"));
	notifyTypeL->addWidget(m_nativeR);
#endif

	m_trayR = new QRadioButton(tr("Show a Popup from the System Tray"));
	m_trayR->setChecked(sets().getBool("TypeTray"));
	notifyTypeL->addWidget(m_trayR);

	/**/

	QGroupBox *generalG = new QGroupBox(tr("General Settings"));
	generalG->setDisabled(sets().getBool("TypeDisabled"));
	connect(m_disabledR, SIGNAL(toggled(bool)), generalG, SLOT(setDisabled(bool)));

	m_timeoutSB = new QDoubleSpinBox;
	m_timeoutSB->setDecimals(4);
	m_timeoutSB->setRange(0.0, 3600.0);
	m_timeoutSB->setSingleStep(0.1);
	m_timeoutSB->setSpecialValueText(tr("Infinite"));
	m_timeoutSB->setSuffix(" " + tr("sec"));
	m_timeoutSB->setValue(sets().getInt("timeout") / 1000.0);

	m_notifyVolumeB = new QCheckBox(tr("Show notification when volume changes"));
	m_notifyVolumeB->setChecked(sets().getBool("ShowVolume"));

	m_notifyTitleB = new QCheckBox(tr("Show notification when media changes"));
	m_notifyTitleB->setChecked(sets().getBool("ShowTitle"));

	m_notifyPlayStateB = new QCheckBox(tr("Show notification when play state changes"));
	m_notifyPlayStateB->setChecked(sets().getBool("ShowPlayState"));

	QFormLayout *generalL = new QFormLayout(generalG);
	generalL->addRow(tr("Notification Timeout") + ":", m_timeoutSB);
	generalL->addRow(m_notifyVolumeB);
	generalL->addRow(m_notifyTitleB);
	generalL->addRow(m_notifyPlayStateB);

	/**/

	m_customMsgG = new QGroupBox(tr("Use a custom message for media change notifications"));
	m_customMsgG->setCheckable(true);
	m_customMsgG->setChecked(sets().getBool("CustomMsg"));
	m_customMsgG->setDisabled(sets().getBool("TypeDisabled"));
	connect(m_disabledR, SIGNAL(toggled(bool)), m_customMsgG, SLOT(setDisabled(bool)));

	m_customSummary = new QLineEdit(sets().getString("CustomSummary"));
	m_customSummary->setPlaceholderText("%title% - %artist%");

	m_customBody = new QLineEdit(sets().getString("CustomBody"));
	m_customBody->setPlaceholderText("%album%");

	QFormLayout *customMsgL = new QFormLayout(m_customMsgG);
	customMsgL->addRow(tr("Summary") + ":", m_customSummary);
	customMsgL->addRow(tr("Body") + ":", m_customBody);

	/**/

	QGridLayout *layout = new QGridLayout(this);
	layout->addWidget(notifyTypeG);
	layout->addWidget(generalG);
	layout->addWidget(m_customMsgG);
}

void ModuleSettingsWidget::saveSettings()
{
	const bool disabled = m_disabledR->isChecked();

	sets().set("TypeDisabled", disabled);
#ifdef Q_OS_LINUX
	sets().set("TypeNative", m_nativeR->isChecked());
#endif
	sets().set("TypeTray", m_trayR->isChecked());

	sets().set("timeout", (int)(m_timeoutSB->value() * 1000));
	sets().set("ShowVolume", m_notifyVolumeB->isChecked());
	sets().set("ShowTitle", m_notifyTitleB->isChecked());
	sets().set("ShowPlayState", m_notifyPlayStateB->isChecked());

	sets().set("CustomMsg", m_customMsgG->isChecked());
	sets().set("CustomSummary", m_customSummary->text());
	sets().set("CustomBody", m_customBody->text());
}
