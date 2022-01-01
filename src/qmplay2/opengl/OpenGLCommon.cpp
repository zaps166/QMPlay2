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

#include "OpenGLCommon.hpp"
#include "OpenGLVertices.hpp"
#include "OpenGLHWInterop.hpp"

#include <Sphere.hpp>

#include <QMPlay2Core.hpp>
#include <Frame.hpp>
#include <Functions.hpp>

#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLShader>
#include <QResizeEvent>
#include <QResource>
#include <QPainter>
#include <QLibrary>
#include <QWidget>

/* OpenGL|ES 2.0 doesn't have those definitions */
#ifndef GL_MAP_WRITE_BIT
    #define GL_MAP_WRITE_BIT 0x0002
#endif
#ifndef GL_MAP_INVALIDATE_BUFFER_BIT
    #define GL_MAP_INVALIDATE_BUFFER_BIT 0x0008
#endif
#ifndef GL_WRITE_ONLY
    #define GL_WRITE_ONLY 0x88B9
#endif
#ifndef GL_PIXEL_UNPACK_BUFFER
    #define GL_PIXEL_UNPACK_BUFFER 0x88EC
#endif
#ifndef GL_TEXTURE_RECTANGLE_ARB
    #define GL_TEXTURE_RECTANGLE_ARB 0x84F5
#endif

OpenGLCommon::OpenGLCommon() :
    VideoOutputCommon(false),
    vSync(true),
    m_glInstance(std::static_pointer_cast<OpenGLInstance>(QMPlay2Core.gpuInstance())),
    shaderProgramVideo(nullptr), shaderProgramOSD(nullptr),
    texCoordYCbCrLoc(-1), positionYCbCrLoc(-1), texCoordOSDLoc(-1), positionOSDLoc(-1),
    numPlanes(0),
    target(0),
    hasPbo(false),
    isPaused(false), isOK(false), hasImage(false), doReset(true), setMatrix(true), correctLinesize(false), canUseHueSharpness(true),
    outW(-1), outH(-1), verticesIdx(0),
    hasVbo(true),
    nIndices(0)
{
#ifndef OPENGL_ES2
    glActiveTexture = m_glInstance->glActiveTexture;
    glGenBuffers = m_glInstance->glGenBuffers;
    glBindBuffer = m_glInstance->glBindBuffer;
    glBufferData = m_glInstance->glBufferData;
    glDeleteBuffers = m_glInstance->glDeleteBuffers;
#endif
    glMapBufferRange = m_glInstance->glMapBufferRange;
    glMapBuffer = m_glInstance->glMapBuffer;
    glUnmapBuffer = m_glInstance->glUnmapBuffer;

    videoAdjustment.unset();

    /* Initialize texCoordYCbCr array */
    texCoordYCbCr[0] = texCoordYCbCr[4] = texCoordYCbCr[5] = texCoordYCbCr[7] = 0.0f;
    texCoordYCbCr[1] = texCoordYCbCr[3] = 1.0f;

#ifndef Q_OS_MACOS
    canUseHueSharpness = (m_glInstance->glVer >= 30);
#endif

    m_matrixChangeFn = [this] {
        setMatrix = true;
        updateGL(true);
    };
}
OpenGLCommon::~OpenGLCommon()
{
    contextAboutToBeDestroyed();
}

void OpenGLCommon::deleteMe()
{
    delete this;
}

