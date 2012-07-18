/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include "config.h"

#if USE(ACCELERATED_COMPOSITING)
#include "LayerRendererChromium.h"

#include "Extensions3D.h"
#include "FloatQuad.h"
#include "GeometryBinding.h"
#include "GrTexture.h"
#include "NotImplemented.h"
#include "PlatformColor.h"
#include "SharedGraphicsContext3D.h"
#include "SkBitmap.h"
#include "SkColor.h"
#include "ThrottledTextureUploader.h"
#include "TraceEvent.h"
#include "TrackingTextureAllocator.h"
#include "cc/CCDamageTracker.h"
#include "cc/CCLayerQuad.h"
#include "cc/CCMathUtil.h"
#include "cc/CCProxy.h"
#include "cc/CCRenderPass.h"
#include "cc/CCRenderSurfaceFilters.h"
#include "cc/CCScopedTexture.h"
#include "cc/CCSettings.h"
#include "cc/CCSingleThreadProxy.h"
#include "cc/CCVideoLayerImpl.h"
#include <public/WebGraphicsContext3D.h>
#include <public/WebVideoFrame.h>
#include <wtf/CurrentTime.h>
#include <wtf/MainThread.h>
#include <wtf/text/StringHash.h>

using namespace std;
using WebKit::WebGraphicsContext3D;
using WebKit::WebGraphicsMemoryAllocation;
using WebKit::WebTransformationMatrix;

namespace WebCore {

namespace {

static WebTransformationMatrix orthoMatrix(float left, float right, float bottom, float top)
{
    float deltaX = right - left;
    float deltaY = top - bottom;
    WebTransformationMatrix ortho;
    if (!deltaX || !deltaY)
        return ortho;
    ortho.setM11(2.0f / deltaX);
    ortho.setM41(-(right + left) / deltaX);
    ortho.setM22(2.0f / deltaY);
    ortho.setM42(-(top + bottom) / deltaY);

    // Z component of vertices is always set to zero as we don't use the depth buffer
    // while drawing.
    ortho.setM33(0);

    return ortho;
}

static WebTransformationMatrix screenMatrix(int x, int y, int width, int height)
{
    WebTransformationMatrix screen;

    // Map to viewport.
    screen.translate3d(x, y, 0);
    screen.scale3d(width, height, 0);

    // Map x, y and z to unit square.
    screen.translate3d(0.5, 0.5, 0.5);
    screen.scale3d(0.5, 0.5, 0.5);

    return screen;
}

bool needsIOSurfaceReadbackWorkaround()
{
#if OS(DARWIN)
    return true;
#else
    return false;
#endif
}

class UnthrottledTextureUploader : public TextureUploader {
    WTF_MAKE_NONCOPYABLE(UnthrottledTextureUploader);
public:
    static PassOwnPtr<UnthrottledTextureUploader> create()
    {
        return adoptPtr(new UnthrottledTextureUploader());
    }
    virtual ~UnthrottledTextureUploader() { }

