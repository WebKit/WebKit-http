/*
    Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies)

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
#include "GraphicsLayerTextureMapper.h"

#include "GraphicsContext.h"
#include "GraphicsLayerAnimation.h"
#include "GraphicsLayerFactory.h"
#include "ImageBuffer.h"
#include "NotImplemented.h"
#include <wtf/CurrentTime.h>

#if USE(CAIRO)
#include "CairoUtilities.h"
#include <wtf/text/CString.h>
#endif

namespace WebCore {

PassOwnPtr<GraphicsLayer> GraphicsLayer::create(GraphicsLayerFactory* factory, GraphicsLayerClient* client)
{
    if (!factory)
        return adoptPtr(new GraphicsLayerTextureMapper(client));

    return factory->createGraphicsLayer(client);
}

PassOwnPtr<GraphicsLayer> GraphicsLayer::create(GraphicsLayerClient* client)
{
    if (s_graphicsLayerFactory)
        return (*s_graphicsLayerFactory)(client);
    return adoptPtr(new GraphicsLayerTextureMapper(client));
}

GraphicsLayerTextureMapper::GraphicsLayerTextureMapper(GraphicsLayerClient* client)
    : GraphicsLayer(client)
    , m_layer(adoptPtr(new TextureMapperLayer()))
    , m_compositedNativeImagePtr(0)
    , m_changeMask(0)
    , m_needsDisplay(false)
    , m_hasOwnBackingStore(true)
    , m_fixedToViewport(false)
    , m_debugBorderWidth(0)
    , m_contentsLayer(0)
    , m_animationStartedTimer(this, &GraphicsLayerTextureMapper::animationStartedTimerFired)
{
}

void GraphicsLayerTextureMapper::notifyChange(TextureMapperLayer::ChangeMask changeMask)
{
    m_changeMask |= changeMask;
    if (!client())
        return;
    client()->notifyFlushRequired(this);
}

void GraphicsLayerTextureMapper::setName(const String& name)
{
    GraphicsLayer::setName(name);
}

GraphicsLayerTextureMapper::~GraphicsLayerTextureMapper()
{
    willBeDestroyed();
}

void GraphicsLayerTextureMapper::willBeDestroyed()
{
    GraphicsLayer::willBeDestroyed();
}

/* \reimp (GraphicsLayer.h): The current size might change, thus we need to update the whole display.
*/
void GraphicsLayerTextureMapper::setNeedsDisplay()
{
    if (!m_hasOwnBackingStore)
        return;

    m_needsDisplay = true;
    notifyChange(TextureMapperLayer::DisplayChange);
    addRepaintRect(FloatRect(FloatPoint(), m_size));
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerTextureMapper::setContentsNeedsDisplay()
{
    notifyChange(TextureMapperLayer::DisplayChange);
    addRepaintRect(contentsRect());
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerTextureMapper::setNeedsDisplayInRect(const FloatRect& rect)
{
    if (!m_hasOwnBackingStore)
        return;

    if (m_needsDisplay)
        return;
    m_needsDisplayRect.unite(rect);
    notifyChange(TextureMapperLayer::DisplayChange);
    addRepaintRect(rect);
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerTextureMapper::setParent(GraphicsLayer* layer)
{
    notifyChange(TextureMapperLayer::ParentChange);
    GraphicsLayer::setParent(layer);
}

/* \reimp (GraphicsLayer.h)
*/
bool GraphicsLayerTextureMapper::setChildren(const Vector<GraphicsLayer*>& children)
{
    notifyChange(TextureMapperLayer::ChildrenChange);
    return GraphicsLayer::setChildren(children);
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerTextureMapper::addChild(GraphicsLayer* layer)
{
    notifyChange(TextureMapperLayer::ChildrenChange);
    GraphicsLayer::addChild(layer);
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerTextureMapper::addChildAtIndex(GraphicsLayer* layer, int index)
{
    GraphicsLayer::addChildAtIndex(layer, index);
    notifyChange(TextureMapperLayer::ChildrenChange);
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerTextureMapper::addChildAbove(GraphicsLayer* layer, GraphicsLayer* sibling)
{
     GraphicsLayer::addChildAbove(layer, sibling);
     notifyChange(TextureMapperLayer::ChildrenChange);
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerTextureMapper::addChildBelow(GraphicsLayer* layer, GraphicsLayer* sibling)
{
    GraphicsLayer::addChildBelow(layer, sibling);
    notifyChange(TextureMapperLayer::ChildrenChange);
}

/* \reimp (GraphicsLayer.h)
*/
bool GraphicsLayerTextureMapper::replaceChild(GraphicsLayer* oldChild, GraphicsLayer* newChild)
{
    if (GraphicsLayer::replaceChild(oldChild, newChild)) {
        notifyChange(TextureMapperLayer::ChildrenChange);
        return true;
    }
    return false;
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerTextureMapper::removeFromParent()
{
    if (!parent())
        return;
    notifyChange(TextureMapperLayer::ParentChange);
    GraphicsLayer::removeFromParent();
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerTextureMapper::setMaskLayer(GraphicsLayer* value)
{
    if (value == maskLayer())
        return;
    GraphicsLayer::setMaskLayer(value);
    notifyChange(TextureMapperLayer::MaskLayerChange);

    if (!value)
        return;
    value->setSize(size());
    value->setContentsVisible(contentsAreVisible());
}


/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerTextureMapper::setReplicatedByLayer(GraphicsLayer* value)
{
    if (value == replicaLayer())
        return;
    GraphicsLayer::setReplicatedByLayer(value);
    notifyChange(TextureMapperLayer::ReplicaLayerChange);
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerTextureMapper::setPosition(const FloatPoint& value)
{
    if (value == position())
        return;
    GraphicsLayer::setPosition(value);
    notifyChange(TextureMapperLayer::PositionChange);
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerTextureMapper::setAnchorPoint(const FloatPoint3D& value)
{
    if (value == anchorPoint())
        return;
    GraphicsLayer::setAnchorPoint(value);
    notifyChange(TextureMapperLayer::AnchorPointChange);
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerTextureMapper::setSize(const FloatSize& value)
{
    if (value == size())
        return;

    GraphicsLayer::setSize(value);
    if (maskLayer())
        maskLayer()->setSize(value);
    notifyChange(TextureMapperLayer::SizeChange);
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerTextureMapper::setTransform(const TransformationMatrix& value)
{
    if (value == transform())
        return;

    GraphicsLayer::setTransform(value);
    notifyChange(TextureMapperLayer::TransformChange);
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerTextureMapper::setChildrenTransform(const TransformationMatrix& value)
{
    if (value == childrenTransform())
        return;
    GraphicsLayer::setChildrenTransform(value);
    notifyChange(TextureMapperLayer::ChildrenTransformChange);
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerTextureMapper::setPreserves3D(bool value)
{
    if (value == preserves3D())
        return;
    GraphicsLayer::setPreserves3D(value);
    notifyChange(TextureMapperLayer::Preserves3DChange);
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerTextureMapper::setMasksToBounds(bool value)
{
    if (value == masksToBounds())
        return;
    GraphicsLayer::setMasksToBounds(value);
    notifyChange(TextureMapperLayer::MasksToBoundsChange);
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerTextureMapper::setDrawsContent(bool value)
{
    if (value == drawsContent())
        return;
    notifyChange(TextureMapperLayer::DrawsContentChange);
    GraphicsLayer::setDrawsContent(value);
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerTextureMapper::setContentsVisible(bool value)
{
    if (value == contentsAreVisible())
        return;
    notifyChange(TextureMapperLayer::ContentsVisibleChange);
    GraphicsLayer::setContentsVisible(value);
    if (maskLayer())
        maskLayer()->setContentsVisible(value);
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerTextureMapper::setContentsOpaque(bool value)
{
    if (value == contentsOpaque())
        return;
    notifyChange(TextureMapperLayer::ContentsOpaqueChange);
    GraphicsLayer::setContentsOpaque(value);
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerTextureMapper::setBackfaceVisibility(bool value)
{
    if (value == backfaceVisibility())
        return;
    GraphicsLayer::setBackfaceVisibility(value);
    notifyChange(TextureMapperLayer::BackfaceVisibilityChange);
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerTextureMapper::setOpacity(float value)
{
    if (value == opacity())
        return;
    GraphicsLayer::setOpacity(value);
    notifyChange(TextureMapperLayer::OpacityChange);
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerTextureMapper::setContentsRect(const IntRect& value)
{
    if (value == contentsRect())
        return;
    GraphicsLayer::setContentsRect(value);
    notifyChange(TextureMapperLayer::ContentsRectChange);
}

void GraphicsLayerTextureMapper::setContentsToSolidColor(const Color& color)
{
    if (color == m_solidColor)
        return;

    m_solidColor = color;
    notifyChange(TextureMapperLayer::ContentChange);
}


/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerTextureMapper::setContentsToImage(Image* image)
{
    if (image) {
        // Make the decision about whether the image has changed.
        // This code makes the assumption that pointer equality on a NativeImagePtr is a valid way to tell if the image is changed.
        // This assumption is true in Qt, GTK and EFL.
        NativeImagePtr newNativeImagePtr = image->nativeImageForCurrentFrame();
        if (!newNativeImagePtr)
            return;

        if (newNativeImagePtr == m_compositedNativeImagePtr)
            return;

        m_compositedNativeImagePtr = newNativeImagePtr;
        if (!m_compositedImage)
            m_compositedImage = TextureMapperTiledBackingStore::create();
        m_compositedImage->setContentsToImage(image);
    } else {
        m_compositedNativeImagePtr = 0;
        m_compositedImage = 0;
    }

    setContentsToMedia(m_compositedImage.get());
    notifyChange(TextureMapperLayer::ContentChange);
    GraphicsLayer::setContentsToImage(image);
}

void GraphicsLayerTextureMapper::setContentsToMedia(TextureMapperPlatformLayer* media)
{
    if (media == m_contentsLayer)
        return;

    GraphicsLayer::setContentsToMedia(media);
    notifyChange(TextureMapperLayer::ContentChange);
    m_contentsLayer = media;
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerTextureMapper::flushCompositingStateForThisLayerOnly()
{
    m_layer->flushCompositingStateForThisLayerOnly(this);
    updateBackingStore();
    didFlushCompositingState();
}

/* \reimp (GraphicsLayer.h)
*/
void GraphicsLayerTextureMapper::flushCompositingState(const FloatRect& rect)
{
    if (!m_layer->textureMapper())
        return;

    flushCompositingStateForThisLayerOnly();

    if (maskLayer())
        maskLayer()->flushCompositingState(rect);
    if (replicaLayer())
        replicaLayer()->flushCompositingState(rect);
    for (size_t i = 0; i < children().size(); ++i)
        children()[i]->flushCompositingState(rect);
}

void GraphicsLayerTextureMapper::didFlushCompositingState()
{
    m_changeMask = 0;
}

void GraphicsLayerTextureMapper::updateBackingStore()
{
    if (!m_hasOwnBackingStore)
        return;

    prepareBackingStore();
    m_layer->setBackingStore(m_backingStore);
}

void GraphicsLayerTextureMapper::prepareBackingStore()
{
    TextureMapper* textureMapper = m_layer->textureMapper();
    if (!textureMapper)
        return;

    if (!shouldHaveBackingStore()) {
        m_backingStore.clear();
        return;
    }

    IntRect dirtyRect = enclosingIntRect(FloatRect(FloatPoint::zero(), m_size));
    if (!m_needsDisplay)
        dirtyRect.intersect(enclosingIntRect(m_needsDisplayRect));
    if (dirtyRect.isEmpty())
        return;

    if (!m_backingStore)
        m_backingStore = TextureMapperTiledBackingStore::create();

#if PLATFORM(QT)
    ASSERT(dynamic_cast<TextureMapperTiledBackingStore*>(m_backingStore.get()));
#endif
    TextureMapperTiledBackingStore* backingStore = static_cast<TextureMapperTiledBackingStore*>(m_backingStore.get());

    if (isShowingRepaintCounter())
        incrementRepaintCount();

    // Paint into an intermediate buffer to avoid painting content more than once.
    bool paintOnce = true;
    const IntSize maxTextureSize = textureMapper->maxTextureSize();
    // We need to paint directly if the dirty rect exceeds one of the maximum dimensions.
    if (dirtyRect.width() > maxTextureSize.width() || dirtyRect.height() > maxTextureSize.height())
        paintOnce = false;

    if (paintOnce) {
        OwnPtr<ImageBuffer> imageBuffer = ImageBuffer::create(dirtyRect.size());
        GraphicsContext* context = imageBuffer->context();
        context->setImageInterpolationQuality(textureMapper->imageInterpolationQuality());
        context->setTextDrawingMode(textureMapper->textDrawingMode());
        context->translate(-dirtyRect.x(), -dirtyRect.y());
        paintGraphicsLayerContents(*context, dirtyRect);

        if (isShowingRepaintCounter())
            drawRepaintCounter(context);

        RefPtr<Image> image = imageBuffer->copyImage(DontCopyBackingStore);
        backingStore->updateContents(textureMapper, image.get(), m_size, dirtyRect, BitmapTexture::UpdateCanModifyOriginalImageData);
    } else
        backingStore->updateContents(textureMapper, this, m_size, dirtyRect, BitmapTexture::UpdateCanModifyOriginalImageData);

    backingStore->setShowDebugBorders(isShowingDebugBorder());
    backingStore->setDebugBorder(m_debugBorderColor, m_debugBorderWidth);

    m_needsDisplay = false;
    m_needsDisplayRect = IntRect();
}

bool GraphicsLayerTextureMapper::shouldHaveBackingStore() const
{
    return drawsContent() && contentsAreVisible() && !m_size.isEmpty();
}

#if USE(CAIRO)
void GraphicsLayerTextureMapper::drawRepaintCounter(GraphicsContext* context)
{
    cairo_t* cr = context->platformContext()->cr();
    cairo_save(cr);

    CString repaintCount = String::format("%i", this->repaintCount()).utf8();
    cairo_select_font_face(cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 18);

    cairo_text_extents_t repaintTextExtents;
    cairo_text_extents(cr, repaintCount.data(), &repaintTextExtents);

    static const int repaintCountBorderWidth = 10;
    setSourceRGBAFromColor(cr, isShowingDebugBorder() ? m_debugBorderColor : Color(0, 255, 0, 127));
    cairo_rectangle(cr, 0, 0,
        repaintTextExtents.width + (repaintCountBorderWidth * 2),
        repaintTextExtents.height + (repaintCountBorderWidth * 2));
    cairo_fill(cr);

    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_move_to(cr, repaintCountBorderWidth, repaintTextExtents.height + repaintCountBorderWidth);
    cairo_show_text(cr, repaintCount.data());

    cairo_restore(cr);
}
#else
void GraphicsLayerTextureMapper::drawRepaintCounter(GraphicsContext* context)
{
    notImplemented();
}

#endif

bool GraphicsLayerTextureMapper::addAnimation(const KeyframeValueList& valueList, const IntSize& boxSize, const Animation* anim, const String& keyframesName, double timeOffset)
{
    ASSERT(!keyframesName.isEmpty());

    if (!anim || anim->isEmptyOrZeroDuration() || valueList.size() < 2 || (valueList.property() != AnimatedPropertyWebkitTransform && valueList.property() != AnimatedPropertyOpacity))
        return false;

    bool listsMatch = false;
    bool hasBigRotation;

    if (valueList.property() == AnimatedPropertyWebkitTransform)
        listsMatch = validateTransformOperations(valueList, hasBigRotation) >= 0;

    m_animations.add(GraphicsLayerAnimation(keyframesName, valueList, boxSize, anim, WTF::currentTime() - timeOffset, listsMatch));
    notifyChange(TextureMapperLayer::AnimationChange);
    m_animationStartedTimer.startOneShot(0);
    return true;
}

void GraphicsLayerTextureMapper::setAnimations(const GraphicsLayerAnimations& animations)
{
    m_animations = animations;
    notifyChange(TextureMapperLayer::AnimationChange);
}


void GraphicsLayerTextureMapper::pauseAnimation(const String& animationName, double timeOffset)
{
    m_animations.pause(animationName, timeOffset);
}

void GraphicsLayerTextureMapper::removeAnimation(const String& animationName)
{
    m_animations.remove(animationName);
}

void GraphicsLayerTextureMapper::animationStartedTimerFired(Timer<GraphicsLayerTextureMapper>*)
{
    client()->notifyAnimationStarted(this, /* DOM time */ WTF::currentTime());
}

void GraphicsLayerTextureMapper::setDebugBorder(const Color& color, float width)
{
    // The default values for GraphicsLayer debug borders are a little
    // hard to see (some less than one pixel wide), so we double their size here.
    m_debugBorderColor = color;
    m_debugBorderWidth = width * 2;
}

#if ENABLE(CSS_FILTERS)
bool GraphicsLayerTextureMapper::setFilters(const FilterOperations& filters)
{
    notifyChange(TextureMapperLayer::FilterChange);
    return GraphicsLayer::setFilters(filters);
}
#endif

}
