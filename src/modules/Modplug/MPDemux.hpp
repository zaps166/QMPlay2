/*
	QMPlay2 is a video and audio player.
	Copyright (C) 2010-2016  Błażej Szczygieł

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

#include <Demuxer.hpp>

#include <IOController.hpp>

struct _ModPlugFile;
class Reader;

class MPDemux : public Demuxer
{
	Q_DECLARE_TR_FUNCTIONS(MPDemux)
public:
	MPDemux(Module &);
private:
	~MPDemux();

	bool set();

	QString name() const;
	QString title() const;
	QList<QMPlay2Tag> tags() const;
	double length() const;
	int bitrate() const;

	bool seek(int, bool);
	bool read(Packet &, int &);
	void abort();

	bool open(const QString &);

	/**/

	bool aborted;
	double pos;
	quint32 srate;
	_ModPlugFile *mpfile;
	IOController<Reader> reader;
};

#define DemuxerName "Modplug Demuxer"
