/*
 Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
 Copyright (C) 2012 Igalia S.L.

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
#include "NotImplemented.h"
#include "TextureMapperShaderManager.h"
#include "Timer.h"
#include <wtf/HashMap.h>
#include <wtf/OwnArrayPtr.h>
#include <wtf/PassOwnArrayPtr.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>

#if USE(GRAPHICS_SURFACE)
#include "GraphicsSurface.h"
#endif

#if PLATFORM(QT)
#include "NativeImageQt.h"
#endif

#if USE(CAIRO)
#include "CairoUtilities.h"
#include "RefPtrCairo.h"
#include <cairo.h>
#endif

#if !USE(TEXMAP_OPENGL_ES_2)
// FIXME: Move to Extensions3D.h.
#define GL_TEXTURE_RECTANGLE_ARB 0x84F5
#define GL_UNSIGNED_INT_8_8_8_8_REV 0x8367
#define GL_UNPACK_ROW_LENGTH 0x0CF2
#define GL_UNPACK_SKIP_PIXELS 0x0CF4
#define GL_UNPACK_SKIP_ROWS 0x0CF3
#endif

namespace WebCore {
struct TextureMapperGLData {
    WTF_MAKE_FAST_ALLOCATED;
public:
    struct SharedGLData : public RefCounted<SharedGLData> {

        typedef HashMap<PlatformGraphicsContext3D, SharedGLData*> GLContextDataMap;
        static GLContextDataMap& glContextDataMap()
        {
            static GLContextDataMap map;
            return map;
        }

        static PassRefPtr<SharedGLData> currentSharedGLData(GraphicsContext3D* context)
        {
            GLContextDataMap::iterator it = glContextDataMap().find(context->platformGraphicsContext3D());
            if (it != glContextDataMap().end())
                return it->second;

            return adoptRef(new SharedGLData(context));
        }



        TextureMapperShaderManager textureMapperShaderManager;

        SharedGLData(GraphicsContext3D* context)
            : textureMapperShaderManager(context)
        {
            glContextDataMap().add(context->platformGraphicsContext3D(), this);
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
        return *m_sharedGLData;
    }

    void initializeStencil();

    TextureMapperGLData(GraphicsContext3D* context)
        : context(context)
        , PaintFlags(0)
        , previousProgram(0)
        , targetFrameBuffer(0)
        , didModifyStencil(false)
        , previousScissorState(0)
        , previousDepthState(0)
        , m_sharedGLData(TextureMapperGLData::SharedGLData::currentSharedGLData(this->context))
    { }

    GraphicsContext3D* context;
    TransformationMatrix projectionMatrix;
    TextureMapper::PaintFlags PaintFlags;
    GC3Dint previousProgram;
    GC3Dint targetFrameBuffer;
    bool didModifyStencil;
    GC3Dint previousScissorState;
    GC3Dint previousDepthState;
    GC3Dint viewport[4];
    GC3Dint previousScissor[4];
    RefPtr<SharedGLData> m_sharedGLData;
    RefPtr<BitmapTexture> currentSurface;
};

void TextureMapperGL::ClipStack::init(const IntRect& rect)
{
    clipStack.clear();
    clipState = TextureMapperGL::ClipState(rect);
}

void TextureMapperGL::ClipStack::push()
{
    clipStack.append(clipState);
}

void TextureMapperGL::ClipStack::pop()
{
    if (clipStack.isEmpty())
        return;
    clipState = clipStack.last();
    clipStack.removeLast();
}

static void scissorClip(GraphicsContext3D* context, const IntRect& rect)
{
    if (rect.isEmpty())
        return;

    GC3Dint viewport[4];
    context->getIntegerv(GraphicsContext3D::VIEWPORT, viewport);
    context->scissor(rect.x(), viewport[3] - rect.maxY(), rect.width(), rect.height());
}

void TextureMapperGL::ClipStack::apply(GraphicsContext3D* context)
{
    scissorClip(context, clipState.scissorBox);
    context->stencilOp(GraphicsContext3D::KEEP, GraphicsContext3D::KEEP, GraphicsContext3D::KEEP);
    context->stencilFunc(GraphicsContext3D::EQUAL, clipState.stencilIndex - 1, clipState.stencilIndex - 1);
    if (clipState.stencilIndex == 1)
        context->disable(GraphicsContext3D::STENCIL_TEST);
    else
        context->enable(GraphicsContext3D::STENCIL_TEST);
}


void TextureMapperGLData::initializeStencil()
{
    if (currentSurface) {
        static_cast<BitmapTextureGL*>(currentSurface.get())->initializeStencil();
        return;
    }

    if (didModifyStencil)
        return;

    context->clearStencil(0);
    context->clear(GraphicsContext3D::STENCIL_BUFFER_BIT);
    didModifyStencil = true;
}

BitmapTextureGL* toBitmapTextureGL(BitmapTexture* texture)
{
    if (!texture || !texture->isBackedByOpenGL())
        return 0;

    return static_cast<BitmapTextureGL*>(texture);
}

TextureMapperGL::TextureMapperGL()
    : TextureMapper(OpenGLMode)
    , m_context(0)
    , m_enableEdgeDistanceAntialiasing(false)
{
    m_context3D = GraphicsContext3D::createForCurrentGLContext();
    m_data = new TextureMapperGLData(m_context3D.get());
}

TextureMapperGL::ClipStack& TextureMapperGL::clipStack()
{
    return data().currentSurface ? toBitmapTextureGL(data().currentSurface.get())->m_clipStack : m_clipStack;
}

void TextureMapperGL::beginPainting(PaintFlags flags)
{
    m_context3D->getIntegerv(GraphicsContext3D::CURRENT_PROGRAM, &data().previousProgram);
    data().previousScissorState = m_context3D->isEnabled(GraphicsContext3D::SCISSOR_TEST);
    data().previousDepthState = m_context3D->isEnabled(GraphicsContext3D::DEPTH_TEST);
#if PLATFORM(QT)
    if (m_context) {
        QPainter* painter = m_context->platformContext();
        painter->save();
        painter->beginNativePainting();
    }
#endif
    m_context3D->disable(GraphicsContext3D::DEPTH_TEST);
    m_context3D->enable(GraphicsContext3D::SCISSOR_TEST);
    data().didModifyStencil = false;
    m_context3D->depthMask(0);
    m_context3D->getIntegerv(GraphicsContext3D::VIEWPORT, data().viewport);
    m_context3D->getIntegerv(GraphicsContext3D::SCISSOR_BOX, data().previousScissor);
    m_clipStack.init(IntRect(0, 0, data().viewport[2], data().viewport[3]));
    m_context3D->getIntegerv(GraphicsContext3D::FRAMEBUFFER_BINDING, &data().targetFrameBuffer);
    data().PaintFlags = flags;
    bindSurface(0);
}

void TextureMapperGL::endPainting()
{
    if (data().didModifyStencil) {
        m_context3D->clearStencil(1);
        m_context3D->clear(GraphicsContext3D::STENCIL_BUFFER_BIT);
    }

    m_context3D->useProgram(data().previousProgram);

    m_context3D->scissor(data().previousScissor[0], data().previousScissor[1], data().previousScissor[2], data().previousScissor[3]);
    if (data().previousScissorState)
        m_context3D->enable(GraphicsContext3D::SCISSOR_TEST);
    else
        m_context3D->disable(GraphicsContext3D::SCISSOR_TEST);

    if (data().previousDepthState)
        m_context3D->enable(GraphicsContext3D::DEPTH_TEST);
    else
        m_context3D->disable(GraphicsContext3D::DEPTH_TEST);

#if PLATFORM(QT)
    if (!m_context)
        return;
    QPainter* painter = m_context->platformContext();
    painter->endNativePainting();
    painter->restore();
#endif
}

void TextureMapperGL::drawQuad(const DrawQuad& quadToDraw, const TransformationMatrix& modelViewMatrix, TextureMapperShaderProgram* shaderProgram, GC3Denum drawingMode, bool needsBlending)
{
    m_context3D->enableVertexAttribArray(shaderProgram->vertexAttrib());
    m_context3D->bindBuffer(GraphicsContext3D::ARRAY_BUFFER, 0);

    const GC3Dfloat quad[] = {
        quadToDraw.targetRectMappedToUnitSquare.p1().x(), quadToDraw.targetRectMappedToUnitSquare.p1().y(),
        quadToDraw.targetRectMappedToUnitSquare.p2().x(), quadToDraw.targetRectMappedToUnitSquare.p2().y(),
        quadToDraw.targetRectMappedToUnitSquare.p3().x(), quadToDraw.targetRectMappedToUnitSquare.p3().y(),
        quadToDraw.targetRectMappedToUnitSquare.p4().x(), quadToDraw.targetRectMappedToUnitSquare.p4().y()
    };
    m_context3D->vertexAttribPointer(shaderProgram->vertexAttrib(), 2, GraphicsContext3D::FLOAT, false, 0, GC3Dintptr(quad));

    TransformationMatrix matrix = TransformationMatrix(data().projectionMatrix).multiply(modelViewMatrix).multiply(TransformationMatrix(
            quadToDraw.originalTargetRect.width(), 0, 0, 0,
            0, quadToDraw.originalTargetRect.height(), 0, 0,
            0, 0, 1, 0,
            quadToDraw.originalTargetRect.x(), quadToDraw.originalTargetRect.y(), 0, 1));
    GC3Dfloat m4[] = {
        matrix.m11(), matrix.m12(), matrix.m13(), matrix.m14(),
        matrix.m21(), matrix.m22(), matrix.m23(), matrix.m24(),
        matrix.m31(), matrix.m32(), matrix.m33(), matrix.m34(),
        matrix.m41(), matrix.m42(), matrix.m43(), matrix.m44()
    };
    m_context3D->uniformMatrix4fv(shaderProgram->matrixLocation(), 1, false, m4);

    if (needsBlending) {
        m_context3D->blendFunc(GraphicsContext3D::ONE, GraphicsContext3D::ONE_MINUS_SRC_ALPHA);
        m_context3D->enable(GraphicsContext3D::BLEND);
    } else
        m_context3D->disable(GraphicsContext3D::BLEND);

    m_context3D->drawArrays(drawingMode, 0, 4);
    m_context3D->disableVertexAttribArray(shaderProgram->vertexAttrib());
}

void TextureMapperGL::drawBorder(const Color& color, float width, const FloatRect& targetRect, const TransformationMatrix& modelViewMatrix)
{
    if (clipStack().current().scissorBox.isEmpty())
        return;

    RefPtr<TextureMapperShaderProgramSolidColor> program = data().sharedGLData().textureMapperShaderManager.solidColorProgram();
    m_context3D->useProgram(program->id());

    float alpha = color.alpha() / 255.0;
    m_context3D->uniform4f(program->colorLocation(),
                       (color.red() / 255.0) * alpha,
                       (color.green() / 255.0) * alpha,
                       (color.blue() / 255.0) * alpha,
                       alpha);
    m_context3D->lineWidth(width);

    drawQuad(targetRect, modelViewMatrix, program.get(), GraphicsContext3D::LINE_LOOP, color.hasAlpha());
}

void TextureMapperGL::drawRepaintCounter(int value, int pointSize, const FloatPoint& targetPoint, const TransformationMatrix& modelViewMatrix)
{
#if PLATFORM(QT)
    QString counterString = QString::number(value);

    QFont font(QString::fromLatin1("Monospace"), pointSize, QFont::Bold);
    font.setStyleHint(QFont::TypeWriter);

    QFontMetrics fontMetrics(font);
    int width = fontMetrics.width(counterString) + 4;
    int height = fontMetrics.height();

    IntSize size(width, height);
    IntRect sourceRect(IntPoint::zero(), size);
    IntRect targetRect(roundedIntPoint(targetPoint), size);

    QImage image(size, NativeImageQt::defaultFormatForAlphaEnabledImages());
    QPainter painter(&image);
    painter.fillRect(sourceRect, Qt::blue); // Since we won't swap R+B for speed, this will paint red.
    painter.setFont(font);
    painter.setPen(Qt::white);
    painter.drawText(2, height * 0.85, counterString);

    RefPtr<BitmapTexture> texture = acquireTextureFromPool(size);
    const uchar* bits = image.bits();
    texture->updateContents(bits, sourceRect, IntPoint::zero(), image.bytesPerLine());
    drawTexture(*texture, targetRect, modelViewMatrix, 1.0f, 0, AllEdges);
#else
    notImplemented();
#endif
}

void TextureMapperGL::drawTexture(const BitmapTexture& texture, const FloatRect& targetRect, const TransformationMatrix& matrix, float opacity, const BitmapTexture* mask, unsigned exposedEdges)
{
    if (!texture.isValid())
        return;

    if (clipStack().current().scissorBox.isEmpty())
        return;

    const BitmapTextureGL& textureGL = static_cast<const BitmapTextureGL&>(texture);
    drawTexture(textureGL.id(), textureGL.isOpaque() ? 0 : SupportsBlending, textureGL.size(), targetRect, matrix, opacity, mask, exposedEdges);
}

#if !USE(TEXMAP_OPENGL_ES_2)
void TextureMapperGL::drawTextureRectangleARB(uint32_t texture, Flags flags, const IntSize& textureSize, const FloatRect& targetRect, const TransformationMatrix& modelViewMatrix, float opacity, const BitmapTexture* maskTexture)
{
    RefPtr<TextureMapperShaderProgram> program;
    if (maskTexture)
        program = data().sharedGLData().textureMapperShaderManager.getShaderProgram(TextureMapperShaderManager::RectOpacityAndMask);
    else
        program = data().sharedGLData().textureMapperShaderManager.getShaderProgram(TextureMapperShaderManager::RectSimple);
    m_context3D->useProgram(program->id());

    m_context3D->enableVertexAttribArray(program->vertexAttrib());
    m_context3D->activeTexture(GraphicsContext3D::TEXTURE0);
    m_context3D->bindTexture(GL_TEXTURE_RECTANGLE_ARB, texture);
    m_context3D->uniform1i(program->sourceTextureLocation(), 0);

    m_context3D->uniform1f(program->flipLocation(), !!(flags & ShouldFlipTexture));
    m_context3D->uniform2f(program->textureSizeLocation(), textureSize.width(), textureSize.height());

    if (TextureMapperShaderProgram::isValidUniformLocation(program->opacityLocation()))
        m_context3D->uniform1f(program->opacityLocation(), opacity);

    if (maskTexture && maskTexture->isValid() && TextureMapperShaderProgram::isValidUniformLocation(program->maskTextureLocation())) {
        const BitmapTextureGL* maskTextureGL = static_cast<const BitmapTextureGL*>(maskTexture);
        m_context3D->activeTexture(GraphicsContext3D::TEXTURE1);
        m_context3D->bindTexture(GraphicsContext3D::TEXTURE_2D, maskTextureGL->id());
        m_context3D->uniform1i(program->maskTextureLocation(), 1);
        m_context3D->activeTexture(GraphicsContext3D::TEXTURE0);
    }

    bool needsBlending = (flags & SupportsBlending) || opacity < 0.99 || maskTexture;
    drawQuad(targetRect, modelViewMatrix, program.get(), GraphicsContext3D::TRIANGLE_FAN, needsBlending);
}
#endif // !USE(TEXMAP_OPENGL_ES_2)

void TextureMapperGL::drawTexture(uint32_t texture, Flags flags, const IntSize& /* textureSize */, const FloatRect& targetRect, const TransformationMatrix& modelViewMatrix, float opacity, const BitmapTexture* maskTexture, unsigned exposedEdges)
{
    bool needsAntialiasing = m_enableEdgeDistanceAntialiasing && !modelViewMatrix.isIntegerTranslation();
    if (needsAntialiasing && drawTextureWithAntialiasing(texture, flags, targetRect, modelViewMatrix, opacity, maskTexture, exposedEdges))
       return;

    RefPtr<TextureMapperShaderProgram> program;
    if (maskTexture)
        program = data().sharedGLData().textureMapperShaderManager.getShaderProgram(TextureMapperShaderManager::OpacityAndMask);
    else
        program = data().sharedGLData().textureMapperShaderManager.getShaderProgram(TextureMapperShaderManager::Simple);
    m_context3D->useProgram(program->id());

    drawTexturedQuadWithProgram(program.get(), texture, flags, targetRect, modelViewMatrix, opacity, maskTexture);
}