    virtual bool isBusy() OVERRIDE { return false; }
    virtual void beginUploads() OVERRIDE { }
    virtual void endUploads() OVERRIDE { }
    virtual void uploadTexture(CCGraphicsContext* context, LayerTextureUpdater::Texture* texture, TextureAllocator* allocator, const IntRect sourceRect, const IntRect destRect) OVERRIDE { texture->updateRect(context, allocator, sourceRect, destRect); }

protected:
    UnthrottledTextureUploader() { }
};

} // anonymous namespace

PassOwnPtr<LayerRendererChromium> LayerRendererChromium::create(CCRendererClient* client, WebGraphicsContext3D* context, TextureUploaderOption textureUploaderSetting)
{
    OwnPtr<LayerRendererChromium> layerRenderer(adoptPtr(new LayerRendererChromium(client, context, textureUploaderSetting)));
    if (!layerRenderer->initialize())
        return nullptr;

    return layerRenderer.release();
}

LayerRendererChromium::LayerRendererChromium(CCRendererClient* client,
                                             WebGraphicsContext3D* context,
                                             TextureUploaderOption textureUploaderSetting)
    : CCRenderer(client)
    , m_currentRenderPass(0)
    , m_currentTexture(0)
    , m_offscreenFramebufferId(0)
    , m_sharedGeometryQuad(FloatRect(-0.5f, -0.5f, 1.0f, 1.0f))
    , m_context(context)
    , m_defaultRenderPass(0)
    , m_isViewportChanged(false)
    , m_isFramebufferDiscarded(false)
    , m_isUsingBindUniform(false)
    , m_visible(true)
    , m_textureUploaderSetting(textureUploaderSetting)
{
    ASSERT(m_context);
}

bool LayerRendererChromium::initialize()
{
    if (!m_context->makeContextCurrent())
        return false;

    m_context->setContextLostCallback(this);

    String extensionsString = m_context->getString(GraphicsContext3D::EXTENSIONS);
    Vector<String> extensionsList;
    extensionsString.split(' ', extensionsList);
    HashSet<String> extensions;
    for (size_t i = 0; i < extensionsList.size(); ++i)
        extensions.add(extensionsList[i]);

    if (settings().acceleratePainting && extensions.contains("GL_EXT_texture_format_BGRA8888")
                                      && extensions.contains("GL_EXT_read_format_bgra"))
        m_capabilities.usingAcceleratedPainting = true;
    else
        m_capabilities.usingAcceleratedPainting = false;


    m_capabilities.contextHasCachedFrontBuffer = extensions.contains("GL_CHROMIUM_front_buffer_cached");

    m_capabilities.usingPartialSwap = CCSettings::partialSwapEnabled() && extensions.contains("GL_CHROMIUM_post_sub_buffer");

    m_capabilities.usingMapSub = extensions.contains("GL_CHROMIUM_map_sub");

    // Use the swapBuffers callback only with the threaded proxy.
    if (CCProxy::hasImplThread())
        m_capabilities.usingSwapCompleteCallback = extensions.contains("GL_CHROMIUM_swapbuffers_complete_callback");
    if (m_capabilities.usingSwapCompleteCallback) {
        m_context->setSwapBuffersCompleteCallbackCHROMIUM(this);
    }

    m_capabilities.usingSetVisibility = extensions.contains("GL_CHROMIUM_set_visibility");

    if (extensions.contains("GL_CHROMIUM_iosurface")) {
        ASSERT(extensions.contains("GL_ARB_texture_rectangle"));
    }

    m_capabilities.usingTextureUsageHint = extensions.contains("GL_ANGLE_texture_usage");

    m_capabilities.usingTextureStorageExtension = extensions.contains("GL_EXT_texture_storage");

    m_capabilities.usingGpuMemoryManager = extensions.contains("GL_CHROMIUM_gpu_memory_manager");
    if (m_capabilities.usingGpuMemoryManager)
        m_context->setMemoryAllocationChangedCallbackCHROMIUM(this);

    m_capabilities.usingDiscardFramebuffer = extensions.contains("GL_CHROMIUM_discard_framebuffer");

    m_capabilities.usingEglImage = extensions.contains("GL_OES_EGL_image_external");

    GLC(m_context, m_context->getIntegerv(GraphicsContext3D::MAX_TEXTURE_SIZE, &m_capabilities.maxTextureSize));
    m_capabilities.bestTextureFormat = PlatformColor::bestTextureFormat(m_context, extensions.contains("GL_EXT_texture_format_BGRA8888"));

    m_isUsingBindUniform = extensions.contains("GL_CHROMIUM_bind_uniform_location");

    if (!initializeSharedObjects())
        return false;

    // Make sure the viewport and context gets initialized, even if it is to zero.
    viewportChanged();
    return true;
}

LayerRendererChromium::~LayerRendererChromium()
{
    ASSERT(CCProxy::isImplThread());
    m_context->setSwapBuffersCompleteCallbackCHROMIUM(0);
    m_context->setMemoryAllocationChangedCallbackCHROMIUM(0);
    m_context->setContextLostCallback(0);
    cleanupSharedObjects();
}

WebGraphicsContext3D* LayerRendererChromium::context()
{
    return m_context;
}

void LayerRendererChromium::debugGLCall(WebGraphicsContext3D* context, const char* command, const char* file, int line)
{
    unsigned long error = context->getError();
    if (error != GraphicsContext3D::NO_ERROR)
        LOG_ERROR("GL command failed: File: %s\n\tLine %d\n\tcommand: %s, error %x\n", file, line, command, static_cast<int>(error));
}

void LayerRendererChromium::setVisible(bool visible)
{
    if (m_visible == visible)
        return;
    m_visible = visible;

    // TODO: Replace setVisibilityCHROMIUM with an extension to explicitly manage front/backbuffers
    // crbug.com/116049
    if (m_capabilities.usingSetVisibility) {
        m_context->setVisibilityCHROMIUM(visible);
    }
}

void LayerRendererChromium::releaseRenderPassTextures()
{
    m_renderPassTextures.clear();
}

void LayerRendererChromium::viewportChanged()
{
    m_isViewportChanged = true;

    // Reset the current RenderPass to force an update of the viewport and
    // projection matrix next time useRenderPass is called.
    m_currentRenderPass = 0;
}

void LayerRendererChromium::clearRenderPass(const CCRenderPass* renderPass, const FloatRect& framebufferDamageRect)
{
    // On DEBUG builds, opaque render passes are cleared to blue to easily see regions that were not drawn on the screen. If we
    // are using partial swap / scissor optimization, then the surface should only
    // clear the damaged region, so that we don't accidentally clear un-changed portions
    // of the screen.

    if (renderPass->hasTransparentBackground())
        GLC(m_context, m_context->clearColor(0, 0, 0, 0));
    else
        GLC(m_context, m_context->clearColor(0, 0, 1, 1));

    if (m_capabilities.usingPartialSwap)
        setScissorToRect(enclosingIntRect(framebufferDamageRect));
    else
        GLC(m_context, m_context->disable(GraphicsContext3D::SCISSOR_TEST));

#if defined(NDEBUG)
    if (renderPass->hasTransparentBackground())
#endif
        m_context->clear(GraphicsContext3D::COLOR_BUFFER_BIT);

    GLC(m_context, m_context->enable(GraphicsContext3D::SCISSOR_TEST));
}

// static
IntSize LayerRendererChromium::renderPassTextureSize(const CCRenderPass* pass)
{
    return pass->framebufferOutputRect().size();
}

// static
GC3Denum LayerRendererChromium::renderPassTextureFormat(const CCRenderPass*)
{
    return GraphicsContext3D::RGBA;
}

void LayerRendererChromium::decideRenderPassAllocationsForFrame(const CCRenderPassList& renderPassesInDrawOrder)
{
    HashMap<int, const CCRenderPass*> passesInFrame;
    for (size_t i = 0; i < renderPassesInDrawOrder.size(); ++i)
        passesInFrame.set(renderPassesInDrawOrder[i]->id(), renderPassesInDrawOrder[i]);

    Vector<int> passesToDelete;
    HashMap<int, OwnPtr<CCScopedTexture> >::const_iterator passIterator;
    for (passIterator = m_renderPassTextures.begin(); passIterator != m_renderPassTextures.end(); ++passIterator) {
        const CCRenderPass* renderPassInFrame = passesInFrame.get(passIterator->first);
        if (!renderPassInFrame) {
            passesToDelete.append(passIterator->first);
            continue;
        }

        const IntSize& requiredSize = renderPassTextureSize(renderPassInFrame);
        GC3Denum requiredFormat = renderPassTextureFormat(renderPassInFrame);
        CCScopedTexture* texture = passIterator->second.get();
        ASSERT(texture);

        if (texture->id() && (texture->size() != requiredSize || texture->format() != requiredFormat))
            texture->free();
    }

    // Delete RenderPass textures from the previous frame that will not be used again.
    for (size_t i = 0; i < passesToDelete.size(); ++i)
        m_renderPassTextures.remove(passesToDelete[i]);

    for (size_t i = 0; i < renderPassesInDrawOrder.size(); ++i) {
        if (!m_renderPassTextures.contains(renderPassesInDrawOrder[i]->id())) {
            OwnPtr<CCScopedTexture> texture = CCScopedTexture::create(m_implTextureAllocator.get());
            m_renderPassTextures.set(renderPassesInDrawOrder[i]->id(), texture.release());
        }
    }
}

bool LayerRendererChromium::haveCachedResourcesForRenderPassId(int id) const
{
    CCScopedTexture* texture = m_renderPassTextures.get(id);
    return texture && texture->id();
}

void LayerRendererChromium::drawFrame(const CCRenderPassList& renderPasses, const FloatRect& rootScissorRect)
{
    const CCRenderPass* rootRenderPass = renderPasses.last();
    beginDrawingFrame(rootRenderPass);

    for (size_t i = 0; i < renderPasses.size(); ++i) {
        const CCRenderPass* renderPass = renderPasses[i];

        FloatRect rootScissorRectInCurrentSurface = renderPass->targetSurface()->computeRootScissorRectInCurrentSurface(rootScissorRect);
        drawRenderPass(renderPass, rootScissorRectInCurrentSurface);
    }
}

void LayerRendererChromium::beginDrawingFrame(const CCRenderPass* rootRenderPass)
{
    // FIXME: Remove this once framebuffer is automatically recreated on first use
    ensureFramebuffer();

    m_defaultRenderPass = rootRenderPass;
    ASSERT(m_defaultRenderPass);

    if (viewportSize().isEmpty())
        return;

    TRACE_EVENT0("cc", "LayerRendererChromium::drawLayers");
    if (m_isViewportChanged) {
        // Only reshape when we know we are going to draw. Otherwise, the reshape
        // can leave the window at the wrong size if we never draw and the proper
        // viewport size is never set.
        m_isViewportChanged = false;
        m_context->reshape(viewportWidth(), viewportHeight());
    }

    makeContextCurrent();
    // Bind the common vertex attributes used for drawing all the layers.
    m_sharedGeometry->prepareForDraw();

    GLC(m_context, m_context->disable(GraphicsContext3D::DEPTH_TEST));
    GLC(m_context, m_context->disable(GraphicsContext3D::CULL_FACE));
    GLC(m_context, m_context->colorMask(true, true, true, true));
    GLC(m_context, m_context->enable(GraphicsContext3D::BLEND));
    GLC(m_context, m_context->blendFunc(GraphicsContext3D::ONE, GraphicsContext3D::ONE_MINUS_SRC_ALPHA));
}

void LayerRendererChromium::doNoOp()
{
    GLC(m_context, m_context->bindFramebuffer(GraphicsContext3D::FRAMEBUFFER, 0));
    GLC(m_context, m_context->flush());
}

void LayerRendererChromium::drawRenderPass(const CCRenderPass* renderPass, const FloatRect& framebufferDamageRect)
{
    if (!useRenderPass(renderPass))
        return;

    clearRenderPass(renderPass, framebufferDamageRect);

    const CCQuadList& quadList = renderPass->quadList();
    for (CCQuadList::constBackToFrontIterator it = quadList.backToFrontBegin(); it != quadList.backToFrontEnd(); ++it)
        drawQuad(it->get());
}

void LayerRendererChromium::drawQuad(const WebKit::WebCompositorQuad* quad)
{
    IntRect scissorRect = quad->scissorRect();

    ASSERT(!scissorRect.isEmpty());
    if (scissorRect.isEmpty())
        return;

    setScissorToRect(scissorRect);

    if (quad->needsBlending())
        GLC(m_context, m_context->enable(GraphicsContext3D::BLEND));
    else
        GLC(m_context, m_context->disable(GraphicsContext3D::BLEND));

    switch (quad->material()) {
    case WebKit::WebCompositorQuad::Invalid:
        ASSERT_NOT_REACHED();
        break;
    case WebKit::WebCompositorQuad::Checkerboard:
        drawCheckerboardQuad(CCCheckerboardDrawQuad::materialCast(quad));
        break;
    case WebKit::WebCompositorQuad::DebugBorder:
        drawDebugBorderQuad(CCDebugBorderDrawQuad::materialCast(quad));
        break;
    case WebKit::WebCompositorQuad::IOSurfaceContent:
        drawIOSurfaceQuad(CCIOSurfaceDrawQuad::materialCast(quad));
        break;
    case WebKit::WebCompositorQuad::RenderPass:
        drawRenderPassQuad(CCRenderPassDrawQuad::materialCast(quad));
        break;
    case WebKit::WebCompositorQuad::SolidColor:
        drawSolidColorQuad(WebKit::WebCompositorSolidColorQuad::materialCast(quad));
        break;
    case WebKit::WebCompositorQuad::StreamVideoContent:
        drawStreamVideoQuad(CCStreamVideoDrawQuad::materialCast(quad));
        break;
    case WebKit::WebCompositorQuad::TextureContent:
        drawTextureQuad(WebKit::WebCompositorTextureQuad::materialCast(quad));
        break;
    case WebKit::WebCompositorQuad::TiledContent:
        drawTileQuad(CCTileDrawQuad::materialCast(quad));
        break;
    case WebKit::WebCompositorQuad::YUVVideoContent:
        drawYUVVideoQuad(CCYUVVideoDrawQuad::materialCast(quad));
        break;
    }
}

void LayerRendererChromium::drawCheckerboardQuad(const CCCheckerboardDrawQuad* quad)
{
    const TileCheckerboardProgram* program = tileCheckerboardProgram();
    ASSERT(program && program->initialized());
    GLC(context(), context()->useProgram(program->program()));

    IntRect tileRect = quad->quadRect();
    WebTransformationMatrix tileTransform = quad->quadTransform();
    tileTransform.translate(tileRect.x() + tileRect.width() / 2.0, tileRect.y() + tileRect.height() / 2.0);

    float texOffsetX = tileRect.x();
    float texOffsetY = tileRect.y();
    float texScaleX = tileRect.width();
    float texScaleY = tileRect.height();
    GLC(context(), context()->uniform4f(program->fragmentShader().texTransformLocation(), texOffsetX, texOffsetY, texScaleX, texScaleY));

    const int checkerboardWidth = 16;
    float frequency = 1.0 / checkerboardWidth;

    GLC(context(), context()->uniform1f(program->fragmentShader().frequencyLocation(), frequency));

    float opacity = quad->opacity();
    drawTexturedQuad(tileTransform,
                     tileRect.width(), tileRect.height(), opacity, FloatQuad(),
                     program->vertexShader().matrixLocation(),
                     program->fragmentShader().alphaLocation(), -1);
}

void LayerRendererChromium::drawDebugBorderQuad(const CCDebugBorderDrawQuad* quad)
{
    static float glMatrix[16];
    const SolidColorProgram* program = solidColorProgram();
    ASSERT(program && program->initialized());
    GLC(context(), context()->useProgram(program->program()));

    const IntRect& layerRect = quad->quadRect();
    WebTransformationMatrix renderMatrix = quad->quadTransform();
    renderMatrix.translate(0.5 * layerRect.width() + layerRect.x(), 0.5 * layerRect.height() + layerRect.y());
    renderMatrix.scaleNonUniform(layerRect.width(), layerRect.height());
    LayerRendererChromium::toGLMatrix(&glMatrix[0], projectionMatrix() * renderMatrix);
    GLC(context(), context()->uniformMatrix4fv(program->vertexShader().matrixLocation(), 1, false, &glMatrix[0]));

    SkColor color = quad->color();
    float alpha = SkColorGetA(color) / 255.0;

    GLC(context(), context()->uniform4f(program->fragmentShader().colorLocation(), (SkColorGetR(color) / 255.0) * alpha, (SkColorGetG(color) / 255.0) * alpha, (SkColorGetB(color) / 255.0) * alpha, alpha));

    GLC(context(), context()->lineWidth(quad->width()));

    // The indices for the line are stored in the same array as the triangle indices.
    GLC(context(), context()->drawElements(GraphicsContext3D::LINE_LOOP, 4, GraphicsContext3D::UNSIGNED_SHORT, 6 * sizeof(unsigned short)));
}

static inline SkBitmap applyFilters(LayerRendererChromium* layerRenderer, const WebKit::WebFilterOperations& filters, CCScopedTexture* sourceTexture)
{
    if (filters.isEmpty())
        return SkBitmap();

    RefPtr<GraphicsContext3D> filterContext = CCProxy::hasImplThread() ? SharedGraphicsContext3D::getForImplThread() : SharedGraphicsContext3D::get();
    if (!filterContext)
        return SkBitmap();

    layerRenderer->context()->flush();

    return CCRenderSurfaceFilters::apply(filters, sourceTexture->id(), sourceTexture->size(), filterContext.get());
}

PassOwnPtr<CCScopedTexture> LayerRendererChromium::drawBackgroundFilters(const CCRenderPassDrawQuad* quad, const WebTransformationMatrix& contentsDeviceTransform)
{
    // This method draws a background filter, which applies a filter to any pixels behind the quad and seen through its background.
    // The algorithm works as follows:
    // 1. Compute a bounding box around the pixels that will be visible through the quad.
    // 2. Read the pixels in the bounding box into a buffer R.
    // 3. Apply the background filter to R, so that it is applied in the pixels' coordinate space.
    // 4. Apply the quad's inverse transform to map the pixels in R into the quad's content space. This implicitly
    // clips R by the content bounds of the quad since the destination texture has bounds matching the quad's content.
    // 5. Draw the background texture for the contents using the same transform as used to draw the contents itself. This is done
    // without blending to replace the current background pixels with the new filtered background.
    // 6. Draw the contents of the quad over drop of the new background with blending, as per usual. The filtered background
    // pixels will show through any non-opaque pixels in this draws.
    //
    // Pixel copies in this algorithm occur at steps 2, 3, 4, and 5.

    // FIXME: When this algorithm changes, update CCLayerTreeHost::prioritizeTextures() accordingly.

    if (quad->backgroundFilters().isEmpty())
        return nullptr;

    // FIXME: We only allow background filters on an opaque render surface because other surfaces may contain
    // translucent pixels, and the contents behind those translucent pixels wouldn't have the filter applied.
    if (m_currentRenderPass->hasTransparentBackground())
        return nullptr;
    ASSERT(!m_currentTexture);

    // FIXME: Do a single readback for both the surface and replica and cache the filtered results (once filter textures are not reused).
    IntRect deviceRect = enclosingIntRect(CCMathUtil::mapClippedRect(contentsDeviceTransform, sharedGeometryQuad().boundingBox()));

    int top, right, bottom, left;
    quad->backgroundFilters().getOutsets(top, right, bottom, left);
    deviceRect.move(-left, -top);
    deviceRect.expand(left + right, top + bottom);

    deviceRect.intersect(m_currentRenderPass->framebufferOutputRect());

    OwnPtr<CCScopedTexture> deviceBackgroundTexture = CCScopedTexture::create(m_implTextureAllocator.get());
    if (!getFramebufferTexture(deviceBackgroundTexture.get(), deviceRect))
        return nullptr;

    SkBitmap filteredDeviceBackground = applyFilters(this, quad->backgroundFilters(), deviceBackgroundTexture.get());
    if (!filteredDeviceBackground.getTexture())
        return nullptr;

    GrTexture* texture = reinterpret_cast<GrTexture*>(filteredDeviceBackground.getTexture());
    int filteredDeviceBackgroundTextureId = texture->getTextureHandle();

    OwnPtr<CCScopedTexture> backgroundTexture = CCScopedTexture::create(m_implTextureAllocator.get());
    if (!backgroundTexture->allocate(quad->quadRect().size(), GraphicsContext3D::RGBA))
        return nullptr;

    const CCRenderPass* targetRenderPass = m_currentRenderPass;
    bool usingBackgroundTexture = useScopedTexture(backgroundTexture.get(), quad->quadRect());

    if (usingBackgroundTexture) {
        // Copy the readback pixels from device to the background texture for the surface.
        WebTransformationMatrix deviceToFramebufferTransform;
        deviceToFramebufferTransform.translate(quad->quadRect().width() / 2.0, quad->quadRect().height() / 2.0);
        deviceToFramebufferTransform.scale3d(quad->quadRect().width(), quad->quadRect().height(), 1);
        deviceToFramebufferTransform.multiply(contentsDeviceTransform.inverse());
        deviceToFramebufferTransform.translate(deviceRect.width() / 2.0, deviceRect.height() / 2.0);
        deviceToFramebufferTransform.translate(deviceRect.x(), deviceRect.y());

        copyTextureToFramebuffer(filteredDeviceBackgroundTextureId, deviceRect.size(), deviceToFramebufferTransform);
    }

    useRenderPass(targetRenderPass);

    if (!usingBackgroundTexture)
        return nullptr;
    return backgroundTexture.release();
}

void LayerRendererChromium::drawRenderPassQuad(const CCRenderPassDrawQuad* quad)
{
    CCScopedTexture* contentsTexture = m_renderPassTextures.get(quad->renderPassId());
    if (!contentsTexture || !contentsTexture->id())
        return;

    WebTransformationMatrix renderTransform = quad->drawTransform();
    // Apply a scaling factor to size the quad from 1x1 to its intended size.
    renderTransform.scale3d(quad->quadRect().width(), quad->quadRect().height(), 1);
    WebTransformationMatrix contentsDeviceTransform = WebTransformationMatrix(windowMatrix() * projectionMatrix() * renderTransform).to2dTransform();

    // Can only draw surface if device matrix is invertible.
    if (!contentsDeviceTransform.isInvertible())
        return;

    OwnPtr<CCScopedTexture> backgroundTexture = drawBackgroundFilters(quad, contentsDeviceTransform);

    // FIXME: Cache this value so that we don't have to do it for both the surface and its replica.
    // Apply filters to the contents texture.
    SkBitmap filterBitmap = applyFilters(this, quad->filters(), contentsTexture);
    int contentsTextureId = contentsTexture->id();
    if (filterBitmap.getTexture()) {
        GrTexture* texture = reinterpret_cast<GrTexture*>(filterBitmap.getTexture());
        contentsTextureId = texture->getTextureHandle();
    }

    // Draw the background texture if there is one.
    if (backgroundTexture) {
        ASSERT(backgroundTexture->size() == quad->quadRect().size());
        copyTextureToFramebuffer(backgroundTexture->id(), quad->quadRect().size(), quad->drawTransform());
    }

    bool clipped = false;
    FloatQuad deviceQuad = CCMathUtil::mapQuad(contentsDeviceTransform, sharedGeometryQuad(), clipped);
    ASSERT(!clipped);
    CCLayerQuad deviceLayerBounds = CCLayerQuad(FloatQuad(deviceQuad.boundingBox()));
    CCLayerQuad deviceLayerEdges = CCLayerQuad(deviceQuad);

    // Use anti-aliasing programs only when necessary.
    bool useAA = (!deviceQuad.isRectilinear() || !deviceQuad.boundingBox().isExpressibleAsIntRect());
    if (useAA) {
        deviceLayerBounds.inflateAntiAliasingDistance();
        deviceLayerEdges.inflateAntiAliasingDistance();
    }

    bool useMask = quad->maskTextureId();

    // FIXME: use the backgroundTexture and blend the background in with this draw instead of having a separate copy of the background texture.

    GLC(context(), context()->activeTexture(GraphicsContext3D::TEXTURE0));
    context()->bindTexture(GraphicsContext3D::TEXTURE_2D, contentsTextureId);

    int shaderQuadLocation = -1;
    int shaderEdgeLocation = -1;
    int shaderMaskSamplerLocation = -1;
    int shaderMatrixLocation = -1;
    int shaderAlphaLocation = -1;
    if (useAA && useMask) {
        const RenderPassMaskProgramAA* program = renderPassMaskProgramAA();
        GLC(context(), context()->useProgram(program->program()));
        GLC(context(), context()->uniform1i(program->fragmentShader().samplerLocation(), 0));

        shaderQuadLocation = program->vertexShader().pointLocation();
        shaderEdgeLocation = program->fragmentShader().edgeLocation();
        shaderMaskSamplerLocation = program->fragmentShader().maskSamplerLocation();
        shaderMatrixLocation = program->vertexShader().matrixLocation();
        shaderAlphaLocation = program->fragmentShader().alphaLocation();
    } else if (!useAA && useMask) {
        const RenderPassMaskProgram* program = renderPassMaskProgram();
        GLC(context(), context()->useProgram(program->program()));
        GLC(context(), context()->uniform1i(program->fragmentShader().samplerLocation(), 0));

        shaderMaskSamplerLocation = program->fragmentShader().maskSamplerLocation();
        shaderMatrixLocation = program->vertexShader().matrixLocation();
        shaderAlphaLocation = program->fragmentShader().alphaLocation();
    } else if (useAA && !useMask) {
        const RenderPassProgramAA* program = renderPassProgramAA();
        GLC(context(), context()->useProgram(program->program()));
        GLC(context(), context()->uniform1i(program->fragmentShader().samplerLocation(), 0));

        shaderQuadLocation = program->vertexShader().pointLocation();
        shaderEdgeLocation = program->fragmentShader().edgeLocation();
        shaderMatrixLocation = program->vertexShader().matrixLocation();
        shaderAlphaLocation = program->fragmentShader().alphaLocation();
    } else {
        const RenderPassProgram* program = renderPassProgram();
        GLC(context(), context()->useProgram(program->program()));
        GLC(context(), context()->uniform1i(program->fragmentShader().samplerLocation(), 0));

        shaderMatrixLocation = program->vertexShader().matrixLocation();
        shaderAlphaLocation = program->fragmentShader().alphaLocation();
    }

    if (shaderMaskSamplerLocation != -1) {
        GLC(context(), context()->activeTexture(GraphicsContext3D::TEXTURE1));
        GLC(context(), context()->uniform1i(shaderMaskSamplerLocation, 1));
        context()->bindTexture(GraphicsContext3D::TEXTURE_2D, quad->maskTextureId());
        GLC(context(), context()->activeTexture(GraphicsContext3D::TEXTURE0));
    }

    if (shaderEdgeLocation != -1) {
        float edge[24];
        deviceLayerEdges.toFloatArray(edge);
        deviceLayerBounds.toFloatArray(&edge[12]);
        GLC(context(), context()->uniform3fv(shaderEdgeLocation, 8, edge));
    }

    // Map device space quad to surface space. contentsDeviceTransform has no 3d component since it was generated with to2dTransform() so we don't need to project.
    FloatQuad surfaceQuad = CCMathUtil::mapQuad(contentsDeviceTransform.inverse(), deviceLayerEdges.floatQuad(), clipped);
    ASSERT(!clipped);

    drawTexturedQuad(quad->drawTransform(), quad->quadRect().width(), quad->quadRect().height(), quad->opacity(), surfaceQuad,
                     shaderMatrixLocation, shaderAlphaLocation, shaderQuadLocation);
}

void LayerRendererChromium::drawSolidColorQuad(const CCSolidColorDrawQuad* quad)
{
    const SolidColorProgram* program = solidColorProgram();
    GLC(context(), context()->useProgram(program->program()));

    IntRect tileRect = quad->quadRect();

    WebTransformationMatrix tileTransform = quad->quadTransform();
    tileTransform.translate(tileRect.x() + tileRect.width() / 2.0, tileRect.y() + tileRect.height() / 2.0);

    SkColor color = quad->color();
    float opacity = quad->opacity();
    float alpha = (SkColorGetA(color) / 255.0) * opacity;

    GLC(context(), context()->uniform4f(program->fragmentShader().colorLocation(), (SkColorGetR(color) / 255.0) * alpha, (SkColorGetG(color) / 255.0) * alpha, (SkColorGetB(color) / 255.0) * alpha, alpha));

    drawTexturedQuad(tileTransform,
                     tileRect.width(), tileRect.height(), 1.0, FloatQuad(),
                     program->vertexShader().matrixLocation(),
                     -1, -1);
}

struct TileProgramUniforms {
    unsigned program;
    unsigned samplerLocation;
    unsigned vertexTexTransformLocation;
    unsigned fragmentTexTransformLocation;
    unsigned edgeLocation;
    unsigned matrixLocation;
    unsigned alphaLocation;
    unsigned pointLocation;
};

template<class T>
static void tileUniformLocation(T program, TileProgramUniforms& uniforms)
{
    uniforms.program = program->program();
    uniforms.vertexTexTransformLocation = program->vertexShader().vertexTexTransformLocation();
    uniforms.matrixLocation = program->vertexShader().matrixLocation();
    uniforms.pointLocation = program->vertexShader().pointLocation();

    uniforms.samplerLocation = program->fragmentShader().samplerLocation();
    uniforms.alphaLocation = program->fragmentShader().alphaLocation();
    uniforms.fragmentTexTransformLocation = program->fragmentShader().fragmentTexTransformLocation();
    uniforms.edgeLocation = program->fragmentShader().edgeLocation();
}

void LayerRendererChromium::drawTileQuad(const CCTileDrawQuad* quad)
{
    const IntRect& tileRect = quad->quadVisibleRect();

    FloatRect clampRect(tileRect);
    // Clamp texture coordinates to avoid sampling outside the layer
    // by deflating the tile region half a texel or half a texel
    // minus epsilon for one pixel layers. The resulting clamp region
    // is mapped to the unit square by the vertex shader and mapped
    // back to normalized texture coordinates by the fragment shader
    // after being clamped to 0-1 range.
    const float epsilon = 1 / 1024.0f;
    float clampX = min(0.5, clampRect.width() / 2.0 - epsilon);
    float clampY = min(0.5, clampRect.height() / 2.0 - epsilon);
    clampRect.inflateX(-clampX);
    clampRect.inflateY(-clampY);
    FloatSize clampOffset = clampRect.minXMinYCorner() - FloatRect(tileRect).minXMinYCorner();

    FloatPoint textureOffset = quad->textureOffset() + clampOffset +
                               IntPoint(quad->quadVisibleRect().location() - quad->quadRect().location());

    // Map clamping rectangle to unit square.
    float vertexTexTranslateX = -clampRect.x() / clampRect.width();
    float vertexTexTranslateY = -clampRect.y() / clampRect.height();
    float vertexTexScaleX = tileRect.width() / clampRect.width();
    float vertexTexScaleY = tileRect.height() / clampRect.height();

    // Map to normalized texture coordinates.
    const IntSize& textureSize = quad->textureSize();
    float fragmentTexTranslateX = textureOffset.x() / textureSize.width();
    float fragmentTexTranslateY = textureOffset.y() / textureSize.height();
    float fragmentTexScaleX = clampRect.width() / textureSize.width();
    float fragmentTexScaleY = clampRect.height() / textureSize.height();


    FloatQuad localQuad;
    WebTransformationMatrix deviceTransform = WebTransformationMatrix(windowMatrix() * projectionMatrix() * quad->quadTransform()).to2dTransform();
    if (!deviceTransform.isInvertible())
        return;

    bool clipped = false;
    FloatQuad deviceLayerQuad = CCMathUtil::mapQuad(deviceTransform, FloatQuad(quad->visibleContentRect()), clipped);
    ASSERT(!clipped);

    TileProgramUniforms uniforms;
    // For now, we simply skip anti-aliasing with the quad is clipped. This only happens
    // on perspective transformed layers that go partially behind the camera.
    if (quad->isAntialiased() && !clipped) {
        if (quad->swizzleContents())
            tileUniformLocation(tileProgramSwizzleAA(), uniforms);
        else
            tileUniformLocation(tileProgramAA(), uniforms);
    } else {
        if (quad->needsBlending()) {
            if (quad->swizzleContents())
                tileUniformLocation(tileProgramSwizzle(), uniforms);
            else
                tileUniformLocation(tileProgram(), uniforms);
        } else {
            if (quad->swizzleContents())
                tileUniformLocation(tileProgramSwizzleOpaque(), uniforms);
            else
                tileUniformLocation(tileProgramOpaque(), uniforms);
        }
    }

    GLC(context(), context()->useProgram(uniforms.program));
    GLC(context(), context()->uniform1i(uniforms.samplerLocation, 0));
    GLC(context(), context()->activeTexture(GraphicsContext3D::TEXTURE0));
    GLC(context(), context()->bindTexture(GraphicsContext3D::TEXTURE_2D, quad->textureId()));
    GLC(context(), context()->texParameteri(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_MIN_FILTER, quad->textureFilter()));
    GLC(context(), context()->texParameteri(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_MAG_FILTER, quad->textureFilter()));


    if (!clipped && quad->isAntialiased()) {

        CCLayerQuad deviceLayerBounds = CCLayerQuad(FloatQuad(deviceLayerQuad.boundingBox()));
        deviceLayerBounds.inflateAntiAliasingDistance();

        CCLayerQuad deviceLayerEdges = CCLayerQuad(deviceLayerQuad);
        deviceLayerEdges.inflateAntiAliasingDistance();

        float edge[24];
        deviceLayerEdges.toFloatArray(edge);
        deviceLayerBounds.toFloatArray(&edge[12]);
        GLC(context(), context()->uniform3fv(uniforms.edgeLocation, 8, edge));

        GLC(context(), context()->uniform4f(uniforms.vertexTexTransformLocation, vertexTexTranslateX, vertexTexTranslateY, vertexTexScaleX, vertexTexScaleY));
        GLC(context(), context()->uniform4f(uniforms.fragmentTexTransformLocation, fragmentTexTranslateX, fragmentTexTranslateY, fragmentTexScaleX, fragmentTexScaleY));

        FloatPoint bottomRight(tileRect.maxX(), tileRect.maxY());
        FloatPoint bottomLeft(tileRect.x(), tileRect.maxY());
        FloatPoint topLeft(tileRect.x(), tileRect.y());
        FloatPoint topRight(tileRect.maxX(), tileRect.y());

        // Map points to device space.
        bottomRight = deviceTransform.mapPoint(bottomRight);
        bottomLeft = deviceTransform.mapPoint(bottomLeft);
        topLeft = deviceTransform.mapPoint(topLeft);
        topRight = deviceTransform.mapPoint(topRight);

        CCLayerQuad::Edge bottomEdge(bottomRight, bottomLeft);
        CCLayerQuad::Edge leftEdge(bottomLeft, topLeft);
        CCLayerQuad::Edge topEdge(topLeft, topRight);
        CCLayerQuad::Edge rightEdge(topRight, bottomRight);

        // Only apply anti-aliasing to edges not clipped during culling.
        if (quad->topEdgeAA() && tileRect.y() == quad->quadRect().y())
            topEdge = deviceLayerEdges.top();
        if (quad->leftEdgeAA() && tileRect.x() == quad->quadRect().x())
            leftEdge = deviceLayerEdges.left();
        if (quad->rightEdgeAA() && tileRect.maxX() == quad->quadRect().maxX())
            rightEdge = deviceLayerEdges.right();
        if (quad->bottomEdgeAA() && tileRect.maxY() == quad->quadRect().maxY())
            bottomEdge = deviceLayerEdges.bottom();

        float sign = FloatQuad(tileRect).isCounterclockwise() ? -1 : 1;
        bottomEdge.scale(sign);
        leftEdge.scale(sign);
        topEdge.scale(sign);
        rightEdge.scale(sign);

        // Create device space quad.
        CCLayerQuad deviceQuad(leftEdge, topEdge, rightEdge, bottomEdge);

        // Map device space quad to local space. contentsDeviceTransform has no 3d component since it was generated with to2dTransform() so we don't need to project.
        WebTransformationMatrix inverseDeviceTransform = deviceTransform.inverse();
        localQuad = CCMathUtil::mapQuad(inverseDeviceTransform, deviceQuad.floatQuad(), clipped);

        // We should not ASSERT(!clipped) here, because anti-aliasing inflation may cause deviceQuad to become
        // clipped.  To our knowledge this scenario does not need to be handled differently than the unclipped case.
    } else {
        // Move fragment shader transform to vertex shader. We can do this while
        // still producing correct results as fragmentTexTransformLocation
        // should always be non-negative when tiles are transformed in a way
        // that could result in sampling outside the layer.
        vertexTexScaleX *= fragmentTexScaleX;
        vertexTexScaleY *= fragmentTexScaleY;
        vertexTexTranslateX *= fragmentTexScaleX;
        vertexTexTranslateY *= fragmentTexScaleY;
        vertexTexTranslateX += fragmentTexTranslateX;
        vertexTexTranslateY += fragmentTexTranslateY;

        GLC(context(), context()->uniform4f(uniforms.vertexTexTransformLocation, vertexTexTranslateX, vertexTexTranslateY, vertexTexScaleX, vertexTexScaleY));

        localQuad = FloatRect(tileRect);
    }

    // Normalize to tileRect.
    localQuad.scale(1.0f / tileRect.width(), 1.0f / tileRect.height());

    drawTexturedQuad(quad->quadTransform(), tileRect.width(), tileRect.height(), quad->opacity(), localQuad, uniforms.matrixLocation, uniforms.alphaLocation, uniforms.pointLocation);
}

void LayerRendererChromium::drawYUVVideoQuad(const CCYUVVideoDrawQuad* quad)
{
    const VideoYUVProgram* program = videoYUVProgram();
    ASSERT(program && program->initialized());

    const CCVideoLayerImpl::FramePlane& yPlane = quad->yPlane();
    const CCVideoLayerImpl::FramePlane& uPlane = quad->uPlane();
    const CCVideoLayerImpl::FramePlane& vPlane = quad->vPlane();

    GLC(context(), context()->activeTexture(GraphicsContext3D::TEXTURE1));
    GLC(context(), context()->bindTexture(GraphicsContext3D::TEXTURE_2D, yPlane.textureId));
    GLC(context(), context()->activeTexture(GraphicsContext3D::TEXTURE2));
    GLC(context(), context()->bindTexture(GraphicsContext3D::TEXTURE_2D, uPlane.textureId));
    GLC(context(), context()->activeTexture(GraphicsContext3D::TEXTURE3));
    GLC(context(), context()->bindTexture(GraphicsContext3D::TEXTURE_2D, vPlane.textureId));

    GLC(context(), context()->useProgram(program->program()));

    float yWidthScaleFactor = static_cast<float>(yPlane.visibleSize.width()) / yPlane.size.width();
    // Arbitrarily take the u sizes because u and v dimensions are identical.
    float uvWidthScaleFactor = static_cast<float>(uPlane.visibleSize.width()) / uPlane.size.width();
    GLC(context(), context()->uniform1f(program->vertexShader().yWidthScaleFactorLocation(), yWidthScaleFactor));
    GLC(context(), context()->uniform1f(program->vertexShader().uvWidthScaleFactorLocation(), uvWidthScaleFactor));

    GLC(context(), context()->uniform1i(program->fragmentShader().yTextureLocation(), 1));
    GLC(context(), context()->uniform1i(program->fragmentShader().uTextureLocation(), 2));
    GLC(context(), context()->uniform1i(program->fragmentShader().vTextureLocation(), 3));

    // These values are magic numbers that are used in the transformation from YUV to RGB color values.
    // They are taken from the following webpage: http://www.fourcc.org/fccyvrgb.php
    float yuv2RGB[9] = {
        1.164f, 1.164f, 1.164f,
        0.f, -.391f, 2.018f,
        1.596f, -.813f, 0.f,
    };
    GLC(context(), context()->uniformMatrix3fv(program->fragmentShader().ccMatrixLocation(), 1, 0, yuv2RGB));

    // These values map to 16, 128, and 128 respectively, and are computed
    // as a fraction over 256 (e.g. 16 / 256 = 0.0625).
    // They are used in the YUV to RGBA conversion formula:
    //   Y - 16   : Gives 16 values of head and footroom for overshooting
    //   U - 128  : Turns unsigned U into signed U [-128,127]
    //   V - 128  : Turns unsigned V into signed V [-128,127]
    float yuvAdjust[3] = {
        -0.0625f,
        -0.5f,
        -0.5f,
    };
    GLC(context(), context()->uniform3fv(program->fragmentShader().yuvAdjLocation(), 1, yuvAdjust));

    WebTransformationMatrix quadTransform = quad->quadTransform();
    IntRect quadRect = quad->quadRect();
    quadTransform.translate(quadRect.x() + quadRect.width() / 2.0, quadRect.y() + quadRect.height() / 2.0);

    drawTexturedQuad(quadTransform, quadRect.width(), quadRect.height(), quad->opacity(), FloatQuad(),
                     program->vertexShader().matrixLocation(), program->fragmentShader().alphaLocation(), -1);

    // Reset active texture back to texture 0.
    GLC(context(), context()->activeTexture(GraphicsContext3D::TEXTURE0));
}

void LayerRendererChromium::drawStreamVideoQuad(const CCStreamVideoDrawQuad* quad)
{
    static float glMatrix[16];

    ASSERT(m_capabilities.usingEglImage);

    const VideoStreamTextureProgram* program = videoStreamTextureProgram();
    GLC(context(), context()->useProgram(program->program()));

    toGLMatrix(&glMatrix[0], quad->matrix());
    GLC(context(), context()->uniformMatrix4fv(program->vertexShader().texMatrixLocation(), 1, false, glMatrix));

    GLC(context(), context()->activeTexture(GraphicsContext3D::TEXTURE0));
    GLC(context(), context()->bindTexture(Extensions3DChromium::GL_TEXTURE_EXTERNAL_OES, quad->textureId()));

    GLC(context(), context()->uniform1i(program->fragmentShader().samplerLocation(), 0));

    WebTransformationMatrix quadTransform = quad->quadTransform();
    IntRect quadRect = quad->quadRect();
    quadTransform.translate(quadRect.x() + quadRect.width() / 2.0, quadRect.y() + quadRect.height() / 2.0);

    drawTexturedQuad(quadTransform, quadRect.width(), quadRect.height(), quad->opacity(), sharedGeometryQuad(),
                     program->vertexShader().matrixLocation(), program->fragmentShader().alphaLocation(), -1);
}

struct TextureProgramBinding {
    template<class Program> void set(Program* program)
    {
        ASSERT(program && program->initialized());
        programId = program->program();
        samplerLocation = program->fragmentShader().samplerLocation();
        matrixLocation = program->vertexShader().matrixLocation();
        alphaLocation = program->fragmentShader().alphaLocation();
    }
    int programId;
    int samplerLocation;
    int matrixLocation;
    int alphaLocation;
};

struct TexTransformTextureProgramBinding : TextureProgramBinding {
    template<class Program> void set(Program* program)
    {
        TextureProgramBinding::set(program);
        texTransformLocation = program->vertexShader().texTransformLocation();
    }
    int texTransformLocation;
};

void LayerRendererChromium::drawTextureQuad(const CCTextureDrawQuad* quad)
{
    ASSERT(CCProxy::isImplThread());

    TexTransformTextureProgramBinding binding;
    if (quad->flipped())
        binding.set(textureProgramFlip());
    else
        binding.set(textureProgram());
    GLC(context(), context()->useProgram(binding.programId));
    GLC(context(), context()->uniform1i(binding.samplerLocation, 0));
    const FloatRect& uvRect = quad->uvRect();
    GLC(context(), context()->uniform4f(binding.texTransformLocation, uvRect.x(), uvRect.y(), uvRect.width(), uvRect.height()));

    GLC(context(), context()->activeTexture(GraphicsContext3D::TEXTURE0));
    GLC(context(), context()->bindTexture(GraphicsContext3D::TEXTURE_2D, quad->textureId()));

    // FIXME: setting the texture parameters every time is redundant. Move this code somewhere
    // where it will only happen once per texture.
    GLC(context(), context()->texParameteri(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_MIN_FILTER, GraphicsContext3D::LINEAR));
    GLC(context(), context()->texParameteri(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_MAG_FILTER, GraphicsContext3D::LINEAR));
    GLC(context(), context()->texParameteri(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_WRAP_S, GraphicsContext3D::CLAMP_TO_EDGE));
    GLC(context(), context()->texParameteri(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_WRAP_T, GraphicsContext3D::CLAMP_TO_EDGE));

    if (!quad->premultipliedAlpha()) {
        // As it turns out, the premultiplied alpha blending function (ONE, ONE_MINUS_SRC_ALPHA)
        // will never cause the alpha channel to be set to anything less than 1.0 if it is
        // initialized to that value! Therefore, premultipliedAlpha being false is the first
        // situation we can generally see an alpha channel less than 1.0 coming out of the
        // compositor. This is causing platform differences in some layout tests (see
        // https://bugs.webkit.org/show_bug.cgi?id=82412), so in this situation, use a separate
        // blend function for the alpha channel to avoid modifying it. Don't use colorMask for this
        // as it has performance implications on some platforms.
        GLC(context(), context()->blendFuncSeparate(GraphicsContext3D::SRC_ALPHA, GraphicsContext3D::ONE_MINUS_SRC_ALPHA, GraphicsContext3D::ZERO, GraphicsContext3D::ONE));
    }

    WebTransformationMatrix quadTransform = quad->quadTransform();
    IntRect quadRect = quad->quadRect();
    quadTransform.translate(quadRect.x() + quadRect.width() / 2.0, quadRect.y() + quadRect.height() / 2.0);

    drawTexturedQuad(quadTransform, quadRect.width(), quadRect.height(), quad->opacity(), sharedGeometryQuad(), binding.matrixLocation, binding.alphaLocation, -1);

    if (!quad->premultipliedAlpha())
        GLC(m_context, m_context->blendFunc(GraphicsContext3D::ONE, GraphicsContext3D::ONE_MINUS_SRC_ALPHA));
}

void LayerRendererChromium::drawIOSurfaceQuad(const CCIOSurfaceDrawQuad* quad)
{
    ASSERT(CCProxy::isImplThread());
    TexTransformTextureProgramBinding binding;
    binding.set(textureIOSurfaceProgram());

    GLC(context(), context()->useProgram(binding.programId));
    GLC(context(), context()->uniform1i(binding.samplerLocation, 0));
    if (quad->orientation() == CCIOSurfaceDrawQuad::Flipped)
        GLC(context(), context()->uniform4f(binding.texTransformLocation, 0, quad->ioSurfaceSize().height(), quad->ioSurfaceSize().width(), quad->ioSurfaceSize().height() * -1.0));
    else
        GLC(context(), context()->uniform4f(binding.texTransformLocation, 0, 0, quad->ioSurfaceSize().width(), quad->ioSurfaceSize().height()));

    GLC(context(), context()->activeTexture(GraphicsContext3D::TEXTURE0));
    GLC(context(), context()->bindTexture(Extensions3D::TEXTURE_RECTANGLE_ARB, quad->ioSurfaceTextureId()));

    // FIXME: setting the texture parameters every time is redundant. Move this code somewhere
    // where it will only happen once per texture.
    GLC(context(), context()->texParameteri(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_MIN_FILTER, GraphicsContext3D::LINEAR));
    GLC(context(), context()->texParameteri(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_MAG_FILTER, GraphicsContext3D::LINEAR));
    GLC(context(), context()->texParameteri(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_WRAP_S, GraphicsContext3D::CLAMP_TO_EDGE));
    GLC(context(), context()->texParameteri(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_WRAP_T, GraphicsContext3D::CLAMP_TO_EDGE));

    WebTransformationMatrix quadTransform = quad->quadTransform();
    IntRect quadRect = quad->quadRect();
    quadTransform.translate(quadRect.x() + quadRect.width() / 2.0, quadRect.y() + quadRect.height() / 2.0);

    drawTexturedQuad(quadTransform, quadRect.width(), quadRect.height(), quad->opacity(), sharedGeometryQuad(), binding.matrixLocation, binding.alphaLocation, -1);

    GLC(context(), context()->bindTexture(Extensions3D::TEXTURE_RECTANGLE_ARB, 0));
}

void LayerRendererChromium::drawHeadsUpDisplay(const CCScopedTexture* hudTexture, const IntSize& hudSize)
{
    ASSERT(hudTexture->id());

    GLC(m_context, m_context->enable(GraphicsContext3D::BLEND));
    GLC(m_context, m_context->blendFunc(GraphicsContext3D::ONE, GraphicsContext3D::ONE_MINUS_SRC_ALPHA));
    GLC(m_context, m_context->disable(GraphicsContext3D::SCISSOR_TEST));
    useRenderPass(m_defaultRenderPass);

    const HeadsUpDisplayProgram* program = headsUpDisplayProgram();
    ASSERT(program && program->initialized());
    GLC(m_context, m_context->activeTexture(GraphicsContext3D::TEXTURE0));
    GLC(m_context, m_context->bindTexture(GraphicsContext3D::TEXTURE_2D, hudTexture->id()));
    GLC(m_context, m_context->useProgram(program->program()));
    GLC(m_context, m_context->uniform1i(program->fragmentShader().samplerLocation(), 0));

    WebTransformationMatrix matrix;
    matrix.translate3d(hudSize.width() * 0.5, hudSize.height() * 0.5, 0);
    drawTexturedQuad(matrix, hudSize.width(), hudSize.height(),
                     1, sharedGeometryQuad(), program->vertexShader().matrixLocation(),
                     program->fragmentShader().alphaLocation(),
                     -1);
}

void LayerRendererChromium::finishDrawingFrame()
{
    GLC(m_context, m_context->disable(GraphicsContext3D::SCISSOR_TEST));
    GLC(m_context, m_context->disable(GraphicsContext3D::BLEND));
}

void LayerRendererChromium::toGLMatrix(float* flattened, const WebTransformationMatrix& m)
{
    flattened[0] = m.m11();
    flattened[1] = m.m12();
    flattened[2] = m.m13();
    flattened[3] = m.m14();
    flattened[4] = m.m21();
    flattened[5] = m.m22();
    flattened[6] = m.m23();
    flattened[7] = m.m24();
    flattened[8] = m.m31();
    flattened[9] = m.m32();
    flattened[10] = m.m33();
    flattened[11] = m.m34();
    flattened[12] = m.m41();
    flattened[13] = m.m42();
    flattened[14] = m.m43();
    flattened[15] = m.m44();
}

void LayerRendererChromium::drawTexturedQuad(const WebTransformationMatrix& drawMatrix,
                                             float width, float height, float opacity, const FloatQuad& quad,
                                             int matrixLocation, int alphaLocation, int quadLocation)
{
    static float glMatrix[16];

    WebTransformationMatrix renderMatrix = drawMatrix;

    // Apply a scaling factor to size the quad from 1x1 to its intended size.
    renderMatrix.scale3d(width, height, 1);

    // Apply the projection matrix before sending the transform over to the shader.
    toGLMatrix(&glMatrix[0], m_projectionMatrix * renderMatrix);

    GLC(m_context, m_context->uniformMatrix4fv(matrixLocation, 1, false, &glMatrix[0]));

    if (quadLocation != -1) {
        float point[8];
        point[0] = quad.p1().x();
        point[1] = quad.p1().y();
        point[2] = quad.p2().x();
        point[3] = quad.p2().y();
        point[4] = quad.p3().x();
        point[5] = quad.p3().y();
        point[6] = quad.p4().x();
        point[7] = quad.p4().y();
        GLC(m_context, m_context->uniform2fv(quadLocation, 4, point));
    }

    if (alphaLocation != -1)
        GLC(m_context, m_context->uniform1f(alphaLocation, opacity));

    GLC(m_context, m_context->drawElements(GraphicsContext3D::TRIANGLES, 6, GraphicsContext3D::UNSIGNED_SHORT, 0));
}

void LayerRendererChromium::copyTextureToFramebuffer(int textureId, const IntSize& bounds, const WebTransformationMatrix& drawMatrix)
{
    const RenderPassProgram* program = renderPassProgram();

    GLC(context(), context()->activeTexture(GraphicsContext3D::TEXTURE0));
    GLC(context(), context()->bindTexture(GraphicsContext3D::TEXTURE_2D, textureId));
    GLC(context(), context()->texParameteri(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_MIN_FILTER, GraphicsContext3D::LINEAR));
    GLC(context(), context()->texParameteri(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_MAG_FILTER, GraphicsContext3D::LINEAR));
    GLC(context(), context()->texParameteri(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_WRAP_S, GraphicsContext3D::CLAMP_TO_EDGE));
    GLC(context(), context()->texParameteri(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_WRAP_T, GraphicsContext3D::CLAMP_TO_EDGE));

    GLC(context(), context()->useProgram(program->program()));
    GLC(context(), context()->uniform1i(program->fragmentShader().samplerLocation(), 0));
    drawTexturedQuad(drawMatrix, bounds.width(), bounds.height(), 1, sharedGeometryQuad(),
                                    program->vertexShader().matrixLocation(),
                                    program->fragmentShader().alphaLocation(),
                                    -1);
}

void LayerRendererChromium::finish()
{
    TRACE_EVENT0("cc", "LayerRendererChromium::finish");
    m_context->finish();
}

bool LayerRendererChromium::swapBuffers(const IntRect& subBuffer)
{
    ASSERT(m_visible);
    ASSERT(!m_isFramebufferDiscarded);

    TRACE_EVENT0("cc", "LayerRendererChromium::swapBuffers");
    // We're done! Time to swapbuffers!

    if (m_capabilities.usingPartialSwap) {
        // If supported, we can save significant bandwidth by only swapping the damaged/scissored region (clamped to the viewport)
        IntRect clippedSubBuffer = subBuffer;
        clippedSubBuffer.intersect(IntRect(IntPoint::zero(), viewportSize()));
        int flippedYPosOfRectBottom = viewportHeight() - clippedSubBuffer.y() - clippedSubBuffer.height();
        m_context->postSubBufferCHROMIUM(clippedSubBuffer.x(), flippedYPosOfRectBottom, clippedSubBuffer.width(), clippedSubBuffer.height());
    } else
        // Note that currently this has the same effect as swapBuffers; we should
        // consider exposing a different entry point on WebGraphicsContext3D.
        m_context->prepareTexture();

    return true;
}

void LayerRendererChromium::onSwapBuffersComplete()
{
    m_client->onSwapBuffersComplete();
}

void LayerRendererChromium::onMemoryAllocationChanged(WebGraphicsMemoryAllocation allocation)
{
    // FIXME: This is called on the main thread in single threaded mode, but we expect it on the impl thread.
    if (!CCProxy::hasImplThread()) {
      ASSERT(CCProxy::isMainThread());
      DebugScopedSetImplThread impl;
      onMemoryAllocationChangedOnImplThread(allocation);
    } else {
      ASSERT(CCProxy::isImplThread());
      onMemoryAllocationChangedOnImplThread(allocation);
    }
}

void LayerRendererChromium::onMemoryAllocationChangedOnImplThread(WebKit::WebGraphicsMemoryAllocation allocation)
{
    if (m_visible && !allocation.gpuResourceSizeInBytes)
        return;

    if (!allocation.suggestHaveBackbuffer && !m_visible)
        discardFramebuffer();

    if (!allocation.gpuResourceSizeInBytes) {
        releaseRenderPassTextures();
        m_client->releaseContentsTextures();
        GLC(m_context, m_context->flush());
    } else
        m_client->setMemoryAllocationLimitBytes(allocation.gpuResourceSizeInBytes);
}

void LayerRendererChromium::discardFramebuffer()
{
    if (m_isFramebufferDiscarded)
        return;

    if (!m_capabilities.usingDiscardFramebuffer)
        return;

    // FIXME: Update attachments argument to appropriate values once they are no longer ignored.
    m_context->discardFramebufferEXT(GraphicsContext3D::TEXTURE_2D, 0, 0);
    m_isFramebufferDiscarded = true;

    // Damage tracker needs a full reset every time framebuffer is discarded.
    m_client->setFullRootLayerDamage();
}

void LayerRendererChromium::ensureFramebuffer()
{
    if (!m_isFramebufferDiscarded)
        return;

    if (!m_capabilities.usingDiscardFramebuffer)
        return;

    m_context->ensureFramebufferCHROMIUM();
    m_isFramebufferDiscarded = false;
}

void LayerRendererChromium::onContextLost()
{
    m_client->didLoseContext();
}


void LayerRendererChromium::getFramebufferPixels(void *pixels, const IntRect& rect)
{
    ASSERT(rect.maxX() <= viewportWidth() && rect.maxY() <= viewportHeight());

    if (!pixels)
        return;

    makeContextCurrent();

    bool doWorkaround = needsIOSurfaceReadbackWorkaround();

    Platform3DObject temporaryTexture = NullPlatform3DObject;
    Platform3DObject temporaryFBO = NullPlatform3DObject;

    if (doWorkaround) {
        // On Mac OS X, calling glReadPixels against an FBO whose color attachment is an
        // IOSurface-backed texture causes corruption of future glReadPixels calls, even those on
        // different OpenGL contexts. It is believed that this is the root cause of top crasher
        // http://crbug.com/99393. <rdar://problem/10949687>

        temporaryTexture = m_context->createTexture();
        GLC(m_context, m_context->bindTexture(GraphicsContext3D::TEXTURE_2D, temporaryTexture));
        GLC(m_context, m_context->texParameteri(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_MIN_FILTER, GraphicsContext3D::LINEAR));
        GLC(m_context, m_context->texParameteri(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_MAG_FILTER, GraphicsContext3D::LINEAR));
        GLC(m_context, m_context->texParameteri(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_WRAP_S, GraphicsContext3D::CLAMP_TO_EDGE));
        GLC(m_context, m_context->texParameteri(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_WRAP_T, GraphicsContext3D::CLAMP_TO_EDGE));
        // Copy the contents of the current (IOSurface-backed) framebuffer into a temporary texture.
        GLC(m_context, m_context->copyTexImage2D(GraphicsContext3D::TEXTURE_2D, 0, GraphicsContext3D::RGBA, 0, 0, rect.maxX(), rect.maxY(), 0));
        temporaryFBO = m_context->createFramebuffer();
        // Attach this texture to an FBO, and perform the readback from that FBO.
        GLC(m_context, m_context->bindFramebuffer(GraphicsContext3D::FRAMEBUFFER, temporaryFBO));
        GLC(m_context, m_context->framebufferTexture2D(GraphicsContext3D::FRAMEBUFFER, GraphicsContext3D::COLOR_ATTACHMENT0, GraphicsContext3D::TEXTURE_2D, temporaryTexture, 0));

        ASSERT(m_context->checkFramebufferStatus(GraphicsContext3D::FRAMEBUFFER) == GraphicsContext3D::FRAMEBUFFER_COMPLETE);
    }

    GLC(m_context, m_context->readPixels(rect.x(), rect.y(), rect.width(), rect.height(),
                                     GraphicsContext3D::RGBA, GraphicsContext3D::UNSIGNED_BYTE, pixels));

    if (doWorkaround) {
        // Clean up.
        GLC(m_context, m_context->bindFramebuffer(GraphicsContext3D::FRAMEBUFFER, 0));
        GLC(m_context, m_context->bindTexture(GraphicsContext3D::TEXTURE_2D, 0));
        GLC(m_context, m_context->deleteFramebuffer(temporaryFBO));
        GLC(m_context, m_context->deleteTexture(temporaryTexture));
    }

    if (!m_visible) {
        TRACE_EVENT0("cc", "LayerRendererChromium::getFramebufferPixels dropping resources after readback");
        discardFramebuffer();
        releaseRenderPassTextures();
        m_client->releaseContentsTextures();
        GLC(m_context, m_context->flush());
    }
}

bool LayerRendererChromium::getFramebufferTexture(CCScopedTexture* texture, const IntRect& deviceRect)
{
    ASSERT(!texture->id() || (texture->size() == deviceRect.size() && texture->format() == GraphicsContext3D::RGB));

    if (!texture->id() && !texture->allocate(deviceRect.size(), GraphicsContext3D::RGB))
        return false;

    GLC(m_context, m_context->bindTexture(GraphicsContext3D::TEXTURE_2D, texture->id()));
    GLC(m_context, m_context->copyTexImage2D(GraphicsContext3D::TEXTURE_2D, 0, texture->format(),
                                             deviceRect.x(), deviceRect.y(), deviceRect.width(), deviceRect.height(), 0));
    return true;
}

bool LayerRendererChromium::isCurrentRenderPass(const CCRenderPass* renderPass)
{
    return m_currentRenderPass == renderPass && !m_currentTexture;
}

bool LayerRendererChromium::useRenderPass(const CCRenderPass* renderPass)
{
    m_currentRenderPass = renderPass;
    m_currentTexture = 0;

    if (renderPass == m_defaultRenderPass) {
        GLC(m_context, m_context->bindFramebuffer(GraphicsContext3D::FRAMEBUFFER, 0));
        setDrawFramebufferRect(renderPass->framebufferOutputRect(), true);
        return true;
    }

    CCScopedTexture* texture = m_renderPassTextures.get(renderPass->id());
    ASSERT(texture);

    if (!texture->id() && !texture->allocate(renderPassTextureSize(renderPass), renderPassTextureFormat(renderPass)))
        return false;

    return bindFramebufferToTexture(texture, renderPass->framebufferOutputRect());
}

bool LayerRendererChromium::useScopedTexture(const CCScopedTexture* texture, const IntRect& viewportRect)
{
    ASSERT(texture->id());

    m_currentRenderPass = 0;
    m_currentTexture = texture;

    return bindFramebufferToTexture(texture, viewportRect);
}

bool LayerRendererChromium::bindFramebufferToTexture(const CCScopedTexture* texture, const IntRect& framebufferRect)
{
    ASSERT(texture->id());

    GLC(m_context, m_context->bindFramebuffer(GraphicsContext3D::FRAMEBUFFER, m_offscreenFramebufferId));
    GLC(m_context, m_context->framebufferTexture2D(GraphicsContext3D::FRAMEBUFFER, GraphicsContext3D::COLOR_ATTACHMENT0, GraphicsContext3D::TEXTURE_2D, texture->id(), 0));

#if !defined ( NDEBUG )
    if (m_context->checkFramebufferStatus(GraphicsContext3D::FRAMEBUFFER) != GraphicsContext3D::FRAMEBUFFER_COMPLETE) {
        ASSERT_NOT_REACHED();
        return false;
    }
#endif

    setDrawFramebufferRect(framebufferRect, false);

    return true;
}

// Sets the scissor region to the given rectangle. The coordinate system for the
// scissorRect has its origin at the top left corner of the current visible rect.
void LayerRendererChromium::setScissorToRect(const IntRect& scissorRect)
{
    IntRect framebufferOutputRect = (m_currentRenderPass ? m_currentRenderPass->framebufferOutputRect() : m_defaultRenderPass->framebufferOutputRect());

    GLC(m_context, m_context->enable(GraphicsContext3D::SCISSOR_TEST));

    // The scissor coordinates must be supplied in viewport space so we need to offset
    // by the relative position of the top left corner of the current render pass.
    int scissorX = scissorRect.x() - framebufferOutputRect.x();
    // When rendering to the default render surface we're rendering upside down so the top
    // of the GL scissor is the bottom of our layer.
    // But, if rendering to offscreen texture, we reverse our sense of 'upside down'.
    int scissorY;
    if (isCurrentRenderPass(m_defaultRenderPass))
        scissorY = framebufferOutputRect.height() - (scissorRect.maxY() - framebufferOutputRect.y());
    else
        scissorY = scissorRect.y() - framebufferOutputRect.y();
    GLC(m_context, m_context->scissor(scissorX, scissorY, scissorRect.width(), scissorRect.height()));
}

bool LayerRendererChromium::makeContextCurrent()
{
    return m_context->makeContextCurrent();
}

// Sets the coordinate range of content that ends being drawn onto the target render surface.
// The target render surface is assumed to have an origin at 0, 0 and the width and height of
// of the drawRect.
void LayerRendererChromium::setDrawFramebufferRect(const IntRect& drawRect, bool flipY)
{
    if (flipY)
        m_projectionMatrix = orthoMatrix(drawRect.x(), drawRect.maxX(), drawRect.maxY(), drawRect.y());
    else
        m_projectionMatrix = orthoMatrix(drawRect.x(), drawRect.maxX(), drawRect.y(), drawRect.maxY());
    GLC(m_context, m_context->viewport(0, 0, drawRect.width(), drawRect.height()));
    m_windowMatrix = screenMatrix(0, 0, drawRect.width(), drawRect.height());
}


bool LayerRendererChromium::initializeSharedObjects()
{
    TRACE_EVENT0("cc", "LayerRendererChromium::initializeSharedObjects");
    makeContextCurrent();

    // Create an FBO for doing offscreen rendering.
    GLC(m_context, m_offscreenFramebufferId = m_context->createFramebuffer());

    // We will always need these programs to render, so create the programs eagerly so that the shader compilation can
    // start while we do other work. Other programs are created lazily on first access.
    m_sharedGeometry = adoptPtr(new GeometryBinding(m_context));
    m_renderPassProgram = adoptPtr(new RenderPassProgram(m_context));
    m_tileProgram = adoptPtr(new TileProgram(m_context));
    m_tileProgramOpaque = adoptPtr(new TileProgramOpaque(m_context));

    GLC(m_context, m_context->flush());

    m_textureCopier = AcceleratedTextureCopier::create(m_context, m_isUsingBindUniform);
    if (m_textureUploaderSetting == ThrottledUploader)
        m_textureUploader = ThrottledTextureUploader::create(m_context);
    else
        m_textureUploader = UnthrottledTextureUploader::create();
    m_contentsTextureAllocator = TrackingTextureAllocator::create(m_context, m_capabilities.maxTextureSize);
    m_implTextureAllocator = TrackingTextureAllocator::create(m_context, m_capabilities.maxTextureSize);
    if (m_capabilities.usingTextureUsageHint)
        m_implTextureAllocator->setTextureUsageHint(TrackingTextureAllocator::FramebufferAttachment);
    if (m_capabilities.usingTextureStorageExtension) {
        m_contentsTextureAllocator->setUseTextureStorageExt(true);
        m_implTextureAllocator->setUseTextureStorageExt(true);
    }

    return true;
}

const LayerRendererChromium::TileCheckerboardProgram* LayerRendererChromium::tileCheckerboardProgram()
{
    if (!m_tileCheckerboardProgram)
        m_tileCheckerboardProgram = adoptPtr(new TileCheckerboardProgram(m_context));
    if (!m_tileCheckerboardProgram->initialized()) {
        TRACE_EVENT0("cc", "LayerRendererChromium::checkerboardProgram::initalize");
        m_tileCheckerboardProgram->initialize(m_context, m_isUsingBindUniform);
    }
    return m_tileCheckerboardProgram.get();
}

const LayerRendererChromium::SolidColorProgram* LayerRendererChromium::solidColorProgram()
{
    if (!m_solidColorProgram)
        m_solidColorProgram = adoptPtr(new SolidColorProgram(m_context));
    if (!m_solidColorProgram->initialized()) {
        TRACE_EVENT0("cc", "LayerRendererChromium::solidColorProgram::initialize");
        m_solidColorProgram->initialize(m_context, m_isUsingBindUniform);
    }
    return m_solidColorProgram.get();
}

const LayerRendererChromium::HeadsUpDisplayProgram* LayerRendererChromium::headsUpDisplayProgram()
{
    if (!m_headsUpDisplayProgram)
        m_headsUpDisplayProgram = adoptPtr(new HeadsUpDisplayProgram(m_context));
    if (!m_headsUpDisplayProgram->initialized()) {
        TRACE_EVENT0("cc", "LayerRendererChromium::headsUpDisplayProgram::initialize");
        m_headsUpDisplayProgram->initialize(m_context, m_isUsingBindUniform);
    }
    return m_headsUpDisplayProgram.get();
}

const LayerRendererChromium::RenderPassProgram* LayerRendererChromium::renderPassProgram()
{
    ASSERT(m_renderPassProgram);
    if (!m_renderPassProgram->initialized()) {
        TRACE_EVENT0("cc", "LayerRendererChromium::renderPassProgram::initialize");
        m_renderPassProgram->initialize(m_context, m_isUsingBindUniform);
    }
    return m_renderPassProgram.get();
}

const LayerRendererChromium::RenderPassProgramAA* LayerRendererChromium::renderPassProgramAA()
{
    if (!m_renderPassProgramAA)
        m_renderPassProgramAA = adoptPtr(new RenderPassProgramAA(m_context));
    if (!m_renderPassProgramAA->initialized()) {
        TRACE_EVENT0("cc", "LayerRendererChromium::renderPassProgramAA::initialize");
        m_renderPassProgramAA->initialize(m_context, m_isUsingBindUniform);
    }
    return m_renderPassProgramAA.get();
}

const LayerRendererChromium::RenderPassMaskProgram* LayerRendererChromium::renderPassMaskProgram()
{
    if (!m_renderPassMaskProgram)
        m_renderPassMaskProgram = adoptPtr(new RenderPassMaskProgram(m_context));
    if (!m_renderPassMaskProgram->initialized()) {
        TRACE_EVENT0("cc", "LayerRendererChromium::renderPassMaskProgram::initialize");
        m_renderPassMaskProgram->initialize(m_context, m_isUsingBindUniform);
    }
    return m_renderPassMaskProgram.get();
}

const LayerRendererChromium::RenderPassMaskProgramAA* LayerRendererChromium::renderPassMaskProgramAA()
{
    if (!m_renderPassMaskProgramAA)
        m_renderPassMaskProgramAA = adoptPtr(new RenderPassMaskProgramAA(m_context));
    if (!m_renderPassMaskProgramAA->initialized()) {
        TRACE_EVENT0("cc", "LayerRendererChromium::renderPassMaskProgramAA::initialize");
        m_renderPassMaskProgramAA->initialize(m_context, m_isUsingBindUniform);
    }
    return m_renderPassMaskProgramAA.get();
}

const LayerRendererChromium::TileProgram* LayerRendererChromium::tileProgram()
{
    ASSERT(m_tileProgram);
    if (!m_tileProgram->initialized()) {
        TRACE_EVENT0("cc", "LayerRendererChromium::tileProgram::initialize");
        m_tileProgram->initialize(m_context, m_isUsingBindUniform);
    }
    return m_tileProgram.get();
}

const LayerRendererChromium::TileProgramOpaque* LayerRendererChromium::tileProgramOpaque()
{
    ASSERT(m_tileProgramOpaque);
    if (!m_tileProgramOpaque->initialized()) {
        TRACE_EVENT0("cc", "LayerRendererChromium::tileProgramOpaque::initialize");
        m_tileProgramOpaque->initialize(m_context, m_isUsingBindUniform);
    }
    return m_tileProgramOpaque.get();
}

const LayerRendererChromium::TileProgramAA* LayerRendererChromium::tileProgramAA()
{
    if (!m_tileProgramAA)
        m_tileProgramAA = adoptPtr(new TileProgramAA(m_context));
    if (!m_tileProgramAA->initialized()) {
        TRACE_EVENT0("cc", "LayerRendererChromium::tileProgramAA::initialize");
        m_tileProgramAA->initialize(m_context, m_isUsingBindUniform);
    }
    return m_tileProgramAA.get();
}

const LayerRendererChromium::TileProgramSwizzle* LayerRendererChromium::tileProgramSwizzle()
{
    if (!m_tileProgramSwizzle)
        m_tileProgramSwizzle = adoptPtr(new TileProgramSwizzle(m_context));
    if (!m_tileProgramSwizzle->initialized()) {
        TRACE_EVENT0("cc", "LayerRendererChromium::tileProgramSwizzle::initialize");
        m_tileProgramSwizzle->initialize(m_context, m_isUsingBindUniform);
    }
    return m_tileProgramSwizzle.get();
}

const LayerRendererChromium::TileProgramSwizzleOpaque* LayerRendererChromium::tileProgramSwizzleOpaque()
{
    if (!m_tileProgramSwizzleOpaque)
        m_tileProgramSwizzleOpaque = adoptPtr(new TileProgramSwizzleOpaque(m_context));
    if (!m_tileProgramSwizzleOpaque->initialized()) {
        TRACE_EVENT0("cc", "LayerRendererChromium::tileProgramSwizzleOpaque::initialize");
        m_tileProgramSwizzleOpaque->initialize(m_context, m_isUsingBindUniform);
    }
    return m_tileProgramSwizzleOpaque.get();
}

const LayerRendererChromium::TileProgramSwizzleAA* LayerRendererChromium::tileProgramSwizzleAA()
{
    if (!m_tileProgramSwizzleAA)
        m_tileProgramSwizzleAA = adoptPtr(new TileProgramSwizzleAA(m_context));
    if (!m_tileProgramSwizzleAA->initialized()) {
        TRACE_EVENT0("cc", "LayerRendererChromium::tileProgramSwizzleAA::initialize");
        m_tileProgramSwizzleAA->initialize(m_context, m_isUsingBindUniform);
    }
    return m_tileProgramSwizzleAA.get();
}

const LayerRendererChromium::TextureProgram* LayerRendererChromium::textureProgram()
{
    if (!m_textureProgram)
        m_textureProgram = adoptPtr(new TextureProgram(m_context));
    if (!m_textureProgram->initialized()) {
        TRACE_EVENT0("cc", "LayerRendererChromium::textureProgram::initialize");
        m_textureProgram->initialize(m_context, m_isUsingBindUniform);
    }
    return m_textureProgram.get();
}

const LayerRendererChromium::TextureProgramFlip* LayerRendererChromium::textureProgramFlip()
{
    if (!m_textureProgramFlip)
        m_textureProgramFlip = adoptPtr(new TextureProgramFlip(m_context));
    if (!m_textureProgramFlip->initialized()) {
        TRACE_EVENT0("cc", "LayerRendererChromium::textureProgramFlip::initialize");
        m_textureProgramFlip->initialize(m_context, m_isUsingBindUniform);
    }
    return m_textureProgramFlip.get();
}

const LayerRendererChromium::TextureIOSurfaceProgram* LayerRendererChromium::textureIOSurfaceProgram()
{
    if (!m_textureIOSurfaceProgram)
        m_textureIOSurfaceProgram = adoptPtr(new TextureIOSurfaceProgram(m_context));
    if (!m_textureIOSurfaceProgram->initialized()) {
        TRACE_EVENT0("cc", "LayerRendererChromium::textureIOSurfaceProgram::initialize");
        m_textureIOSurfaceProgram->initialize(m_context, m_isUsingBindUniform);
    }
    return m_textureIOSurfaceProgram.get();
}

const LayerRendererChromium::VideoYUVProgram* LayerRendererChromium::videoYUVProgram()
{
    if (!m_videoYUVProgram)
        m_videoYUVProgram = adoptPtr(new VideoYUVProgram(m_context));
    if (!m_videoYUVProgram->initialized()) {
        TRACE_EVENT0("cc", "LayerRendererChromium::videoYUVProgram::initialize");
        m_videoYUVProgram->initialize(m_context, m_isUsingBindUniform);
    }
    return m_videoYUVProgram.get();
}

const LayerRendererChromium::VideoStreamTextureProgram* LayerRendererChromium::videoStreamTextureProgram()
{
    if (!m_videoStreamTextureProgram)
        m_videoStreamTextureProgram = adoptPtr(new VideoStreamTextureProgram(m_context));
    if (!m_videoStreamTextureProgram->initialized()) {
        TRACE_EVENT0("cc", "LayerRendererChromium::streamTextureProgram::initialize");
        m_videoStreamTextureProgram->initialize(m_context, m_isUsingBindUniform);
    }
    return m_videoStreamTextureProgram.get();
}

void LayerRendererChromium::cleanupSharedObjects()
{
    makeContextCurrent();

    m_sharedGeometry.clear();

    if (m_tileProgram)
        m_tileProgram->cleanup(m_context);
    if (m_tileProgramOpaque)
        m_tileProgramOpaque->cleanup(m_context);
    if (m_tileProgramSwizzle)
        m_tileProgramSwizzle->cleanup(m_context);
    if (m_tileProgramSwizzleOpaque)
        m_tileProgramSwizzleOpaque->cleanup(m_context);
    if (m_tileProgramAA)
        m_tileProgramAA->cleanup(m_context);
    if (m_tileProgramSwizzleAA)
        m_tileProgramSwizzleAA->cleanup(m_context);
    if (m_tileCheckerboardProgram)
        m_tileCheckerboardProgram->cleanup(m_context);

    if (m_renderPassMaskProgram)
        m_renderPassMaskProgram->cleanup(m_context);
    if (m_renderPassProgram)
        m_renderPassProgram->cleanup(m_context);
    if (m_renderPassMaskProgramAA)
        m_renderPassMaskProgramAA->cleanup(m_context);
    if (m_renderPassProgramAA)
        m_renderPassProgramAA->cleanup(m_context);

    if (m_textureProgram)
        m_textureProgram->cleanup(m_context);
    if (m_textureProgramFlip)
        m_textureProgramFlip->cleanup(m_context);
    if (m_textureIOSurfaceProgram)
        m_textureIOSurfaceProgram->cleanup(m_context);

    if (m_videoYUVProgram)
        m_videoYUVProgram->cleanup(m_context);
    if (m_videoStreamTextureProgram)
        m_videoStreamTextureProgram->cleanup(m_context);

    if (m_solidColorProgram)
        m_solidColorProgram->cleanup(m_context);

    if (m_headsUpDisplayProgram)
        m_headsUpDisplayProgram->cleanup(m_context);

    if (m_offscreenFramebufferId)
        GLC(m_context, m_context->deleteFramebuffer(m_offscreenFramebufferId));

    m_textureCopier.clear();
    m_textureUploader.clear();

    releaseRenderPassTextures();
}

bool LayerRendererChromium::isContextLost()
{
    return (m_context->getGraphicsResetStatusARB() != GraphicsContext3D::NO_ERROR);
}

} // namespace WebCore

#endif // USE(ACCELERATED_COMPOSITING)
