/*
	QMPlay2 is a video and audio player.
	Copyright (C) 2010-2017  Błażej Szczygieł

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

#include <Cuvid.hpp>
#include <CuvidDec.hpp>

#include <QComboBox>

#ifdef Q_OS_WIN
	#include <windows.h>
#endif

Cuvid::Cuvid() :
	Module("CUVID"),
	m_cudaLoaded(-1)
{
	moduleImg = QImage(":/CUVID");
	moduleImg.setText("Path", ":/CUVID");

	init("Enabled", true);
	init("DeintMethod", 2);
	init("CopyVideo", Qt::PartiallyChecked);
#ifdef Q_OS_WIN
	init("CheckFirstGPU", true);
#endif

	m_deintMethodB = new QComboBox;
	m_deintMethodB->addItems(QStringList() << "Bob" << tr("Adaptive"));
	m_deintMethodB->setCurrentIndex(getInt("DeintMethod") - 1);
	if (m_deintMethodB->currentIndex() < 0)
		m_deintMethodB->setCurrentIndex(1);
	m_deintMethodB->setProperty("text", tr("Deinterlacing method") + " (CUVID): ");
	m_deintMethodB->setProperty("module", QVariant::fromValue((void *)this));
	QMPlay2Core.addVideoDeintMethod(m_deintMethodB);
}
Cuvid::~Cuvid()
{
	delete m_deintMethodB;
}

void Cuvid::initCuvidDec()
{
	if (m_cudaLoaded == -1)
	{
#ifdef Q_OS_WIN
		if (getBool("CheckFirstGPU"))
		{
			DISPLAY_DEVICEA displayDev;
			memset(&displayDev, 0, sizeof displayDev);
			displayDev.cb = sizeof displayDev;
			if (EnumDisplayDevicesA(NULL, 0, &displayDev, 0))
			{
				const bool isNvidia = QByteArray::fromRawData(displayDev.DeviceString, sizeof displayDev.DeviceString).toLower().contains("nvidia");
				if (!isNvidia)
				{
					m_cudaLoaded = 0;
					return;
				}
			}
		}
#endif
		m_cudaLoaded = CuvidDec::loadLibrariesAndInit();
	}
}

QList<Module::Info> Cuvid::getModulesInfo(const bool showDisabled) const
{
	Q_UNUSED(showDisabled)

	QList<Info> modulesInfo;
	if (showDisabled || getBool("Enabled"))
		modulesInfo += Info(CuvidName, DECODER, moduleImg);
	return modulesInfo;
}
void *Cuvid::createInstance(const QString &name)
{
	if (name == CuvidName && getBool("Enabled"))
	{
		initCuvidDec();
		if (m_cudaLoaded == 1)
			return new CuvidDec(*this);
	}
	return NULL;
}

Module::SettingsWidget *Cuvid::getSettingsWidget()
{
	return new ModuleSettingsWidget(*this);
}

void Cuvid::videoDeintSave()
{
	set("DeintMethod", m_deintMethodB->currentIndex() + 1);
	setInstance<CuvidDec>();
}

QMPLAY2_EXPORT_PLUGIN(Cuvid)

/**/

#include <QGridLayout>
#include <QCheckBox>
#include <QLabel>

ModuleSettingsWidget::ModuleSettingsWidget(Module &module) :
	Module::SettingsWidget(module)
{
	m_enabledB = new QCheckBox(tr("Decoder enabled"));
	m_enabledB->setChecked(sets().getBool("Enabled"));

	m_copyVideoB = new QCheckBox(tr("Copy decoded video to CPU memory (not recommended)"));
	m_copyVideoB->setTristate();
	m_copyVideoB->setCheckState((Qt::CheckState)sets().getInt("CopyVideo"));
	m_copyVideoB->setToolTip(tr("Partially checked means that it will copy a video data only if the fast method fails"));

#ifdef Q_OS_WIN
	m_checkFirstGPU = new QCheckBox(tr("Use CUVID only when primary GPU is NVIDIA"));
	m_checkFirstGPU->setChecked(sets().getBool("CheckFirstGPU"));
#endif

	connect(m_enabledB, SIGNAL(clicked(bool)), m_copyVideoB, SLOT(setEnabled(bool)));
	m_copyVideoB->setEnabled(m_enabledB->isChecked());

	QGridLayout *layout = new QGridLayout(this);
	layout->addWidget(m_enabledB);
	layout->addWidget(m_copyVideoB);
#ifdef Q_OS_WIN
	layout->addWidget(m_checkFirstGPU);
#endif
}

void ModuleSettingsWidget::saveSettings()
{
	sets().set("Enabled", m_enabledB->isChecked());
	sets().set("CopyVideo", m_copyVideoB->checkState());
#ifdef Q_OS_WIN
	sets().set("CheckFirstGPU", m_checkFirstGPU->isChecked());
#endif
}
