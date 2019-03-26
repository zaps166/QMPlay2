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

#include <FFDecVTB.hpp>

#include <HWAccelInterface.hpp>
#include <VideoWriter.hpp>
#include <StreamInfo.hpp>
#include <ImgScaler.hpp>
#include <FFCommon.hpp>

#include <QDebug>

extern "C"
{
    #include <libavcodec/avcodec.h>
    #include <libswscale/swscale.h>
    #include <libavcodec/videotoolbox.h>
}

#include <gl.h>

/**/

static AVPixelFormat getVTBFormat(AVCodecContext *ctx, const AVPixelFormat *fmt)
{
    Q_UNUSED(fmt)
    av_videotoolbox_default_free(ctx);
    const int ret = av_videotoolbox_default_init(ctx);
    if (ret < 0)
        return AV_PIX_FMT_NONE;
    return AV_PIX_FMT_VIDEOTOOLBOX;
};

/**/

class VTBHwaccel final : public HWAccelInterface
{
public:
    inline VTBHwaccel() :
        m_pixelBufferToRelease(nullptr),
        m_glTextures(nullptr)
    {}
    ~VTBHwaccel()
    {
        QMutexLocker locker(&m_buffersMutex);
        for (quintptr buffer : asConst(m_buffers))
            CVPixelBufferRelease((CVPixelBufferRef)buffer);
        CVPixelBufferRelease(m_pixelBufferToRelease);
    }

    QString name() const override
    {
        return "VideoToolBox";
    }

    Format getFormat() const override
    {
        return NV12;
    }
    bool isTextureRectangle() const override
    {
        return true;
    }

    bool canInitializeTextures() const override
    {
        return false;
    }

    bool init(quint32 *textures) override
    {
        m_glTextures = textures;
        return true;
    }
    void clear(bool contextChange) override
    {
        Q_UNUSED(contextChange)
        CVPixelBufferRelease(m_pixelBufferToRelease);
        m_pixelBufferToRelease = nullptr;
        m_glTextures = nullptr;
    }

    CopyResult copyFrame(const VideoFrame &videoFrame, Field field) override
    {
        Q_UNUSED(field)

        {
            QMutexLocker locker(&m_buffersMutex);
            const int idx = m_buffers.indexOf(videoFrame.surfaceId);
            if (idx < 0)
                return CopyNotReady;
            m_buffers.removeAt(idx);
            while (m_buffers.size() > 5)
                CVPixelBufferRelease((CVPixelBufferRef)m_buffers.takeFirst());
        }

        CVPixelBufferRef pixelBuffer = (CVPixelBufferRef)videoFrame.surfaceId;
        CGLContextObj glCtx = CGLGetCurrentContext();

        IOSurfaceRef surface = CVPixelBufferGetIOSurface(pixelBuffer);

        const OSType pixelFormat = IOSurfaceGetPixelFormat(surface);
        if (pixelFormat != kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange)
        {
            CVPixelBufferRelease(pixelBuffer);
            return CopyError;
        }

        glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_glTextures[0]);
        if (CGLTexImageIOSurface2D(glCtx, GL_TEXTURE_RECTANGLE_ARB, GL_R8, videoFrame.size.getWidth(0), videoFrame.size.getHeight(0), GL_RED, GL_UNSIGNED_BYTE, surface, 0) != kCGLNoError)
        {
            CVPixelBufferRelease(pixelBuffer);
            return CopyError;
        }

        glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_glTextures[1]);
        if (CGLTexImageIOSurface2D(glCtx, GL_TEXTURE_RECTANGLE_ARB, GL_RG8, videoFrame.size.getWidth(1), videoFrame.size.getHeight(1), GL_RG, GL_UNSIGNED_BYTE, surface, 1) != kCGLNoError)
        {
            CVPixelBufferRelease(pixelBuffer);
            return CopyError;
        }

        CVPixelBufferRelease(m_pixelBufferToRelease);
        m_pixelBufferToRelease = pixelBuffer;

        return CopyOk;
    }

    bool getImage(const VideoFrame &videoFrame, void *dest, ImgScaler *nv12ToRGB32) override
    {
        {
            QMutexLocker locker(&m_buffersMutex);
            if (m_buffers.indexOf(videoFrame.surfaceId) < 0)
                return false;
        }

        CVPixelBufferRef pixelBuffer = (CVPixelBufferRef)videoFrame.surfaceId;
        if (CVPixelBufferGetPixelFormatType(pixelBuffer) == kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange)
        {
            CVPixelBufferLockBaseAddress(pixelBuffer, kCVPixelBufferLock_ReadOnly);

            const quint8 *srcData[2] = {
                (const quint8 *)CVPixelBufferGetBaseAddressOfPlane(pixelBuffer, 0),
                (const quint8 *)CVPixelBufferGetBaseAddressOfPlane(pixelBuffer, 1)
            };
            const qint32 srcLinesize[2] = {
                (qint32)CVPixelBufferGetBytesPerRowOfPlane(pixelBuffer, 0),
                (qint32)CVPixelBufferGetBytesPerRowOfPlane(pixelBuffer, 1)
            };

            nv12ToRGB32->scale((const void **)srcData, srcLinesize, dest);

            CVPixelBufferUnlockBaseAddress(pixelBuffer, kCVPixelBufferLock_ReadOnly);

            return true;
        }

        return false;
    }

    /**/

    void addBuffer(CVPixelBufferRef pixelBuffer)
    {
        QMutexLocker locker(&m_buffersMutex);
        m_buffers.append((quintptr)pixelBuffer);
    }

