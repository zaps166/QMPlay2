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

#include <MPDemux.hpp>

#include <Functions.hpp>
#include <Packet.hpp>
#include <Reader.hpp>

#include <libmodplug/libmodplug.hpp>

MPDemux::MPDemux(Module &module) :
    aborted(false),
    pos(0.0),
    srate(Functions::getBestSampleRate()),
    mpfile(nullptr)
{
    SetModule(module);
}

MPDemux::~MPDemux()
{
    if (mpfile)
        QMPlay2ModPlug::Unload(mpfile);
}

bool MPDemux::set()
{
    bool restartPlaying = false;

    QMPlay2ModPlug::Settings settings;
    QMPlay2ModPlug::GetSettings(&settings);
    if (settings.mResamplingMode != sets().getInt("ModplugResamplingMethod"))
    {
        settings.mResamplingMode = sets().getInt("ModplugResamplingMethod");
        restartPlaying = true;
    }
    settings.mFlags = QMPlay2ModPlug::ENABLE_OVERSAMPLING;
    settings.mChannels = 2;
    settings.mBits = 32;
    settings.mFrequency = srate;
    QMPlay2ModPlug::SetSettings(&settings);

    return !restartPlaying && sets().getBool("ModplugEnabled");
}

QString MPDemux::name() const
{
    switch (QMPlay2ModPlug::GetModuleType(mpfile))
    {
        case 0x01:
            return "ProTracker MOD";
        case 0x02:
            return "ScreamTracker S3M";
        case 0x04:
            return "FastTracker XM";
        case 0x08:
            return "OctaMED";
        case 0x10:
            return "Multitracker MTM";
        case 0x20:
            return "ImpulseTracker IT";
        case 0x40:
            return "UNIS Composer 669";
        case 0x80:
            return "UltraTracker ULT";
        case 0x100:
            return "ScreamTracker STM";
        case 0x200:
            return "Farandole Composer FAR";
        case 0x800:
        case 0x200000:
            return "Advanced Module File AMF";
        case 0x1000:
            return "Extreme Tracker Module AMS";
        case 0x2000:
            return "Digital Sound Module DSM";
        case 0x4000:
            return "DigiTrakker Module MDL";
        case 0x8000:
            return "Oktalyzer Module OKT";
        case 0x20000:
            return "Delusion Digital Music File DMF";
        case 0x40000:
            return "PolyTracker Module PTM";
        case 0x80000:
            return "DigiBooster Pro DBM";
        case 0x100000:
            return "MadTracker MT2";
        case 0x400000:
            return "Protracker Studio Module PSM";
        case 0x800000:
            return "Jazz Jackrabbit 2 Music J2B";
        case 0x1000000:
            return "Amiga SoundFX";
    }
    return "?";
}
QString MPDemux::title() const
{
    return QMPlay2ModPlug::GetName(mpfile);
}

QList<QMPlay2Tag> MPDemux::tags() const
{
    QList<QMPlay2Tag> tags;
    tags += {QString::number(QMPLAY2_TAG_TITLE), QString(QMPlay2ModPlug::GetName(mpfile))};
    tags += {tr("Samples"), QString::number(QMPlay2ModPlug::NumSamples(mpfile))};
    tags += {tr("Patterns"), QString::number(QMPlay2ModPlug::NumPatterns(mpfile))};
    tags += {tr("Channels"), QString::number(QMPlay2ModPlug::NumChannels(mpfile))};
    return tags;
}
double MPDemux::length() const
{
    return QMPlay2ModPlug::GetLength(mpfile) / 1000.0;
}
int MPDemux::bitrate() const
{
    return -1;
}

bool MPDemux::seek(double val, bool)
{
    const double len = length();
    if (val >= len)
        val = len - 1.0;
    QMPlay2ModPlug::Seek(mpfile, val * 1000);
    pos = val;
    return true;
}
bool MPDemux::read(Packet &decoded, int &idx)
{
    if (aborted)
        return false;

    decoded.resize(1024 * 2 * 4); //BASE_SIZE * CHN * BITS/8
    decoded.resize(QMPlay2ModPlug::Read(mpfile, decoded.data(), decoded.size()));
    if (!decoded.size())
        return false;

    //Konwersja 32bit-int na 32bit-float
    float *decodedFloat = (float *)decoded.data();
    const int *decodedInt = (const int *)decodedFloat;
    for (unsigned i = 0; i < decoded.size() / sizeof(float); ++i)
        decodedFloat[i] = decodedInt[i] / 2147483648.0;

    idx = 0;
    decoded.setTS(pos);
    decoded.setDuration((double)decoded.size() / (srate * 2 * 4)); //SRATE * CHN * BITS/8
    pos += decoded.duration();

    return true;
}
void MPDemux::abort()
{
    aborted = true;
    reader.abort();
}

bool MPDemux::open(const QString &url)
{
    if (Reader::create(url, reader))
    {
        if (reader->size() > 0)
            mpfile = QMPlay2ModPlug::Load(reader->read(reader->size()), reader->size());
        reader.reset();
        if (mpfile && QMPlay2ModPlug::GetModuleType(mpfile))
        {
            streams_info += new StreamInfo(srate, 2);
            QMPlay2ModPlug::SetMasterVolume(mpfile, 256); //OK?
            return true;
        }
    }
    return false;
}
