/*
 * Copyright (C) 2006 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2008 Holger Hans Peter Freyther
 * Copyright (C) 2009 Dirk Schulze <krit@webkit.org>
 * Copyright (C) 2010 Torch Mobile (Beijing) Co. Ltd. All rights reserved.
 * Copyright (C) 2014 Digia Plc. and/or its subsidiary(-ies)
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "ImageBuffer.h"

#include "GraphicsContext.h"
#include "GraphicsSurface.h"
#include "ImageData.h"
#include "StillImageQt.h"

#include <QImage>
#include <QPaintEngine>
#include <QPainter>
#include <QPixmap>

#if ENABLE(ACCELERATED_2D_CANVAS)
#include "QFramebufferPaintDevice.h"
#include "TextureMapper.h"
#include "TextureMapperPlatformLayer.h"
#include "TextureMapperGL.h"
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>
#include <QOpenGLPaintDevice>
#include <QThreadStorage>
#include <private/qopenglpaintengine_p.h>
#endif

namespace WebCore {

#if ENABLE(ACCELERATED_2D_CANVAS)

class QOpenGLContextThreadStorage
{
public:
    QOpenGLContext *context() {
        QOpenGLContext *&context = storage.localData();
        if (!context) {
            context = new QOpenGLContext;
            context->create();
        }
        return context;
    }

private:
    QThreadStorage<QOpenGLContext *> storage;
};

Q_GLOBAL_STATIC(QOpenGLContextThreadStorage, imagebuffer_opengl_context)

// The owner of the surface needs to be separate from QFramebufferPaintDevice, since the surface
// must already be current with the QFramebufferObject constructor is called.
class ImageBufferContext {
public:
    ImageBufferContext(QOpenGLContext* sharedContext)
        : m_ownSurface(0)
    {
        if (sharedContext)
            m_format = sharedContext->format();

        m_context = sharedContext ? sharedContext : imagebuffer_opengl_context->context();

        m_surface = m_context->surface();
    }
    ~ImageBufferContext()
    {
        if (QOpenGLContext::currentContext() == m_context && m_context->surface() == m_ownSurface)
            m_context->doneCurrent();
        delete m_ownSurface;
    }
    void createSurfaceIfNeeded()
    {
        if (m_surface)
            return;

        m_ownSurface = new QOffscreenSurface;
        m_ownSurface->setFormat(m_format);
        m_ownSurface->create();

        m_surface = m_ownSurface;
    }
    void makeCurrentIfNeeded()
    {
        if (QOpenGLContext::currentContext() != m_context) {
            createSurfaceIfNeeded();

            m_context->makeCurrent(m_surface);
        }
    }
    QOpenGLContext* context() { return m_context; }

private:
    QSurface *m_surface;
    QOffscreenSurface *m_ownSurface;
    QOpenGLContext *m_context;
    QSurfaceFormat m_format;
};

// ---------------------- ImageBufferDataPrivateAccelerated

struct ImageBufferDataPrivateAccelerated : public TextureMapperPlatformLayer, public ImageBufferDataPrivate {
    ImageBufferDataPrivateAccelerated(const IntSize& size, QOpenGLContext* sharedContext);
    virtual ~ImageBufferDataPrivateAccelerated();

    virtual QPaintDevice* paintDevice() OVERRIDE { return m_paintDevice; }
    virtual QImage toQImage() const OVERRIDE;
    virtual PassRefPtr<Image> image() const OVERRIDE;
    virtual PassRefPtr<Image> copyImage() const OVERRIDE;
    virtual bool isAccelerated() const OVERRIDE { return true; }
    virtual PlatformLayer* platformLayer() OVERRIDE { return this; }

    void invalidateState() const;
    virtual void draw(GraphicsContext* destContext, ColorSpace styleColorSpace, const FloatRect& destRect,
                      const FloatRect& srcRect, CompositeOperator op, BlendMode blendMode, bool useLowQualityScale,
                      bool ownContext) OVERRIDE;
    virtual void drawPattern(GraphicsContext* destContext, const FloatRect& srcRect, const AffineTransform& patternTransform,
                             const FloatPoint& phase, ColorSpace styleColorSpace, CompositeOperator op,
                             const FloatRect& destRect, bool ownContext) OVERRIDE;
    virtual void clip(GraphicsContext* context, const FloatRect& floatRect) const OVERRIDE;
    virtual void platformTransformColorSpace(const Vector<int>& lookUpTable) OVERRIDE;

    // TextureMapperPlatformLayer:
    virtual void paintToTextureMapper(TextureMapper*, const FloatRect&, const TransformationMatrix& modelViewMatrix = TransformationMatrix(), float opacity = 1.0) OVERRIDE;
#if USE(GRAPHICS_SURFACE)
    virtual IntSize platformLayerSize() const OVERRIDE;
    virtual uint32_t copyToGraphicsSurface() OVERRIDE;
    virtual GraphicsSurfaceToken graphicsSurfaceToken() const OVERRIDE;
    RefPtr<GraphicsSurface> m_graphicsSurface;
#endif
private:
    QFramebufferPaintDevice* m_paintDevice;
    ImageBufferContext* m_context;
};

ImageBufferDataPrivateAccelerated::ImageBufferDataPrivateAccelerated(const IntSize& size, QOpenGLContext* sharedContext)
{
    m_context = new ImageBufferContext(sharedContext);
    m_context->makeCurrentIfNeeded();

    m_paintDevice = new QFramebufferPaintDevice(size);
}

ImageBufferDataPrivateAccelerated::~ImageBufferDataPrivateAccelerated()
{
    if (TextureMapperPlatformLayer::Client* client = TextureMapperPlatformLayer::client())
        client->platformLayerWasDestroyed();
    delete m_paintDevice;
    delete m_context;
}

QImage ImageBufferDataPrivateAccelerated::toQImage() const
{
    invalidateState();
    return m_paintDevice->toImage();
}

PassRefPtr<Image> ImageBufferDataPrivateAccelerated::image() const
{
    return copyImage();
}

PassRefPtr<Image> ImageBufferDataPrivateAccelerated::copyImage() const
{
    return StillImage::create(QPixmap::fromImage(toQImage()));
}

void ImageBufferDataPrivateAccelerated::invalidateState() const
{
    // This will flush pending QPainter operations and force ensureActiveTarget() to be called on the next paint.
    QOpenGL2PaintEngineEx* acceleratedPaintEngine = static_cast<QOpenGL2PaintEngineEx*>(m_paintDevice->paintEngine());
    acceleratedPaintEngine->invalidateState();
}

void ImageBufferDataPrivateAccelerated::draw(GraphicsContext* destContext, ColorSpace styleColorSpace, const FloatRect& destRect,
                                             const FloatRect& srcRect, CompositeOperator op, BlendMode blendMode,
                                             bool useLowQualityScale, bool /*ownContext*/)
{
    if (destContext->isAcceleratedContext()) {
        invalidateState();

        // If accelerated compositing is disabled, this may be the painter of the QGLWidget, which is a QGL2PaintEngineEx.
        QOpenGL2PaintEngineEx* acceleratedPaintEngine = dynamic_cast<QOpenGL2PaintEngineEx*>(destContext->platformContext()->paintEngine());
        if (acceleratedPaintEngine) {
            QPaintDevice* targetPaintDevice = acceleratedPaintEngine->paintDevice();

            QRect rect(QPoint(), m_paintDevice->size());

            // drawTexture's rendering is flipped relative to QtWebKit's convention, so we need to compensate
            FloatRect srcRectFlipped = m_paintDevice->paintFlipped()
                ? FloatRect(srcRect.x(), srcRect.maxY(), srcRect.width(), -srcRect.height())
                : FloatRect(srcRect.x(), rect.height() - srcRect.maxY(), srcRect.width(), srcRect.height());

            // Using the same texture as source and target of a rendering operation is undefined in OpenGL,
            // so if that's the case we need to use a temporary intermediate buffer.
            if (m_paintDevice == targetPaintDevice) {
                m_context->makeCurrentIfNeeded();

                QFramebufferPaintDevice device(rect.size(), QOpenGLFramebufferObject::NoAttachment, false);

                // We disable flipping in order to do a pure blit into the intermediate buffer
                device.setPaintFlipped(false);

                QPainter painter(&device);
                QOpenGL2PaintEngineEx* pe = static_cast<QOpenGL2PaintEngineEx*>(painter.paintEngine());
                pe->drawTexture(rect, m_paintDevice->texture(), rect.size(), rect);
                painter.end();

                acceleratedPaintEngine->drawTexture(destRect, device.texture(), rect.size(), srcRectFlipped);
            } else {
                acceleratedPaintEngine->drawTexture(destRect, m_paintDevice->texture(), rect.size(), srcRectFlipped);
            }

            return;
        }
    }
    RefPtr<Image> image = StillImage::create(QPixmap::fromImage(toQImage()));
    destContext->drawImage(image.get(), styleColorSpace, destRect, srcRect, op, blendMode,
                           DoNotRespectImageOrientation, useLowQualityScale);
}