static TransformationMatrix viewportMatrix(GraphicsContext3D* context3D)
{
    GC3Dint viewport[4];
    context3D->getIntegerv(GraphicsContext3D::VIEWPORT, viewport);

    TransformationMatrix matrix;
    matrix.translate3d(viewport[0], viewport[1], 0);
    matrix.scale3d(viewport[2], viewport[3], 0);

    // Map x, y and z to unit square from OpenGL normalized device
    // coordinates which are -1 to 1 on every axis.
    matrix.translate3d(0.5, 0.5, 0.5);
    matrix.scale3d(0.5, 0.5, 0.5);

    return matrix;
}

static void scaleLineEquationCoeffecientsToOptimizeDistanceCalculation(float* coeffecients)
{
    // In the fragment shader we want to calculate the distance from this
    // line to a point (p), which is given by the formula:
    // (A*p.x + B*p.y + C) / sqrt (a^2 + b^2)
    // We can do a small amount of precalculation here to reduce the
    // amount of math in the shader by scaling the coeffecients now.
    float scale = 1.0 / FloatPoint(coeffecients[0], coeffecients[1]).length();
    coeffecients[0] = coeffecients[0] * scale;
    coeffecients[1] = coeffecients[1] * scale;
    coeffecients[2] = coeffecients[2] * scale;
}