void OpenGLCommon::initialize(const std::shared_ptr<OpenGLHWInterop> &hwInterop)
{
    if (isOK && m_hwInterop == hwInterop)
        return;

    isOK = true;

    numPlanes = 3;
    target = GL_TEXTURE_2D;

    if (!m_hwInterop && !hwInterop)
        return;

    const bool windowContext = makeContextCurrent();

    if (windowContext)
        contextAboutToBeDestroyed();
    m_hwInterop.reset();

    videoAdjustmentKeys.clear();

    if (hwInterop)
    {
        QOffscreenSurface surface;
        QOpenGLContext context;
        if (!windowContext)
        {
            surface.create();
            if (!context.create() || !context.makeCurrent(&surface))
            {
                isOK = false;
                return;
            }
        }

        switch (hwInterop->getFormat())
        {
            case OpenGLHWInterop::NV12:
                numPlanes = 2;
                break;
            case OpenGLHWInterop::RGB32:
                numPlanes = 1;
                break;
        }

        if (hwInterop->isTextureRectangle())
        {
            target = GL_TEXTURE_RECTANGLE_ARB;
            if (numPlanes == 1)
                isOK = false; // Not used and not supported
        }

        const QVector<int> sizes(numPlanes * 2, 1);
        if (!hwInterop->init(&sizes[0], &sizes[numPlanes], [](quint32){}))
            isOK = false;

        if (numPlanes == 1) //For RGB32 format, HWAccel should be able to adjust the video
        {
            VideoAdjustment videoAdjustmentCap;
            hwInterop->getVideAdjustmentCap(videoAdjustmentCap);
            if (videoAdjustmentCap.brightness)
                videoAdjustmentKeys += "Brightness";
            if (videoAdjustmentCap.contrast)
                videoAdjustmentKeys += "Contrast";
            if (videoAdjustmentCap.saturation)
                videoAdjustmentKeys += "Saturation";
            if (videoAdjustmentCap.hue)
                videoAdjustmentKeys += "Hue";
            if (videoAdjustmentCap.sharpness)
                videoAdjustmentKeys += "Sharpness";
        }

        hwInterop->clear();

        if (isOK)
            m_hwInterop = hwInterop;
    }

    if (windowContext)
    {
        initializeGL();
        doneContextCurrent();
    }
}

#ifdef Q_OS_WIN
void OpenGLCommon::setWindowsBypassCompositor(bool bypassCompositor)
{
    if (!bypassCompositor && QSysInfo::windowsVersion() <= QSysInfo::WV_6_1) // Windows 7 and Vista can disable DWM composition, so check it
    {
        using DwmIsCompositionEnabledProc = HRESULT (WINAPI *)(BOOL *pfEnabled);
        if (auto DwmIsCompositionEnabled = (DwmIsCompositionEnabledProc)GetProcAddress(GetModuleHandleA("dwmapi.dll"), "DwmIsCompositionEnabled"))
        {
            BOOL enabled = false;
            if (DwmIsCompositionEnabled(&enabled) == S_OK && !enabled)
                bypassCompositor = true; // Don't try to avoid compositor bypass if compositor is disabled
        }
    }
    widget()->setProperty("bypassCompositor", bypassCompositor ? 1 : -1);
}
#endif

void OpenGLCommon::newSize(bool canUpdate)
{
    updateSizes(isRotate90());
    doReset = true;
    if (canUpdate)
    {
        if (isPaused)
            updateGL(false);
        else if (!updateTimer.isActive())
            updateTimer.start(40);
    }
}
void OpenGLCommon::clearImg()
{
    hasImage = false;
    osdImg = QImage();
    videoFrame.clear();
    osd_ids.clear();
}

bool OpenGLCommon::setSphericalView(bool spherical)
{
    if (hasVbo)
        return VideoOutputCommon::setSphericalView(spherical);
    return false;
}

void OpenGLCommon::setTextureParameters(GLenum target, quint32 texture, GLint param)
{
    glBindTexture(target, texture);
    glTexParameteri(target, GL_TEXTURE_MIN_FILTER, param);
    glTexParameteri(target, GL_TEXTURE_MAG_FILTER, param);
    glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(target, 0);
}

