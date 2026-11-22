// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QMPlay2Extensions.hpp>
#include <VisWidget.hpp>
#include <FFT.hpp>

#include <QCoreApplication>
#include <QLinearGradient>

class FFTSpectrum;

class FFTSpectrumW final : public VisWidget
{
    friend class FFTSpectrum;
    Q_DECLARE_TR_FUNCTIONS(FFTSpectrumW)
public:
    FFTSpectrumW(FFTSpectrum &);
private:
    void paint(QPainter &p) override;

    void mouseMoveEvent(QMouseEvent *e) override;

    void start() override;
    void stop() override;

    QVector<float> spectrumData;
    QVector<QPair<qreal, QPair<qreal, double>>> lastData;
    uchar chn;
    uint srate;
    int m_limitFreq = 0;
    int interval, fftSize;
    FFTSpectrum &fftSpectrum;
    QLinearGradient linearGrad;
    QImage mGradientImg;
};

/**/

class FFTSpectrum final : public QMPlay2Extensions
{
public:
    FFTSpectrum(Module &);

    void soundBuffer(const bool);

    bool set() override;
private:
    DockWidget *getDockWidget() override;

    bool isVisualization() const override;
    void connectDoubleClick(const QObject *, const char *) override;
    void visState(bool, uchar, uint) override;
    void sendSoundData(const QByteArray &) override;
    void clearSoundData() override;

    /**/

    FFTSpectrumW w;

    FFT m_fft;
    FFT::Complex *m_complex = nullptr;
    std::vector<float> m_winFunc;
    int tmpDataSize, tmpDataPos;
    bool m_linearScale;
    QMutex mutex;
};

#define FFTSpectrumName "Widmo FFT"
