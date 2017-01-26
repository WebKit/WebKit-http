/*
 * Copyright (C) 2017 Konstantin Tokarev <annulen@yandex.ru>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include <private/qopenglextensions_p.h>

#ifndef FUNCTIONS
#error You must define FUNCTIONS macro before including this header
#endif

namespace OpenGLShimsQt {
ALWAYS_INLINE static void doNothing() { }
}

#define LOOKUP_GL_FUNCTION(f, ...) OpenGLShimsQt::doNothing(), FUNCTIONS->f(__VA_ARGS__)

// Extra items
#define glBindTexture(...)                     LOOKUP_GL_FUNCTION(glBindTexture, __VA_ARGS__)
#define glBlendFunc(...)                       LOOKUP_GL_FUNCTION(glBlendFunc, __VA_ARGS__)
#define glClear(...)                           LOOKUP_GL_FUNCTION(glClear, __VA_ARGS__)
#define glClearColor(...)                      LOOKUP_GL_FUNCTION(glClearColor, __VA_ARGS__)
#define glClearDepth(...)                      LOOKUP_GL_FUNCTION(glClearDepthf, __VA_ARGS__)
#define glClearStencil(...)                    LOOKUP_GL_FUNCTION(glClearStencil, __VA_ARGS__)
#define glColorMask(...)                       LOOKUP_GL_FUNCTION(glColorMask, __VA_ARGS__)
#define glCopyTexImage2D(...)                  LOOKUP_GL_FUNCTION(glCopyTexImage2D, __VA_ARGS__)
#define glCopyTexSubImage2D(...)               LOOKUP_GL_FUNCTION(glCopyTexSubImage2D, __VA_ARGS__)
#define glCullFace(...)                        LOOKUP_GL_FUNCTION(glCullFace, __VA_ARGS__)
#define glDeleteTextures(...)                  LOOKUP_GL_FUNCTION(glDeleteTextures, __VA_ARGS__)
#define glDepthFunc(...)                       LOOKUP_GL_FUNCTION(glDepthFunc, __VA_ARGS__)
#define glDepthMask(...)                       LOOKUP_GL_FUNCTION(glDepthMask, __VA_ARGS__)
#define glDepthRange(...)                      LOOKUP_GL_FUNCTION(glDepthRangef, __VA_ARGS__)
#define glDisable(...)                         LOOKUP_GL_FUNCTION(glDisable, __VA_ARGS__)
#define glDrawArrays(...)                      LOOKUP_GL_FUNCTION(glDrawArrays, __VA_ARGS__)
#define glDrawElements(...)                    LOOKUP_GL_FUNCTION(glDrawElements, __VA_ARGS__)
#define glEnable(...)                          LOOKUP_GL_FUNCTION(glEnable, __VA_ARGS__)
#define glFinish(...)                          LOOKUP_GL_FUNCTION(glFinish, __VA_ARGS__)
#define glFlush(...)                           LOOKUP_GL_FUNCTION(glFlush, __VA_ARGS__)
#define glFrontFace(...)                       LOOKUP_GL_FUNCTION(glFrontFace, __VA_ARGS__)
#define glGenTextures(...)                     LOOKUP_GL_FUNCTION(glGenTextures, __VA_ARGS__)
#define glGetBooleanv(...)                     LOOKUP_GL_FUNCTION(glGetBooleanv, __VA_ARGS__)
#define glGetFloatv(...)                       LOOKUP_GL_FUNCTION(glGetFloatv, __VA_ARGS__)
#define glGetIntegerv(...)                     LOOKUP_GL_FUNCTION(glGetIntegerv, __VA_ARGS__)
#define glGetString(...)                       LOOKUP_GL_FUNCTION(glGetString, __VA_ARGS__)
#define glGetTexParameterfv(...)               LOOKUP_GL_FUNCTION(glGetTexParameterfv, __VA_ARGS__)
#define glGetTexParameteriv(...)               LOOKUP_GL_FUNCTION(glGetTexParameteriv, __VA_ARGS__)
#define glHint(...)                            LOOKUP_GL_FUNCTION(glHint, __VA_ARGS__)
#define glIsTexture(...)                       LOOKUP_GL_FUNCTION(glIsTexture, __VA_ARGS__)
#define glLineWidth(...)                       LOOKUP_GL_FUNCTION(glLineWidth, __VA_ARGS__)
#define glPixelStorei(...)                     LOOKUP_GL_FUNCTION(glPixelStorei, __VA_ARGS__)
#define glPolygonOffset(...)                   LOOKUP_GL_FUNCTION(glPolygonOffset, __VA_ARGS__)
#define glReadPixels(...)                      LOOKUP_GL_FUNCTION(glReadPixels, __VA_ARGS__)
#define glScissor(...)                         LOOKUP_GL_FUNCTION(glScissor, __VA_ARGS__)
#define glStencilFunc(...)                     LOOKUP_GL_FUNCTION(glStencilFunc, __VA_ARGS__)
#define glStencilMask(...)                     LOOKUP_GL_FUNCTION(glStencilMask, __VA_ARGS__)
#define glStencilOp(...)                       LOOKUP_GL_FUNCTION(glStencilOp, __VA_ARGS__)
#define glTexImage2D(...)                      LOOKUP_GL_FUNCTION(glTexImage2D, __VA_ARGS__)
#define glTexParameterf(...)                   LOOKUP_GL_FUNCTION(glTexParameterf, __VA_ARGS__)
#define glTexParameteri(...)                   LOOKUP_GL_FUNCTION(glTexParameteri, __VA_ARGS__)
#define glTexSubImage2D(...)                   LOOKUP_GL_FUNCTION(glTexSubImage2D, __VA_ARGS__)
#define glViewport(...)                        LOOKUP_GL_FUNCTION(glViewport, __VA_ARGS__)

// Keep in sync with OpenGLShims.h

#define glActiveTexture(...)                   LOOKUP_GL_FUNCTION(glActiveTexture, __VA_ARGS__)
#define glAttachShader(...)                    LOOKUP_GL_FUNCTION(glAttachShader, __VA_ARGS__)
#define glBindAttribLocation(...)              LOOKUP_GL_FUNCTION(glBindAttribLocation, __VA_ARGS__)
#define glBindBuffer(...)                      LOOKUP_GL_FUNCTION(glBindBuffer, __VA_ARGS__)
#define glBindFramebufferEXT                   glBindFramebuffer
#define glBindFramebuffer(...)                 LOOKUP_GL_FUNCTION(glBindFramebuffer, __VA_ARGS__)
#define glBindRenderbufferEXT                  glBindRenderbuffer
#define glBindRenderbuffer(...)                LOOKUP_GL_FUNCTION(glBindRenderbuffer, __VA_ARGS__)
#define glBlendColor(...)                      LOOKUP_GL_FUNCTION(glBlendColor, __VA_ARGS__)
#define glBlendEquation(...)                   LOOKUP_GL_FUNCTION(glBlendEquation, __VA_ARGS__)
#define glBlendEquationSeparate(...)           LOOKUP_GL_FUNCTION(glBlendEquationSeparate, __VA_ARGS__)
#define glBlendFuncSeparate(...)               LOOKUP_GL_FUNCTION(glBlendFuncSeparate, __VA_ARGS__)
#define glBlitFramebufferEXT                   glBlitFramebuffer
#define glBlitFramebuffer(...)                 LOOKUP_GL_FUNCTION(glBlitFramebuffer, __VA_ARGS__)
#define glBufferData(...)                      LOOKUP_GL_FUNCTION(glBufferData, __VA_ARGS__)
#define glBufferSubData(...)                   LOOKUP_GL_FUNCTION(glBufferSubData, __VA_ARGS__)
#define glCheckFramebufferStatusEXT            glCheckFramebufferStatus
#define glCheckFramebufferStatus(...)          LOOKUP_GL_FUNCTION(glCheckFramebufferStatus, __VA_ARGS__)
#define glCompileShader(...)                   LOOKUP_GL_FUNCTION(glCompileShader, __VA_ARGS__)
#define glCompressedTexImage2D(...)            LOOKUP_GL_FUNCTION(glCompressedTexImage2D, __VA_ARGS__)
#define glCompressedTexSubImage2D(...)         LOOKUP_GL_FUNCTION(glCompressedTexSubImage2D, __VA_ARGS__)
#define glCreateProgram(...)                   LOOKUP_GL_FUNCTION(glCreateProgram, __VA_ARGS__)
#define glCreateShader(...)                    LOOKUP_GL_FUNCTION(glCreateShader, __VA_ARGS__)
#define glDeleteBuffers(...)                   LOOKUP_GL_FUNCTION(glDeleteBuffers, __VA_ARGS__)
#define glDeleteFramebuffersEXT                glDeleteFramebuffers
#define glDeleteFramebuffers(...)              LOOKUP_GL_FUNCTION(glDeleteFramebuffers, __VA_ARGS__)
#define glDeleteProgram(...)                   LOOKUP_GL_FUNCTION(glDeleteProgram, __VA_ARGS__)
#define glDeleteRenderbuffersEXT               glDeleteRenderbuffers
#define glDeleteRenderbuffers(...)             LOOKUP_GL_FUNCTION(glDeleteRenderbuffers, __VA_ARGS__)
#define glDeleteShader(...)                    LOOKUP_GL_FUNCTION(glDeleteShader, __VA_ARGS__)
#define glDetachShader(...)                    LOOKUP_GL_FUNCTION(glDetachShader, __VA_ARGS__)
#define glDisableVertexAttribArray(...)        LOOKUP_GL_FUNCTION(glDisableVertexAttribArray, __VA_ARGS__)
#define glDrawArraysInstancedEXT               glDrawArraysInstanced
#define glDrawArraysInstanced(...)             LOOKUP_GL_FUNCTION(glDrawArraysInstanced, __VA_ARGS__)
#define glDrawBuffersEXT                       glDrawBuffers
#define glDrawBuffers(...)                     LOOKUP_GL_FUNCTION(glDrawBuffers, __VA_ARGS__)
#define glDrawElementsInstancedEXT             glDrawElementsInstanced
#define glDrawElementsInstanced(...)           LOOKUP_GL_FUNCTION(glDrawElementsInstanced, __VA_ARGS__)
#define glEnableVertexAttribArray(...)         LOOKUP_GL_FUNCTION(glEnableVertexAttribArray, __VA_ARGS__)
#define glFramebufferRenderbufferEXT           glFramebufferRenderbuffer
#define glFramebufferRenderbuffer(...)         LOOKUP_GL_FUNCTION(glFramebufferRenderbuffer, __VA_ARGS__)
#define glFramebufferTexture2DEXT              glFramebufferTexture2D
#define glFramebufferTexture2D(...)            LOOKUP_GL_FUNCTION(glFramebufferTexture2D, __VA_ARGS__)
#define glGenBuffers(...)                      LOOKUP_GL_FUNCTION(glGenBuffers, __VA_ARGS__)
#define glGenerateMipmapEXT                    glGenerateMipmap
#define glGenerateMipmap(...)                  LOOKUP_GL_FUNCTION(glGenerateMipmap, __VA_ARGS__)
#define glGenFramebuffersEXT                   glGenFramebuffers
#define glGenFramebuffers(...)                 LOOKUP_GL_FUNCTION(glGenFramebuffers, __VA_ARGS__)
#define glGenRenderbuffersEXT                  glGenRenderbuffers
#define glGenRenderbuffers(...)                LOOKUP_GL_FUNCTION(glGenRenderbuffers, __VA_ARGS__)
#define glGetActiveAttrib(...)                 LOOKUP_GL_FUNCTION(glGetActiveAttrib, __VA_ARGS__)
#define glGetActiveUniform(...)                LOOKUP_GL_FUNCTION(glGetActiveUniform, __VA_ARGS__)
#define glGetAttachedShaders(...)              LOOKUP_GL_FUNCTION(glGetAttachedShaders, __VA_ARGS__)
#define glGetAttribLocation(...)               LOOKUP_GL_FUNCTION(glGetAttribLocation, __VA_ARGS__)
#define glGetBufferParameterivEXT              glGetBufferParameteriv
#define glGetBufferParameteriv(...)            LOOKUP_GL_FUNCTION(glGetBufferParameteriv, __VA_ARGS__)
#define glGetFramebufferAttachmentParameterivEXT glGetFramebufferAttachmentParameteriv
#define glGetFramebufferAttachmentParameteriv(...)  LOOKUP_GL_FUNCTION(glGetFramebufferAttachmentParameteriv, __VA_ARGS__)
#define glGetProgramInfoLog(...)               LOOKUP_GL_FUNCTION(glGetProgramInfoLog, __VA_ARGS__)
#define glGetProgramiv(...)                    LOOKUP_GL_FUNCTION(glGetProgramiv, __VA_ARGS__)
#define glGetRenderbufferParameterivEXT        glGetRenderbufferParameteriv
#define glGetRenderbufferParameteriv(...)      LOOKUP_GL_FUNCTION(glGetRenderbufferParameteriv, __VA_ARGS__)
#define glGetShaderInfoLog(...)                LOOKUP_GL_FUNCTION(glGetShaderInfoLog, __VA_ARGS__)
#define glGetShaderiv(...)                     LOOKUP_GL_FUNCTION(glGetShaderiv, __VA_ARGS__)
#define glGetShaderSource(...)                 LOOKUP_GL_FUNCTION(glGetShaderSource, __VA_ARGS__)
#define glGetUniformfv(...)                    LOOKUP_GL_FUNCTION(glGetUniformfv, __VA_ARGS__)
#define glGetUniformiv(...)                    LOOKUP_GL_FUNCTION(glGetUniformiv, __VA_ARGS__)
#define glGetUniformLocation(...)              LOOKUP_GL_FUNCTION(glGetUniformLocation, __VA_ARGS__)
#define glGetVertexAttribfv(...)               LOOKUP_GL_FUNCTION(glGetVertexAttribfv, __VA_ARGS__)
#define glGetVertexAttribiv(...)               LOOKUP_GL_FUNCTION(glGetVertexAttribiv, __VA_ARGS__)
#define glGetVertexAttribPointerv(...)         LOOKUP_GL_FUNCTION(glGetVertexAttribPointerv, __VA_ARGS__)
#define glIsBuffer(...)                        LOOKUP_GL_FUNCTION(glIsBuffer, __VA_ARGS__)
#define glIsFramebufferEXT                     glIsFramebuffer
#define glIsFramebuffer(...)                   LOOKUP_GL_FUNCTION(glIsFramebuffer, __VA_ARGS__)
#define glIsProgram(...)                       LOOKUP_GL_FUNCTION(glIsProgram, __VA_ARGS__)
#define glIsRenderbufferEXT                    glIsRenderbuffer
#define glIsRenderbuffer(...)                  LOOKUP_GL_FUNCTION(glIsRenderbuffer, __VA_ARGS__)
#define glIsShader(...)                        LOOKUP_GL_FUNCTION(glIsShader, __VA_ARGS__)
#define glLinkProgram(...)                     LOOKUP_GL_FUNCTION(glLinkProgram, __VA_ARGS__)
#define glRenderbufferStorageEXT               glRenderbufferStorage
#define glRenderbufferStorage(...)             LOOKUP_GL_FUNCTION(glRenderbufferStorage, __VA_ARGS__)
#define glRenderbufferStorageMultisampleEXT    glRenderbufferStorageMultisample
#define glRenderbufferStorageMultisample(...)  LOOKUP_GL_FUNCTION(glRenderbufferStorageMultisample, __VA_ARGS__)
#define glSampleCoverage(...)                  LOOKUP_GL_FUNCTION(glSampleCoverage, __VA_ARGS__)
#define glShaderSource(...)                    LOOKUP_GL_FUNCTION(glShaderSource, __VA_ARGS__)
#define glStencilFuncSeparate(...)             LOOKUP_GL_FUNCTION(glStencilFuncSeparate, __VA_ARGS__)
#define glStencilMaskSeparate(...)             LOOKUP_GL_FUNCTION(glStencilMaskSeparate, __VA_ARGS__)
#define glStencilOpSeparate(...)               LOOKUP_GL_FUNCTION(glStencilOpSeparate, __VA_ARGS__)
#define glUniform1f(...)                       LOOKUP_GL_FUNCTION(glUniform1f, __VA_ARGS__)
#define glUniform1fv(...)                      LOOKUP_GL_FUNCTION(glUniform1fv, __VA_ARGS__)
#define glUniform1i(...)                       LOOKUP_GL_FUNCTION(glUniform1i, __VA_ARGS__)
#define glUniform1iv(...)                      LOOKUP_GL_FUNCTION(glUniform1iv, __VA_ARGS__)
#define glUniform2f(...)                       LOOKUP_GL_FUNCTION(glUniform2f, __VA_ARGS__)
#define glUniform2fv(...)                      LOOKUP_GL_FUNCTION(glUniform2fv, __VA_ARGS__)
#define glUniform2i(...)                       LOOKUP_GL_FUNCTION(glUniform2i, __VA_ARGS__)
#define glUniform2iv(...)                      LOOKUP_GL_FUNCTION(glUniform2iv, __VA_ARGS__)
#define glUniform3f(...)                       LOOKUP_GL_FUNCTION(glUniform3f, __VA_ARGS__)
#define glUniform3fv(...)                      LOOKUP_GL_FUNCTION(glUniform3fv, __VA_ARGS__)
#define glUniform3i(...)                       LOOKUP_GL_FUNCTION(glUniform3i, __VA_ARGS__)
#define glUniform3iv(...)                      LOOKUP_GL_FUNCTION(glUniform3iv, __VA_ARGS__)
#define glUniform4f(...)                       LOOKUP_GL_FUNCTION(glUniform4f, __VA_ARGS__)
#define glUniform4fv(...)                      LOOKUP_GL_FUNCTION(glUniform4fv, __VA_ARGS__)
#define glUniform4i(...)                       LOOKUP_GL_FUNCTION(glUniform4i, __VA_ARGS__)
#define glUniform4iv(...)                      LOOKUP_GL_FUNCTION(glUniform4iv, __VA_ARGS__)
#define glUniformMatrix2fv(...)                LOOKUP_GL_FUNCTION(glUniformMatrix2fv, __VA_ARGS__)
#define glUniformMatrix3fv(...)                LOOKUP_GL_FUNCTION(glUniformMatrix3fv, __VA_ARGS__)
#define glUniformMatrix4fv(...)                LOOKUP_GL_FUNCTION(glUniformMatrix4fv, __VA_ARGS__)
#define glUseProgram(...)                      LOOKUP_GL_FUNCTION(glUseProgram, __VA_ARGS__)
#define glValidateProgram(...)                 LOOKUP_GL_FUNCTION(glValidateProgram, __VA_ARGS__)
#define glVertexAttrib1f(...)                  LOOKUP_GL_FUNCTION(glVertexAttrib1f, __VA_ARGS__)
#define glVertexAttrib1fv(...)                 LOOKUP_GL_FUNCTION(glVertexAttrib1fv, __VA_ARGS__)
#define glVertexAttrib2f(...)                  LOOKUP_GL_FUNCTION(glVertexAttrib2f, __VA_ARGS__)
#define glVertexAttrib2fv(...)                 LOOKUP_GL_FUNCTION(glVertexAttrib2fv, __VA_ARGS__)
#define glVertexAttrib3f(...)                  LOOKUP_GL_FUNCTION(glVertexAttrib3f, __VA_ARGS__)
#define glVertexAttrib3fv(...)                 LOOKUP_GL_FUNCTION(glVertexAttrib3fv, __VA_ARGS__)
#define glVertexAttrib4f(...)                  LOOKUP_GL_FUNCTION(glVertexAttrib4f, __VA_ARGS__)
#define glVertexAttrib4fv(...)                 LOOKUP_GL_FUNCTION(glVertexAttrib4fv, __VA_ARGS__)
#define glVertexAttribDivisorEXT               glVertexAttribDivisor
#define glVertexAttribDivisor(...)             LOOKUP_GL_FUNCTION(glVertexAttribDivisor, __VA_ARGS__)
#define glVertexAttribPointer(...)             LOOKUP_GL_FUNCTION(glVertexAttribPointer, __VA_ARGS__)