void OpenGLCommon::initializeGL()
{
    shaderProgramVideo.reset(new QOpenGLShaderProgram);
    shaderProgramOSD.reset(new QOpenGLShaderProgram);

    /* YCbCr shader */
    shaderProgramVideo->addShaderFromSourceCode(QOpenGLShader::Vertex, readShader(":/opengl/Video.vert"));
    QByteArray videoFrag;
    if (numPlanes == 1)
    {
        videoFrag = readShader(":/opengl/VideoRGB.frag");
        if (canUseHueSharpness)
        {
            //Use sharpness only when OpenGL/OpenGL|ES version >= 3.0, because it can be slow on old hardware and/or buggy drivers and may increase CPU usage!
            videoFrag.prepend("#define Sharpness\n");
        }
    }
    else
    {
        videoFrag = readShader(":/opengl/VideoYCbCr.frag");
        if (canUseHueSharpness)
        {
            //Use hue and sharpness only when OpenGL/OpenGL|ES version >= 3.0, because it can be slow on old hardware and/or buggy drivers and may increase CPU usage!
            videoFrag.prepend("#define HueAndSharpness\n");
        }
        if (numPlanes == 2)
            videoFrag.prepend("#define NV12\n");
    }
    if (target == GL_TEXTURE_RECTANGLE_ARB)
        videoFrag.prepend("#define TEXTURE_RECTANGLE\n");
    shaderProgramVideo->addShaderFromSourceCode(QOpenGLShader::Fragment, videoFrag);
    if (shaderProgramVideo->bind())
    {
        texCoordYCbCrLoc = shaderProgramVideo->attributeLocation("aTexCoord");
        positionYCbCrLoc = shaderProgramVideo->attributeLocation("aPosition");
        shaderProgramVideo->setUniformValue((numPlanes == 1) ? "uRGB" : "uY" , 0);
        if (numPlanes == 2)
            shaderProgramVideo->setUniformValue("uCbCr", 1);
        else if (numPlanes == 3)
        {
            shaderProgramVideo->setUniformValue("uCb", 1);
            shaderProgramVideo->setUniformValue("uCr", 2);
        }
        shaderProgramVideo->release();
    }
    else
    {
        QMPlay2Core.logError(tr("Shader compile/link error"));
        isOK = false;
        return;
    }

    /* OSD shader */
    shaderProgramOSD->addShaderFromSourceCode(QOpenGLShader::Vertex, readShader(":/opengl/OSD.vert"));
    shaderProgramOSD->addShaderFromSourceCode(QOpenGLShader::Fragment, readShader(":/opengl/OSD.frag"));
    if (shaderProgramOSD->bind())
    {
        texCoordOSDLoc = shaderProgramOSD->attributeLocation("aTexCoord");
        positionOSDLoc = shaderProgramOSD->attributeLocation("aPosition");
        shaderProgramOSD->setUniformValue("uTex", 3);
        shaderProgramOSD->release();
    }
    else
    {
        QMPlay2Core.logError(tr("Shader compile/link error"));
        isOK = false;
        return;
    }

    /* Set OpenGL parameters */
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_DITHER);

    /* Prepare textures */
    const int texturesToGen = m_hwInterop ? 0 : numPlanes;
    glGenTextures(texturesToGen + 1, textures);
    for (int i = 0; i < texturesToGen + 1; ++i)
    {
        const quint32 tmpTarget = (i == 0) ? GL_TEXTURE_2D : target;
        setTextureParameters(tmpTarget, textures[i], (i == 0) ? GL_NEAREST : GL_LINEAR);
    }

    if (hasPbo)
    {
        glGenBuffers(1 + texturesToGen, pbo);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    }

    setVSync(vSync);

    doReset = true;
    resetSphereVbo();
}

