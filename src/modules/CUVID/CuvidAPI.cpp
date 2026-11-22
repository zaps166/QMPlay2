// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <CuvidAPI.hpp>

#include <QLibrary>
#include <QMutex>

namespace cu {

static QRecursiveMutex mutex;

cuInitType init = nullptr;
cuDeviceGetType deviceGet = nullptr;
cuCtxCreateType ctxCreate = nullptr;
cuCtxPushCurrentType ctxPushCurrent = nullptr;
cuCtxPopCurrentType ctxPopCurrent = nullptr;
cuMemcpyDtoHType memcpyDtoH = nullptr;
cuMemcpy2DType memcpy2D = nullptr;
cuGraphicsGLRegisterImageType graphicsGLRegisterImage = nullptr;
cuGraphicsMapResourcesType graphicsMapResources = nullptr;
cuGraphicsSubResourceGetMappedArrayType graphicsSubResourceGetMappedArray = nullptr;
cuGraphicsUnmapResourcesType graphicsUnmapResources = nullptr;
cuGraphicsUnregisterResourceType graphicsUnregisterResource = nullptr;
cuMemcpy2DAsyncType memcpy2DAsync = nullptr;
cuImportExternalSemaphoreType importExternalSemaphore = nullptr;
cuSignalExternalSemaphoresAsyncType signalExternalSemaphoresAsync = nullptr;
cuWaitExternalSemaphoresAsyncType waitExternalSemaphoresAsync = nullptr;
cuDestroyExternalSemaphoreType destroyExternalSemaphore = nullptr;
cuStreamCreateType streamCreate = nullptr;
cuStreamDestroyType streamDestroy = nullptr;
cuImportExternalMemory importExternalMemory = nullptr;
cuExternalMemoryGetMappedBuffer externalMemoryGetMappedBuffer = nullptr;
cuExternalMemoryGetMappedMipmappedArray externalMemoryGetMappedMipmappedArray = nullptr;
cuMipmappedArrayGetLevel mipmappedArrayGetLevel = nullptr;
cuMipmappedArrayDestroy mipmappedArrayDestroy = nullptr;
cuDestroyExternalMemory destroyExternalMemory = nullptr;
cuDeviceGetPCIBusId deviceGetPCIBusId = nullptr;
cuMemFree memFree = nullptr;
cuCtxDestroyType ctxDestroy = nullptr;

bool load(bool doInit, bool gl, bool vk)
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
        ctxDestroy = (cuCtxDestroyType)lib.resolve("cuCtxDestroy_v2");

        bool hasPointers =
            init &&
            deviceGet &&
            ctxCreate &&
            ctxPushCurrent &&
            ctxPopCurrent &&
            memcpyDtoH &&
            memcpy2D &&
            ctxDestroy
        ;

        if (gl)
        {
            graphicsGLRegisterImage = (cuGraphicsGLRegisterImageType)lib.resolve("cuGraphicsGLRegisterImage");
            graphicsMapResources = (cuGraphicsMapResourcesType)lib.resolve("cuGraphicsMapResources");
            graphicsSubResourceGetMappedArray = (cuGraphicsSubResourceGetMappedArrayType)lib.resolve("cuGraphicsSubResourceGetMappedArray");
            graphicsUnmapResources = (cuGraphicsUnmapResourcesType)lib.resolve("cuGraphicsUnmapResources");
            graphicsUnregisterResource = (cuGraphicsUnregisterResourceType)lib.resolve("cuGraphicsUnregisterResource");

            hasPointers &=
                graphicsGLRegisterImage &&
                graphicsMapResources &&
                graphicsSubResourceGetMappedArray &&
                graphicsUnmapResources &&
                graphicsUnregisterResource
            ;
        }

