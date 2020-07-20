/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include "GraphicsTypesGL.h"

#include <wtf/text/WTFString.h>

namespace WebCore {

// This is a base class containing only pure virtual functions.
// Implementations must provide a subclass.
//
// The supported extensions are defined below and in subclasses,
// possibly platform-specific ones.
//
// Calling any extension function not supported by the current context
// must be a no-op; in particular, it may not have side effects. In
// this situation, if the function has a return value, 0 is returned.
class ExtensionsGL {
public:
    virtual ~ExtensionsGL() = default;

    // Supported extensions:
    //   GL_EXT_texture_format_BGRA8888
    //   GL_EXT_read_format_bgra
    //   GL_ARB_robustness
    //   GL_ARB_texture_non_power_of_two / GL_OES_texture_npot
    //   GL_EXT_packed_depth_stencil / GL_OES_packed_depth_stencil
    //   GL_ANGLE_framebuffer_blit / GL_ANGLE_framebuffer_multisample
    //   GL_IMG_multisampled_render_to_texture
    //   GL_OES_texture_float
    //   GL_OES_texture_float_linear
    //   GL_OES_texture_half_float
    //   GL_OES_texture_half_float_linear
    //   GL_OES_standard_derivatives
    //   GL_OES_rgb8_rgba8
    //   GL_OES_vertex_array_object
    //   GL_OES_element_index_uint
    //   GL_ANGLE_translated_shader_source
    //   GL_ARB_texture_rectangle (only the subset required to
    //     implement IOSurface binding; it's recommended to support
    //     this only on Mac OS X to limit the amount of code dependent
    //     on this extension)
    //   GL_EXT_texture_compression_dxt1
    //   GL_EXT_texture_compression_s3tc
    //   GL_OES_compressed_ETC1_RGB8_texture
    //   GL_IMG_texture_compression_pvrtc
    //   GL_KHR_texture_compression_astc_hdr
    //   GL_KHR_texture_compression_astc_ldr
    //   EXT_texture_filter_anisotropic
    //   GL_EXT_debug_marker
    //   GL_ARB_draw_buffers / GL_EXT_draw_buffers
    //   GL_ANGLE_instanced_arrays
    //   GL_ANGLE_robust_client_memory

    // Takes full name of extension; for example,
    // "GL_EXT_texture_format_BGRA8888".
    virtual bool supports(const String&) = 0;

    // Certain OpenGL and WebGL implementations may support enabling
    // extensions lazily. This method may only be called with
    // extension names for which supports returns true.
    virtual void ensureEnabled(const String&) = 0;

    // Takes full name of extension: for example, "GL_EXT_texture_format_BGRA8888".
    // Checks to see whether the given extension is actually enabled (see ensureEnabled).
    // Has no other side-effects.
    virtual bool isEnabled(const String&) = 0;

    enum ExtensionsEnumType {
        // EXT_sRGB formats
        SRGB_EXT = 0x8C40,
        SRGB_ALPHA_EXT = 0x8C42,
        SRGB8_ALPHA8_EXT = 0x8C43,
        FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING_EXT = 0x8210,

        // EXT_blend_minmax enums
        MIN_EXT = 0x8007,
        MAX_EXT = 0x8008,

        // GL_EXT_texture_format_BGRA8888 enums
        BGRA_EXT = 0x80E1,

        // GL_ARB_robustness enums
        GUILTY_CONTEXT_RESET_ARB = 0x8253,
        INNOCENT_CONTEXT_RESET_ARB = 0x8254,
        UNKNOWN_CONTEXT_RESET_ARB = 0x8255,
        CONTEXT_ROBUST_ACCESS = 0x90F3,

        // GL_EXT/OES_packed_depth_stencil enums
        DEPTH24_STENCIL8 = 0x88F0,

        // GL_ANGLE_framebuffer_blit names
        READ_FRAMEBUFFER = 0x8CA8,
        DRAW_FRAMEBUFFER = 0x8CA9,
        DRAW_FRAMEBUFFER_BINDING = 0x8CA6,
        READ_FRAMEBUFFER_BINDING = 0x8CAA,

