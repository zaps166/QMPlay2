/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2024  Błażej Szczygieł

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

#include <Equalizer.hpp>

#include <cmath>

static inline float cosI(const float y1, const float y2, float p)
{
    p = (1.0f - cos(p * static_cast<float>(M_PI))) / 2.0f;
    return y1 * (1.0f - p) + y2 * p;
}

QVector<float> Equalizer::interpolate(const QVector<float> &src, const int len)
{
    QVector<float> dest(len);
    const int size = src.size();
    if (size >= 2)
    {
        const float mn = (size - 1.0f) / len;
        for (int i = 0; i < len; ++i)
        {
            int   x = i * mn;
            float p = i * mn - x;
            dest[i] = cosI(src[x], src[x + 1], p);
        }
    }
    return dest;
}

QVector<float> Equalizer::freqs(Settings &sets)
{
    QVector<float> freqs(sets.getInt("Equalizer/count"));
    const int minFreq = sets.getInt("Equalizer/minFreq"), maxFreq = sets.getInt("Equalizer/maxFreq");
    const float l = powf(maxFreq / minFreq, 1.0f / (freqs.count() - 1));
    for (int i = 0; i < freqs.count(); ++i)
        freqs[i] = minFreq * powf(l, i);
    return freqs;
}

float Equalizer::getAmpl(int val)
{
    if (val < 0)
        return 0.0f; //-inf
    if (val == 50)
        return 1.0f;
    if (val > 50)
        return powf(val / 50.0f, 3.33f);
    return powf(50.0f / (100 - val), 3.33f);
}

Equalizer::Equalizer(Module &module)
{
    SetModule(module);
}
Equalizer::~Equalizer()
{
    alloc(false);
}

bool Equalizer::set()
{
    QMutexLocker locker(&m_mutex);
    m_enabled = sets().getBool("Equalizer");
    if (m_fftNBits && sets().getInt("Equalizer/nbits") != m_fftNBits)
        alloc(false);
    alloc(m_enabled && m_hasParameters);
    return true;
}

bool Equalizer::setAudioParameters(uchar chn, uint srate)
{
    m_hasParameters = chn && srate;
    if (m_hasParameters)
    {
        m_chn = chn;
        m_srate = srate;
        clearBuffers();
    }
    alloc(m_enabled && m_hasParameters);
    return true;
}
int Equalizer::bufferedSamples() const
{
    if (m_canFilter)
    {
        QMutexLocker locker(&m_mutex);
        int bufferedSamples = m_input[0].size();
        return bufferedSamples;
    }
    return 0;
}
void Equalizer::clearBuffers()
{
    QMutexLocker locker(&m_mutex);
    if (m_canFilter)
    {
        m_input.clear();
        m_input.resize(m_chn);
        m_lastSamples.clear();
        m_lastSamples.resize(m_chn);
    }
}
double Equalizer::filter(QByteArray &data, bool flush)
{
    if (!m_canFilter)
        return 0.0;

    QMutexLocker locker(&m_mutex);

    const int fftSize = m_fftSize;
    const int fftSizeDiv2 = fftSize / 2;
    const float fftSizeFlt = fftSize;
    const int chn = m_chn;

    if (!flush)
    {
        float *samples = (float *)data.data();
        const int size = data.size() / sizeof(float);
        for (int c = 0; c < chn; ++c) // Buffering data
            for (int i = 0; i < size; i += chn)
                m_input[c].push_back(samples[c + i]);
    }
    else for (int c = 0; c < chn; ++c) // Adding silence
    {
        m_input[c].resize(fftSize);
    }

    data.resize(0);
    const int chunks = m_input[0].size() / fftSizeDiv2 - 1;
    if (chunks > 0) // If there's enough data
    {
        data.resize(chn * sizeof(float) * fftSizeDiv2 * chunks);
        auto samples = reinterpret_cast<float *>(data.data());
        for (int c = 0; c < chn; ++c)
        {
            int pos = c;
            while (static_cast<int>(m_input[c].size()) >= fftSize)
            {
                for (int i = 0; i < fftSize; ++i)
                {
                    m_complex[i].re = m_input[c][i];
                    m_complex[i].im = 0.0f;
                }

                if (!flush)
                    m_input[c].erase(m_input[c].begin(), m_input[c].begin() + fftSizeDiv2);
                else
                    m_input[c].clear();

                m_fftIn.calc(m_complex);
                for (int i = 0; i < fftSizeDiv2; ++i)
                {
                    const float coeff = m_f[i] * m_preamp;
                    m_complex[              i].re *= coeff;
                    m_complex[              i].im *= coeff;
                    m_complex[fftSize - 1 - i].re *= coeff;
                    m_complex[fftSize - 1 - i].im *= coeff;
                }
                m_fftOut.calc(m_complex);

                if (m_lastSamples[c].empty())
                {
                    for (int i = 0; i < fftSizeDiv2; ++i, pos += chn)
                        samples[pos] = m_complex[i].re / fftSizeFlt;
                    m_lastSamples[c].resize(fftSizeDiv2);
                }
                else for (int i = 0; i < fftSizeDiv2; ++i, pos += chn)
                {
                    samples[pos] = (m_complex[i].re / fftSizeFlt) * m_windF[i] + m_lastSamples[c][i];
                }

                for (int i = fftSizeDiv2; i < fftSize; ++i)
                    m_lastSamples[c][i - fftSizeDiv2] = (m_complex[i].re / fftSizeFlt) * m_windF[i];
            }
        }
    }

    return fftSizeFlt / m_srate;
}

