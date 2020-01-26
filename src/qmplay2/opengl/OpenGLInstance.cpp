#include "OpenGLInstance.hpp"
#include "OpenGLWriter.hpp"

#include <QOffscreenSurface>
#include <QSet>

bool OpenGLInstance::init()
{
    QOffscreenSurface surface;
    QOpenGLContext context;

    surface.create();
    if (!context.create() || !context.makeCurrent(&surface))
        return false;

#ifndef OPENGL_ES2
    const auto extenstions = context.extensions();
    const bool supportsShaders = extenstions.contains("GL_ARB_vertex_shader") && extenstions.contains("GL_ARB_fragment_shader") && extenstions.contains("GL_ARB_shader_objects");
    const bool canCreateNonPowerOfTwoTextures = extenstions.contains("GL_ARB_texture_non_power_of_two");
    if (!supportsShaders || !canCreateNonPowerOfTwoTextures)
        return false;

    glActiveTexture = (GLActiveTexture)context.getProcAddress("glActiveTexture");
    if (!glActiveTexture)
        return false;

    glGenBuffers = (GLGenBuffers)context.getProcAddress("glGenBuffers");
    glBindBuffer = (GLBindBuffer)context.getProcAddress("glBindBuffer");
    glBufferData = (GLBufferData)context.getProcAddress("glBufferData");
    glDeleteBuffers = (GLDeleteBuffers)context.getProcAddress("glDeleteBuffers");
    hasVBO = glGenBuffers && glBindBuffer && glBufferData && glDeleteBuffers;
#endif
    glMapBufferRange = (GLMapBufferRange)context.getProcAddress("glMapBufferRange");
    glMapBuffer = (GLMapBuffer)context.getProcAddress("glMapBuffer");
    glUnmapBuffer = (GLUnmapBuffer)context.getProcAddress("glUnmapBuffer");
    hasPBO = hasVBO && (glMapBufferRange || glMapBuffer) && glUnmapBuffer;

#if !defined(OPENGL_ES2) && !defined(Q_OS_MACOS) // On macOS I have always OpenGL 2.1...
    int glMajor = 0, glMinor = 0;
    glGetIntegerv(GL_MAJOR_VERSION, &glMajor);
    glGetIntegerv(GL_MINOR_VERSION, &glMinor);

    if (!glMajor)
    {
        const QString glVersionStr = (const char *)glGetString(GL_VERSION);
        const int dotIdx = glVersionStr.indexOf('.');
        if (dotIdx > 0)
        {
            const int vIdx = glVersionStr.lastIndexOf(' ', dotIdx);
            if (sscanf(glVersionStr.mid(vIdx < 0 ? 0 : vIdx).toLatin1().constData(), "%d.%d", &glMajor, &glMinor) != 2)
                glMajor = glMinor = 0;
        }
    }
    if (glMajor)
        glVer = glMajor * 10 + glMinor;
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
