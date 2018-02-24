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

#include <FFDecVAAPI.hpp>
#include <HWAccelInterface.hpp>
#include <VAAPIWriter.hpp>
#include <FFCommon.hpp>

#include <StreamInfo.hpp>

extern "C"
{
	#include <libavformat/avformat.h>
	#include <libavutil/pixdesc.h>
	#include <libavcodec/vaapi.h>
	#include <libswscale/swscale.h>
}

#include <va/va_glx.h>

class VAAPIOpenGL : public HWAccelInterface
{
public:
	VAAPIOpenGL(VAAPI *vaapi) :
		m_vaapi(vaapi),
		m_glSurface(nullptr),
		m_canDeleteVAAPI(false)
	{}
	~VAAPIOpenGL() final
	{
		if (m_canDeleteVAAPI)
			delete m_vaapi;
	}

	QString name() const override
	{
		return VAAPIWriterName;
	}

	Format getFormat() const override
	{
		return RGB32;
	}

	bool init(quint32 *textures) override
	{
		return (vaCreateSurfaceGLX(m_vaapi->VADisp, GL_TEXTURE_2D, *textures, &m_glSurface) == VA_STATUS_SUCCESS);
	}
	void clear(bool contextChange) override
	{
		Q_UNUSED(contextChange)
		if (m_glSurface)
		{
			vaDestroySurfaceGLX(m_vaapi->VADisp, m_glSurface);
			m_glSurface = nullptr;
		}
	}

