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

class Reader;

class PCM : public Demuxer
{
public:
	enum FORMAT {PCM_U8, PCM_S8, PCM_S16, PCM_S24, PCM_S32, PCM_FLT, FORMAT_COUNT};

	PCM(Module &);
private:
	bool set() override final;

	QString name() const override final;
	QString title() const override final;
	double length() const override final;
	int bitrate() const override final;

	bool seek(double, bool) override final;
	bool read(Packet &, int &) override final;
	void abort() override final;

	bool open(const QString &) override final;

	/**/

	IOController<Reader> reader;

	double len;
	FORMAT fmt;
	unsigned char chn;
	int srate, offset;
	bool bigEndian;
};

#define PCMName "PCM Audio"