        // GL_ANGLE_framebuffer_multisample names
        RENDERBUFFER_SAMPLES = 0x8CAB,
        FRAMEBUFFER_INCOMPLETE_MULTISAMPLE = 0x8D56,
        MAX_SAMPLES = 0x8D57,

        // GL_IMG_multisampled_render_to_texture
        RENDERBUFFER_SAMPLES_IMG = 0x9133,
        FRAMEBUFFER_INCOMPLETE_MULTISAMPLE_IMG = 0x9134,
        MAX_SAMPLES_IMG = 0x9135,
        TEXTURE_SAMPLES_IMG = 0x9136,

        // GL_OES_standard_derivatives names
        FRAGMENT_SHADER_DERIVATIVE_HINT_OES = 0x8B8B,

        // GL_OES_rgb8_rgba8 names
        RGB8_OES = 0x8051,
        RGBA8_OES = 0x8058,

        // GL_OES_vertex_array_object names
        VERTEX_ARRAY_BINDING_OES = 0x85B5,

        // GL_ANGLE_translated_shader_source
        TRANSLATED_SHADER_SOURCE_LENGTH_ANGLE = 0x93A0,

        // GL_ARB_texture_rectangle
        TEXTURE_RECTANGLE_ARB =  0x84F5,
        TEXTURE_BINDING_RECTANGLE_ARB = 0x84F6,

        // GL_EXT_texture_compression_dxt1
        // GL_EXT_texture_compression_s3tc
        COMPRESSED_RGB_S3TC_DXT1_EXT = 0x83F0,
        COMPRESSED_RGBA_S3TC_DXT1_EXT = 0x83F1,
        COMPRESSED_RGBA_S3TC_DXT3_EXT = 0x83F2,
        COMPRESSED_RGBA_S3TC_DXT5_EXT = 0x83F3,

        // GL_OES_compressed_ETC1_RGB8_texture
        ETC1_RGB8_OES = 0x8D64,

        // WEBGL_compressed_texture_etc
        COMPRESSED_R11_EAC = 0x9270,
        COMPRESSED_SIGNED_R11_EAC = 0x9271,
        COMPRESSED_RG11_EAC = 0x9272,
        COMPRESSED_SIGNED_RG11_EAC = 0x9273,
        COMPRESSED_RGB8_ETC2 = 0x9274,
        COMPRESSED_SRGB8_ETC2 = 0x9275,
        COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2 = 0x9276,
        COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2 = 0x9277,
        COMPRESSED_RGBA8_ETC2_EAC = 0x9278,
        COMPRESSED_SRGB8_ALPHA8_ETC2_EAC = 0x9279,

        // GL_IMG_texture_compression_pvrtc
        COMPRESSED_RGB_PVRTC_4BPPV1_IMG = 0x8C00,
        COMPRESSED_RGB_PVRTC_2BPPV1_IMG = 0x8C01,
        COMPRESSED_RGBA_PVRTC_4BPPV1_IMG = 0x8C02,
        COMPRESSED_RGBA_PVRTC_2BPPV1_IMG = 0x8C03,

        // GL_AMD_compressed_ATC_texture
        COMPRESSED_ATC_RGB_AMD = 0x8C92,
        COMPRESSED_ATC_RGBA_EXPLICIT_ALPHA_AMD = 0x8C93,
        COMPRESSED_ATC_RGBA_INTERPOLATED_ALPHA_AMD = 0x87EE,

