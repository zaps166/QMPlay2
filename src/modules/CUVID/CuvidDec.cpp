/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2022  Błażej Szczygieł

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

#include <CuvidDec.hpp>

#include <DeintHWPrepareFilter.hpp>
#include <CuvidHWInterop.hpp>
#include <HWDecContext.hpp>
#include <GPUInstance.hpp>
#include <StreamInfo.hpp>
#include <CuvidAPI.hpp>
#ifdef USE_OPENGL
#   include <CuvidOpenGL.hpp>
#endif
#ifdef USE_VULKAN
#   include <CuvidVulkan.hpp>
#endif

#include <QDebug>

#ifdef Q_OS_WIN
    #include <windows.h>
#endif

using namespace std;

extern "C"
{
    #include <libavcodec/avcodec.h>
    #include <libavcodec/version.h>
    #include <libavutil/pixdesc.h>
    #include <libswscale/swscale.h>
}

namespace cuvid {
    static int CUDAAPI videoSequence(CuvidDec *cuvidDec, CUVIDEOFORMAT *format)
    {
        return cuvidDec->videoSequence(format);
    }
    static int CUDAAPI pictureDecode(CuvidDec *cuvidDec, CUVIDPICPARAMS *picParams)
    {
        return cuvidDec->pictureDecode(picParams);
    }
    static int CUDAAPI pictureDisplay(CuvidDec *cuvidDec, CUVIDPARSERDISPINFO *dispInfo)
    {
        return cuvidDec->pictureDisplay(dispInfo);
    }
}

/**/

constexpr quint32 g_maxSurfaces = 25;

static QMutex g_loadMutex;
static int g_loadState = -1;
static bool g_initGL = false;
static bool g_initVK = false;
static bool g_initialized = false;

bool CuvidDec::canCreateInstance()
{
    QMutexLocker locker(&g_loadMutex);

    const bool initVK = QMPlay2Core.isVulkanRenderer();
    const bool initGL = (QMPlay2Core.renderer() == QMPlay2CoreClass::Renderer::OpenGL);

    if (initGL != g_initGL || initVK != g_initVK)
    {
        g_initGL = initGL;
        g_initVK = initVK;
        g_loadState = -1;
    }

    return (g_loadState != 0);
}

CuvidDec::CuvidDec(Module &module) :
    m_limited(false),
    m_colorSpace(AVCOL_SPC_UNSPECIFIED),
    m_lastCuvidTS(0),
    m_deintMethod(cudaVideoDeinterlaceMode_Weave),
    m_forceFlush(false),
    m_tsWorkaround(false),
    m_bsfCtx(nullptr),
    m_swsCtx(nullptr),
    m_pkt(nullptr),
    m_cuCtx(nullptr),
    m_cuvidParser(nullptr),
    m_cuvidDec(nullptr),
    m_decodeMPEG4(true),
    m_hasCriticalError(false),
    m_skipFrames(false)
{
    SetModule(module);
}
CuvidDec::~CuvidDec()
{
    if (m_cuCtx)
    {
        cu::ContextGuard cuCtxGuard(m_cuCtx);
        destroyCuvid(true);
        m_cuCtx.reset();
    }
    av_bsf_free(&m_bsfCtx);
    if (m_swsCtx)
        sws_freeContext(m_swsCtx);
    av_packet_free(&m_pkt);
}

bool CuvidDec::set()
{
    if (sets().getBool("Enabled"))
    {
        bool restart = false;

        const cudaVideoDeinterlaceMode deintMethod = (cudaVideoDeinterlaceMode)sets().getInt("DeintMethod");
        if (deintMethod != m_deintMethod)
        {
            m_forceFlush = true;
            m_deintMethod = deintMethod;
        }

        const bool decodeMPEG4 = sets().getBool("DecodeMPEG4");
        if (decodeMPEG4 != m_decodeMPEG4)
        {
            m_decodeMPEG4 = decodeMPEG4;
            restart = true;
        }

        if (!restart)
            return true;
    }
    return false;
}

