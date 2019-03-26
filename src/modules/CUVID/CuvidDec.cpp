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

#include <CuvidDec.hpp>

#include <HWAccelInterface.hpp>
#include <VideoWriter.hpp>
#include <StreamInfo.hpp>
#include <ImgScaler.hpp>

#include <QLibrary>
#include <QMutex>

#ifdef Q_OS_WIN
    #include <windows.h>
#endif

#ifndef GL_TEXTURE_2D
    #define GL_TEXTURE_2D 0x0DE1
#endif

static QMutex g_cudaMutex(QMutex::Recursive);

extern "C"
{
    #include <libavcodec/avcodec.h>
    #include <libavcodec/version.h>
    #include <libavutil/pixdesc.h>
    #include <libswscale/swscale.h>
}

namespace cu
{
    using  cuInitType = CUresult CUDAAPI (*)(unsigned int flags);
    static cuInitType init;

    using  cuDeviceGetType = CUresult CUDAAPI (*)(CUdevice *device, int ordinal);
    static cuDeviceGetType deviceGet;

    using  cuCtxCreateType = CUresult CUDAAPI (*)(CUcontext *pctx, unsigned int flags, CUdevice dev);
    static cuCtxCreateType ctxCreate;

    using  cuCtxPushCurrentType = CUresult CUDAAPI (*)(CUcontext ctx);
    static cuCtxPushCurrentType ctxPushCurrent;

    using  cuCtxPopCurrentType = CUresult CUDAAPI (*)(CUcontext *pctx);
    static cuCtxPopCurrentType ctxPopCurrent;

    using  cuMemcpyDtoHType = CUresult CUDAAPI (*)(void *dstHost, CUdeviceptr srcDevice, size_t byteCount);
    static cuMemcpyDtoHType memcpyDtoH;

    using  cuMemcpy2DType = CUresult CUDAAPI (*)(const CUDA_MEMCPY2D *pCopy);
    static cuMemcpy2DType memcpy2D;

    using  cuGraphicsGLRegisterImageType = CUresult CUDAAPI (*)(CUgraphicsResource *pCudaResource, quint32 image, qint32 target, unsigned int flags);
    static cuGraphicsGLRegisterImageType graphicsGLRegisterImage;

    using  cuGraphicsMapResourcesType = CUresult CUDAAPI (*)(unsigned int count, CUgraphicsResource *resources, CUstream hStream);
    static cuGraphicsMapResourcesType graphicsMapResources;

    using  cuGraphicsSubResourceGetMappedArrayType = CUresult CUDAAPI (*)(CUarray *pArray, CUgraphicsResource resource, unsigned int arrayIndex, unsigned int mipLevel);
    static cuGraphicsSubResourceGetMappedArrayType graphicsSubResourceGetMappedArray;

    using  cuGraphicsUnmapResourcesType = CUresult CUDAAPI (*)(unsigned int count, CUgraphicsResource *resources, CUstream hStream);
    static cuGraphicsUnmapResourcesType graphicsUnmapResources;

    using  cuGraphicsUnregisterResourceType = CUresult CUDAAPI (*)(CUgraphicsResource resource);
    static cuGraphicsUnregisterResourceType graphicsUnregisterResource;

    using  cuCtxDestroyType = CUresult CUDAAPI (*)(CUcontext ctx);
    static cuCtxDestroyType ctxDestroy;

