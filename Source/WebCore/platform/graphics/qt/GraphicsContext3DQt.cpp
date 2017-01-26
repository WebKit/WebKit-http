/*
    Copyright (C) 2010 Tieto Corporation.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.
     
    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.
     
    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "config.h"
#include "GraphicsContext3D.h"

#include "Extensions3DOpenGLCommon.h"
#include "GraphicsContext.h"
#include "GraphicsSurface.h"
#include "HostWindow.h"
#include "ImageBuffer.h"
#include "ImageData.h"
#include "NativeImageQt.h"
#include "NotImplemented.h"
#include "QWebPageClient.h"
#include "SharedBuffer.h"
#include "TextureMapperPlatformLayer.h"
#include <QOffscreenSurface>
#include <private/qopenglextensions_p.h>
#include <qpa/qplatformpixmap.h>
#include <wtf/text/CString.h>

#if USE(TEXTURE_MAPPER_GL)
#include <texmap/TextureMapperGL.h>
#endif

#if ENABLE(GRAPHICS_CONTEXT_3D)

QT_BEGIN_NAMESPACE
extern Q_GUI_EXPORT QImage qt_gl_read_framebuffer(const QSize&, bool alpha_format, bool include_alpha);
QT_END_NAMESPACE

namespace WebCore {

#if !defined(GLchar)
typedef char GLchar;
#endif

#ifndef GL_VERTEX_PROGRAM_POINT_SIZE
#define GL_VERTEX_PROGRAM_POINT_SIZE      0x8642
#endif

#ifndef GL_POINT_SPRITE
#define GL_POINT_SPRITE                   0x8861
#endif

#ifndef GL_DEPTH24_STENCIL8
#define GL_DEPTH24_STENCIL8               0x88F0
#endif

#ifndef GL_READ_FRAMEBUFFER
#define GL_READ_FRAMEBUFFER               0x8CA8
#endif

#ifndef GL_DRAW_FRAMEBUFFER
#define GL_DRAW_FRAMEBUFFER               0x8CA9
#endif

class GraphicsContext3DPrivate : public TextureMapperPlatformLayer, public QOpenGLExtensions {
public:
    GraphicsContext3DPrivate(GraphicsContext3D*, HostWindow*, GraphicsContext3D::RenderStyle);
    ~GraphicsContext3DPrivate();

    virtual void paintToTextureMapper(TextureMapper&, const FloatRect& target, const TransformationMatrix&, float opacity);
#if USE(GRAPHICS_SURFACE)
    virtual IntSize platformLayerSize() const;
    virtual uint32_t copyToGraphicsSurface();
    virtual GraphicsSurfaceToken graphicsSurfaceToken() const;
#endif

    QRectF boundingRect() const;
    void blitMultisampleFramebuffer();
    void blitMultisampleFramebufferAndRestoreContext();
    bool makeCurrentIfNeeded() const;
    void createOffscreenBuffers();
    void initializeANGLE();
    void createGraphicsSurfaces(const IntSize&);

    bool isOpenGLES() const;
    bool isValid() const;

    GraphicsContext3D* m_context;
    HostWindow* m_hostWindow;
    PlatformGraphicsSurface3D m_surface;
    PlatformGraphicsContext3D m_platformContext;
    QObject* m_surfaceOwner;
#if USE(GRAPHICS_SURFACE)
    GraphicsSurface::Flags m_surfaceFlags;
    RefPtr<GraphicsSurface> m_graphicsSurface;
#endif

    // Register as a child of a Qt context to make the necessary when it may be destroyed before the GraphicsContext3D instance
    class QtContextWatcher : public QObject {
    public:
        QtContextWatcher(QObject* ctx, GraphicsContext3DPrivate* watcher)
            : QObject(ctx), m_watcher(watcher) { }
        ~QtContextWatcher() { m_watcher->m_platformContext = 0; m_watcher->m_platformContextWatcher = 0; }

    private:
        GraphicsContext3DPrivate* m_watcher;
    };
    QtContextWatcher* m_platformContextWatcher;
};

bool GraphicsContext3DPrivate::isOpenGLES() const
{
    if (m_platformContext)
        return m_platformContext->isOpenGLES();
#if USE(OPENGL_ES_2)
    return true;
#else
    return false;
#endif
}

bool GraphicsContext3DPrivate::isValid() const
{
    if (!m_platformContext || !m_platformContext->isValid())
        return false;
    return m_platformContext->isOpenGLES() || m_platformContext->format().majorVersion() >= 2;
}

bool GraphicsContext3D::isGLES2Compliant() const
{
    if (m_private)
        return m_private->isOpenGLES();
    ASSERT_NOT_REACHED();
#if USE(OPENGL_ES_2)
    return true;
#else
    return false;
#endif
}

GraphicsContext3DPrivate::GraphicsContext3DPrivate(GraphicsContext3D* context, HostWindow* hostWindow, GraphicsContext3D::RenderStyle renderStyle)
    : m_context(context)
    , m_hostWindow(hostWindow)
    , m_surface(0)
    , m_platformContext(0)
    , m_surfaceOwner(0)
    , m_platformContextWatcher(0)
{
    if (renderStyle == GraphicsContext3D::RenderToCurrentGLContext) {
        m_platformContext = QOpenGLContext::currentContext();
        if (m_platformContext)
            m_surface = m_platformContext->surface();

        // Watcher needed to invalidate the GL context if destroyed before this instance
        m_platformContextWatcher = new QtContextWatcher(m_platformContext, this);

        initializeOpenGLFunctions();
        return;
    }

    QOpenGLContext* shareContext = 0;
    if (hostWindow && hostWindow->platformPageClient())
        shareContext = hostWindow->platformPageClient()->openGLContextIfAvailable();

    QOffscreenSurface* surface = new QOffscreenSurface;
    surface->create();
    m_surface = surface;
    m_surfaceOwner = surface;

    m_platformContext = new QOpenGLContext(m_surfaceOwner);
    if (shareContext)
        m_platformContext->setShareContext(shareContext);

    if (!m_platformContext->create()) {
        delete m_platformContext;
        m_platformContext = 0;
        return;
    }

    makeCurrentIfNeeded();

    initializeOpenGLFunctions();
#if USE(GRAPHICS_SURFACE)
    IntSize surfaceSize(m_context->m_currentWidth, m_context->m_currentHeight);
    m_surfaceFlags = GraphicsSurface::SupportsTextureTarget | GraphicsSurface::SupportsSharing;

    if (!surfaceSize.isEmpty())
        m_graphicsSurface = GraphicsSurface::create(surfaceSize, m_surfaceFlags, m_platformContext);
#endif
}

void GraphicsContext3DPrivate::createOffscreenBuffers()
{
    glGenFramebuffers(/* count */ 1, &m_context->m_fbo);

    glGenTextures(1, &m_context->m_texture);
    glBindTexture(GL_TEXTURE_2D, m_context->m_texture);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Create a multisample FBO.
    if (m_context->m_attrs.antialias) {
        glGenFramebuffers(1, &m_context->m_multisampleFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, m_context->m_multisampleFBO);
        m_context->m_state.boundFBO = m_context->m_multisampleFBO;
        glGenRenderbuffers(1, &m_context->m_multisampleColorBuffer);
        if (m_context->m_attrs.stencil || m_context->m_attrs.depth)
            glGenRenderbuffers(1, &m_context->m_multisampleDepthStencilBuffer);
    } else {
        // Bind canvas FBO.
        glBindFramebuffer(GL_FRAMEBUFFER, m_context->m_fbo);
        m_context->m_state.boundFBO = m_context->m_fbo;
        if (isOpenGLES()) {
            if (m_context->m_attrs.depth)
                glGenRenderbuffers(1, &m_context->m_depthBuffer);
            if (m_context->m_attrs.stencil)
                glGenRenderbuffers(1, &m_context->m_stencilBuffer);
        }
        if (m_context->m_attrs.stencil || m_context->m_attrs.depth)
            glGenRenderbuffers(1, &m_context->m_depthStencilBuffer);
    }
}