int CuvidDec::videoSequence(CUVIDEOFORMAT *format)
{
    CUVIDDECODECREATEINFO cuvidDecInfo;
    memset(&cuvidDecInfo, 0, sizeof cuvidDecInfo);

    cuvidDecInfo.CodecType = format->codec;
    cuvidDecInfo.ChromaFormat = format->chroma_format;
    cuvidDecInfo.DeinterlaceMode = (!m_filter || format->progressive_sequence) ? cudaVideoDeinterlaceMode_Weave : m_deintMethod;
    cuvidDecInfo.OutputFormat = (m_depth > 8 && m_cuvidHwInterop && m_hasP016)
        ? cudaVideoSurfaceFormat_P016
        : cudaVideoSurfaceFormat_NV12
    ;

    cuvidDecInfo.ulWidth = format->coded_width;
    cuvidDecInfo.ulHeight = format->coded_height;
    cuvidDecInfo.ulTargetWidth = cuvidDecInfo.ulWidth;
    cuvidDecInfo.ulTargetHeight = cuvidDecInfo.ulHeight;

    cuvidDecInfo.target_rect.right = cuvidDecInfo.ulWidth;
    cuvidDecInfo.target_rect.bottom = cuvidDecInfo.ulHeight;

    cuvidDecInfo.ulNumDecodeSurfaces = g_maxSurfaces;
    cuvidDecInfo.ulNumOutputSurfaces = 1;
    cuvidDecInfo.ulCreationFlags = cudaVideoCreate_PreferCUVID;
    cuvidDecInfo.bitDepthMinus8 = format->bit_depth_luma_minus8;

    m_width = format->display_area.right;
    m_height = format->display_area.bottom;
    m_codedHeight = format->coded_height;
    resetTimeStampHelpers();

    destroyCuvid(false);

    if (!m_cuvidHwInterop)
    {
        m_swsCtx = sws_getCachedContext(m_swsCtx, m_width, m_height, AV_PIX_FMT_NV12, m_width, m_height, AV_PIX_FMT_YUV420P, SWS_POINT, nullptr, nullptr, nullptr);
        if (!m_swsCtx)
            return 0;
    }

    const CUresult ret = cuvid::createDecoder(&m_cuvidDec, &cuvidDecInfo);
    if (ret != CUDA_SUCCESS)
    {
        QMPlay2Core.logError("CUVID :: Error '" + QString::number(ret) + "' while creating decoder");
        m_hasCriticalError = true;
        return 0;
    }

    if (m_cuvidHwInterop)
        m_cuvidHwInterop->setDecoderAndCodedHeight(m_cuvidDec, m_codedHeight);

    return 1;
}
int CuvidDec::pictureDecode(CUVIDPICPARAMS *picParams)
{
    if (m_skipFrames && picParams->ref_pic_flag == 0 && picParams->intra_pic_flag == 0)
        return 0;
    if (cuvid::decodePicture(m_cuvidDec, picParams) != CUDA_SUCCESS)
        return 0;
    return 1;
}
int CuvidDec::pictureDisplay(CUVIDPARSERDISPINFO *dispInfo)
{
    if (dispInfo->timestamp > 0 && dispInfo->timestamp <= m_lastCuvidTS)
        m_tsWorkaround = true;
    m_lastCuvidTS = dispInfo->timestamp;

    //"m_cuvidSurfaces" shouldn't be larger than "MaxSurfaces"
    m_cuvidSurfaces.enqueue(*dispInfo);

    return 1;
}

QString CuvidDec::name() const
{
    return CuvidName;
}

bool CuvidDec::hasHWDecContext() const
{
    return static_cast<bool>(m_cuvidHwInterop);
}
shared_ptr<VideoFilter> CuvidDec::hwAccelFilter() const
{
    return m_filter;
}

void CuvidDec::setSupportedPixelFormats(const AVPixelFormats &pixelFormats)
{
    m_hasP016 = pixelFormats.contains(AV_PIX_FMT_P016);
}

