// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QMPlay2Headers.hpp>

#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(57, 28, 100)
#   define CODECPAR_NB_CHANNELS ch_layout.nb_channels
#else
#   define CODECPAR_NB_CHANNELS channels
#endif

using QMPlay2Tag = QPair<QString, QString>;

enum QMPlay2Tags
{
    QMPLAY2_TAG_UNKNOWN = -1,
    QMPLAY2_TAG_NAME, //should be as first
    QMPLAY2_TAG_DESCRIPTION, //should be as second
    QMPLAY2_TAG_LANGUAGE,
    QMPLAY2_TAG_TITLE,
    QMPLAY2_TAG_COMPOSER,
    QMPLAY2_TAG_ARTIST,
    QMPLAY2_TAG_ALBUM,
    QMPLAY2_TAG_GENRE,
    QMPLAY2_TAG_DATE,
    QMPLAY2_TAG_COMMENT,
    QMPLAY2_TAG_LYRICS,
    QMPLAY2_TAG_DEFAULT,
    QMPLAY2_TAG_DRC,
};

class QMPLAY2SHAREDLIB_EXPORT StreamInfo
{
    Q_DECLARE_TR_FUNCTIONS(StreamInfo)
    Q_DISABLE_COPY(StreamInfo)

public:
    static QMPlay2Tags getTag(const QString &tag);
    static QString getTagName(const QString &tag);

    StreamInfo();
    StreamInfo(AVCodecParameters *codecpar);
    StreamInfo(quint32 sampleRateArg, quint8 channelsArg);
    ~StreamInfo();

    inline double getSampleAspectRatio() const
    {
        return av_q2d(params->sample_aspect_ratio);
    }

    inline double getAspectRatio() const
    {
        return getSampleAspectRatio() * params->width / params->height;
    }

    inline double getFPS() const
    {
        return av_q2d(fps);
    }

    inline int getExtraDataCapacity() const
    {
        return params->extradata_size + AV_INPUT_BUFFER_PADDING_SIZE;
    }
    inline const QByteArray getExtraData() const
    {
        return QByteArray::fromRawData((const char *)params->extradata, params->extradata_size);
    }

    inline AVPixelFormat pixelFormat() const
    {
        return static_cast<AVPixelFormat>(params->format);
    }
    inline AVSampleFormat sampleFormat() const
    {
        return static_cast<AVSampleFormat>(params->format);
    }

    QByteArray getFormatName() const;
    void setFormat(int newFormat);

    QByteArray getColorRangeName() const;
    QByteArray getColorPrimariesName() const;
    QByteArray getColorTrcName() const;
    QByteArray getColorSpaceName() const;

    QByteArray codec_name, title, composer, artist;
    QByteArray codec_name_backup;
    QVector<QMPlay2Tag> other_info;
    bool is_default = true;
    bool must_decode = false;
    bool decode_to_ass = false;
    bool skip_auto_select = false;
    bool ts_discont_possible = false;
    AVRational time_base = {1, 10000};

    // Video only
    AVRational fps = {0, 1};
    double rotation = qQNaN();
    bool spherical = false;
    bool custom_sar = false;

    AVCodecParameters *params = nullptr;

private:
    inline void resetSAR();
};
