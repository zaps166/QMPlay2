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

#include <VTBOpenGL.hpp>

#include <ImgScaler.hpp>
#include <Frame.hpp>

#include <QOpenGLContext>
#include <QImage>

extern "C"
{
    #include <libavutil/hwcontext.h>
    #include <libavutil/hwcontext_videotoolbox.h>
}

VTBOpenGL::VTBOpenGL(AVBufferRef *hwDeviceBufferRef)
    : m_hwDeviceBufferRef(av_buffer_ref(hwDeviceBufferRef))
{}
VTBOpenGL::~VTBOpenGL()
{
    av_buffer_unref(&m_hwDeviceBufferRef);
}

QString VTBOpenGL::name() const
{
    return "VideoToolBox";
}

VTBOpenGL::Format VTBOpenGL::getFormat() const
{
    return NV12;
}
bool VTBOpenGL::isTextureRectangle() const
{
    return true;
}
bool VTBOpenGL::isCopy() const
{
    return false;
}

bool VTBOpenGL::init(const int *widths, const int *heights, const SetTextureParamsFn &setTextureParamsFn)
{
    for (int p = 0; p < 2; ++p)
    {
        if (m_widths[p] != widths[p] || m_heights[p] != heights[p])
        {
            clear();
            for (int p = 0; p < 2; ++p)
            {
                m_widths[p] = widths[p];
                m_heights[p] = heights[p];
            }
            glGenTextures(2, m_textures);
            break;
        }
    }
    for (int p = 0; p < 2; ++p)
        setTextureParamsFn(m_textures[p]);
    return true;
}
void VTBOpenGL::clear()
{
    glDeleteTextures(2, m_textures);
    memset(m_textures, 0, sizeof(m_textures));
    memset(m_widths, 0, sizeof(m_widths));
    memset(m_heights, 0, sizeof(m_heights));
}

bool VTBOpenGL::mapFrame(Frame &videoFrame)
{
    CVPixelBufferRef pixelBuffer = (CVPixelBufferRef)videoFrame.hwData();
    CGLContextObj glCtx = CGLGetCurrentContext();

    IOSurfaceRef surface = CVPixelBufferGetIOSurface(pixelBuffer);

    const OSType pixelFormat = IOSurfaceGetPixelFormat(surface);
    if (pixelFormat != kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange)
    {
        m_error = true;
        return false;
    }

    glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_textures[0]);
    if (CGLTexImageIOSurface2D(glCtx, GL_TEXTURE_RECTANGLE_ARB, GL_R8, videoFrame.width(0), videoFrame.height(0), GL_RED, GL_UNSIGNED_BYTE, surface, 0) != kCGLNoError)
    {
        m_error = true;
        return false;
    }

    glBindTexture(GL_TEXTURE_RECTANGLE_ARB, m_textures[1]);
    if (CGLTexImageIOSurface2D(glCtx, GL_TEXTURE_RECTANGLE_ARB, GL_RG8, videoFrame.width(1), videoFrame.height(1), GL_RG, GL_UNSIGNED_BYTE, surface, 1) != kCGLNoError)
    {
        m_error = true;
        return false;
    }

    return true;
}
quint32 VTBOpenGL::getTexture(int plane)
{
    return m_textures[plane];
}

QImage VTBOpenGL::getImage(const Frame &videoFrame)
{
    CVPixelBufferRef pixelBuffer = (CVPixelBufferRef)videoFrame.hwData();
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

        QImage img;

        ImgScaler imgScaler;
        if (imgScaler.create(videoFrame))
        {
            img = QImage(videoFrame.width(), videoFrame.height(), QImage::Format_RGB32);
            imgScaler.scale((const void **)srcData, srcLinesize, img.bits());
        }

        CVPixelBufferUnlockBaseAddress(pixelBuffer, kCVPixelBufferLock_ReadOnly);

        return img;
    }
    return QImage();
}
