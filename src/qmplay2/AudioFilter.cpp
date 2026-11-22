// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

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
