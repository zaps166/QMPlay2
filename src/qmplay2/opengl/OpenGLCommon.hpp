/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2020  Błażej Szczygieł

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

#pragma once

#include "OpenGLInstance.hpp"

#include <Frame.hpp>
#include <VideoAdjustment.hpp>
#include <X11BypassCompositor.hpp>

#include <QOpenGLShaderProgram>

#include <QVariantAnimation>
#include <QCoreApplication>
#include <QImage>
#include <QMutex>
#include <QTimer>

#if !defined OPENGL_ES2 && !defined Q_OS_MACOS
    #include <GL/glext.h>
#endif

#if defined OPENGL_ES2 && !defined APIENTRY
    #define APIENTRY
#endif

class HWOpenGLInterop;
class OpenGLCommon;
class QMPlay2OSD;
class QMouseEvent;

class RotAnimation final : public QVariantAnimation
{
public:
    inline RotAnimation(OpenGLCommon &glCommon) :
        glCommon(glCommon)
    {}
private:
    void updateCurrentValue(const QVariant &value) override;

    OpenGLCommon &glCommon;
};

/**/

class OpenGLCommon : public X11BypassCompositor
{
    Q_DECLARE_TR_FUNCTIONS(OpenGLCommon)

public:
    OpenGLCommon();
    virtual ~OpenGLCommon();

    virtual void deleteMe();

    virtual QWidget *widget() = 0;

    virtual bool makeContextCurrent() = 0;
    virtual void doneContextCurrent() = 0;

    void initialize(const std::shared_ptr<HWOpenGLInterop> &hwInterop);

    virtual void setVSync(bool enable) = 0;
    virtual void updateGL(bool requestDelayed) = 0;

#ifdef Q_OS_WIN
    void setWindowsBypassCompositor(bool bypassCompositor);
#endif

    void newSize(const QSize &size = QSize());
    void clearImg();

    void setSpherical(bool spherical);
protected:
    static void setTextureParameters(GLenum target, quint32 texture, GLint param);

    void initializeGL();
    void paintGL();

    void contextAboutToBeDestroyed();

#ifndef OPENGL_ES2
    OpenGLInstance::GLActiveTexture glActiveTexture = nullptr;
    OpenGLInstance::GLGenBuffers glGenBuffers = nullptr;
    OpenGLInstance::GLBindBuffer glBindBuffer = nullptr;
    OpenGLInstance::GLBufferData glBufferData = nullptr;
    OpenGLInstance::GLDeleteBuffers glDeleteBuffers = nullptr;
#endif
    OpenGLInstance::GLMapBufferRange glMapBufferRange = nullptr;
    OpenGLInstance::GLMapBuffer glMapBuffer = nullptr;
    OpenGLInstance::GLUnmapBuffer glUnmapBuffer = nullptr;

    bool vSync;

    void dispatchEvent(QEvent *e, QObject *p);
private:
    inline bool isRotate90() const;

    QByteArray readShader(const QString &fileName, bool pure = false);

    void mousePress(QMouseEvent *e);
    void mouseMove(QMouseEvent *e);
    void mouseRelease(QMouseEvent *e);

    /* Spherical view */
    void mousePress360(QMouseEvent *e);
    void mouseMove360(QMouseEvent *e);
    void mouseRelease360(QMouseEvent *e);
    inline void resetSphereVbo();
    inline void deleteSphereVbo();
    void loadSphere();
public:
    const std::shared_ptr<OpenGLInstance> m_glInstance;
    std::shared_ptr<HWOpenGLInterop> m_hwInterop;
    QStringList videoAdjustmentKeys;
    Frame videoFrame;

    bool m_limited = false;
    AVColorSpace m_colorSpace = AVCOL_SPC_UNSPECIFIED;

    std::unique_ptr<QOpenGLShaderProgram> shaderProgramVideo, shaderProgramOSD;

    qint32 texCoordYCbCrLoc, positionYCbCrLoc, texCoordOSDLoc, positionOSDLoc;
    VideoAdjustment videoAdjustment;
    float texCoordYCbCr[8];
    quint32 textures[4];
    QSize m_textureSize;
    qint32 numPlanes;
    quint32 target;

    quint32 pbo[4];
    bool hasPbo;

    bool isPaused, isOK, hasImage, doReset, setMatrix, correctLinesize, canUseHueSharpness;
    int subsX, subsY, W, H, subsW, subsH, outW, outH, verticesIdx;

    double aspectRatio, zoom;

    bool moveVideo = false, moveOSD = false;
    QPointF videoOffset, osdOffset;

    QList<const QMPlay2OSD *> osdList;
    QMutex osdMutex;

    QVector<quint64> osd_ids;
    QImage osdImg;

    QTimer updateTimer;

    /* Spherical view */
    bool sphericalView, buttonPressed, hasVbo, mouseWrapped, canWrapMouse;
    RotAnimation rotAnimation;
    quint32 sphereVbo[3];
    quint32 nIndices;
    double mouseTime;
    QPoint mousePos; // also for moving video/subtitles
    QPointF rot;
};
