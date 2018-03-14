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

#include <QtGlobal>

#include <string.h>

class QMPLAY2SHAREDLIB_EXPORT VideoAdjustment
{
public:
	inline void unset()
	{
		brightness = contrast = saturation = hue = sharpness = (qint16)0x8000;
	}

	inline void zero()
	{
		memset(this, 0, sizeof *this);
	}

	inline bool operator !=(const VideoAdjustment &other) const
	{
		return memcmp(this, &other, sizeof *this);
	}

	qint16 brightness, contrast, saturation, hue, sharpness;
};
