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

#ifndef FFDECDXVA2_HPP
#define FFDECDXVA2_HPP

#include <FFDecHWAccel.hpp>

extern "C"
{
	#include <libavcodec/dxva2.h>
}

#include <QSharedPointer>

class DXVA2Surfaces;
struct SwsContext;

class FFDecDXVA2 : public FFDecHWAccel
{
public:
	typedef QSharedPointer<QVector<IDirect3DSurface9 *> > Surfaces;

	static bool loadLibraries();

	FFDecDXVA2(QMutex &avcodec_mutex, Module &module);
	~FFDecDXVA2();

	bool set();

	QString name() const;

	void downloadVideoFrame(VideoFrame &decoded);

	bool open(StreamInfo &streamInfo, VideoWriter *writer);

private:
	Qt::CheckState m_copyVideo;

	Surfaces m_surfaces;

	IDirect3DDevice9 *m_d3d9Device;
	IDirect3DDeviceManager9 *m_devMgr;
	HANDLE m_devHandle;
	IDirectXVideoDecoderService *m_decoderService;
	IDirectXVideoDecoder *m_videoDecoder;
	DXVA2_ConfigPictureDecode m_config;

	SwsContext *m_swsCtx;
};

#endif // FFDECDXVA2_HPP
