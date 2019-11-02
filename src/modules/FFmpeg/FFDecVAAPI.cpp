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

#include <FFDecVAAPI.hpp>
#include <HWAccelInterface.hpp>
#include <VideoWriter.hpp>
#include <FFCommon.hpp>

#include <StreamInfo.hpp>

#include <QDebug>

extern "C"
{
    #include <libavformat/avformat.h>
    #include <libavutil/pixdesc.h>
    #include <libavutil/hwcontext.h>
    #include <libavutil/hwcontext_vaapi.h>
    #include <libswscale/swscale.h>
}

#include <va/va_glx.h>
#include <va/va_version.h>

#if defined(USE_OPENGL) && VA_CHECK_VERSION(1, 1, 0) // >= 2.1.0
#   define VAAPI_HAS_ESH

#   include <QOpenGLContext>

#   include <va/va_drmcommon.h>
#   include <unistd.h>

#   include <EGL/egl.h>
#   include <EGL/eglext.h>

#   include <GL/gl.h>

#   ifndef EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT
#       define EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT 0x3443
#   endif
#   ifndef EGL_DMA_BUF_PLANE0_MODIFIER_HI_EXT
#       define EGL_DMA_BUF_PLANE0_MODIFIER_HI_EXT 0x3444
#   endif
#endif

class VAAPIOpenGL : public HWAccelInterface
{
public:
    VAAPIOpenGL(const std::shared_ptr<VAAPI> &vaapi)
        : m_vaapi(vaapi)
        , m_isEGL(m_vaapi->m_fd > -1)
    {}
    ~VAAPIOpenGL() final
    {}

    QString name() const override
    {
        QString name(VAAPIWriterName);
        if (!m_isEGL)
            name += " (GLX)";
        return name;
    }

    Format getFormat() const override
    {
        return m_isEGL ? NV12 : RGB32;
    }
    bool isCopy() const override
    {
        return m_isEGL ? false : true;
    }

    bool canInitializeTextures() const override
    {
        return true;
    }

    bool init(quint32 *textures) override
    {
        if (!m_isEGL)
            return (vaCreateSurfaceGLX(m_vaapi->VADisp, GL_TEXTURE_2D, *textures, &m_glSurface) == VA_STATUS_SUCCESS);

#ifdef VAAPI_HAS_ESH
        const auto context = QOpenGLContext::currentContext();
        if (!context)
        {
            QMPlay2Core.logError("VA-API :: Unable to get OpenGL context");
            return false;
        }

        m_eglDpy = eglGetCurrentDisplay();
        if (!m_eglDpy)
        {
            QMPlay2Core.logError("VA-API :: Unable to get EGL display");
            return false;
        }

        const auto extensionsRaw = eglQueryString(m_eglDpy, EGL_EXTENSIONS);
        if (!extensionsRaw)
        {
            QMPlay2Core.logError("VA-API :: Unable to get EGL extensions");
            return false;
        }

        const auto extensions = QByteArray::fromRawData(extensionsRaw, qstrlen(extensionsRaw));
        if (!extensions.contains("EGL_EXT_image_dma_buf_import"))
        {
            QMPlay2Core.logError("VA-API :: EGL_EXT_image_dma_buf_import extension is not available");
            return false;
        }

        eglCreateImageKHR = (PFNEGLCREATEIMAGEKHRPROC)context->getProcAddress("eglCreateImageKHR");
        eglDestroyImageKHR = (PFNEGLDESTROYIMAGEKHRPROC)context->getProcAddress("eglDestroyImageKHR");
        glEGLImageTargetTexture2DOES = (PFNGLEGLIMAGETARGETTEXTURE2DOESPROC)context->getProcAddress("glEGLImageTargetTexture2DOES");
        if (!eglCreateImageKHR || !eglDestroyImageKHR || !glEGLImageTargetTexture2DOES)
        {
            QMPlay2Core.logError("VA-API :: Unable to get EGL function pointers");
            return false;
        }

        m_hasDmaBufImportModifiers = extensions.contains("EGL_EXT_image_dma_buf_import_modifiers");

        m_textures = textures;

        return true;
#else
        return false;
#endif
    }
    void clear(bool contextChange) override
    {
        Q_UNUSED(contextChange)

#ifdef VAAPI_HAS_ESH
        if (m_isEGL)
        {
            m_eglDpy = EGL_NO_DISPLAY;

            eglCreateImageKHR = nullptr;
            eglDestroyImageKHR = nullptr;
            glEGLImageTargetTexture2DOES = nullptr;

            m_hasDmaBufImportModifiers = false;

            m_textures = nullptr;
        }
#endif
        if (m_glSurface)
        {
            vaDestroySurfaceGLX(m_vaapi->VADisp, m_glSurface);
            m_glSurface = nullptr;
        }
    }

