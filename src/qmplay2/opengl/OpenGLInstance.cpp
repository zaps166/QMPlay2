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

    hasVbo = fns->hasOpenGLFeature(QOpenGLFunctions::Buffers);
    if (hasVbo)
    {
        const auto extensions = context.extensions();

        if (!isGLES)
            hasMapBufferRange = extensions.contains("GL_ARB_map_buffer_range");
        else if (surface.format().majorVersion() >= 3)
            hasMapBufferRange = true;

        if (!isGLES)
            hasMapBuffer = true;
        else
            hasMapBuffer = extensions.contains("GL_OES_mapbuffer");

        hasPbo = hasVbo && (hasMapBuffer || hasMapBufferRange);
    }

    glVer = context.format().majorVersion() * 10 + context.format().minorVersion();

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
