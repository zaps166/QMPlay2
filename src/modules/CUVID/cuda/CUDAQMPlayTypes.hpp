/* Types compatible with cuda.h */

#pragma once

#include <QtGlobal>

#ifdef Q_OS_WIN
    #define CUDAAPI __stdcall
#else
    #define CUDAAPI
#endif

enum CUresult
{
    CUDA_SUCCESS               =   0,
    CUDA_ERROR_INVALID_VALUE   =   1,
    CUDA_ERROR_OUT_OF_MEMORY   =   2,

    //...

    CUDA_ERROR_INVALID_CONTEXT = 201,

    //...

    CUDA_ERROR_INVALID_HANDLE  = 400,

    //...

    CUDA_ERROR_NOT_SUPPORTED   = 801,
    CUDA_ERROR_UNKNOWN         = 999,
};
enum CUctx_flags
{
    CU_CTX_SCHED_AUTO          = 0x00,
    CU_CTX_SCHED_SPIN          = 0x01,
    CU_CTX_SCHED_YIELD         = 0x02,
    CU_CTX_SCHED_BLOCKING_SYNC = 0x04,
    CU_CTX_BLOCKING_SYNC       = 0x04,


    CU_CTX_SCHED_MASK          = 0x07,
    CU_CTX_MAP_HOST            = 0x08,
    CU_CTX_LMEM_RESIZE_TO_MAX  = 0x10,
    CU_CTX_FLAGS_MASK          = 0x1f
};
enum CUmemorytype
{
    CU_MEMORYTYPE_HOST    = 0x01,
    CU_MEMORYTYPE_DEVICE  = 0x02,
    CU_MEMORYTYPE_ARRAY   = 0x03,
    CU_MEMORYTYPE_UNIFIED = 0x04
};
enum CUgraphicsRegisterFlags
{
    CU_GRAPHICS_REGISTER_FLAGS_NONE           = 0x00,
    CU_GRAPHICS_REGISTER_FLAGS_READ_ONLY      = 0x01,
    CU_GRAPHICS_REGISTER_FLAGS_WRITE_DISCARD  = 0x02,
    CU_GRAPHICS_REGISTER_FLAGS_SURFACE_LDST   = 0x04,
    CU_GRAPHICS_REGISTER_FLAGS_TEXTURE_GATHER = 0x08
};
enum CUexternalMemoryHandleType
{
    CU_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD        = 0x01,
    CU_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32     = 0x02,
    CU_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_KMT = 0x03
};
enum CUexternalSemaphoreHandleType {
    CU_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD        = 1,
    CU_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32     = 2,
    CU_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_KMT = 3,
    CU_EXTERNAL_SEMAPHORE_HANDLE_TYPE_D3D12_FENCE      = 4
};
enum CUstreamFlags
{
    CU_STREAM_DEFAULT = 0x0,
    CU_STREAM_NON_BLOCKING = 0x1
};

typedef int CUdevice;
typedef void *CUcontext;
typedef quintptr CUdeviceptr;
typedef void *CUarray;
typedef void *CUgraphicsResource;
typedef void *CUstream;
typedef void *CUexternalMemory;
typedef void *CUexternalSemaphore;

struct CUDA_MEMCPY2D
{
    size_t srcXInBytes, srcY;

    CUmemorytype srcMemoryType;
    const void *srcHost;
    CUdeviceptr srcDevice;
    CUarray srcArray;
    size_t srcPitch;

    size_t dstXInBytes, dstY;

    CUmemorytype dstMemoryType;
    void *dstHost;
    CUdeviceptr dstDevice;
    CUarray dstArray;
    size_t dstPitch;

    size_t WidthInBytes, Height;
};

struct CUDA_EXTERNAL_MEMORY_HANDLE_DESC
{
    CUexternalMemoryHandleType type;
    union
    {
        int fd;
        struct
        {
            void *handle;
            const void *name;
        } win32;
    } handle;
    unsigned long long size;
    unsigned int flags;
    unsigned int reserved[16];
};

struct CUDA_EXTERNAL_MEMORY_BUFFER_DESC
{
    unsigned long long offset;
    unsigned long long size;
    unsigned int flags;
    unsigned int reserved[16];
};

struct CUDA_EXTERNAL_SEMAPHORE_HANDLE_DESC
{
    CUexternalSemaphoreHandleType type;
    union {
        int fd;
        struct {
            void *handle;
            const void *name;
        } win32;
    } handle;
    unsigned int flags;
    unsigned int reserved[16];
};

struct CUDA_EXTERNAL_SEMAPHORE_SIGNAL_PARAMS
{
    struct {
        struct {
            unsigned long long value;
        } fence;
        unsigned int reserved[16];
    } params;
    unsigned int flags;
    unsigned int reserved[16];
};
using CUDA_EXTERNAL_SEMAPHORE_WAIT_PARAMS = CUDA_EXTERNAL_SEMAPHORE_SIGNAL_PARAMS;
