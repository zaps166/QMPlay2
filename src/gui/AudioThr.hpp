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
	AudioThr(PlayClass &, const QStringList &pluginsName = QStringList());

	void stop(bool terminate = false);

	bool setParams(uchar realChn, uint realSRate, uchar chn = 0, uint sRate = 0);

	void silence(bool invert = false);
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

	QVector< QMPlay2Extensions * > visualizations;
	QVector< AudioFilter * > filters;
private slots:
	void pauseVis(bool);
signals:
	void pauseVisSig(bool);
};

#endif //AUDIOTHR_HPP
