/*
	QMPlay2 is a video and audio player.
	Copyright (C) 2010-2018  Błażej Szczygieł

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

#include <ChapterProgramInfo.hpp>
#include <ModuleCommon.hpp>
#include <Playlist.hpp>

#include <QString>

struct Packet;

class QMPLAY2SHAREDLIB_EXPORT Demuxer : protected ModuleCommon, public BasicIO
{
public:
	class FetchTracks
	{
	public:
		inline FetchTracks(bool onlyTracks) :
			onlyTracks(onlyTracks),
			isOK(true)
		{}

		Playlist::Entries tracks;
		bool onlyTracks, isOK;
	};

	static bool create(const QString &url, IOController<Demuxer> &demuxer, FetchTracks *fetchTracks = nullptr);

	virtual ~Demuxer() = default;

	virtual bool metadataChanged() const;

	virtual bool isStillImage() const;

	inline QList<StreamInfo *> streamsInfo() const
	{
		return streams_info;
	}

	virtual QList<ProgramInfo> getPrograms() const;
	virtual QList<ChapterInfo> getChapters() const;

	virtual QString name() const = 0;
	virtual QString title() const = 0;
	virtual QList<QMPlay2Tag> tags() const;
	virtual bool getReplayGain(bool album, float &gain_db, float &peak) const
	{
		Q_UNUSED(album)
		Q_UNUSED(gain_db)
		Q_UNUSED(peak)
		return false;
	}
	virtual qint64 size() const;
	virtual double length() const = 0;
	virtual int bitrate() const = 0;
	virtual QByteArray image(bool forceCopy = false) const;

	virtual bool localStream() const;
	virtual bool dontUseBuffer() const;

	virtual bool seek(double pos, bool backward) = 0;
	virtual bool read(Packet &, int &) = 0;

private:
	virtual bool open(const QString &url) = 0;

	virtual Playlist::Entries fetchTracks(const QString &url, bool &ok);

protected:
	StreamsInfo streams_info;
};
