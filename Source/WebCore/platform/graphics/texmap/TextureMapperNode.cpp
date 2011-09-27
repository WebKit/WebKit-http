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
#include "TextureMapperNode.h"

#if USE(ACCELERATED_COMPOSITING)

#include "GraphicsLayerTextureMapper.h"

namespace {
    static const float gTileDimension = 1024.0;
}

namespace WebCore {

class TextureMapperSurfaceManager {
public:
    TextureMapper* textureMapper;
    Vector<RefPtr<BitmapTexture> > surfaces;
    IntSize viewportSize;

    PassRefPtr<BitmapTexture> getIntermediateSurface()
    {
        IntSize newViewportSize = textureMapper->viewportSize();
        if (newViewportSize != viewportSize) {
            viewportSize = newViewportSize;
            surfaces.clear();
        }
        if (surfaces.isEmpty()) {
            RefPtr<BitmapTexture> newSurface = textureMapper->createTexture();
            newSurface->reset(viewportSize, false);
            return newSurface.get();
        }
        RefPtr<BitmapTexture> surface = surfaces.last();
        surface->reset(viewportSize, false);
        surfaces.removeLast();
        surface->lock();
        return surface.get();
    }

    void releaseIntermediateSurface(BitmapTexture* surface)
    {
        if (!surface)
            return;
        surfaces.append(surface);
        surface->unlock();
    }
};

TextureMapperNode* toTextureMapperNode(GraphicsLayer* layer)
{
    return layer ? toGraphicsLayerTextureMapper(layer)->node() : 0;
}

TextureMapperNode* TextureMapperNode::rootLayer()
{
    if (m_effectTarget)
        return m_effectTarget->rootLayer();
    if (m_parent)
        return m_parent->rootLayer();
    return this;
}

void TextureMapperNode::setTransform(const TransformationMatrix& matrix)
{
    if (m_transforms.base == matrix)
        return;

    m_transforms.base = matrix;
}

void TextureMapperNode::computePerspectiveTransformIfNeeded()
{
    if (m_children.isEmpty() || m_state.childrenTransform.isIdentity())
        return;

    const FloatPoint centerPoint = FloatPoint(m_size.width() / 2, m_size.height() / 2);
    m_transforms.perspective = TransformationMatrix()
            .translate(centerPoint.x(), centerPoint.y())
            .multiply(m_state.childrenTransform)
            .translate(-centerPoint.x(), -centerPoint.y());
}

int TextureMapperNode::countDescendantsWithContent() const
{
    if (!m_state.visible || m_opacity < 0.01 || (!m_size.width() && !m_size.height() && m_state.masksToBounds))
        return 0;
    int count = (m_size.width() && m_size.height() && (m_state.drawsContent || m_currentContent.contentType != HTMLContentType)) ? 1 : 0;
    for (size_t i = 0; i < m_children.size(); ++i)
        count += m_children[i]->countDescendantsWithContent();

    return count;
}

void TextureMapperNode::computeOverlapsIfNeeded()
{
    m_state.mightHaveOverlaps = countDescendantsWithContent() > 1;
}

void TextureMapperNode::computeReplicaTransformIfNeeded()
{
    if (!m_state.replicaLayer)
        return;

    m_transforms.replica =
        TransformationMatrix(m_transforms.target)
            .multiply(m_state.replicaLayer->m_transforms.local)
            .multiply(TransformationMatrix(m_transforms.target).inverse());
}

void TextureMapperNode::computeLocalTransformIfNeeded()
{
    float originX = m_state.anchorPoint.x() * m_size.width();
    float originY = m_state.anchorPoint.y() * m_size.height();
    m_transforms.local =
        TransformationMatrix()
        .translate3d(originX + m_state.pos.x(), originY + m_state.pos.y(), m_state.anchorPoint.z() )
        .multiply(m_transforms.base)
        .translate3d(-originX, -originY, -m_state.anchorPoint.z());
}

void TextureMapperNode::computeAllTransforms()
{
    if (m_size.isEmpty() && m_state.masksToBounds)
        return;

    computeLocalTransformIfNeeded();
    computeReplicaTransformIfNeeded();
    computePerspectiveTransformIfNeeded();

    m_transforms.target = TransformationMatrix(m_parent ? m_parent->m_transforms.forDescendants : TransformationMatrix()).multiply(m_transforms.local);

    m_state.visible = m_state.backfaceVisibility || m_transforms.target.inverse().m33() >= 0;
    if (!m_state.visible)
        return;

    // This transform is only applied if using a two-pass for the replica, because the transform needs to be adjusted to the size of the intermediate surface, insteaf of the size of the content layer.
    if (m_parent && m_parent->m_state.preserves3D)
        m_transforms.centerZ = m_transforms.target.mapPoint(FloatPoint3D(m_size.width() / 2, m_size.height() / 2, 0)).z();

    if (!m_children.size())
        return;

    m_transforms.forDescendants = m_transforms.target;

    if (!m_state.preserves3D) {
        m_transforms.forDescendants = TransformationMatrix(
                    m_transforms.forDescendants.m11(), m_transforms.forDescendants.m12(), 0, m_transforms.forDescendants.m14(),
                    m_transforms.forDescendants.m21(), m_transforms.forDescendants.m22(), 0, m_transforms.forDescendants.m24(),
                    0, 0, 1, 0,
                    m_transforms.forDescendants.m41(), m_transforms.forDescendants.m42(), 0, m_transforms.forDescendants.m44());
    }

    m_transforms.forDescendants.multiply(m_transforms.perspective);
}

void TextureMapperNode::computeTiles()
{
    if (m_currentContent.contentType == HTMLContentType && !m_state.drawsContent) {
        m_tiles.clear();
        return;
    }
    Vector<FloatRect> tilesToAdd;
    Vector<int> tilesToRemove;
    const int TileEraseThreshold = 6;
    FloatSize size = contentSize();
    FloatRect contentRect(0, 0, size.width() * m_state.contentScale, size.height() * m_state.contentScale);

    for (float y = 0; y < contentRect.height(); y += gTileDimension) {
        for (float x = 0; x < contentRect.width(); x += gTileDimension) {
            FloatRect tileRect(x, y, gTileDimension, gTileDimension);
            tileRect.intersect(contentRect);
            FloatRect tileRectInRootCoordinates = tileRect;
            tileRectInRootCoordinates.scale(1.0 / m_state.contentScale);
            tileRectInRootCoordinates = m_transforms.target.mapRect(tileRectInRootCoordinates);
            tilesToAdd.append(tileRect);
        }
    }

    for (int i = m_tiles.size() - 1; i >= 0; --i) {
        const FloatRect oldTile = m_tiles[i].rect;
        bool found = false;

        for (int j = tilesToAdd.size() - 1; j >= 0; --j) {
            const FloatRect newTile = tilesToAdd[j];
            if (oldTile != newTile)
                continue;

            found = true;
            tilesToAdd.remove(j);
            break;
        }

        if (!found)
            tilesToRemove.append(i);
    }

    for (size_t i = 0; i < tilesToAdd.size(); ++i) {
        if (!tilesToRemove.isEmpty()) {
            Tile& tile = m_tiles[tilesToRemove[0]];
            tilesToRemove.remove(0);
            tile.rect = tilesToAdd[i];
            tile.needsReset = true;
            continue;
        }

        Tile tile;
        tile.rect = tilesToAdd[i];
        tile.needsReset = true;
        m_tiles.append(tile);
    }

    for (size_t i = 0; i < tilesToRemove.size() && m_tiles.size() > TileEraseThreshold; ++i)
        m_tiles.remove(tilesToRemove[i]);
}

void TextureMapperNode::renderContent(TextureMapper* textureMapper, GraphicsLayer* layer)
{
    if (m_size.isEmpty() || !layer || (!m_state.drawsContent && m_currentContent.contentType == HTMLContentType)) {
        m_tiles.clear();
        return;
    }

    if (!textureMapper)
        return;

    if (m_currentContent.contentType == MediaContentType)
        return;

    // FIXME: Add directly composited images.
    FloatRect dirtyRect = m_currentContent.needsDisplay ? entireRect() : m_currentContent.needsDisplayRect;
    dirtyRect.scale(m_state.contentScale);

    for (size_t tileIndex = 0; tileIndex < m_tiles.size(); ++tileIndex) {
        Tile& tile = m_tiles[tileIndex];
        FloatRect rect = dirtyRect;
        if (!tile.texture)
            tile.texture = textureMapper->createTexture();
        RefPtr<BitmapTexture>& texture = tile.texture;
        IntSize tileSize = enclosingIntRect(tile.rect).size();

        if (tile.needsReset || texture->contentSize() != tileSize || !texture->isValid()) {
            tile.needsReset = false;
            texture->reset(tileSize, m_currentContent.contentType == DirectImageContentType ? false : m_state.contentsOpaque);
            rect = tile.rect;
        }

        IntRect contentRect = enclosingIntRect(tile.rect);
        contentRect.intersect(enclosingIntRect(rect));
        if (contentRect.isEmpty())
            continue;

        FloatRect contentRectInTileCoordinates = contentRect;
        FloatPoint offset(-tile.rect.x(), -tile.rect.y());
        contentRectInTileCoordinates.move(offset.x(), offset.y());

        {
            GraphicsContext context(texture->beginPaint(enclosingIntRect(contentRectInTileCoordinates)));
            context.setImageInterpolationQuality(textureMapper->imageInterpolationQuality());
            context.setTextDrawingMode(textureMapper->textDrawingMode());
            context.translate(offset.x(), offset.y());
            context.scale(FloatSize(m_state.contentScale, m_state.contentScale));
            FloatRect scaledContentRect(contentRect);
            scaledContentRect.scale(1.0 / m_state.contentScale);
            if (m_currentContent.contentType == DirectImageContentType)
                context.drawImage(m_currentContent.image.get(), ColorSpaceDeviceRGB, IntPoint(0, 0));
            else
                layer->paintGraphicsLayerContents(context, enclosingIntRect(scaledContentRect));
            texture->endPaint();
        }
    }

    m_currentContent.needsDisplay = false;
    m_currentContent.needsDisplayRect = IntRect();
}

void TextureMapperNode::paint()
{
    if (m_size.isEmpty())
        return;

    if (!m_surfaceManager)
        m_surfaceManager = new TextureMapperSurfaceManager;
    m_surfaceManager->textureMapper = m_textureMapper;
    TextureMapperPaintOptions opt;
    opt.surfaceManager = m_surfaceManager;
    opt.textureMapper = m_textureMapper;
    opt.textureMapper->bindSurface(0);
    paintRecursive(opt);
}

FloatRect TextureMapperNode::targetRectForTileRect(const FloatRect& targetRect, const FloatRect& tileRect) const
{
    return FloatRect(
                targetRect.x() + (tileRect.x() - targetRect.x()) / m_state.contentScale,
                targetRect.y() + (tileRect.y() - targetRect.y()) / m_state.contentScale,
                tileRect.width() / m_state.contentScale,
                tileRect.height() / m_state.contentScale);
}

void TextureMapperNode::paintSelf(const TextureMapperPaintOptions& options)
{
    if (m_size.isEmpty() || (!m_state.drawsContent && m_currentContent.contentType == HTMLContentType))
        return;

    RefPtr<BitmapTexture> maskTexture;
    RefPtr<BitmapTexture> replicaMaskTexture;

    if (m_state.maskLayer)
        maskTexture = m_state.maskLayer->texture();
    if (m_state.replicaLayer && m_state.replicaLayer->m_state.maskLayer)
        replicaMaskTexture = m_state.replicaLayer->m_state.maskLayer->texture();

    const float opacity = options.isSurface ? 1 : options.opacity;

    const FloatRect targetRect = this->targetRect();

    if (m_currentContent.contentType == MediaContentType && m_currentContent.media) {
        if (m_state.replicaLayer && !options.isSurface)
            m_currentContent.media->paintToTextureMapper(options.textureMapper, targetRect,
                                                         TransformationMatrix(m_transforms.target).multiply(m_state.replicaLayer->m_transforms.local),
                                                         opacity * m_state.replicaLayer->m_opacity,
                                                         replicaMaskTexture ? replicaMaskTexture.get() : maskTexture.get());
        m_currentContent.media->paintToTextureMapper(options.textureMapper, targetRect, m_transforms.target, opacity, options.isSurface ? 0 : maskTexture.get());
        return;
    }

    for (size_t i = 0; i < m_tiles.size(); ++i) {
        BitmapTexture* texture = m_tiles[i].texture.get();
        if (m_state.replicaLayer && !options.isSurface) {
            options.textureMapper->drawTexture(*texture, targetRectForTileRect(targetRect, m_tiles[i].rect),
                             TransformationMatrix(m_transforms.target).multiply(m_state.replicaLayer->m_transforms.local),
                             opacity * m_state.replicaLayer->m_opacity,
                             replicaMaskTexture ? replicaMaskTexture.get() : maskTexture.get());
        }

        const FloatRect rect = targetRectForTileRect(targetRect, m_tiles[i].rect);
        options.textureMapper->drawTexture(*texture, rect, m_transforms.target, opacity, options.isSurface ? 0 : maskTexture.get());
    }
}

int TextureMapperNode::compareGraphicsLayersZValue(const void* a, const void* b)
{
    TextureMapperNode* const* nodeA = static_cast<TextureMapperNode* const*>(a);
    TextureMapperNode* const* nodeB = static_cast<TextureMapperNode* const*>(b);
    return int(((*nodeA)->m_transforms.centerZ - (*nodeB)->m_transforms.centerZ) * 1000);
}

void TextureMapperNode::sortByZOrder(Vector<TextureMapperNode* >& array, int first, int last)
{
    qsort(array.data(), array.size(), sizeof(TextureMapperNode*), compareGraphicsLayersZValue);
}

void TextureMapperNode::paintSelfAndChildren(const TextureMapperPaintOptions& options, TextureMapperPaintOptions& optionsForDescendants)
{
    bool hasClip = m_state.masksToBounds && !m_children.isEmpty();
    if (hasClip)
        options.textureMapper->beginClip(m_transforms.forDescendants, FloatRect(0, 0, m_size.width(), m_size.height()));

    paintSelf(options);

    for (int i = 0; i < m_children.size(); ++i)
        m_children[i]->paintRecursive(optionsForDescendants);

    if (hasClip)
        options.textureMapper->endClip();
}

bool TextureMapperNode::paintReflection(const TextureMapperPaintOptions& options, BitmapTexture* contentSurface)
{
    if (!m_state.replicaLayer)
        return false;

    RefPtr<BitmapTexture> surface(contentSurface);
    RefPtr<BitmapTexture> maskSurface;
    RefPtr<BitmapTexture> replicaMaskSurface;
    RefPtr<BitmapTexture> replicaMaskTexture;

    if (TextureMapperNode* replicaMask = m_state.replicaLayer->m_state.maskLayer)
        replicaMaskTexture = replicaMask->texture();

    RefPtr<BitmapTexture> maskTexture;
    if (TextureMapperNode* mask = m_state.maskLayer)
        maskTexture = mask->texture();

    const IntSize viewportSize = options.textureMapper->viewportSize();
    const bool useIntermediateBufferForReplica = m_state.replicaLayer && options.opacity < 0.99;
    const bool useIntermediateBufferForMask = maskTexture && replicaMaskTexture;
    const FloatRect viewportRect(0, 0, viewportSize.width(), viewportSize.height());

    // The mask has to be adjusted to target coordinates.
    if (maskTexture) {
        maskSurface = options.surfaceManager->getIntermediateSurface();
        options.textureMapper->bindSurface(maskSurface.get());
        options.textureMapper->drawTexture(*maskTexture.get(), entireRect(), m_transforms.target, 1, 0);
        maskTexture = maskSurface;
    }

    // The replica's mask has to be adjusted to target coordinates.
    if (replicaMaskTexture) {
        replicaMaskSurface = options.surfaceManager->getIntermediateSurface();
        options.textureMapper->bindSurface(replicaMaskSurface.get());
        options.textureMapper->drawTexture(*replicaMaskTexture.get(), entireRect(), m_transforms.target, 1, 0);
        replicaMaskTexture = replicaMaskSurface;
    }

    // We might need to apply the mask of the content layer before we draw the reflection, as there might be yet another mask for the reflection itself.
    if (useIntermediateBufferForMask) {
        RefPtr<BitmapTexture> maskSurface = options.surfaceManager->getIntermediateSurface();
        options.textureMapper->bindSurface(maskSurface.get());
        options.textureMapper->drawTexture(*surface.get(), viewportRect, TransformationMatrix(), 1, maskTexture.get());
        options.surfaceManager->releaseIntermediateSurface(surface.get());
        surface = maskSurface;
        maskTexture.clear();
    }

    // We blend the layer and its replica in an intermediate buffer before blending into the target surface.
    if (useIntermediateBufferForReplica) {
        RefPtr<BitmapTexture> replicaSurface = options.surfaceManager->getIntermediateSurface();
        options.textureMapper->bindSurface(replicaSurface.get());
        options.textureMapper->drawTexture(*surface.get(), viewportRect, m_transforms.replica, m_state.replicaLayer->m_opacity, replicaMaskTexture.get());
        options.textureMapper->drawTexture(*surface.get(), viewportRect, TransformationMatrix(), 1, maskTexture.get());
        options.surfaceManager->releaseIntermediateSurface(surface.get());
        surface = replicaSurface;
    }

    options.textureMapper->bindSurface(options.surface);

    // Draw the reflection.
    if (!useIntermediateBufferForReplica)
        options.textureMapper->drawTexture(*surface.get(), viewportRect, m_transforms.replica, m_state.replicaLayer->m_opacity, replicaMaskTexture.get());

    // Draw the original.
    options.textureMapper->drawTexture(*surface.get(), viewportRect, TransformationMatrix(), options.opacity, maskTexture.get());

    options.surfaceManager->releaseIntermediateSurface(maskSurface.get());
    options.surfaceManager->releaseIntermediateSurface(replicaMaskSurface.get());

    return true;
}

void TextureMapperNode::paintRecursive(TextureMapperPaintOptions options)
{
    if ((m_size.isEmpty() && (m_state.masksToBounds
        || m_children.isEmpty())) || !m_state.visible || options.opacity < 0.01 || m_opacity < 0.01)
        return;

    options.opacity *= m_opacity;
    RefPtr<BitmapTexture> surface;
    const bool needsTwoPass = ((m_state.replicaLayer || m_state.maskLayer) && !m_children.isEmpty()) || (m_opacity < 0.99 && m_state.mightHaveOverlaps) || (m_opacity < 0.99 && m_state.replicaLayer);
    const IntSize viewportSize = options.textureMapper->viewportSize();
    options.isSurface = false;

    TextureMapperPaintOptions optionsForDescendants(options);

    if (!needsTwoPass) {
        paintSelfAndChildren(options, optionsForDescendants);
        return;
    }

    FloatRect viewportRect(0, 0, viewportSize.width(), viewportSize.height());

    RefPtr<BitmapTexture> maskSurface;

    // The mask has to be adjusted to target coordinates.
    if (m_state.maskLayer) {
        maskSurface = options.surfaceManager->getIntermediateSurface();
        options.textureMapper->bindSurface(maskSurface.get());
        options.textureMapper->drawTexture(*m_state.maskLayer->texture(), entireRect(), m_transforms.target, 1.0, 0);
    }

    surface = options.surfaceManager->getIntermediateSurface();
    optionsForDescendants.surface = surface.get();
    options.isSurface = true;
    optionsForDescendants.opacity = 1;
    options.textureMapper->bindSurface(surface.get());

    paintSelfAndChildren(options, optionsForDescendants);

    if (!paintReflection(options, surface.get())) {
        options.textureMapper->bindSurface(options.surface);
        options.textureMapper->drawTexture(*surface.get(), viewportRect, TransformationMatrix(), options.opacity, 0);
    }

    options.surfaceManager->releaseIntermediateSurface(surface.get());
    options.surfaceManager->releaseIntermediateSurface(maskSurface.get());
}

TextureMapperNode::~TextureMapperNode()
{
    for (int i = m_children.size() - 1; i >= 0; --i)
        m_children[i]->m_parent = 0;

    if (m_parent)
        m_parent->m_children.remove(m_parent->m_children.find(this));
}

void TextureMapperNode::syncCompositingState(GraphicsLayerTextureMapper* graphicsLayer, int options)
{
    syncCompositingState(graphicsLayer, rootLayer()->m_textureMapper, options);
}

void TextureMapperNode::syncCompositingStateSelf(GraphicsLayerTextureMapper* graphicsLayer, TextureMapper* textureMapper)
{
    int changeMask = graphicsLayer->changeMask();
    const TextureMapperNode::ContentData& pendingContent = graphicsLayer->pendingContent();
    if (changeMask == NoChanges && graphicsLayer->m_animations.isEmpty() && pendingContent.needsDisplayRect.isEmpty() && !pendingContent.needsDisplay)
        return;

    if (m_currentContent.contentType == HTMLContentType && (changeMask & ParentChange)) {
        TextureMapperNode* newParent = toTextureMapperNode(graphicsLayer->parent());
        if (newParent != m_parent) {
            // Remove node from current from child list first.
            if (m_parent) {
                size_t index = m_parent->m_children.find(this);
                m_parent->m_children.remove(index);
                m_parent = 0;
            }
            // Set new node parent and add node to the parents child list.
            if (newParent) {
                m_parent = newParent;
                m_parent->m_children.append(this);
            }
        }
    }

    if (changeMask & ChildrenChange) {
        // Clear children parent pointer to avoid unsync and crash on node delete.
        for (size_t i = 0; i < m_children.size(); i++)
            m_children[i]->m_parent = 0;

        m_children.clear();
        for (size_t i = 0; i < graphicsLayer->children().size(); ++i) {
            TextureMapperNode* child = toTextureMapperNode(graphicsLayer->children()[i]);
            if (!child)
                continue;
            m_children.append(child);
            child->m_parent = this;
        }
    }

    if (changeMask & (SizeChange | ContentsRectChange)) {
        FloatSize wantedSize(graphicsLayer->size().width(), graphicsLayer->size().height());
        if (wantedSize.isEmpty() && pendingContent.contentType == HTMLContentType)
            wantedSize = FloatSize(graphicsLayer->contentsRect().width(), graphicsLayer->contentsRect().height());

        if (wantedSize != m_size)
            m_tiles.clear();

        m_size = wantedSize;
    }

    if (changeMask & MaskLayerChange) {
       if (TextureMapperNode* layer = toTextureMapperNode(graphicsLayer->maskLayer()))
           layer->m_effectTarget = this;
    }

    if (changeMask & ReplicaLayerChange) {
       if (TextureMapperNode* layer = toTextureMapperNode(graphicsLayer->replicaLayer()))
           layer->m_effectTarget = this;
    }

    if (changeMask & AnimationChange) {
        m_animations.clear();
        for (size_t i = 0; i < graphicsLayer->m_animations.size(); ++i)
            m_animations.append(graphicsLayer->m_animations[i]);
    }

    m_state.maskLayer = toTextureMapperNode(graphicsLayer->maskLayer());
    m_state.replicaLayer = toTextureMapperNode(graphicsLayer->replicaLayer());
    m_state.pos = graphicsLayer->position();
    m_state.anchorPoint = graphicsLayer->anchorPoint();
    m_state.size = graphicsLayer->size();
    m_state.contentsRect = graphicsLayer->contentsRect();
    m_state.transform = graphicsLayer->transform();
    m_state.contentsRect = graphicsLayer->contentsRect();
    m_state.preserves3D = graphicsLayer->preserves3D();
    m_state.masksToBounds = graphicsLayer->masksToBounds();
    m_state.drawsContent = graphicsLayer->drawsContent();
    m_state.contentsOpaque = graphicsLayer->contentsOpaque();
    m_state.backfaceVisibility = graphicsLayer->backfaceVisibility();
    m_state.childrenTransform = graphicsLayer->childrenTransform();
    m_state.opacity = graphicsLayer->opacity();
    m_currentContent.contentType = pendingContent.contentType;
    m_currentContent.image = pendingContent.image;
    m_currentContent.media = pendingContent.media;
    m_currentContent.needsDisplay = m_currentContent.needsDisplay || pendingContent.needsDisplay;
    if (!m_currentContent.needsDisplay)
        m_currentContent.needsDisplayRect.unite(pendingContent.needsDisplayRect);

    if (!hasOpacityAnimation())
        m_opacity = m_state.opacity;
    if (!hasTransformAnimation())
        m_transforms.base = m_state.transform;
}

bool TextureMapperNode::descendantsOrSelfHaveRunningAnimations() const
{
    for (size_t i = 0; i < m_animations.size(); ++i) {
        if (!m_animations[i]->paused)
            return true;
    }

    for (size_t i = 0; i < m_children.size(); ++i) {
        if (m_children[i]->descendantsOrSelfHaveRunningAnimations())
            return true;
    }

    return false;
}

static double normalizedAnimationValue(double runningTime, double duration, bool alternate)
{
    if (!duration)
        return 0;
    const int loopCount = runningTime / duration;
    const double lastFullLoop = duration * double(loopCount);
    const double remainder = runningTime - lastFullLoop;
    const double normalized = remainder / duration;
    return (loopCount % 2 && alternate) ? (1 - normalized) : normalized;
}

void TextureMapperNode::applyOpacityAnimation(float fromOpacity, float toOpacity, double progress)
{
    // Optimization: special case the edge values (0 and 1).
    if (progress == 1.0)
        setOpacity(toOpacity);
    else if (!progress)
        setOpacity(fromOpacity);
    else
        setOpacity(fromOpacity + progress * (toOpacity - fromOpacity));
}

static inline double solveEpsilon(double duration)
{
    return 1.0 / (200.0 * duration);
}

static inline double solveCubicBezierFunction(qreal p1x, qreal p1y, qreal p2x, qreal p2y, double t, double duration)
{
    UnitBezier bezier(p1x, p1y, p2x, p2y);
    return bezier.solve(t, solveEpsilon(duration));
}

static inline double solveStepsFunction(int numSteps, bool stepAtStart, double t)
{
    if (stepAtStart)
        return qMin(1.0, (floor(numSteps * t) + 1) / numSteps);
    return floor(numSteps * t) / numSteps;
}

static inline float applyTimingFunction(const TimingFunction* timingFunction, float progress, double duration)
{
    if (!timingFunction)
        return progress;

    if (timingFunction->isCubicBezierTimingFunction()) {
        const CubicBezierTimingFunction* ctf = static_cast<const CubicBezierTimingFunction*>(timingFunction);
        return solveCubicBezierFunction(ctf->x1(),
                                        ctf->y1(),
                                        ctf->x2(),
                                        ctf->y2(),
                                        progress, duration);
    }

    if (timingFunction->isStepsTimingFunction()) {
        const StepsTimingFunction* stf = static_cast<const StepsTimingFunction*>(timingFunction);
        return solveStepsFunction(stf->numberOfSteps(), stf->stepAtStart(), double(progress));
    }

    return progress;
}

void TextureMapperNode::applyTransformAnimation(const TextureMapperAnimation& animation, const TransformOperations* from, const TransformOperations* to, double progress)
{
    // Optimization: special case the edge values (0 and 1).
    if (progress == 1.0 || !progress) {
        TransformationMatrix matrix;
        const TransformOperations* ops = progress ? to : from;
        ops->apply(animation.boxSize, matrix);
        setTransform(matrix);
    }

    if (!animation.listsMatch) {
        TransformationMatrix toMatrix, fromMatrix;
        to->apply(animation.boxSize, toMatrix);
        from->apply(animation.boxSize, fromMatrix);
        toMatrix.blend(fromMatrix, progress);
        setTransform(toMatrix);
        return;
    }

    TransformationMatrix matrix;

    if (!to->size()) {
        const TransformOperations* swap = to;
        to = from;
        from = swap;
    } else if (!from->size())
        progress = 1.0 - progress;

    TransformOperations blended(*to);
    for (size_t i = 0; i < animation.functionList.size(); ++i)
        blended.operations()[i]->blend(from->at(i), progress, !from->at(i))->apply(matrix, animation.boxSize);

    setTransform(matrix);
}

void TextureMapperNode::applyAnimationFrame(const TextureMapperAnimation& animation, const AnimationValue* from, const AnimationValue* to, float progress)
{
    switch (animation.keyframes.property()) {
    case AnimatedPropertyOpacity:
        applyOpacityAnimation((static_cast<const FloatAnimationValue*>(from)->value()), (static_cast<const FloatAnimationValue*>(to)->value()), progress);
        return;
    case AnimatedPropertyWebkitTransform:
        applyTransformAnimation(animation, static_cast<const TransformAnimationValue*>(from)->value(), static_cast<const TransformAnimationValue*>(to)->value(), progress);
        return;
    default:
        ASSERT_NOT_REACHED();
    }
}

void TextureMapperNode::applyAnimation(const TextureMapperAnimation& animation, double normalizedValue)
{
    // Optimization: special case the edge values (0 and 1).
    if (!normalizedValue) {
        applyAnimationFrame(animation, animation.keyframes.at(0), animation.keyframes.at(1), 0);
        return;
    }
    if (normalizedValue == 1.0) {
        applyAnimationFrame(animation, animation.keyframes.at(animation.keyframes.size() - 2), animation.keyframes.at(animation.keyframes.size() - 1), 1);
        return;
    }
    if (animation.keyframes.size() == 2) {
        normalizedValue = applyTimingFunction(animation.animation->timingFunction().get(), normalizedValue, animation.animation->duration());
        applyAnimationFrame(animation, animation.keyframes.at(0), animation.keyframes.at(1), normalizedValue);
        return;
    }

    for (size_t i = 0; i < animation.keyframes.size() - 1; ++i) {
        const AnimationValue* from = animation.keyframes.at(i);
        const AnimationValue* to = animation.keyframes.at(i + 1);
        if (from->keyTime() > normalizedValue || to->keyTime() < normalizedValue)
            continue;

        normalizedValue = (normalizedValue - from->keyTime()) / (to->keyTime() - from->keyTime());
        normalizedValue = applyTimingFunction(from->timingFunction(), normalizedValue, animation.animation->duration());
        applyAnimationFrame(animation, from, to, normalizedValue);
        break;
    }
}

bool TextureMapperNode::hasOpacityAnimation() const
{
    for (size_t i = 0; i < m_animations.size(); ++i) {
        const TextureMapperAnimation& animation = *m_animations[i].get();
        if (animation.keyframes.property() == AnimatedPropertyOpacity)
            return true;
    }
    return false;
}

bool TextureMapperNode::hasTransformAnimation() const
{
    for (size_t i = 0; i < m_animations.size(); ++i) {
        const TextureMapperAnimation& animation = *m_animations[i].get();
        if (animation.keyframes.property() == AnimatedPropertyWebkitTransform)
            return true;
    }
    return false;
}

void TextureMapperNode::syncAnimations(GraphicsLayerTextureMapper* layer)
{
    for (int i = m_animations.size() - 1; i >= 0; --i) {
        RefPtr<TextureMapperAnimation> animation = m_animations[i];

        double totalRunningTime = WTF::currentTime() - animation->startTime;
        RefPtr<Animation> anim = animation->animation;
        double normalizedValue = normalizedAnimationValue(totalRunningTime, anim->duration(), anim->direction());

        if (anim->iterationCount() != Animation::IterationCountInfinite && totalRunningTime >= anim->duration() * anim->iterationCount()) {
            // We apply an animation that very close to the edge, so that the final frame is applied, oterwise we might get, for example, an opacity of 0.01 which is still visible.
            if (anim->fillsForwards()) {
                if (animation->keyframes.property() == AnimatedPropertyWebkitTransform)
                    m_state.transform = m_transforms.base;
                else if (animation->keyframes.property() == AnimatedPropertyOpacity)
                    m_opacity = m_state.opacity;
            }

            m_animations.remove(i);
            continue;
        }

        if (!animation->paused)
            applyAnimation(*animation.get(), normalizedValue);
    }
}

void TextureMapperNode::syncCompositingState(GraphicsLayerTextureMapper* graphicsLayer, TextureMapper* textureMapper, int options)
{
    if (graphicsLayer) {
        syncCompositingStateSelf(graphicsLayer, textureMapper);
        graphicsLayer->didSynchronize();
    }

    if (graphicsLayer && m_state.maskLayer) {
        m_state.maskLayer->syncCompositingState(toGraphicsLayerTextureMapper(graphicsLayer->maskLayer()), textureMapper);

        // A mask layer has its parent's size by default, in case it's not set specifically.
        if (m_state.maskLayer->m_size.isEmpty())
            m_state.maskLayer->m_size = m_size;
    }

    if (m_state.replicaLayer)
        m_state.replicaLayer->syncCompositingState(toGraphicsLayerTextureMapper(graphicsLayer->replicaLayer()), textureMapper);

    syncAnimations(graphicsLayer);

    computeAllTransforms();
    computePerspectiveTransformIfNeeded();
    computeTiles();
    computeOverlapsIfNeeded();

    if (graphicsLayer)
        renderContent(textureMapper, graphicsLayer);

    if (!(options & TraverseDescendants))
        return;

    if (graphicsLayer) {
        Vector<GraphicsLayer*> children = graphicsLayer->children();
        for (int i = children.size() - 1; i >= 0; --i) {
            TextureMapperNode* node = toTextureMapperNode(children[i]);
            if (!node)
                continue;
            node->syncCompositingState(toGraphicsLayerTextureMapper(children[i]), textureMapper, options);
        }
    } else {
        for (int i = m_children.size() - 1; i >= 0; --i)
            m_children[i]->syncCompositingState(0, textureMapper, options);
    }

    if (m_state.preserves3D)
        sortByZOrder(m_children, 0, m_children.size());
}

static PassRefPtr<TimingFunction> copyTimingFunction(const TimingFunction* tfunc)
{
    if (!tfunc)
        return 0;

    if (tfunc->isLinearTimingFunction())
        return LinearTimingFunction::create();

    if (tfunc->isCubicBezierTimingFunction()) {
        const CubicBezierTimingFunction* btfunc = static_cast<const CubicBezierTimingFunction*>(tfunc);
        return CubicBezierTimingFunction::create(btfunc->x1(), btfunc->y1(), btfunc->x2(), btfunc->y2());
    }

    if (tfunc->isStepsTimingFunction()) {
        const StepsTimingFunction* stfunc = static_cast<const StepsTimingFunction*>(tfunc);
        return StepsTimingFunction::create(stfunc->numberOfSteps(), stfunc->stepAtStart());
    }

    return 0;
}

static const AnimationValue* copyAnimationValue(AnimatedPropertyID property, const AnimationValue* value)
{
    switch (property) {
    case AnimatedPropertyWebkitTransform: {
        const TransformAnimationValue* transformValue = static_cast<const TransformAnimationValue*>(value);
        return new TransformAnimationValue(transformValue->keyTime(), transformValue->value(), copyTimingFunction(transformValue->timingFunction()));
    }
    case AnimatedPropertyOpacity: {
        const FloatAnimationValue* floatValue = static_cast<const FloatAnimationValue*>(value);
        return new FloatAnimationValue(floatValue->keyTime(), floatValue->value(), copyTimingFunction(floatValue->timingFunction()));
    }
    default:
        ASSERT_NOT_REACHED();
    }

    return 0;
}

TextureMapperAnimation::TextureMapperAnimation(const KeyframeValueList& values)
    : keyframes(values.property())
{
    for (size_t i = 0; i < values.size(); ++i)
        keyframes.insert(copyAnimationValue(values.property(), values.at(i)));
}

}

#endif
