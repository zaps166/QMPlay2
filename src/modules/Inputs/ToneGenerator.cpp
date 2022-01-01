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

#include <ToneGenerator.hpp>

#include <Functions.hpp>
#include <Packet.hpp>

#include <QUrlQuery>

#include <cmath>

ToneGenerator::ToneGenerator(Module &module) :
    aborted(false), metadata_changed(false), fromUrl(false), pos(0.0), srate(0)
{
    SetModule(module);
}

bool ToneGenerator::set()
{
    bool restartPlaying = false;
    if (!fromUrl)
    {
        const QStringList newFreqs = sets().getString("ToneGenerator/freqs").split(',');
        restartPlaying = freqs.size() && (srate != sets().getUInt("ToneGenerator/srate") || freqs.size() != newFreqs.size());
        if (!restartPlaying)
        {
            srate = sets().getUInt("ToneGenerator/srate");
            if (!freqs.size())
                freqs.resize(qMin(8, newFreqs.size()));
            else
                metadata_changed = true;
            for (int i = 0; i < freqs.size(); ++i)
                freqs[i] = newFreqs[i].toInt();
        }
    }
    return !restartPlaying;
}

bool ToneGenerator::metadataChanged() const
{
    if (metadata_changed)
    {
        metadata_changed = false;
        return true;
    }
    return false;
}

QString ToneGenerator::name() const
{
    return tr("Tone generator");
}
QString ToneGenerator::title() const
{
    QString t;
    for (quint32 hz : freqs)
        t += "   - " + QString::number(hz) + tr("Hz") + "\n";
    t.chop(1);
    return tr("Tone generator") + " (" + QString::number(srate) + tr("Hz") + "):\n" + t;
}
double ToneGenerator::length() const
{
    return -1;
}
int ToneGenerator::bitrate() const
{
    return 8 * (srate * freqs.size() * sizeof(float)) / 1000;
}

bool ToneGenerator::dontUseBuffer() const
{
    return true;
}

bool ToneGenerator::seek(double, bool)
{
    return false;
}
bool ToneGenerator::read(Packet &decoded, int &idx)
{
    if (aborted)
        return false;

    int chn = freqs.size();

    decoded.resize(sizeof(float) * chn * srate);
    float *samples = (float *)decoded.data();

    for (quint32 i = 0; i < srate * chn; i += chn)
        for (int c = 0; c < chn; ++c)
            samples[i + c] = sin(2.0 * M_PI * freqs[c] * i / srate / chn); //don't use sinf()!

    idx = 0;
    decoded.setTS(pos);
    decoded.setDuration(1.0);
    pos += decoded.duration();

    return true;
}
void ToneGenerator::abort()
{
    aborted = true;
}

bool ToneGenerator::open(const QString &entireUrl)
{
    QString prefix, _url;
    if (!Functions::splitPrefixAndUrlIfHasPluginPrefix(entireUrl, &prefix, &_url) || prefix != ToneGeneratorName)
        return false;

    const QUrl url("?" + _url);

    if (!(fromUrl = url.toString() != "?"))
    {
        streams_info += new StreamInfo(srate, freqs.size());
        return true;
    }

    srate = QUrlQuery(url).queryItemValue("samplerate").toUInt();
    if (!srate)
        srate = 44100;

    freqs.clear();
    for (const QString &freq : QUrlQuery(url).queryItemValue("freqs").split(',', QString::SkipEmptyParts))
        freqs += freq.toInt();
    if (freqs.isEmpty())
    {
        bool ok;
        quint32 freq = url.toString().remove('?').toUInt(&ok);
        if (ok)
            freqs += freq;
        else
            freqs += 440;
    }

    if (freqs.size() <= 8)
    {
        streams_info += new StreamInfo(srate, freqs.size());
        return true;
    }
    return false;
}
