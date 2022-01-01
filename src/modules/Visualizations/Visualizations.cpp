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

#include <Visualizations.hpp>

#include <SimpleVis.hpp>
#include <FFTSpectrum.hpp>


Visualizations::Visualizations() :
    Module("Visualizations")
{
    m_icon = QIcon(":/Visualizations.svgz");

    constexpr int ms = 17;

    init("RefreshTime", ms);
    init("SimpleVis/SoundLength", ms);
    init("FFTSpectrum/Size", 8);
}

QList<Visualizations::Info> Visualizations::getModulesInfo(const bool) const
{
    QList<Info> modulesInfo;
    modulesInfo += Info(SimpleVisName, QMPLAY2EXTENSION);
    modulesInfo += Info(FFTSpectrumName, QMPLAY2EXTENSION);
    return modulesInfo;
}
void *Visualizations::createInstance(const QString &name)
{
    if (name == SimpleVisName)
        return new SimpleVis(*this);
    else if (name == FFTSpectrumName)
        return new FFTSpectrum(*this);
    return nullptr;
}

Visualizations::SettingsWidget *Visualizations::getSettingsWidget()
{
    return new ModuleSettingsWidget(*this);
}

QMPLAY2_EXPORT_MODULE(Visualizations)

/**/

#include <QFormLayout>
#include <QCheckBox>
#include <QSpinBox>
#include <QLabel>

ModuleSettingsWidget::ModuleSettingsWidget(Module &module) :
    Module::SettingsWidget(module)
{
    if (!QMPlay2Core.isGlOnWindow())
    {
        refTimeB = new QSpinBox;
        refTimeB->setRange(1, 500);
        refTimeB->setSuffix(" " + tr("ms"));
        refTimeB->setValue(sets().getInt("RefreshTime"));
    }

    sndLenB = new QSpinBox;
    sndLenB->setRange(1, 500);
    sndLenB->setSuffix(" " + tr("ms"));
    sndLenB->setValue(sets().getInt("SimpleVis/SoundLength"));

    fftSizeB = new QSpinBox;
    fftSizeB->setRange(5, 12);
    fftSizeB->setPrefix("2^");
    fftSizeB->setValue(sets().getInt("FFTSpectrum/Size"));

    m_fftLinearScaleB = new QCheckBox(tr("Linear scale"));
    m_fftLinearScaleB->setChecked(sets().getBool("FFTSpectrum/LinearScale"));

    QFormLayout *layout = new QFormLayout(this);
    if (refTimeB)
        layout->addRow(tr("Refresh time") + ": ", refTimeB);
    layout->addRow(tr("Displayed sound length") + ": ", sndLenB);
    layout->addRow(tr("FFT spectrum size") + ": ", fftSizeB);
    layout->addRow(m_fftLinearScaleB);

    if (refTimeB)
        connect(refTimeB, SIGNAL(valueChanged(int)), sndLenB, SLOT(setValue(int)));
}

void ModuleSettingsWidget::saveSettings()
{
    if (refTimeB)
        sets().set("RefreshTime", refTimeB->value());
    sets().set("SimpleVis/SoundLength", sndLenB->value());
    sets().set("FFTSpectrum/Size", fftSizeB->value());
    sets().set("FFTSpectrum/LinearScale", m_fftLinearScaleB->isChecked());
}
