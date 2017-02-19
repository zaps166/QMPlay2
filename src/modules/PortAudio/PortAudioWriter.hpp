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

#include <Writer.hpp>

#include <PortAudioCommon.hpp>
#include <portaudio.h>

#include <QCoreApplication>

class PortAudioWriter : public Writer
{
	Q_DECLARE_TR_FUNCTIONS(PortAudioWriter)
public:
	PortAudioWriter(Module &);
private:
	~PortAudioWriter();

	bool set() override;

	bool readyWrite() const override final;

	bool processParams(bool *paramsCorrected) override;
	qint64 write(const QByteArray &) override;
	void pause() override;

	QString name() const override;

	bool open() override;

	/**/

	bool openStream();
	bool startStream();
	inline bool writeStream(const QByteArray &arr);
	qint64 playbackError();

#ifdef Q_OS_WIN
	bool isNoDriverError() const;
#endif
	bool reopenStream();

	void close();

	QString outputDevice;
	PaStreamParameters outputParameters;
	PaStream *stream;
	int sample_rate;
	double outputLatency;
	bool err, fullBufferReached;
	int underflows;
};

#define PortAudioWriterName "PortAudio"