void OpenGLCommon::paintGL()
{
    const bool frameIsEmpty = videoFrame.isEmpty();

    if (updateTimer.isActive())
        updateTimer.stop();

    if (frameIsEmpty && !hasImage)
        return;

    bool resetDone = false;

    if (!frameIsEmpty)
    {
        const GLsizei widths[3] = {
            videoFrame.width(0),
            videoFrame.width(1),
            videoFrame.width(2),
        };
        const GLsizei heights[3] = {
            videoFrame.height(0),
            videoFrame.height(1),
            videoFrame.height(2),
        };

        if (doReset)
        {
            if (m_hwInterop)
            {
                m_textureSize = QSize(widths[0], heights[0]);

                const bool hwAccelInitError = !m_hwInterop->init(widths, heights, [this](quint32 texture) {
                    setTextureParameters(target, texture, GL_LINEAR);
                });
                if (hwAccelInitError)
                    QMPlay2Core.logError("OpenGL 2 :: " + tr("Can't init %1") .arg(m_hwInterop->name()));

                if (numPlanes == 1)
                    m_hwInterop->setVideoAdjustment(videoAdjustment);

                /* Prepare texture coordinates */
                texCoordYCbCr[2] = texCoordYCbCr[6] = 1.0f;
            }
            else
            {
                /* Check linesize */
                const qint32 halfLinesize = (videoFrame.linesize(0) >> videoFrame.chromaShiftW());
                correctLinesize =
                (
                    (halfLinesize == videoFrame.linesize(1) && videoFrame.linesize(1) == videoFrame.linesize(2)) &&
                    (!m_sphericalView ? (videoFrame.linesize(1) == halfLinesize) : (videoFrame.linesize(0) == widths[0]))
                );

                /* Prepare textures */
                for (qint32 p = 0; p < 3; ++p)
                {
                    const GLsizei w = correctLinesize ? videoFrame.linesize(p) : widths[p];
                    const GLsizei h = heights[p];
                    if (p == 0)
                        m_textureSize = QSize(w, h);
                    if (hasPbo)
                    {
                        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo[p + 1]);
                        glBufferData(GL_PIXEL_UNPACK_BUFFER, w * h, nullptr, GL_DYNAMIC_DRAW);
                    }
                    glBindTexture(GL_TEXTURE_2D, textures[p + 1]);
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, w, h, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, nullptr);
                }

                /* Prepare texture coordinates */
                texCoordYCbCr[2] = texCoordYCbCr[6] = (videoFrame.linesize(0) == widths[0]) ? 1.0f : (widths[0] / (videoFrame.linesize(0) + 1.0f));
            }
            resetDone = true;
            hasImage = false;
        }

        if (m_hwInterop)
        {
            bool imageReady = false;
            if (!m_hwInterop->hasError())
            {
                if (m_hwInterop->mapFrame(videoFrame))
                {
                    imageReady = true;
                }
                else if (m_hwInterop->hasError())
                {
                    QMPlay2Core.logError("OpenGL 2 :: " + m_hwInterop->name() + " " + tr("texture map error"));
                }
            }
            if (!imageReady && !hasImage)
                return;
            for (int p = 0; p < numPlanes; ++p)
            {
                glActiveTexture(GL_TEXTURE0 + p);
                glBindTexture(target, m_hwInterop->getTexture(p));
            }
        }
        else
        {
            /* Load textures */
            for (qint32 p = 0; p < 3; ++p)
            {
                const quint8 *data = videoFrame.constData(p);
                const GLsizei w = correctLinesize ? videoFrame.linesize(p) : widths[p];
                const GLsizei h = heights[p];
                if (hasPbo)
                {
                    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo[p + 1]);
                    quint8 *dst;
                    if (glMapBufferRange)
                        dst = (quint8 *)glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, w * h, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
                    else
                        dst = (quint8 *)glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
                    if (!dst)
                        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
                    else
                    {
                        if (correctLinesize)
                            memcpy(dst, data, w * h);
                        else for (int y = 0; y < h; ++y)
                        {
                            memcpy(dst, data, w);
                            data += videoFrame.linesize(p);
                            dst  += w;
                        }
                        glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
                        data = nullptr;
                    }
                }
                glActiveTexture(GL_TEXTURE0 + p);
                glBindTexture(GL_TEXTURE_2D, textures[p + 1]);
                if (hasPbo || correctLinesize)
                    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_LUMINANCE, GL_UNSIGNED_BYTE, data);
                else for (int y = 0; y < h; ++y)
                {
                    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, y, w, 1, GL_LUMINANCE, GL_UNSIGNED_BYTE, data);
                    data += videoFrame.linesize(p);
                }
            }
            if (hasPbo)
                glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
        }

        if (!m_hwInterop || m_hwInterop->isCopy())
            videoFrame.clear();
        hasImage = true;
    }

    if (!m_sphericalView)
    {
        deleteSphereVbo();
        shaderProgramVideo->setAttributeArray(positionYCbCrLoc, verticesYCbCr[verticesIdx], 2);
        shaderProgramVideo->setAttributeArray(texCoordYCbCrLoc, texCoordYCbCr, 2);
    }
    else
    {
        if (nIndices == 0)
            loadSphere();

        glBindBuffer(GL_ARRAY_BUFFER, sphereVbo[0]);
        shaderProgramVideo->setAttributeBuffer(positionYCbCrLoc, GL_FLOAT, 0, 3);

        glBindBuffer(GL_ARRAY_BUFFER, sphereVbo[1]);
        shaderProgramVideo->setAttributeBuffer(texCoordYCbCrLoc, GL_FLOAT, 0, 2);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    shaderProgramVideo->enableAttributeArray(positionYCbCrLoc);
    shaderProgramVideo->enableAttributeArray(texCoordYCbCrLoc);

    shaderProgramVideo->bind();
    if (doReset)
    {
        const float brightness = videoAdjustment.brightness / 100.0f;
        const float contrast   = (videoAdjustment.contrast + 100) / 100.0f;
        const float sharpness  = videoAdjustment.sharpness / 50.0f;
        if (m_hwInterop && numPlanes == 1)
        {
            const bool hasBrightness = videoAdjustmentKeys.contains("Brightness");
            const bool hasContrast   = videoAdjustmentKeys.contains("Contrast");
            const bool hasSharpness  = videoAdjustmentKeys.contains("Sharpness");
            shaderProgramVideo->setUniformValue
            (
                "uVideoAdj",
                hasBrightness ? 0.0f : brightness,
                hasContrast   ? 1.0f : contrast,
                hasSharpness  ? 0.0f : sharpness
            );
        }
        else
        {
            const auto mat = Functions::getYUVtoRGBmatrix(Functions::getLumaCoeff(m_colorSpace), m_limited).toGenericMatrix<3, 3>();
            shaderProgramVideo->setUniformValue("uYUVtRGB", mat);
            shaderProgramVideo->setUniformValue("uBL", m_limited ? 16.0f / 255.0f : 0.0f);

            const float saturation = (videoAdjustment.saturation + 100) / 100.0f;
            const float hue = videoAdjustment.hue / -31.831f;
            shaderProgramVideo->setUniformValue("uVideoEq", brightness, contrast, saturation, hue);
            shaderProgramVideo->setUniformValue("uSharpness", sharpness);
        }
        shaderProgramVideo->setUniformValue("uTextureSize", m_textureSize);

        doReset = !resetDone;
        setMatrix = true;
    }
    if (setMatrix)
    {
        updateMatrix();
        shaderProgramVideo->setUniformValue("uMatrix", m_matrix);
        setMatrix = false;
    }
    if (!m_sphericalView)
    {
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }
    else
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereVbo[2]);
        glDrawElements(GL_TRIANGLE_STRIP, nIndices, GL_UNSIGNED_SHORT, nullptr);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }
    shaderProgramVideo->release();

    shaderProgramVideo->disableAttributeArray(texCoordYCbCrLoc);
    shaderProgramVideo->disableAttributeArray(positionYCbCrLoc);

    glActiveTexture(GL_TEXTURE3);

    /* OSD */
    osdMutex.lock();
    if (!osdList.isEmpty())
    {
        glBindTexture(GL_TEXTURE_2D, textures[0]);

        QRect bounds;
        const qreal scaleW = m_subsRect.width() / outW, scaleH = m_subsRect.height() / outH;
        bool mustRepaint = Functions::mustRepaintOSD(osdList, osd_ids, &scaleW, &scaleH, &bounds);
        bool hasNewSize = false;
        if (!mustRepaint)
            mustRepaint = osdImg.size() != bounds.size();
        if (mustRepaint)
        {
            if (osdImg.size() != bounds.size())
            {
                osdImg = QImage(bounds.size(), QImage::Format_ARGB32);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bounds.width(), bounds.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
                hasNewSize = true;
            }
            osdImg.fill(0);
            QPainter p(&osdImg);
            p.translate(-bounds.topLeft());
            Functions::paintOSD(false, osdList, scaleW, scaleH, p, &osd_ids);
            const quint8 *data = osdImg.constBits();
            if (hasPbo)
            {
                const GLsizeiptr dataSize = (osdImg.width() * osdImg.height()) << 2;
                glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo[0]);
                if (hasNewSize)
                    glBufferData(GL_PIXEL_UNPACK_BUFFER, dataSize, nullptr, GL_DYNAMIC_DRAW);
                quint8 *dst;
                if (glMapBufferRange)
                    dst = (quint8 *)glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, dataSize, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
                else
                    dst = (quint8 *)glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
                if (!dst)
                    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
                else
                {
                    memcpy(dst, data, dataSize);
                    glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
                    data = nullptr;
                }
            }
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, bounds.width(), bounds.height(), GL_RGBA, GL_UNSIGNED_BYTE, data);
            if (hasPbo && !data)
                glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
        }

        const auto realWinSize = m_widget->devicePixelRatioF() * m_widget->size();
        const float left   = (bounds.left() + m_subsRect.x()) * 2.0f / realWinSize.width() - m_osdOffset.x();
        const float right  = (bounds.right() + m_subsRect.x() + 1.0f) * 2.0f / realWinSize.width() - m_osdOffset.x();
        const float top    = (bounds.top() + m_subsRect.y()) * 2.0f / realWinSize.height() - m_osdOffset.y();
        const float bottom = (bounds.bottom() + m_subsRect.y() + 1.0f) * 2.0f / realWinSize.height() - m_osdOffset.y();
        const float verticesOSD[8] = {
            left  - 1.0f, -bottom + 1.0f,
            right - 1.0f, -bottom + 1.0f,
            left  - 1.0f, -top    + 1.0f,
            right - 1.0f, -top    + 1.0f,
        };

        shaderProgramOSD->setAttributeArray(positionOSDLoc, verticesOSD, 2);
        shaderProgramOSD->setAttributeArray(texCoordOSDLoc, texCoordOSD, 2);
        shaderProgramOSD->enableAttributeArray(positionOSDLoc);
        shaderProgramOSD->enableAttributeArray(texCoordOSDLoc);

        glEnable(GL_BLEND);
        shaderProgramOSD->bind();
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        shaderProgramOSD->release();
        glDisable(GL_BLEND);

        shaderProgramOSD->disableAttributeArray(texCoordOSDLoc);
        shaderProgramOSD->disableAttributeArray(positionOSDLoc);
    }
    osdMutex.unlock();

    glBindTexture(GL_TEXTURE_2D, 0);
}

