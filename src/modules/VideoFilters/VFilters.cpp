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

#include <VFilters.hpp>
#include <BobDeint.hpp>
#include <YadifDeint.hpp>
#include <BlendDeint.hpp>
#include <DiscardDeint.hpp>
#include <MotionBlur.hpp>

VFilters::VFilters() :
    Module("VideoFilters")
{
    m_icon = QIcon(":/VideoFilters.svgz");
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
    modulesInfo += Info(MotionBlurName, VIDEOFILTER, tr("Produce one extra frame which is average of two neighbour frames"));
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
    else if (name == MotionBlurName)
        return new MotionBlur;
    return nullptr;
}

QMPLAY2_EXPORT_MODULE(VFilters)