void ImageBufferDataPrivateAccelerated::drawPattern(GraphicsContext* destContext, const FloatRect& srcRect, const AffineTransform& patternTransform,
                                                    const FloatPoint& phase, ColorSpace styleColorSpace, CompositeOperator op,
                                                    const FloatRect& destRect, bool /*ownContext*/)
{
    RefPtr<Image> image = StillImage::create(QPixmap::fromImage(toQImage()));
    image->drawPattern(destContext, srcRect, patternTransform, phase, styleColorSpace, op, destRect);
}

void ImageBufferDataPrivateAccelerated::clip(GraphicsContext* context, const FloatRect& floatRect) const
{
    QPixmap alphaMask = QPixmap::fromImage(toQImage());
    IntRect rect = enclosingIntRect(floatRect);
    context->pushTransparencyLayerInternal(rect, 1.0, alphaMask);
}

void ImageBufferDataPrivateAccelerated::platformTransformColorSpace(const Vector<int>& lookUpTable)
{
    QPainter* painter = paintDevice()->paintEngine()->painter();

    QImage image = toQImage().convertToFormat(QImage::Format_ARGB32);
    ASSERT(!image.isNull());

    uchar* bits = image.bits();
    const int bytesPerLine = image.bytesPerLine();

    for (int y = 0; y < image.height(); ++y) {
        quint32* scanLine = reinterpret_cast_ptr<quint32*>(bits + y * bytesPerLine);
        for (int x = 0; x < image.width(); ++x) {
            QRgb& pixel = scanLine[x];
            pixel = qRgba(lookUpTable[qRed(pixel)],
                          lookUpTable[qGreen(pixel)],
                          lookUpTable[qBlue(pixel)],
                          qAlpha(pixel));
        }
    }

    painter->save();
    painter->resetTransform();
    painter->setOpacity(1.0);
    painter->setClipping(false);
    painter->setCompositionMode(QPainter::CompositionMode_Source);
    painter->drawImage(QPoint(0,0), image);
    painter->restore();
}

