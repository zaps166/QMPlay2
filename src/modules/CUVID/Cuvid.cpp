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

#include <Cuvid.hpp>
#include <CuvidDec.hpp>

#include <QComboBox>

Cuvid::Cuvid() :
	Module("CUVID")
{
	moduleImg = QImage(":/CUVID");
	moduleImg.setText("Path", ":/CUVID");

	m_cudaLoaded = CuvidDec::loadLibraries();

	init("Enabled", true);
	init("DeintMethod", 2);
	init("CopyVideo", false);

	m_deintMethodB = new QComboBox;
	m_deintMethodB->addItems(QStringList() << tr("None") << "Bob" << tr("Adaptive"));
	m_deintMethodB->setCurrentIndex(getInt("DeintMethod"));
	if (m_deintMethodB->currentIndex() < 0)
		m_deintMethodB->setCurrentIndex(2);
	m_deintMethodB->setProperty("text", tr("Deinterlacing method") + " (CUVID): ");
	m_deintMethodB->setProperty("module", QVariant::fromValue((void *)this));
	QMPlay2Core.addVideoDeintMethod(m_deintMethodB);
}
Cuvid::~Cuvid()
{}

QList<Module::Info> Cuvid::getModulesInfo(const bool showDisabled) const
{
	Q_UNUSED(showDisabled)

	QList<Info> modulesInfo;
	if (showDisabled || (m_cudaLoaded && getBool("Enabled")))
		modulesInfo += Info(CuvidName, DECODER, moduleImg);
	return modulesInfo;
}
void *Cuvid::createInstance(const QString &name)
{
	if (name == CuvidName)
		return new CuvidDec(*this);
	return NULL;
}

Module::SettingsWidget *Cuvid::getSettingsWidget()
{
	return new ModuleSettingsWidget(*this, m_cudaLoaded);
}

void Cuvid::videoDeintSave()
{
	set("DeintMethod", m_deintMethodB->currentIndex());
	setInstance<CuvidDec>();
}

QMPLAY2_EXPORT_PLUGIN(Cuvid)

/**/

#include <QGridLayout>
#include <QCheckBox>
#include <QLabel>

ModuleSettingsWidget::ModuleSettingsWidget(Module &module, bool cudaLoaded) :
	Module::SettingsWidget(module),
	m_cudaLoaded(cudaLoaded)
{
	m_enabledB = new QCheckBox(tr("Decoder enabled"));
	m_enabledB->setChecked(sets().getBool("Enabled"));

	m_copyVideoB = new QCheckBox(tr("Copy decoded video to CPU memory (not recommended)"));
	m_copyVideoB->setChecked(sets().getBool("CopyVideo"));

	QGridLayout *layout = new QGridLayout(this);
	if (!m_cudaLoaded)
		layout->addWidget(new QLabel("<b>" + tr("Can't load CUDA and/or CUVID library!") + "</b>"));
	layout->addWidget(m_enabledB);
	layout->addWidget(m_copyVideoB);
}

void ModuleSettingsWidget::saveSettings()
{
	sets().set("Enabled", m_enabledB->isChecked());
	sets().set("CopyVideo", m_copyVideoB->isChecked());
}