static void getStandardEquationCoeffecientsForLine(const FloatPoint& p1, const FloatPoint& p2, float* coeffecients)
{
    // Given two points, the standard equation of a line (Ax + By + C = 0)
    // can be calculated via the formula:
    // (p1.y – p2.y)x + (p1.x – p2.x)y + ((p1.x*p2.y) – (p2.x*p1.y)) = 0
    coeffecients[0] = p1.y() - p2.y();
    coeffecients[1] = p2.x() - p1.x();
    coeffecients[2] = p1.x() * p2.y() - p2.x() * p1.y();
    scaleLineEquationCoeffecientsToOptimizeDistanceCalculation(coeffecients);
}

static void quadToEdgeArray(const FloatQuad& quad, float* edgeArray)
{
    if (quad.isCounterclockwise()) {
        getStandardEquationCoeffecientsForLine(quad.p4(), quad.p3(), edgeArray);
        getStandardEquationCoeffecientsForLine(quad.p3(), quad.p2(), edgeArray + 3);
        getStandardEquationCoeffecientsForLine(quad.p2(), quad.p1(), edgeArray + 6);
        getStandardEquationCoeffecientsForLine(quad.p1(), quad.p4(), edgeArray + 9);
        return;
    }
    getStandardEquationCoeffecientsForLine(quad.p4(), quad.p1(), edgeArray);
    getStandardEquationCoeffecientsForLine(quad.p1(), quad.p2(), edgeArray + 3);
    getStandardEquationCoeffecientsForLine(quad.p2(), quad.p3(), edgeArray + 6);
    getStandardEquationCoeffecientsForLine(quad.p3(), quad.p4(), edgeArray + 9);
}

