//
// Copyright 2019 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// validationGL43.cpp: Validation functions for OpenGL 4.3 entry point parameters

#include "libANGLE/validationGL43_autogen.h"

namespace gl
{

bool ValidateClearBufferData(const Context *context,
                             GLenum target,
                             GLenum internalformat,
                             GLenum format,
                             GLenum type,
                             const void *data)
{
    return true;
}

bool ValidateClearBufferSubData(const Context *context,
                                GLenum target,
                                GLenum internalformat,
                                GLintptr offset,
                                GLsizeiptr size,
                                GLenum format,
                                GLenum type,
                                const void *data)
{
    return true;
}

bool ValidateGetInternalformati64v(const Context *context,
                                   GLenum target,
                                   GLenum internalformat,
                                   GLenum pname,
                                   GLsizei bufSize,
                                   const GLint64 *params)
{
    return true;
}

bool ValidateGetProgramResourceLocationIndex(const Context *context,
                                             ShaderProgramID program,
                                             GLenum programInterface,
                                             const GLchar *name)
{
    return true;
}

bool ValidateInvalidateBufferData(const Context *context, BufferID buffer)
{
    return true;
}

bool ValidateInvalidateBufferSubData(const Context *context,
                                     BufferID buffer,
                                     GLintptr offset,
                                     GLsizeiptr length)
{
    return true;
}

bool ValidateInvalidateTexImage(const Context *context, TextureID texture, GLint level)
{
    return true;
}

bool ValidateInvalidateTexSubImage(const Context *context,
                                   TextureID texture,
                                   GLint level,
                                   GLint xoffset,
                                   GLint yoffset,
                                   GLint zoffset,
                                   GLsizei width,
                                   GLsizei height,
                                   GLsizei depth)
{
    return true;
}

bool ValidateMultiDrawArraysIndirect(const Context *context,
                                     GLenum mode,
                                     const void *indirect,
                                     GLsizei drawcount,
                                     GLsizei stride)
{
    return true;
}

bool ValidateMultiDrawElementsIndirect(const Context *context,
                                       GLenum mode,
                                       GLenum type,
                                       const void *indirect,
                                       GLsizei drawcount,
                                       GLsizei stride)
{
    return true;
}

bool ValidateShaderStorageBlockBinding(const Context *context,
                                       ShaderProgramID program,
                                       GLuint storageBlockIndex,
                                       GLuint storageBlockBinding)
{
    return true;
}

bool ValidateTextureView(const Context *context,
                         TextureID texture,
                         GLenum target,
                         GLuint origtexture,
                         GLenum internalformat,
                         GLuint minlevel,
                         GLuint numlevels,
                         GLuint minlayer,
                         GLuint numlayers)
{
    return true;
}

bool ValidateVertexAttribLFormat(const Context *context,
                                 GLuint attribindex,
                                 GLint size,
                                 GLenum type,
                                 GLuint relativeoffset)
{
    return true;
}

}  // namespace gl
