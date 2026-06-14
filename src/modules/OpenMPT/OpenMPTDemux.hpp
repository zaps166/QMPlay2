/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2026  Błażej Szczygieł

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

#pragma once

#include <Demuxer.hpp>
#include <Packet.hpp>
#include <OpenMPTVisBase.hpp>

#include <IOController.hpp>

#include <memory>

class Reader;
namespace openmpt {
    class module;
}

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
