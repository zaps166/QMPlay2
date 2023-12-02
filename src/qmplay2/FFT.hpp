#pragma once

extern "C"
{
    #include <libavutil/version.h>
    #include <libavutil/mem.h>
    #if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(58, 29, 100)
    #   include <libavutil/tx.h>
    #else
    #   include <libavcodec/avfft.h>
    #endif
}

class FFT
{
public:
    struct Complex
    {
        float re, im;
    };

    static inline Complex *allocComplex(int n);
    static inline void freeComplex(Complex *&complex);

public:
    inline FFT() = default;
    inline ~FFT();

    inline bool isValid() const;

    inline void init(int nbits, int inverse);
    inline void calc(Complex *complex);
    inline void finish();

private:
#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(58, 29, 100)
    AVTXContext *m_ctx = nullptr;
    av_tx_fn m_fn = nullptr;
#else
    FFTContext *m_ctx = nullptr;
#endif
};

/* Inline implementation */

FFT::Complex *FFT::allocComplex(int n)
{
    return reinterpret_cast<Complex *>(av_malloc(n * sizeof(Complex)));
}
void FFT::freeComplex(Complex *&complex)
{
    av_freep(&complex);
}

FFT::~FFT()
{
    finish();
}

bool FFT::isValid() const
{
    return static_cast<bool>(m_ctx);
}

#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(58, 29, 100)
void FFT::init(int nbits, int inverse)
{
    finish();

    const float scale = 1.0f;
    av_tx_init(&m_ctx, &m_fn, AV_TX_FLOAT_FFT, inverse, 1 << nbits, &scale, AV_TX_INPLACE);
}
void FFT::calc(Complex *complex)
{
    if (m_ctx && m_fn)
    {
        m_fn(m_ctx, complex, complex, sizeof(Complex));
    }
}
void FFT::finish()
{
    av_tx_uninit(&m_ctx);
}
#else
void FFT::init(int nbits, int inverse)
{
    finish();

    m_ctx = av_fft_init(nbits, inverse);
}
void FFT::calc(Complex *complex)
{
    if (m_ctx)
    {
        av_fft_permute(m_ctx, reinterpret_cast<FFTComplex *>(complex));
        av_fft_calc(m_ctx, reinterpret_cast<FFTComplex *>(complex));
    }
}
void FFT::finish()
{
    av_fft_end(m_ctx);
    m_ctx = nullptr;
}
#endif
