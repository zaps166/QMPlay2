/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2024  Błażej Szczygieł

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
    init("FFTSpectrum/LimitFreq", 20000);
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
#include <QComboBox>
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

    const auto freqStr = tr("%1 kHz");
    m_fftLimitFreqB = new QComboBox;
    m_fftLimitFreqB->addItems({
        tr("No limit"),
        freqStr.arg(20),
        freqStr.arg(22),
    });
    m_fftLimitFreqB->setItemData(0, int(0));
    m_fftLimitFreqB->setItemData(1, 20000);
    m_fftLimitFreqB->setItemData(2, 22000);
    switch (int freq = sets().getInt("FFTSpectrum/LimitFreq"))
    {
        case 20000:
            m_fftLimitFreqB->setCurrentIndex(1);
            break;
        case 22000:
            m_fftLimitFreqB->setCurrentIndex(2);
            break;
        default:
            if (freq > 0)
            {
                m_fftLimitFreqB->addItem(freqStr.arg(freq / 1000.0));
                m_fftLimitFreqB->setCurrentIndex(3);
            }
            else
            {
                m_fftLimitFreqB->setCurrentIndex(0);
            }
            break;
    }

    m_fftLinearScaleB = new QCheckBox(tr("Linear volume scale in FFT spectrum"));
    m_fftLinearScaleB->setChecked(sets().getBool("FFTSpectrum/LinearScale"));

    QFormLayout *layout = new QFormLayout(this);
    if (refTimeB)
        layout->addRow(tr("Refresh time") + ": ", refTimeB);
    layout->addRow(tr("Displayed sound length") + ": ", sndLenB);
    layout->addRow(tr("FFT spectrum size") + ": ", fftSizeB);
    layout->addRow(tr("Limit frequency in FFT spectrum"), m_fftLimitFreqB);
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
    sets().set("FFTSpectrum/LimitFreq", m_fftLimitFreqB->currentData().toInt());
}