    static bool load()
    {
#ifdef Q_OS_WIN
        QLibrary lib("nvcuda");
#else
        QLibrary lib("cuda");
#endif
        if (lib.load())
        {
            init = (cuInitType)lib.resolve("cuInit");
            deviceGet = (cuDeviceGetType)lib.resolve("cuDeviceGet");
            ctxCreate = (cuCtxCreateType)lib.resolve("cuCtxCreate_v2");
            ctxPushCurrent = (cuCtxPushCurrentType)lib.resolve("cuCtxPushCurrent_v2");
            ctxPopCurrent = (cuCtxPopCurrentType)lib.resolve("cuCtxPopCurrent_v2");
            memcpyDtoH = (cuMemcpyDtoHType)lib.resolve("cuMemcpyDtoH_v2");
            memcpy2D = (cuMemcpy2DType)lib.resolve("cuMemcpy2D_v2");
            graphicsGLRegisterImage = (cuGraphicsGLRegisterImageType)lib.resolve("cuGraphicsGLRegisterImage");
            graphicsMapResources = (cuGraphicsMapResourcesType)lib.resolve("cuGraphicsMapResources");
            graphicsSubResourceGetMappedArray = (cuGraphicsSubResourceGetMappedArrayType)lib.resolve("cuGraphicsSubResourceGetMappedArray");
            graphicsUnmapResources = (cuGraphicsUnmapResourcesType)lib.resolve("cuGraphicsUnmapResources");
            graphicsUnregisterResource = (cuGraphicsUnregisterResourceType)lib.resolve("cuGraphicsUnregisterResource");
            ctxDestroy = (cuCtxDestroyType)lib.resolve("cuCtxDestroy_v2");
            if (init && init(0) != CUDA_SUCCESS)
                return false;
            return (deviceGet && ctxCreate && ctxPushCurrent && ctxPopCurrent && memcpyDtoH && memcpy2D && graphicsGLRegisterImage && graphicsMapResources && graphicsSubResourceGetMappedArray && graphicsUnmapResources && graphicsUnregisterResource && ctxDestroy);
        }
        return false;
    }

    static CUcontext createContext()
    {
        CUcontext ctx, tmpCtx;
        CUdevice dev = -1;
        const int devIdx = 0;
        if (deviceGet(&dev, devIdx) != CUDA_SUCCESS)
            return nullptr;
        if (ctxCreate(&ctx, CU_CTX_SCHED_BLOCKING_SYNC, dev) != CUDA_SUCCESS)
            return nullptr;
        ctxPopCurrent(&tmpCtx);
        return ctx;
    }

    class ContextGuard
    {
        Q_DISABLE_COPY(ContextGuard)

    public:
        inline ContextGuard(CUcontext ctx) :
            m_locked(true)
        {
            g_cudaMutex.lock();
            ctxPushCurrent(ctx);
        }
        inline ~ContextGuard()
        {
            unlock();
        }

        inline void unlock()
        {
            if (m_locked)
            {
                CUcontext ctx;
                ctxPopCurrent(&ctx);
                g_cudaMutex.unlock();
                m_locked = false;
            }
        }

    private:
        bool m_locked;
    };
}

namespace cuvid
{
    using  cuvidCreateVideoParserType = CUresult CUDAAPI (*)(CUvideoparser *pObj, CUVIDPARSERPARAMS *pParams);
    static cuvidCreateVideoParserType createVideoParser;

    using  cuvidDestroyVideoParserType = CUresult CUDAAPI (*)(CUvideoparser obj);
    static cuvidDestroyVideoParserType destroyVideoParser;

    using  cuvidDecodePictureType = CUresult CUDAAPI (*)(CUvideodecoder hDecoder, CUVIDPICPARAMS *pPicParams);
    static cuvidDecodePictureType decodePicture;

    using  cuvidCreateDecoderType = CUresult CUDAAPI (*)(CUvideodecoder *phDecoder, CUVIDDECODECREATEINFO *pdci);
    static cuvidCreateDecoderType createDecoder;

    using  cuvidDestroyDecoderType = CUresult CUDAAPI (*)(CUvideodecoder hDecoder);
    static cuvidDestroyDecoderType destroyDecoder;

    using  cuvidMapVideoFrameType = CUresult CUDAAPI (*)(CUvideodecoder hDecoder, int nPicIdx, quintptr *pDevPtr, unsigned int *pPitch, CUVIDPROCPARAMS *pVPP);
    static cuvidMapVideoFrameType mapVideoFrame;