static FloatSize scaledVectorDifference(const FloatPoint& point1, const FloatPoint& point2, float scale)
{
    FloatSize vector = point1 - point2;
    if (vector.diagonalLengthSquared())
        vector.scale(1.0 / vector.diagonalLength());

    vector.scale(scale);
    return vector;
}

static FloatQuad inflateQuad(const FloatQuad& quad, float distance)
{
    FloatQuad expandedQuad = quad;
    expandedQuad.setP1(expandedQuad.p1() + scaledVectorDifference(quad.p1(), quad.p2(), distance));
    expandedQuad.setP4(expandedQuad.p4() + scaledVectorDifference(quad.p4(), quad.p3(), distance));

    expandedQuad.setP1(expandedQuad.p1() + scaledVectorDifference(quad.p1(), quad.p4(), distance));
    expandedQuad.setP2(expandedQuad.p2() + scaledVectorDifference(quad.p2(), quad.p3(), distance));

    expandedQuad.setP2(expandedQuad.p2() + scaledVectorDifference(quad.p2(), quad.p1(), distance));
    expandedQuad.setP3(expandedQuad.p3() + scaledVectorDifference(quad.p3(), quad.p4(), distance));

    expandedQuad.setP3(expandedQuad.p3() + scaledVectorDifference(quad.p3(), quad.p2(), distance));
    expandedQuad.setP4(expandedQuad.p4() + scaledVectorDifference(quad.p4(), quad.p1(), distance));

    return expandedQuad;
}

