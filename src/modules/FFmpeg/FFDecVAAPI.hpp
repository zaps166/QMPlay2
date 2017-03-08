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

#include <FFDecHWAccel.hpp>
#include <VAAPI.hpp>

struct SwsContext;

class FFDecVAAPI : public FFDecHWAccel
{
public:
	FFDecVAAPI(QMutex &, Module &);
	~FFDecVAAPI() final;

	bool set() override final;

	QString name() const override final;

	void downloadVideoFrame(VideoFrame &decoded) override final;

	bool open(StreamInfo &, VideoWriter *) override final;

private:
	bool m_useOpenGL, m_allowVDPAU;
	Qt::CheckState m_copyVideo;
#ifdef HAVE_VPP
	VAProcDeinterlacingType m_vppDeintType;
#endif
	VAAPI *m_vaapi;
	SwsContext *m_swsCtx;
};