    MapResult mapFrame(const VideoFrame &videoFrame, Field field) override
    {
        VASurfaceID id;
        int vaField = field; // VA-API field codes are compatible with "HWAccelInterface::Field" codes.
        if (!m_vaapi->filterVideo(videoFrame, id, vaField))
            return MapNotReady;

        if (!m_isEGL)
        {
            if (vaCopySurfaceGLX(m_vaapi->VADisp, m_glSurface, id, vaField) == VA_STATUS_SUCCESS)
                return MapOk;
            return MapError;
        }

#ifdef VAAPI_HAS_ESH
        VADRMPRIMESurfaceDescriptor vaSurfaceDescr = {};
        if (vaExportSurfaceHandle(
                m_vaapi->VADisp,
                id,
                VA_SURFACE_ATTRIB_MEM_TYPE_DRM_PRIME_2,
                VA_EXPORT_SURFACE_READ_ONLY | VA_EXPORT_SURFACE_SEPARATE_LAYERS,
                &vaSurfaceDescr
            ) != VA_STATUS_SUCCESS)
        {
            QMPlay2Core.logError("VA-API :: Unable to export surface handle");
            return MapError;
        }

        auto closeFDs = [&] {
            for (uint32_t o = 0; o < vaSurfaceDescr.num_objects; ++o)
                ::close(vaSurfaceDescr.objects[o].fd);
        };

        if (vaSyncSurface(m_vaapi->VADisp, id) != VA_STATUS_SUCCESS)
        {
            QMPlay2Core.logError("VA-API :: Unable to sync surface");
            closeFDs();
            return MapError;
        }

        for (uint32_t p = 0; p < vaSurfaceDescr.num_layers; ++p)
        {
            const auto &layer = vaSurfaceDescr.layers[p];
            const auto &object = vaSurfaceDescr.objects[layer.object_index[0]];

            EGLint attribs[] = {
                EGL_LINUX_DRM_FOURCC_EXT, (EGLint)layer.drm_format,
                EGL_WIDTH, (EGLint)videoFrame.size.getWidth(p),
                EGL_HEIGHT, (EGLint)videoFrame.size.getHeight(p),
                EGL_DMA_BUF_PLANE0_FD_EXT, object.fd,
                EGL_DMA_BUF_PLANE0_OFFSET_EXT, (EGLint)layer.offset[0],
                EGL_DMA_BUF_PLANE0_PITCH_EXT, (EGLint)layer.pitch[0],
                EGL_NONE, 0,
                EGL_NONE, 0,
                EGL_NONE
            };
            if (m_hasDmaBufImportModifiers)
            {
                attribs[12] = EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT;
                attribs[13] = static_cast<EGLint>(object.drm_format_modifier);
                attribs[14] = EGL_DMA_BUF_PLANE0_MODIFIER_HI_EXT;
                attribs[15] = static_cast<EGLint>(object.drm_format_modifier >> 32);
            }

            const auto image = eglCreateImageKHR(
                m_eglDpy,
                EGL_NO_CONTEXT,
                EGL_LINUX_DMA_BUF_EXT,
                nullptr,
                attribs
            );
            if (!image)
            {
                QMPlay2Core.logError("VA-API :: Unable to create EGL image");
                closeFDs();
                return MapError;
            }

            glBindTexture(GL_TEXTURE_2D, m_textures[p]);
            glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, image);

            eglDestroyImageKHR(m_eglDpy, image);
        }

