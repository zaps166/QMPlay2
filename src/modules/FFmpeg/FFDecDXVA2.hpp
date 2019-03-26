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

#include <FFDecHWAccel.hpp>

extern "C"
{
    #include <libavcodec/dxva2.h>
}

#include <memory>

class DXVA2Surfaces;
struct SwsContext;

class FFDecDXVA2 final : public FFDecHWAccel
{
public:
    using Surfaces = std::shared_ptr<QVector<IDirect3DSurface9 *>>;

    static bool loadLibraries();

    FFDecDXVA2(Module &module);
    ~FFDecDXVA2();

    bool set() override;

    QString name() const override;

    void downloadVideoFrame(VideoFrame &decoded) override;

    bool open(StreamInfo &streamInfo, VideoWriter *writer) override;

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
