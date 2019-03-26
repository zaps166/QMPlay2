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

#include <HWAccelHelper.hpp>

extern "C"
{
    #include <libavcodec/avcodec.h>
}

static void releaseBuffer(HWAccelHelper *hwAccelHelper, QMPlay2SurfaceID surfaceID)
{
    if (surfaceID != QMPlay2InvalidSurfaceID)
        hwAccelHelper->putSurface(surfaceID);
}

using ReleaseBufferProc = void (*)(void *, uint8_t *);

/**/

AVPixelFormat HWAccelHelper::getFormat(AVCodecContext *codecCtx, const AVPixelFormat *pixFmt)
{
    Q_UNUSED(pixFmt)
    return ((HWAccelHelper *)codecCtx->opaque)->getPixelFormat();
}
int HWAccelHelper::getBuffer(AVCodecContext *codecCtx, AVFrame *frame, int flags)
{
    Q_UNUSED(flags)
    const QMPlay2SurfaceID surface_id = ((HWAccelHelper *)codecCtx->opaque)->getSurface();
    if (surface_id != QMPlay2InvalidSurfaceID)
    {
        frame->data[3] = (uint8_t *)(uintptr_t)surface_id;
        frame->buf[0] = av_buffer_create(frame->data[3], 0, (ReleaseBufferProc)releaseBuffer, codecCtx->opaque, AV_BUFFER_FLAG_READONLY);
        return 0;
    }
    /* This should never happen */
    fprintf(stderr, "Surface queue is empty!\n");
    return -1;
}

HWAccelHelper::HWAccelHelper(AVCodecContext *codecCtx, AVPixelFormat pixFmt, void *hwAccelCtx, const SurfacesQueue &surfacesQueue) :
    m_surfacesQueue(surfacesQueue),
    m_pixFmt(pixFmt)
{
    codecCtx->opaque          = this;
    codecCtx->get_format      = getFormat;
    codecCtx->get_buffer2     = getBuffer;
    codecCtx->hwaccel_context = hwAccelCtx;
    codecCtx->thread_count    = 1;
}
HWAccelHelper::~HWAccelHelper()
{}

inline AVPixelFormat HWAccelHelper::getPixelFormat() const
{
    return m_pixFmt;
}

inline QMPlay2SurfaceID HWAccelHelper::getSurface()
{
    return m_surfacesQueue.isEmpty() ? QMPlay2InvalidSurfaceID : m_surfacesQueue.dequeue();
}
inline void HWAccelHelper::putSurface(QMPlay2SurfaceID id)
{
    m_surfacesQueue.enqueue(id);
}
