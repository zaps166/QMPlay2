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

#include <QMPlay2_OSD.hpp>

#include <QCryptographicHash>

void QMPlay2_OSD::genChecksum()
{
	QCryptographicHash hash(QCryptographicHash::Md4);
	foreach (const Image &img, images)
		hash.addData(img.data);
	checksum = hash.result();
}

void QMPlay2_OSD::clear(bool all)
{
	images.clear();
	_text.clear();
	if (all)
		_pts = _duration = -1.0;
	_needsRescale = started = false;
	checksum.clear();
}

void QMPlay2_OSD::start()
{
	started = true;
	if (_pts == -1.0)
		timer.start();
}
double QMPlay2_OSD::left_duration()
{
	if (!started || _pts != -1.0)
		return 0.0;
	return _duration - timer.elapsed() / 1000.0;
}
