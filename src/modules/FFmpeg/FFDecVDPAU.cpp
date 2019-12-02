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

#include <FFDecVDPAU.hpp>
#include <HWAccelInterface.hpp>
#include <VideoWriter.hpp>
#include <StreamInfo.hpp>
#include <Functions.hpp>
#include <FFCommon.hpp>
#include <VDPAU.hpp>

extern "C"
{
    #include <libavformat/avformat.h>
    #include <libavutil/pixdesc.h>
    #include <libavutil/hwcontext.h>
    #include <libavutil/hwcontext_vdpau.h>
}

#include <QOpenGLContext>
#include <QDebug>

class VDPAUOpenGL : public HWAccelInterface
{
public:
    VDPAUOpenGL(const std::shared_ptr<VDPAU> &vdpau)
        : m_vdpau(vdpau)
    {}
    ~VDPAUOpenGL() final
    {}

    QString name() const override
    {
        return VDPAUWriterName;
    }

    Format getFormat() const override
    {
        return RGB32;
    }

    bool init(const int *widths, const int *heights, const SetTextureParamsFn &setTextureParamsFn) override
    {
        if (m_width != widths[0] || m_height != heights[0])
        {
            clearTextures();

            m_width = widths[0];
            m_height = heights[0];

            glGenTextures(1, &m_texture);
        }

        setTextureParamsFn(m_texture);

        if (m_isInitialized)
            return true;

        const auto context = QOpenGLContext::currentContext();
        if (!context)
        {
            QMPlay2Core.logError("VDPAU :: Unable to get OpenGL context");
            return false;
        }

        if (!context->extensions().contains("GL_NV_vdpau_interop"))
        {
            QMPlay2Core.logError("VDPAU :: GL_NV_vdpau_interop extension is not available");
            return false;
        }

        VDPAUInitNV = (PFNVDPAUInitNVPROC)context->getProcAddress("VDPAUInitNV");
        VDPAUFiniNV = (PFNVDPAUFiniNVPROC)context->getProcAddress("VDPAUFiniNV");
        VDPAURegisterOutputSurfaceNV = (PFNVDPAURegisterSurfaceNVPROC)context->getProcAddress("VDPAURegisterOutputSurfaceNV");
        VDPAUUnregisterSurfaceNV = (PFNVDPAUUnregisterSurfaceNVPROC)context->getProcAddress("VDPAUUnregisterSurfaceNV");
        VDPAUSurfaceAccessNV = (PFNVDPAUSurfaceAccessNVPROC)context->getProcAddress("VDPAUSurfaceAccessNV");
        VDPAUMapSurfacesNV = (PFNVDPAUMapUnmapSurfacesNVPROC)context->getProcAddress("VDPAUMapSurfacesNV");
        VDPAUUnmapSurfacesNV = (PFNVDPAUMapUnmapSurfacesNVPROC)context->getProcAddress("VDPAUUnmapSurfacesNV");
        if (!VDPAUInitNV || !VDPAUFiniNV || !VDPAURegisterOutputSurfaceNV || !VDPAUUnregisterSurfaceNV || !VDPAUSurfaceAccessNV || !VDPAUMapSurfacesNV || !VDPAUUnmapSurfacesNV)
        {
            QMPlay2Core.logError("VDPAU :: Unable to get VDPAU interop function pointers");
            return false;
        }

        VDPAUInitNV(m_vdpau->m_device, m_vdpau->vdp_get_proc_address);
        if (glGetError() != 0)
        {
            QMPlay2Core.logError("VDPAU :: Unable to initialize VDPAU <-> GL interop");
            return false;
        }

        m_isInitialized = true;
        return true;
    }
    void clear() override
    {
        clearTextures();

        if (!m_isInitialized)
            return;

        VDPAUFiniNV();

        VDPAUInitNV = nullptr;
        VDPAUFiniNV = nullptr;
        VDPAURegisterOutputSurfaceNV = nullptr;
        VDPAUUnregisterSurfaceNV = nullptr;
        VDPAUSurfaceAccessNV = nullptr;
        VDPAUMapSurfacesNV = nullptr;
        VDPAUUnmapSurfacesNV = nullptr;

        m_isInitialized = false;
    }