    using  cuvidUnmapVideoFrameType = CUresult CUDAAPI (*)(CUvideodecoder hDecoder, quintptr DevPtr);
    static cuvidUnmapVideoFrameType unmapVideoFrame;

    using  cuvidParseVideoDataType = CUresult CUDAAPI (*)(CUvideoparser obj, CUVIDSOURCEDATAPACKET *pPacket);
    static cuvidParseVideoDataType parseVideoData;

    static bool load()
    {
        QLibrary lib("nvcuvid");
        if (lib.load())
        {
            createVideoParser = (cuvidCreateVideoParserType)lib.resolve("cuvidCreateVideoParser");
            destroyVideoParser = (cuvidDestroyVideoParserType)lib.resolve("cuvidDestroyVideoParser");
            decodePicture = (cuvidDecodePictureType)lib.resolve("cuvidDecodePicture");
            createDecoder = (cuvidCreateDecoderType)lib.resolve("cuvidCreateDecoder");
            destroyDecoder = (cuvidDestroyDecoderType)lib.resolve("cuvidDestroyDecoder");
            if (sizeof(void *) == 8)
            {
                mapVideoFrame = (cuvidMapVideoFrameType)lib.resolve("cuvidMapVideoFrame64");
                unmapVideoFrame = (cuvidUnmapVideoFrameType)lib.resolve("cuvidUnmapVideoFrame64");
            }
            else
            {
                mapVideoFrame = (cuvidMapVideoFrameType)lib.resolve("cuvidMapVideoFrame");
                unmapVideoFrame = (cuvidUnmapVideoFrameType)lib.resolve("cuvidUnmapVideoFrame");
            }
            parseVideoData = (cuvidParseVideoDataType)lib.resolve("cuvidParseVideoData");
            return (createVideoParser && destroyVideoParser && decodePicture && createDecoder && destroyDecoder && mapVideoFrame && unmapVideoFrame && parseVideoData);
        }
        return false;
    }

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

/* HWAccelInterface implementation */

class CuvidHWAccel : public HWAccelInterface
{
public:
    CuvidHWAccel(CUcontext cuCtx) :
        m_canDestroyCuda(false),
        m_codedHeight(0),
        m_lastId(0),
        m_tff(false),
        m_cuCtx(cuCtx),
        m_cuvidDec(nullptr)
    {
        memset(m_res, 0, sizeof m_res);
    }
    ~CuvidHWAccel() final
    {
        if (m_canDestroyCuda)
        {
            cu::ContextGuard cuCtxGuard(m_cuCtx); //Is it necessary here?
            cu::ctxDestroy(m_cuCtx);
        }
    }

    QString name() const override
    {
        return "CUVID";
    }

    Format getFormat() const override
    {
        return NV12;
    }

    bool lock() override
    {
        g_cudaMutex.lock();
        if (cu::ctxPushCurrent(m_cuCtx) == CUDA_SUCCESS)
            return true;
        g_cudaMutex.unlock();
        return false;
    }
    void unlock() override
    {
        CUcontext cuCtx;
        cu::ctxPopCurrent(&cuCtx);
        g_cudaMutex.unlock();
    }

    bool init(quint32 *textures) override
    {
        for (int p = 0; p < 2; ++p)
        {
            if (cu::graphicsGLRegisterImage(&m_res[p], textures[p], GL_TEXTURE_2D, CU_GRAPHICS_REGISTER_FLAGS_WRITE_DISCARD) != CUDA_SUCCESS)
                return false;
        }
        return true;
    }
    void clear(bool contextChange) override
    {
        Q_UNUSED(contextChange)
        for (int p = 0; p < 2; ++p)
        {
            if (m_res[p])
            {
                cu::graphicsUnregisterResource(m_res[p]);
                m_res[p] = nullptr;
            }
        }
    }