        closeFDs();
        return MapOk;
#else
        return MapError;
#endif
    }

    bool getImage(const VideoFrame &videoFrame, void *dest, ImgScaler *nv12ToRGB32) override
    {
        return m_vaapi->getImage(videoFrame, dest, nv12ToRGB32);
    }

    void getVideAdjustmentCap(VideoAdjustment &videoAdjustmentCap) override
    {
        videoAdjustmentCap.brightness = false;
        videoAdjustmentCap.contrast = false;
        videoAdjustmentCap.saturation = true;
        videoAdjustmentCap.hue = true;
        videoAdjustmentCap.sharpness = false;
    }
    void setVideoAdjustment(const VideoAdjustment &videoAdjustment) override
    {
        if (!m_isEGL)
            m_vaapi->applyVideoAdjustment(0, 0, videoAdjustment.saturation, videoAdjustment.hue);
    }

    /**/

    inline std::shared_ptr<VAAPI> getVAAPI() const
    {
        return m_vaapi;
    }

private:
    const std::shared_ptr<VAAPI> m_vaapi;
    const bool m_isEGL;

    // GLX
    void *m_glSurface = nullptr;

#ifdef VAAPI_HAS_ESH
    // EGL
    EGLDisplay m_eglDpy = EGL_NO_DISPLAY;

    PFNEGLCREATEIMAGEKHRPROC eglCreateImageKHR = nullptr;
    PFNEGLDESTROYIMAGEKHRPROC eglDestroyImageKHR = nullptr;
    PFNGLEGLIMAGETARGETTEXTURE2DOESPROC glEGLImageTargetTexture2DOES = nullptr;

    bool m_hasDmaBufImportModifiers = false;

    quint32 *m_textures = nullptr;
#endif
};

/**/

static AVPixelFormat vaapiGetFormat(AVCodecContext *codecCtx, const AVPixelFormat *pixFmt)
{
    Q_UNUSED(codecCtx)
    while (*pixFmt != AV_PIX_FMT_NONE)
    {
        if (*pixFmt == AV_PIX_FMT_VAAPI)
            return *pixFmt;
        ++pixFmt;
    }
    return AV_PIX_FMT_NONE;
}

/**/

FFDecVAAPI::FFDecVAAPI(Module &module)
{
    SetModule(module);
}
FFDecVAAPI::~FFDecVAAPI()
{
    if (codecIsOpen)
        avcodec_flush_buffers(codec_ctx);
    if (m_swsCtx)
        sws_freeContext(m_swsCtx);
    destroyDecoder(); // Destroy before deleting "m_vaapi"
}

bool FFDecVAAPI::set()
{
    bool ret = true;

#ifdef USE_OPENGL
    const bool copyVideo = sets().getBool("CopyVideoVAAPI");
    if (copyVideo != m_copyVideo)
    {
        m_copyVideo = copyVideo;
        ret = false;
    }

    switch (sets().getInt("VAAPIDeintMethod"))
    {
        case 0:
            m_vppDeintType = VAProcDeinterlacingBob;
            break;
        case 2:
            m_vppDeintType = VAProcDeinterlacingMotionCompensated;
            break;
        default:
            m_vppDeintType = VAProcDeinterlacingMotionAdaptive;
    }
    if (m_vaapi)
    {
        const bool reloadVpp = m_vaapi->ok && m_vaapi->use_vpp && (m_vaapi->vpp_deint_type != m_vppDeintType);
        m_vaapi->vpp_deint_type = m_vppDeintType;
        if (reloadVpp)
            m_vaapi->clearVPP(false);
    }
#else
    m_useOpenGL = false;
#endif

    return sets().getBool("DecoderVAAPIEnabled") && ret;
}

QString FFDecVAAPI::name() const
{
    return "FFmpeg/" VAAPIWriterName;
}

