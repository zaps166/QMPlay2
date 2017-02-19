/*
	QMPlay2 is a video and audio player.
	Copyright (C) 2010-2017  Błażej Szczygieł

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

#ifndef AUDIOTHR_HPP
#define AUDIOTHR_HPP

#include <AVThread.hpp>

#include <SndResampler.hpp>

#include <QVector>

class QMPlay2Extensions;
class PlayClass;
class AudioFilter;

class AudioThr : public AVThread
{
	Q_OBJECT
public:
	AudioThr(PlayClass &, const QStringList &pluginsName = {});
	~AudioThr();

	void stop(bool terminate = false);
	void clearVisualizations();

	bool setParams(uchar realChn, uint realSRate, uchar chn = 0, uint sRate = 0);

	void silence(bool invert = false);

	inline void setAllowAudioDrain()
	{
		allowAudioDrain = true;
	}
private:
	void run();

	bool resampler_create();

#ifdef Q_OS_WIN
	void timerEvent(QTimerEvent *);
#endif

	SndResampler sndResampler;
	uchar realChannels, channels;
	uint  realSample_rate, sample_rate;
	double lastSpeed;

	int tmp_br;
	double tmp_time, silence_step;
	volatile double doSilence;
	QMutex silenceChMutex;
#ifdef Q_OS_WIN
	bool canUpdatePos, canUpdateBitrate;
#endif
	bool allowAudioDrain;

	QVector<QMPlay2Extensions *> visualizations;
	QVector<AudioFilter *> filters;
private slots:
	void pauseVis(bool);
signals:
	void pauseVisSig(bool);
};

#endif //AUDIOTHR_HPP