    CopyResult copyFrame(const VideoFrame &videoFrame, Field field) override
    {
        if (!m_cuvidDec || !m_validSurfaces.contains(videoFrame.surfaceId))
            return CopyNotReady;

        CUVIDPROCPARAMS vidProcParams;
        memset(&vidProcParams, 0, sizeof vidProcParams);

        if (m_lastId != videoFrame.surfaceId)
            m_tff = videoFrame.tff;
        m_lastId = videoFrame.surfaceId;

        vidProcParams.top_field_first = m_tff;
        switch (field)
        {
            case FullFrame:
                vidProcParams.progressive_frame = true;
                break;
            case TopField:
                vidProcParams.second_field = false;
                break;
            case BottomField:
                vidProcParams.second_field = true;
                break;
        }

        quintptr mappedFrame = 0;
        unsigned pitch = 0;

        if (cuvid::mapVideoFrame(m_cuvidDec, videoFrame.surfaceId - 1, &mappedFrame, &pitch, &vidProcParams) != CUDA_SUCCESS)
            return CopyError;

        if (cu::graphicsMapResources(2, m_res, nullptr) == CUDA_SUCCESS)
        {
            bool copied = true;

            CUDA_MEMCPY2D cpy;
            memset(&cpy, 0, sizeof cpy);
            cpy.srcMemoryType = CU_MEMORYTYPE_DEVICE;
            cpy.dstMemoryType = CU_MEMORYTYPE_ARRAY;
            cpy.srcDevice = mappedFrame;
            cpy.srcPitch = pitch;
            cpy.WidthInBytes = videoFrame.size.width;
            for (int p = 0; p < 2; ++p)
            {
                CUarray array = nullptr;
                if (cu::graphicsSubResourceGetMappedArray(&array, m_res[p], 0, 0) != CUDA_SUCCESS)
                {
                    copied = false;
                    break;
                }

                cpy.srcY = p ? m_codedHeight : 0;
                cpy.dstArray = array;
                cpy.Height = videoFrame.size.getHeight(p);

                if (cu::memcpy2D(&cpy) != CUDA_SUCCESS)
                {
                    copied = false;
                    break;
                }
            }

            cu::graphicsUnmapResources(2, m_res, nullptr);

            if (cuvid::unmapVideoFrame(m_cuvidDec, mappedFrame) == CUDA_SUCCESS && copied)
                return CopyOk;
        }

        return CopyError;
    }

    bool getImage(const VideoFrame &videoFrame, void *dest, ImgScaler *nv12ToRGB32) override
    {
        cu::ContextGuard cuCtxGuard(m_cuCtx);

        quintptr mappedFrame = 0;
        unsigned pitch = 0;

        CUVIDPROCPARAMS vidProcParams;
        memset(&vidProcParams, 0, sizeof vidProcParams);
        vidProcParams.progressive_frame = !videoFrame.interlaced;
        vidProcParams.top_field_first = m_tff;

        if (cuvid::mapVideoFrame(m_cuvidDec, videoFrame.surfaceId - 1, &mappedFrame, &pitch, &vidProcParams) != CUDA_SUCCESS)
            return false;

        const size_t size = pitch * videoFrame.size.height;
        const size_t halfSize = pitch * ((videoFrame.size.height + 1) >> 1);

        const qint32 linesize[2] = {
            (qint32)pitch,
            (qint32)pitch
        };
        quint8 *data[2] = {
            new quint8[size],
            new quint8[halfSize]
        };

        bool copied = (cu::memcpyDtoH(data[0], mappedFrame, size) == CUDA_SUCCESS);
        if (copied)
            copied &= (cu::memcpyDtoH(data[1], mappedFrame + m_codedHeight * pitch, halfSize) == CUDA_SUCCESS);

        cuvid::unmapVideoFrame(m_cuvidDec, mappedFrame);

        cuCtxGuard.unlock();

        if (copied)
            nv12ToRGB32->scale((const void **)data, linesize, dest);

        for (int p = 0; p < 2; ++p)
            delete[] data[p];

        return copied;
    }

