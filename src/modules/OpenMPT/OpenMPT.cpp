/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2026  Błażej Szczygieł

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

#include <OpenMPT.hpp>
#include <OpenMPTDemux.hpp>

#include <libopenmpt/libopenmpt.hpp>

OpenMPT::OpenMPT()
    : Module("OpenMPT")
{
    m_icon = QIcon(":/OpenMPT.svg");

    init("Enabled", true);
    init("SubsongMode", QVariant(2)); // Groups
    init("StereoSeparation", QVariant(100));
    init("InterpolationFilter", QVariant(0));
    init("VisMode", QVariant(0)); // Off
    init("VisChannels", QVariant(0)); // All
    init("VisRows", QVariant(16));
}

QList<OpenMPT::Info> OpenMPT::getModulesInfo(const bool showDisabled) const
{
    QList<Info> modulesInfo;
    if (showDisabled || getBool("Enabled"))
    {
        const auto supportedExts = openmpt::get_supported_extensions();
        QStringList extensions;
        extensions.reserve(supportedExts.size());
        for (const auto &ext : supportedExts)
            extensions += QString::fromStdString(ext);
        modulesInfo += Info(DemuxerName, DEMUXER, extensions, m_modIcon);
    }
    return modulesInfo;
}
void *OpenMPT::createInstance(const QString &name)
{
    if (name == DemuxerName && getBool("Enabled"))
        return new OpenMPDemux(*this);
    return nullptr;
}
OpenMPT::SettingsWidget *OpenMPT::getSettingsWidget()
{
    return new ModuleSettingsWidget(*this);
}

QMPLAY2_EXPORT_MODULE(OpenMPT)

/**/

#include <QFormLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QSpinBox>

ModuleSettingsWidget::ModuleSettingsWidget(Module &module)
    : Module::SettingsWidget(module)
{
    m_enabled = new QCheckBox(tr("OpenMPT enabled"));
    m_enabled->setChecked(sets().getBool("Enabled"));

    m_subsongMode = new QComboBox;
    m_subsongMode->addItem(tr("Only first"));
    m_subsongMode->addItem(tr("All sequentially"));
    m_subsongMode->addItem(tr("Group"));
    m_subsongMode->setCurrentIndex(sets().getInt("SubsongMode"));

    m_stereoSeparation = new QSpinBox;
    m_stereoSeparation->setRange(0, 200);
    m_stereoSeparation->setValue(sets().getInt("StereoSeparation"));
    m_stereoSeparation->setSuffix(QStringLiteral("%"));

    m_interpolationFilter = new QComboBox;
    m_interpolationFilter->addItem(tr("Default"), 0);
    m_interpolationFilter->addItem(tr("Nearest"), 1);
    m_interpolationFilter->addItem(tr("Linear"), 2);
    m_interpolationFilter->addItem(tr("Cubic"), 4);
    m_interpolationFilter->addItem(tr("Windowed sinc (8 taps)"), 8);
    for (int i = 0; i < m_interpolationFilter->count(); ++i)
    {
        if (m_interpolationFilter->itemData(i).toInt() == sets().getInt("InterpolationFilter"))
        {
            m_interpolationFilter->setCurrentIndex(i);
            break;
        }
    }

    m_visMode = new QComboBox;
    m_visMode->addItem(tr("Off"));
    m_visMode->addItem(tr("Patterns"));
    m_visMode->addItem(tr("Samples"));
    m_visMode->setCurrentIndex(sets().getInt("VisMode"));

    m_visChannels = new QSpinBox;
    m_visChannels->setRange(0, 64);
    m_visChannels->setValue(sets().getInt("VisChannels"));
    m_visChannels->setSpecialValueText(tr("All"));

    m_visRows = new QSpinBox;
    m_visRows->setRange(1, 64);
    m_visRows->setValue(sets().getInt("VisRows"));

    m_visSettingsWidget = new QWidget;
    auto *visSettingsLayout = new QFormLayout(m_visSettingsWidget);
    visSettingsLayout->addRow(tr("Channels:"), m_visChannels);
    visSettingsLayout->addRow(tr("Rows:"), m_visRows);

    auto updateVisSettingsVisibility = [this](int index) {
        m_visSettingsWidget->setVisible(index == 1);
    };
    connect(m_visMode, qOverload<int>(&QComboBox::currentIndexChanged), this, updateVisSettingsVisibility);
    updateVisSettingsVisibility(m_visMode->currentIndex());

    auto *layout = new QFormLayout(this);
    layout->addRow(m_enabled);
    layout->addRow(tr("Subsongs:"), m_subsongMode);
    layout->addRow(tr("Stereo separation:"), m_stereoSeparation);
    layout->addRow(tr("Interpolation filter:"), m_interpolationFilter);
    layout->addRow(tr("Visualization:"), m_visMode);
    layout->addRow(m_visSettingsWidget);
}

void ModuleSettingsWidget::saveSettings()
{
    sets().set("Enabled", m_enabled->isChecked());
    sets().set("SubsongMode", m_subsongMode->currentIndex());
    sets().set("StereoSeparation", m_stereoSeparation->value());
    sets().set("InterpolationFilter", m_interpolationFilter->currentData().toInt());
    sets().set("VisMode", m_visMode->currentIndex());
    sets().set("VisChannels", m_visChannels->value());
    sets().set("VisRows", m_visRows->value());
}
