/*
	QMPlay2 is a video and audio player.
	Copyright (C) 2010-2017  Błażej Szczygieł

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

class FFDemux : public Demuxer
{
	Q_DECLARE_TR_FUNCTIONS(FFDemux)
public:
	FFDemux(QMutex &, Module &);
private:
	~FFDemux() final;

	bool set() override final;

	bool metadataChanged() const override final;

	bool isStillImage() const override final;

	QList<ProgramInfo> getPrograms() const override final;
	QList<ChapterInfo> getChapters() const override final;

	QString name() const override final;
	QString title() const override final;
	QList<QMPlay2Tag> tags() const override final;
	bool getReplayGain(bool album, float &gain_db, float &peak) const override final;
	double length() const override final;
	int bitrate() const override final;
	QByteArray image(bool forceCopy) const override final;

	bool localStream() const override final;

	bool seek(int pos, bool backward) override final;
	bool read(Packet &encoded, int &idx) override final;
	void pause() override final;
	void abort() override final;

	bool open(const QString &entireUrl) override final;

	Playlist::Entries fetchTracks(const QString &url, bool &ok) override final;

	/**/

	void addFormatContext(QString entireUrl, const QString &param = QString());

	QVector<FormatContext *> formatContexts;

	QMutex &avcodec_mutex;
	QMutex mutex;

	bool abortFetchTracks;
};