void GraphicsContext3DPrivate::initializeANGLE()
{
    ShBuiltInResources ANGLEResources;
    ShInitBuiltInResources(&ANGLEResources);

    m_context->getIntegerv(GraphicsContext3D::MAX_VERTEX_ATTRIBS, &ANGLEResources.MaxVertexAttribs);
    m_context->getIntegerv(GraphicsContext3D::MAX_VERTEX_UNIFORM_VECTORS, &ANGLEResources.MaxVertexUniformVectors);
    m_context->getIntegerv(GraphicsContext3D::MAX_VARYING_VECTORS, &ANGLEResources.MaxVaryingVectors);
    m_context->getIntegerv(GraphicsContext3D::MAX_VERTEX_TEXTURE_IMAGE_UNITS, &ANGLEResources.MaxVertexTextureImageUnits);
    m_context->getIntegerv(GraphicsContext3D::MAX_COMBINED_TEXTURE_IMAGE_UNITS, &ANGLEResources.MaxCombinedTextureImageUnits);
    m_context->getIntegerv(GraphicsContext3D::MAX_TEXTURE_IMAGE_UNITS, &ANGLEResources.MaxTextureImageUnits);
    m_context->getIntegerv(GraphicsContext3D::MAX_FRAGMENT_UNIFORM_VECTORS, &ANGLEResources.MaxFragmentUniformVectors);

    // Always set to 1 for OpenGL ES.
    ANGLEResources.MaxDrawBuffers = 1;

    Extensions3D* extensions = m_context->getExtensions();
    if (extensions->supports("GL_ARB_texture_rectangle"))
        ANGLEResources.ARB_texture_rectangle = 1;

    GC3Dint range[2], precision;
    m_context->getShaderPrecisionFormat(GraphicsContext3D::FRAGMENT_SHADER, GraphicsContext3D::HIGH_FLOAT, range, &precision);
    ANGLEResources.FragmentPrecisionHigh = (range[0] || range[1] || precision);

    m_context->m_compiler.setResources(ANGLEResources);
}