    MapResult mapFrame(const VideoFrame &videoFrame, Field field) override
    {
        maybeUnmapOutputSurface();

        VdpOutputSurface id = 0;
        VdpVideoMixerPictureStructure videoMixerPictureStructure = VDP_VIDEO_MIXER_PICTURE_STRUCTURE_FRAME;
        switch (field)
        {
            case Field::TopField:
                videoMixerPictureStructure = VDP_VIDEO_MIXER_PICTURE_STRUCTURE_TOP_FIELD;
                break;
            case Field::BottomField:
                videoMixerPictureStructure = VDP_VIDEO_MIXER_PICTURE_STRUCTURE_BOTTOM_FIELD;
                break;
            default:
                break;
        }
        if (!m_vdpau->videoMixerRender(videoFrame, id, videoMixerPictureStructure))
            return MapError;
        if (id == VDP_INVALID_HANDLE)
            return MapNotReady;

        if (id != m_registeredOutputSurface)
        {
            maybeUnregisterOutputSurface();

            m_glSurface = VDPAURegisterOutputSurfaceNV(id, GL_TEXTURE_2D, 1, &m_texture);
            if (!m_glSurface)
                return MapError;

            VDPAUSurfaceAccessNV(m_glSurface, GL_READ_ONLY);
            m_registeredOutputSurface = id;
        }

        VDPAUMapSurfacesNV(1, &m_glSurface);
        if (glGetError() != 0)
            return MapError;

        m_isSurfaceMapped = true;
        return MapOk;
    }
    quint32 getTexture(int plane) override
    {
        Q_UNUSED(plane)
        return m_texture;
    }

    bool getImage(const VideoFrame &videoFrame, void *dest, ImgScaler *nv12ToRGB32) override
    {
        Q_UNUSED(nv12ToRGB32) // FIXME: Don't use ImgScaler in VideoThe if not needed
        return m_vdpau->getRGB((uint8_t *)dest, videoFrame.size);
    }

    void getVideAdjustmentCap(VideoAdjustment &videoAdjustmentCap) override
    {
        videoAdjustmentCap.brightness = false;
        videoAdjustmentCap.contrast = false;
        videoAdjustmentCap.saturation = true;
        videoAdjustmentCap.hue = true;
        videoAdjustmentCap.sharpness = true;
    }
    void setVideoAdjustment(const VideoAdjustment &videoAdjustment) override
    {
        m_vdpau->applyVideoAdjustment(videoAdjustment.saturation, videoAdjustment.hue, videoAdjustment.sharpness);
    }

    /**/

    void maybeUnmapOutputSurface()
    {
        if (!m_isSurfaceMapped)
            return;

        VDPAUUnmapSurfacesNV(1, &m_glSurface);
        m_isSurfaceMapped = false;
    }
    void maybeUnregisterOutputSurface()
    {
        if (!m_glSurface)
            return;

        VDPAUUnregisterSurfaceNV(m_glSurface);
        m_registeredOutputSurface = VDP_INVALID_HANDLE;
        m_glSurface = 0;
    }

    inline std::shared_ptr<VDPAU> getVDPAU() const
    {
        return m_vdpau;
    }

private:
    void clearTextures()
    {
        maybeUnmapOutputSurface();
        maybeUnregisterOutputSurface();
        if (m_texture)
        {
            glDeleteTextures(1, &m_texture);
            m_texture = 0;
        }
        m_width = m_height = 0;
    }

private:
    using GLvdpauSurfaceNV = GLintptr;

    std::shared_ptr<VDPAU> m_vdpau;

    bool m_isInitialized = false;
    uint32_t m_texture = 0;

