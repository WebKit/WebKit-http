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

#if USE(ACCELERATED_COMPOSITING)

#import "PlatformCALayerRemote.h"

#import "PlatformCALayerRemoteTiledBacking.h"
#import "RemoteLayerBackingStore.h"
#import "RemoteLayerTreeContext.h"
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
using namespace WebKit;

static RemoteLayerTreeTransaction::LayerID generateLayerID()
{
    static RemoteLayerTreeTransaction::LayerID layerID;
    return ++layerID;
}

static PlatformCALayerRemote* toPlatformCALayerRemote(PlatformCALayer* layer)
{
    ASSERT_WITH_SECURITY_IMPLICATION(!layer || layer->isRemote());
    return static_cast<PlatformCALayerRemote*>(layer);
}

PassRefPtr<PlatformCALayer> PlatformCALayerRemote::create(LayerType layerType, PlatformCALayerClient* owner, RemoteLayerTreeContext* context)
{
    if (layerType == LayerTypeTiledBackingLayer ||  layerType == LayerTypePageTiledBackingLayer)
        return adoptRef(new PlatformCALayerRemoteTiledBacking(layerType, owner, context));

    return adoptRef(new PlatformCALayerRemote(layerType, owner, context));
}

PlatformCALayerRemote::PlatformCALayerRemote(LayerType layerType, PlatformCALayerClient* owner, RemoteLayerTreeContext* context)
    : PlatformCALayer(layerType, owner)
    , m_layerID(generateLayerID())
    , m_superlayer(nullptr)
    , m_acceleratesDrawing(false)
    , m_context(context)
{
    // FIXME: match all default values from CA.
    setContentsScale(1);

    m_context->layerWasCreated(this, layerType);
}

PassRefPtr<PlatformCALayer> PlatformCALayerRemote::clone(PlatformCALayerClient* owner) const
{
    return nullptr;
}

PlatformCALayerRemote::~PlatformCALayerRemote()
{
    for (const auto& layer : m_children)
        toPlatformCALayerRemote(layer.get())->m_superlayer = nullptr;
    m_context->layerWillBeDestroyed(this);
}

void PlatformCALayerRemote::recursiveBuildTransaction(RemoteLayerTreeTransaction& transaction)
{
    if (m_properties.backingStore.display())
        m_properties.notePropertiesChanged(RemoteLayerTreeTransaction::BackingStoreChanged);

    if (m_properties.changedProperties != RemoteLayerTreeTransaction::NoChange) {
        if (m_properties.changedProperties & RemoteLayerTreeTransaction::ChildrenChanged) {
            m_properties.children.clear();
            for (auto layer : m_children)
                m_properties.children.append(toPlatformCALayerRemote(layer.get())->layerID());
        }

        transaction.layerPropertiesChanged(this, m_properties);
        m_properties.changedProperties = RemoteLayerTreeTransaction::NoChange;
    }

    for (size_t i = 0; i < m_children.size(); ++i) {
        PlatformCALayerRemote* child = toPlatformCALayerRemote(m_children[i].get());
        ASSERT(child->superlayer() == this);
        child->recursiveBuildTransaction(transaction);
    }
}

void PlatformCALayerRemote::animationStarted(CFTimeInterval beginTime)
{
}

void PlatformCALayerRemote::ensureBackingStore()
{
    m_properties.backingStore.ensureBackingStore(this, expandedIntSize(m_properties.size), m_properties.contentsScale, m_acceleratesDrawing);
}

void PlatformCALayerRemote::setNeedsDisplay(const FloatRect* rect)
{
    ensureBackingStore();

    if (!rect) {
        m_properties.backingStore.setNeedsDisplay();
        return;
    }

    // FIXME: Need to map this through contentsRect/etc.
    m_properties.backingStore.setNeedsDisplay(enclosingIntRect(*rect));
}

void PlatformCALayerRemote::setContentsChanged()
{
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
        m_children.remove(referenceIndex);
        m_children.insert(referenceIndex, layer);
    }

    m_properties.notePropertiesChanged(RemoteLayerTreeTransaction::ChildrenChanged);
}

