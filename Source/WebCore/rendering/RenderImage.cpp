/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Dirk Mueller (mueller@kde.org)
 *           (C) 2006 Allan Sandfeld Jensen (kde@carewolf.com)
 *           (C) 2006 Samuel Weinig (sam.weinig@gmail.com)
 * Copyright (C) 2003, 2004, 2005, 2006, 2008, 2009, 2010, 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"
#include "RenderImage.h"

#include "BitmapImage.h"
#include "FontCache.h"
#include "Frame.h"
#include "FrameSelection.h"
#include "GraphicsContext.h"
#include "HTMLAreaElement.h"
#include "HTMLImageElement.h"
#include "HTMLInputElement.h"
#include "HTMLMapElement.h"
#include "HTMLNames.h"
#include "HitTestResult.h"
#include "Page.h"
#include "RenderLayer.h"
#include "RenderView.h"
#include "SVGImage.h"
#include <wtf/UnusedParam.h>

using namespace std;

namespace WebCore {

using namespace HTMLNames;

RenderImage::RenderImage(Node* node)
    : RenderReplaced(node, IntSize())
    , m_needsToSetSizeForAltText(false)
    , m_didIncrementVisuallyNonEmptyPixelCount(false)
{
    updateAltText();
}

RenderImage::~RenderImage()
{
    ASSERT(m_imageResource);
    m_imageResource->shutdown();
}

void RenderImage::setImageResource(PassOwnPtr<RenderImageResource> imageResource)
{
    ASSERT(!m_imageResource);
    m_imageResource = imageResource;
    m_imageResource->initialize(this);
}

// If we'll be displaying either alt text or an image, add some padding.
static const unsigned short paddingWidth = 4;
static const unsigned short paddingHeight = 4;

// Alt text is restricted to this maximum size, in pixels.  These are
// signed integers because they are compared with other signed values.
static const float maxAltTextWidth = 1024;
static const int maxAltTextHeight = 256;

IntSize RenderImage::imageSizeForError(CachedImage* newImage) const
{
    ASSERT_ARG(newImage, newImage);
    ASSERT_ARG(newImage, newImage->imageForRenderer(this));

    IntSize imageSize;
    if (newImage->willPaintBrokenImage()) {
        float deviceScaleFactor = WebCore::deviceScaleFactor(frame());
        pair<Image*, float> brokenImageAndImageScaleFactor = newImage->brokenImage(deviceScaleFactor);
        imageSize = brokenImageAndImageScaleFactor.first->size();
        imageSize.scale(1 / brokenImageAndImageScaleFactor.second);
    } else
        imageSize = newImage->imageForRenderer(this)->size();

    // imageSize() returns 0 for the error image. We need the true size of the
    // error image, so we have to get it by grabbing image() directly.
    return IntSize(paddingWidth + imageSize.width() * style()->effectiveZoom(), paddingHeight + imageSize.height() * style()->effectiveZoom());
}

// Sets the image height and width to fit the alt text.  Returns true if the
// image size changed.
bool RenderImage::setImageSizeForAltText(CachedImage* newImage /* = 0 */)
{
    IntSize imageSize;
    if (newImage && newImage->imageForRenderer(this))
        imageSize = imageSizeForError(newImage);
    else if (!m_altText.isEmpty() || newImage) {
        // If we'll be displaying either text or an image, add a little padding.
        imageSize = IntSize(paddingWidth, paddingHeight);
    }

    // we have an alt and the user meant it (its not a text we invented)
    if (!m_altText.isEmpty()) {
        FontCachePurgePreventer fontCachePurgePreventer;

        const Font& font = style()->font();
        IntSize textSize(min(font.width(RenderBlock::constructTextRun(this, font, m_altText, style())), maxAltTextWidth), min(font.fontMetrics().height(), maxAltTextHeight));
        imageSize = imageSize.expandedTo(textSize);
    }

    if (imageSize == intrinsicSize())
        return false;

    setIntrinsicSize(imageSize);
    return true;
}

void RenderImage::styleDidChange(StyleDifference diff, const RenderStyle* oldStyle)
{
    RenderReplaced::styleDidChange(diff, oldStyle);
    if (m_needsToSetSizeForAltText) {
        if (!m_altText.isEmpty() && setImageSizeForAltText(m_imageResource->cachedImage()))
            imageDimensionsChanged(true /* imageSizeChanged */);
        m_needsToSetSizeForAltText = false;
    }
}

void RenderImage::imageChanged(WrappedImagePtr newImage, const IntRect* rect)
{
    if (documentBeingDestroyed())
        return;

    if (hasBoxDecorations() || hasMask())
        RenderReplaced::imageChanged(newImage, rect);
    
    if (!m_imageResource)
        return;

    if (newImage != m_imageResource->imagePtr() || !newImage)
        return;
    
    if (!m_didIncrementVisuallyNonEmptyPixelCount) {
        view()->frameView()->incrementVisuallyNonEmptyPixelCount(m_imageResource->imageSize(1.0f));
        m_didIncrementVisuallyNonEmptyPixelCount = true;
    }

    bool imageSizeChanged = false;

    // Set image dimensions, taking into account the size of the alt text.
    if (m_imageResource->errorOccurred()) {
        if (!m_altText.isEmpty() && document()->isPendingStyleRecalc()) {
            ASSERT(node());
            if (node()) {
                m_needsToSetSizeForAltText = true;
                node()->setNeedsStyleRecalc(SyntheticStyleChange);
            }
            return;
        }
        imageSizeChanged = setImageSizeForAltText(m_imageResource->cachedImage());
    }

    imageDimensionsChanged(imageSizeChanged, rect);
}

bool RenderImage::updateIntrinsicSizeIfNeeded(const LayoutSize& newSize, bool imageSizeChanged)
{
    if (newSize == intrinsicSize() && !imageSizeChanged)
        return false;
    if (m_imageResource->errorOccurred())
        return imageSizeChanged;
    setIntrinsicSize(newSize);
    return true;
}

void RenderImage::imageDimensionsChanged(bool imageSizeChanged, const IntRect* rect)
{
    bool shouldRepaint = true;
    if (updateIntrinsicSizeIfNeeded(m_imageResource->imageSize(style()->effectiveZoom()), imageSizeChanged)) {
        // In the case of generated image content using :before/:after, we might not be in the
        // render tree yet.  In that case, we don't need to worry about check for layout, since we'll get a
        // layout when we get added in to the render tree hierarchy later.
        if (containingBlock()) {
            // lets see if we need to relayout at all..
            int oldwidth = width();
            int oldheight = height();
            if (!preferredLogicalWidthsDirty())
                setPreferredLogicalWidthsDirty(true);
            computeLogicalWidth();
            computeLogicalHeight();

            if (imageSizeChanged || width() != oldwidth || height() != oldheight) {
                shouldRepaint = false;
                if (!selfNeedsLayout())
                    setNeedsLayout(true);
            }

            setWidth(oldwidth);
            setHeight(oldheight);
        }
    }

    if (shouldRepaint) {
        LayoutRect repaintRect;
        if (rect) {
            // The image changed rect is in source image coordinates (pre-zooming),
            // so map from the bounds of the image to the contentsBox.
            repaintRect = enclosingIntRect(mapRect(*rect, FloatRect(FloatPoint(), m_imageResource->imageSize(1.0f)), contentBoxRect()));
            // Guard against too-large changed rects.
            repaintRect.intersect(contentBoxRect());
        } else
            repaintRect = contentBoxRect();
        
        repaintRectangle(repaintRect);

#if USE(ACCELERATED_COMPOSITING)
        if (hasLayer()) {
            // Tell any potential compositing layers that the image needs updating.
            layer()->contentChanged(RenderLayer::ImageChanged);
        }
#endif
    }
}

void RenderImage::notifyFinished(CachedResource* newImage)
{
    if (!m_imageResource)
        return;
    
    if (documentBeingDestroyed())
        return;

#if USE(ACCELERATED_COMPOSITING)
    if (newImage == m_imageResource->cachedImage() && hasLayer()) {
        // tell any potential compositing layers
        // that the image is done and they can reference it directly.
        layer()->contentChanged(RenderLayer::ImageChanged);
    }
#else
    UNUSED_PARAM(newImage);
#endif
}

void RenderImage::paintReplaced(PaintInfo& paintInfo, const LayoutPoint& paintOffset)
{
    LayoutUnit cWidth = contentWidth();
    LayoutUnit cHeight = contentHeight();
    LayoutUnit leftBorder = borderLeft();
    LayoutUnit topBorder = borderTop();
    LayoutUnit leftPad = paddingLeft();
    LayoutUnit topPad = paddingTop();

    GraphicsContext* context = paintInfo.context;

    if (!m_imageResource->hasImage() || m_imageResource->errorOccurred()) {
        if (paintInfo.phase == PaintPhaseSelection)
            return;

        if (cWidth > 2 && cHeight > 2) {
            // Draw an outline rect where the image should be.
            context->setStrokeStyle(SolidStroke);
            context->setStrokeColor(Color::lightGray, style()->colorSpace());
            context->setFillColor(Color::transparent, style()->colorSpace());
            context->drawRect(LayoutRect(paintOffset.x() + leftBorder + leftPad, paintOffset.y() + topBorder + topPad, cWidth, cHeight));

            bool errorPictureDrawn = false;
            LayoutSize imageOffset;
            // When calculating the usable dimensions, exclude the pixels of
            // the ouline rect so the error image/alt text doesn't draw on it.
            LayoutUnit usableWidth = cWidth - 2;
            LayoutUnit usableHeight = cHeight - 2;

            RefPtr<Image> image = m_imageResource->image();

            if (m_imageResource->errorOccurred() && !image->isNull() && usableWidth >= image->width() && usableHeight >= image->height()) {
                float deviceScaleFactor = WebCore::deviceScaleFactor(frame());
                // Call brokenImage() explicitly to ensure we get the broken image icon at the appropriate resolution.
                pair<Image*, float> brokenImageAndImageScaleFactor = m_imageResource->cachedImage()->brokenImage(deviceScaleFactor);
                image = brokenImageAndImageScaleFactor.first;
                IntSize imageSize = image->size();
                imageSize.scale(1 / brokenImageAndImageScaleFactor.second);
                // Center the error image, accounting for border and padding.
                LayoutUnit centerX = (usableWidth - imageSize.width()) / 2;
                if (centerX < 0)
                    centerX = 0;
                LayoutUnit centerY = (usableHeight - imageSize.height()) / 2;
                if (centerY < 0)
                    centerY = 0;
                imageOffset = LayoutSize(leftBorder + leftPad + centerX + 1, topBorder + topPad + centerY + 1);
                context->drawImage(image.get(), style()->colorSpace(), IntRect(paintOffset + imageOffset, imageSize));
                errorPictureDrawn = true;
            }

            if (!m_altText.isEmpty()) {
                String text = document()->displayStringModifiedByEncoding(m_altText);
                context->setFillColor(style()->visitedDependentColor(CSSPropertyColor), style()->colorSpace());
                const Font& font = style()->font();
                const FontMetrics& fontMetrics = font.fontMetrics();
                LayoutUnit ascent = fontMetrics.ascent();
                LayoutPoint altTextOffset = paintOffset;
                altTextOffset.move(leftBorder + leftPad, topBorder + topPad + ascent);

                // Only draw the alt text if it'll fit within the content box,
                // and only if it fits above the error image.
                TextRun textRun = RenderBlock::constructTextRun(this, font, text, style());
                LayoutUnit textWidth = font.width(textRun);
                if (errorPictureDrawn) {
                    if (usableWidth >= textWidth && fontMetrics.height() <= imageOffset.height())
                        context->drawText(font, textRun, altTextOffset);
                } else if (usableWidth >= textWidth && cHeight >= fontMetrics.height())
                    context->drawText(font, textRun, altTextOffset);
            }
        }
    } else if (m_imageResource->hasImage() && cWidth > 0 && cHeight > 0) {
        RefPtr<Image> img = m_imageResource->image(cWidth, cHeight);
        if (!img || img->isNull())
            return;

#if PLATFORM(MAC)
        if (style()->highlight() != nullAtom && !paintInfo.context->paintingDisabled())
            paintCustomHighlight(toPoint(paintOffset - location()), style()->highlight(), true);
#endif

        LayoutSize contentSize(cWidth, cHeight);
        LayoutPoint contentLocation = paintOffset;
        contentLocation.move(leftBorder + leftPad, topBorder + topPad);
        paintIntoRect(context, LayoutRect(contentLocation, contentSize));
    }
}

void RenderImage::paint(PaintInfo& paintInfo, const LayoutPoint& paintOffset)
{
    RenderReplaced::paint(paintInfo, paintOffset);
    
    if (paintInfo.phase == PaintPhaseOutline)
        paintAreaElementFocusRing(paintInfo);
}
    
void RenderImage::paintAreaElementFocusRing(PaintInfo& paintInfo)
{
    Document* document = this->document();
    
    if (document->printing() || !document->frame()->selection()->isFocusedAndActive())
        return;
    
    if (paintInfo.context->paintingDisabled() && !paintInfo.context->updatingControlTints())
        return;

    Node* focusedNode = document->focusedNode();
    if (!focusedNode || !focusedNode->hasTagName(areaTag))
        return;

    HTMLAreaElement* areaElement = static_cast<HTMLAreaElement*>(focusedNode);
    if (areaElement->imageElement() != node())
        return;

    // Even if the theme handles focus ring drawing for entire elements, it won't do it for
    // an area within an image, so we don't call RenderTheme::supportsFocusRing here.

    Path path = areaElement->computePath(this);
    if (path.isEmpty())
        return;

    // FIXME: Do we need additional code to clip the path to the image's bounding box?

    RenderStyle* areaElementStyle = areaElement->computedStyle();
    unsigned short outlineWidth = areaElementStyle->outlineWidth();
    if (!outlineWidth)
        return;

    paintInfo.context->drawFocusRing(path, outlineWidth,
        areaElementStyle->outlineOffset(),
        areaElementStyle->visitedDependentColor(CSSPropertyOutlineColor));
}

void RenderImage::areaElementFocusChanged(HTMLAreaElement* element)
{
    ASSERT_UNUSED(element, element->imageElement() == node());

    // It would be more efficient to only repaint the focus ring rectangle
    // for the passed-in area element. That would require adding functions
    // to the area element class.
    repaint();
}

void RenderImage::paintIntoRect(GraphicsContext* context, const LayoutRect& rect)
{
    if (!m_imageResource->hasImage() || m_imageResource->errorOccurred() || rect.width() <= 0 || rect.height() <= 0)
        return;

    RefPtr<Image> img = m_imageResource->image(rect.width(), rect.height());
    if (!img || img->isNull())
        return;

    HTMLImageElement* imageElt = (node() && node()->hasTagName(imgTag)) ? static_cast<HTMLImageElement*>(node()) : 0;
    CompositeOperator compositeOperator = imageElt ? imageElt->compositeOperator() : CompositeSourceOver;
    Image* image = m_imageResource->image().get();
    bool useLowQualityScaling = shouldPaintAtLowQuality(context, image, image, rect.size());
    context->drawImage(m_imageResource->image(rect.width(), rect.height()).get(), style()->colorSpace(), rect, compositeOperator, useLowQualityScaling);
}

bool RenderImage::backgroundIsObscured() const
{
    if (!m_imageResource->hasImage() || m_imageResource->errorOccurred())
        return false;

    if (m_imageResource->cachedImage() && !m_imageResource->cachedImage()->isLoaded())
        return false;

    EFillBox backgroundClip = style()->backgroundClip();

    // Background paints under borders.
    if (backgroundClip == BorderFillBox && style()->hasBorder() && !borderObscuresBackground())
        return false;

    // Background shows in padding area.
    if ((backgroundClip == BorderFillBox || backgroundClip == PaddingFillBox) && style()->hasPadding())
        return false;

    // Check for bitmap image with alpha.
    Image* image = m_imageResource->image().get();
    if (!image || !image->isBitmapImage() || image->currentFrameHasAlpha())
        return false;
        
    return true;
}

int RenderImage::minimumReplacedHeight() const
{
    return m_imageResource->errorOccurred() ? intrinsicSize().height() : 0;
}

HTMLMapElement* RenderImage::imageMap() const
{
    HTMLImageElement* i = node() && node()->hasTagName(imgTag) ? static_cast<HTMLImageElement*>(node()) : 0;
    return i ? i->treeScope()->getImageMap(i->fastGetAttribute(usemapAttr)) : 0;
}

bool RenderImage::nodeAtPoint(const HitTestRequest& request, HitTestResult& result, const LayoutPoint& pointInContainer, const LayoutPoint& accumulatedOffset, HitTestAction hitTestAction)
{
    HitTestResult tempResult(result.point(), result.topPadding(), result.rightPadding(), result.bottomPadding(), result.leftPadding());
    bool inside = RenderReplaced::nodeAtPoint(request, tempResult, pointInContainer, accumulatedOffset, hitTestAction);

    if (tempResult.innerNode() && node()) {
        if (HTMLMapElement* map = imageMap()) {
            LayoutRect contentBox = contentBoxRect();
            float scaleFactor = 1 / style()->effectiveZoom();
            LayoutPoint mapLocation(pointInContainer.x() - accumulatedOffset.x() - this->x() - contentBox.x(), pointInContainer.y() - accumulatedOffset.y() - this->y() - contentBox.y());
            mapLocation.scale(scaleFactor, scaleFactor);
            
            if (map->mapMouseEvent(mapLocation, contentBox.size(), tempResult))
                tempResult.setInnerNonSharedNode(node());
        }
    }

    if (!inside && result.isRectBasedTest())
        result.append(tempResult);
    if (inside)
        result = tempResult;
    return inside;
}

void RenderImage::updateAltText()
{
    if (!node())
        return;

    if (node()->hasTagName(inputTag))
        m_altText = static_cast<HTMLInputElement*>(node())->altText();
    else if (node()->hasTagName(imgTag))
        m_altText = static_cast<HTMLImageElement*>(node())->altText();
}

LayoutUnit RenderImage::computeReplacedLogicalWidth(bool includeMaxWidth) const
{
    // If we've got an explicit width/height assigned, propagate it to the image resource.    
    if (style()->logicalWidth().isFixed() && style()->logicalHeight().isFixed()) {
        LayoutUnit width = RenderReplaced::computeReplacedLogicalWidth(includeMaxWidth);
        m_imageResource->setContainerSizeForRenderer(IntSize(width, computeReplacedLogicalHeight()));
        return width;
    }

    RenderBox* contentRenderer = embeddedContentBox();
    bool hasRelativeWidth = contentRenderer ? contentRenderer->style()->width().isPercent() : m_imageResource->imageHasRelativeWidth();
    bool hasRelativeHeight = contentRenderer ? contentRenderer->style()->height().isPercent() : m_imageResource->imageHasRelativeHeight();

    IntSize containerSize;
    if (hasRelativeWidth || hasRelativeHeight) {
        // Propagate the containing block size to the image resource, otherwhise we can't compute our own intrinsic size, if it's relative.
        RenderObject* containingBlock = isPositioned() ? container() : this->containingBlock();
        if (containingBlock->isBox()) {
            RenderBox* box = toRenderBox(containingBlock);
            containerSize = IntSize(box->availableWidth(), box->availableHeight()); // Already contains zooming information.
        }
    } else {
        // Propagate the current zoomed image size to the image resource, otherwhise the image size will remain the same on-screen.
        CachedImage* cachedImage = m_imageResource->cachedImage();
        if (cachedImage && cachedImage->image()) {
            containerSize = cachedImage->image()->size();
            // FIXME: Remove unnecessary rounding when layout is off ints: webkit.org/b/63656
            containerSize.setWidth(static_cast<LayoutUnit>(containerSize.width() * style()->effectiveZoom()));
            containerSize.setHeight(static_cast<LayoutUnit>(containerSize.height() * style()->effectiveZoom()));
        }
    }

    if (!containerSize.isEmpty()) {
        m_imageResource->setContainerSizeForRenderer(containerSize);
        const_cast<RenderImage*>(this)->updateIntrinsicSizeIfNeeded(containerSize, false);
    }

    return RenderReplaced::computeReplacedLogicalWidth(includeMaxWidth);
}

void RenderImage::computeIntrinsicRatioInformation(FloatSize& intrinsicRatio, bool& isPercentageIntrinsicSize) const
{
    // Assure this method is never used for SVGImages.
    ASSERT(!embeddedContentBox());
    isPercentageIntrinsicSize = false;
    CachedImage* cachedImage = m_imageResource ? m_imageResource->cachedImage() : 0;
    if (cachedImage && cachedImage->image())
        intrinsicRatio = cachedImage->image()->size();
}

bool RenderImage::needsPreferredWidthsRecalculation() const
{
    if (RenderReplaced::needsPreferredWidthsRecalculation())
        return true;
    return embeddedContentBox();
}

RenderBox* RenderImage::embeddedContentBox() const
{
    if (!m_imageResource)
        return 0;

#if ENABLE(SVG)
    CachedImage* cachedImage = m_imageResource->cachedImage();
    if (cachedImage && cachedImage->image() && cachedImage->image()->isSVGImage())
        return static_cast<SVGImage*>(cachedImage->image())->embeddedContentBox();
#endif

    return 0;
}

} // namespace WebCore