    /**/

    inline void allowDestroyCuda()
    {
        m_canDestroyCuda = true;
    }

    inline CUcontext getCudaContext() const
    {
        return m_cuCtx;
    }

    inline void setAvailableSurface(quintptr surfaceId)
    {
        m_validSurfaces.insert(surfaceId);
    }

    void setDecoderAndCodedHeight(CUvideodecoder cuvidDec, int codedHeight)
    {
        m_codedHeight = codedHeight;
        m_lastId = 0;
        m_tff = false;
        m_cuvidDec = cuvidDec;
        m_validSurfaces.clear();
    }

private:
    bool m_canDestroyCuda;

    int m_codedHeight;

    quintptr m_lastId;
    bool m_tff;

    CUcontext m_cuCtx;
    CUvideodecoder m_cuvidDec;

    CUgraphicsResource m_res[2];

    QSet<quintptr> m_validSurfaces;
};

/**/

constexpr quint32 g_maxSurfaces = 25;

QMutex CuvidDec::loadMutex;
int CuvidDec::loadState = -1;

bool CuvidDec::canCreateInstance()
{
    QMutexLocker locker(&loadMutex);
    return (loadState != 0);
}

CuvidDec::CuvidDec(Module &module) :
    m_writer(nullptr),
    m_cuvidHWAccel(nullptr),
    m_lastCuvidTS(0),
    m_deintMethod(cudaVideoDeinterlaceMode_Weave),
    m_copyVideo(Qt::PartiallyChecked),
    m_forceFlush(false),
    m_tsWorkaround(false),
    m_nv12Chroma(nullptr),
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
    memset(m_frameBuffer, 0, sizeof m_frameBuffer);
    SetModule(module);
}
CuvidDec::~CuvidDec()
{
    if (m_cuCtx)
    {
        cu::ContextGuard cuCtxGuard(m_cuCtx);
        destroyCuvid(true);
        if (!m_writer)
            cu::ctxDestroy(m_cuCtx);
    }
    av_bsf_free(&m_bsfCtx);
    if (m_swsCtx)
        sws_freeContext(m_swsCtx);
    av_packet_free(&m_pkt);
    av_buffer_unref(&m_nv12Chroma);
    for (int p = 0; p < 3; ++p)
        av_buffer_unref(&m_frameBuffer[p]);
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

        const Qt::CheckState copyVideo = (Qt::CheckState)sets().getInt("CopyVideo");
        if (copyVideo != m_copyVideo)
        {
            m_copyVideo = copyVideo;
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
    cuvidDecInfo.DeinterlaceMode = (!m_writer || format->progressive_sequence) ? cudaVideoDeinterlaceMode_Weave : m_deintMethod;
    cuvidDecInfo.OutputFormat = cudaVideoSurfaceFormat_NV12;

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

    if (!m_cuvidHWAccel)
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

    if (m_cuvidHWAccel)
        m_cuvidHWAccel->setDecoderAndCodedHeight(m_cuvidDec, m_codedHeight);

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
    if (dispInfo->timestamp >= 0 && dispInfo->timestamp < m_lastCuvidTS)
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

VideoWriter *CuvidDec::HWAccel() const
{
    return m_writer;
}

int CuvidDec::decodeVideo(Packet &encodedPacket, VideoFrame &decoded, QByteArray &newPixFmt, bool flush, unsigned hurry_up)
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
        {
            encodedPacket.ts.setInvalid();
            return -1;
        }
    }

    memset(&cuvidPkt, 0, sizeof cuvidPkt);
    if (encodedPacket.isEmpty())
        cuvidPkt.flags = CUVID_PKT_ENDOFSTREAM;
    else
    {
        cuvidPkt.flags = CUVID_PKT_TIMESTAMP;
        cuvidPkt.timestamp = encodedPacket.ts.pts() * 10000000.0 + 0.5;
        if (m_bsfCtx)
        {
            m_pkt->buf = encodedPacket.toAvBufferRef();
            m_pkt->data = m_pkt->buf->data + encodedPacket.offset();
            m_pkt->size = encodedPacket.size();

            if (av_bsf_send_packet(m_bsfCtx, m_pkt) < 0) //It unrefs "pkt"
            {
                encodedPacket.ts.setInvalid();
                return -1;
            }

            if (av_bsf_receive_packet(m_bsfCtx, m_pkt) < 0) //Can it return more than one packet in this case?
            {
                encodedPacket.ts.setInvalid();
                return -1;
            }

            cuvidPkt.payload = m_pkt->data;
            cuvidPkt.payload_size = m_pkt->size;
        }
        else
        {
            cuvidPkt.payload = encodedPacket.constData();
            cuvidPkt.payload_size = encodedPacket.size();
        }
    }

    const bool videoDataParsed = (cuvid::parseVideoData(m_cuvidParser, &cuvidPkt) == CUDA_SUCCESS);

    if (cuvidPkt.flags == CUVID_PKT_TIMESTAMP && videoDataParsed)
        m_timestamps.enqueue(encodedPacket.ts);

    if (m_pkt)
        av_packet_unref(m_pkt);

    if (m_cuvidSurfaces.isEmpty())
        encodedPacket.ts.setInvalid();
    else
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
        encodedPacket.ts = ts;

        if (~hurry_up)
        {
            const VideoFrameSize frameSize(m_width, m_height);
            if (m_cuvidHWAccel)
            {
                const quintptr surfaceId = dispInfo.picture_index + 1;
                m_cuvidHWAccel->setAvailableSurface(surfaceId);
                decoded = VideoFrame(frameSize, surfaceId, (bool)!dispInfo.progressive_frame, (bool)dispInfo.top_field_first);
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

                    if (!m_nv12Chroma || m_nv12Chroma->size != size)
                    {
                        av_buffer_unref(&m_nv12Chroma);
                        m_nv12Chroma = av_buffer_alloc(size);
                    }

                    for (int p = 0; p < 3; ++p)
                    {
                        const int planeSize = p ? halfSize : size;
                        if (!m_frameBuffer[p] || m_frameBuffer[p]->size != planeSize)
                        {
                            av_buffer_unref(&m_frameBuffer[p]);
                            m_frameBuffer[p] = av_buffer_alloc(planeSize);
                        }
                    }

                    bool copied = (cu::memcpyDtoH(m_frameBuffer[0]->data, mappedFrame, size) == CUDA_SUCCESS);
                    if (copied)
                        copied &= (cu::memcpyDtoH(m_nv12Chroma->data, mappedFrame + m_codedHeight * pitch, halfSize) == CUDA_SUCCESS);

                    cuvid::unmapVideoFrame(m_cuvidDec, mappedFrame);

                    if (copied)
                    {
                        const uint8_t *srcData[2] =
                        {
                            m_frameBuffer[0]->data,
                            m_nv12Chroma->data
                        };
                        const qint32 srcLinesize[2] =
                        {
                            (int)pitch,
                            (int)pitch
                        };

                        uint8_t *dtsData[3] =
                        {
                            m_frameBuffer[0]->data,
                            m_frameBuffer[1]->data,
                            m_frameBuffer[2]->data
                        };
                        qint32 dstLinesize[3] =
                        {
                            (qint32)pitch,
                            (qint32)(pitch >> 1),
                            (qint32)(pitch >> 1)
                        };

                        if (m_swsCtx)
                            sws_scale(m_swsCtx, srcData, srcLinesize, 0, m_height, dtsData, dstLinesize);

                        decoded = VideoFrame(frameSize, m_frameBuffer, dstLinesize, !dispInfo.progressive_frame, dispInfo.top_field_first);
                    }
                }
            }
        }
    }

    if (!videoDataParsed)
        return -1;

    return encodedPacket.size();
}

