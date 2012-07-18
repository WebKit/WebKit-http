/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 * Copyright (C) 2009 Apple Inc. All rights reserved.
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


/** FIXME
 * This file borrows code heavily from platform/graphics/win/GraphicsLayerCACF.cpp
 * (and hence it includes both copyrights)
 * Ideally the common code (mostly the code that keeps track of the layer hierarchy)
 * should be kept separate and shared between platforms. It would be a well worthwhile
 * effort once the Windows implementation (binaries and headers) of CoreAnimation is
 * checked in to the WebKit repository. Until then only Apple can make this happen.
 */

#include "config.h"

#if USE(ACCELERATED_COMPOSITING)

#include "GraphicsLayerChromium.h"

#include "AnimationIdVendor.h"
#include "AnimationTranslationUtil.h"
#include "ContentLayerChromium.h"
#include "FloatConversion.h"
#include "FloatRect.h"
#include "GraphicsContext.h"
#include "Image.h"
#include "LayerChromium.h"
#include "LinkHighlight.h"
#include "NativeImageSkia.h"
#include "PlatformContextSkia.h"
#include "PlatformString.h"
#include "SkMatrix44.h"
#include "SystemTime.h"

#include "cc/CCActiveAnimation.h"

#include <public/WebFilterOperation.h>
#include <public/WebFilterOperations.h>
#include <public/WebFloatPoint.h>
#include <public/WebFloatRect.h>
#include <public/WebImageLayer.h>
#include <public/WebSize.h>
#include <public/WebTransformationMatrix.h>
#include <wtf/CurrentTime.h>
#include <wtf/StringExtras.h>
#include <wtf/text/CString.h>

using namespace std;
using namespace WebKit;

