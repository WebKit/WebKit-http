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

#ifndef GraphicsLayerTextureMapper_h
#define GraphicsLayerTextureMapper_h

#include "GraphicsContext.h"
#include "GraphicsLayer.h"
#include "GraphicsLayerClient.h"
#include "Image.h"
#include "TextureMapperNode.h"

#if ENABLE(WEBGL)
#include "GraphicsContext3D.h"
#endif

namespace WebCore {

class TextureMapperNode;
class BitmapTexture;
class TextureMapper;

class GraphicsLayerTextureMapper : public GraphicsLayer {
    friend class TextureMapperNode;

public:
    GraphicsLayerTextureMapper(GraphicsLayerClient*);
    virtual ~GraphicsLayerTextureMapper();

    // reimps from GraphicsLayer.h
    virtual void setNeedsDisplay();
    virtual void setContentsNeedsDisplay() { setNeedsDisplay(); };
    virtual void setNeedsDisplayInRect(const FloatRect&);
    virtual void setParent(GraphicsLayer* layer);
    virtual bool setChildren(const Vector<GraphicsLayer*>&);
    virtual void addChild(GraphicsLayer*);
    virtual void addChildAtIndex(GraphicsLayer*, int index);
    virtual void addChildAbove(GraphicsLayer* layer, GraphicsLayer* sibling);
    virtual void addChildBelow(GraphicsLayer* layer, GraphicsLayer* sibling);
    virtual bool replaceChild(GraphicsLayer* oldChild, GraphicsLayer* newChild);
    virtual void removeFromParent();
    virtual void setMaskLayer(GraphicsLayer* layer);
    virtual void setPosition(const FloatPoint& p);
    virtual void setAnchorPoint(const FloatPoint3D& p);
    virtual void setSize(const FloatSize& size);
    virtual void setTransform(const TransformationMatrix& t);
    virtual void setChildrenTransform(const TransformationMatrix& t);
    virtual void setPreserves3D(bool b);
    virtual void setMasksToBounds(bool b);
    virtual void setDrawsContent(bool b);
    virtual void setBackgroundColor(const Color&);
    virtual void clearBackgroundColor();
    virtual void setContentsOpaque(bool b);
    virtual void setBackfaceVisibility(bool b);
    virtual void setOpacity(float opacity);
    virtual void setContentsRect(const IntRect& r);
    virtual void setReplicatedByLayer(GraphicsLayer*);
    virtual void setContentsToImage(Image*);
    virtual void setContentsToMedia(PlatformLayer*);
    virtual void setContentsToCanvas(PlatformLayer* canvas) { setContentsToMedia(canvas); }
    virtual void syncCompositingState(const FloatRect&);
    virtual void syncCompositingStateForThisLayerOnly();
    virtual void setName(const String& name);
    virtual PlatformLayer* platformLayer() const;

    void notifyChange(TextureMapperNode::ChangeMask changeMask);
    inline TextureMapperNode::ContentData& pendingContent() { return m_pendingContent; }
    inline int changeMask() const { return m_changeMask; }
    void didSynchronize();

    virtual bool addAnimation(const KeyframeValueList&, const IntSize&, const Animation*, const String&, double);
    virtual void pauseAnimation(const String&, double);
    virtual void removeAnimation(const String&);

    TextureMapperNode* node() const { return m_node.get(); }

private:
    OwnPtr<TextureMapperNode> m_node;
    bool m_syncQueued;
    int m_changeMask;
    TextureMapperNode::ContentData m_pendingContent;
    Vector<RefPtr<TextureMapperAnimation> > m_animations;
    void animationStartedTimerFired(Timer<GraphicsLayerTextureMapper>*);
    Timer<GraphicsLayerTextureMapper> m_animationStartedTimer;
};

inline static GraphicsLayerTextureMapper* toGraphicsLayerTextureMapper(GraphicsLayer* layer)
{
    return static_cast<GraphicsLayerTextureMapper*>(layer);
}

}
#endif // GraphicsLayerTextureMapper_h
