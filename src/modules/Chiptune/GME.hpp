// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <ChiptuneHeaders.hpp>

#include <Demuxer.hpp>

class Reader;

class GME final : public Demuxer
{
    Q_DECLARE_TR_FUNCTIONS(GME)
public:
    GME(Module &);
    ~GME();
private:
    bool set() override;

    QString name() const override;
    QString title() const override;
    QList<QMPlay2Tag> tags() const override;
    double length() const override;
    int bitrate() const override;

    bool seek(double, bool) override;
    bool read(Packet &, int &) override;
    void abort() override;

    bool open(const QString &) override;

    Playlist::Entries fetchTracks(const QString &url, bool &ok) override;

    bool open(const QString &url, bool tracksOnly);

    QString getTitle(gme_info_t *info, int track) const;
    int getLength(gme_info_t *info) const;

    IOController<Reader> m_reader;
    quint32 m_srate;
    bool m_aborted;
    int m_length;

    QList<QMPlay2Tag> m_tags;
    QString m_url, m_title;

    Music_Emu *m_gme;
};

#define GMEName "Game-Music-Emu"