void Equalizer::alloc(bool b)
{
    QMutexLocker locker(&m_mutex);
    if (!b && (m_fftIn.isValid() || m_fftOut.isValid()))
    {
        m_canFilter = false;
        m_fftNBits = m_fftSize = 0;
        m_fftIn.finish();
        m_fftOut.finish();
        av_free(m_complex);
        m_complex = nullptr;
        m_input.clear();
        m_input.shrink_to_fit();
        m_lastSamples.clear();
        m_lastSamples.shrink_to_fit();
        m_windF.clear();
        m_windF.shrink_to_fit();
        m_f.clear();
        m_f.shrink_to_fit();
    }
    else if (b)
    {
        if (!m_fftIn.isValid() || !m_fftOut.isValid())
        {
            m_fftNBits  = sets().getInt("Equalizer/nbits");
            m_fftSize   = 1 << m_fftNBits;
            m_fftIn.init(m_fftNBits, false);
            m_fftOut.init(m_fftNBits, true);
            m_complex = FFT::allocComplex(m_fftSize);
            m_input.resize(m_chn);
            m_lastSamples.resize(m_chn);
            m_windF.resize(m_fftSize);
            for (int i = 0; i < m_fftSize; ++i)
                m_windF[i] = 0.5f - 0.5f * cos(2.0f * M_PI * i / (m_fftSize - 1));
        }
        interpolateFilterCurve();
        m_canFilter = true;
    }
}
void Equalizer::interpolateFilterCurve()
{
    const int size = sets().getInt("Equalizer/count");

    QVector<float> src(size);
    for (int i = 0; i < size; ++i)
        src[i] = getAmpl(sets().getInt(QString("Equalizer/%1").arg(i)));

    int preampVal = sets().getInt("Equalizer/-1");
    if (preampVal >= 0)
    {
        m_preamp = getAmpl(preampVal);
    }
    else // Auto preamp
    {
        preampVal = 0;
        for (int i = 0; i < size; ++i)
        {
            const int val = sets().getInt(QString("Equalizer/%1").arg(i));
            preampVal = qMax(val < 0 ? 0 : val, preampVal);
        }
        m_preamp = getAmpl(100 - preampVal);
    }

    const int len = m_fftSize / 2;
    if (static_cast<int>(m_f.size()) != len)
        m_f.resize(len);
    if (m_srate && size >= 2)
    {
        QVector<float> freqs = Equalizer::freqs(sets());
        const int maxHz = m_srate / 2;
        int x = 0, start = 0;
        for (int i = 0; i < len; ++i)
        {
            const int hz = (i+1) * maxHz / len;
            for (int j = x; j < size; ++j)
            {
                if (freqs[j] >= hz)
                    break;
                if (x != j)
                {
                    x = j;
                    start = i;
                }
            }
            if (x+1 < size)
                m_f[i] = cosI(src[x], src[x + 1], (i - start) / (len * freqs[x + 1] / maxHz - 1 - start)); /* start / end */
            else
                m_f[i] = src[x];
        }
    }
}
