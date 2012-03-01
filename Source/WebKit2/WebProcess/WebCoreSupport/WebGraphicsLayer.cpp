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

#if USE(UI_SIDE_COMPOSITING)
#include "WebGraphicsLayer.h"

#include "Animation.h"
#include "BackingStore.h"
#include "FloatQuad.h"
#include "Frame.h"
#include "FrameView.h"
#include "GraphicsContext.h"
#include "GraphicsLayer.h"
#include "LayerTreeHostProxyMessages.h"
#include "Page.h"
#include "TiledBackingStoreRemoteTile.h"
#include "WebPage.h"
#include "text/CString.h"
#include <HashMap.h>
#include <wtf/CurrentTime.h>

using namespace WebKit;

namespace WebCore {

static HashMap<WebLayerID, WebGraphicsLayer*>& layerByIDMap()
{
    static HashMap<WebLayerID, WebGraphicsLayer*> globalMap;
    return globalMap;
}

WebGraphicsLayer* WebGraphicsLayer::layerByID(WebKit::WebLayerID id)
{
    HashMap<WebLayerID, WebGraphicsLayer*>& table = layerByIDMap();
    HashMap<WebLayerID, WebGraphicsLayer*>::iterator it = table.find(id);
    if (it == table.end())
        return 0;
    return it->second;
}

static WebLayerID toWebLayerID(GraphicsLayer* layer)
{
    return layer ? toWebGraphicsLayer(layer)->id() : 0;
}

void WebGraphicsLayer::notifyChange()
{
    m_modified = true;
    if (client())
        client()->notifySyncRequired(this);
}

void WebGraphicsLayer::notifyChangeRecursively()
{
    notifyChange();
    for (size_t i = 0; i < children().size(); ++i)
        toWebGraphicsLayer(children()[i])->notifyChangeRecursively();
    if (replicaLayer())
        toWebGraphicsLayer(replicaLayer())->notifyChange();
}

WebGraphicsLayer::WebGraphicsLayer(GraphicsLayerClient* client)
    : GraphicsLayer(client)
    , m_maskTarget(0)
    , m_needsDisplay(false)
    , m_modified(true)
    , m_contentNeedsDisplay(false)
    , m_hasPendingAnimations(false)
    , m_inUpdateMode(false)
#if USE(TILED_BACKING_STORE)
    , m_webGraphicsLayerClient(0)
    , m_contentsScale(1.f)
#endif
{
    static WebLayerID nextLayerID = 1;
    m_layerInfo.id = nextLayerID++;
    layerByIDMap().add(id(), this);
}

WebGraphicsLayer::~WebGraphicsLayer()
{
    layerByIDMap().remove(id());

    if (m_webGraphicsLayerClient) {
        purgeBackingStores();
        m_webGraphicsLayerClient->detachLayer(this);
    }
}

bool WebGraphicsLayer::setChildren(const Vector<GraphicsLayer*>& children)
{
    bool ok = GraphicsLayer::setChildren(children);
    if (!ok)
        return false;
    for (int i = 0; i < children.size(); ++i) {
        WebGraphicsLayer* child = toWebGraphicsLayer(children[i]);
        child->setWebGraphicsLayerClient(m_webGraphicsLayerClient);
        child->notifyChange();
    }
    notifyChange();
    return true;
}

void WebGraphicsLayer::addChild(GraphicsLayer* layer)
{
    GraphicsLayer::addChild(layer);
    toWebGraphicsLayer(layer)->setWebGraphicsLayerClient(m_webGraphicsLayerClient);
    toWebGraphicsLayer(layer)->notifyChange();
    notifyChange();
}

void WebGraphicsLayer::addChildAtIndex(GraphicsLayer* layer, int index)
{
    GraphicsLayer::addChildAtIndex(layer, index);
    toWebGraphicsLayer(layer)->setWebGraphicsLayerClient(m_webGraphicsLayerClient);
    toWebGraphicsLayer(layer)->notifyChange();
    notifyChange();
}

void WebGraphicsLayer::addChildAbove(GraphicsLayer* layer, GraphicsLayer* sibling)
{
    GraphicsLayer::addChildAbove(layer, sibling);
    toWebGraphicsLayer(layer)->setWebGraphicsLayerClient(m_webGraphicsLayerClient);
    toWebGraphicsLayer(layer)->notifyChange();
    notifyChange();
}

void WebGraphicsLayer::addChildBelow(GraphicsLayer* layer, GraphicsLayer* sibling)
{
    GraphicsLayer::addChildBelow(layer, sibling);
    toWebGraphicsLayer(layer)->setWebGraphicsLayerClient(m_webGraphicsLayerClient);
    toWebGraphicsLayer(layer)->notifyChange();
    notifyChange();
}

bool WebGraphicsLayer::replaceChild(GraphicsLayer* oldChild, GraphicsLayer* newChild)
{
    bool ok = GraphicsLayer::replaceChild(oldChild, newChild);
    if (!ok)
        return false;
    notifyChange();
    toWebGraphicsLayer(oldChild)->notifyChange();
    toWebGraphicsLayer(newChild)->setWebGraphicsLayerClient(m_webGraphicsLayerClient);
    toWebGraphicsLayer(newChild)->notifyChange();
    return true;
}

void WebGraphicsLayer::removeFromParent()
{
    if (WebGraphicsLayer* parentLayer = toWebGraphicsLayer(parent()))
        parentLayer->notifyChange();
    GraphicsLayer::removeFromParent();

    notifyChange();
}

void WebGraphicsLayer::setPosition(const FloatPoint& p)
{
    if (position() == p)
        return;

    GraphicsLayer::setPosition(p);
    notifyChangeRecursively();
}

void WebGraphicsLayer::setAnchorPoint(const FloatPoint3D& p)
{
    if (anchorPoint() == p)
        return;

    GraphicsLayer::setAnchorPoint(p);
    notifyChangeRecursively();
}

void WebGraphicsLayer::setSize(const FloatSize& size)
{
    if (this->size() == size)
        return;

    GraphicsLayer::setSize(size);
    setNeedsDisplay();
    if (maskLayer())
        maskLayer()->setSize(size);
    notifyChangeRecursively();
}

void WebGraphicsLayer::setTransform(const TransformationMatrix& t)
{
    if (transform() == t)
        return;

    GraphicsLayer::setTransform(t);
    notifyChangeRecursively();
}

void WebGraphicsLayer::setChildrenTransform(const TransformationMatrix& t)
{
    if (childrenTransform() == t)
        return;

    GraphicsLayer::setChildrenTransform(t);
    notifyChangeRecursively();
}

void WebGraphicsLayer::setPreserves3D(bool b)
{
    if (preserves3D() == b)
        return;

    GraphicsLayer::setPreserves3D(b);
    notifyChangeRecursively();
}

void WebGraphicsLayer::setMasksToBounds(bool b)
{
    if (masksToBounds() == b)
        return;
    GraphicsLayer::setMasksToBounds(b);
    notifyChangeRecursively();
}

void WebGraphicsLayer::setDrawsContent(bool b)
{
    if (drawsContent() == b)
        return;
    GraphicsLayer::setDrawsContent(b);

    notifyChange();
}

void WebGraphicsLayer::setContentsOpaque(bool b)
{
    if (contentsOpaque() == b)
        return;
    if (m_mainBackingStore)
        m_mainBackingStore->setSupportsAlpha(!b);
    GraphicsLayer::setContentsOpaque(b);
    notifyChange();
}

void WebGraphicsLayer::setBackfaceVisibility(bool b)
{
    if (backfaceVisibility() == b)
        return;

    GraphicsLayer::setBackfaceVisibility(b);
    notifyChange();
}

void WebGraphicsLayer::setOpacity(float opacity)
{
    if (this->opacity() == opacity)
        return;

    GraphicsLayer::setOpacity(opacity);
    notifyChange();
}

void WebGraphicsLayer::setContentsRect(const IntRect& r)
{
    if (contentsRect() == r)
        return;

    GraphicsLayer::setContentsRect(r);
    notifyChange();
}

void WebGraphicsLayer::notifyAnimationStarted(double time)
{
    if (client())
        client()->notifyAnimationStarted(this, time);
}

bool WebGraphicsLayer::addAnimation(const KeyframeValueList& valueList, const IntSize& boxSize, const Animation* anim, const String& keyframesName, double timeOffset)
{
    if (!anim || anim->isEmptyOrZeroDuration() || valueList.size() < 2 || (valueList.property() != AnimatedPropertyWebkitTransform && valueList.property() != AnimatedPropertyOpacity))
        return false;

    WebLayerAnimation webAnimation(valueList);
    webAnimation.name = keyframesName;
    webAnimation.operation = WebLayerAnimation::AddAnimation;
    webAnimation.boxSize = boxSize;
    webAnimation.animation = Animation::create(anim);
    webAnimation.startTime = timeOffset;
    m_layerInfo.animations.append(webAnimation);
    if (valueList.property() == AnimatedPropertyWebkitTransform)
        m_transformAnimations.add(keyframesName);

    m_hasPendingAnimations = true;
    notifyChangeRecursively();

    return true;
}

void WebGraphicsLayer::pauseAnimation(const String& animationName, double timeOffset)
{
    WebLayerAnimation webAnimation;
    webAnimation.name = animationName;
    webAnimation.operation = WebLayerAnimation::PauseAnimation;
    webAnimation.startTime = WTF::currentTime() - timeOffset;
    m_layerInfo.animations.append(webAnimation);
    notifyChange();
}

void WebGraphicsLayer::removeAnimation(const String& animationName)
{
    WebLayerAnimation webAnimation;
    webAnimation.name = animationName;
    webAnimation.operation = WebLayerAnimation::RemoveAnimation;
    m_layerInfo.animations.append(webAnimation);
    m_transformAnimations.remove(animationName);
    notifyChange();
}

void WebGraphicsLayer::setContentsNeedsDisplay()
{
    RefPtr<Image> image = m_image;
    setContentsToImage(0);
    setContentsToImage(image.get());
}

void WebGraphicsLayer::setContentsToImage(Image* image)
{
    if (image == m_image)
        return;
    int64_t newID = 0;
    if (m_webGraphicsLayerClient) {
        // We adopt first, in case this is the same frame - that way we avoid destroying and recreating the image.
        newID = m_webGraphicsLayerClient->adoptImageBackingStore(image);
        m_webGraphicsLayerClient->releaseImageBackingStore(m_layerInfo.imageBackingStoreID);
        notifyChange();
        if (m_layerInfo.imageBackingStoreID && newID == m_layerInfo.imageBackingStoreID)
            return;
    } else {
        // If m_webGraphicsLayerClient is not set yet there should be no backing store ID.
        ASSERT(!m_layerInfo.imageBackingStoreID);
        notifyChange();
    }

    m_layerInfo.imageBackingStoreID = newID;
    m_image = image;
    m_layerInfo.imageIsUpdated = true;
    GraphicsLayer::setContentsToImage(image);
}

void WebGraphicsLayer::setMaskLayer(GraphicsLayer* layer)
{
    if (layer == maskLayer())
        return;

    GraphicsLayer::setMaskLayer(layer);

    if (!layer)
        return;

    layer->setSize(size());
    WebGraphicsLayer* webGraphicsLayer = toWebGraphicsLayer(layer);
    webGraphicsLayer->setWebGraphicsLayerClient(m_webGraphicsLayerClient);
    webGraphicsLayer->setMaskTarget(this);
    webGraphicsLayer->notifyChange();
    notifyChange();

}

void WebGraphicsLayer::setReplicatedByLayer(GraphicsLayer* layer)
{
    if (layer == replicaLayer())
        return;

    if (layer)
        toWebGraphicsLayer(layer)->setWebGraphicsLayerClient(m_webGraphicsLayerClient);

    GraphicsLayer::setReplicatedByLayer(layer);
    notifyChange();
}

void WebGraphicsLayer::setNeedsDisplay()
{
    setNeedsDisplayInRect(IntRect(IntPoint::zero(), IntSize(size().width(), size().height())));
}

void WebGraphicsLayer::setNeedsDisplayInRect(const FloatRect& rect)
{
    if (m_mainBackingStore)
        m_mainBackingStore->invalidate(IntRect(rect));
    notifyChange();
}

WebLayerID WebGraphicsLayer::id() const
{
    return m_layerInfo.id;
}

void WebGraphicsLayer::syncCompositingState(const FloatRect& rect)
{
    if (WebGraphicsLayer* mask = toWebGraphicsLayer(maskLayer()))
        mask->syncCompositingStateForThisLayerOnly();

    if (WebGraphicsLayer* replica = toWebGraphicsLayer(replicaLayer()))
        replica->syncCompositingStateForThisLayerOnly();

    syncCompositingStateForThisLayerOnly();

    for (size_t i = 0; i < children().size(); ++i)
        children()[i]->syncCompositingState(rect);
}

WebGraphicsLayer* toWebGraphicsLayer(GraphicsLayer* layer)
{
    return static_cast<WebGraphicsLayer*>(layer);
}

void WebGraphicsLayer::syncCompositingStateForThisLayerOnly()
{
    updateContentBuffers();

    if (!m_modified)
        return;

    computeTransformedVisibleRect();
    m_layerInfo.name = name();
    m_layerInfo.anchorPoint = anchorPoint();
    m_layerInfo.backfaceVisible = backfaceVisibility();
    m_layerInfo.childrenTransform = childrenTransform();
    m_layerInfo.contentsOpaque = contentsOpaque();
    m_layerInfo.contentsRect = contentsRect();
    m_layerInfo.drawsContent = drawsContent();
    m_layerInfo.mask = toWebLayerID(maskLayer());
    m_layerInfo.masksToBounds = masksToBounds();
    m_layerInfo.opacity = opacity();
    m_layerInfo.parent = toWebLayerID(parent());
    m_layerInfo.pos = position();
    m_layerInfo.preserves3D = preserves3D();
    m_layerInfo.replica = toWebLayerID(replicaLayer());
    m_layerInfo.size = size();
    m_layerInfo.transform = transform();
    m_contentNeedsDisplay = false;
    m_layerInfo.children.clear();

    for (size_t i = 0; i < children().size(); ++i)
        m_layerInfo.children.append(toWebLayerID(children()[i]));

    m_webGraphicsLayerClient->didSyncCompositingStateForLayer(m_layerInfo);
    m_modified = false;
    m_layerInfo.imageIsUpdated = false;
    if (m_hasPendingAnimations)
        notifyAnimationStarted(WTF::currentTime());
    m_layerInfo.animations.clear();
    m_hasPendingAnimations = false;
}

#if USE(TILED_BACKING_STORE)
void WebGraphicsLayer::tiledBackingStorePaintBegin()
{
}

void WebGraphicsLayer::setRootLayer(bool isRoot)
{
    m_layerInfo.isRootLayer = isRoot;
    notifyChange();
}

void WebGraphicsLayer::setVisibleContentRectTrajectoryVector(const FloatPoint& trajectoryVector)
{
    if (m_mainBackingStore)
        m_mainBackingStore->coverWithTilesIfNeeded(trajectoryVector);
}

void WebGraphicsLayer::setContentsScale(float scale)
{
    m_contentsScale = scale;

    if (!m_mainBackingStore || m_mainBackingStore->contentsScale() == scale)
        return;

    // Between creating the new backing store and painting the content,
    // we do not want to drop the previous one as that might result in
    // briefly seeing flickering as the old tiles may be dropped before
    // something replaces them.
    m_previousBackingStore = m_mainBackingStore.release();

    // No reason to save the previous backing store for non-visible areas.
    m_previousBackingStore->removeAllNonVisibleTiles();

    createBackingStore();
}

void WebGraphicsLayer::createBackingStore()
{
    m_mainBackingStore = adoptPtr(new TiledBackingStore(this, TiledBackingStoreRemoteTileBackend::create(this)));
    m_mainBackingStore->setSupportsAlpha(!contentsOpaque());
    m_mainBackingStore->setContentsScale(m_contentsScale);
}

void WebGraphicsLayer::tiledBackingStorePaint(GraphicsContext* context, const IntRect& rect)
{
    if (rect.isEmpty())
        return;
    m_modified = true;
    paintGraphicsLayerContents(*context, rect);
}

void WebGraphicsLayer::tiledBackingStorePaintEnd(const Vector<IntRect>& updatedRects)
{
}

bool WebGraphicsLayer::tiledBackingStoreUpdatesAllowed() const
{
    if (!m_inUpdateMode)
        return false;
    return m_webGraphicsLayerClient->layerTreeTileUpdatesAllowed();
}

IntRect WebGraphicsLayer::tiledBackingStoreContentsRect()
{
    return IntRect(0, 0, size().width(), size().height());
}

IntRect WebGraphicsLayer::tiledBackingStoreVisibleRect()
{
    // If this layer is part of an active transform animation, the visible rect might change,
    // so we rather render the whole layer until some better optimization is available.
    if (selfOrAncestorHasActiveTransformAnimations())
        return tiledBackingStoreContentsRect();

    // Non-invertible layers are not visible.
    if (!m_layerTransform.combined().isInvertible())
        return IntRect();

    // Return a projection of the visible rect (surface coordinates) onto the layer's plane (layer coordinates).
    // The resulting quad might be squewed and the visible rect is the bounding box of this quad,
    // so it might spread further than the real visible area (and then even more amplified by the cover rect multiplier).
    return m_layerTransform.combined().inverse().clampedBoundsOfProjectedQuad(FloatQuad(FloatRect(m_webGraphicsLayerClient->visibleContentsRect())));
}

Color WebGraphicsLayer::tiledBackingStoreBackgroundColor() const
{
    return contentsOpaque() ? Color::white : Color::transparent;

}
void WebGraphicsLayer::createTile(int tileID, const UpdateInfo& updateInfo)
{
    m_modified = true;
    m_webGraphicsLayerClient->createTile(id(), tileID, updateInfo);
}

void WebGraphicsLayer::updateTile(int tileID, const UpdateInfo& updateInfo)
{
    m_modified = true;
    m_webGraphicsLayerClient->updateTile(id(), tileID, updateInfo);
}

void WebGraphicsLayer::removeTile(int tileID)
{
    m_modified = true;
    m_webGraphicsLayerClient->removeTile(id(), tileID);
}

void WebGraphicsLayer::updateContentBuffers()
{
    // The remote image might have been released by purgeBackingStores.
    if (m_image) {
        if (!m_layerInfo.imageBackingStoreID) {
            m_layerInfo.imageBackingStoreID = m_webGraphicsLayerClient->adoptImageBackingStore(m_image.get());
            m_layerInfo.imageIsUpdated = true;
        }
    }

    if (!drawsContent()) {
        m_mainBackingStore.clear();
        m_previousBackingStore.clear();
        return;
    }

    m_inUpdateMode = true;
    // This is the only place we (re)create the main tiled backing store, once we
    // have a remote client and we are ready to send our data to the UI process.
    if (!m_mainBackingStore)
        createBackingStore();
    m_mainBackingStore->updateTileBuffers();
    m_inUpdateMode = false;

    // The previous backing store is kept around to avoid flickering between
    // removing the existing tiles and painting the new ones. The first time
    // the visibleRect is full painted we remove the previous backing store.
    if (m_mainBackingStore->visibleAreaIsCovered())
        m_previousBackingStore.clear();
}

void WebGraphicsLayer::purgeBackingStores()
{
    m_mainBackingStore.clear();
    m_previousBackingStore.clear();

    if (m_layerInfo.imageBackingStoreID) {
        m_webGraphicsLayerClient->releaseImageBackingStore(m_layerInfo.imageBackingStoreID);
        m_layerInfo.imageBackingStoreID = 0;
    }
}

void WebGraphicsLayer::setWebGraphicsLayerClient(WebKit::WebGraphicsLayerClient* client)
{
    if (m_webGraphicsLayerClient == client)
        return;

    if (WebGraphicsLayer* replica = toWebGraphicsLayer(replicaLayer()))
        replica->setWebGraphicsLayerClient(client);
    if (WebGraphicsLayer* mask = toWebGraphicsLayer(maskLayer()))
        mask->setWebGraphicsLayerClient(client);
    for (size_t i = 0; i < children().size(); ++i) {
        WebGraphicsLayer* layer = toWebGraphicsLayer(this->children()[i]);
        layer->setWebGraphicsLayerClient(client);
    }

    // We have to release resources on the UI process here if the remote client has changed or is removed.
    if (m_webGraphicsLayerClient) {
        purgeBackingStores();
        m_webGraphicsLayerClient->detachLayer(this);
    }
    m_webGraphicsLayerClient = client;
    if (client)
        client->attachLayer(this);
}

void WebGraphicsLayer::adjustVisibleRect()
{
    if (m_mainBackingStore)
        m_mainBackingStore->coverWithTilesIfNeeded();
}

void WebGraphicsLayer::computeTransformedVisibleRect()
{
    // FIXME: Consider transform animations in the visible rect calculation.
    m_layerTransform.setLocalTransform(transform());
    m_layerTransform.setPosition(position());
    m_layerTransform.setAnchorPoint(anchorPoint());
    m_layerTransform.setSize(size());
    m_layerTransform.setFlattening(!preserves3D());
    m_layerTransform.setChildrenTransform(childrenTransform());
    m_layerTransform.combineTransforms(parent() ? toWebGraphicsLayer(parent())->m_layerTransform.combinedForChildren() : TransformationMatrix());

    // The combined transform will be used in tiledBackingStoreVisibleRect.
    adjustVisibleRect();
}
#endif

static PassOwnPtr<GraphicsLayer> createWebGraphicsLayer(GraphicsLayerClient* client)
{
    return adoptPtr(new WebGraphicsLayer(client));
}

void WebGraphicsLayer::initFactory()
{
    GraphicsLayer::setGraphicsLayerFactory(createWebGraphicsLayer);
}

bool WebGraphicsLayer::selfOrAncestorHasActiveTransformAnimations() const
{
    if (!m_transformAnimations.isEmpty())
        return true;

    if (parent())
        return toWebGraphicsLayer(parent())->selfOrAncestorHasActiveTransformAnimations();

    return false;
}

}
#endif