        // GL_KHR_texture_compression_astc_hdr
        COMPRESSED_RGBA_ASTC_4x4_KHR = 0x93B0,
        COMPRESSED_RGBA_ASTC_5x4_KHR = 0x93B1,
        COMPRESSED_RGBA_ASTC_5x5_KHR = 0x93B2,
        COMPRESSED_RGBA_ASTC_6x5_KHR = 0x93B3,
        COMPRESSED_RGBA_ASTC_6x6_KHR = 0x93B4,
        COMPRESSED_RGBA_ASTC_8x5_KHR = 0x93B5,
        COMPRESSED_RGBA_ASTC_8x6_KHR = 0x93B6,
        COMPRESSED_RGBA_ASTC_8x8_KHR = 0x93B7,
        COMPRESSED_RGBA_ASTC_10x5_KHR = 0x93B8,
        COMPRESSED_RGBA_ASTC_10x6_KHR = 0x93B9,
        COMPRESSED_RGBA_ASTC_10x8_KHR = 0x93BA,
        COMPRESSED_RGBA_ASTC_10x10_KHR = 0x93BB,
        COMPRESSED_RGBA_ASTC_12x10_KHR = 0x93BC,
        COMPRESSED_RGBA_ASTC_12x12_KHR = 0x93BD,

        COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR = 0x93D0,
        COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR = 0x93D1,
        COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR = 0x93D2,
        COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR = 0x93D3,
        COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR = 0x93D4,
        COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR = 0x93D5,
        COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR = 0x93D6,
        COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR = 0x93D7,
        COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR = 0x93D8,
        COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR = 0x93D9,
        COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR = 0x93DA,
        COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR = 0x93DB,
        COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR = 0x93DC,
        COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR = 0x93DD,

        // GL_EXT_texture_filter_anisotropic
        TEXTURE_MAX_ANISOTROPY_EXT = 0x84FE,
        MAX_TEXTURE_MAX_ANISOTROPY_EXT = 0x84FF,

        // GL_ARB_draw_buffers / GL_EXT_draw_buffers
        MAX_DRAW_BUFFERS_EXT = 0x8824,
        DRAW_BUFFER0_EXT = 0x8825,
        DRAW_BUFFER1_EXT = 0x8826,
        DRAW_BUFFER2_EXT = 0x8827,
        DRAW_BUFFER3_EXT = 0x8828,
        DRAW_BUFFER4_EXT = 0x8829,
        DRAW_BUFFER5_EXT = 0x882A,
        DRAW_BUFFER6_EXT = 0x882B,
        DRAW_BUFFER7_EXT = 0x882C,
        DRAW_BUFFER8_EXT = 0x882D,
        DRAW_BUFFER9_EXT = 0x882E,
        DRAW_BUFFER10_EXT = 0x882F,
        DRAW_BUFFER11_EXT = 0x8830,
        DRAW_BUFFER12_EXT = 0x8831,
        DRAW_BUFFER13_EXT = 0x8832,
        DRAW_BUFFER14_EXT = 0x8833,
        DRAW_BUFFER15_EXT = 0x8834,
        MAX_COLOR_ATTACHMENTS_EXT = 0x8CDF,
        COLOR_ATTACHMENT0_EXT = 0x8CE0,
        COLOR_ATTACHMENT1_EXT = 0x8CE1,
        COLOR_ATTACHMENT2_EXT = 0x8CE2,
        COLOR_ATTACHMENT3_EXT = 0x8CE3,
        COLOR_ATTACHMENT4_EXT = 0x8CE4,
        COLOR_ATTACHMENT5_EXT = 0x8CE5,
        COLOR_ATTACHMENT6_EXT = 0x8CE6,
        COLOR_ATTACHMENT7_EXT = 0x8CE7,
        COLOR_ATTACHMENT8_EXT = 0x8CE8,
        COLOR_ATTACHMENT9_EXT = 0x8CE9,
        COLOR_ATTACHMENT10_EXT = 0x8CEA,
        COLOR_ATTACHMENT11_EXT = 0x8CEB,
        COLOR_ATTACHMENT12_EXT = 0x8CEC,
        COLOR_ATTACHMENT13_EXT = 0x8CED,
        COLOR_ATTACHMENT14_EXT = 0x8CEE,
        COLOR_ATTACHMENT15_EXT = 0x8CEF
    };