int CuvidDec::decodeVideo(const Packet &encodedPacket, Frame &decoded, AVPixelFormat &newPixFmt, bool flush, unsigned hurry_up)
{
    Q_UNUSED(newPixFmt)

    cu::ContextGuard cuCtxGuard(m_cuCtx);
    CUVIDSOURCEDATAPACKET cuvidPkt;

    m_skipFrames = (m_cuvidParserParams.CodecType != cudaVideoCodec_VP9 && hurry_up > 1);

    if (flush || m_forceFlush)
    {
        destroyCuvid(true);
        resetTimeStampHelpers();
        m_cuvidSurfaces.clear();
        m_forceFlush = false;
        if (!createCuvidVideoParser())
            return -1;
    }

    memset(&cuvidPkt, 0, sizeof cuvidPkt);
    if (encodedPacket.isEmpty())
        cuvidPkt.flags = CUVID_PKT_ENDOFSTREAM;
    else
    {
        cuvidPkt.flags = CUVID_PKT_TIMESTAMP;
        cuvidPkt.timestamp = encodedPacket.pts() * 10000000.0 + 0.5;
        if (m_bsfCtx)
        {
            m_pkt->buf = encodedPacket.getBufferRef();
            m_pkt->data = encodedPacket.data();
            m_pkt->size = encodedPacket.size();

            if (av_bsf_send_packet(m_bsfCtx, m_pkt) < 0) //It unrefs "pkt"
                return -1;

            if (av_bsf_receive_packet(m_bsfCtx, m_pkt) < 0) //Can it return more than one packet in this case?
                return -1;

            cuvidPkt.payload = m_pkt->data;
            cuvidPkt.payload_size = m_pkt->size;
        }
        else
        {
            cuvidPkt.payload = encodedPacket.data();
            cuvidPkt.payload_size = encodedPacket.size();
        }
    }

    const bool videoDataParsed = (cuvid::parseVideoData(m_cuvidParser, &cuvidPkt) == CUDA_SUCCESS);

    if (cuvidPkt.flags == CUVID_PKT_TIMESTAMP && videoDataParsed)
        m_timestamps.enqueue(encodedPacket.ts());

    if (m_pkt)
        av_packet_unref(m_pkt);

    if (!m_cuvidSurfaces.isEmpty())
    {
        const CUVIDPARSERDISPINFO dispInfo = m_cuvidSurfaces.dequeue();

        double ts = dispInfo.timestamp / 10000000.0;
        if (!m_timestamps.isEmpty())
        {
            const double dequeuedTS = m_timestamps.dequeue();
            if (!m_timestamps.isEmpty() && !dispInfo.progressive_frame)
                m_timestamps.dequeue();
            if (m_tsWorkaround)
                ts = dequeuedTS;
        }
        if (ts >= 0.0)
        {
            m_lastTS[1] = m_lastTS[0];
            m_lastTS[0] = ts;
        }
        else if (m_lastTS[0] >= 0.0 && m_lastTS[1] >= 0.0)
        {
            const double diff = m_lastTS[0] - m_lastTS[1];
            m_lastTS[1] = m_lastTS[0];
            m_lastTS[0] += diff;
            ts = m_lastTS[0];
        }

        if (~hurry_up)
        {
            auto createFrame = [&] {

                decoded = Frame::createEmpty(
                    m_width,
                    m_height,
                    m_cuvidHwInterop
                        ? (m_depth > 8 && m_hasP016
                           ? AV_PIX_FMT_P016
                           : AV_PIX_FMT_NV12)
                        : (m_limited
                           ? AV_PIX_FMT_YUV420P
                           : AV_PIX_FMT_YUVJ420P),
                    !dispInfo.progressive_frame,
                    dispInfo.top_field_first,
                    m_colorSpace,
                    m_limited
                );
            };
            if (m_cuvidHwInterop)
            {
                createFrame();
                decoded.setCustomData(dispInfo.picture_index);
                m_cuvidHwInterop->setAvailableSurface(dispInfo.picture_index);
            }
            else
            {
                CUdeviceptr mappedFrame = 0;
                unsigned int pitch = 0;

                CUVIDPROCPARAMS vidProcParams;
                memset(&vidProcParams, 0, sizeof vidProcParams);
                vidProcParams.progressive_frame = dispInfo.progressive_frame;
                vidProcParams.second_field = 0;
                vidProcParams.top_field_first = dispInfo.top_field_first;
                if (cuvid::mapVideoFrame(m_cuvidDec, dispInfo.picture_index, &mappedFrame, &pitch, &vidProcParams) == CUDA_SUCCESS)
                {
                    const int size = pitch * m_height;
                    const int halfSize = pitch * ((m_height + 1) >> 1);

                    auto nv12Luma = new quint8[size];
                    auto nv12Chroma = new quint8[halfSize];

                    bool copied = (cu::memcpyDtoH(nv12Luma, mappedFrame, size) == CUDA_SUCCESS);
                    if (copied)
                        copied &= (cu::memcpyDtoH(nv12Chroma, mappedFrame + m_codedHeight * pitch, halfSize) == CUDA_SUCCESS);

                    cuvid::unmapVideoFrame(m_cuvidDec, mappedFrame);

                    if (copied)
                    {
                        const uint8_t *srcData[2] = {
                            nv12Luma,
                            nv12Chroma,
                        };
                        const qint32 srcLinesize[2] = {
                            (int)pitch,
                            (int)pitch,
                        };

                        AVBufferRef *frameBuffer[3] = {
                            av_buffer_alloc(size),
                            av_buffer_alloc((halfSize + 1) >> 1),
                            av_buffer_alloc((halfSize + 1) >> 1),
                        };
                        uint8_t *dstData[3] = {
                            frameBuffer[0]->data,
                            frameBuffer[1]->data,
                            frameBuffer[2]->data,
                        };
                        qint32 dstLinesize[3] = {
                            (qint32)pitch,
                            (qint32)(pitch >> 1),
                            (qint32)(pitch >> 1),
                        };

                        if (m_swsCtx)
                            sws_scale(m_swsCtx, srcData, srcLinesize, 0, m_height, dstData, dstLinesize);

                        createFrame();
                        decoded.setVideoData(frameBuffer, dstLinesize);
                    }

                    delete[] nv12Luma;
                    delete[] nv12Chroma;
                }
            }
        }

        decoded.setTS(ts);
    }

    if (!videoDataParsed)
        return -1;

    return encodedPacket.size();
}