void ImageBufferDataPrivateAccelerated::paintToTextureMapper(TextureMapper* textureMapper, const FloatRect& targetRect, const TransformationMatrix& matrix, float opacity)
{
    bool canRenderDirectly = false;
    if (textureMapper->accelerationMode() == TextureMapper::OpenGLMode) {
        if (QOpenGLContext::areSharing(m_context->context(),
                                       static_cast<TextureMapperGL*>(textureMapper)->graphicsContext3D()->platformGraphicsContext3D()))
            canRenderDirectly = true;
    }

    if (!canRenderDirectly) {
        QImage image = toQImage();
        TransformationMatrix oldTransform = textureMapper->graphicsContext()->get3DTransform();
        textureMapper->graphicsContext()->concat3DTransform(matrix);
        textureMapper->graphicsContext()->platformContext()->drawImage(targetRect, image);
        textureMapper->graphicsContext()->set3DTransform(oldTransform);
        return;
    }

    invalidateState();
#if QT_VERSION >= QT_VERSION_CHECK(5, 4, 0)
    static_cast<TextureMapperGL*>(textureMapper)->drawTexture(m_paintDevice->texture(), TextureMapperGL::ShouldBlend, m_paintDevice->size(), targetRect, matrix, opacity);
#else
    static_cast<TextureMapperGL*>(textureMapper)->drawTexture(m_paintDevice->texture(), TextureMapperGL::ShouldBlend | TextureMapperGL::ShouldFlipTexture, m_paintDevice->size(), targetRect, matrix, opacity);
#endif
}

#if USE(GRAPHICS_SURFACE)
IntSize ImageBufferDataPrivateAccelerated::platformLayerSize() const
{
    return m_paintDevice->size();
}