        if (vk)
        {
            memcpy2DAsync = (cuMemcpy2DAsyncType)lib.resolve("cuMemcpy2DAsync_v2");
            importExternalSemaphore = (cuImportExternalSemaphoreType)lib.resolve("cuImportExternalSemaphore");
            signalExternalSemaphoresAsync = (cuSignalExternalSemaphoresAsyncType)lib.resolve("cuSignalExternalSemaphoresAsync");
            waitExternalSemaphoresAsync = (cuWaitExternalSemaphoresAsyncType)lib.resolve("cuWaitExternalSemaphoresAsync");
            destroyExternalSemaphore = (cuDestroyExternalSemaphoreType)lib.resolve("cuDestroyExternalSemaphore");
            streamCreate = (cuStreamCreateType)lib.resolve("cuStreamCreate");
            streamDestroy = (cuStreamDestroyType)lib.resolve("cuStreamDestroy_v2");
            importExternalMemory = (cuImportExternalMemory)lib.resolve("cuImportExternalMemory");
            externalMemoryGetMappedBuffer = (cuExternalMemoryGetMappedBuffer)lib.resolve("cuExternalMemoryGetMappedBuffer");
            externalMemoryGetMappedMipmappedArray = (cuExternalMemoryGetMappedMipmappedArray)lib.resolve("cuExternalMemoryGetMappedMipmappedArray");
            mipmappedArrayGetLevel = (cuMipmappedArrayGetLevel)lib.resolve("cuMipmappedArrayGetLevel");
            mipmappedArrayDestroy = (cuMipmappedArrayDestroy)lib.resolve("cuMipmappedArrayDestroy");
            destroyExternalMemory = (cuDestroyExternalMemory)lib.resolve("cuDestroyExternalMemory");
            deviceGetPCIBusId = (cuDeviceGetPCIBusId)lib.resolve("cuDeviceGetPCIBusId");
            memFree = (cuMemFree)lib.resolve("cuMemFree_v2");

            hasPointers &=
                memcpy2DAsync &&
                importExternalSemaphore &&
                signalExternalSemaphoresAsync &&
                waitExternalSemaphoresAsync &&
                destroyExternalSemaphore &&
                streamCreate &&
                streamDestroy &&
                importExternalMemory &&
                externalMemoryGetMappedBuffer &&
                externalMemoryGetMappedMipmappedArray &&
                mipmappedArrayGetLevel &&
                mipmappedArrayDestroy &&
                destroyExternalMemory &&
                memFree
            ;
        }

        if (!hasPointers)
            return false;

        return (!doInit || init(0) == CUDA_SUCCESS);
    }
    return false;
}

std::shared_ptr<CUcontext> createContext()
{
    CUcontext ctx, tmpCtx;
    CUdevice dev = -1;
    const int devIdx = 0;
    if (deviceGet(&dev, devIdx) != CUDA_SUCCESS)
        return nullptr;
    if (ctxCreate(&ctx, CU_CTX_SCHED_BLOCKING_SYNC, dev) != CUDA_SUCCESS)
        return nullptr;
    ctxPopCurrent(&tmpCtx);
    return std::shared_ptr<CUcontext>(new CUcontext(ctx), [](CUcontext *ctx) {
        cu::ContextGuard cuCtxGuard(*ctx);
        cu::ctxDestroy(*ctx);
        delete ctx;
    });
}

ContextGuard::ContextGuard(const std::shared_ptr<CUcontext> &ctx)
    : ContextGuard(*ctx)
{}
ContextGuard::ContextGuard(CUcontext ctx)
{
    mutex.lock();
    ctxPushCurrent(ctx);
}
ContextGuard::~ContextGuard()
{
    unlock();
}

void ContextGuard::unlock()
{
    if (m_locked)
    {
        CUcontext ctx;
        ctxPopCurrent(&ctx);
        mutex.unlock();
        m_locked = false;
    }
}

}

namespace cuvid {

cuvidCreateVideoParserType createVideoParser = nullptr;
cuvidDestroyVideoParserType destroyVideoParser = nullptr;
cuvidDecodePictureType decodePicture = nullptr;
cuvidCreateDecoderType createDecoder = nullptr;
cuvidDestroyDecoderType destroyDecoder = nullptr;
cuvidMapVideoFrameType mapVideoFrame = nullptr;
cuvidUnmapVideoFrameType unmapVideoFrame = nullptr;
cuvidParseVideoDataType parseVideoData = nullptr;

bool load()
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

}