bool CuvidDec::hasCriticalError() const
{
    return m_hasCriticalError;
}

bool CuvidDec::open(StreamInfo &streamInfo)
{
    if (streamInfo.codec_type != AVMEDIA_TYPE_VIDEO)
        return false;

    const AVCodec *avCodec = avcodec_find_decoder_by_name(streamInfo.codec_name);
    if (!avCodec)
        return false;

    const AVPixelFormat pixFmt = streamInfo.pixelFormat();
    if (!(pixFmt == AV_PIX_FMT_YUV420P || pixFmt == AV_PIX_FMT_YUV420P10 || pixFmt == AV_PIX_FMT_YUVJ420P || avCodec->id == AV_CODEC_ID_MJPEG))
        return false;

    m_depth = 8;
    if (const AVPixFmtDescriptor *pixDesc = av_pix_fmt_desc_get(pixFmt))
    {
        m_depth = pixDesc->comp[0].depth;
    }

    cudaVideoCodec codec;
    switch (avCodec->id)
    {
        case AV_CODEC_ID_MPEG1VIDEO:
            codec = cudaVideoCodec_MPEG1;
            break;
        case AV_CODEC_ID_MPEG2VIDEO:
            codec = cudaVideoCodec_MPEG2;
            break;
        case AV_CODEC_ID_MJPEG:
            codec = cudaVideoCodec_JPEG;
            break;
        case AV_CODEC_ID_MPEG4:
            if (!m_decodeMPEG4)
                return false;
            codec = cudaVideoCodec_MPEG4;
            break;
        case AV_CODEC_ID_H264:
            if (m_depth > 8)
                return false;
            codec = cudaVideoCodec_H264;
            break;
        case AV_CODEC_ID_VC1:
            codec = cudaVideoCodec_VC1;
            break;
        case AV_CODEC_ID_VP8:
            codec = cudaVideoCodec_VP8;
            break;
        case AV_CODEC_ID_VP9:
            codec = cudaVideoCodec_VP9;
            break;
        case AV_CODEC_ID_HEVC:
            codec = cudaVideoCodec_HEVC;
            break;
        default:
            return false;
    }

    if (!loadLibrariesAndInit())
        return false;

    const AVBitStreamFilter *bsf = nullptr;
    switch (codec)
    {
        case cudaVideoCodec_H264:
            bsf = av_bsf_get_by_name("h264_mp4toannexb");
            if (!bsf)
                return false;
            break;
        case cudaVideoCodec_HEVC:
            bsf = av_bsf_get_by_name("hevc_mp4toannexb");
            if (!bsf)
                return false;
            break;
        default:
            break;
    }

    QByteArray extraData;

    if (!bsf)
    {
        extraData = streamInfo.getExtraData();
    }
    else
    {
        av_bsf_alloc(bsf, &m_bsfCtx);

        m_bsfCtx->par_in->codec_id = avCodec->id;
        m_bsfCtx->par_in->extradata_size = streamInfo.extradata_size;
        m_bsfCtx->par_in->extradata = (uint8_t *)av_mallocz(streamInfo.getExtraDataCapacity());
        memcpy(m_bsfCtx->par_in->extradata, streamInfo.extradata, m_bsfCtx->par_in->extradata_size);

        if (av_bsf_init(m_bsfCtx) < 0)
            return false;

        extraData = QByteArray::fromRawData((const char *)m_bsfCtx->par_out->extradata, m_bsfCtx->par_out->extradata_size);

        m_pkt = av_packet_alloc();
    }

    m_width = streamInfo.width;
    m_height = streamInfo.height;
    m_codedHeight = m_height;

#if defined(USE_OPENGL) || defined(USE_VULKAN)
    if (QMPlay2Core.renderer() == QMPlay2CoreClass::Renderer::OpenGL || QMPlay2Core.isVulkanRenderer())
    {
        m_cuvidHwInterop = QMPlay2Core.gpuInstance()->getHWDecContext<CuvidHWInterop>();
        if (m_cuvidHwInterop)
            m_cuCtx = m_cuvidHwInterop->getCudaContext();
    }
#endif

    if (!m_cuCtx && !(m_cuCtx = cu::createContext()))
        return false;

    cu::ContextGuard cuCtxGuard(m_cuCtx);

    memset(&m_cuvidFmt, 0, sizeof m_cuvidFmt);
    m_cuvidFmt.format.seqhdr_data_length = qMin<size_t>(sizeof m_cuvidFmt.raw_seqhdr_data, extraData.size());
    memcpy(m_cuvidFmt.raw_seqhdr_data, extraData.constData(), m_cuvidFmt.format.seqhdr_data_length);

    memset(&m_cuvidParserParams, 0, sizeof m_cuvidParserParams);
    m_cuvidParserParams.CodecType = codec;
    m_cuvidParserParams.ulMaxNumDecodeSurfaces = g_maxSurfaces;
    m_cuvidParserParams.ulMaxDisplayDelay = 4;
    m_cuvidParserParams.pUserData = this;
    m_cuvidParserParams.pfnSequenceCallback = (PFNVIDSEQUENCECALLBACK)cuvid::videoSequence;
    m_cuvidParserParams.pfnDecodePicture = (PFNVIDDECODECALLBACK)cuvid::pictureDecode;
    m_cuvidParserParams.pfnDisplayPicture = (PFNVIDDISPLAYCALLBACK)cuvid::pictureDisplay;
    m_cuvidParserParams.pExtVideoInfo = &m_cuvidFmt;

    bool err = false;

    if (!testDecoder(m_depth))
        err = true;
    else if (!createCuvidVideoParser())
        err = true;

    if (err)
        return false;

    m_limited = (streamInfo.color_range != AVCOL_RANGE_JPEG);
    m_colorSpace = streamInfo.color_space;

    if (!m_cuvidHwInterop)
    {
        shared_ptr<HWDecContext> cuvidHwInterop;
        if (QMPlay2Core.isVulkanRenderer())
        {
#ifdef USE_VULKAN
            auto cuvidVulkan = make_shared<CuvidVulkan>(m_cuCtx);
            if (cuvidVulkan->hasError())
                return false;
            cuvidHwInterop = move(cuvidVulkan);
#endif
        }
        else if (QMPlay2Core.renderer() == QMPlay2CoreClass::Renderer::OpenGL)
        {
#ifdef USE_OPENGL
            cuvidHwInterop = make_shared<CuvidOpenGL>(m_cuCtx);
#endif
        }
        if (cuvidHwInterop)
        {
            if (!QMPlay2Core.gpuInstance()->setHWDecContextForVideoOutput(cuvidHwInterop))
                return false;
            m_cuvidHwInterop = dynamic_pointer_cast<CuvidHWInterop>(cuvidHwInterop);
        }
    }

    if (m_cuvidHwInterop && (!QMPlay2Core.isVulkanRenderer() || !QMPlay2Core.getSettings().getBool("Vulkan/ForceVulkanYadif") || !QMPlay2Core.gpuInstance()->checkFiltersSupported()))
        m_filter = make_shared<DeintHWPrepareFilter>();

    return true;
}