int FFDecVAAPI::decodeVideo(Packet &encodedPacket, VideoFrame &decoded, QByteArray &newPixFmt, bool flush, unsigned hurryUp)
{
    int ret = FFDecHWAccel::decodeVideo(encodedPacket, decoded, newPixFmt, flush, hurryUp);
    if (m_hwAccelWriter && ret > -1)
    {
        decoded.setAVFrame(frame);
        m_vaapi->maybeInitVPP(codec_ctx->coded_width, codec_ctx->coded_height);
    }

    return ret;
}
void FFDecVAAPI::downloadVideoFrame(VideoFrame &decoded)
{
    VAImage image;
    quint8 *vaData = m_vaapi->getNV12Image(image, (quintptr)frame->data[3]);
    if (vaData)
    {
        AVBufferRef *dstBuffer[3] = {
            av_buffer_alloc(image.pitches[0] * frame->height),
            av_buffer_alloc((image.pitches[1] / 2) * ((frame->height + 1) / 2)),
            av_buffer_alloc((image.pitches[1] / 2) * ((frame->height + 1) / 2))
        };

        quint8 *srcData[2] = {
            vaData + image.offsets[0],
            vaData + image.offsets[1]
        };
        qint32 srcLinesize[2] = {
            (qint32)image.pitches[0],
            (qint32)image.pitches[1]
        };

        uint8_t *dstData[3] = {
            dstBuffer[0]->data,
            dstBuffer[1]->data,
            dstBuffer[2]->data
        };
        qint32 dstLinesize[3] = {
            (qint32)image.pitches[0],
            (qint32)image.pitches[1] / 2,
            (qint32)image.pitches[1] / 2
        };

        m_swsCtx = sws_getCachedContext(m_swsCtx, frame->width, frame->height, AV_PIX_FMT_NV12, frame->width, frame->height, AV_PIX_FMT_YUV420P, SWS_POINT, nullptr, nullptr, nullptr);
        sws_scale(m_swsCtx, srcData, srcLinesize, 0, frame->height, dstData, dstLinesize);

        decoded = VideoFrame(VideoFrameSize(frame->width, frame->height), dstBuffer, dstLinesize, frame->interlaced_frame, frame->top_field_first);

        vaUnmapBuffer(m_vaapi->VADisp, image.buf);
        vaDestroyImage(m_vaapi->VADisp, image.image_id);
    }
}

bool FFDecVAAPI::open(StreamInfo &streamInfo, VideoWriter *writer)
{
    const AVPixelFormat pix_fmt = av_get_pix_fmt(streamInfo.format);
    if (pix_fmt != AV_PIX_FMT_YUV420P && pix_fmt != AV_PIX_FMT_YUVJ420P)
        return false;

    AVCodec *codec = init(streamInfo);
    if (!codec || !hasHWAccel("vaapi"))
        return false;

    if (writer) //Writer is already created
    {
        VAAPIOpenGL *vaapiOpenGL = dynamic_cast<VAAPIOpenGL *>(writer->getHWAccelInterface());
        if (vaapiOpenGL)
        {
            m_vaapi = vaapiOpenGL->getVAAPI();
            m_hwAccelWriter = writer;
        }
    }

    if (!m_vaapi)
    {
        m_vaapi = std::make_shared<VAAPI>();
        if (!m_vaapi->open(avcodec_get_name(codec_ctx->codec_id), !m_copyVideo))
            return false;
    }

    auto bufferRef = av_hwdevice_ctx_alloc(AV_HWDEVICE_TYPE_VAAPI);
    if (!bufferRef)
        return false;

    auto vaapiDevCtx = (AVVAAPIDeviceContext *)((AVHWDeviceContext *)bufferRef->data)->hwctx;
    vaapiDevCtx->display = m_vaapi->VADisp;
    if (av_hwdevice_ctx_init(bufferRef) != 0)
    {
        av_buffer_unref(&bufferRef);
        return false;
    }

    if (!m_hwAccelWriter && !m_copyVideo)
    {
        auto vaapiOpengGL = new VAAPIOpenGL(m_vaapi);
        m_hwAccelWriter = VideoWriter::createOpenGL2(vaapiOpengGL);
        if (!m_hwAccelWriter)
        {
            av_buffer_unref(&bufferRef);
            return false;
        }
        m_vaapi->vpp_deint_type = m_vppDeintType;
    }

    m_vaapi->init(codec_ctx->width, codec_ctx->height, !m_copyVideo);

    codec_ctx->hw_device_ctx = bufferRef;
    codec_ctx->get_format = vaapiGetFormat;
    codec_ctx->thread_count = 1;
    codec_ctx->extra_hw_frames = 3;
    if (!openCodec(codec))
    {
        av_buffer_unref(&bufferRef);
        return false;
    }

    time_base = streamInfo.getTimeBase();
    return true;
}
