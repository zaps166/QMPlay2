#pragma once

#include <CuvidAPI.hpp>

#include <unordered_set>

class Frame;

class CuvidHWInterop
{
public:
    inline CuvidHWInterop(const std::shared_ptr<CUcontext> &cuCtx)
        : m_cuCtx(cuCtx)
    {}
    virtual ~CuvidHWInterop() = default;

    inline std::shared_ptr<CUcontext> getCudaContext() const
    {
        return m_cuCtx;
    }

    inline void setAvailableSurface(quintptr surfaceId)
    {
        // Mutex is not needed, because "m_cuCtx" is already locked
        m_validPictures.insert(surfaceId);
    }

    inline void setDecoderAndCodedHeight(CUvideodecoder cuvidDec, int codedHeight)
    {
        // Mutex is not needed, because "m_cuCtx" is already locked
        m_codedHeight = codedHeight;
        m_cuvidDec = cuvidDec;
        m_validPictures.clear();
    }

protected:
    const std::shared_ptr<CUcontext> m_cuCtx;

    CUvideodecoder m_cuvidDec = nullptr;
    int m_codedHeight = 0;
    std::unordered_set<int> m_validPictures;
};