uint32_t ImageBufferDataPrivateAccelerated::copyToGraphicsSurface()
{
    if (!m_graphicsSurface) {
        GraphicsSurface::Flags flags = GraphicsSurface::SupportsAlpha | GraphicsSurface::SupportsTextureTarget | GraphicsSurface::SupportsSharing;
        m_graphicsSurface = GraphicsSurface::create(m_paintDevice->size(), flags);
    }

    invalidateState();

    m_graphicsSurface->copyFromTexture(m_paintDevice->texture(), IntRect(IntPoint(), m_paintDevice->size()));
    return m_graphicsSurface->swapBuffers();
}

GraphicsSurfaceToken ImageBufferDataPrivateAccelerated::graphicsSurfaceToken() const
{
    return m_graphicsSurface->exportToken();
}
#endif // USE(GRAPHICS_SURFACE)

#endif // ENABLE(ACCELERATED_2D_CANVAS)

// ---------------------- ImageBufferDataPrivateUnaccelerated

struct ImageBufferDataPrivateUnaccelerated : public ImageBufferDataPrivate {
    ImageBufferDataPrivateUnaccelerated(const IntSize& size);
    virtual QPaintDevice* paintDevice() OVERRIDE { return m_pixmap.isNull() ? 0 : &m_pixmap; }
    virtual QImage toQImage() const OVERRIDE;
    virtual PassRefPtr<Image> image() const OVERRIDE;
    virtual PassRefPtr<Image> copyImage() const OVERRIDE;
    virtual bool isAccelerated() const OVERRIDE { return false; }
    virtual PlatformLayer* platformLayer() OVERRIDE { return 0; }
    virtual void draw(GraphicsContext* destContext, ColorSpace styleColorSpace, const FloatRect& destRect,
              const FloatRect& srcRect, CompositeOperator op, BlendMode blendMode, bool useLowQualityScale,
              bool ownContext) OVERRIDE;
    virtual void drawPattern(GraphicsContext* destContext, const FloatRect& srcRect, const AffineTransform& patternTransform,
                     const FloatPoint& phase, ColorSpace styleColorSpace, CompositeOperator op,
                     const FloatRect& destRect, bool ownContext) OVERRIDE;
    virtual void clip(GraphicsContext* context, const FloatRect& floatRect) const OVERRIDE;
    virtual void platformTransformColorSpace(const Vector<int>& lookUpTable) OVERRIDE;

    QPixmap m_pixmap;
    RefPtr<Image> m_image;
};

ImageBufferDataPrivateUnaccelerated::ImageBufferDataPrivateUnaccelerated(const IntSize& size)
    : m_pixmap(size)
    , m_image(StillImage::createForRendering(&m_pixmap))
{
    m_pixmap.fill(QColor(Qt::transparent));
}

QImage ImageBufferDataPrivateUnaccelerated::toQImage() const
{
    QPaintEngine* paintEngine = m_pixmap.paintEngine();
    if (!paintEngine || paintEngine->type() != QPaintEngine::Raster)
        return m_pixmap.toImage();

    // QRasterPixmapData::toImage() will deep-copy the backing QImage if there's an active QPainter on it.
    // For performance reasons, we don't want that here, so we temporarily redirect the paint engine.
    QPaintDevice* currentPaintDevice = paintEngine->paintDevice();
    paintEngine->setPaintDevice(0);
    QImage image = m_pixmap.toImage();
    paintEngine->setPaintDevice(currentPaintDevice);
    return image;
}

PassRefPtr<Image> ImageBufferDataPrivateUnaccelerated::image() const
{
    return StillImage::createForRendering(&m_pixmap);
}

PassRefPtr<Image> ImageBufferDataPrivateUnaccelerated::copyImage() const
{
    return StillImage::create(m_pixmap);
}

void ImageBufferDataPrivateUnaccelerated::draw(GraphicsContext* destContext, ColorSpace styleColorSpace, const FloatRect& destRect,
                                               const FloatRect& srcRect, CompositeOperator op, BlendMode blendMode,
                                               bool useLowQualityScale, bool ownContext)
{
    if (ownContext) {
        // We're drawing into our own buffer.  In order for this to work, we need to copy the source buffer first.
        RefPtr<Image> copy = copyImage();
        destContext->drawImage(copy.get(), ColorSpaceDeviceRGB, destRect, srcRect, op, blendMode, DoNotRespectImageOrientation, useLowQualityScale);
    } else
        destContext->drawImage(m_image.get(), styleColorSpace, destRect, srcRect, op, blendMode, DoNotRespectImageOrientation, useLowQualityScale);
}

