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

#include <Cuvid.hpp>
#include <CuvidDec.hpp>

#include <QComboBox>

Cuvid::Cuvid() :
    Module("CUVID")
{
    m_icon = QIcon(":/CUVID.svgz");

    init("Enabled", true);
    init("DeintMethod", 2);
    init("DecodeMPEG4", true);
#ifdef Q_OS_WIN
    init("CheckFirstGPU", true);
#endif

    m_deintMethodB = new QComboBox;
    m_deintMethodB->addItems({"Bob", tr("Adaptive")});
    m_deintMethodB->setCurrentIndex(getInt("DeintMethod") - 1);
    if (m_deintMethodB->currentIndex() < 0)
        m_deintMethodB->setCurrentIndex(1);
    m_deintMethodB->setProperty("text", QString(tr("Deinterlacing method") + " (CUVID): "));
    m_deintMethodB->setProperty("module", QVariant::fromValue((void *)this));
    QMPlay2Core.addVideoDeintMethod(m_deintMethodB);
}
Cuvid::~Cuvid()
{
    delete m_deintMethodB;
}

QList<Module::Info> Cuvid::getModulesInfo(const bool showDisabled) const
{
    QList<Info> modulesInfo;
    if (showDisabled || getBool("Enabled"))
        modulesInfo += Info(CuvidName, DECODER, m_icon);
    return modulesInfo;
}
void *Cuvid::createInstance(const QString &name)
{
    if (name == CuvidName && getBool("Enabled"))
    {
        if (CuvidDec::canCreateInstance())
            return new CuvidDec(*this);
    }
    return nullptr;
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

QMPLAY2_EXPORT_MODULE(Cuvid)

/**/

#include <QGridLayout>
#include <QCheckBox>
#include <QLabel>

ModuleSettingsWidget::ModuleSettingsWidget(Module &module) :
    Module::SettingsWidget(module)
{
    m_enabledB = new QCheckBox(tr("Decoder enabled"));
    m_enabledB->setChecked(sets().getBool("Enabled"));

    m_decodeMPEG4 = new QCheckBox(tr("Decode MPEG4 videos"));
    m_decodeMPEG4->setChecked(sets().getBool("DecodeMPEG4"));
    m_decodeMPEG4->setToolTip(tr("Disable if you have problems with decoding MPEG4 (DivX5) videos"));

#ifdef Q_OS_WIN
    m_checkFirstGPU = new QCheckBox(tr("Use CUVID only when primary GPU is NVIDIA"));
    m_checkFirstGPU->setChecked(sets().getBool("CheckFirstGPU"));
#endif

    QGridLayout *layout = new QGridLayout(this);
    layout->addWidget(m_enabledB);
    layout->addWidget(m_decodeMPEG4);
#ifdef Q_OS_WIN
    layout->addWidget(m_checkFirstGPU);
#endif
}

void ModuleSettingsWidget::saveSettings()
{
    sets().set("Enabled", m_enabledB->isChecked());
    sets().set("DecodeMPEG4", m_decodeMPEG4->isChecked());
#ifdef Q_OS_WIN
    sets().set("CheckFirstGPU", m_checkFirstGPU->isChecked());
#endif
}
