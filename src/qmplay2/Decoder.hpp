// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <ModuleCommon.hpp>
#include <Frame.hpp>
#include <Packet.hpp>

class QMPlay2OSD;
class VideoFilter;
class StreamInfo;
class LibASS;

class QMPLAY2SHAREDLIB_EXPORT Decoder : public ModuleCommon
{
public:
    static Decoder *create(
        StreamInfo &streamInfo,
        const QStringList &modNames,
        QString *modNameOutput
    );

    virtual QString name() const = 0;

    virtual bool hasHWDecContext() const;
    virtual std::shared_ptr<VideoFilter> hwAccelFilter() const;

    virtual QByteArray subtitleHeader() const;

    virtual void setSupportedPixelFormats(const AVPixelFormats &pixelFormats);

    /*
     * hurry_up ==  0 -> no frame skipping, normal quality
     * hurry_up >=  1 -> faster decoding, lower image quality, frame skipping during decode
     * hurry_up == ~0 -> much faster decoding, no frame copying
    */
    virtual int decodeVideo(const Packet &encodedPacket, Frame &decoded, AVPixelFormat &newPixFmt, bool flush, unsigned hurry_up);
    virtual int decodeAudio(const Packet &encodedPacket, QByteArray &decoded, double &ts, quint8 &channels, quint32 &sampleRate, bool flush = false);
    virtual bool decodeSubtitle(const QVector<Packet> &encodedPackets, double pos, std::shared_ptr<QMPlay2OSD> &osd, const QSize &size, bool flush = false);
    virtual QList<QByteArray> decodeSubtitle(const Packet &encodedPacket);

    virtual int pendingFrames() const;

    virtual bool hasCriticalError() const;

    virtual bool isDummyDecoder() const;

private:
    virtual bool open(StreamInfo &streamInfo) = 0;
};