GraphicsContext3DPrivate::~GraphicsContext3DPrivate()
{
    delete m_surfaceOwner;
    m_surfaceOwner = 0;

    delete m_platformContextWatcher;
    m_platformContextWatcher = 0;
}

void GraphicsContext3DPrivate::paintToTextureMapper(TextureMapper& textureMapper, const FloatRect& targetRect, const TransformationMatrix& matrix, float opacity)
{
    m_context->markLayerComposited();
    blitMultisampleFramebufferAndRestoreContext();

    // QTFIXME: Restore SoftwareMode
    if (true /*textureMapper.accelerationMode() == TextureMapper::OpenGLMode*/) {
        TextureMapperGL& texmapGL = static_cast<TextureMapperGL&>(textureMapper);
        TextureMapperGL::Flags flags = TextureMapperGL::ShouldFlipTexture | (m_context->m_attrs.alpha ? TextureMapperGL::ShouldBlend : 0);
        IntSize textureSize(m_context->m_currentWidth, m_context->m_currentHeight);
        texmapGL.drawTexture(m_context->m_texture, flags, textureSize, targetRect, matrix, opacity);
        return;
    }

    // Alternatively read pixels to a memory buffer.
    GraphicsContext* context = textureMapper.graphicsContext();
    QPainter* painter = context->platformContext();
    painter->save();
    painter->setTransform(matrix);
    painter->setOpacity(opacity);

    const int height = m_context->m_currentHeight;
    const int width = m_context->m_currentWidth;

    painter->beginNativePainting();
    makeCurrentIfNeeded();
    glBindFramebuffer(GL_FRAMEBUFFER, m_context->m_fbo);
    QImage offscreenImage = qt_gl_read_framebuffer(QSize(width, height), true, true);
    glBindFramebuffer(GL_FRAMEBUFFER, m_context->m_state.boundFBO);

    painter->endNativePainting();

    painter->drawImage(targetRect, offscreenImage);
    painter->restore();
}

#if USE(GRAPHICS_SURFACE)
IntSize GraphicsContext3DPrivate::platformLayerSize() const
{
    return IntSize(m_context->m_currentWidth, m_context->m_currentHeight);
}

uint32_t GraphicsContext3DPrivate::copyToGraphicsSurface()
{
    if (!m_graphicsSurface)
        return 0;

    m_context->markLayerComposited();
    blitMultisampleFramebufferAndRestoreContext();
    m_graphicsSurface->copyFromTexture(m_context->m_texture, IntRect(0, 0, m_context->m_currentWidth, m_context->m_currentHeight));
    return m_graphicsSurface->frontBuffer();
}

GraphicsSurfaceToken GraphicsContext3DPrivate::graphicsSurfaceToken() const
{
    return m_graphicsSurface->exportToken();
}
#endif

QRectF GraphicsContext3DPrivate::boundingRect() const
{
    return QRectF(QPointF(0, 0), QSizeF(m_context->m_currentWidth, m_context->m_currentHeight));
}