    int m_width = 0;
    int m_height = 0;

    VdpOutputSurface m_registeredOutputSurface = VDP_INVALID_HANDLE;
    GLvdpauSurfaceNV m_glSurface = 0;
    bool m_isSurfaceMapped = false;

    using PFNVDPAUInitNVPROC = void(*)(uintptr_t vdpDevice, VdpGetProcAddress getProcAddress);
    PFNVDPAUInitNVPROC VDPAUInitNV = nullptr;

    using PFNVDPAUFiniNVPROC = void(*)();
    PFNVDPAUFiniNVPROC VDPAUFiniNV = nullptr;

    using PFNVDPAURegisterSurfaceNVPROC = GLvdpauSurfaceNV(*)(uintptr_t vdpSurface, GLenum target, GLsizei numTextureNames, const GLuint *textureNames);
    PFNVDPAURegisterSurfaceNVPROC VDPAURegisterOutputSurfaceNV = nullptr;

    using PFNVDPAUUnregisterSurfaceNVPROC = void(*)(GLvdpauSurfaceNV surface);
    PFNVDPAUUnregisterSurfaceNVPROC VDPAUUnregisterSurfaceNV = nullptr;

    using PFNVDPAUSurfaceAccessNVPROC = void(*)(GLvdpauSurfaceNV surface, GLenum access);
    PFNVDPAUSurfaceAccessNVPROC VDPAUSurfaceAccessNV = nullptr;

    using PFNVDPAUMapUnmapSurfacesNVPROC = void(*)(GLsizei numSurfaces, const GLvdpauSurfaceNV *surfaces);
    PFNVDPAUMapUnmapSurfacesNVPROC VDPAUMapSurfacesNV = nullptr;
    PFNVDPAUMapUnmapSurfacesNVPROC VDPAUUnmapSurfacesNV = nullptr;
};

/**/

static inline void YUVjToYUV(AVPixelFormat &pixFmt)
{
    // FFmpeg VDPAU implementation doesn't support YUVJ
    if (pixFmt == AV_PIX_FMT_YUVJ420P)
        pixFmt = AV_PIX_FMT_YUV420P;
}

static AVPixelFormat vdpauGetFormat(AVCodecContext *codecCtx, const AVPixelFormat *pixFmt)
{
    while (*pixFmt != AV_PIX_FMT_NONE)
    {
        if (*pixFmt == AV_PIX_FMT_VDPAU)
        {
            YUVjToYUV(codecCtx->sw_pix_fmt);
            return *pixFmt;
        }
        ++pixFmt;
    }
    return AV_PIX_FMT_NONE;
}

/**/

FFDecVDPAU::FFDecVDPAU(Module &module)
{
    SetModule(module);
}
FFDecVDPAU::~FFDecVDPAU()
{}

bool FFDecVDPAU::set()
{
    bool ret = true;

    const bool copyVideo = sets().getBool("CopyVideoVDPAU");
    if (m_copyVideo != copyVideo)
        ret = false;
    m_copyVideo = copyVideo;

    m_deintMethod = sets().getInt("VDPAUDeintMethod");
    m_nrEnabled = sets().getBool("VDPAUNoiseReductionEnabled");
    m_nrLevel = sets().getDouble("VDPAUNoiseReductionLvl");

    if (m_vdpau)
        m_vdpau->setVideoMixerDeintNr(m_deintMethod, m_nrEnabled, m_nrLevel);

    return (sets().getBool("DecoderVDPAUEnabled") && ret);
}

QString FFDecVDPAU::name() const
{
    return "FFmpeg/" VDPAUWriterName;
}

