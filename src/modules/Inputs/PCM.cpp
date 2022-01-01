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

#include <PCM.hpp>

#include <ByteArray.hpp>
#include <Packet.hpp>
#include <Reader.hpp>

/**/

constexpr quint8 bytes[PCM::FORMAT_COUNT] =
{
    1, 1, 2, 3, 4, 4
};

/**/

PCM::PCM(Module &module)
{
    SetModule(module);
}

bool PCM::set()
{
    FORMAT format = (FORMAT)sets().getInt("PCM/format");
    int channels = sets().getInt("PCM/chn");
    int samplerate = sets().getInt("PCM/srate");
    int fileoffset = sets().getInt("PCM/offset");

    if (reader && (fmt != format || chn != channels || srate != samplerate || offset != fileoffset))
        return false;

    bigEndian = sets().getBool("PCM/BE");
    if (!reader)
    {
        fmt = format;
        chn = channels;
        srate = samplerate;
        offset = fileoffset;
    }

    return sets().getBool("PCM");
}

QString PCM::name() const
{
    return PCMName;
}
QString PCM::title() const
{
    return QString();
}
double PCM::length() const
{
    return len;
}
int PCM::bitrate() const
{
    return 8 * (srate * chn * bytes[fmt]) / 1000;
}

bool PCM::seek(double s, bool)
{
    const int64_t filePos = offset + qRound64(s * srate * chn) * bytes[fmt];
    return reader->seek(filePos);
}
bool PCM::read(Packet &decoded, int &idx)
{
    if (reader.isAborted())
        return false;

    decoded.setTS((reader->pos() - offset) / (double)bytes[fmt] / chn / srate);

    const QByteArray dataBA = reader->read(chn * bytes[fmt] * 256);
    const int samples_with_channels = dataBA.size() / bytes[fmt];
    decoded.resize(samples_with_channels * sizeof(float));
    float *decoded_data = (float *)decoded.data();
    ByteArray data(dataBA.constData(), dataBA.size(), bigEndian);
    switch (fmt)
    {
        case PCM_U8:
            for (int i = 0; i < samples_with_channels; i++)
                decoded_data[i] = (data.getBYTE() - 0x7F) / 128.0f;
            break;
        case PCM_S8:
            for (int i = 0; i < samples_with_channels; i++)
                decoded_data[i] = (qint8)data.getBYTE() / 128.0f;
            break;
        case PCM_S16:
            for (int i = 0; i < samples_with_channels; i++)
                decoded_data[i] = (qint16)data.getWORD() / 32768.0f;
            break;
        case PCM_S24:
            for (int i = 0; i < samples_with_channels; i++)
                decoded_data[i] = (qint32)data.get24bAs32b() / 2147483648.0f;
            break;
        case PCM_S32:
            for (int i = 0; i < samples_with_channels; i++)
                decoded_data[i] = (qint32)data.getDWORD() / 2147483648.0f;
            break;
        case PCM_FLT:
            for (int i = 0; i < samples_with_channels; i++)
                decoded_data[i] = data.getFloat();
            break;
        default:
            break;
    }

    idx = 0;
    decoded.setDuration(decoded.size() / chn / sizeof(float) / (double)srate);

    return decoded.size();
}
void PCM::abort()
{
    reader.abort();
}

bool PCM::open(const QString &url)
{
    if (Reader::create(url, reader) && (!offset || reader->seek(offset)))
    {
        if (reader->size() >= 0)
            len = (double)reader->size() / srate / chn / bytes[fmt];
        else
            len = -1.0;

        streams_info += new StreamInfo(srate, chn);
        return true;
    }
    return false;
}
