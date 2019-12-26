/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2019  Błażej Szczygieł

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

#include <Decoder.hpp>

#include <StreamInfo.hpp>
#include <Module.hpp>

class QMPlay2DummyDecoder : public Decoder
{
    QString name() const override
    {
        return QString();
    }

    bool open(StreamInfo &, VideoWriter *) override
    {
        return true;
    }
};

Decoder *Decoder::create(StreamInfo &streamInfo, VideoWriter *writer, const QStringList &modNames, QString *modNameOutput)
{
    if (!streamInfo.must_decode)
    {
        Decoder *decoder = new QMPlay2DummyDecoder;
        decoder->open(streamInfo);
        return decoder;
    }
    QVector<QPair<Module *, Module::Info>> pluginsInstances(modNames.count());
    for (Module *pluginInstance : QMPlay2Core.getPluginsInstance())
        for (const Module::Info &mod : pluginInstance->getModulesInfo())
            if (mod.type == Module::DECODER)
            {
                if (modNames.isEmpty())
                    pluginsInstances += {pluginInstance, mod};
                else
                {
                    const int idx = modNames.indexOf(mod.name);
                    if (idx > -1)
                        pluginsInstances[idx] = {pluginInstance, mod};
                }
            }
    for (int i = 0; i < pluginsInstances.count(); i++)
    {
        Module *module = pluginsInstances[i].first;
        Module::Info &moduleInfo = pluginsInstances[i].second;
        if (!module || moduleInfo.name.isEmpty())
            continue;
        Decoder *decoder = (Decoder *)module->createInstance(moduleInfo.name);
        if (!decoder)
            continue;
        if (decoder->open(streamInfo, writer))
        {
            if (modNameOutput)
                *modNameOutput = moduleInfo.name;
            return decoder;
        }
        delete decoder;
    }
    return nullptr;
}

VideoWriter *Decoder::HWAccel() const
{
    return nullptr;
}

void Decoder::setSupportedPixelFormats(const AVPixelFormats &pixelFormats)
{
    Q_UNUSED(pixelFormats)
}

int Decoder::decodeVideo(Packet &encodedPacket, Frame &decoded, QByteArray &newPixFmt, bool flush, unsigned hurry_up)
{
    Q_UNUSED(encodedPacket)
    Q_UNUSED(decoded)
    Q_UNUSED(newPixFmt)
    Q_UNUSED(flush)
    Q_UNUSED(hurry_up)
    return 0;
}
int Decoder::decodeAudio(Packet &encodedPacket, Buffer &decoded, quint8 &channels, quint32 &sampleRate, bool flush)
{
    Q_UNUSED(channels)
    Q_UNUSED(sampleRate)
    Q_UNUSED(flush)
    return (decoded = encodedPacket).size();
}
bool Decoder::decodeSubtitle(const Packet &encodedPacket, double pos, QMPlay2OSD *&osd, const QSize &size, bool flush)
{
    Q_UNUSED(encodedPacket)
    Q_UNUSED(pos)
    Q_UNUSED(osd)
    Q_UNUSED(size)
    Q_UNUSED(flush)
    return false;
}

int Decoder::pendingFrames() const
{
    return 0;
}

bool Decoder::hasCriticalError() const
{
    return false;
}
