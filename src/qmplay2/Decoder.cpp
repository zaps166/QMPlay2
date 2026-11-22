// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <Decoder.hpp>

#include <StreamInfo.hpp>
#include <Module.hpp>

class QMPlay2DummyDecoder : public Decoder
{
    QString name() const override
    {
        return QString();
    }

    bool open(StreamInfo &) override
    {
        return true;
    }

    int decodeVideo(const Packet &encodedPacket, Frame &decoded, AVPixelFormat &newPixFmt, bool flush, unsigned hurry_up) override
    {
        Q_UNUSED(newPixFmt)
        Q_UNUSED(flush)
        Q_UNUSED(hurry_up)
        if (encodedPacket.hasFrame())
        {
            decoded = encodedPacket.frame();
            decoded.setTS(encodedPacket.ts());
            return encodedPacket.size();
        }
        return 0;
    }

    int decodeAudio(const Packet &encodedPacket, QByteArray &decoded, double &ts, quint8 &channels, quint32 &sampleRate, bool flush) override
    {
        Q_UNUSED(channels)
        Q_UNUSED(sampleRate)
        Q_UNUSED(flush)
        decoded = QByteArray::fromRawData((const char *)encodedPacket.data(), encodedPacket.size());
        ts = encodedPacket.ts();
        return decoded.size();
    }

    bool isDummyDecoder() const override
    {
        return true;
    }
};

Decoder *Decoder::create(
    StreamInfo &streamInfo,
    const QStringList &modNames,
    QString *modNameOutput)
{
    auto maybeRestoreCodecName = [&] {
        if (!streamInfo.codec_name_backup.isEmpty())
        {
            streamInfo.codec_name = streamInfo.codec_name_backup;
            streamInfo.codec_name_backup.clear();
        }
    };
    if (!streamInfo.must_decode)
    {
        Decoder *decoder = new QMPlay2DummyDecoder;
        maybeRestoreCodecName();
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
        maybeRestoreCodecName();
        if (decoder->open(streamInfo))
        {
            if (modNameOutput)
                *modNameOutput = moduleInfo.name;
            return decoder;
        }
        delete decoder;
        maybeRestoreCodecName();
    }
    return nullptr;
}

bool Decoder::hasHWDecContext() const
{
    return false;
}
std::shared_ptr<VideoFilter> Decoder::hwAccelFilter() const
{
    return nullptr;
}

QByteArray Decoder::subtitleHeader() const
{
    return QByteArray();
}

void Decoder::setSupportedPixelFormats(const AVPixelFormats &pixelFormats)
{
    Q_UNUSED(pixelFormats)
}

int Decoder::decodeVideo(const Packet &encodedPacket, Frame &decoded, AVPixelFormat &newPixFmt, bool flush, unsigned hurry_up)
{
    Q_UNUSED(encodedPacket)
    Q_UNUSED(decoded)
    Q_UNUSED(newPixFmt)
    Q_UNUSED(flush)
    Q_UNUSED(hurry_up)
    return 0;
}
int Decoder::decodeAudio(const Packet &encodedPacket, QByteArray &decoded, double &ts, quint8 &channels, quint32 &sampleRate, bool flush)
{
    Q_UNUSED(encodedPacket)
    Q_UNUSED(decoded)
    Q_UNUSED(ts)
    Q_UNUSED(channels)
    Q_UNUSED(sampleRate)
    Q_UNUSED(flush)
    return 0;
}
bool Decoder::decodeSubtitle(const QVector<Packet> &encodedPackets, double pos, std::shared_ptr<QMPlay2OSD> &osd, const QSize &size, bool flush)
{
    Q_UNUSED(encodedPackets)
    Q_UNUSED(pos)
    Q_UNUSED(osd)
    Q_UNUSED(size)
    Q_UNUSED(flush)
    return false;
}
QList<QByteArray> Decoder::decodeSubtitle(const Packet &encodedPacket)
{
    Q_UNUSED(encodedPacket)
    return {};
}

int Decoder::pendingFrames() const
{
    return 0;
}

bool Decoder::hasCriticalError() const
{
    return false;
}

bool Decoder::isDummyDecoder() const
{
    return false;
}