bool TextureMapperGL::drawTextureWithAntialiasing(uint32_t texture, Flags flags, const FloatRect& originalTargetRect, const TransformationMatrix& modelViewMatrix, float opacity, const BitmapTexture* maskTexture, unsigned exposedEdges)
{
    // The antialiasing path does not support mask textures at the moment.
    if (maskTexture)
        return false;

    // For now we punt on rendering tiled layers with antialiasing. It's quite hard
    // to render them without seams.
    if (exposedEdges != AllEdges)
        return false;

    // The goal here is render a slightly larger (0.75 pixels in screen space) quad and to
    // gradually taper off the alpha values to do a simple version of edge distance
    // antialiasing. Note here that we are also including the viewport matrix (which
    // translates from normalized device coordinates to screen coordinates), because these
    // values are consumed in the fragment shader, which works in screen coordinates.
    TransformationMatrix screenSpaceTransform = viewportMatrix(m_context3D.get()).multiply(TransformationMatrix(data().projectionMatrix)).multiply(modelViewMatrix).to2dTransform();
    if (!screenSpaceTransform.isInvertible())
        return false;
    FloatQuad quadInScreenSpace = screenSpaceTransform.mapQuad(originalTargetRect);

    const float inflationDistance = 0.75;
    FloatQuad expandedQuadInScreenSpace = inflateQuad(quadInScreenSpace, inflationDistance);

    // In the non-antialiased case the vertices passed are the unit rectangle and double
    // as the texture coordinates (0,0 1,0, 1,1 and 0,1). Here we map the expanded quad
    // coordinates in screen space back to the original rect's texture coordinates.
    // This has the effect of slightly increasing the size of the original quad's geometry
    // in the vertex shader.
    FloatQuad expandedQuadInTextureCoordinates = screenSpaceTransform.inverse().mapQuad(expandedQuadInScreenSpace);
    expandedQuadInTextureCoordinates.move(-originalTargetRect.x(), -originalTargetRect.y());
    expandedQuadInTextureCoordinates.scale(1 / originalTargetRect.width(), 1 / originalTargetRect.height());

    // We prepare both the expanded quad for the fragment shader as well as the rectangular bounding
    // box of that quad, as that seems necessary to properly antialias backfacing quads.
    float targetQuadEdges[24];
    quadToEdgeArray(expandedQuadInScreenSpace, targetQuadEdges);
    quadToEdgeArray(inflateQuad(quadInScreenSpace.boundingBox(),  inflationDistance), targetQuadEdges + 12);

    RefPtr<TextureMapperShaderProgramAntialiasingNoMask> program = data().sharedGLData().textureMapperShaderManager.antialiasingNoMaskProgram();
    m_context3D->useProgram(program->id());
    m_context3D->uniform3fv(program->expandedQuadEdgesInScreenSpaceLocation(), 8, targetQuadEdges);

    drawTexturedQuadWithProgram(program.get(), texture, flags, DrawQuad(originalTargetRect, expandedQuadInTextureCoordinates), modelViewMatrix, opacity, 0 /* maskTexture */);
    return true;
}

void TextureMapperGL::drawTexturedQuadWithProgram(TextureMapperShaderProgram* program, uint32_t texture, Flags flags, const DrawQuad& quadToDraw, const TransformationMatrix& modelViewMatrix, float opacity, const BitmapTexture* maskTexture)
{
    m_context3D->enableVertexAttribArray(program->vertexAttrib());
    m_context3D->activeTexture(GraphicsContext3D::TEXTURE0);
    m_context3D->bindTexture(GraphicsContext3D::TEXTURE_2D, texture);
    m_context3D->uniform1i(program->sourceTextureLocation(), 0);

    m_context3D->uniform1f(program->flipLocation(), !!(flags & ShouldFlipTexture));

    if (TextureMapperShaderProgram::isValidUniformLocation(program->opacityLocation()))
        m_context3D->uniform1f(program->opacityLocation(), opacity);

    if (maskTexture && maskTexture->isValid() && TextureMapperShaderProgram::isValidUniformLocation(program->maskTextureLocation())) {
        const BitmapTextureGL* maskTextureGL = static_cast<const BitmapTextureGL*>(maskTexture);
        m_context3D->activeTexture(GraphicsContext3D::TEXTURE1);
        m_context3D->bindTexture(GraphicsContext3D::TEXTURE_2D, maskTextureGL->id());
        m_context3D->uniform1i(program->maskTextureLocation(), 1);
        m_context3D->activeTexture(GraphicsContext3D::TEXTURE0);
    }

    bool needsBlending = (flags & SupportsBlending) || opacity < 0.99 || maskTexture;
    drawQuad(quadToDraw, modelViewMatrix, program, GraphicsContext3D::TRIANGLE_FAN, needsBlending);
}

bool BitmapTextureGL::canReuseWith(const IntSize& contentsSize, Flags)
{
    return contentsSize == m_textureSize;
}

#if OS(DARWIN)
#define DEFAULT_TEXTURE_PIXEL_TRANSFER_TYPE GL_UNSIGNED_INT_8_8_8_8_REV
#else
#define DEFAULT_TEXTURE_PIXEL_TRANSFER_TYPE GraphicsContext3D::UNSIGNED_BYTE
#endif

static void swizzleBGRAToRGBA(uint32_t* data, const IntRect& rect, int stride = 0)
{
    stride = stride ? stride : rect.width();
    for (int y = rect.y(); y < rect.maxY(); ++y) {
        uint32_t* p = data + y * stride;
        for (int x = rect.x(); x < rect.maxX(); ++x)
            p[x] = ((p[x] << 16) & 0xff0000) | ((p[x] >> 16) & 0xff) | (p[x] & 0xff00ff00);
    }
}

static bool driverSupportsBGRASwizzling()
{
#if defined(TEXMAP_OPENGL_ES_2)
    // FIXME: Implement reliable detection. See also https://bugs.webkit.org/show_bug.cgi?id=81103.
    return false;
#else
    return true;
#endif
}

static bool driverSupportsSubImage()
{
#if defined(TEXMAP_OPENGL_ES_2)
    // FIXME: Implement reliable detection.
    return false;
#else
    return true;
#endif
}