    // GL_ARB_robustness
    // Note: This method's behavior differs from the GL_ARB_robustness
    // specification in the following way:
    // The implementation must not reset the error state during this call.
    // If getGraphicsResetStatusARB returns an error, it should continue
    // returning the same error. Restoring the GraphicsContextGLOpenGL is handled
    // externally.
    virtual int getGraphicsResetStatusARB() = 0;

    // GL_ANGLE_framebuffer_blit
    virtual void blitFramebuffer(long srcX0, long srcY0, long srcX1, long srcY1, long dstX0, long dstY0, long dstX1, long dstY1, unsigned long mask, unsigned long filter) = 0;

    // GL_ANGLE_framebuffer_multisample
    virtual void renderbufferStorageMultisample(unsigned long target, unsigned long samples, unsigned long internalformat, unsigned long width, unsigned long height) = 0;

    // GL_OES_vertex_array_object
    virtual PlatformGLObject createVertexArrayOES() = 0;
    virtual void deleteVertexArrayOES(PlatformGLObject) = 0;
    virtual GCGLboolean isVertexArrayOES(PlatformGLObject) = 0;
    virtual void bindVertexArrayOES(PlatformGLObject) = 0;

    // GL_ANGLE_translated_shader_source
    virtual String getTranslatedShaderSourceANGLE(PlatformGLObject) = 0;

    // EXT Robustness - uses getGraphicsResetStatusARB
    virtual void readnPixelsEXT(int x, int y, GCGLsizei width, GCGLsizei height, GCGLenum format, GCGLenum type, GCGLsizei bufSize, void *data) = 0;
    virtual void getnUniformfvEXT(GCGLuint program, int location, GCGLsizei bufSize, float *params) = 0;
    virtual void getnUniformivEXT(GCGLuint program, int location, GCGLsizei bufSize, int *params) = 0;

    // GL_EXT_debug_marker
    virtual void insertEventMarkerEXT(const String&) = 0;
    virtual void pushGroupMarkerEXT(const String&) = 0;
    virtual void popGroupMarkerEXT(void) = 0;

    // GL_ARB_draw_buffers / GL_EXT_draw_buffers
    virtual void drawBuffersEXT(GCGLsizei n, const GCGLenum* bufs) = 0;

    // GL_ANGLE_instanced_arrays
    virtual void drawArraysInstanced(GCGLenum mode, GCGLint first, GCGLsizei count, GCGLsizei primcount) = 0;
    virtual void drawElementsInstanced(GCGLenum mode, GCGLsizei count, GCGLenum type, long long offset, GCGLsizei primcount) = 0;
    virtual void vertexAttribDivisor(GCGLuint index, GCGLuint divisor) = 0;

    virtual bool isNVIDIA() = 0;
    virtual bool isAMD() = 0;
    virtual bool isIntel() = 0;
    virtual bool isImagination() = 0;
    virtual String vendor() = 0;

    // Some configurations have bugs regarding built-in functions in their OpenGL drivers
    // that must be avoided. Ports should implement these flags on such configurations.
    virtual bool requiresBuiltInFunctionEmulation() = 0;
    virtual bool requiresRestrictedMaximumTextureSize() = 0;

