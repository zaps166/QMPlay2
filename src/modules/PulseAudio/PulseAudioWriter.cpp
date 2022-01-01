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

#include <PulseAudioWriter.hpp>

PulseAudioWriter::PulseAudioWriter(Module &module) :
    err(false)
{
    addParam("delay");
    addParam("chn");
    addParam("rate");
    addParam("drain");

    SetModule(module);
}
PulseAudioWriter::~PulseAudioWriter()
{
    pulse.stop(!err && getParam("drain").toBool());
}

bool PulseAudioWriter::set()
{
    if (pulse.delay != sets().getDouble("Delay"))
    {
        pulse.delay = sets().getDouble("Delay");
        return false;
    }
    return sets().getBool("WriterEnabled");
}

bool PulseAudioWriter::readyWrite() const
{
    return !err && pulse.isOpen();
}

bool PulseAudioWriter::processParams(bool *)
{
    bool resetAudio = false;

    uchar chn = getParam("chn").toUInt();
    if (pulse.channels != chn)
    {
        resetAudio = true;
        pulse.channels = chn;
    }
    uint rate = getParam("rate").toUInt();
    if (pulse.sample_rate != rate)
    {
        resetAudio = true;
        pulse.sample_rate = rate;
    }

    if (resetAudio || err)
    {
        pulse.stop();
        err = !pulse.start();
        if (!err)
            modParam("delay", pulse.delay);
        else
            QMPlay2Core.logError("PulseAudio :: " + tr("Cannot open audio output stream"));
    }

    return readyWrite();
}
qint64 PulseAudioWriter::write(const QByteArray &arr)
{
    if (!arr.size() || !readyWrite())
        return 0;

    bool showError = true;
    err = !pulse.write(arr, showError);
    if (err)
    {
        if (showError)
            QMPlay2Core.logError("PulseAudio :: " + tr("Playback error"));
        return 0;
    }

    return arr.size();
}

QString PulseAudioWriter::name() const
{
    return PulseAudioWriterName;
}

bool PulseAudioWriter::open()
{
    return pulse.isOK();
}
