// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <FFmpegHeaders.hpp>

#include <Demuxer.hpp>

class FormatContext;

class FFDemux final : public Demuxer
{
    Q_DECLARE_TR_FUNCTIONS(FFDemux)
public:
    FFDemux(Module &);
private:
    ~FFDemux();

    bool set() override;

    bool metadataChanged() const override;

    bool isStillImage() const override;

    QList<ProgramInfo> getPrograms() const override;
    QList<ChapterInfo> getChapters() const override;

    QString name() const override;
    QString title() const override;
    QList<QMPlay2Tag> tags() const override;
    bool getReplayGain(bool album, float &gain_db, float &peak) const override;
    qint64 size() const override;
    double length() const override;
    int bitrate() const override;
    QByteArray image(bool forceCopy) const override;

    bool localStream() const override;

    void selectStreams(const QSet<int> &selectedStreams) override;

    bool seek(double pos, bool backward) override;
    bool read(Packet &encoded, int &idx) override;
    void pause() override;
    void abort() override;

    bool open(const QString &entireUrl) override;

    Playlist::Entries fetchTracks(const QString &url, bool &ok) override;

    /**/

    void addFormatContext(QString entireUrl, const QString &param = QString());

    QVector<FormatContext *> formatContexts;

    QMutex mutex;

    bool abortFetchTracks;
    bool m_reconnectNetwork;
    bool m_allowExperimental = false;
};