bool CuvidDec::testDecoder(const int depth)
{
    CUVIDDECODECREATEINFO cuvidDecInfo;
    memset(&cuvidDecInfo, 0, sizeof cuvidDecInfo);

    cuvidDecInfo.CodecType = m_cuvidParserParams.CodecType;
    cuvidDecInfo.ChromaFormat = cudaVideoChromaFormat_420;
    cuvidDecInfo.OutputFormat = cudaVideoSurfaceFormat_NV12;
    cuvidDecInfo.DeinterlaceMode = cudaVideoDeinterlaceMode_Weave;

    cuvidDecInfo.ulWidth = m_width ? m_width : 1280;
    cuvidDecInfo.ulHeight = m_height ? m_height : 720;
    cuvidDecInfo.ulTargetWidth = cuvidDecInfo.ulWidth;
    cuvidDecInfo.ulTargetHeight = cuvidDecInfo.ulHeight;

    cuvidDecInfo.target_rect.right = cuvidDecInfo.ulWidth;
    cuvidDecInfo.target_rect.bottom = cuvidDecInfo.ulHeight;

    cuvidDecInfo.ulNumDecodeSurfaces = g_maxSurfaces;
    cuvidDecInfo.ulNumOutputSurfaces = 1;
    cuvidDecInfo.ulCreationFlags = cudaVideoCreate_PreferCUVID;
    cuvidDecInfo.bitDepthMinus8 = qMax(0, depth - 8);

    CUvideodecoder tmpCuvidDec = nullptr;
    if (cuvid::createDecoder(&tmpCuvidDec, &cuvidDecInfo) != CUDA_SUCCESS)
        return false;

    if (cuvid::destroyDecoder(tmpCuvidDec) != CUDA_SUCCESS)
        return false;

    return true;
}

