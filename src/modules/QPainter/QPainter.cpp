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

#include <QPainter.hpp>
#include <QPainterWriter.hpp>

QPainter_Qt::QPainter_Qt() :
	Module("QPainter_Qt")
{
	moduleImg = QImage(":/Qt");

	init("Enabled", true);
}

QList<QPainter_Qt::Info> QPainter_Qt::getModulesInfo(const bool showDisabled) const
{
	QList<Info> modulesInfo;
	if (showDisabled || getBool("Enabled"))
		modulesInfo += Info(QPainterWriterName, WRITER, QStringList("video"));
	return modulesInfo;
}
void *QPainter_Qt::createInstance(const QString &name)
{
	if (name == QPainterWriterName && getBool("Enabled"))
		return new QPainterWriter(*this);
	return NULL;
}

QPainter_Qt::SettingsWidget *QPainter_Qt::getSettingsWidget()
{
	return new ModuleSettingsWidget(*this);
}

QMPLAY2_EXPORT_PLUGIN(QPainter_Qt)

/**/

#include <QGridLayout>
#include <QCheckBox>

ModuleSettingsWidget::ModuleSettingsWidget(Module &module) :
	Module::SettingsWidget(module)
{
	enabledB = new QCheckBox(tr("Enabled"));
	enabledB->setChecked(sets().getBool("Enabled"));

	QGridLayout *layout = new QGridLayout(this);
	layout->addWidget(enabledB);
}

void ModuleSettingsWidget::saveSettings()
{
	sets().set("Enabled", enabledB->isChecked());
}
