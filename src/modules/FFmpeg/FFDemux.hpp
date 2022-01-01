/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2022  Błażej Szczygieł

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
    bool reconnectStreamed;
};
