/*
 * Copyright (C) 2006 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2008, 2009 Apple Inc. All rights reserved.
 * Copyright (C) Research In Motion Limited 2011. All rights reserved.
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

#include "config.h"
#include "SVGImage.h"

#include "Chrome.h"
#include "DocumentLoader.h"
#include "ElementIterator.h"
#include "FrameView.h"
#include "ImageBuffer.h"
#include "ImageObserver.h"
#include "IntRect.h"
#include "MainFrame.h"
#include "RenderSVGRoot.h"
#include "RenderStyle.h"
#include "SVGDocument.h"
#include "SVGForeignObjectElement.h"
#include "SVGImageChromeClient.h"
#include "SVGSVGElement.h"
#include "Settings.h"

namespace WebCore {

SVGImage::SVGImage(ImageObserver* observer)
    : Image(observer)
{
}

SVGImage::~SVGImage()
{
    if (m_page) {
        // Store m_page in a local variable, clearing m_page, so that SVGImageChromeClient knows we're destructed.
        std::unique_ptr<Page> currentPage = WTF::move(m_page);
        currentPage->mainFrame().loader().frameDetached(); // Break both the loader and view references to the frame
    }

    // Verify that page teardown destroyed the Chrome
    ASSERT(!m_chromeClient || !m_chromeClient->image());
}

bool SVGImage::hasSingleSecurityOrigin() const
{
    if (!m_page)
        return true;

    SVGSVGElement* rootElement = toSVGDocument(m_page->mainFrame().document())->rootElement();
    if (!rootElement)
        return true;

    // Don't allow foreignObject elements since they can leak information with arbitrary HTML (like spellcheck or control theme).
    if (descendantsOfType<SVGForeignObjectElement>(*rootElement).first())
        return false;

    // Because SVG image rendering disallows external resources and links,
    // these images effectively are restricted to a single security origin.
    return true;
}

void SVGImage::setContainerSize(const FloatSize& size)
{
    if (!m_page || !usesContainerSize())
        return;

    SVGSVGElement* rootElement = toSVGDocument(m_page->mainFrame().document())->rootElement();
    if (!rootElement)
        return;
    RenderSVGRoot* renderer = toRenderSVGRoot(rootElement->renderer());
    if (!renderer)
        return;

    FrameView* view = frameView();
    view->resize(this->containerSize());

    renderer->setContainerSize(IntSize(size));
}

IntSize SVGImage::containerSize() const
{
    if (!m_page)
        return IntSize();
    SVGSVGElement* rootElement = toSVGDocument(m_page->mainFrame().document())->rootElement();
    if (!rootElement)
        return IntSize();

    RenderSVGRoot* renderer = toRenderSVGRoot(rootElement->renderer());
    if (!renderer)
        return IntSize();

    // If a container size is available it has precedence.
    IntSize containerSize = renderer->containerSize();
    if (!containerSize.isEmpty())
        return containerSize;

    // Assure that a container size is always given for a non-identity zoom level.
    ASSERT(renderer->style().effectiveZoom() == 1);

    FloatSize currentSize;
    if (rootElement->hasIntrinsicWidth() && rootElement->hasIntrinsicHeight())
        currentSize = rootElement->currentViewportSize();
    else
        currentSize = rootElement->currentViewBoxRect().size();

    if (!currentSize.isEmpty())
        return IntSize(static_cast<int>(ceilf(currentSize.width())), static_cast<int>(ceilf(currentSize.height())));

    // As last resort, use CSS default intrinsic size.
    return IntSize(300, 150);
}

void SVGImage::drawForContainer(GraphicsContext* context, const FloatSize containerSize, float zoom, const FloatRect& dstRect,
    const FloatRect& srcRect, ColorSpace colorSpace, CompositeOperator compositeOp, BlendMode blendMode)
{
    if (!m_page)
        return;

    ImageObserver* observer = imageObserver();
    ASSERT(observer);

    // Temporarily reset image observer, we don't want to receive any changeInRect() calls due to this relayout.
    setImageObserver(0);

    IntSize roundedContainerSize = roundedIntSize(containerSize);
    setContainerSize(roundedContainerSize);

    FloatRect scaledSrc = srcRect;
    scaledSrc.scale(1 / zoom);

    // Compensate for the container size rounding by adjusting the source rect.
    FloatSize adjustedSrcSize = scaledSrc.size();
    adjustedSrcSize.scale(roundedContainerSize.width() / containerSize.width(), roundedContainerSize.height() / containerSize.height());
    scaledSrc.setSize(adjustedSrcSize);

    draw(context, dstRect, scaledSrc, colorSpace, compositeOp, blendMode, ImageOrientationDescription());

    setImageObserver(observer);
}

#if USE(CAIRO)
// Passes ownership of the native image to the caller so PassNativeImagePtr needs
// to be a smart pointer type.
PassNativeImagePtr SVGImage::nativeImageForCurrentFrame()
{
    if (!m_page)
        return 0;

    std::unique_ptr<ImageBuffer> buffer = ImageBuffer::create(size(), 1);
    if (!buffer) // failed to allocate image
        return 0;

    draw(buffer->context(), rect(), rect(), ColorSpaceDeviceRGB, CompositeSourceOver, BlendModeNormal, ImageOrientationDescription());

    // FIXME: WK(Bug 113657): We should use DontCopyBackingStore here.
    return buffer->copyImage(CopyBackingStore)->nativeImageForCurrentFrame();
}
#endif

void SVGImage::drawPatternForContainer(GraphicsContext* context, const FloatSize containerSize, float zoom, const FloatRect& srcRect,
    const AffineTransform& patternTransform, const FloatPoint& phase, ColorSpace colorSpace, CompositeOperator compositeOp, const FloatRect& dstRect, BlendMode blendMode)
{
    FloatRect zoomedContainerRect = FloatRect(FloatPoint(), containerSize);
    zoomedContainerRect.scale(zoom);

    // The ImageBuffer size needs to be scaled to match the final resolution.
    AffineTransform transform = context->getCTM();
    FloatSize imageBufferScale = FloatSize(transform.xScale(), transform.yScale());
    ASSERT(imageBufferScale.width());
    ASSERT(imageBufferScale.height());

    FloatRect imageBufferSize = zoomedContainerRect;
    imageBufferSize.scale(imageBufferScale.width(), imageBufferScale.height());

    std::unique_ptr<ImageBuffer> buffer = ImageBuffer::create(expandedIntSize(imageBufferSize.size()), 1);
    if (!buffer) // Failed to allocate buffer.
        return;
    drawForContainer(buffer->context(), containerSize, zoom, imageBufferSize, zoomedContainerRect, ColorSpaceDeviceRGB, CompositeSourceOver, BlendModeNormal);
    if (context->drawLuminanceMask())
        buffer->convertToLuminanceMask();

    RefPtr<Image> image = buffer->copyImage(DontCopyBackingStore, Unscaled);
    image->setSpaceSize(spaceSize());

    // Adjust the source rect and transform due to the image buffer's scaling.
    FloatRect scaledSrcRect = srcRect;
    scaledSrcRect.scale(imageBufferScale.width(), imageBufferScale.height());
    AffineTransform unscaledPatternTransform(patternTransform);
    unscaledPatternTransform.scale(1 / imageBufferScale.width(), 1 / imageBufferScale.height());

    context->setDrawLuminanceMask(false);
    image->drawPattern(context, scaledSrcRect, unscaledPatternTransform, phase, colorSpace, compositeOp, dstRect, blendMode);
}

void SVGImage::draw(GraphicsContext* context, const FloatRect& dstRect, const FloatRect& srcRect, ColorSpace, CompositeOperator compositeOp, BlendMode blendMode, ImageOrientationDescription)
{
    if (!m_page)
        return;

    FrameView* view = frameView();
    ASSERT(view);

    GraphicsContextStateSaver stateSaver(*context);
    context->setCompositeOperation(compositeOp, blendMode);
    context->clip(enclosingIntRect(dstRect));
    bool compositingRequiresTransparencyLayer = compositeOp != CompositeSourceOver || blendMode != BlendModeNormal;
    if (compositingRequiresTransparencyLayer) {
        context->beginTransparencyLayer(1);
        context->setCompositeOperation(CompositeSourceOver, BlendModeNormal);
    }

    FloatSize scale(dstRect.width() / srcRect.width(), dstRect.height() / srcRect.height());
    
    // We can only draw the entire frame, clipped to the rect we want. So compute where the top left
    // of the image would be if we were drawing without clipping, and translate accordingly.
    FloatSize topLeftOffset(srcRect.location().x() * scale.width(), srcRect.location().y() * scale.height());
    FloatPoint destOffset = dstRect.location() - topLeftOffset;

    context->translate(destOffset.x(), destOffset.y());
    context->scale(scale);

    view->resize(containerSize());

    if (view->needsLayout())
        view->layout();

    view->paint(context, enclosingIntRect(srcRect));

    if (compositingRequiresTransparencyLayer)
        context->endTransparencyLayer();

    stateSaver.restore();

    if (!m_url.isEmpty())
        view->scrollToFragment(m_url);

    if (imageObserver())
        imageObserver()->didDraw(this);
}

RenderBox* SVGImage::embeddedContentBox() const
{
    if (!m_page)
        return 0;
    SVGSVGElement* rootElement = toSVGDocument(m_page->mainFrame().document())->rootElement();
    if (!rootElement)
        return 0;
    return toRenderBox(rootElement->renderer());
}

FrameView* SVGImage::frameView() const
{
    if (!m_page)
        return 0;
    return m_page->mainFrame().view();
}

bool SVGImage::hasRelativeWidth() const
{
    if (!m_page)
        return false;
    SVGSVGElement* rootElement = toSVGDocument(m_page->mainFrame().document())->rootElement();
    if (!rootElement)
        return false;
    return rootElement->intrinsicWidth().isPercent();
}

bool SVGImage::hasRelativeHeight() const
{
    if (!m_page)
        return false;
    SVGSVGElement* rootElement = toSVGDocument(m_page->mainFrame().document())->rootElement();
    if (!rootElement)
        return false;
    return rootElement->intrinsicHeight().isPercent();
}

void SVGImage::computeIntrinsicDimensions(Length& intrinsicWidth, Length& intrinsicHeight, FloatSize& intrinsicRatio)
{
    if (!m_page)
        return;
    SVGSVGElement* rootElement = toSVGDocument(m_page->mainFrame().document())->rootElement();
    if (!rootElement)
        return;

    intrinsicWidth = rootElement->intrinsicWidth();
    intrinsicHeight = rootElement->intrinsicHeight();
    if (rootElement->preserveAspectRatio().align() == SVGPreserveAspectRatio::SVG_PRESERVEASPECTRATIO_NONE)
        return;

    intrinsicRatio = rootElement->viewBox().size();
    if (intrinsicRatio.isEmpty() && intrinsicWidth.isFixed() && intrinsicHeight.isFixed())
        intrinsicRatio = FloatSize(floatValueForLength(intrinsicWidth, 0), floatValueForLength(intrinsicHeight, 0));
}

// FIXME: support catchUpIfNecessary.
void SVGImage::startAnimation(CatchUpAnimation)
{
    if (!m_page)
        return;
    SVGSVGElement* rootElement = toSVGDocument(m_page->mainFrame().document())->rootElement();
    if (!rootElement)
        return;
    rootElement->unpauseAnimations();
    rootElement->setCurrentTime(0);
}

void SVGImage::stopAnimation()
{
    if (!m_page)
        return;
    SVGSVGElement* rootElement = toSVGDocument(m_page->mainFrame().document())->rootElement();
    if (!rootElement)
        return;
    rootElement->pauseAnimations();
}

void SVGImage::resetAnimation()
{
    stopAnimation();
}

bool SVGImage::dataChanged(bool allDataReceived)
{
    // Don't do anything if is an empty image.
    if (!data()->size())
        return true;

    if (allDataReceived) {
        Page::PageClients pageClients;
        fillWithEmptyClients(pageClients);
        m_chromeClient = std::make_unique<SVGImageChromeClient>(this);
        pageClients.chromeClient = m_chromeClient.get();

        // FIXME: If this SVG ends up loading itself, we might leak the world.
        // The Cache code does not know about CachedImages holding Frames and
        // won't know to break the cycle.
        // This will become an issue when SVGImage will be able to load other
        // SVGImage objects, but we're safe now, because SVGImage can only be
        // loaded by a top-level document.
        m_page = std::make_unique<Page>(pageClients);
        m_page->settings().setMediaEnabled(false);
        m_page->settings().setScriptEnabled(false);
        m_page->settings().setPluginsEnabled(false);

        Frame& frame = m_page->mainFrame();
        frame.setView(FrameView::create(frame));
        frame.init();
        FrameLoader& loader = frame.loader();
        loader.forceSandboxFlags(SandboxAll);

        frame.view()->setCanHaveScrollbars(false); // SVG Images will always synthesize a viewBox, if it's not available, and thus never see scrollbars.
        frame.view()->setTransparent(true); // SVG Images are transparent.

        ASSERT(loader.activeDocumentLoader()); // DocumentLoader should have been created by frame->init().
        loader.activeDocumentLoader()->writer().setMIMEType("image/svg+xml");
        loader.activeDocumentLoader()->writer().begin(URL()); // create the empty document
        loader.activeDocumentLoader()->writer().addData(data()->data(), data()->size());
        loader.activeDocumentLoader()->writer().end();

        // Set the intrinsic size before a container size is available.
        m_intrinsicSize = containerSize();
    }

    return m_page != nullptr;
}

String SVGImage::filenameExtension() const
{
    return "svg";
}

bool isInSVGImage(const Element* element)
{
    ASSERT(element);

    Page* page = element->document().page();
    if (!page)
        return false;

    return page->chrome().client().isSVGImageChromeClient();
}

}