    // GL_ANGLE_robust_client_memory
    virtual void getBooleanvRobustANGLE(GCGLenum pname, GCGLsizei bufSize, GCGLsizei *length, GCGLboolean *data) = 0;
    virtual void getBufferParameterivRobustANGLE(GCGLenum target, GCGLenum pname, GCGLsizei bufSize, GCGLsizei *length, GCGLint *params) = 0;
    virtual void getFloatvRobustANGLE(GCGLenum pname, GCGLsizei bufSize, GCGLsizei *length, GCGLfloat *data) = 0;
    virtual void getFramebufferAttachmentParameterivRobustANGLE(GCGLenum target, GCGLenum attachment, GCGLenum pname, GCGLsizei bufSize, GCGLsizei *length, GCGLint *params) = 0;
    virtual void getIntegervRobustANGLE(GCGLenum pname, GCGLsizei bufSize, GCGLsizei *length, GCGLint *data) = 0;
    virtual void getProgramivRobustANGLE(GCGLuint program, GCGLenum pname, GCGLsizei bufSize, GCGLsizei *length, GCGLint *params) = 0;
    virtual void getRenderbufferParameterivRobustANGLE(GCGLenum target, GCGLenum pname, GCGLsizei bufSize, GCGLsizei *length, GCGLint *params) = 0;
    virtual void getShaderivRobustANGLE(GCGLuint shader, GCGLenum pname, GCGLsizei bufSize, GCGLsizei *length, GCGLint *params) = 0;
    virtual void getTexParameterfvRobustANGLE(GCGLenum target, GCGLenum pname, GCGLsizei bufSize, GCGLsizei *length, GCGLfloat *params) = 0;
    virtual void getTexParameterivRobustANGLE(GCGLenum target, GCGLenum pname, GCGLsizei bufSize, GCGLsizei *length, GCGLint *params) = 0;
    virtual void getUniformfvRobustANGLE(GCGLuint program, int location, GCGLsizei bufSize, GCGLsizei *length, GCGLfloat *params) = 0;
    virtual void getUniformivRobustANGLE(GCGLuint program, int location, GCGLsizei bufSize, GCGLsizei *length, GCGLint *params) = 0;
    virtual void getVertexAttribfvRobustANGLE(GCGLuint index, GCGLenum pname, GCGLsizei bufSize, GCGLsizei *length, GCGLfloat *params) = 0;
    virtual void getVertexAttribivRobustANGLE(GCGLuint index, GCGLenum pname, GCGLsizei bufSize, GCGLsizei *length, GCGLint *params) = 0;
    virtual void getVertexAttribPointervRobustANGLE(GCGLuint index, GCGLenum pname, GCGLsizei bufSize, GCGLsizei *length, void **pointer) = 0;
    virtual void readPixelsRobustANGLE(int x, int y, GCGLsizei width, GCGLsizei height, GCGLenum format, GCGLenum type, GCGLsizei bufSize, GCGLsizei *length, GCGLsizei *columns, GCGLsizei *rows, void *pixels) = 0;
    virtual void texImage2DRobustANGLE(GCGLenum target, int level, int internalformat, GCGLsizei width, GCGLsizei height, int border, GCGLenum format, GCGLenum type, GCGLsizei bufSize, const void *pixels) = 0;
    virtual void texParameterfvRobustANGLE(GCGLenum target, GCGLenum pname, GCGLsizei bufSize, const GCGLfloat *params) = 0;
    virtual void texParameterivRobustANGLE(GCGLenum target, GCGLenum pname, GCGLsizei bufSize, const GCGLint *params) = 0;
    virtual void texSubImage2DRobustANGLE(GCGLenum target, int level, int xoffset, int yoffset, GCGLsizei width, GCGLsizei height, GCGLenum format, GCGLenum type, GCGLsizei bufSize, const void *pixels) = 0;
    virtual void compressedTexImage2DRobustANGLE(GCGLenum target, int level, GCGLenum internalformat, GCGLsizei width, GCGLsizei height, int border, GCGLsizei imageSize, GCGLsizei bufSize, const void* data) = 0;
    virtual void compressedTexSubImage2DRobustANGLE(GCGLenum target, int level, int xoffset, int yoffset, GCGLsizei width, GCGLsizei height, GCGLenum format, GCGLsizei imageSize, GCGLsizei bufSize, const void* data) = 0;
    virtual void compressedTexImage3DRobustANGLE(GCGLenum target, int level, GCGLenum internalformat, GCGLsizei width, GCGLsizei height, GCGLsizei depth, int border, GCGLsizei imageSize, GCGLsizei bufSize, const void* data) = 0;
    virtual void compressedTexSubImage3DRobustANGLE(GCGLenum target, int level, int xoffset, int yoffset, int zoffset, GCGLsizei width, GCGLsizei height, GCGLsizei depth, GCGLenum format, GCGLsizei imageSize, GCGLsizei bufSize, const void* data) = 0;

