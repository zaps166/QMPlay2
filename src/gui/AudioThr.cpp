/*
	QMPlay2 is a video and audio player.
	Copyright (C) 2010-2018  Błażej Szczygieł

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

#include <AudioThr.hpp>

#include <Main.hpp>
#include <Writer.hpp>
#include <Decoder.hpp>
#include <Settings.hpp>
#include <Functions.hpp>
#include <PlayClass.hpp>
#include <AudioFilter.hpp>
#include <ScreenSaver.hpp>
#include <QMPlay2Extensions.hpp>

#include <QCoreApplication>

#include <cmath>

AudioThr::AudioThr(PlayClass &playC, const QStringList &pluginsName) :
	AVThread(playC, "audio:", nullptr, pluginsName)
{
	allowAudioDrain = false;

	for (QMPlay2Extensions *QMPlay2Ext : QMPlay2Extensions::QMPlay2ExtensionsList())
		if (QMPlay2Ext->isVisualization())
			visualizations += QMPlay2Ext;
	visualizations.squeeze();

	filters = AudioFilter::open();

	connect(this, SIGNAL(pauseVisSig(bool)), this, SLOT(pauseVis(bool)));
#ifdef Q_OS_WIN
	startTimer(500);
#endif

	if (QMPlay2GUI.mainW->property("fullScreen").toBool())
		QMPlay2GUI.screenSaver->inhibit(1);

	maybeStartThread();
}
AudioThr::~AudioThr()
{
	QMPlay2GUI.screenSaver->unInhibit(1);
}

void AudioThr::stop(bool terminate)
{
	for (QMPlay2Extensions *vis : asConst(visualizations))
		vis->visState(false);
	for (AudioFilter *filter : asConst(filters))
		delete filter;
	playC.audioSeekPos = -1;
	AVThread::stop(terminate);
}
void AudioThr::clearVisualizations()
{
	for (QMPlay2Extensions *vis : asConst(visualizations))
		vis->clearSoundData();
}

bool AudioThr::setParams(uchar realChn, uint realSRate, uchar chn, uint sRate)
{
	doSilence = -1.0;
	lastSpeed = playC.speed;

	realChannels    = realChn;
	realSample_rate = realSRate;

	writer->modParam("chn",  (channels = chn ? chn : realChannels));
	writer->modParam("rate", (sample_rate = sRate ? sRate : realSRate));

	bool paramsCorrected = false;
	if (writer->processParams((sRate && chn) ? nullptr : &paramsCorrected)) //nie pozwala na korektę jeżeli są wymuszone parametry
	{
		if (paramsCorrected)
		{
			const uchar lastChn = channels;
			const uint lastSRate = sample_rate;
			channels = writer->getParam("chn").toUInt();
			sample_rate = writer->getParam("rate").toUInt();
			if ((!chn || channels == lastChn) && (!sRate || sample_rate == lastSRate))
				QMPlay2Core.logInfo(tr("Module") + " \"" + writer->name() + "\" " + tr("sets the parameters to") + ": " + QString::number(channels) + " " + tr("channels") + ", " + QString::number(sample_rate) + " " + tr("Hz"));
			else
			{
				QMPlay2Core.logError(tr("Module") + " \"" + writer->name() + "\" " + tr("requires a change in one of the forced parameters, sound disabled ..."));
				return false;
			}
		}

		if (!resampler_create())
		{
			if (paramsCorrected)
				return false;
			channels    = realChannels;
			sample_rate = realSample_rate;
		}

		for (QMPlay2Extensions *vis : asConst(visualizations))
			vis->visState(true, realChannels, realSample_rate);
		for (AudioFilter *filter : asConst(filters))
			filter->setAudioParameters(realChannels, realSample_rate);

		return true;
	}

	return false;
}

void AudioThr::silence(bool invert)
{
	if (QMPlay2Core.getSettings().getBool("Silence") && playC.frame_last_pts <= 0.0 && doSilence == -1.0 && isRunning() && (invert || !playC.paused))
	{
		playC.doSilenceBreak = false;
		allowAudioDrain |= !invert;
		silence_step = (invert ? -4.0 : 4.0) / sample_rate;
		silenceChMutex.lock();
		doSilence = invert ? -silence_step : (1.0 - silence_step);
		silenceChMutex.unlock();
		while (!invert && isRunning())
		{
			silenceChMutex.lock();
			const double lastDoSilence = doSilence;
			silenceChMutex.unlock();
			if (lastDoSilence <= 0.0 || lastDoSilence >= 1.0)
				break;
			for (int i = 0; i < 100; ++i)
			{
				QCoreApplication::processEvents();
				if (playC.doSilenceBreak)
				{
					playC.doSilenceBreak = false;
					break;
				}
				Functions::s_wait(0.01);
				silenceChMutex.lock();
				if (doSilence <= 0.0)
				{
					silenceChMutex.unlock();
					break;
				}
				silenceChMutex.unlock();
			}
			silenceChMutex.lock();
			if (lastDoSilence == doSilence)
			{
				doSilence = -1.0;
				silenceChMutex.unlock();
				break;
			}
			silenceChMutex.unlock();
		}
	}
}

void AudioThr::run()
{
	setPriority(QThread::HighestPriority);

	QMutex emptyBufferMutex;
	bool paused = false;
	bool oneFrame = false;
	tmp_br = tmp_time = 0;
#ifdef Q_OS_WIN
	canUpdatePos = canUpdateBitrate = false;
#endif
	while (!br)
	{
		Packet packet;
		double delay = 0.0, audio_pts = 0.0; //"audio_pts" odporny na zerowanie przy przewijaniu
		Decoder *last_dec = dec;
		int bytes_consumed = -1;
		while (!br && dec == last_dec)
		{
			playC.aPackets.lock();
			const bool hasAPackets = playC.aPackets.canFetch();
			bool hasBufferedSamples = false;
			if (playC.endOfStream && !hasAPackets)
				for (AudioFilter *filter : asConst(filters))
					if (filter->bufferedSamples())
					{
						hasBufferedSamples = true;
						break;
					}

			if ((playC.paused && !oneFrame) || (!hasAPackets && !hasBufferedSamples) || playC.waitForData || (playC.audioSeekPos <= 0.0 && playC.videoSeekPos > 0.0))
			{
#ifdef Q_OS_WIN
				canUpdatePos = canUpdateBitrate = false;
#endif
				tmp_br = tmp_time = 0;
				if (playC.paused && !paused)
				{
					doSilence = -1.0;
					writer->pause();
					paused = true;
					emit pauseVisSig(paused);
				}
				playC.aPackets.unlock();

				if (!playC.paused)
					waiting = playC.fillBufferB = true;

				emptyBufferMutex.lock();
				playC.emptyBufferCond.wait(&emptyBufferMutex, MUTEXWAIT_TIMEOUT);
				emptyBufferMutex.unlock();
				continue;
			}
			if (paused)
			{
				paused = false;
				emit pauseVisSig(paused);
			}
			waiting = false;

			const bool flushAudio = playC.flushAudio;

			if (!hasBufferedSamples && (bytes_consumed < 0 || flushAudio))
				packet = playC.aPackets.fetch();
			else if (hasBufferedSamples)
				packet.ts = audio_pts + playC.audio_last_delay + delay; //szacowanie czasu
			playC.aPackets.unlock();

			if (playC.nextFrameB && playC.seekTo < 0.0 && playC.audioSeekPos <= 0.0 && playC.frame_last_pts <= 0.0)
			{
				playC.nextFrameB = false;
				oneFrame = playC.paused = true;
			}

			playC.fillBufferB = true;

			mutex.lock();
			if (br)
			{
				mutex.unlock();
				break;
			}

			Buffer decoded;
			if (!hasBufferedSamples)
			{
				quint8 newChannels = 0;
				quint32 newSampleRate = 0;
				bytes_consumed = dec->decodeAudio(packet, decoded, newChannels, newSampleRate, flushAudio);
				tmp_br += bytes_consumed;
				if (newChannels && newSampleRate && (newChannels != realChannels || newSampleRate != realSample_rate))
				{
					//Audio parameters has been changed
					updateMutex.lock();
					mutex.unlock();
					emit playC.audioParamsUpdate(newChannels, newSampleRate);
					updateMutex.lock(); //Wait for "audioParamsUpdate()" to be finished
					mutex.lock();
					updateMutex.unlock();
				}
			}

			if (tmp_time >= 1000.0)
			{
#ifdef Q_OS_WIN
				canUpdateBitrate = true;
#else
				emit playC.updateBitrateAndFPS(round((tmp_br << 3) / tmp_time), -1);
				tmp_br = tmp_time = 0;
#endif
			}

			delay = writer->getParam("delay").toDouble();
			for (AudioFilter *filter : asConst(filters))
			{
				if (flushAudio)
					filter->clearBuffers();
				delay += filter->filter(decoded, hasBufferedSamples);
			}

			if (flushAudio)
				playC.flushAudio = false;
			int decodedSize = decoded.size();
			int decodedPos = 0;
			while (decodedSize > 0 && (!playC.paused || oneFrame) && !br && !br2)
			{
				const double max_len = 0.02; //TODO: zrobić opcje?
				const int chunk = qMin(decodedSize, (int)(ceil(realSample_rate * max_len) * realChannels * sizeof(float)));
				float vol[2] = {0.0f, 0.0f};
				if (!playC.muted)
					for (int c = 0; c < 2; ++c)
						if (playC.vol[c] > 0.0)
							vol[c] = playC.replayGain * (playC.vol[c] == 1.0 ? 1.0 : playC.vol[c] * playC.vol[c]);

				const bool isMuted = qFuzzyIsNull(vol[0]) && qFuzzyIsNull(vol[1]);

				QByteArray decodedChunk;
				if (isMuted)
					decodedChunk.fill(0, chunk);
				else
					decodedChunk = QByteArray::fromRawData((const char *)decoded.constData() + decodedPos, chunk);

				decodedPos += chunk;
				decodedSize -= chunk;

				playC.audio_last_delay = (double)decodedChunk.size() / (double)(sizeof(float) * realChannels * realSample_rate);
				if (packet.ts.isValid())
				{
					audio_pts = playC.audio_current_pts = packet.ts - delay;
					if (!playC.vThr && playC.audioSeekPos <= 0)
					{
#ifdef Q_OS_WIN
						playC.chPos(playC.audio_current_pts, playC.flushAudio);
#else
						playC.chPos(playC.audio_current_pts);
#endif
					}
				}

				if (playC.audioSeekPos > 0.0)
				{
					bool cont = true;
					if (audio_pts >= playC.audioSeekPos)
					{
						tmp_br = 0;
						playC.audioSeekPos = -1.0;
						playC.emptyBufferCond.wakeAll();
						if (playC.videoSeekPos <= 0.0)
							cont = false; // Don't play if video is not ready
					}
					if (cont)
						break;
				}

				tmp_time += playC.audio_last_delay * 1000.0;
				packet.ts += playC.audio_last_delay;

#ifdef Q_OS_WIN
				canUpdatePos = true;
#endif

				if (playC.skipAudioFrame <= 0.0 || oneFrame)
				{
					const double speed = playC.speed;
					if (speed != lastSpeed)
					{
						resampler_create();
						lastSpeed = speed;
					}

					if (!isMuted && (!qFuzzyCompare(vol[0], 1.0f) || !qFuzzyCompare(vol[1], 1.0f)))
					{
						const int size = decodedChunk.size() / sizeof(float);
						float *data = (float *)decodedChunk.data();
						for (int i = 0; i < size; ++i)
							data[i] *= vol[i & 1];
					}

					for (QMPlay2Extensions *vis : asConst(visualizations))
						vis->sendSoundData(decodedChunk);

					QByteArray dataToWrite;
					if (sndResampler.isOpen())
						sndResampler.convert(decodedChunk, dataToWrite);
					else
						dataToWrite = decodedChunk;

					if (!isMuted && doSilence >= 0.0)
					{
						silenceChMutex.lock();
						if (doSilence >= 0.0)
						{
							float *data = (float *)dataToWrite.data();
							const int s = dataToWrite.size() / sizeof(float);
							for (int i = 0; i < s; i += channels)
							{
								for (int j = 0; j < channels; ++j)
									data[i+j] *= doSilence;
								doSilence -= silence_step;
								if (doSilence < 0.0)
									doSilence = 0.0;
								else if (doSilence > 1.0)
								{
									doSilence = -1.0;
									break;
								}
							}
						}
						silenceChMutex.unlock();
					}

					oneFrame = false;
					writer->write(dataToWrite);
				}
				else
				{
					playC.skipAudioFrame -= playC.audio_last_delay;
				}
			}

			mutex.unlock();

			if (playC.flushAudio || packet.size() == bytes_consumed || (!bytes_consumed && !decoded.size()))
			{
				bytes_consumed = -1;
				packet = Packet();
			}
			else if (bytes_consumed != packet.size())
				packet.remove(0, bytes_consumed);
		}
	}
	writer->modParam("drain", allowAudioDrain);
}

bool AudioThr::resampler_create()
{
	const double speed = playC.speed > 0.0 ? playC.speed : 1.0;
	if (realSample_rate != sample_rate || realChannels != channels || speed != 1.0)
	{
		const bool OK = sndResampler.create(realSample_rate, realChannels, sample_rate / speed, channels);
		if (!OK)
			QMPlay2Core.logError(tr("Error during initialization") + ": " + sndResampler.name());
		return OK;
	}
	sndResampler.destroy();
	return true;
}

#ifdef Q_OS_WIN
void AudioThr::timerEvent(QTimerEvent *)
{
	if (br || !isRunning())
		return;
	if (canUpdatePos)
	{
		if (!playC.vThr)
			emit playC.updatePos(playC.pos);
		canUpdatePos = false;
	}
	if (canUpdateBitrate)
	{
		emit playC.updateBitrateAndFPS(round((tmp_br << 3) / tmp_time), -1);
		canUpdateBitrate = false;
		tmp_time = tmp_br = 0;
	}
}
#endif

void AudioThr::pauseVis(bool b)
{
	for (QMPlay2Extensions *vis : asConst(visualizations))
		vis->visState(!b, realChannels, realSample_rate);
}
