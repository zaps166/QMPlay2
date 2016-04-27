#include <PulseAudioWriter.hpp>
#include <QMPlay2Core.hpp>

PulseAudioWriter::PulseAudioWriter(Module &module) :
	err(false)
{
	addParam("delay");
	addParam("chn");
	addParam("rate");

	SetModule(module);
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

	err = !pulse.write(arr);
	if (err)
	{
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
