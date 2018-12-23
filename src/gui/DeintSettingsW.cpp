/*
	QMPlay2 is a video and audio player.
	Copyright (C) 2010-2018  Błażej Szczygieł

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

#include <DeintSettingsW.hpp>

#include <Settings.hpp>
#include <Module.hpp>
#include <Main.hpp>

#include <QFormLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QSet>

void DeintSettingsW::init()
{
	Settings &QMPSettings = QMPlay2Core.getSettings();
	QMPSettings.init("Deinterlace/ON", true);
	QMPSettings.init("Deinterlace/Auto", true);
	QMPSettings.init("Deinterlace/Doubler", true);
	QMPSettings.init("Deinterlace/AutoParity", true);
	QMPSettings.init("Deinterlace/SoftwareMethod", "Bob");
	QMPSettings.init("Deinterlace/TFF", false);
}

DeintSettingsW::DeintSettingsW()
{
	Settings &QMPSettings = QMPlay2Core.getSettings();

	setCheckable(true);
	setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));

	setChecked(QMPSettings.getBool("Deinterlace/ON"));
	setTitle(tr("Remove interlacing"));

	autoDeintB = new QCheckBox(tr("Automatically detect interlaced frames"));
	autoDeintB->setChecked(QMPSettings.getBool("Deinterlace/Auto"));

	doublerB = new QCheckBox(tr("Doubles the the number of frames per second"));
	doublerB->setChecked(QMPSettings.getBool("Deinterlace/Doubler"));
	connect(doublerB, SIGNAL(clicked(bool)), this, SLOT(softwareMethods(bool)));

	autoParityB = new QCheckBox(tr("Automatically detect parity"));
	autoParityB->setChecked(QMPSettings.getBool("Deinterlace/AutoParity"));

	softwareMethodsCB = new QComboBox;

	parityCB = new QComboBox;
	parityCB->addItems({"Bottom field first", "Top field first"});
	parityCB->setCurrentIndex(QMPSettings.getBool("Deinterlace/TFF"));

	connect(softwareMethodsCB, SIGNAL(currentIndexChanged(int)), this, SLOT(setSoftwareMethodsToolTip(int)));
	softwareMethods(doublerB->isChecked());

	QFormLayout *layout = new QFormLayout(this);
	layout->addRow(autoDeintB);
	layout->addRow(doublerB);
	layout->addRow(autoParityB);
	layout->addRow(tr("Deinterlacing method") + " (" + tr("software decoding") + "): ", softwareMethodsCB);
	for (QWidget *w : QMPlay2Core.getVideoDeintMethods())
	{
		const QVariant text = w->property("text");
		layout->addRow(text.isNull() ? QString() : text.toString(), w);
	}
	layout->addRow(tr("Parity (if not detected automatically)") + ": ", parityCB);
}
DeintSettingsW::~DeintSettingsW()
{
	for (QObject *obj : children())
	{
		if (QWidget *w = qobject_cast<QWidget *>(obj))
		{
			if (!w->property("module").isNull())
				w->setParent(nullptr);
		}
	}
}

void DeintSettingsW::writeSettings()
{
	Settings &QMPSettings = QMPlay2Core.getSettings();
	QMPSettings.set("Deinterlace/ON", isChecked());
	QMPSettings.set("Deinterlace/Auto", autoDeintB->isChecked());
	QMPSettings.set("Deinterlace/Doubler", doublerB->isChecked());
	QMPSettings.set("Deinterlace/AutoParity", autoParityB->isChecked());
	QMPSettings.set("Deinterlace/SoftwareMethod", softwareMethodsCB->currentText());
	QMPSettings.set("Deinterlace/TFF", (bool)parityCB->currentIndex());

	QSet<Module *> videoDeintModules;
	for (QObject *obj : children())
		if (obj->isWidgetType() && !obj->property("module").isNull())
			videoDeintModules.insert((Module *)obj->property("module").value<void *>());
	for (Module *module : asConst(videoDeintModules))
		module->videoDeintSave();
}

void DeintSettingsW::softwareMethods(bool doubler)
{
	softwareMethodsCB->clear();
	for (Module *module : QMPlay2Core.getPluginsInstance())
		for (const Module::Info &mod : module->getModulesInfo())
			if ((mod.type & 0xF) == Module::VIDEOFILTER && (mod.type & Module::DEINTERLACE) && (doubler == (bool)(mod.type & Module::DOUBLER)))
				softwareMethodsCB->addItem(mod.name, mod.description);
	const int idx = softwareMethodsCB->findText(QMPlay2Core.getSettings().getString("Deinterlace/SoftwareMethod"));
	softwareMethodsCB->setCurrentIndex(idx < 0 ? 0 : idx);
}
void DeintSettingsW::setSoftwareMethodsToolTip(int idx)
{
	softwareMethodsCB->setToolTip(softwareMethodsCB->itemData(idx).toString());
}