bool CuvidDec::createCuvidVideoParser()
{
    if (cuvid::createVideoParser(&m_cuvidParser, &m_cuvidParserParams) != CUDA_SUCCESS)
        return false;

    CUVIDSOURCEDATAPACKET cuvidExtradata;
    memset(&cuvidExtradata, 0, sizeof cuvidExtradata);
    cuvidExtradata.payload = m_cuvidFmt.raw_seqhdr_data;
    cuvidExtradata.payload_size = m_cuvidFmt.format.seqhdr_data_length;

    if (cuvid::parseVideoData(m_cuvidParser, &cuvidExtradata) != CUDA_SUCCESS)
        return false;

    return true;
}
void CuvidDec::destroyCuvid(bool all)
{
    if (m_cuvidHwInterop)
        m_cuvidHwInterop->setDecoderAndCodedHeight(nullptr, 0);
    cuvid::destroyDecoder(m_cuvidDec);
    m_cuvidDec = nullptr;
    if (all)
    {
        cuvid::destroyVideoParser(m_cuvidParser);
        m_cuvidParser = nullptr;
    }
}

inline void CuvidDec::resetTimeStampHelpers()
{
    m_lastTS[0] = m_lastTS[1] = -1.0;
    m_tsWorkaround = false;
    m_timestamps.clear();
    m_lastCuvidTS = 0;
}

bool CuvidDec::loadLibrariesAndInit()
{
    QMutexLocker locker(&g_loadMutex);

    if (g_loadState == -1)
    {
#ifdef Q_OS_WIN
        if (sets().getBool("CheckFirstGPU"))
        {
            DISPLAY_DEVICEA displayDev;
            memset(&displayDev, 0, sizeof displayDev);
            displayDev.cb = sizeof displayDev;
            if (EnumDisplayDevicesA(nullptr, 0, &displayDev, 0))
            {
                const bool isNvidia = QByteArray::fromRawData(displayDev.DeviceString, sizeof displayDev.DeviceString).toLower().contains("nvidia");
                if (!isNvidia)
                {
                    g_loadState = 0;
                    return false;
                }
            }
        }
#endif
        g_loadState = (cuvid::load() && cu::load(!g_initialized, g_initGL, g_initVK));
        if (g_loadState)
            g_initialized = true;
        else
            QMPlay2Core.logError("CUVID :: Unable to get function pointers");
    }

    return (g_loadState == 1);
}