void GraphicsContext3DPrivate::blitMultisampleFramebuffer()
{
    if (!m_context->m_attrs.antialias)
        return;

    if (!isOpenGLES()) {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, m_context->m_multisampleFBO);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_context->m_fbo);
        glBlitFramebuffer(0, 0, m_context->m_currentWidth, m_context->m_currentHeight, 0, 0, m_context->m_currentWidth, m_context->m_currentHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, m_context->m_state.boundFBO);
}

void GraphicsContext3DPrivate::blitMultisampleFramebufferAndRestoreContext()
{
    const QOpenGLContext* currentContext = QOpenGLContext::currentContext();
    QSurface* currentSurface = 0;
    if (currentContext && currentContext != m_platformContext) {
        currentSurface = currentContext->surface();
        m_platformContext->makeCurrent(m_surface);
    }

    if (m_context->m_attrs.antialias)
        blitMultisampleFramebuffer();

    // While the context is still bound, make sure all the Framebuffer content is in finished state.
    // This is necessary as we are doing our own double buffering instead of using a drawable that provides swapBuffers.
    glFinish();

    if (currentContext && currentContext != m_platformContext)
        const_cast<QOpenGLContext*>(currentContext)->makeCurrent(currentSurface);
}

bool GraphicsContext3DPrivate::makeCurrentIfNeeded() const
{
    if (!m_platformContext)
        return false;
    const QOpenGLContext* currentContext = QOpenGLContext::currentContext();
    if (currentContext == m_platformContext)
        return true;
    return m_platformContext->makeCurrent(m_surface);
}

void GraphicsContext3DPrivate::createGraphicsSurfaces(const IntSize& size)
{
#if USE(GRAPHICS_SURFACE)
    if (size.isEmpty())
        m_graphicsSurface = nullptr;
    else
        m_graphicsSurface = GraphicsSurface::create(size, m_surfaceFlags, m_platformContext);
#else
    UNUSED_PARAM(size);
#endif
}

PassRefPtr<GraphicsContext3D> GraphicsContext3D::create(GraphicsContext3D::Attributes attrs, HostWindow* hostWindow, GraphicsContext3D::RenderStyle renderStyle)
{
    // This implementation doesn't currently support rendering directly to the HostWindow.
    if (renderStyle == RenderDirectlyToHostWindow)
        return 0;
    RefPtr<GraphicsContext3D> context = adoptRef(new GraphicsContext3D(attrs, hostWindow, renderStyle));
    return context->m_private ? context.release() : 0;
}

GraphicsContext3D::GraphicsContext3D(GraphicsContext3D::Attributes attrs, HostWindow* hostWindow, GraphicsContext3D::RenderStyle renderStyle)
    : m_currentWidth(0)
    , m_currentHeight(0)
    , m_attrs(attrs)
    , m_renderStyle(renderStyle)
    , m_texture(0)
    , m_compositorTexture(0)
    , m_fbo(0)
    , m_depthBuffer(0)
    , m_stencilBuffer(0)
    , m_depthStencilBuffer(0)
    , m_layerComposited(false)
    , m_internalColorFormat(0)
    , m_multisampleFBO(0)
    , m_multisampleDepthStencilBuffer(0)
    , m_multisampleColorBuffer(0)
    , m_functions(0)
    , m_private(std::make_unique<GraphicsContext3DPrivate>(this, hostWindow, renderStyle))
    , m_compiler(isGLES2Compliant() ? SH_ESSL_OUTPUT : SH_GLSL_OUTPUT)
    , m_webglContext(nullptr)
{
    if (!m_private->m_surface || !m_private->m_platformContext) {
        LOG_ERROR("GraphicsContext3D: GL context creation failed.");
        m_private = nullptr;
        return;
    }

    m_functions = m_private.get();
    validateAttributes();

    if (!m_private->isValid()) {
        m_private = nullptr;
        return;
    }

    if (renderStyle == RenderOffscreen)
        m_private->createOffscreenBuffers();

    m_private->initializeANGLE();

    if (!isGLES2Compliant()) {
        m_functions->glEnable(GL_POINT_SPRITE);
        m_functions->glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
    }

    if (renderStyle != RenderToCurrentGLContext)
        m_functions->glClearColor(0.0, 0.0, 0.0, 0.0);
}