    virtual void texImage3DRobustANGLE(GCGLenum target, int level, int internalformat, GCGLsizei width, GCGLsizei height, GCGLsizei depth, int border, GCGLenum format, GCGLenum type, GCGLsizei bufSize, const void *pixels) = 0;
    virtual void texSubImage3DRobustANGLE(GCGLenum target, int level, int xoffset, int yoffset, int zoffset, GCGLsizei width, GCGLsizei height, GCGLsizei depth, GCGLenum format, GCGLenum type, GCGLsizei bufSize, const void *pixels) = 0;
    virtual void getQueryivRobustANGLE(GCGLenum target, GCGLenum pname, GCGLsizei bufSize, GCGLsizei *length, GCGLint *params) = 0;
    virtual void getQueryObjectuivRobustANGLE(GCGLuint id, GCGLenum pname, GCGLsizei bufSize, GCGLsizei *length, GCGLuint *params) = 0;
    virtual void getBufferPointervRobustANGLE(GCGLenum target, GCGLenum pname, GCGLsizei bufSize, GCGLsizei *length, void **params) = 0;
    virtual void getIntegeri_vRobustANGLE(GCGLenum target, GCGLuint index, GCGLsizei bufSize, GCGLsizei *length, int *data) = 0;
    virtual void getInternalformativRobustANGLE(GCGLenum target, GCGLenum internalformat, GCGLenum pname, GCGLsizei bufSize, GCGLsizei *length, GCGLint *params) = 0;
    virtual void getVertexAttribIivRobustANGLE(GCGLuint index, GCGLenum pname, GCGLsizei bufSize, GCGLsizei *length, GCGLint *params) = 0;
    virtual void getVertexAttribIuivRobustANGLE(GCGLuint index, GCGLenum pname, GCGLsizei bufSize, GCGLsizei *length, GCGLuint *params) = 0;
    virtual void getUniformuivRobustANGLE(GCGLuint program, int location, GCGLsizei bufSize, GCGLsizei *length, GCGLuint *params) = 0;
    virtual void getActiveUniformBlockivRobustANGLE(GCGLuint program, GCGLuint uniformBlockIndex, GCGLenum pname, GCGLsizei bufSize, GCGLsizei *length, GCGLint *params) = 0;
    virtual void getInteger64vRobustANGLE(GCGLenum pname, GCGLsizei bufSize, GCGLsizei *length, GCGLint64 *data) = 0;
    virtual void getInteger64i_vRobustANGLE(GCGLenum target, GCGLuint index, GCGLsizei bufSize, GCGLsizei *length, GCGLint64 *data) = 0;
    virtual void getBufferParameteri64vRobustANGLE(GCGLenum target, GCGLenum pname, GCGLsizei bufSize, GCGLsizei *length, GCGLint64 *params) = 0;
    virtual void samplerParameterivRobustANGLE(GCGLuint sampler, GCGLenum pname, GCGLsizei bufSize, const GCGLint *param) = 0;
    virtual void samplerParameterfvRobustANGLE(GCGLuint sampler, GCGLenum pname, GCGLsizei bufSize, const GCGLfloat *param) = 0;
    virtual void getSamplerParameterivRobustANGLE(GCGLuint sampler, GCGLenum pname, GCGLsizei bufSize, GCGLsizei *length, GCGLint *params) = 0;
    virtual void getSamplerParameterfvRobustANGLE(GCGLuint sampler, GCGLenum pname, GCGLsizei bufSize, GCGLsizei *length, GCGLfloat *params) = 0;

