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

#include <FFTSpectrum.hpp>
#include <Functions.hpp>

#include <QPainter>

extern "C"
{
	#include <libavutil/mem.h>
	#include <libavcodec/avfft.h>
}

static inline void fltmix(FFTComplex *dest, const float *src, const int size, const uchar chn)
{
	for (int i = 0, j = 0; i < size; i += chn)
	{
		dest[j].re = dest[j].im = 0.0;
		for (uchar c = 0; c < chn; ++c)
		{
			if (src[i+c] == src[i+c]) //not NaN
				dest[j].re += src[i+c];
		}
		++j;
	}
}

/**/

FFTSpectrumW::FFTSpectrumW(FFTSpectrum &fftSpectrum) :
	fftSpectrum(fftSpectrum)
{
	dw->setObjectName(FFTSpectrumName);
	dw->setWindowTitle(tr("FFT Spectrum"));
	dw->setWidget(this);

	chn = 0;
	srate = 0;
	interval = -1;
	fftSize = 0;

	linearGrad.setStart(0.0, 0.0);
	linearGrad.setColorAt(0.0, Qt::red);
	linearGrad.setColorAt(0.1, Qt::yellow);
	linearGrad.setColorAt(0.4, Qt::green);
	linearGrad.setColorAt(0.9, Qt::blue);
}

void FFTSpectrumW::paint(QPainter &p)
{
	bool canStop = true;

	const int size = spectrumData.size();
	if (size)
	{
		QTransform t;
		t.scale((width() - 1.0) / size, height() - 1.0);

		linearGrad.setFinalStop(t.map(QPointF(size, 0.0)));

		p.setPen(QPen(linearGrad, 1.0));

		const double currTime = Functions::gettime();
		const double realInterval = currTime - time;
		time = currTime;

		const float *spectrum = spectrumData.constData();
		QPainterPath path(t.map(QPointF(0.0, 1.0)));
		for (int x = 0; x < size; ++x)
		{
			/* Bars */
			setValue(lastData[x].first, spectrum[x], realInterval * 2.0);
			path.lineTo(t.map(QPointF(x,       1.0 - lastData[x].first)));
			path.lineTo(t.map(QPointF(x + 1.0, 1.0 - lastData[x].first)));

			/* Horizontal lines over bars */
			setValue(lastData[x].second, spectrum[x], realInterval * 0.5);
			p.drawLine(t.map(QLineF(x, 1.0 - lastData[x].second.first, x + 1.0, 1.0 - lastData[x].second.first)));

			canStop &= (lastData[x].second.first == spectrum[x]);
		}
		path.lineTo(t.map(QPointF(size, 1.0)));
		p.fillPath(path, linearGrad);
	}

	if (stopped && tim.isActive() && canStop)
		tim.stop();
}

void FFTSpectrumW::start(bool v, bool dontCheckRegion)
{
	if (v || (!dontCheckRegion && regionIsVisible()))
	{
		fftSpectrum.soundBuffer(true);
		tim.start(interval);
		time = Functions::gettime();
	}
}
void FFTSpectrumW::stop()
{
	tim.stop();
	fftSpectrum.soundBuffer(false);
	VisWidget::stop();
}

/**/

FFTSpectrum::FFTSpectrum(Module &module) :
	w(*this), fft_ctx(nullptr), tmpData(nullptr), tmpDataSize(0), tmpDataPos(0)
{
	SetModule(module);
}

void FFTSpectrum::soundBuffer(const bool enable)
{
	QMutexLocker mL(&mutex);
	const int arrSize = enable ? (1 << w.fftSize) : 0;
	if (arrSize != tmpDataSize)
	{
		tmpDataPos = 0;
		av_free(tmpData);
		tmpData = nullptr;
		w.spectrumData.clear();
		w.lastData.clear();
		av_fft_end(fft_ctx);
		fft_ctx = nullptr;
		if ((tmpDataSize = arrSize))
		{
			fft_ctx = av_fft_init(w.fftSize, false);
			tmpData = (FFTComplex *)av_malloc(tmpDataSize * sizeof(FFTComplex));
			w.spectrumData.resize(tmpDataSize / 2);
			w.lastData.resize(tmpDataSize / 2);
		}
	}
}

bool FFTSpectrum::set()
{
#ifdef USE_OPENGL
	w.setUseOpenGL(sets().getBool("UseOpenGL"));
#endif
	w.fftSize = sets().getInt("FFTSpectrum/Size");
	if (w.fftSize > 16)
		w.fftSize = 16;
	else if (w.fftSize < 3)
		w.fftSize = 3;
	w.interval = sets().getInt("RefreshTime");
	scale = sets().getInt("FFTSpectrum/Scale");
	if (w.tim.isActive())
		w.start();
	return true;
}

DockWidget *FFTSpectrum::getDockWidget()
{
	return w.dw;
}

bool FFTSpectrum::isVisualization() const
{
	return true;
}
void FFTSpectrum::connectDoubleClick(const QObject *receiver, const char *method)
{
	w.connect(&w, SIGNAL(doubleClicked()), receiver, method);
}
void FFTSpectrum::visState(bool playing, uchar chn, uint srate)
{
	if (playing)
	{
		if (!chn || !srate)
			return;
		w.chn = chn;
		w.srate = srate;
		w.stopped = false;
		w.start();
	}
	else
	{
		if (!chn && !srate)
		{
			w.srate = 0;
			w.stop();
		}
		w.stopped = true;
		w.update();
	}
}
void FFTSpectrum::sendSoundData(const QByteArray &data)
{
	if (!w.tim.isActive() || !data.size())
		return;
	QMutexLocker mL(&mutex);
	if (!tmpDataSize)
		return;
	int newDataPos = 0;
	while (newDataPos < data.size())
	{
		const int size = qMin((data.size() - newDataPos) / (int)sizeof(float), (tmpDataSize - tmpDataPos) * w.chn);
		if (!size)
			break;
		fltmix(tmpData + tmpDataPos, (const float *)(data.constData() + newDataPos), size, w.chn);
		newDataPos += size * sizeof(float);
		tmpDataPos += size / w.chn;
		if (tmpDataPos == tmpDataSize)
		{
			av_fft_permute(fft_ctx, tmpData);
			av_fft_calc(fft_ctx, tmpData);
			tmpDataPos /= 2;
			float *spectrumData = w.spectrumData.data();
			for (int i = 0; i < tmpDataPos; ++i)
			{
				spectrumData[i] = sqrt(tmpData[i].re * tmpData[i].re + tmpData[i].im * tmpData[i].im) / tmpDataPos * scale;
				if (spectrumData[i] > 1.0)
					spectrumData[i] = 1.0;
			}
			tmpDataPos = 0;
		}
	}
}
void FFTSpectrum::clearSoundData()
{
	if (w.tim.isActive())
	{
		QMutexLocker mL(&mutex);
		w.spectrumData.fill(0.0f);
		w.stopped = true;
		w.update();
	}
}