int FFDecVDPAU::decodeVideo(Packet &encodedPacket, VideoFrame &decoded, QByteArray &newPixFmt, bool flush, unsigned hurryUp)
{
    int ret = FFDecHWAccel::decodeVideo(encodedPacket, decoded, newPixFmt, flush, hurryUp);
    decoded.limited = m_limitedRange;
    if (m_hwAccelWriter && ret > -1)
    {
        if (flush)
            m_vdpau->clearBufferedFrames();
        if (!decoded.isEmpty())
            m_vdpau->maybeCreateVideoMixer(codec_ctx->coded_width, codec_ctx->coded_height, decoded);
    }
    return ret;
}
void FFDecVDPAU::downloadVideoFrame(VideoFrame &decoded)
{
    if (codec_ctx->coded_width <= 0 || codec_ctx->coded_height <= 0)
        return;

    const int32_t linesize[] = {
        codec_ctx->coded_width,
        (codec_ctx->coded_width + 1) / 2,
        (codec_ctx->coded_width + 1) / 2,
    };

    decoded = VideoFrame({codec_ctx->coded_width, codec_ctx->coded_height}, linesize, frame->interlaced_frame, frame->top_field_first);
    decoded.size.width = frame->width;
    decoded.size.height = frame->height;

    if (!m_vdpau->getYV12(decoded, (quintptr)frame->data[3]))
        decoded.clear();
}

bool FFDecVDPAU::open(StreamInfo &streamInfo, VideoWriter *writer)
{
    if (!m_copyVideo && Functions::isX11EGL())
        return false;

    const AVPixelFormat pix_fmt = av_get_pix_fmt(streamInfo.format);
    if (pix_fmt != AV_PIX_FMT_YUV420P && pix_fmt != AV_PIX_FMT_YUVJ420P)
        return false;

    AVCodec *codec = init(streamInfo);
    if (!codec || !hasHWAccel("vdpau"))
        return false;

    if (writer) // Writer is already created
    {
        if (auto vdpauOpenGL = dynamic_cast<VDPAUOpenGL *>(writer->getHWAccelInterface()))
        {
            m_vdpau = vdpauOpenGL->getVDPAU();
            m_hwAccelWriter = writer;
        }
    }

    AVBufferRef *hwDeviceBufferRef = nullptr;
    if (!m_vdpau)
    {
        if (av_hwdevice_ctx_create(&hwDeviceBufferRef, AV_HWDEVICE_TYPE_VDPAU, nullptr, nullptr, 0) != 0)
            return false;

        m_vdpau = std::make_shared<VDPAU>(hwDeviceBufferRef);
        if (!m_vdpau->init())
            return false;

        m_vdpau->registerPreemptionCallback(preemptionCallback, m_vdpau.get());
    }
    else
    {
        hwDeviceBufferRef = av_buffer_ref(m_vdpau->m_hwDeviceBufferRef);
    }

    if (!m_vdpau->checkCodec(streamInfo.codec_name.constData()))
        return false;

    if (!m_hwAccelWriter && !m_copyVideo)
    {
        m_hwAccelWriter = VideoWriter::createOpenGL2(new VDPAUOpenGL(m_vdpau));
        if (!m_hwAccelWriter)
            return false;
        m_vdpau->setVideoMixerDeintNr(m_deintMethod, m_nrEnabled, m_nrLevel);
    }

    YUVjToYUV(codec_ctx->pix_fmt);
    codec_ctx->hw_device_ctx = hwDeviceBufferRef;
    codec_ctx->get_format = vdpauGetFormat;
    codec_ctx->thread_count = 1;
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(58, 18, 100)
    codec_ctx->extra_hw_frames = 4;
#endif
    if (!openCodec(codec))
        return false;

    if (pix_fmt == AV_PIX_FMT_YUVJ420P)
        m_limitedRange = false;

    time_base = streamInfo.getTimeBase();
    return true;
}

void FFDecVDPAU::preemptionCallback(uint32_t device, void *context)
{
    Q_UNUSED(device)
    Q_UNUSED(context)
    // IMPLEMENT ME: When VDPAU is preempted (VT switch) everything must be recreated,
    // but it is only possible after switching VT to X11.
    QMPlay2Core.logError("VDPAU :: Preemption");
}
