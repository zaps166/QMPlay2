/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2020  Błażej Szczygieł

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

#include "CuvidAPI.hpp"

#include <HWAccelInterface.hpp>

#include <QSet>

class CuvidOpenGL : public HWAccelInterface
{
public:
    CuvidOpenGL(CUcontext cuCtx);
    ~CuvidOpenGL() final;

    QString name() const override;

    Format getFormat() const override;

    bool init(const int *widths, const int *heights, const SetTextureParamsFn &setTextureParamsFn) override;
    void clear() override;

    MapResult mapFrame(Frame &videoFrame) override;
    quint32 getTexture(int plane) override;

    bool getImage(const Frame &videoFrame, void *dest, ImgScaler *nv12ToRGB32) override;

    /**/

    inline void allowDestroyCuda()
    {
        m_canDestroyCuda = true;
    }

    inline CUcontext getCudaContext() const
    {
        return m_cuCtx;
    }

    void setAvailableSurface(quintptr surfaceId);

    void setDecoderAndCodedHeight(CUvideodecoder cuvidDec, int codedHeight);

private:
    bool m_canDestroyCuda = false;

    int m_codedHeight = 0;

    quint32 m_textures[2] = {};

    int m_widths[2] = {};
    int m_heights[2] = {};

    const CUcontext m_cuCtx;
    CUvideodecoder m_cuvidDec = nullptr;

    CUgraphicsResource m_res[2] = {};

    QSet<int> m_validPictures;
};