GraphicsContext3D::~GraphicsContext3D()
{
    // If GraphicsContext3D init failed in constructor, m_private set to nullptr and no buffers are allocated.
    if (!m_private)
        return;

    if (makeContextCurrent()) {
        m_functions->glDeleteTextures(1, &m_texture);
        m_functions->glDeleteFramebuffers(1, &m_fbo);
        if (m_attrs.antialias) {
            m_functions->glDeleteRenderbuffers(1, &m_multisampleColorBuffer);
            m_functions->glDeleteFramebuffers(1, &m_multisampleFBO);
            if (m_attrs.stencil || m_attrs.depth)
                m_functions->glDeleteRenderbuffers(1, &m_multisampleDepthStencilBuffer);
        } else if (m_attrs.stencil || m_attrs.depth) {
            if (isGLES2Compliant()) {
                if (m_attrs.depth)
                    m_functions->glDeleteRenderbuffers(1, &m_depthBuffer);
                if (m_attrs.stencil)
                    m_functions->glDeleteRenderbuffers(1, &m_stencilBuffer);
            }
            m_functions->glDeleteRenderbuffers(1, &m_depthStencilBuffer);
        }
    }

    m_functions = 0;
}

PlatformGraphicsContext3D GraphicsContext3D::platformGraphicsContext3D()
{
    return m_private->m_platformContext;
}

Platform3DObject GraphicsContext3D::platformTexture() const
{
    return m_texture;
}

PlatformLayer* GraphicsContext3D::platformLayer() const
{
    return m_private.get();
}

bool GraphicsContext3D::makeContextCurrent()
{
    if (!m_private)
        return false;
    return m_private->makeCurrentIfNeeded();
}

void GraphicsContext3D::paintToCanvas(const unsigned char* imagePixels, int imageWidth, int imageHeight,
                                      int canvasWidth, int canvasHeight, QPainter* context)
{
    QImage image(imagePixels, imageWidth, imageHeight, NativeImageQt::defaultFormatForAlphaEnabledImages());
    context->save();
    context->translate(0, imageHeight);
    context->scale(1, -1);
    context->setCompositionMode(QPainter::CompositionMode_Source);
    context->drawImage(QRect(0, 0, canvasWidth, -canvasHeight), image);
    context->restore();
}

#if USE(GRAPHICS_SURFACE)
void GraphicsContext3D::createGraphicsSurfaces(const IntSize& size)
{
    m_private->createGraphicsSurfaces(size);
}
#endif

GraphicsContext3D::ImageExtractor::~ImageExtractor()
{
}

bool GraphicsContext3D::ImageExtractor::extractImage(bool premultiplyAlpha, bool ignoreGammaAndColorProfile)
{
    UNUSED_PARAM(ignoreGammaAndColorProfile);
    if (!m_image)
        return false;

    // Is image already loaded? If not, load it.
    if (m_image->data())
        m_qtImage = QImage::fromData(reinterpret_cast<const uchar*>(m_image->data()->data()), m_image->data()->size());
    else {
        QPixmap* nativePixmap = m_image->nativeImageForCurrentFrame();
        if (!nativePixmap)
            return false;

        // With QPA, we can avoid a deep copy.
        m_qtImage = *nativePixmap->handle()->buffer();
    }

    m_alphaOp = AlphaDoNothing;
    switch (m_qtImage.format()) {
    case QImage::Format_RGB32:
        // For opaque images, we should not premultiply or unmultiply alpha.
        break;
    case QImage::Format_ARGB32:
        if (premultiplyAlpha)
            m_alphaOp = AlphaDoPremultiply;
        break;
    case QImage::Format_ARGB32_Premultiplied:
        if (!premultiplyAlpha)
            m_alphaOp = AlphaDoUnmultiply;
        break;
    default:
        // The image has a format that is not supported in packPixels. We have to convert it here.
        m_qtImage = m_qtImage.convertToFormat(premultiplyAlpha ? QImage::Format_ARGB32_Premultiplied : QImage::Format_ARGB32);
        break;
    }

    m_imageWidth = m_image->width();
    m_imageHeight = m_image->height();
    if (!m_imageWidth || !m_imageHeight)
        return false;
    m_imagePixelData = m_qtImage.constBits();
    m_imageSourceFormat = DataFormatBGRA8;
    m_imageSourceUnpackAlignment = 0;

    return true;
}

void GraphicsContext3D::checkGPUStatusIfNecessary()
{
}

void GraphicsContext3D::setContextLostCallback(std::unique_ptr<ContextLostCallback>)
{
}

void GraphicsContext3D::setErrorMessageCallback(std::unique_ptr<ErrorMessageCallback>)
{
}

}

#endif // ENABLE(GRAPHICS_CONTEXT_3D)