    virtual void getFramebufferParameterivRobustANGLE(GCGLenum target, GCGLenum pname, GCGLsizei bufSize, GCGLsizei *length, GCGLint *params) = 0;
    virtual void getProgramInterfaceivRobustANGLE(GCGLuint program, GCGLenum programInterface, GCGLenum pname, GCGLsizei bufSize, GCGLsizei *length, GCGLint *params) = 0;
    virtual void getBooleani_vRobustANGLE(GCGLenum target, GCGLuint index, GCGLsizei bufSize, GCGLsizei *length, GCGLboolean *data) = 0;
    virtual void getMultisamplefvRobustANGLE(GCGLenum pname, GCGLuint index, GCGLsizei bufSize, GCGLsizei *length, GCGLfloat *val) = 0;
    virtual void getTexLevelParameterivRobustANGLE(GCGLenum target, int level, GCGLenum pname, GCGLsizei bufSize, GCGLsizei *length, GCGLint *params) = 0;
    virtual void getTexLevelParameterfvRobustANGLE(GCGLenum target, int level, GCGLenum pname, GCGLsizei bufSize, GCGLsizei *length, GCGLfloat *params) = 0;

    virtual void getPointervRobustANGLERobustANGLE(GCGLenum pname, GCGLsizei bufSize, GCGLsizei *length, void **params) = 0;
    virtual void readnPixelsRobustANGLE(int x, int y, GCGLsizei width, GCGLsizei height, GCGLenum format, GCGLenum type, GCGLsizei bufSize, GCGLsizei *length, GCGLsizei *columns, GCGLsizei *rows, void *data, bool readingToPixelBufferObject) = 0;
    virtual void getnUniformfvRobustANGLE(GCGLuint program, int location, GCGLsizei bufSize, GCGLsizei *length, GCGLfloat *params) = 0;
    virtual void getnUniformivRobustANGLE(GCGLuint program, int location, GCGLsizei bufSize, GCGLsizei *length, GCGLint *params) = 0;
    virtual void getnUniformuivRobustANGLE(GCGLuint program, int location, GCGLsizei bufSize, GCGLsizei *length, GCGLuint *params) = 0;
    virtual void texParameterIivRobustANGLE(GCGLenum target, GCGLenum pname, GCGLsizei bufSize, const GCGLint *params) = 0;
    virtual void texParameterIuivRobustANGLE(GCGLenum target, GCGLenum pname, GCGLsizei bufSize, const GCGLuint *params) = 0;
    virtual void getTexParameterIivRobustANGLE(GCGLenum target, GCGLenum pname, GCGLsizei bufSize, GCGLsizei *length, GCGLint *params) = 0;
    virtual void getTexParameterIuivRobustANGLE(GCGLenum target, GCGLenum pname, GCGLsizei bufSize, GCGLsizei *length, GCGLuint *params) = 0;
    virtual void samplerParameterIivRobustANGLE(GCGLuint sampler, GCGLenum pname, GCGLsizei bufSize, const GCGLint *param) = 0;
    virtual void samplerParameterIuivRobustANGLE(GCGLuint sampler, GCGLenum pname, GCGLsizei bufSize, const GCGLuint *param) = 0;
    virtual void getSamplerParameterIivRobustANGLE(GCGLuint sampler, GCGLenum pname, GCGLsizei bufSize, GCGLsizei *length, GCGLint *params) = 0;
    virtual void getSamplerParameterIuivRobustANGLE(GCGLuint sampler, GCGLenum pname, GCGLsizei bufSize, GCGLsizei *length, GCGLuint *params) = 0;

    virtual void getQueryObjectivRobustANGLE(GCGLuint id, GCGLenum pname, GCGLsizei bufSize, GCGLsizei *length, GCGLint *params) = 0;
    virtual void getQueryObjecti64vRobustANGLE(GCGLuint id, GCGLenum pname, GCGLsizei bufSize, GCGLsizei *length, GCGLint64 *params) = 0;
    virtual void getQueryObjectui64vRobustANGLE(GCGLuint id, GCGLenum pname, GCGLsizei bufSize, GCGLsizei *length, GCGLuint64 *params) = 0;
};

} // namespace WebCore
