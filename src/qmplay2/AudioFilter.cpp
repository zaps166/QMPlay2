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

#include <AudioFilter.hpp>

#include <Module.hpp>

QVector<AudioFilter *> AudioFilter::open()
{
    QVector<AudioFilter *> filterList;
    for (Module *module : QMPlay2Core.getPluginsInstance())
        for (const Module::Info &mod : module->getModulesInfo())
            if (mod.type == Module::AUDIOFILTER)
            {
                AudioFilter *filter = (AudioFilter *)module->createInstance(mod.name);
                if (filter)
                    filterList.append(filter);
            }
    filterList.squeeze();
    return filterList;
}

int AudioFilter::bufferedSamples() const
{
    return 0;
}
void AudioFilter::clearBuffers()
{}
