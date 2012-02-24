/*
 Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Library General Public
 License as published by the Free Software Foundation; either
 version 2 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Library General Public License for more details.

 You should have received a copy of the GNU Library General Public License
 along with this library; see the file COPYING.LIB.  If not, write to
 the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "TextureMapperGL.h"

#include "GraphicsContext.h"
#include "Image.h"
#include "TextureMapperShaderManager.h"
#include "Timer.h"
#include <wtf/HashMap.h>
#include <wtf/OwnArrayPtr.h>
#include <wtf/PassOwnArrayPtr.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>

#if PLATFORM(QT)
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <QPlatformPixmap>
#endif
#endif

#if defined(TEXMAP_OPENGL_ES_2)
#include <EGL/egl.h>
#elif OS(WINDOWS)
#include <windows.h>
#elif OS(MAC_OS_X)
#include <AGL/agl.h>
#elif defined(XP_UNIX)
#include <GL/glx.h>
#endif

#if USE(CAIRO)
#include "CairoUtilities.h"
#include "RefPtrCairo.h"
#include <cairo.h>
#include <wtf/ByteArray.h>
#endif

namespace WebCore {

inline static void debugGLCommand(const char* command, int line)
{
    const GLenum err = glGetError();
    if (!err)
        return;
    WTFReportError(__FILE__, line, WTF_PRETTY_FUNCTION, "[TextureMapper GL] Command failed: %s (%x)\n", command, err);
    ASSERT_NOT_REACHED();
}

#ifndef NDEBUG
#define GL_CMD(x) {x, debugGLCommand(#x, __LINE__); }
#else
#define GL_CMD(x) x;
#endif

struct TextureMapperGLData {
    struct SharedGLData : public RefCounted<SharedGLData> {
#if defined(TEXMAP_OPENGL_ES_2)
        typedef EGLContext GLContext;
        static GLContext getCurrentGLContext()
        {
            return eglGetCurrentContext();
        }
#elif OS(WINDOWS)
        typedef HGLRC GLContext;
        static GLContext getCurrentGLContext()
        {
            return wglGetCurrentContext();
        }
#elif OS(MAC_OS_X)
        typedef AGLContext GLContext;
        static GLContext getCurrentGLContext()
        {
            return aglGetCurrentContext();
        }
#elif defined(XP_UNIX)
        typedef GLXContext GLContext;
        static GLContext getCurrentGLContext()
        {
            return glXGetCurrentContext();
        }
#else
        // Default implementation for unknown opengl.
        // Returns always increasing number and disables GL context data sharing.
        typedef unsigned int GLContext;
        static GLContext getCurrentGLContext()
        {
            static GLContext dummyContextCounter = 0;
            return ++dummyContextCounter;
        }

#endif

        typedef HashMap<GLContext, SharedGLData*> GLContextDataMap;
        static GLContextDataMap& glContextDataMap()
        {
            static GLContextDataMap map;
            return map;
        }

        static PassRefPtr<SharedGLData> currentSharedGLData()
        {
            GLContext currentGLConext = getCurrentGLContext();
            GLContextDataMap::iterator it = glContextDataMap().find(currentGLConext);
            if (it != glContextDataMap().end())
                return it->second;

            return adoptRef(new SharedGLData(getCurrentGLContext()));
        }

        struct ClipState {
            IntRect scissorBox;
            int stencilIndex;
            ClipState(const IntRect& scissors, int stencil)
                : scissorBox(scissors)
                , stencilIndex(stencil)
            { }

            ClipState()
                : stencilIndex(1)
            { }
        };

        ClipState clipState;
        Vector<ClipState> clipStack;

        void pushClipState()
        {
            clipStack.append(clipState);
        }

        void popClipState()
        {
            if (clipStack.isEmpty())
                return;
            clipState = clipStack.last();
            clipStack.removeLast();
        }

        static void scissorClip(const IntRect& rect)
        {
            if (rect.isEmpty())
                return;

            GLint viewport[4];
            GL_CMD(glGetIntegerv(GL_VIEWPORT, viewport))
            GL_CMD(glScissor(rect.x(), viewport[3] - rect.maxY() + 1, rect.width() - 1, rect.height() - 1))
        }

        void applyCurrentClip()
        {
            scissorClip(clipState.scissorBox);
            GL_CMD(glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP))
            glStencilFunc(GL_EQUAL, clipState.stencilIndex - 1, clipState.stencilIndex - 1);
            if (clipState.stencilIndex == 1)
                glDisable(GL_STENCIL_TEST);
            else
                glEnable(GL_STENCIL_TEST);
        }

        TextureMapperShaderManager textureMapperShaderManager;

        SharedGLData(GLContext glContext)
        {
            glContextDataMap().add(glContext, this);
        }

        ~SharedGLData()
        {
            GLContextDataMap::const_iterator end = glContextDataMap().end();
            GLContextDataMap::iterator it;
            for (it = glContextDataMap().begin(); it != end; ++it) {
                if (it->second == this)
                    break;
            }

            ASSERT(it != end);
            glContextDataMap().remove(it);
        }

    };

    SharedGLData& sharedGLData() const
    {
        return *(m_sharedGLData.get());
    }

    void initializeStencil();

    TextureMapperGLData()
        : previousProgram(0)
        , didModifyStencil(false)
        , previousScissorState(0)
        , previousDepthState(0)
        , m_sharedGLData(TextureMapperGLData::SharedGLData::currentSharedGLData())
    { }

    TransformationMatrix projectionMatrix;
    GLint previousProgram;
    bool didModifyStencil;
    GLint previousScissorState;
    GLint previousDepthState;
    GLint viewport[4];
    GLint previousScissor[4];
    RefPtr<SharedGLData> m_sharedGLData;
    RefPtr<BitmapTexture> currentSurface;
};

void TextureMapperGLData::initializeStencil()
{
    if (currentSurface) {
        static_cast<BitmapTextureGL*>(currentSurface.get())->initializeStencil();
        return;
    }

    if (didModifyStencil)
        return;

    glClearStencil(0);
    glClear(GL_STENCIL_BUFFER_BIT);
    didModifyStencil = true;
}

TextureMapperGL::TextureMapperGL()
    : m_data(new TextureMapperGLData)
    , m_context(0)
{
}

void TextureMapperGL::beginPainting()
{
    // Make sure that no GL error code stays from previous operations.
    glGetError();

    if (!initializeOpenGLShims())
        return;

    glGetIntegerv(GL_CURRENT_PROGRAM, &data().previousProgram);
    data().previousScissorState = glIsEnabled(GL_SCISSOR_TEST);
    data().previousDepthState = glIsEnabled(GL_DEPTH_TEST);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
#if PLATFORM(QT)
    if (m_context) {
        QPainter* painter = m_context->platformContext();
        painter->save();
        painter->beginNativePainting();
    }
#endif
    data().didModifyStencil = false;
    glDepthMask(0);
    glGetIntegerv(GL_VIEWPORT, data().viewport);
    glGetIntegerv(GL_SCISSOR_BOX, data().previousScissor);
    data().sharedGLData().clipState.stencilIndex = 1;
    data().sharedGLData().clipState.scissorBox = IntRect(0, 0, data().viewport[2], data().viewport[3]);
    bindSurface(0);
}

void TextureMapperGL::endPainting()
{
    if (data().didModifyStencil) {
        glClearStencil(1);
        glClear(GL_STENCIL_BUFFER_BIT);
    }

    glUseProgram(data().previousProgram);

    glScissor(data().previousScissor[0], data().previousScissor[1], data().previousScissor[2], data().previousScissor[3]);
    if (data().previousScissorState)
        glEnable(GL_SCISSOR_TEST);
    else
        glDisable(GL_SCISSOR_TEST);

    if (data().previousDepthState)
        glEnable(GL_DEPTH_TEST);
    else
        glDisable(GL_DEPTH_TEST);


#if PLATFORM(QT)
    if (!m_context)
        return;
    QPainter* painter = m_context->platformContext();
    painter->endNativePainting();
    painter->restore();
#endif
}


void TextureMapperGL::drawTexture(const BitmapTexture& texture, const FloatRect& targetRect, const TransformationMatrix& matrix, float opacity, const BitmapTexture* mask)
{
    if (!texture.isValid())
        return;

    if (data().sharedGLData().clipState.scissorBox.isEmpty())
        return;

    const BitmapTextureGL& textureGL = static_cast<const BitmapTextureGL&>(texture);
    drawTexture(textureGL.id(), textureGL.isOpaque() ? 0 : SupportsBlending, textureGL.relativeSize(), targetRect, matrix, opacity, mask);
}

void TextureMapperGL::drawTexture(uint32_t texture, Flags flags, const FloatSize& relativeSize, const FloatRect& targetRect, const TransformationMatrix& modelViewMatrix, float opacity, const BitmapTexture* maskTexture)
{
    RefPtr<TextureMapperShaderProgram> shaderInfo;
    if (maskTexture)
        shaderInfo = data().sharedGLData().textureMapperShaderManager.getShaderProgram<TextureMapperShaderProgramOpacityAndMask>();
    else
        shaderInfo = data().sharedGLData().textureMapperShaderManager.getShaderProgram<TextureMapperShaderProgramSimple>();

    GL_CMD(glUseProgram(shaderInfo->id()))
    GL_CMD(glEnableVertexAttribArray(shaderInfo->vertexAttrib()))
    GL_CMD(glActiveTexture(GL_TEXTURE0))
    GL_CMD(glBindTexture(GL_TEXTURE_2D, texture))
    GL_CMD(glBindBuffer(GL_ARRAY_BUFFER, 0))
    const GLfloat unitRect[] = {0, 0, 1, 0, 1, 1, 0, 1};
    GL_CMD(glVertexAttribPointer(shaderInfo->vertexAttrib(), 2, GL_FLOAT, GL_FALSE, 0, unitRect))

    TransformationMatrix matrix = TransformationMatrix(data().projectionMatrix).multiply(modelViewMatrix).multiply(TransformationMatrix(
            targetRect.width(), 0, 0, 0,
            0, targetRect.height(), 0, 0,
            0, 0, 1, 0,
            targetRect.x(), targetRect.y(), 0, 1));

    const GLfloat m4[] = {
        matrix.m11(), matrix.m12(), matrix.m13(), matrix.m14(),
        matrix.m21(), matrix.m22(), matrix.m23(), matrix.m24(),
        matrix.m31(), matrix.m32(), matrix.m33(), matrix.m34(),
        matrix.m41(), matrix.m42(), matrix.m43(), matrix.m44()
    };
    const GLfloat m4src[] = {relativeSize.width(), 0, 0, 0,
                                     0, relativeSize.height() * ((flags & ShouldFlipTexture) ? -1 : 1), 0, 0,
                                     0, 0, 1, 0,
                                     0, (flags & ShouldFlipTexture) ? relativeSize.height() : 0, 0, 1};

    GL_CMD(glUniformMatrix4fv(shaderInfo->matrixVariable(), 1, GL_FALSE, m4))
    GL_CMD(glUniformMatrix4fv(shaderInfo->sourceMatrixVariable(), 1, GL_FALSE, m4src))
    GL_CMD(glUniform1i(shaderInfo->sourceTextureVariable(), 0))

    shaderInfo->prepare(opacity, maskTexture);

    bool needsBlending = (flags & SupportsBlending) || opacity < 0.99 || maskTexture;

    if (needsBlending) {
        GL_CMD(glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA))
        GL_CMD(glEnable(GL_BLEND))
    } else
        GL_CMD(glDisable(GL_BLEND))

    GL_CMD(glDrawArrays(GL_TRIANGLE_FAN, 0, 4))
    GL_CMD(glDisableVertexAttribArray(shaderInfo->vertexAttrib()))
}

const char* TextureMapperGL::type() const
{
    return "OpenGL";
}

void BitmapTextureGL::didReset()
{
    IntSize newTextureSize = nextPowerOfTwo(contentSize());
    bool justCreated = false;
    if (!m_id) {
        GL_CMD(glGenTextures(1, &m_id))
        justCreated = true;
    }

    if (justCreated || newTextureSize.width() > m_textureSize.width() || newTextureSize.height() > m_textureSize.height()) {
        m_textureSize = newTextureSize;
        GL_CMD(glBindTexture(GL_TEXTURE_2D, m_id))
        GL_CMD(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR))
        GL_CMD(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR))
        GL_CMD(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE))
        GL_CMD(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE))
        GL_CMD(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_textureSize.width(), m_textureSize.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, 0))
    }

    // We decrease the size by one, since this is used as rectangle coordinates and not as size.
    m_relativeSize = FloatSize(float(contentSize().width() - 1) / m_textureSize.width(), float(contentSize().height() - 1) / m_textureSize.height());
    m_surfaceNeedsReset = true;
}

static void swizzleBGRAToRGBA(uint32_t* data, const IntSize& size)
{
    int width = size.width();
    int height = size.height();
    for (int y = 0; y < height; ++y) {
        uint32_t* p = data + y * width;
        for (int x = 0; x < width; ++x)
            p[x] = ((p[x] << 16) & 0xff0000) | ((p[x] >> 16) & 0xff) | (p[x] & 0xff00ff00);
    }
}

// FIXME: Move this to Extensions3D when we move TextureMapper to use GC3D.
static bool hasExtension(const char* extension)
{
    static Vector<String> availableExtensions;
    if (!availableExtensions.isEmpty())
        return availableExtensions.contains(extension);
    String extensionsString(reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS)));
    extensionsString.split(" ", availableExtensions);
    return availableExtensions.contains(extension);
}
static bool hasBGRAExtension()
{
#if !defined(TEXMAP_OPENGL_ES_2)
    return true;
#endif
    static bool hasBGRA = hasExtension("GL_EXT_texture_format_BGRA8888");
    return hasBGRA;
}

void BitmapTextureGL::updateContents(const void* data, const IntRect& targetRect)
{
    GLuint glFormat = GL_RGBA;
    GL_CMD(glBindTexture(GL_TEXTURE_2D, m_id))
    if (hasBGRAExtension())
        glFormat = GL_BGRA;
    else {
        swizzleBGRAToRGBA(static_cast<uint32_t*>(const_cast<void*>(data)), targetRect.size());
        glFormat = GL_RGBA;
    }

    GL_CMD(glTexSubImage2D(GL_TEXTURE_2D, 0, targetRect.x(), targetRect.y(), targetRect.width(), targetRect.height(), glFormat, GL_UNSIGNED_BYTE, data))
}

void BitmapTextureGL::updateContents(Image* image, const IntRect& targetRect, const IntRect& sourceRect, BitmapTexture::PixelFormat format)
{
    if (!image)
        return;
    GL_CMD(glBindTexture(GL_TEXTURE_2D, m_id))
    GLuint glFormat = isOpaque() ? GL_RGB : GL_RGBA;
    NativeImagePtr frameImage = image->nativeImageForCurrentFrame();
    if (!frameImage)
        return;

#if PLATFORM(QT)
    QImage qtImage;

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    // With QPA, we can avoid a deep copy.
    qtImage = *frameImage->handle()->buffer();
#else
    // This might be a deep copy, depending on other references to the pixmap.
    qtImage = frameImage->toImage();
#endif

    if (IntSize(qtImage.size()) != sourceRect.size())
        qtImage = qtImage.copy(sourceRect);
    if (format == BGRAFormat || format == BGRFormat) {
        if (hasBGRAExtension())
            glFormat = isOpaque() ? GL_BGR : GL_BGRA;
        else
            swizzleBGRAToRGBA(reinterpret_cast<uint32_t*>(qtImage.bits()), qtImage.size());
    }
    GL_CMD(glTexSubImage2D(GL_TEXTURE_2D, 0, targetRect.x(), targetRect.y(), targetRect.width(), targetRect.height(), glFormat, GL_UNSIGNED_BYTE, qtImage.constBits()))

#elif USE(CAIRO)

#if !CPU(BIG_ENDIAN)
#if defined(TEXMAP_OPENGL_ES_2)
    swizzleBGRAToRGBA(reinterpret_cast<uint32_t*>(cairo_image_surface_get_data(frameImage)),
                      cairo_image_surface_get_stride(frameImage) * cairo_image_surface_get_height(frameImage));
#else
    glFormat = isOpaque() ? GL_BGR : GL_BGRA;
#endif
#endif

    glPixelStorei(GL_UNPACK_ROW_LENGTH, cairo_image_surface_get_stride(frameImage) / 4);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, sourceRect.y());
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, sourceRect.x());
    GL_CMD(glTexSubImage2D(GL_TEXTURE_2D, 0,
                           targetRect.x(), targetRect.y(),
                           targetRect.width(), targetRect.height(),
                           glFormat, GL_UNSIGNED_BYTE,
                           cairo_image_surface_get_data(frameImage)));
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
#endif
}

static inline TransformationMatrix createProjectionMatrix(const IntSize& size, bool flip)
{
    const float near = 9999999;
    const float far = -99999;

    return TransformationMatrix(2.0 / float(size.width()), 0, 0, 0,
                                0, (flip ? -2.0 : 2.0) / float(size.height()), 0, 0,
                                0, 0, -2.f / (far - near), 0,
                                -1, flip ? 1 : -1, -(far + near) / (far - near), 1);
}

void BitmapTextureGL::initializeStencil()
{
    if (m_rbo)
        return;
    GL_CMD(glGenRenderbuffers(1, &m_rbo));
    GL_CMD(glBindRenderbuffer(GL_RENDERBUFFER, m_rbo))
#ifdef TEXMAP_OPENGL_ES_2
    GL_CMD(glRenderbufferStorage(GL_RENDERBUFFER, GL_STENCIL_INDEX8, m_textureSize.width(), m_textureSize.height()))
#else
    GL_CMD(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_STENCIL, m_textureSize.width(), m_textureSize.height()))
#endif
    GL_CMD(glBindRenderbuffer(GL_RENDERBUFFER, 0))
    GL_CMD(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_rbo))
    GL_CMD(glClearStencil(0))
    GL_CMD(glClear(GL_STENCIL_BUFFER_BIT))
}

void BitmapTextureGL::bind()
{
    if (m_surfaceNeedsReset || !m_fbo) {
        if (!m_fbo)
            GL_CMD(glGenFramebuffers(1, &m_fbo))
        GL_CMD(glBindFramebuffer(GL_FRAMEBUFFER, m_fbo))
        GL_CMD(glBindTexture(GL_TEXTURE_2D, 0))
        GL_CMD(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, id(), 0))
        GL_CMD(glClearColor(0, 0, 0, 0))
        GL_CMD(glClear(GL_COLOR_BUFFER_BIT))
        m_surfaceNeedsReset = false;
    } else
        GL_CMD(glBindFramebuffer(GL_FRAMEBUFFER, m_fbo))

    GL_CMD(glViewport(0, 0, size().width(), size().height()))
    m_textureMapper->data().projectionMatrix = createProjectionMatrix(size(), false);
    m_textureMapper->beginClip(TransformationMatrix(), FloatRect(IntPoint::zero(), contentSize()));
}

void BitmapTextureGL::destroy()
{
    if (m_id)
        GL_CMD(glDeleteTextures(1, &m_id))

    if (m_fbo)
        GL_CMD(glDeleteFramebuffers(1, &m_fbo))

    if (m_rbo)
        GL_CMD(glDeleteRenderbuffers(1, &m_rbo))

    m_fbo = 0;
    m_rbo = 0;
    m_id = 0;
    m_textureSize = IntSize();
    m_relativeSize = FloatSize(1, 1);
}

bool BitmapTextureGL::isValid() const
{
    return m_id;
}

IntSize BitmapTextureGL::size() const
{
    return m_textureSize;
}

TextureMapperGL::~TextureMapperGL()
{
    delete m_data;
}

void TextureMapperGL::bindSurface(BitmapTexture *surfacePointer)
{
    BitmapTextureGL* surface = static_cast<BitmapTextureGL*>(surfacePointer);

    if (!surface) {
        IntSize viewportSize(data().viewport[2], data().viewport[3]);
        GL_CMD(glBindFramebuffer(GL_FRAMEBUFFER, 0))
        data().projectionMatrix = createProjectionMatrix(viewportSize, true);
        GL_CMD(glViewport(0, 0, viewportSize.width(), viewportSize.height()))
        if (data().currentSurface)
            endClip();
        data().currentSurface.clear();
        return;
    }


    surface->bind();
    data().currentSurface = surface;
}

bool TextureMapperGL::beginScissorClip(const TransformationMatrix& modelViewMatrix, const FloatRect& targetRect)
{
    FloatQuad quad = modelViewMatrix.projectQuad(targetRect);
    IntRect rect = quad.enclosingBoundingBox();

    // Only use scissors on rectilinear clips.
    if (!quad.isRectilinear() || rect.isEmpty())
        return false;

    data().sharedGLData().clipState.scissorBox.intersect(rect);
    data().sharedGLData().applyCurrentClip();
    return true;
}

void TextureMapperGL::beginClip(const TransformationMatrix& modelViewMatrix, const FloatRect& targetRect)
{
    data().sharedGLData().pushClipState();
    if (beginScissorClip(modelViewMatrix, targetRect))
        return;

    data().initializeStencil();

    RefPtr<TextureMapperShaderProgramSimple> shaderInfo = data().sharedGLData().textureMapperShaderManager.getShaderProgram<TextureMapperShaderProgramSimple>();

    GL_CMD(glUseProgram(shaderInfo->id()))
    GL_CMD(glEnableVertexAttribArray(shaderInfo->vertexAttrib()))
    const GLfloat unitRect[] = {0, 0, 1, 0, 1, 1, 0, 1};
    GL_CMD(glVertexAttribPointer(shaderInfo->vertexAttrib(), 2, GL_FLOAT, GL_FALSE, 0, unitRect))

    TransformationMatrix matrix = TransformationMatrix(data().projectionMatrix)
            .multiply(modelViewMatrix)
            .multiply(TransformationMatrix(targetRect.width(), 0, 0, 0,
                0, targetRect.height(), 0, 0,
                0, 0, 1, 0,
                targetRect.x(), targetRect.y(), 0, 1));

    const GLfloat m4[] = {
        matrix.m11(), matrix.m12(), matrix.m13(), matrix.m14(),
        matrix.m21(), matrix.m22(), matrix.m23(), matrix.m24(),
        matrix.m31(), matrix.m32(), matrix.m33(), matrix.m34(),
        matrix.m41(), matrix.m42(), matrix.m43(), matrix.m44()
    };

    const GLfloat m4all[] = {
        2, 0, 0, 0,
        0, 2, 0, 0,
        0, 0, 1, 0,
        -1, -1, 0, 1
    };

    int& stencilIndex = data().sharedGLData().clipState.stencilIndex;

    GL_CMD(glEnable(GL_STENCIL_TEST))

    // Make sure we don't do any actual drawing.
    GL_CMD(glStencilFunc(GL_NEVER, stencilIndex, stencilIndex))

    // Operate only on the stencilIndex and above.
    GL_CMD(glStencilMask(0xff & ~(stencilIndex - 1)))

    // First clear the entire buffer at the current index.
    GL_CMD(glUniformMatrix4fv(shaderInfo->matrixVariable(), 1, GL_FALSE, m4all))
    GL_CMD(glStencilOp(GL_ZERO, GL_ZERO, GL_ZERO))
    GL_CMD(glDrawArrays(GL_TRIANGLE_FAN, 0, 4))

    // Now apply the current index to the new quad.
    GL_CMD(glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE))
    GL_CMD(glUniformMatrix4fv(shaderInfo->matrixVariable(), 1, GL_FALSE, m4))
    GL_CMD(glDrawArrays(GL_TRIANGLE_FAN, 0, 4))

    // Clear the state.
    GL_CMD(glDisableVertexAttribArray(shaderInfo->vertexAttrib()))
    GL_CMD(glStencilMask(0))

    // Increase stencilIndex and apply stencil testing.
    stencilIndex *= 2;
    data().sharedGLData().applyCurrentClip();
}

void TextureMapperGL::endClip()
{
    data().sharedGLData().popClipState();
    data().sharedGLData().applyCurrentClip();
}

PassRefPtr<BitmapTexture> TextureMapperGL::createTexture()
{
    BitmapTextureGL* texture = new BitmapTextureGL();
    texture->setTextureMapper(this);
    return adoptRef(texture);
}

PassOwnPtr<TextureMapper> TextureMapper::platformCreateAccelerated()
{
    return TextureMapperGL::create();
}

};