void OpenGLCommon::contextAboutToBeDestroyed()
{
    if (m_hwInterop)
        m_hwInterop->clear();
    deleteSphereVbo();
    const int texturesToDel = m_hwInterop ? 0 : numPlanes;
    if (hasPbo)
        glDeleteBuffers(1 + texturesToDel, pbo);
    glDeleteTextures(texturesToDel + 1, textures);
}

void OpenGLCommon::dispatchEvent(QEvent *e, QObject *p)
{
    if (e->type() == QEvent::Resize)
        newSize(false);
    VideoOutputCommon::dispatchEvent(e, p);
}

inline bool OpenGLCommon::isRotate90() const
{
    return verticesIdx >= 4 && !m_sphericalView;
}

QByteArray OpenGLCommon::readShader(const QString &fileName, bool pure)
{
    QResource res(fileName);
    QByteArray shader;
    if (!pure)
    {
#ifdef OPENGL_ES2
        shader = "precision highp float;\n";
#endif
        shader.append("#line 1\n");
    }
    shader.append((const char *)res.data(), res.size());
    return shader;
}

inline void OpenGLCommon::resetSphereVbo()
{
    memset(sphereVbo, 0, sizeof sphereVbo);
    nIndices = 0;
}
inline void OpenGLCommon::deleteSphereVbo()
{
    if (nIndices > 0)
    {
        glDeleteBuffers(3, sphereVbo);
        resetSphereVbo();
    }
}
void OpenGLCommon::loadSphere()
{
    const quint32 slices = 50;
    const quint32 stacks = 50;
    const GLenum targets[3] = {
        GL_ARRAY_BUFFER,
        GL_ARRAY_BUFFER,
        GL_ELEMENT_ARRAY_BUFFER
    };
    void *pointers[3];
    quint32 sizes[3];
    nIndices = Sphere::getSizes(slices, stacks, sizes[0], sizes[1], sizes[2]);
    glGenBuffers(3, sphereVbo);
    for (qint32 i = 0; i < 3; ++i)
        pointers[i] = malloc(sizes[i]);
    Sphere::generate(1.0f, slices, stacks, (float *)pointers[0], (float *)pointers[1], (quint16 *)pointers[2]);
    for (qint32 i = 0; i < 3; ++i)
    {
        glBindBuffer(targets[i], sphereVbo[i]);
        glBufferData(targets[i], sizes[i], pointers[i], GL_STATIC_DRAW);
        free(pointers[i]);
    }
}
