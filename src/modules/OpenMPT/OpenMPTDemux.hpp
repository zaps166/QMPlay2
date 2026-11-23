// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <OpenMPTHeaders.hpp>

#include <Demuxer.hpp>
#include <Packet.hpp>
#include <OpenMPTVisBase.hpp>

#include <IOController.hpp>

class Reader;

class OpenMPDemux final : public Demuxer
{
    Q_DECLARE_TR_FUNCTIONS(OpenMPDemux)

    enum class SubsongMode
    {
        OnlyFirst = 0,
        AllSequentially = 1,
        Group = 2
    };

public:
    OpenMPDemux(Module &);

private:
    ~OpenMPDemux() override;

    bool set() override;

    QString name() const override;
    QString title() const override;
    QList<QMPlay2Tag> tags() const override;
    double length() const override;
    int bitrate() const override;

    bool dontUseBuffer() const override;

    bool seek(double, bool) override;
    bool read(Packet &, int &) override;
    void abort() override;

    Playlist::Entries fetchTracks(const QString &url, bool &ok) override;

    bool open(const QString &urlArg) override;

    void openModule(const QString &url);

    std::pair<QString, QString> formatSubsongTitle(int subsong, const std::vector<std::string> &subsongNames, const QString &rawTitle) const;

private:
    const quint32 m_sampleRate;
    qint64 m_fileSize{};
    bool m_aborted{false};

    std::unique_ptr<openmpt::module> m_module;
    IOController<Reader> m_reader;
    double m_overallLength{};
    QString m_moduleTitle;
    QString m_title;
    QList<QMPlay2Tag> m_tags;

    std::unique_ptr<OpenMPTVisBase> m_visualizer;
    Packet m_pendingVideoPacket;
    double m_lastAudioDuration{0.0};

    SubsongMode m_subsongMode{};
    int m_stereoSeparation{};
    int m_interpolationFilter{};
    OpenMPTVisBase::VisMode m_visMode{};
    int m_visChannels{};
    int m_visRows{};
};

#define DemuxerName "OpenMPT"
