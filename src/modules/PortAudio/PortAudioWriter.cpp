/*
	QMPlay2 is a video and audio player.
	Copyright (C) 2010-2016  Błażej Szczygieł

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

#include <PortAudioWriter.hpp>
#include <QMPlay2Core.hpp>

#ifdef Q_OS_WIN
	#define MMSYSERR_NODRIVER 6
#endif

PortAudioWriter::PortAudioWriter(Module &module) :
	stream(NULL),
	sample_rate(0),
	err(false)
{
	addParam("delay");
	addParam("chn");
	addParam("rate");
	addParam("drain");

	memset(&outputParameters, 0, sizeof outputParameters);
	outputParameters.sampleFormat = paFloat32;
	outputParameters.hostApiSpecificStreamInfo = NULL;

	SetModule(module);
}
PortAudioWriter::~PortAudioWriter()
{
	close();
}

bool PortAudioWriter::set()
{
	bool restartPlaying = false;
	const double delay = sets().getDouble("Delay");
	const QString newOutputDevice = sets().getString("OutputDevice");
	if (outputDevice != newOutputDevice)
	{
		outputDevice = newOutputDevice;
		restartPlaying = true;
	}
	if (outputParameters.suggestedLatency != delay)
	{
		outputParameters.suggestedLatency = delay;
		restartPlaying = true;
	}
	return !restartPlaying && sets().getBool("WriterEnabled");
}

bool PortAudioWriter::readyWrite() const
{
	return stream && !err;
}

bool PortAudioWriter::processParams(bool *paramsCorrected)
{
	bool resetAudio = false;

	int chn = getParam("chn").toInt();

	const int devIdx = PortAudioCommon::getDeviceIndexForOutput(outputDevice, chn);
	if (outputParameters.device != devIdx)
	{
		outputParameters.device = devIdx;
		resetAudio = true;
	}

	if (paramsCorrected)
	{
		const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo(outputParameters.device);
		if (deviceInfo && deviceInfo->maxOutputChannels < chn)
		{
			modParam("chn", (chn = deviceInfo->maxOutputChannels));
			*paramsCorrected = true;
		}
	}

	if (outputParameters.channelCount != chn)
	{
		resetAudio = true;
		outputParameters.channelCount = chn;
	}

	int rate = getParam("rate").toInt();
	if (sample_rate != rate)
	{
		resetAudio = true;
		sample_rate = rate;
	}

	if (resetAudio || err)
	{
		close();
		if (!openStream())
		{
			QMPlay2Core.logError("PortAudio :: " + tr("Cannot open audio output stream"));
			err = true;
		}
	}

	return readyWrite();
}
qint64 PortAudioWriter::write(const QByteArray &arr)
{
	if (!readyWrite())
		return 0;

	if (Pa_IsStreamStopped(stream))
	{
		if (!startStream())
			return playbackError();
		fullBufferReached = false;
	}
#ifndef Q_OS_MAC
	else
	{
		const int diff = Pa_GetStreamWriteAvailable(stream) - outputLatency * sample_rate;
		if (diff <= 0)
			fullBufferReached = true;
		else if (fullBufferReached)
		{
			//Reset stream to prevent potential short audio garbage on Windows
			Pa_AbortStream(stream);
			if (!startStream())
				return playbackError();
			fullBufferReached = false;
		}
	}
#endif

#ifdef Q_OS_LINUX //FIXME: Does OSS on FreeBSD need channel swapping? Also don't do it on const data.
	const int chn = outputParameters.channelCount;
	if (chn == 6 || chn == 8)
	{
		float *audio_buffer = (float *)arr.data();
		for (int i = 0; i < arr.size() / 4; i += chn)
		{
			float tmp = audio_buffer[i+2];
			audio_buffer[i+2] = audio_buffer[i+4];
			audio_buffer[i+4] = tmp;
			tmp = audio_buffer[i+3];
			audio_buffer[i+3] = audio_buffer[i+5];
			audio_buffer[i+5] = tmp;
		}
	}
#endif

	if (!writeStream(arr))
	{
		bool isError = true;

#ifdef Q_OS_WIN
		if (isNoDriverError() && reopenStream()) //"writeStream()" must fail only on "paUnanticipatedHostError"
		{
			isError = !writeStream(arr);
			fullBufferReached = false;
		}
#endif

		if (isError)
			return playbackError();
	}

	return arr.size();
}
void PortAudioWriter::pause()
{
	if (readyWrite())
		Pa_StopStream(stream);
}

QString PortAudioWriter::name() const
{
	return PortAudioWriterName;
}

bool PortAudioWriter::open()
{
	return true;
}

/**/

bool PortAudioWriter::openStream()
{
	PaStream *newStream = NULL;
	if (Pa_OpenStream(&newStream, NULL, &outputParameters, sample_rate, 0, paDitherOff, NULL, NULL) == paNoError)
	{
		stream = newStream;
		outputLatency = Pa_GetStreamInfo(stream)->outputLatency;
		modParam("delay", outputLatency);
		return true;
	}
	return false;
}
bool PortAudioWriter::startStream()
{
	const PaError e = Pa_StartStream(stream);
	if (e != paNoError)
	{
#ifdef Q_OS_WIN
		if (e == paUnanticipatedHostError && isNoDriverError())
			return reopenStream();
#endif
		return false;
	}
	return true;
}
inline bool PortAudioWriter::writeStream(const QByteArray &arr)
{
	const PaError e = Pa_WriteStream(stream, arr.data(), arr.size() / outputParameters.channelCount / sizeof(float));
	if (e != paNoError)
		fullBufferReached = false;
	return (e != paUnanticipatedHostError);
}
qint64 PortAudioWriter::playbackError()
{
	QMPlay2Core.logError("PortAudio :: " + tr("Playback error"));
	err = true;
	return 0;
}

#ifdef Q_OS_WIN
bool PortAudioWriter::isNoDriverError() const
{
	const PaHostErrorInfo *errorInfo = Pa_GetLastHostErrorInfo();
	return errorInfo && errorInfo->hostApiType == paMME && errorInfo->errorCode == MMSYSERR_NODRIVER;
}
bool PortAudioWriter::reopenStream()
{
	Pa_CloseStream(stream);
	if (openStream())
		return (Pa_StartStream(stream) == paNoError);
	stream = NULL;
	return false;
}
#endif

void PortAudioWriter::close()
{
	if (stream)
	{
		if (!err && getParam("drain").toBool())
			Pa_StopStream(stream);
		Pa_CloseStream(stream);
		stream = NULL;
	}
	err = false;
}