private:
    CVPixelBufferRef m_pixelBufferToRelease;
    QList<quintptr> m_buffers;
    QMutex m_buffersMutex;
    quint32 *m_glTextures;
};

/**/

FFDecVTB::FFDecVTB(Module &module) :
    m_swsCtx(nullptr),
    m_copyVideo(false),
    m_hasCriticalError(false)
{
    SetModule(module);
}
FFDecVTB::~FFDecVTB()
{
    if (codecIsOpen)
    {
        avcodec_flush_buffers(codec_ctx);
        av_videotoolbox_default_free(codec_ctx);
    }
    if (m_swsCtx)
        sws_freeContext(m_swsCtx);
}

bool FFDecVTB::set()
{
    const bool copyVideo = sets().getBool("CopyVideoVTB");
    if (copyVideo != m_copyVideo)
    {
        m_copyVideo = copyVideo;
        return false;
    }
    return sets().getBool("DecoderVTBEnabled");
}

QString FFDecVTB::name() const
{
    return "FFmpeg/VideoToolBox";
}

int FFDecVTB::decodeVideo(Packet &encodedPacket, VideoFrame &decoded, QByteArray &newPixFmt, bool flush, unsigned hurryUp)
{
    const int ret = FFDecHWAccel::decodeVideo(encodedPacket, decoded, newPixFmt, flush, hurryUp);
    if (m_hwAccelWriter && decoded.surfaceId != 0)
    {
        CVPixelBufferRef pixelBuffer = CVPixelBufferRetain((CVPixelBufferRef)decoded.surfaceId);
        ((VTBHwaccel *)m_hwAccelWriter->getHWAccelInterface())->addBuffer(pixelBuffer);
        decoded.surfaceId = (quintptr)pixelBuffer;
    }
    m_hasCriticalError = (ret < 0 && !codec_ctx->hwaccel);
    return ret;
}
void FFDecVTB::downloadVideoFrame(VideoFrame &decoded)
{
    CVPixelBufferRef pixelBuffer = (CVPixelBufferRef)frame->data[3];
    if (!pixelBuffer)
        return;

    if (CVPixelBufferGetPixelFormatType(pixelBuffer) == kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange)
    {
        const size_t w = CVPixelBufferGetWidth(pixelBuffer);
        const size_t h = CVPixelBufferGetHeight(pixelBuffer);

        CVPixelBufferLockBaseAddress(pixelBuffer, kCVPixelBufferLock_ReadOnly);

        const quint8 *srcData[2] = {
            (const quint8 *)CVPixelBufferGetBaseAddressOfPlane(pixelBuffer, 0),
            (const quint8 *)CVPixelBufferGetBaseAddressOfPlane(pixelBuffer, 1)
        };
        const qint32 srcLinesize[2] = {
            (qint32)CVPixelBufferGetBytesPerRowOfPlane(pixelBuffer, 0),
            (qint32)CVPixelBufferGetBytesPerRowOfPlane(pixelBuffer, 1)
        };

        AVBufferRef *dstBuffer[3] = {
            av_buffer_alloc(srcLinesize[0] * h),
            av_buffer_alloc((srcLinesize[1] / 2) * ((h + 1) / 2)),
            av_buffer_alloc((srcLinesize[1] / 2) * ((h + 1) / 2))
        };

        quint8 *dstData[3] = {
            dstBuffer[0]->data,
            dstBuffer[1]->data,
            dstBuffer[2]->data
        };
        const qint32 dstLinesize[3] = {
            srcLinesize[0],
            srcLinesize[1] / 2,
            srcLinesize[1] / 2
        };

        m_swsCtx = sws_getCachedContext(m_swsCtx, w, h, AV_PIX_FMT_NV12, w, h, AV_PIX_FMT_YUV420P, SWS_POINT, nullptr, nullptr, nullptr);
        sws_scale(m_swsCtx, srcData, srcLinesize, 0, h, dstData, dstLinesize);

        CVPixelBufferUnlockBaseAddress(pixelBuffer, kCVPixelBufferLock_ReadOnly);

        decoded = VideoFrame(VideoFrameSize(w, h), dstBuffer, dstLinesize, frame->interlaced_frame, frame->top_field_first);
    }
}

bool FFDecVTB::hasCriticalError() const
{
    return m_hasCriticalError;
}

bool FFDecVTB::open(StreamInfo &streamInfo, VideoWriter *writer)
{
    if (streamInfo.type != QMPLAY2_TYPE_VIDEO)
        return false;
    AVCodec *codec = init(streamInfo);
    if (!codec || !hasHWAccel("videotoolbox"))
        return false;

    VTBHwaccel *vtbHwaccel = nullptr;
    if (writer)
    {
        vtbHwaccel = dynamic_cast<VTBHwaccel *>(writer->getHWAccelInterface());
        if (vtbHwaccel)
            m_hwAccelWriter = writer;
    }

    codec_ctx->get_format = getVTBFormat;
    codec_ctx->thread_count = 1;

    if (!openCodec(codec))
        return false;

    if (!m_copyVideo && !m_hwAccelWriter)
    {
        m_hwAccelWriter = VideoWriter::createOpenGL2(new VTBHwaccel);
        if (!m_hwAccelWriter)
            return false;
    }

    time_base = streamInfo.getTimeBase();
    return true;
}