void PlatformCALayerRemote::adoptSublayers(PlatformCALayer* source)
{
    setSublayers(toPlatformCALayerRemote(source)->m_children);
}

void PlatformCALayerRemote::addAnimationForKey(const String& key, PlatformCAAnimation* animation)
{
    ASSERT_NOT_REACHED();
}

void PlatformCALayerRemote::removeAnimationForKey(const String& key)
{
    ASSERT_NOT_REACHED();
}

PassRefPtr<PlatformCAAnimation> PlatformCALayerRemote::animationForKey(const String& key)
{
    ASSERT_NOT_REACHED();

    return nullptr;
}

void PlatformCALayerRemote::setMask(PlatformCALayer* layer)
{
    m_properties.maskLayer = toPlatformCALayerRemote(layer)->layerID();
    m_properties.notePropertiesChanged(RemoteLayerTreeTransaction::MaskLayerChanged);
}

bool PlatformCALayerRemote::isOpaque() const
{
    return m_properties.opaque;
}

void PlatformCALayerRemote::setOpaque(bool value)
{
    m_properties.opaque = value;
    m_properties.notePropertiesChanged(RemoteLayerTreeTransaction::OpaqueChanged);
}

FloatRect PlatformCALayerRemote::bounds() const
{
    return FloatRect(FloatPoint(), m_properties.size);
}

void PlatformCALayerRemote::setBounds(const FloatRect& value)
{
    m_properties.size = value.size();
    m_properties.notePropertiesChanged(RemoteLayerTreeTransaction::SizeChanged);

    ensureBackingStore();
}

FloatPoint3D PlatformCALayerRemote::position() const
{
    return m_properties.position;
}

void PlatformCALayerRemote::setPosition(const FloatPoint3D& value)
{
    m_properties.position = value;
    m_properties.notePropertiesChanged(RemoteLayerTreeTransaction::PositionChanged);
}

FloatPoint3D PlatformCALayerRemote::anchorPoint() const
{
    return m_properties.anchorPoint;
}

void PlatformCALayerRemote::setAnchorPoint(const FloatPoint3D& value)
{
    m_properties.anchorPoint = value;
    m_properties.notePropertiesChanged(RemoteLayerTreeTransaction::AnchorPointChanged);
}

TransformationMatrix PlatformCALayerRemote::transform() const
{
    return m_properties.transform;
}

void PlatformCALayerRemote::setTransform(const TransformationMatrix& value)
{
    m_properties.transform = value;
    m_properties.notePropertiesChanged(RemoteLayerTreeTransaction::TransformChanged);
}

TransformationMatrix PlatformCALayerRemote::sublayerTransform() const
{
    return m_properties.sublayerTransform;
}

void PlatformCALayerRemote::setSublayerTransform(const TransformationMatrix& value)
{
    m_properties.sublayerTransform = value;
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
    ensureBackingStore();
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
    m_properties.borderWidth = value;
    m_properties.notePropertiesChanged(RemoteLayerTreeTransaction::BorderWidthChanged);
}

void PlatformCALayerRemote::setBorderColor(const Color& value)
{
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
    m_properties.filters = filters;
    m_properties.notePropertiesChanged(RemoteLayerTreeTransaction::FiltersChanged);
}

void PlatformCALayerRemote::copyFiltersFrom(const PlatformCALayer* sourceLayer)
{
    ASSERT_NOT_REACHED();
}

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

    ensureBackingStore();
}

void PlatformCALayerRemote::setEdgeAntialiasingMask(unsigned value)
{
    m_properties.edgeAntialiasingMask = value;
    m_properties.notePropertiesChanged(RemoteLayerTreeTransaction::EdgeAntialiasingMaskChanged);
}

AVPlayerLayer* PlatformCALayerRemote::playerLayer() const
{
    return nullptr;
}

PassRefPtr<PlatformCALayer> PlatformCALayerRemote::createCompatibleLayer(PlatformCALayer::LayerType layerType, PlatformCALayerClient* client) const
{
    return PlatformCALayerRemote::create(layerType, client, m_context);
}

#endif // USE(ACCELERATED_COMPOSITING)