namespace WebCore {

PassOwnPtr<GraphicsLayer> GraphicsLayer::create(GraphicsLayerClient* client)
{
    return adoptPtr(new GraphicsLayerChromium(client));
}

GraphicsLayerChromium::GraphicsLayerChromium(GraphicsLayerClient* client)
    : GraphicsLayer(client)
    , m_contentsLayerPurpose(NoContentsLayer)
    , m_contentsLayerHasBackgroundColor(false)
    , m_inSetChildren(false)
    , m_pageScaleChanged(false)
{
    m_opaqueRectTrackingContentLayerDelegate = adoptPtr(new OpaqueRectTrackingContentLayerDelegate(this));
    m_layer = WebContentLayer::create(m_opaqueRectTrackingContentLayerDelegate.get());
    m_layer.setDrawsContent(m_drawsContent && m_contentsVisible);
    if (client)
        deviceOrPageScaleFactorChanged();
    updateDebugIndicators();
}

GraphicsLayerChromium::~GraphicsLayerChromium()
{
    // Do cleanup while we can still safely call methods on the derived class.
    willBeDestroyed();
}

void GraphicsLayerChromium::willBeDestroyed()
{
    if (!m_layer.isNull()) {
        m_layer.clearClient();
        m_layer.unwrap<LayerChromium>()->clearRenderSurface();
        m_layer.unwrap<LayerChromium>()->setLayerAnimationDelegate(0);
    }

    if (!m_contentsLayer.isNull()) {
        m_contentsLayer.unwrap<LayerChromium>()->clearRenderSurface();
        m_contentsLayer.unwrap<LayerChromium>()->setLayerAnimationDelegate(0);
    }

    if (!m_transformLayer.isNull()) {
        m_transformLayer.unwrap<LayerChromium>()->clearRenderSurface();
        m_transformLayer.unwrap<LayerChromium>()->setLayerAnimationDelegate(0);
    }

    if (m_linkHighlight)
        m_linkHighlight.clear();

    GraphicsLayer::willBeDestroyed();
}

void GraphicsLayerChromium::setName(const String& inName)
{
    m_nameBase = inName;
    String name = String::format("GraphicsLayerChromium(%p) GraphicsLayer(%p) ", m_layer.unwrap<LayerChromium>(), this) + inName;
    GraphicsLayer::setName(name);
    updateNames();
}

void GraphicsLayerChromium::updateNames()
{
    if (!m_layer.isNull())
        m_layer.unwrap<LayerChromium>()->setDebugName("Layer for " + m_nameBase);
    if (!m_transformLayer.isNull())
        m_transformLayer.unwrap<LayerChromium>()->setDebugName("TransformLayer for " + m_nameBase);
    if (!m_contentsLayer.isNull())
        m_contentsLayer.unwrap<LayerChromium>()->setDebugName("ContentsLayer for " + m_nameBase);
    if (m_linkHighlight)
        m_linkHighlight->contentLayer()->setDebugName("LinkHighlight for " + m_nameBase);
}

bool GraphicsLayerChromium::setChildren(const Vector<GraphicsLayer*>& children)
{
    m_inSetChildren = true;
    bool childrenChanged = GraphicsLayer::setChildren(children);

    if (childrenChanged)
        updateChildList();
    m_inSetChildren = false;

    return childrenChanged;
}

void GraphicsLayerChromium::addChild(GraphicsLayer* childLayer)
{
    GraphicsLayer::addChild(childLayer);
    if (!m_inSetChildren)
        updateChildList();
}

void GraphicsLayerChromium::addChildAtIndex(GraphicsLayer* childLayer, int index)
{
    GraphicsLayer::addChildAtIndex(childLayer, index);
    updateChildList();
}

void GraphicsLayerChromium::addChildBelow(GraphicsLayer* childLayer, GraphicsLayer* sibling)
{
    GraphicsLayer::addChildBelow(childLayer, sibling);
    updateChildList();
}

void GraphicsLayerChromium::addChildAbove(GraphicsLayer* childLayer, GraphicsLayer *sibling)
{
    GraphicsLayer::addChildAbove(childLayer, sibling);
    updateChildList();
}

bool GraphicsLayerChromium::replaceChild(GraphicsLayer* oldChild, GraphicsLayer* newChild)
{
    if (GraphicsLayer::replaceChild(oldChild, newChild)) {
        updateChildList();
        return true;
    }
    return false;
}

void GraphicsLayerChromium::removeFromParent()
{
    GraphicsLayer::removeFromParent();
    layerForParent().removeFromParent();
}

void GraphicsLayerChromium::setPosition(const FloatPoint& point)
{
    GraphicsLayer::setPosition(point);
    updateLayerPosition();
}

void GraphicsLayerChromium::setAnchorPoint(const FloatPoint3D& point)
{
    GraphicsLayer::setAnchorPoint(point);
    updateAnchorPoint();
}

void GraphicsLayerChromium::setSize(const FloatSize& size)
{
    // We are receiving negative sizes here that cause assertions to fail in the compositor. Clamp them to 0 to
    // avoid those assertions.
    // FIXME: This should be an ASSERT instead, as negative sizes should not exist in WebCore.
    FloatSize clampedSize = size;
    if (clampedSize.width() < 0 || clampedSize.height() < 0)
        clampedSize = FloatSize();

    if (clampedSize == m_size)
        return;

    GraphicsLayer::setSize(clampedSize);
    updateLayerSize();

    if (m_pageScaleChanged && !m_layer.isNull())
        m_layer.invalidate();
    m_pageScaleChanged = false;
}

void GraphicsLayerChromium::setTransform(const TransformationMatrix& transform)
{
    // Call this method first to assign contents scale to our layer so the painter can apply the scale transform.
    updateContentsScale();

    GraphicsLayer::setTransform(transform);
    updateTransform();
}

void GraphicsLayerChromium::setChildrenTransform(const TransformationMatrix& transform)
{
    GraphicsLayer::setChildrenTransform(transform);
    updateChildrenTransform();
}

void GraphicsLayerChromium::setPreserves3D(bool preserves3D)
{
    if (preserves3D == m_preserves3D)
        return;

    GraphicsLayer::setPreserves3D(preserves3D);
    updateLayerPreserves3D();
}

void GraphicsLayerChromium::setMasksToBounds(bool masksToBounds)
{
    GraphicsLayer::setMasksToBounds(masksToBounds);
    updateMasksToBounds();
}

void GraphicsLayerChromium::setDrawsContent(bool drawsContent)
{
    // Note carefully this early-exit is only correct because we also properly initialize
    // LayerChromium::m_isDrawable whenever m_contentsLayer is set to a new layer in setupContentsLayer().
    if (drawsContent == m_drawsContent)
        return;

    GraphicsLayer::setDrawsContent(drawsContent);
    updateLayerIsDrawable();
}

void GraphicsLayerChromium::setContentsVisible(bool contentsVisible)
{
    // Note carefully this early-exit is only correct because we also properly initialize
    // LayerChromium::m_isDrawable whenever m_contentsLayer is set to a new layer in setupContentsLayer().
    if (contentsVisible == m_contentsVisible)
        return;

    GraphicsLayer::setContentsVisible(contentsVisible);
    updateLayerIsDrawable();
}

void GraphicsLayerChromium::setBackgroundColor(const Color& color)
{
    GraphicsLayer::setBackgroundColor(color.rgb());

    m_contentsLayerHasBackgroundColor = true;
    updateLayerBackgroundColor();
}

void GraphicsLayerChromium::clearBackgroundColor()
{
    GraphicsLayer::clearBackgroundColor();
    m_contentsLayer.setBackgroundColor(static_cast<RGBA32>(0));
}

void GraphicsLayerChromium::setContentsOpaque(bool opaque)
{
    GraphicsLayer::setContentsOpaque(opaque);
    m_layer.setOpaque(m_contentsOpaque);
}

static bool copyWebCoreFilterOperationsToWebFilterOperations(const FilterOperations& filters, WebFilterOperations& webFilters)
{
    for (size_t i = 0; i < filters.size(); ++i) {
        const FilterOperation& op = *filters.at(i);
        switch (op.getOperationType()) {
        case FilterOperation::REFERENCE:
            return false; // Not supported.
        case FilterOperation::GRAYSCALE:
        case FilterOperation::SEPIA:
        case FilterOperation::SATURATE:
        case FilterOperation::HUE_ROTATE: {
            float amount = static_cast<const BasicColorMatrixFilterOperation*>(&op)->amount();
            switch (op.getOperationType()) {
            case FilterOperation::GRAYSCALE:
                webFilters.append(WebFilterOperation::createGrayscaleFilter(amount));
                break;
            case FilterOperation::SEPIA:
                webFilters.append(WebFilterOperation::createSepiaFilter(amount));
                break;
            case FilterOperation::SATURATE:
                webFilters.append(WebFilterOperation::createSaturateFilter(amount));
                break;
            case FilterOperation::HUE_ROTATE:
                webFilters.append(WebFilterOperation::createHueRotateFilter(amount));
                break;
            default:
                ASSERT_NOT_REACHED();
            }
            break;
        }
        case FilterOperation::INVERT:
        case FilterOperation::OPACITY:
        case FilterOperation::BRIGHTNESS:
        case FilterOperation::CONTRAST: {
            float amount = static_cast<const BasicComponentTransferFilterOperation*>(&op)->amount();
            switch (op.getOperationType()) {
            case FilterOperation::INVERT:
                webFilters.append(WebFilterOperation::createInvertFilter(amount));
                break;
            case FilterOperation::OPACITY:
                webFilters.append(WebFilterOperation::createOpacityFilter(amount));
                break;
            case FilterOperation::BRIGHTNESS:
                webFilters.append(WebFilterOperation::createBrightnessFilter(amount));
                break;
            case FilterOperation::CONTRAST:
                webFilters.append(WebFilterOperation::createContrastFilter(amount));
                break;
            default:
                ASSERT_NOT_REACHED();
            }
            break;
        }
        case FilterOperation::BLUR: {
            float pixelRadius = static_cast<const BlurFilterOperation*>(&op)->stdDeviation().getFloatValue();
            webFilters.append(WebFilterOperation::createBlurFilter(pixelRadius));
            break;
        }
        case FilterOperation::DROP_SHADOW: {
            const DropShadowFilterOperation& dropShadowOp = *static_cast<const DropShadowFilterOperation*>(&op);
            webFilters.append(WebFilterOperation::createDropShadowFilter(WebPoint(dropShadowOp.x(), dropShadowOp.y()), dropShadowOp.stdDeviation(), dropShadowOp.color().rgb()));
            break;
        }
#if ENABLE(CSS_SHADERS)
        case FilterOperation::CUSTOM:
            return false; // Not supported.
#endif
        case FilterOperation::PASSTHROUGH:
        case FilterOperation::NONE:
            break;
        }
    }
    return true;
}

bool GraphicsLayerChromium::setFilters(const FilterOperations& filters)
{
    WebFilterOperations webFilters;
    if (!copyWebCoreFilterOperationsToWebFilterOperations(filters, webFilters))
        return false;
    m_layer.setFilters(webFilters);
    return GraphicsLayer::setFilters(filters);
}

void GraphicsLayerChromium::setBackgroundFilters(const FilterOperations& filters)
{
    WebFilterOperations webFilters;
    if (!copyWebCoreFilterOperationsToWebFilterOperations(filters, webFilters))
        return;
    m_layer.setBackgroundFilters(webFilters);
}

void GraphicsLayerChromium::setMaskLayer(GraphicsLayer* maskLayer)
{
    if (maskLayer == m_maskLayer)
        return;

    GraphicsLayer::setMaskLayer(maskLayer);

    LayerChromium* maskLayerChromium = m_maskLayer ? m_maskLayer->platformLayer() : 0;
    m_layer.unwrap<LayerChromium>()->setMaskLayer(maskLayerChromium);
}

void GraphicsLayerChromium::setBackfaceVisibility(bool visible)
{
    GraphicsLayer::setBackfaceVisibility(visible);
    m_layer.setDoubleSided(m_backfaceVisibility);
}

void GraphicsLayerChromium::setOpacity(float opacity)
{
    float clampedOpacity = max(min(opacity, 1.0f), 0.0f);
    GraphicsLayer::setOpacity(clampedOpacity);
    primaryLayer().setOpacity(opacity);
}

void GraphicsLayerChromium::setReplicatedByLayer(GraphicsLayer* layer)
{
    GraphicsLayerChromium* layerChromium = static_cast<GraphicsLayerChromium*>(layer);
    GraphicsLayer::setReplicatedByLayer(layer);
    LayerChromium* replicaLayer = layerChromium ? layerChromium->primaryLayer().unwrap<LayerChromium>() : 0;
    primaryLayer().unwrap<LayerChromium>()->setReplicaLayer(replicaLayer);
}


void GraphicsLayerChromium::setContentsNeedsDisplay()
{
    if (!m_contentsLayer.isNull())
        m_contentsLayer.invalidate();
}

void GraphicsLayerChromium::setNeedsDisplay()
{
    if (drawsContent())
        m_layer.invalidate();
}

void GraphicsLayerChromium::setNeedsDisplayInRect(const FloatRect& rect)
{
    if (drawsContent())
        m_layer.invalidateRect(rect);
}

void GraphicsLayerChromium::setContentsRect(const IntRect& rect)
{
    if (rect == m_contentsRect)
        return;

    GraphicsLayer::setContentsRect(rect);
    updateContentsRect();
}

void GraphicsLayerChromium::setContentsToImage(Image* image)
{
    bool childrenChanged = false;
    if (image) {
        if (m_contentsLayer.isNull() || m_contentsLayerPurpose != ContentsLayerForImage) {
            WebKit::WebImageLayer imageLayer = WebKit::WebImageLayer::create();
            setupContentsLayer(imageLayer.unwrap<LayerChromium>());
            m_contentsLayerPurpose = ContentsLayerForImage;
            childrenChanged = true;
        }
        WebKit::WebImageLayer imageLayer = m_contentsLayer.to<WebKit::WebImageLayer>();
        NativeImageSkia* nativeImage = image->nativeImageForCurrentFrame();
        imageLayer.setBitmap(nativeImage->bitmap());
        imageLayer.setOpaque(image->isBitmapImage() && !image->currentFrameHasAlpha());
        updateContentsRect();
    } else {
        if (!m_contentsLayer.isNull()) {
            childrenChanged = true;

            // The old contents layer will be removed via updateChildList.
            m_contentsLayer.reset();
        }
    }

    if (childrenChanged)
        updateChildList();
}

void GraphicsLayerChromium::setContentsToCanvas(PlatformLayer* platformLayer)
{
    bool childrenChanged = false;
    if (platformLayer) {
        if (m_contentsLayer.unwrap<LayerChromium>() != platformLayer) {
            setupContentsLayer(platformLayer);
            m_contentsLayerPurpose = ContentsLayerForCanvas;
            childrenChanged = true;
        }
        updateContentsRect();
    } else {
        if (!m_contentsLayer.isNull()) {
            childrenChanged = true;

            // The old contents layer will be removed via updateChildList.
            m_contentsLayer.reset();
        }
    }

    if (childrenChanged)
        updateChildList();
}

bool GraphicsLayerChromium::addAnimation(const KeyframeValueList& values, const IntSize& boxSize, const Animation* animation, const String& animationName, double timeOffset)
{
    primaryLayer().unwrap<LayerChromium>()->setLayerAnimationDelegate(this);

    int animationId = mapAnimationNameToId(animationName);
    int groupId = AnimationIdVendor::getNextGroupId();

    OwnPtr<CCActiveAnimation> toAdd(createActiveAnimation(values, animation, animationId, groupId, timeOffset, boxSize));

    if (toAdd.get()) {
        // Remove any existing animations with the same animation id and target property.
        primaryLayer().unwrap<LayerChromium>()->layerAnimationController()->removeAnimation(toAdd->id(), toAdd->targetProperty());
        return primaryLayer().unwrap<LayerChromium>()->addAnimation(toAdd.release());
    }

    return false;
}

void GraphicsLayerChromium::pauseAnimation(const String& animationName, double timeOffset)
{
    primaryLayer().unwrap<LayerChromium>()->pauseAnimation(mapAnimationNameToId(animationName), timeOffset);
}

void GraphicsLayerChromium::removeAnimation(const String& animationName)
{
    primaryLayer().unwrap<LayerChromium>()->removeAnimation(mapAnimationNameToId(animationName));
}

void GraphicsLayerChromium::suspendAnimations(double wallClockTime)
{
    // |wallClockTime| is in the wrong time base. Need to convert here.
    // FIXME: find a more reliable way to do this.
    double monotonicTime = wallClockTime + monotonicallyIncreasingTime() - currentTime();
    primaryLayer().unwrap<LayerChromium>()->suspendAnimations(monotonicTime);
}

void GraphicsLayerChromium::resumeAnimations()
{
    primaryLayer().unwrap<LayerChromium>()->resumeAnimations(monotonicallyIncreasingTime());
}

void GraphicsLayerChromium::addLinkHighlight(const Path& path)
{
    m_linkHighlight = LinkHighlight::create(this, path, AnimationIdVendor::LinkHighlightAnimationId, AnimationIdVendor::getNextGroupId());
    updateChildList();
}

void GraphicsLayerChromium::didFinishLinkHighlight()
{
    if (m_linkHighlight)
        m_linkHighlight->contentLayer()->removeFromParent();

    m_linkHighlight.clear();
}

void GraphicsLayerChromium::setContentsToMedia(PlatformLayer* layer)
{
    bool childrenChanged = false;
    if (layer) {
        if (m_contentsLayer.isNull() || m_contentsLayerPurpose != ContentsLayerForVideo) {
            setupContentsLayer(layer);
            m_contentsLayerPurpose = ContentsLayerForVideo;
            childrenChanged = true;
        }
        updateContentsRect();
    } else {
        if (!m_contentsLayer.isNull()) {
            childrenChanged = true;

            // The old contents layer will be removed via updateChildList.
            m_contentsLayer.reset();
        }
    }

    if (childrenChanged)
        updateChildList();
}

WebLayer GraphicsLayerChromium::hostLayerForChildren() const
{
    return m_transformLayer.isNull() ? m_layer :  m_transformLayer;
}

WebLayer GraphicsLayerChromium::layerForParent() const
{
    return m_transformLayer.isNull() ? m_layer :  m_transformLayer;
}

PlatformLayer* GraphicsLayerChromium::platformLayer() const
{
    return primaryLayer().unwrap<LayerChromium>();
}

void GraphicsLayerChromium::setDebugBackgroundColor(const Color& color)
{
    if (color.isValid())
        m_layer.setBackgroundColor(color.rgb());
    else
        m_layer.setBackgroundColor(static_cast<RGBA32>(0));
}

void GraphicsLayerChromium::setDebugBorder(const Color& color, float borderWidth)
{
    if (color.isValid()) {
        m_layer.setDebugBorderColor(color.rgb());
        m_layer.setDebugBorderWidth(borderWidth);
    } else {
        m_layer.setDebugBorderColor(static_cast<RGBA32>(0));
        m_layer.setDebugBorderWidth(0);
    }
}

void GraphicsLayerChromium::updateChildList()
{
    Vector<RefPtr<LayerChromium> > newChildren;

    if (!m_transformLayer.isNull()) {
        // Add the primary layer first. Even if we have negative z-order children, the primary layer always comes behind.
        newChildren.append(m_layer.unwrap<LayerChromium>());
    } else if (!m_contentsLayer.isNull()) {
        // FIXME: add the contents layer in the correct order with negative z-order children.
        // This does not cause visible rendering issues because currently contents layers are only used
        // for replaced elements that don't have children.
        newChildren.append(m_contentsLayer.unwrap<LayerChromium>());
    }

    const Vector<GraphicsLayer*>& childLayers = children();
    size_t numChildren = childLayers.size();
    for (size_t i = 0; i < numChildren; ++i) {
        GraphicsLayerChromium* curChild = static_cast<GraphicsLayerChromium*>(childLayers[i]);

        LayerChromium* childLayer = curChild->layerForParent().unwrap<LayerChromium>();
        newChildren.append(childLayer);
    }

    if (m_linkHighlight)
        newChildren.append(m_linkHighlight->contentLayer());

    for (size_t i = 0; i < newChildren.size(); ++i)
        newChildren[i]->removeFromParent();

    if (!m_transformLayer.isNull()) {
        m_transformLayer.unwrap<LayerChromium>()->setChildren(newChildren);

        if (!m_contentsLayer.isNull()) {
            // If we have a transform layer, then the contents layer is parented in the
            // primary layer (which is itself a child of the transform layer).
            m_layer.removeAllChildren();
            m_layer.addChild(m_contentsLayer);
        }
    } else
        m_layer.unwrap<LayerChromium>()->setChildren(newChildren);
}

void GraphicsLayerChromium::updateLayerPosition()
{
    primaryLayer().setPosition(m_position);
}

void GraphicsLayerChromium::updateLayerSize()
{
    IntSize layerSize(m_size.width(), m_size.height());
    if (!m_transformLayer.isNull()) {
        m_transformLayer.setBounds(layerSize);
        m_layer.setPosition(FloatPoint());
    }

    m_layer.setBounds(layerSize);

    // Note that we don't resize m_contentsLayer. It's up the caller to do that.
}

void GraphicsLayerChromium::updateAnchorPoint()
{
    primaryLayer().setAnchorPoint(FloatPoint(m_anchorPoint.x(), m_anchorPoint.y()));
    primaryLayer().setAnchorPointZ(m_anchorPoint.z());
}

void GraphicsLayerChromium::updateTransform()
{
    primaryLayer().setTransform(WebTransformationMatrix(m_transform));
}

void GraphicsLayerChromium::updateChildrenTransform()
{
    primaryLayer().setSublayerTransform(WebTransformationMatrix(m_childrenTransform));
}

void GraphicsLayerChromium::updateMasksToBounds()
{
    m_layer.setMasksToBounds(m_masksToBounds);
    updateDebugIndicators();
}

void GraphicsLayerChromium::updateLayerPreserves3D()
{
    if (m_preserves3D && m_transformLayer.isNull()) {
        // Create the transform layer.
        m_transformLayer = WebLayer::create();
        m_transformLayer.setPreserves3D(true);
        m_transformLayer.unwrap<LayerChromium>()->setLayerAnimationDelegate(this);
        m_transformLayer.unwrap<LayerChromium>()->setLayerAnimationController(m_layer.unwrap<LayerChromium>()->releaseLayerAnimationController());

        // Copy the position from this layer.
        updateLayerPosition();
        updateLayerSize();
        updateAnchorPoint();
        updateTransform();
        updateChildrenTransform();

        m_layer.setPosition(FloatPoint::zero());

        m_layer.setAnchorPoint(FloatPoint(0.5f, 0.5f));
        m_layer.setTransform(SkMatrix44());

        // Set the old layer to opacity of 1. Further down we will set the opacity on the transform layer.
        m_layer.setOpacity(1);

        m_layer.setContentsScale(contentsScale());

        // Move this layer to be a child of the transform layer.
        if (!m_layer.parent().isNull())
            m_layer.parent().replaceChild(m_layer, m_transformLayer);
        m_transformLayer.addChild(m_layer);

        updateChildList();
    } else if (!m_preserves3D && !m_transformLayer.isNull()) {
        // Relace the transformLayer in the parent with this layer.
        m_layer.removeFromParent();
        if (!m_transformLayer.parent().isNull())
            m_transformLayer.parent().replaceChild(m_transformLayer, m_layer);

        m_layer.unwrap<LayerChromium>()->setLayerAnimationDelegate(this);
        m_layer.unwrap<LayerChromium>()->setLayerAnimationController(m_transformLayer.unwrap<LayerChromium>()->releaseLayerAnimationController());

        // Release the transform layer.
        m_transformLayer.unwrap<LayerChromium>()->setLayerAnimationDelegate(0);
        m_transformLayer.reset();

        updateLayerPosition();
        updateLayerSize();
        updateAnchorPoint();
        updateTransform();
        updateChildrenTransform();

        updateChildList();
    }

    m_layer.setPreserves3D(m_preserves3D);
    primaryLayer().setOpacity(m_opacity);
    updateNames();
}

void GraphicsLayerChromium::updateLayerIsDrawable()
{
    // For the rest of the accelerated compositor code, there is no reason to make a
    // distinction between drawsContent and contentsVisible. So, for m_layer, these two
    // flags are combined here. m_contentsLayer shouldn't receive the drawsContent flag
    // so it is only given contentsVisible.

    m_layer.setDrawsContent(m_drawsContent && m_contentsVisible);

    if (!m_contentsLayer.isNull())
        m_contentsLayer.setDrawsContent(m_contentsVisible);

    if (m_drawsContent)
        m_layer.invalidate();

    updateDebugIndicators();
}

void GraphicsLayerChromium::updateLayerBackgroundColor()
{
    if (m_contentsLayer.isNull())
        return;

    // We never create the contents layer just for background color yet.
    if (m_backgroundColorSet)
        m_contentsLayer.setBackgroundColor(m_backgroundColor.rgb());
    else
        m_contentsLayer.setBackgroundColor(static_cast<RGBA32>(0));
}

void GraphicsLayerChromium::updateContentsVideo()
{
    // FIXME: Implement
}

void GraphicsLayerChromium::updateContentsRect()
{
    if (m_contentsLayer.isNull())
        return;

    m_contentsLayer.setPosition(FloatPoint(m_contentsRect.x(), m_contentsRect.y()));
    m_contentsLayer.setBounds(IntSize(m_contentsRect.width(), m_contentsRect.height()));
}

void GraphicsLayerChromium::updateContentsScale()
{
    // If page scale is already applied then there's no need to apply it again.
    if (appliesPageScale() || m_layer.isNull())
        return;

    m_layer.setContentsScale(contentsScale());
}

void GraphicsLayerChromium::setupContentsLayer(LayerChromium* contentsLayer)
{
    if (contentsLayer == m_contentsLayer.unwrap<LayerChromium>())
        return;

    if (!m_contentsLayer.isNull()) {
        m_contentsLayer.setUseParentBackfaceVisibility(false);
        m_contentsLayer.removeFromParent();
        m_contentsLayer.reset();
    }

    if (contentsLayer) {
        m_contentsLayer = WebLayer(contentsLayer);

        m_contentsLayer.setAnchorPoint(FloatPoint(0, 0));
        m_contentsLayer.setUseParentBackfaceVisibility(true);

        // It is necessary to call setDrawsContent as soon as we receive the new contentsLayer, for
        // the correctness of early exit conditions in setDrawsContent() and setContentsVisible().
        m_contentsLayer.setDrawsContent(m_contentsVisible);

        // Insert the content layer first. Video elements require this, because they have
        // shadow content that must display in front of the video.
        m_layer.insertChild(m_contentsLayer, 0);

        if (showDebugBorders()) {
            m_contentsLayer.setDebugBorderColor(Color(0, 0, 128, 180).rgb());
            m_contentsLayer.setDebugBorderWidth(1);
        }
    }
    updateDebugIndicators();
    updateNames();
}

float GraphicsLayerChromium::contentsScale() const
{
    if (!appliesPageScale())
        return pageScaleFactor() * deviceScaleFactor();
    return 1;
}

void GraphicsLayerChromium::deviceOrPageScaleFactorChanged()
{
    updateContentsScale();
    // Invalidations are clamped to the layer's bounds but we receive the scale changed notification before receiving
    // the new layer bounds. When the scale changes, we really want to invalidate the post-scale layer bounds, so we
    // remember that the scale has changed and then invalidate the full layer bounds when we receive the new size.
    m_pageScaleChanged = true;
}

void GraphicsLayerChromium::paint(GraphicsContext& context, const IntRect& clip)
{
    context.platformContext()->setDrawingToImageBuffer(true);
    paintGraphicsLayerContents(context, clip);
}

int GraphicsLayerChromium::mapAnimationNameToId(const String& animationName)
{
    if (animationName.isEmpty())
        return 0;

    if (!m_animationIdMap.contains(animationName))
        m_animationIdMap.add(animationName, AnimationIdVendor::getNextAnimationId());

    return m_animationIdMap.find(animationName)->second;
}

void GraphicsLayerChromium::notifyAnimationStarted(double startTime)
{
    if (m_client)
        m_client->notifyAnimationStarted(this, startTime);
}

void GraphicsLayerChromium::notifyAnimationFinished(double)
{
    // Do nothing.
}

} // namespace WebCore

#endif // USE(ACCELERATED_COMPOSITING)
