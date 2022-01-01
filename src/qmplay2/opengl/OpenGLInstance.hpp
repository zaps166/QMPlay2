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

#pragma once

#include <GPUInstance.hpp>

#include <QOpenGLContext>

#if defined OPENGL_ES2 && !defined APIENTRY
    #define APIENTRY
#endif

class OpenGLInstance final : public GPUInstance
{
public:
#ifndef OPENGL_ES2
    using GLActiveTexture  = void  (APIENTRY *)(GLenum);
    using GLGenBuffers     = void  (APIENTRY *)(GLsizei, GLuint *);
    using GLBindBuffer     = void  (APIENTRY *)(GLenum, GLuint);
    using GLBufferData     = void  (APIENTRY *)(GLenum, GLsizeiptr, const void *, GLenum);
    using GLDeleteBuffers  = void  (APIENTRY *)(GLsizei, const GLuint *);
    using GLGenerateMipmap = void  (APIENTRY *)(GLenum);
#endif
    using GLMapBufferRange = void *(APIENTRY *)(GLenum, GLintptr, GLsizeiptr, GLbitfield);
    using GLMapBuffer      = void *(APIENTRY *)(GLenum, GLbitfield);
    using GLUnmapBuffer    = GLboolean(APIENTRY *)(GLenum);

public:
    bool init();

public:
    QString name() const override;
    QMPlay2CoreClass::Renderer renderer() const override;

    VideoWriter *createOrGetVideoOutput() override;

public:
#ifndef OPENGL_ES2
    GLActiveTexture glActiveTexture = nullptr;
    GLGenBuffers glGenBuffers = nullptr;
    GLBindBuffer glBindBuffer = nullptr;
    GLBufferData glBufferData = nullptr;
    GLDeleteBuffers glDeleteBuffers = nullptr;
#endif
    GLMapBufferRange glMapBufferRange = nullptr;
    GLMapBuffer glMapBuffer = nullptr;
    GLUnmapBuffer glUnmapBuffer = nullptr;

    bool hasVBO = false;
    bool hasPBO = false;

    int glVer = 0;
};
