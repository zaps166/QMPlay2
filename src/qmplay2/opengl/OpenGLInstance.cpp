#include "OpenGLInstance.hpp"
#include "OpenGLWriter.hpp"

#include <QOffscreenSurface>
#include <QOpenGLFunctions>
#include <QSet>

bool OpenGLInstance::init()
{
    QOffscreenSurface surface;
    QOpenGLContext context;

    surface.create();
    if (!context.create() || !context.makeCurrent(&surface))
        return false;

    auto fns = context.functions();

    if (!fns->hasOpenGLFeature(QOpenGLFunctions::Multitexture))
        return false;

    if (!fns->hasOpenGLFeature(QOpenGLFunctions::Shaders))
        return false;

    if (!fns->hasOpenGLFeature(QOpenGLFunctions::NPOTTextures))
        return false;

    isGLES = context.isOpenGLES();

    const auto extensions = context.extensions();
    const auto majorVersion = context.format().majorVersion();

    if (fns->hasOpenGLFeature(QOpenGLFunctions::TextureRGFormats))
    {
        canUse16bitTexture = (isGLES ? extensions.contains("GL_EXT_texture_norm16") : (majorVersion >= 3));
    }

    hasVbo = (majorVersion >= 2 || extensions.contains("GL_ARB_vertex_buffer_object"));
    if (hasVbo)
    {
        if (!isGLES)
            hasMapBufferRange = extensions.contains("GL_ARB_map_buffer_range");
        else if (majorVersion >= 3)
            hasMapBufferRange = true;

        hasMapBuffer = extensions.contains("GL_OES_mapbuffer");
        if (Q_UNLIKELY(!isGLES && !hasMapBuffer))
            hasMapBuffer = (majorVersion >= 2);

        hasPbo = (hasMapBuffer || hasMapBufferRange) && (majorVersion >= (isGLES ? 3 : 2) || extensions.contains("GL_ARB_pixel_buffer_object"));
    }

#ifndef Q_OS_MACOS // On macOS I have always OpenGL 2.1...
    glVer = majorVersion * 10 + context.format().minorVersion();
#endif

    return true;
}

QString OpenGLInstance::name() const
{
    return "opengl";
}
QMPlay2CoreClass::Renderer OpenGLInstance::renderer() const
{
    return QMPlay2CoreClass::Renderer::OpenGL;
}

VideoWriter *OpenGLInstance::createOrGetVideoOutput()
{
    if (!m_videoWriter)
        m_videoWriter = new OpenGLWriter;
    return m_videoWriter;
}