bool CuvidDec::hasCriticalError() const
{
    return m_hasCriticalError;
}

bool CuvidDec::open(StreamInfo &streamInfo, VideoWriter *writer)
{
    if (streamInfo.type != QMPLAY2_TYPE_VIDEO)
        return false;

    AVCodec *avCodec = avcodec_find_decoder_by_name(streamInfo.codec_name);
    if (!avCodec)
        return false;

    const AVPixelFormat pixFmt = av_get_pix_fmt(streamInfo.format);
    if (!(pixFmt == AV_PIX_FMT_YUV420P || pixFmt == AV_PIX_FMT_YUV420P10 || pixFmt == AV_PIX_FMT_YUVJ420P || avCodec->id == AV_CODEC_ID_MJPEG))
        return false;

    int depth = 8;
    if (const AVPixFmtDescriptor *pixDesc = av_pix_fmt_desc_get(pixFmt))
    {
        depth = pixDesc->comp[0].depth;
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
            if (depth > 8)
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
            if (depth > 8)
                return false;
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
        extraData = streamInfo.data;
    else
    {
        av_bsf_alloc(bsf, &m_bsfCtx);

        m_bsfCtx->par_in->codec_id = avCodec->id;
        m_bsfCtx->par_in->extradata_size = streamInfo.data.size();
        m_bsfCtx->par_in->extradata = (uint8_t *)av_malloc(m_bsfCtx->par_in->extradata_size + AV_INPUT_BUFFER_PADDING_SIZE);
        memcpy(m_bsfCtx->par_in->extradata, streamInfo.data.constData(), m_bsfCtx->par_in->extradata_size);
        memset(m_bsfCtx->par_in->extradata + m_bsfCtx->par_in->extradata_size, 0, AV_INPUT_BUFFER_PADDING_SIZE);

        if (av_bsf_init(m_bsfCtx) < 0)
            return false;

        extraData = QByteArray::fromRawData((const char *)m_bsfCtx->par_out->extradata, m_bsfCtx->par_out->extradata_size);

        m_pkt = av_packet_alloc();
    }

    m_width = streamInfo.W;
    m_height = streamInfo.H;
    m_codedHeight = m_height;

    if (writer)
    {
        m_cuvidHWAccel = dynamic_cast<CuvidHWAccel *>(writer->getHWAccelInterface());
        if (m_cuvidHWAccel)
        {
            m_cuCtx = m_cuvidHWAccel->getCudaContext();
            m_writer = writer;
        }
    }

    if (!m_cuCtx && !(m_cuCtx = cu::createContext()))
        return false;

    if (!m_writer && m_copyVideo != Qt::Checked)
    {
        m_writer = VideoWriter::createOpenGL2(new CuvidHWAccel(m_cuCtx));
        if (m_writer)
            m_cuvidHWAccel = (CuvidHWAccel *)m_writer->getHWAccelInterface();
        else if (m_copyVideo == Qt::Unchecked)
        {
            QMPlay2Core.logError("CUVID :: " + tr("Can't open OpenGL 2 module"), true, true);
            return false;
        }
    }

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

    if (!testDecoder(depth))
        err = true;
    else if (!createCuvidVideoParser())
        err = true;

    if (err)
    {
        if (!writer)
        {
            delete m_writer;
            m_writer = nullptr;
            m_cuvidHWAccel = nullptr;
        }
        return false;
    }

    if (m_cuvidHWAccel)
        m_cuvidHWAccel->allowDestroyCuda();

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
    if (m_cuvidHWAccel)
        m_cuvidHWAccel->setDecoderAndCodedHeight(nullptr, 0);
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
    QMutexLocker locker(&loadMutex);
    if (loadState == -1)
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
                    loadState = 0;
                    return false;
                }
            }
        }
#endif
        loadState = (cuvid::load() && cu::load());
    }
    return (loadState == 1);
}