void ImageBufferDataPrivateUnaccelerated::drawPattern(GraphicsContext* destContext, const FloatRect& srcRect, const AffineTransform& patternTransform,
                                                      const FloatPoint& phase, ColorSpace styleColorSpace, CompositeOperator op,
                                                      const FloatRect& destRect, bool ownContext)
{
    if (ownContext) {
        // We're drawing into our own buffer.  In order for this to work, we need to copy the source buffer first.
        RefPtr<Image> copy = copyImage();
        copy->drawPattern(destContext, srcRect, patternTransform, phase, styleColorSpace, op, destRect);
    } else
        m_image->drawPattern(destContext, srcRect, patternTransform, phase, styleColorSpace, op, destRect);
}

void ImageBufferDataPrivateUnaccelerated::clip(GraphicsContext* context, const FloatRect& floatRect) const
{
    QPixmap* nativeImage = m_image->nativeImageForCurrentFrame();

    if (!nativeImage)
        return;

    IntRect rect = enclosingIntRect(floatRect);
    QPixmap alphaMask = *nativeImage;

    context->pushTransparencyLayerInternal(rect, 1.0, alphaMask);
}

void ImageBufferDataPrivateUnaccelerated::platformTransformColorSpace(const Vector<int>& lookUpTable)
{
    QPainter* painter = paintDevice()->paintEngine()->painter();

    bool isPainting = painter->isActive();
    if (isPainting)
        painter->end();

    QImage image = toQImage().convertToFormat(QImage::Format_ARGB32);
    ASSERT(!image.isNull());

    uchar* bits = image.bits();
    const int bytesPerLine = image.bytesPerLine();

    for (int y = 0; y < m_pixmap.height(); ++y) {
        quint32* scanLine = reinterpret_cast_ptr<quint32*>(bits + y * bytesPerLine);
        for (int x = 0; x < m_pixmap.width(); ++x) {
            QRgb& pixel = scanLine[x];
            pixel = qRgba(lookUpTable[qRed(pixel)],
                          lookUpTable[qGreen(pixel)],
                          lookUpTable[qBlue(pixel)],
                          qAlpha(pixel));
        }
    }

    m_pixmap = QPixmap::fromImage(image);

    if (isPainting)
        painter->begin(&m_pixmap);
}

// ---------------------- ImageBufferData

ImageBufferData::ImageBufferData(const IntSize& size)
{
    m_painter = new QPainter;

    m_impl = new ImageBufferDataPrivateUnaccelerated(size);

    if (!m_impl->paintDevice())
        return;
    if (!m_painter->begin(m_impl->paintDevice()))
        return;

    initPainter();
}

#if ENABLE(ACCELERATED_2D_CANVAS)
ImageBufferData::ImageBufferData(const IntSize& size, QOpenGLContext* compatibleContext)
{
    m_painter = new QPainter;

    m_impl = new ImageBufferDataPrivateAccelerated(size, compatibleContext);

    if (!m_impl->paintDevice())
        return;
    if (!m_painter->begin(m_impl->paintDevice()))
        return;

    initPainter();
}
#endif

ImageBufferData::~ImageBufferData()
{
#if ENABLE(ACCELERATED_2D_CANVAS)
    if (m_impl->isAccelerated())
        static_cast<QFramebufferPaintDevice*>(m_impl->paintDevice())->ensureActiveTarget();
#endif
    m_painter->end();
    delete m_painter;
    delete m_impl;
}

void ImageBufferData::initPainter()
{
    m_painter->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

    // Since ImageBuffer is used mainly for Canvas, explicitly initialize
    // its painter's pen and brush with the corresponding canvas defaults
    // NOTE: keep in sync with CanvasRenderingContext2D::State
    QPen pen = m_painter->pen();
    pen.setColor(Qt::black);
    pen.setWidth(1);
    pen.setCapStyle(Qt::FlatCap);
    pen.setJoinStyle(Qt::SvgMiterJoin);
    pen.setMiterLimit(10);
    m_painter->setPen(pen);
    QBrush brush = m_painter->brush();
    brush.setColor(Qt::black);
    m_painter->setBrush(brush);
    m_painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
}

}
