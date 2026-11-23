// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include "OpenGLInstance.hpp"

#include <Frame.hpp>
#include <VideoAdjustment.hpp>
#include <VideoOutputCommon.hpp>
#include <QMPlay2OSD.hpp>

class OpenGLHWInterop;

class OpenGLCommon : public VideoOutputCommon, public QOpenGLExtraFunctions
{
    Q_DECLARE_TR_FUNCTIONS(OpenGLCommon)

public:
    OpenGLCommon();
    virtual ~OpenGLCommon();

    virtual void deleteMe();

    virtual bool makeContextCurrent() = 0;
    virtual void doneContextCurrent() = 0;

    inline double &zoomRef()
    {
        return m_zoom;
    }
    inline double &aRatioRef()
    {
        return m_aRatio;;
    }

    inline bool isSphericalView() const
    {
        return m_sphericalView;
    }

    void initialize(const std::shared_ptr<OpenGLHWInterop> &hwInterop);

    virtual void setVSync(bool enable) = 0;
    virtual void updateGL(bool requestDelayed) = 0;

#ifdef Q_OS_WIN
    void setWindowsBypassCompositor(bool bypassCompositor);
#endif

    void newSize(bool canUpdate);
    void clearImg();

    bool setSphericalView(bool spherical) override;
protected:
    void setTextureParameters(GLenum target, quint32 texture, GLint param);

    void initializeGL();
    void paintGL();

    void contextAboutToBeDestroyed();

#if !defined(QT_OPENGL_ES_2) && !defined(QT_FEATURE_opengles2)
    QOpenGLFunctions_1_5 m_gl15;
#endif

    bool vSync;

    void dispatchEvent(QEvent *e, QObject *p) override;
private:
    inline bool isRotate90() const;

    QByteArray readShader(const QString &fileName, bool pure = false);

    inline void resetSphereVbo();
    inline void deleteSphereVbo();
    void loadSphere();
public:
    const std::shared_ptr<OpenGLInstance> m_glInstance;
    std::shared_ptr<OpenGLHWInterop> m_hwInterop;
    QStringList videoAdjustmentKeys;
    Frame videoFrame;

    AVColorPrimaries m_colorPrimaries = AVCOL_PRI_UNSPECIFIED;
    AVColorTransferCharacteristic m_colorTrc = AVCOL_TRC_UNSPECIFIED;
    AVColorSpace m_colorSpace = AVCOL_SPC_UNSPECIFIED;
    float m_maxLuminance = 1000.0f;
    float m_bitsMultiplier = 1.0f;
    int m_depth = 8;
    bool m_limited = false;

    std::unique_ptr<QOpenGLShaderProgram> shaderProgramVideo, shaderProgramOSD;

    qint32 texCoordYCbCrLoc, positionYCbCrLoc, texCoordOSDLoc, positionOSDLoc;
    VideoAdjustment videoAdjustment;
    float texCoordYCbCr[8];
    quint32 textures[4];
    QSize m_textureSize;
    qint32 numPlanes;
    quint32 target;

    bool m_canUse16bitTexture = false;

    quint32 pbo[4];
    bool hasPbo;

    bool isPaused, isOK, hasImage, doReset, setMatrix, correctLinesize, m_gl3;
    int outW, outH, verticesIdx;

    QMPlay2OSDList osdList;

    QVector<quint64> osd_ids;
    QImage osdImg;

    QTimer updateTimer;

    /* Spherical view */
    bool hasVbo;
    quint32 sphereVbo[3];
    quint32 nIndices;
};
