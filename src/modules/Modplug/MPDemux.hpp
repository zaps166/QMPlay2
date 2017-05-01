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

#include <IOController.hpp>

namespace QMPlay2ModPlug {
	struct File;
}
class Reader;

class MPDemux : public Demuxer
{
	Q_DECLARE_TR_FUNCTIONS(MPDemux)
public:
	MPDemux(Module &);
private:
	~MPDemux() final;

	bool set() override final;

	QString name() const override final;
	QString title() const override final;
	QList<QMPlay2Tag> tags() const override final;
	double length() const override final;
	int bitrate() const override final;

	bool seek(double, bool) override final;
	bool read(Packet &, int &) override final;
	void abort() override final;

	bool open(const QString &) override final;

	/**/

	bool aborted;
	double pos;
	quint32 srate;
	QMPlay2ModPlug::File *mpfile;
	IOController<Reader> reader;
};

#define DemuxerName "Modplug Demuxer"