void BitmapTextureGL::didReset()
{
    GraphicsContext3D* context3D = m_textureMapper->graphicsContext3D();

    if (!m_id)
        m_id = context3D->createTexture();

    m_shouldClear = true;
    if (m_textureSize == contentSize())
        return;

    Platform3DObject format = driverSupportsBGRASwizzling() ? GraphicsContext3D::BGRA : GraphicsContext3D::RGBA;

    m_textureSize = contentSize();
    context3D->bindTexture(GraphicsContext3D::TEXTURE_2D, m_id);
    context3D->texParameteri(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_MIN_FILTER, GraphicsContext3D::LINEAR);
    context3D->texParameteri(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_MAG_FILTER, GraphicsContext3D::LINEAR);
    context3D->texParameteri(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_WRAP_S, GraphicsContext3D::CLAMP_TO_EDGE);
    context3D->texParameteri(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_WRAP_T, GraphicsContext3D::CLAMP_TO_EDGE);
    context3D->texImage2DDirect(GraphicsContext3D::TEXTURE_2D, 0, GraphicsContext3D::RGBA, m_textureSize.width(), m_textureSize.height(), 0, format, DEFAULT_TEXTURE_PIXEL_TRANSFER_TYPE, 0);
}


void BitmapTextureGL::updateContents(const void* data, const IntRect& targetRect, const IntPoint& sourceOffset, int bytesPerLine)
{
    GraphicsContext3D* context3D = m_textureMapper->graphicsContext3D();

    Platform3DObject glFormat = GraphicsContext3D::RGBA;
    context3D->bindTexture(GraphicsContext3D::TEXTURE_2D, m_id);

    const unsigned bytesPerPixel = 4;
    if (driverSupportsBGRASwizzling())
        glFormat = GraphicsContext3D::BGRA;
    else
        swizzleBGRAToRGBA(reinterpret_cast<uint32_t*>(const_cast<void*>(data)), IntRect(sourceOffset, targetRect.size()), bytesPerLine / bytesPerPixel);

    if (bytesPerLine == targetRect.width() / 4 && sourceOffset == IntPoint::zero()) {
        context3D->texSubImage2D(GraphicsContext3D::TEXTURE_2D, 0, targetRect.x(), targetRect.y(), targetRect.width(), targetRect.height(), glFormat, DEFAULT_TEXTURE_PIXEL_TRANSFER_TYPE, (const char*)data);
        return;
    }

    // For ES drivers that don't support sub-images.
    if (!driverSupportsSubImage()) {
        const char* bits = static_cast<const char*>(data);
        const char* src = bits + sourceOffset.y() * bytesPerLine + sourceOffset.x() * bytesPerPixel;
        Vector<char> temporaryData(targetRect.width() * targetRect.height() * bytesPerPixel);
        char* dst = temporaryData.data();

        const int targetBytesPerLine = targetRect.width() * bytesPerPixel;
        for (int y = 0; y < targetRect.height(); ++y) {
            memcpy(dst, src, targetBytesPerLine);
            src += bytesPerLine;
            dst += targetBytesPerLine;
        }

        context3D->texSubImage2D(GraphicsContext3D::TEXTURE_2D, 0, targetRect.x(), targetRect.y(), targetRect.width(), targetRect.height(), glFormat, DEFAULT_TEXTURE_PIXEL_TRANSFER_TYPE, temporaryData.data());
        return;
    }

#if !defined(TEXMAP_OPENGL_ES_2)
    // Use the OpenGL sub-image extension, now that we know it's available.
    context3D->pixelStorei(GL_UNPACK_ROW_LENGTH, bytesPerLine / bytesPerPixel);
    context3D->pixelStorei(GL_UNPACK_SKIP_ROWS, sourceOffset.y());
    context3D->pixelStorei(GL_UNPACK_SKIP_PIXELS, sourceOffset.x());
    context3D->texSubImage2D(GraphicsContext3D::TEXTURE_2D, 0, targetRect.x(), targetRect.y(), targetRect.width(), targetRect.height(), glFormat, DEFAULT_TEXTURE_PIXEL_TRANSFER_TYPE, (const char*)data);
    context3D->pixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    context3D->pixelStorei(GL_UNPACK_SKIP_ROWS, 0);
    context3D->pixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
#endif
}

void BitmapTextureGL::updateContents(Image* image, const IntRect& targetRect, const IntPoint& offset)
{
    if (!image)
        return;
    NativeImagePtr frameImage = image->nativeImageForCurrentFrame();
    if (!frameImage)
        return;

    int bytesPerLine;
    const char* imageData;

#if PLATFORM(QT)
    imageData = reinterpret_cast<const char*>(frameImage->constBits());
    bytesPerLine = frameImage->bytesPerLine();
#elif USE(CAIRO)
    cairo_surface_t* surface = frameImage->surface();
    imageData = reinterpret_cast<const char*>(cairo_image_surface_get_data(surface));
    bytesPerLine = cairo_image_surface_get_stride(surface);
#endif

    updateContents(imageData, targetRect, offset, bytesPerLine);
}