	CopyResult copyFrame(const VideoFrame &videoFrame, Field field) override
	{
		VASurfaceID id;
		int vaField = field; //VA-API field codes are compatible with "HWAccelInterface::Field" codes.
		if (m_vaapi->filterVideo(videoFrame, id, vaField))
		{
			if (vaCopySurfaceGLX(m_vaapi->VADisp, m_glSurface, id, vaField) == VA_STATUS_SUCCESS)
				return CopyOk;
			return CopyError;
		}
		return CopyNotReady;
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
	void setVideAdjustment(const VideoAdjustment &videoAdjustment) override
	{
		m_vaapi->applyVideoAdjustment(0, 0, videoAdjustment.saturation, videoAdjustment.hue);
	}

	/**/

	inline VAAPI *getVAAPI() const
	{
		return m_vaapi;
	}

	inline void allowDeleteVAAPI()
	{
		m_canDeleteVAAPI = true;
	}

private:
	VAAPI *m_vaapi;
	void *m_glSurface;
	bool m_canDeleteVAAPI;
};

/**/

FFDecVAAPI::FFDecVAAPI(QMutex &avcodec_mutex, Module &module) :
	FFDecHWAccel(avcodec_mutex),
	m_useOpenGL(true), m_allowVDPAU(false),
	m_copyVideo(Qt::Unchecked),
	m_vaapi(nullptr),
	m_swsCtx(nullptr)
{
	SetModule(module);
}
FFDecVAAPI::~FFDecVAAPI()
{
	if (codecIsOpen)
		avcodec_flush_buffers(codec_ctx);
	if (m_swsCtx)
		sws_freeContext(m_swsCtx);
}

bool FFDecVAAPI::set()
{
	bool ret = true;

	const bool useOpenGL = sets().getBool("UseOpenGLinVAAPI");
	if (useOpenGL != m_useOpenGL)
	{
		m_useOpenGL = useOpenGL;
		ret = false;
	}

	const bool allowVDPAU = sets().getBool("AllowVDPAUinVAAPI");
	if (allowVDPAU != m_allowVDPAU)
	{
		m_allowVDPAU = allowVDPAU;
		ret = false;
	}

	const Qt::CheckState copyVideo = (Qt::CheckState)sets().getInt("CopyVideoVAAPI");
	if (copyVideo != m_copyVideo)
	{
		m_copyVideo = copyVideo;
		ret = false;
	}

#ifdef HAVE_VPP
	switch (sets().getInt("VAAPIDeintMethod"))
	{
		case 0:
			m_vppDeintType = VAProcDeinterlacingNone;
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
		{
			m_vaapi->clr_vpp();
			if (m_hwAccelWriter)
				m_vaapi->init_vpp();
		}
	}
#endif

	return sets().getBool("DecoderVAAPIEnabled") && ret;
}

QString FFDecVAAPI::name() const
{
	return "FFmpeg/" VAAPIWriterName;
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
	if ((pix_fmt == AV_PIX_FMT_YUV420P || pix_fmt == AV_PIX_FMT_YUVJ420P))
	{
		AVCodec *codec = init(streamInfo);
		if (codec && hasHWAccel("vaapi"))
		{
			if (writer) //Writer is already created
			{
				VAAPIOpenGL *vaapiOpenGL = dynamic_cast<VAAPIOpenGL *>(writer->getHWAccelInterface());
				if (vaapiOpenGL) //Check if it is OpenGL 2
				{
					m_vaapi = vaapiOpenGL->getVAAPI();
					m_hwAccelWriter = writer;
				}
				else if (writer->name() == VAAPIWriterName) //Check if it is VA-API
				{
					VAAPIWriter *vaapiWriter = (VAAPIWriter *)writer;
					m_vaapi = vaapiWriter->getVAAPI();
					if (m_vaapi)
						m_hwAccelWriter = vaapiWriter;
				}
				if (!m_hwAccelWriter)
					writer = nullptr;
			}

			bool useOpenGL = m_useOpenGL;

			if (!m_vaapi) //VA-API context doesn't exist yet, so create it
			{
				m_vaapi = new VAAPI;
				if (!m_vaapi->open(m_allowVDPAU, useOpenGL))
				{
					delete m_vaapi;
					m_vaapi = nullptr;
				}
			}

			if (m_vaapi) //Initialize VA-API context
			{
#ifdef HAVE_VPP
				m_vaapi->vpp_deint_type = m_vppDeintType;
#endif
				if (!m_vaapi->init(codec_ctx->width, codec_ctx->height, avcodec_get_name(codec_ctx->codec_id), (m_copyVideo != Qt::Checked)))
				{
					if (!m_hwAccelWriter)
						delete m_vaapi;
					m_vaapi = nullptr;
				}
			}

			if (m_vaapi) //VA-API context is properly initialized
			{
				if (m_copyVideo != Qt::Checked && !m_hwAccelWriter) //Open writer if doesn't exist yet and if we don't want to copy the video
				{
					if (useOpenGL)
					{
						VAAPIOpenGL *vaapiOpengGL = new VAAPIOpenGL(m_vaapi);
						m_hwAccelWriter = VideoWriter::createOpenGL2(vaapiOpengGL);
						if (m_hwAccelWriter)
							vaapiOpengGL->allowDeleteVAAPI();
					}
					if (!m_hwAccelWriter)
					{
						VAAPIWriter *vaapiWriter = new VAAPIWriter(getModule(), m_vaapi);
						if (!vaapiWriter->open())
							delete vaapiWriter;
						else
						{
							vaapiWriter->init();
							m_hwAccelWriter = vaapiWriter;
						}
					}
				}

				if (m_copyVideo != Qt::Unchecked || m_hwAccelWriter)
				{
					vaapi_context *vaapiCtx = (vaapi_context *)av_mallocz(sizeof(vaapi_context));
					vaapiCtx->display    = m_vaapi->VADisp;
					vaapiCtx->context_id = m_vaapi->context;
					vaapiCtx->config_id  = m_vaapi->config;

					new HWAccelHelper(codec_ctx, AV_PIX_FMT_VAAPI_VLD, vaapiCtx, m_vaapi->getSurfacesQueue());

					if (openCodec(codec))
					{
						time_base = streamInfo.getTimeBase();
						if (m_hwAccelWriter || m_vaapi->nv12ImageFmt.fourcc == VA_FOURCC_NV12)
							return true;
					}
				}

				if (!m_hwAccelWriter)
				{
					delete m_vaapi;
					m_vaapi = nullptr;
				}
			}
		}
	}
	return false;
}
