/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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
#import "PlatformCALayerRemote.h"

#import "PlatformCALayerRemoteCustom.h"
#import "PlatformCALayerRemoteTiledBacking.h"
#import "RemoteLayerBackingStore.h"
#import "RemoteLayerTreeContext.h"
#import "RemoteLayerTreePropertyApplier.h"
#import <WebCore/AnimationUtilities.h>
#import <WebCore/GraphicsContext.h>
#import <WebCore/GraphicsLayerCA.h>
#import <WebCore/LengthFunctions.h>
#import <WebCore/PlatformCAFilters.h>
#import <WebCore/PlatformCALayerMac.h>
#import <WebCore/TiledBacking.h>
#import <wtf/CurrentTime.h>
#import <wtf/RetainPtr.h>

using namespace WebCore;

namespace WebKit {

PassRefPtr<PlatformCALayerRemote> PlatformCALayerRemote::create(LayerType layerType, PlatformCALayerClient* owner, RemoteLayerTreeContext& context)
{
    RefPtr<PlatformCALayerRemote> layer;

    if (layerType == LayerTypeTiledBackingLayer ||  layerType == LayerTypePageTiledBackingLayer)
        layer = adoptRef(new PlatformCALayerRemoteTiledBacking(layerType, owner, context));
    else
        layer = adoptRef(new PlatformCALayerRemote(layerType, owner, context));

    context.layerWasCreated(*layer, layerType);

    return layer.release();
}

PassRefPtr<PlatformCALayerRemote> PlatformCALayerRemote::create(PlatformLayer *platformLayer, PlatformCALayerClient* owner, RemoteLayerTreeContext& context)
{
    return PlatformCALayerRemoteCustom::create(platformLayer, owner, context);
}

PassRefPtr<PlatformCALayerRemote> PlatformCALayerRemote::create(const PlatformCALayerRemote& other, WebCore::PlatformCALayerClient* owner, RemoteLayerTreeContext& context)
{
    RefPtr<PlatformCALayerRemote> layer = adoptRef(new PlatformCALayerRemote(other, owner, context));

    context.layerWasCreated(*layer, other.layerType());

    return layer.release();
}

PlatformCALayerRemote::PlatformCALayerRemote(LayerType layerType, PlatformCALayerClient* owner, RemoteLayerTreeContext& context)
    : PlatformCALayer(layerType, owner)
    , m_superlayer(nullptr)
    , m_maskLayer(nullptr)
    , m_acceleratesDrawing(false)
    , m_context(&context)
{
    if (owner)
        m_properties.contentsScale = owner->platformCALayerDeviceScaleFactor();
}

PlatformCALayerRemote::PlatformCALayerRemote(const PlatformCALayerRemote& other, PlatformCALayerClient* owner, RemoteLayerTreeContext& context)
    : PlatformCALayer(other.layerType(), owner)
    , m_superlayer(nullptr)
    , m_maskLayer(nullptr)
    , m_acceleratesDrawing(other.acceleratesDrawing())
    , m_context(&context)
{
}

PassRefPtr<PlatformCALayer> PlatformCALayerRemote::clone(PlatformCALayerClient* owner) const
{
    RefPtr<PlatformCALayerRemote> clone = PlatformCALayerRemote::create(*this, owner, *m_context);

    updateClonedLayerProperties(*clone);

    clone->setClonedLayer(this);
    return clone.release();
}

PlatformCALayerRemote::~PlatformCALayerRemote()
{
    for (const auto& layer : m_children)
        toPlatformCALayerRemote(layer.get())->m_superlayer = nullptr;

    if (m_context)
        m_context->layerWillBeDestroyed(*this);
}

void PlatformCALayerRemote::updateClonedLayerProperties(PlatformCALayerRemote& clone, bool copyContents) const
{
    clone.setPosition(position());
    clone.setBounds(bounds());
    clone.setAnchorPoint(anchorPoint());

    if (m_properties.transform)
        clone.setTransform(*m_properties.transform);

    if (m_properties.sublayerTransform)
        clone.setSublayerTransform(*m_properties.sublayerTransform);

    if (copyContents)
        clone.setContents(contents());

    clone.setMasksToBounds(masksToBounds());
    clone.setDoubleSided(isDoubleSided());
    clone.setOpaque(isOpaque());
    clone.setBackgroundColor(backgroundColor());
    clone.setContentsScale(contentsScale());
#if ENABLE(CSS_FILTERS)
    if (m_properties.filters)
        clone.copyFiltersFrom(this);
#endif
    clone.updateCustomAppearance(customAppearance());
}

void PlatformCALayerRemote::recursiveBuildTransaction(RemoteLayerTreeContext& context, RemoteLayerTreeTransaction& transaction)
{
    ASSERT(!m_properties.backingStore || owner());
    ASSERT_WITH_SECURITY_IMPLICATION(&context == m_context);
    
    if (m_properties.backingStore && (!owner() || !owner()->platformCALayerDrawsContent())) {
        m_properties.backingStore = nullptr;
        m_properties.notePropertiesChanged(RemoteLayerTreeTransaction::BackingStoreChanged);
    }

    if (m_properties.backingStore && m_properties.backingStore->display())
        m_properties.notePropertiesChanged(RemoteLayerTreeTransaction::BackingStoreChanged);

    if (m_properties.changedProperties != RemoteLayerTreeTransaction::NoChange) {
        if (m_properties.changedProperties & RemoteLayerTreeTransaction::ChildrenChanged) {
            m_properties.children.clear();
            for (const auto& layer : m_children)
                m_properties.children.append(layer->layerID());
        }

        if (isPlatformCALayerRemoteCustom()) {
            RemoteLayerTreePropertyApplier::applyProperties(platformLayer(), nullptr, m_properties, RemoteLayerTreePropertyApplier::RelatedLayerMap());
            didCommit();
            return;
        }

        transaction.layerPropertiesChanged(*this);
    }

    for (size_t i = 0; i < m_children.size(); ++i) {
        PlatformCALayerRemote* child = toPlatformCALayerRemote(m_children[i].get());
        ASSERT(child->superlayer() == this);
        child->recursiveBuildTransaction(context, transaction);
    }

    if (m_maskLayer)
        m_maskLayer->recursiveBuildTransaction(context, transaction);
}

void PlatformCALayerRemote::didCommit()
{
    m_properties.addedAnimations.clear();
    m_properties.keyPathsOfAnimationsToRemove.clear();
    m_properties.resetChangedProperties();
}

void PlatformCALayerRemote::ensureBackingStore()
{
    ASSERT(owner());
    
    if (!m_properties.backingStore)
        m_properties.backingStore = std::make_unique<RemoteLayerBackingStore>(this);

    updateBackingStore();
}

void PlatformCALayerRemote::updateBackingStore()
{
    if (!m_properties.backingStore)
        return;

    m_properties.backingStore->ensureBackingStore(m_properties.bounds.size(), m_properties.contentsScale, m_acceleratesDrawing, m_properties.opaque);
}

void PlatformCALayerRemote::setNeedsDisplay(const FloatRect* rect)
{
    ensureBackingStore();

    if (!rect) {
        m_properties.backingStore->setNeedsDisplay();
        return;
    }

    // FIXME: Need to map this through contentsRect/etc.
    m_properties.backingStore->setNeedsDisplay(enclosingIntRect(*rect));
}

void PlatformCALayerRemote::copyContentsFromLayer(PlatformCALayer* layer)
{
    ASSERT(m_properties.clonedLayerID == layer->layerID());
    
    if (!m_properties.changedProperties)
        m_context->layerPropertyChangedWhileBuildingTransaction(*this);

    m_properties.notePropertiesChanged(RemoteLayerTreeTransaction::ClonedContentsChanged);
}

PlatformCALayer* PlatformCALayerRemote::superlayer() const
{
    return m_superlayer;
}

void PlatformCALayerRemote::removeFromSuperlayer()
{
    if (!m_superlayer)
        return;

    m_superlayer->removeSublayer(this);
}

void PlatformCALayerRemote::removeSublayer(PlatformCALayerRemote* layer)
{
    size_t childIndex = m_children.find(layer);
    if (childIndex != notFound)
        m_children.remove(childIndex);
    layer->m_superlayer = nullptr;
    m_properties.notePropertiesChanged(RemoteLayerTreeTransaction::ChildrenChanged);
}

void PlatformCALayerRemote::setSublayers(const PlatformCALayerList& list)
{
    removeAllSublayers();
    m_children = list;

    for (const auto& layer : list) {
        layer->removeFromSuperlayer();
        toPlatformCALayerRemote(layer.get())->m_superlayer = this;
    }

    m_properties.notePropertiesChanged(RemoteLayerTreeTransaction::ChildrenChanged);
}

void PlatformCALayerRemote::removeAllSublayers()
{
    PlatformCALayerList layersToRemove = m_children;
    for (const auto& layer : layersToRemove)
        layer->removeFromSuperlayer();
    ASSERT(m_children.isEmpty());
    m_properties.notePropertiesChanged(RemoteLayerTreeTransaction::ChildrenChanged);
}

void PlatformCALayerRemote::appendSublayer(PlatformCALayer* layer)
{
    RefPtr<PlatformCALayer> layerProtector(layer);

    layer->removeFromSuperlayer();
    m_children.append(layer);
    toPlatformCALayerRemote(layer)->m_superlayer = this;
    m_properties.notePropertiesChanged(RemoteLayerTreeTransaction::ChildrenChanged);
}

void PlatformCALayerRemote::insertSublayer(PlatformCALayer* layer, size_t index)
{
    RefPtr<PlatformCALayer> layerProtector(layer);

    layer->removeFromSuperlayer();
    m_children.insert(index, layer);
    toPlatformCALayerRemote(layer)->m_superlayer = this;
    m_properties.notePropertiesChanged(RemoteLayerTreeTransaction::ChildrenChanged);
}

void PlatformCALayerRemote::replaceSublayer(PlatformCALayer* reference, PlatformCALayer* layer)
{
    ASSERT(reference->superlayer() == this);
    RefPtr<PlatformCALayer> layerProtector(layer);

    layer->removeFromSuperlayer();
    size_t referenceIndex = m_children.find(reference);
    if (referenceIndex != notFound) {
        m_children[referenceIndex]->removeFromSuperlayer();
        m_children.insert(referenceIndex, layer);
        toPlatformCALayerRemote(layer)->m_superlayer = this;
    }

    m_properties.notePropertiesChanged(RemoteLayerTreeTransaction::ChildrenChanged);
}

void PlatformCALayerRemote::adoptSublayers(PlatformCALayer* source)
{
    PlatformCALayerList layersToMove = toPlatformCALayerRemote(source)->m_children;

    if (const PlatformCALayerList* customLayers = source->customSublayers()) {
        for (const auto& layer : *customLayers) {
            size_t layerIndex = layersToMove.find(layer);
            if (layerIndex != notFound)
                layersToMove.remove(layerIndex);
        }
    }

    setSublayers(layersToMove);
}

void PlatformCALayerRemote::addAnimationForKey(const String& key, PlatformCAAnimation* animation)
{
    auto addResult = m_animations.set(key, animation);
    if (addResult.isNewEntry)
        m_properties.addedAnimations.append(std::pair<String, PlatformCAAnimationRemote::Properties>(key, toPlatformCAAnimationRemote(animation)->properties()));
    else {
        for (auto& keyAnimationPair : m_properties.addedAnimations) {
            if (keyAnimationPair.first == key) {
                keyAnimationPair.second = toPlatformCAAnimationRemote(animation)->properties();
                break;
            }
        }
    }
    
    m_properties.notePropertiesChanged(RemoteLayerTreeTransaction::AnimationsChanged);

    if (m_context)
        m_context->willStartAnimationOnLayer(*this);
}

void PlatformCALayerRemote::removeAnimationForKey(const String& key)
{
    if (m_animations.remove(key)) {
        for (size_t i = 0; i < m_properties.addedAnimations.size(); ++i) {
            if (m_properties.addedAnimations[i].first == key) {
                m_properties.addedAnimations.remove(i);
                break;
            }
        }
    }
    m_properties.keyPathsOfAnimationsToRemove.add(key);
    m_properties.notePropertiesChanged(RemoteLayerTreeTransaction::AnimationsChanged);
}

PassRefPtr<PlatformCAAnimation> PlatformCALayerRemote::animationForKey(const String& key)
{
    return m_animations.get(key);
}

void PlatformCALayerRemote::animationStarted(const String& key, CFTimeInterval beginTime)
{
    auto it = m_animations.find(key);
    if (it != m_animations.end())
        toPlatformCAAnimationRemote(it->value.get())->didStart(beginTime);
    
    if (m_owner)
        m_owner->platformCALayerAnimationStarted(beginTime);
}

void PlatformCALayerRemote::setMask(PlatformCALayer* layer)
{
    if (layer) {
        m_maskLayer = toPlatformCALayerRemote(layer);
        m_properties.maskLayerID = m_maskLayer->layerID();
    } else {
        m_maskLayer = nullptr;
        m_properties.maskLayerID = 0;
    }

    m_properties.notePropertiesChanged(RemoteLayerTreeTransaction::MaskLayerChanged);
}

void PlatformCALayerRemote::setClonedLayer(const PlatformCALayer* layer)
{
    if (layer)
        m_properties.clonedLayerID = layer->layerID();
    else
        m_properties.clonedLayerID = 0;

    m_properties.notePropertiesChanged(RemoteLayerTreeTransaction::ClonedContentsChanged);
}

bool PlatformCALayerRemote::isOpaque() const
{
    return m_properties.opaque;
}

void PlatformCALayerRemote::setOpaque(bool value)
{
    m_properties.opaque = value;
    m_properties.notePropertiesChanged(RemoteLayerTreeTransaction::OpaqueChanged);

    updateBackingStore();
}

FloatRect PlatformCALayerRemote::bounds() const
{
    return m_properties.bounds;
}

void PlatformCALayerRemote::setBounds(const FloatRect& value)
{
    if (value == m_properties.bounds)
        return;

    m_properties.bounds = value;
    m_properties.notePropertiesChanged(RemoteLayerTreeTransaction::BoundsChanged);
    
    if (requiresCustomAppearanceUpdateOnBoundsChange())
        m_properties.notePropertiesChanged(RemoteLayerTreeTransaction::CustomAppearanceChanged);

    updateBackingStore();
}

FloatPoint3D PlatformCALayerRemote::position() const
{
    return m_properties.position;
}

void PlatformCALayerRemote::setPosition(const FloatPoint3D& value)
{
    if (value == m_properties.position)
        return;

    m_properties.position = value;
    m_properties.notePropertiesChanged(RemoteLayerTreeTransaction::PositionChanged);
}

FloatPoint3D PlatformCALayerRemote::anchorPoint() const
{
    return m_properties.anchorPoint;
}

void PlatformCALayerRemote::setAnchorPoint(const FloatPoint3D& value)
{
    if (value == m_properties.anchorPoint)
        return;

    m_properties.anchorPoint = value;
    m_properties.notePropertiesChanged(RemoteLayerTreeTransaction::AnchorPointChanged);
}

TransformationMatrix PlatformCALayerRemote::transform() const
{
    return m_properties.transform ? *m_properties.transform : TransformationMatrix();
}

void PlatformCALayerRemote::setTransform(const TransformationMatrix& value)
{
    m_properties.transform = std::make_unique<TransformationMatrix>(value);
    m_properties.notePropertiesChanged(RemoteLayerTreeTransaction::TransformChanged);
}

TransformationMatrix PlatformCALayerRemote::sublayerTransform() const
{
    return m_properties.sublayerTransform ? *m_properties.sublayerTransform : TransformationMatrix();
}

void PlatformCALayerRemote::setSublayerTransform(const TransformationMatrix& value)
{
    m_properties.sublayerTransform = std::make_unique<TransformationMatrix>(value);
    m_properties.notePropertiesChanged(RemoteLayerTreeTransaction::SublayerTransformChanged);
}

void PlatformCALayerRemote::setHidden(bool value)
{
    m_properties.hidden = value;
    m_properties.notePropertiesChanged(RemoteLayerTreeTransaction::HiddenChanged);
}

void PlatformCALayerRemote::setGeometryFlipped(bool value)
{
    m_properties.geometryFlipped = value;
    m_properties.notePropertiesChanged(RemoteLayerTreeTransaction::GeometryFlippedChanged);
}

bool PlatformCALayerRemote::isDoubleSided() const
{
    return m_properties.doubleSided;
}

void PlatformCALayerRemote::setDoubleSided(bool value)
{
    m_properties.doubleSided = value;
    m_properties.notePropertiesChanged(RemoteLayerTreeTransaction::DoubleSidedChanged);
}

bool PlatformCALayerRemote::masksToBounds() const
{
    return m_properties.masksToBounds;
}

void PlatformCALayerRemote::setMasksToBounds(bool value)
{
    if (value == m_properties.masksToBounds)
        return;

    m_properties.masksToBounds = value;
    m_properties.notePropertiesChanged(RemoteLayerTreeTransaction::MasksToBoundsChanged);
}

bool PlatformCALayerRemote::acceleratesDrawing() const
{
    return m_acceleratesDrawing;
}

void PlatformCALayerRemote::setAcceleratesDrawing(bool acceleratesDrawing)
{
    m_acceleratesDrawing = acceleratesDrawing;
    updateBackingStore();
}

CFTypeRef PlatformCALayerRemote::contents() const
{
    return nullptr;
}

void PlatformCALayerRemote::setContents(CFTypeRef value)
{
}

void PlatformCALayerRemote::setContentsRect(const FloatRect& value)
{
    m_properties.contentsRect = value;
    m_properties.notePropertiesChanged(RemoteLayerTreeTransaction::ContentsRectChanged);
}

void PlatformCALayerRemote::setMinificationFilter(FilterType value)
{
    m_properties.minificationFilter = value;
    m_properties.notePropertiesChanged(RemoteLayerTreeTransaction::MinificationFilterChanged);
}

void PlatformCALayerRemote::setMagnificationFilter(FilterType value)
{
    m_properties.magnificationFilter = value;
    m_properties.notePropertiesChanged(RemoteLayerTreeTransaction::MagnificationFilterChanged);
}

Color PlatformCALayerRemote::backgroundColor() const
{
    return m_properties.backgroundColor;
}

void PlatformCALayerRemote::setBackgroundColor(const Color& value)
{
    m_properties.backgroundColor = value;
    m_properties.notePropertiesChanged(RemoteLayerTreeTransaction::BackgroundColorChanged);
}

void PlatformCALayerRemote::setBorderWidth(float value)
{
    if (value == m_properties.borderWidth)
        return;

    m_properties.borderWidth = value;
    m_properties.notePropertiesChanged(RemoteLayerTreeTransaction::BorderWidthChanged);
}

void PlatformCALayerRemote::setBorderColor(const Color& value)
{
    if (value == m_properties.borderColor)
        return;

    m_properties.borderColor = value;
    m_properties.notePropertiesChanged(RemoteLayerTreeTransaction::BorderColorChanged);
}

float PlatformCALayerRemote::opacity() const
{
    return m_properties.opacity;
}

void PlatformCALayerRemote::setOpacity(float value)
{
    m_properties.opacity = value;
    m_properties.notePropertiesChanged(RemoteLayerTreeTransaction::OpacityChanged);
}

#if ENABLE(CSS_FILTERS)
void PlatformCALayerRemote::setFilters(const FilterOperations& filters)
{
    m_properties.filters = std::make_unique<FilterOperations>(filters);
    m_properties.notePropertiesChanged(RemoteLayerTreeTransaction::FiltersChanged);
}

void PlatformCALayerRemote::copyFiltersFrom(const PlatformCALayer* sourceLayer)
{
    if (const FilterOperations* filters = toPlatformCALayerRemote(sourceLayer)->m_properties.filters.get())
        setFilters(*filters);
    else if (m_properties.filters)
        m_properties.filters = nullptr;

    m_properties.notePropertiesChanged(RemoteLayerTreeTransaction::FiltersChanged);
}

#if ENABLE(CSS_COMPOSITING)
void PlatformCALayerRemote::setBlendMode(BlendMode blendMode)
{
    m_properties.blendMode = blendMode;
    m_properties.notePropertiesChanged(RemoteLayerTreeTransaction::BlendModeChanged);
}
#endif

bool PlatformCALayerRemote::filtersCanBeComposited(const FilterOperations& filters)
{
    return PlatformCALayerMac::filtersCanBeComposited(filters);
}
#endif

void PlatformCALayerRemote::setName(const String& value)
{
    m_properties.name = value;
    m_properties.notePropertiesChanged(RemoteLayerTreeTransaction::NameChanged);
}

void PlatformCALayerRemote::setSpeed(float value)
{
    m_properties.speed = value;
    m_properties.notePropertiesChanged(RemoteLayerTreeTransaction::SpeedChanged);
}

void PlatformCALayerRemote::setTimeOffset(CFTimeInterval value)
{
    m_properties.timeOffset = value;
    m_properties.notePropertiesChanged(RemoteLayerTreeTransaction::TimeOffsetChanged);
}

float PlatformCALayerRemote::contentsScale() const
{
    return m_properties.contentsScale;
}

void PlatformCALayerRemote::setContentsScale(float value)
{
    m_properties.contentsScale = value;
    m_properties.notePropertiesChanged(RemoteLayerTreeTransaction::ContentsScaleChanged);

    updateBackingStore();
}

void PlatformCALayerRemote::setEdgeAntialiasingMask(unsigned value)
{
    m_properties.edgeAntialiasingMask = value;
    m_properties.notePropertiesChanged(RemoteLayerTreeTransaction::EdgeAntialiasingMaskChanged);
}

bool PlatformCALayerRemote::requiresCustomAppearanceUpdateOnBoundsChange() const
{
    return m_properties.customAppearance == GraphicsLayer::ScrollingShadow;
}

GraphicsLayer::CustomAppearance PlatformCALayerRemote::customAppearance() const
{
    return m_properties.customAppearance;
}

void PlatformCALayerRemote::updateCustomAppearance(GraphicsLayer::CustomAppearance customAppearance)
{
    m_properties.customAppearance = customAppearance;
    m_properties.notePropertiesChanged(RemoteLayerTreeTransaction::CustomAppearanceChanged);
}

GraphicsLayer::CustomBehavior PlatformCALayerRemote::customBehavior() const
{
    return m_properties.customBehavior;
}

void PlatformCALayerRemote::updateCustomBehavior(GraphicsLayer::CustomBehavior customBehavior)
{
    m_properties.customBehavior = customBehavior;
    m_properties.notePropertiesChanged(RemoteLayerTreeTransaction::CustomBehaviorChanged);
}

PassRefPtr<PlatformCALayer> PlatformCALayerRemote::createCompatibleLayer(PlatformCALayer::LayerType layerType, PlatformCALayerClient* client) const
{
    return PlatformCALayerRemote::create(layerType, client, *m_context);
}

void PlatformCALayerRemote::enumerateRectsBeingDrawn(CGContextRef context, void (^block)(CGRect))
{
    m_properties.backingStore->enumerateRectsBeingDrawn(context, block);
}

uint32_t PlatformCALayerRemote::hostingContextID()
{
    ASSERT_NOT_REACHED();
    return 0;
}

LayerPool& PlatformCALayerRemote::layerPool()
{
    return m_context->layerPool();
}

} // namespace WebKit