#if ENABLE(CSS_FILTERS)
void TextureMapperGL::drawFiltered(const BitmapTexture& sourceTexture, const BitmapTexture& contentTexture, const FilterOperation& filter, int pass)
{
    // For standard filters, we always draw the whole texture without transformations.
    RefPtr<StandardFilterProgram> program = data().sharedGLData().textureMapperShaderManager.getShaderForFilter(filter, pass);
    ASSERT(program);

    program->prepare(filter, pass, sourceTexture.contentSize(), static_cast<const BitmapTextureGL&>(contentTexture).id());

    m_context3D->enableVertexAttribArray(program->vertexAttrib());
    m_context3D->enableVertexAttribArray(program->texCoordAttrib());
    m_context3D->activeTexture(GraphicsContext3D::TEXTURE0);
    m_context3D->bindTexture(GraphicsContext3D::TEXTURE_2D, static_cast<const BitmapTextureGL&>(sourceTexture).id());
    m_context3D->uniform1i(program->textureUniform(), 0);
    const GC3Dfloat targetVertices[] = {-1, -1, 1, -1, 1, 1, -1, 1};
    const GC3Dfloat sourceVertices[] = {0, 0, 1, 0, 1, 1, 0, 1};
    m_context3D->vertexAttribPointer(program->vertexAttrib(), 2, GraphicsContext3D::FLOAT, false, 0, GC3Dintptr(targetVertices));
    m_context3D->vertexAttribPointer(program->texCoordAttrib(), 2, GraphicsContext3D::FLOAT, false, 0, GC3Dintptr(sourceVertices));
    m_context3D->disable(GraphicsContext3D::BLEND);
    m_context3D->drawArrays(GraphicsContext3D::TRIANGLE_FAN, 0, 4);
    m_context3D->disableVertexAttribArray(program->vertexAttrib());
    m_context3D->disableVertexAttribArray(program->texCoordAttrib());
}

PassRefPtr<BitmapTexture> BitmapTextureGL::applyFilters(const BitmapTexture& contentTexture, const FilterOperations& filters)
{
    RefPtr<BitmapTexture> previousSurface = m_textureMapper->data().currentSurface;

    RefPtr<BitmapTexture> source = this;
    RefPtr<BitmapTexture> target = m_textureMapper->acquireTextureFromPool(m_textureSize);
    for (int i = 0; i < filters.size(); ++i) {
        const FilterOperation* filter = filters.at(i);
        ASSERT(filter);

        int numPasses = m_textureMapper->data().sharedGLData().textureMapperShaderManager.getPassesRequiredForFilter(*filter);
        for (int j = 0; j < numPasses; ++j) {
            m_textureMapper->bindSurface(target.get());
            m_textureMapper->drawFiltered((i || j) ? *source : contentTexture, contentTexture, *filter, j);
            std::swap(source, target);
        }
    }

    m_textureMapper->bindSurface(previousSurface.get());
    return source;
}
#endif

static inline TransformationMatrix createProjectionMatrix(const IntSize& size, bool mirrored)
{
    const float nearValue = 9999999;
    const float farValue = -99999;

    return TransformationMatrix(2.0 / float(size.width()), 0, 0, 0,
                                0, (mirrored ? 2.0 : -2.0) / float(size.height()), 0, 0,
                                0, 0, -2.f / (farValue - nearValue), 0,
                                -1, mirrored ? -1 : 1, -(farValue + nearValue) / (farValue - nearValue), 1);
}

void BitmapTextureGL::initializeStencil()
{
    if (m_rbo)
        return;

    GraphicsContext3D* context3D = m_textureMapper->graphicsContext3D();
    m_rbo = context3D->createRenderbuffer();
    context3D->bindRenderbuffer(GraphicsContext3D::RENDERBUFFER, m_rbo);
#ifdef TEXMAP_OPENGL_ES_2
    context3D->renderbufferStorage(GraphicsContext3D::RENDERBUFFER, GraphicsContext3D::STENCIL_INDEX8, m_textureSize.width(), m_textureSize.height());
#else
    context3D->renderbufferStorage(GraphicsContext3D::RENDERBUFFER, GraphicsContext3D::DEPTH_STENCIL, m_textureSize.width(), m_textureSize.height());
#endif
    context3D->bindRenderbuffer(GraphicsContext3D::RENDERBUFFER, 0);
    context3D->framebufferRenderbuffer(GraphicsContext3D::FRAMEBUFFER, GraphicsContext3D::STENCIL_ATTACHMENT, GraphicsContext3D::RENDERBUFFER, m_rbo);
    context3D->clearStencil(0);
    context3D->clear(GraphicsContext3D::STENCIL_BUFFER_BIT);
}

void BitmapTextureGL::clearIfNeeded()
{
    if (!m_shouldClear)
        return;

    GraphicsContext3D* context3D = m_textureMapper->graphicsContext3D();

    m_clipStack.init(IntRect(IntPoint::zero(), m_textureSize));
    m_clipStack.apply(context3D);
    context3D->clearColor(0, 0, 0, 0);
    context3D->clear(GraphicsContext3D::COLOR_BUFFER_BIT);
    m_shouldClear = false;
}

void BitmapTextureGL::createFboIfNeeded()
{
    if (m_fbo)
        return;

    GraphicsContext3D* context3D = m_textureMapper->graphicsContext3D();
    m_fbo = context3D->createFramebuffer();
    context3D->bindFramebuffer(GraphicsContext3D::FRAMEBUFFER, m_fbo);
    context3D->framebufferTexture2D(GraphicsContext3D::FRAMEBUFFER, GraphicsContext3D::COLOR_ATTACHMENT0, GraphicsContext3D::TEXTURE_2D, id(), 0);
    m_shouldClear = true;
}

void BitmapTextureGL::bind()
{
    GraphicsContext3D* context3D = m_textureMapper->graphicsContext3D();
    context3D->bindTexture(GraphicsContext3D::TEXTURE_2D, 0);
    createFboIfNeeded();
    context3D->bindFramebuffer(GraphicsContext3D::FRAMEBUFFER, m_fbo);
    context3D->viewport(0, 0, m_textureSize.width(), m_textureSize.height());
    clearIfNeeded();
    m_textureMapper->data().projectionMatrix = createProjectionMatrix(m_textureSize, true /* mirrored */);
    m_clipStack.apply(context3D);
}

