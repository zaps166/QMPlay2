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

#include <CuvidOpenGL.hpp>
#include <ImgScaler.hpp>
#include <Frame.hpp>

#include <QOpenGLContext>
#include <QImage>

#ifndef GL_R8
    #define GL_R8 0x8229
#endif
#ifndef GL_RG8
    #define GL_RG8 0x822B
#endif
#ifndef GL_RED
    #define GL_RED 0x1903
#endif
#ifndef GL_RG
    #define GL_RG 0x8227
#endif

CuvidOpenGL::CuvidOpenGL(const std::shared_ptr<CUcontext> &cuCtx)
    : CuvidHWInterop(cuCtx)
{}
CuvidOpenGL::~CuvidOpenGL()
{
}

QString CuvidOpenGL::name() const
{
    return "CUVID";
}

CuvidOpenGL::Format CuvidOpenGL::getFormat() const
{
    return NV12;
}

bool CuvidOpenGL::init(const int *widths, const int *heights, const SetTextureParamsFn &setTextureParamsFn)
{
    cu::ContextGuard cuCtxGuard(m_cuCtx);

    bool mustRegister = false;
    for (int p = 0; p < 2; ++p)
    {
        if (m_widths[p] != widths[p] || m_heights[p] != heights[p])
        {
            clear();
            for (int p = 0; p < 2; ++p)
            {
                m_widths[p] = widths[p];
                m_heights[p] = heights[p];

                glGenTextures(1, &m_textures[p]);
                glBindTexture(GL_TEXTURE_2D, m_textures[p]);
                glTexImage2D(GL_TEXTURE_2D, 0, (p == 0) ? GL_R8 : GL_RG8, widths[p], heights[p], 0, (p == 0) ? GL_RED : GL_RG, GL_UNSIGNED_BYTE, nullptr);
            }
            mustRegister = true;
            break;
        }
    }

    for (int p = 0; p < 2; ++p)
        setTextureParamsFn(m_textures[p]);

    if (!mustRegister)
        return true;

    for (int p = 0; p < 2; ++p)
    {
        if (cu::graphicsGLRegisterImage(&m_res[p], m_textures[p], GL_TEXTURE_2D, CU_GRAPHICS_REGISTER_FLAGS_WRITE_DISCARD) != CUDA_SUCCESS)
        {
            m_error = true;
            return false;
        }
    }

    return true;
}
void CuvidOpenGL::clear()
{
    cu::ContextGuard cuCtxGuard(m_cuCtx);
    for (int p = 0; p < 2; ++p)
    {
        if (m_res[p])
        {
            cu::graphicsUnregisterResource(m_res[p]);
            m_res[p] = nullptr;
        }
        if (m_textures[p])
        {
            glDeleteTextures(1, &m_textures[p]);
            m_textures[p] = 0;
        }
        m_widths[p] = 0;
        m_heights[p] = 0;
    }
}

bool CuvidOpenGL::mapFrame(Frame &videoFrame)
{
    cu::ContextGuard cuCtxGuard(m_cuCtx);

    const int pictureIdx = videoFrame.customData();

    if (!m_cuvidDec || m_validPictures.count(pictureIdx) == 0)
        return false;

    CUVIDPROCPARAMS vidProcParams;
    memset(&vidProcParams, 0, sizeof vidProcParams);

    vidProcParams.top_field_first = videoFrame.isTopFieldFirst();
    if (videoFrame.isInterlaced())
        vidProcParams.second_field = videoFrame.isSecondField();
    else
        vidProcParams.progressive_frame = true;

    quintptr mappedFrame = 0;
    unsigned pitch = 0;

    if (cuvid::mapVideoFrame(m_cuvidDec, pictureIdx, &mappedFrame, &pitch, &vidProcParams) != CUDA_SUCCESS)
    {
        m_error = true;
        return false;
    }

    if (cu::graphicsMapResources(2, m_res, nullptr) == CUDA_SUCCESS)
    {
        bool copied = true;

        CUDA_MEMCPY2D cpy;
        memset(&cpy, 0, sizeof cpy);
        cpy.srcMemoryType = CU_MEMORYTYPE_DEVICE;
        cpy.dstMemoryType = CU_MEMORYTYPE_ARRAY;
        cpy.srcDevice = mappedFrame;
        cpy.srcPitch = pitch;
        cpy.WidthInBytes = videoFrame.width();
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
            cpy.Height = videoFrame.height(p);

            if (cu::memcpy2D(&cpy) != CUDA_SUCCESS)
            {
                copied = false;
                break;
            }
        }

        cu::graphicsUnmapResources(2, m_res, nullptr);

        if (cuvid::unmapVideoFrame(m_cuvidDec, mappedFrame) == CUDA_SUCCESS && copied)
            return true;
    }

    m_error = true;
    return false;
}
quint32 CuvidOpenGL::getTexture(int plane)
{
    return m_textures[plane];
}

QImage CuvidOpenGL::getImage(const Frame &videoFrame)
{
    cu::ContextGuard cuCtxGuard(m_cuCtx);

    quintptr mappedFrame = 0;
    unsigned pitch = 0;

    CUVIDPROCPARAMS vidProcParams;
    memset(&vidProcParams, 0, sizeof vidProcParams);
    vidProcParams.progressive_frame = !videoFrame.isInterlaced();
    vidProcParams.top_field_first = videoFrame.isTopFieldFirst();

    if (cuvid::mapVideoFrame(m_cuvidDec, videoFrame.customData(), &mappedFrame, &pitch, &vidProcParams) != CUDA_SUCCESS)
        return QImage();

    const size_t size = pitch * videoFrame.height();
    const size_t halfSize = pitch * ((videoFrame.height() + 1) >> 1);

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

    QImage img;

    if (copied)
    {
        ImgScaler imgScaler;
        if (imgScaler.create(videoFrame))
        {
            img = QImage(videoFrame.width(), videoFrame.height(), QImage::Format_RGB32);
            imgScaler.scale((const void **)data, linesize, img.bits());
        }
    }

    for (int p = 0; p < 2; ++p)
        delete[] data[p];

    return img;
}
