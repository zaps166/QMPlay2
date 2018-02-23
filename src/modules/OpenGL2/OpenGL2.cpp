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

#include <OpenGL2.hpp>
#include <OpenGL2Writer.hpp>

#include <QGuiApplication>

OpenGL2::OpenGL2() :
	Module("OpenGL2")
{
	m_icon = QIcon(":/OpenGL2.svgz");

	init("Enabled", true);
	init("AllowPBO", true);
	init("HQScaling", Qt::PartiallyChecked);
	const QString platformName = QGuiApplication::platformName();
	init("ForceRtt", (platformName == "cocoa" || platformName == "android"));
	init("VSync", true);
#ifdef Q_OS_WIN
	if (QSysInfo::windowsVersion() >= QSysInfo::WV_6_0)
		init("PreventFullScreen", true);
#endif
}

QList<OpenGL2::Info> OpenGL2::getModulesInfo(const bool showDisabled) const
{
	QList<Info> modulesInfo;
	if (showDisabled || getBool("Enabled"))
		modulesInfo += Info(OpenGL2WriterName, WRITER, QStringList{"video"});
	return modulesInfo;
}
void *OpenGL2::createInstance(const QString &name)
{
	if (name == OpenGL2WriterName && getBool("Enabled"))
		return new OpenGL2Writer(*this);
	return nullptr;
}

OpenGL2::SettingsWidget *OpenGL2::getSettingsWidget()
{
	return new ModuleSettingsWidget(*this);
}

QMPLAY2_EXPORT_MODULE(OpenGL2)

/**/

#include <QGridLayout>
#include <QCheckBox>

ModuleSettingsWidget::ModuleSettingsWidget(Module &module) :
	Module::SettingsWidget(module)
{
	enabledB = new QCheckBox(tr("Enabled"));
	enabledB->setChecked(sets().getBool("Enabled"));

	allowPboB = new QCheckBox(tr("Allow to use PBO (if available)"));
	allowPboB->setChecked(sets().getBool("AllowPBO"));

	hqScalingB = new QCheckBox(tr("High quality video scaling"));
	hqScalingB->setTristate(true);
	hqScalingB->setToolTip(tr("Trilinear filtering for minification and bicubic filtering for magnification.\nPartially checked means that it is used for OpenGL 3.0 or higher."));
	hqScalingB->setCheckState((Qt::CheckState)sets().getWithBounds("HQScaling", Qt::Unchecked, Qt::Checked, Qt::PartiallyChecked));

	forceRttB = new QCheckBox(tr("Force render to texture if possible (not recommended)"));
	forceRttB->setToolTip(tr("Always enabled on Wayland and Android platforms.\nSet visualizations to OpenGL mode if enabled."));
	forceRttB->setChecked(sets().getBool("ForceRtt"));

	vsyncB = new QCheckBox(tr("Vertical sync") +  " (VSync)");
	vsyncB->setChecked(sets().getBool("VSync"));

#ifdef Q_OS_WIN
	if (QSysInfo::windowsVersion() >= QSysInfo::WV_6_0)
	{
		preventFullScreenB = new QCheckBox(tr("Try to prevent exclusive full screen"));
		preventFullScreenB->setChecked(sets().getBool("PreventFullScreen"));
	}
	else
	{
		preventFullScreenB = nullptr;
	}
#endif

	QGridLayout *layout = new QGridLayout(this);
	layout->addWidget(enabledB);
	layout->addWidget(allowPboB);
	layout->addWidget(hqScalingB);
	layout->addWidget(forceRttB);
	layout->addWidget(vsyncB);
#ifdef Q_OS_WIN
	if (preventFullScreenB)
		layout->addWidget(preventFullScreenB);
#endif
}

void ModuleSettingsWidget::saveSettings()
{
	sets().set("Enabled", enabledB->isChecked());
	sets().set("AllowPBO", allowPboB->isChecked());
	sets().set("HQScaling", hqScalingB->checkState());
	sets().set("ForceRtt", forceRttB->isChecked());
	sets().set("VSync", vsyncB->isChecked());
#ifdef Q_OS_WIN
	if (preventFullScreenB)
		sets().set("PreventFullScreen", preventFullScreenB->isChecked());
#endif
}