BitmapTextureGL::~BitmapTextureGL()
{
    GraphicsContext3D* context3D = m_textureMapper->graphicsContext3D();

    if (m_id)
        context3D->deleteTexture(m_id);

    if (m_fbo)
        context3D->deleteFramebuffer(m_fbo);

    if (m_rbo)
        context3D->deleteRenderbuffer(m_rbo);
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

void TextureMapperGL::bindDefaultSurface()
{
    m_context3D->bindFramebuffer(GraphicsContext3D::FRAMEBUFFER, data().targetFrameBuffer);
    IntSize viewportSize(data().viewport[2], data().viewport[3]);
    data().projectionMatrix = createProjectionMatrix(viewportSize, data().PaintFlags & PaintingMirrored);
    m_context3D->viewport(data().viewport[0], data().viewport[1], viewportSize.width(), viewportSize.height());
    m_clipStack.apply(m_context3D.get());
    data().currentSurface.clear();
}

void TextureMapperGL::bindSurface(BitmapTexture *surface)
{
    if (!surface) {
        bindDefaultSurface();
        return;
    }

    static_cast<BitmapTextureGL*>(surface)->bind();
    data().currentSurface = surface;
}

bool TextureMapperGL::beginScissorClip(const TransformationMatrix& modelViewMatrix, const FloatRect& targetRect)
{
    // 3D transforms are currently not supported in scissor clipping
    // resulting in cropped surfaces when z>0.
    if (!modelViewMatrix.isAffine())
        return false;

    FloatQuad quad = modelViewMatrix.projectQuad(targetRect);
    IntRect rect = quad.enclosingBoundingBox();

    // Only use scissors on rectilinear clips.
    if (!quad.isRectilinear() || rect.isEmpty())
        return false;

    clipStack().current().scissorBox.intersect(rect);
    clipStack().apply(m_context3D.get());
    return true;
}

void TextureMapperGL::beginClip(const TransformationMatrix& modelViewMatrix, const FloatRect& targetRect)
{
    clipStack().push();
    if (beginScissorClip(modelViewMatrix, targetRect))
        return;

    data().initializeStencil();

    RefPtr<TextureMapperShaderProgram> program = data().sharedGLData().textureMapperShaderManager.getShaderProgram(TextureMapperShaderManager::Simple);

    m_context3D->useProgram(program->id());
    m_context3D->enableVertexAttribArray(program->vertexAttrib());
    const GC3Dfloat unitRect[] = {0, 0, 1, 0, 1, 1, 0, 1};
    m_context3D->vertexAttribPointer(program->vertexAttrib(), 2, GraphicsContext3D::FLOAT, false, 0, GC3Dintptr(unitRect));

    TransformationMatrix matrix = TransformationMatrix(data().projectionMatrix)
            .multiply(modelViewMatrix)
            .multiply(TransformationMatrix(targetRect.width(), 0, 0, 0,
                0, targetRect.height(), 0, 0,
                0, 0, 1, 0,
                targetRect.x(), targetRect.y(), 0, 1));

    const GC3Dfloat m4[] = {
        matrix.m11(), matrix.m12(), matrix.m13(), matrix.m14(),
        matrix.m21(), matrix.m22(), matrix.m23(), matrix.m24(),
        matrix.m31(), matrix.m32(), matrix.m33(), matrix.m34(),
        matrix.m41(), matrix.m42(), matrix.m43(), matrix.m44()
    };

    const GC3Dfloat m4all[] = {
        2, 0, 0, 0,
        0, 2, 0, 0,
        0, 0, 1, 0,
        -1, -1, 0, 1
    };

    int& stencilIndex = clipStack().current().stencilIndex;

    m_context3D->enable(GraphicsContext3D::STENCIL_TEST);

    // Make sure we don't do any actual drawing.
    m_context3D->stencilFunc(GraphicsContext3D::NEVER, stencilIndex, stencilIndex);

    // Operate only on the stencilIndex and above.
    m_context3D->stencilMask(0xff & ~(stencilIndex - 1));

    // First clear the entire buffer at the current index.
    m_context3D->uniformMatrix4fv(program->matrixLocation(), 1, false, const_cast<GC3Dfloat*>(m4all));
    m_context3D->stencilOp(GraphicsContext3D::ZERO, GraphicsContext3D::ZERO, GraphicsContext3D::ZERO);
    m_context3D->drawArrays(GraphicsContext3D::TRIANGLE_FAN, 0, 4);

    // Now apply the current index to the new quad.
    m_context3D->stencilOp(GraphicsContext3D::REPLACE, GraphicsContext3D::REPLACE, GraphicsContext3D::REPLACE);
    m_context3D->uniformMatrix4fv(program->matrixLocation(), 1, false, const_cast<GC3Dfloat*>(m4));
    m_context3D->drawArrays(GraphicsContext3D::TRIANGLE_FAN, 0, 4);

    // Clear the state.
    m_context3D->disableVertexAttribArray(program->vertexAttrib());
    m_context3D->stencilMask(0);

    // Increase stencilIndex and apply stencil testing.
    stencilIndex *= 2;
    clipStack().apply(m_context3D.get());
}

void TextureMapperGL::endClip()
{
    clipStack().pop();
    clipStack().apply(m_context3D.get());
}

PassRefPtr<BitmapTexture> TextureMapperGL::createTexture()
{
    BitmapTextureGL* texture = new BitmapTextureGL(this);
    return adoptRef(texture);
}

PassOwnPtr<TextureMapper> TextureMapper::platformCreateAccelerated()
{
    return TextureMapperGL::create();
}

};
