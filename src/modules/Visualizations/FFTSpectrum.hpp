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

#include <QMPlay2Extensions.hpp>
#include <VisWidget.hpp>

#include <QCoreApplication>
#include <QLinearGradient>

class FFTSpectrum;

class FFTSpectrumW : public VisWidget
{
	friend class FFTSpectrum;
	Q_DECLARE_TR_FUNCTIONS(FFTSpectrumW)
public:
	FFTSpectrumW(FFTSpectrum &);
private:
	void paintEvent(QPaintEvent *);

	void start(bool v = false, bool dontCheckRegion = false);
	void stop();

	QVector<float> spectrumData;
	QVector<QPair<qreal, QPair<qreal, double> > > lastData;
	uchar chn;
	uint srate;
	int interval, fftSize;
	FFTSpectrum &fftSpectrum;
	QLinearGradient linearGrad;
};

/**/

struct FFTContext;
struct FFTComplex;

class FFTSpectrum : public QMPlay2Extensions
{
public:
	FFTSpectrum(Module &);

	void soundBuffer(const bool);

	bool set();
private:
	DockWidget *getDockWidget();

	bool isVisualization() const;
	void connectDoubleClick(const QObject *, const char *);
	void visState(bool, uchar, uint);
	void sendSoundData(const QByteArray &);
	void clearSoundData();

	/**/

	FFTSpectrumW w;

	FFTContext *fft_ctx;
	FFTComplex *tmpData;
	int tmpDataSize, tmpDataPos, scale;
	QMutex mutex;
};

#define FFTSpectrumName "Widmo FFT"
