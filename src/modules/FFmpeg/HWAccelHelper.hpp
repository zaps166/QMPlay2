/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2019  Błażej Szczygieł

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

#include <QQueue>

extern "C"
{
    #include <libavutil/pixfmt.h>
}

using QMPlay2SurfaceID = quintptr;
using SurfacesQueue = QQueue<QMPlay2SurfaceID>;

#define QMPlay2InvalidSurfaceID ((QMPlay2SurfaceID)-1)

struct AVCodecContext;
struct AVFrame;

class HWAccelHelper
{
public:
    static AVPixelFormat getFormat(AVCodecContext *codecCtx, const AVPixelFormat *pixFmt);
    static int getBuffer(AVCodecContext *codecCtx, AVFrame *frame, int flags);

    HWAccelHelper(AVCodecContext *codecCtx, AVPixelFormat pixFmt, void *hwAccelCtx, const SurfacesQueue &surfacesQueue);
    ~HWAccelHelper();

    inline AVPixelFormat getPixelFormat() const;

    inline QMPlay2SurfaceID getSurface();
    inline void putSurface(QMPlay2SurfaceID id);

private:
    SurfacesQueue m_surfacesQueue;
    const AVPixelFormat m_pixFmt;
};
