/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2023  Błażej Szczygieł

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

#include <VFilters.hpp>
#include <BobDeint.hpp>
#include <YadifDeint.hpp>
#include <BlendDeint.hpp>
#include <DiscardDeint.hpp>
#include <FPSDoubler.hpp>
#ifdef MOTION_BLUR
#   include <MotionBlur.hpp>
#endif

VFilters::VFilters() :
    Module("VideoFilters")
{
    m_icon = QIcon(":/VideoFilters.svgz");

    init("FPSDoubler/MinFPS", 21.000);
    init("FPSDoubler/MaxFPS", 29.990);
    init("FPSDoubler/OnlyFullScreen", true);

    connect(&QMPlay2Core, &QMPlay2CoreClass::fullScreenChanged,
            this, [this](bool fs) {
        m_fullScreen = fs;
    });
}

QList<VFilters::Info> VFilters::getModulesInfo(const bool) const
{
    QList<Info> modulesInfo;
    modulesInfo += Info(BobDeintName, VIDEOFILTER | DEINTERLACE | DOUBLER);
    modulesInfo += Info(Yadif2xDeintName, VIDEOFILTER | DEINTERLACE | DOUBLER, YadifDescr);
    modulesInfo += Info(Yadif2xNoSpatialDeintName, VIDEOFILTER | DEINTERLACE | DOUBLER, YadifDescr);
    modulesInfo += Info(YadifDeintName, VIDEOFILTER | DEINTERLACE, YadifDescr);
    modulesInfo += Info(BlendDeintName, VIDEOFILTER | DEINTERLACE);
    modulesInfo += Info(DiscardDeintName, VIDEOFILTER | DEINTERLACE);
    modulesInfo += Info(YadifNoSpatialDeintName, VIDEOFILTER | DEINTERLACE, YadifDescr);
    modulesInfo += Info(FPSDoublerName, VIDEOFILTER | DATAPRESERVE, tr("Doubles the frame rate. Useful to get into the FreeSync range. This filter works with hardware accelerated videos."));
#ifdef MOTION_BLUR
    modulesInfo += Info(MotionBlurName, VIDEOFILTER, tr("Produce one extra frame which is average of two neighbour frames"));
#endif
    return modulesInfo;
}
void *VFilters::createInstance(const QString &name)
{
    if (name == BobDeintName)
        return new BobDeint;
    else if (name == Yadif2xDeintName)
        return new YadifDeint(true, true);
    else if (name == Yadif2xNoSpatialDeintName)
        return new YadifDeint(true, false);
    else if (name == BlendDeintName)
        return new BlendDeint;
    else if (name == DiscardDeintName)
        return new DiscardDeint;
    else if (name == YadifDeintName)
        return new YadifDeint(false, true);
    else if (name == YadifNoSpatialDeintName)
        return new YadifDeint(false, false);
    else if (name == FPSDoublerName)
        return new FPSDoubler(*this, m_fullScreen);
#ifdef MOTION_BLUR
    else if (name == MotionBlurName)
        return new MotionBlur;
#endif
    return nullptr;
}

Module::SettingsWidget *VFilters::getSettingsWidget()
{
    return new ModuleSettingsWidget(*this);
}

QMPLAY2_EXPORT_MODULE(VFilters)

/**/

#include <QGridLayout>
#include <QFormLayout>
#include <QCheckBox>
#include <QGroupBox>
#include <QSpinBox>

ModuleSettingsWidget::ModuleSettingsWidget(Module &module)
    : Module::SettingsWidget(module)
    , m_minFpsSpinBox(new QDoubleSpinBox)
    , m_maxFpsSpinBox(new QDoubleSpinBox)
    , m_onlyFullScreenCheckBox(new QCheckBox(tr("Only in full screen")))
{
    m_minFpsSpinBox->setDecimals(3);
    m_maxFpsSpinBox->setDecimals(3);

    m_minFpsSpinBox->setRange(10.0, 100.0);
    m_maxFpsSpinBox->setRange(20.0, 200.0);

    m_minFpsSpinBox->setSuffix(" " + tr("FPS"));
    m_maxFpsSpinBox->setSuffix(" " + tr("FPS"));

    m_minFpsSpinBox->setToolTip(tr("Minimum video FPS to double the frame rate"));
    m_maxFpsSpinBox->setToolTip(tr("Maximum video FPS to double the frame rate"));

    m_minFpsSpinBox->setValue(sets().getDouble("FPSDoubler/MinFPS"));
    m_maxFpsSpinBox->setValue(sets().getDouble("FPSDoubler/MaxFPS"));

    m_onlyFullScreenCheckBox->setChecked(sets().getBool("FPSDoubler/OnlyFullScreen"));

    auto fpsDoublerLayout = new QFormLayout;
    fpsDoublerLayout->addRow(tr("Minimum:"), m_minFpsSpinBox);
    fpsDoublerLayout->addRow(tr("Maximum:"), m_maxFpsSpinBox);
    fpsDoublerLayout->addRow(m_onlyFullScreenCheckBox);

    auto fpsDoublerGroup = new QGroupBox(FPSDoublerName);
    fpsDoublerGroup->setLayout(fpsDoublerLayout);

    auto layout = new QGridLayout(this);
    layout->addWidget(fpsDoublerGroup);
}

void ModuleSettingsWidget::saveSettings()
{
    double min = m_minFpsSpinBox->value();
    double max = m_maxFpsSpinBox->value();
    if (min < max)
    {
        sets().set("FPSDoubler/MinFPS", min);
        sets().set("FPSDoubler/MaxFPS", max);
    }
    sets().set("FPSDoubler/OnlyFullScreen", m_onlyFullScreenCheckBox->isChecked());
}
