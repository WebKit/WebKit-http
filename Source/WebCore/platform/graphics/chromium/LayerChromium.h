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


#ifndef LayerChromium_h
#define LayerChromium_h

#if USE(ACCELERATED_COMPOSITING)

#include "FloatPoint.h"
#include "GraphicsContext.h"
#include "PlatformString.h"
#include "ProgramBinding.h"
#include "RenderSurfaceChromium.h"
#include "ShaderChromium.h"
#include "TransformationMatrix.h"

#include <wtf/OwnPtr.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/Vector.h>
#include <wtf/text/StringHash.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class CCLayerImpl;
class CCLayerTreeHost;
class CCTextureUpdater;
class GraphicsContext3D;

class CCLayerDelegate {
public:
    virtual ~CCLayerDelegate() { }
    virtual bool drawsContent() const = 0;
    virtual void paintContents(GraphicsContext&, const IntRect& clip) = 0;
    virtual void notifySyncRequired() = 0;
};

// Base class for composited layers. Special layer types are derived from
// this class.
class LayerChromium : public RefCounted<LayerChromium> {
    friend class LayerTilerChromium;
public:
    static PassRefPtr<LayerChromium> create(CCLayerDelegate*);

    virtual ~LayerChromium();

    const LayerChromium* rootLayer() const;
    LayerChromium* parent() const;
    void addChild(PassRefPtr<LayerChromium>);
    void insertChild(PassRefPtr<LayerChromium>, size_t index);
    void replaceChild(LayerChromium* reference, PassRefPtr<LayerChromium> newLayer);
    void removeFromParent();
    void removeAllChildren();
    void setChildren(const Vector<RefPtr<LayerChromium> >&);
    const Vector<RefPtr<LayerChromium> >& children() const { return m_children; }

    void setAnchorPoint(const FloatPoint& anchorPoint) { m_anchorPoint = anchorPoint; setNeedsCommit(); }
    FloatPoint anchorPoint() const { return m_anchorPoint; }

    void setAnchorPointZ(float anchorPointZ) { m_anchorPointZ = anchorPointZ; setNeedsCommit(); }
    float anchorPointZ() const { return m_anchorPointZ; }

    void setBackgroundColor(const Color& color) { m_backgroundColor = color; setNeedsCommit(); }
    Color backgroundColor() const { return m_backgroundColor; }

    void setBounds(const IntSize&);
    const IntSize& bounds() const { return m_bounds; }
    virtual IntSize contentBounds() const { return bounds(); }

    void setMasksToBounds(bool masksToBounds) { m_masksToBounds = masksToBounds; setNeedsCommit(); }
    bool masksToBounds() const { return m_masksToBounds; }

    void setName(const String&);
    const String& name() const { return m_name; }

    void setMaskLayer(LayerChromium* maskLayer) { m_maskLayer = maskLayer; setNeedsCommit(); }
    LayerChromium* maskLayer() const { return m_maskLayer.get(); }

    void setNeedsDisplay(const FloatRect& dirtyRect);
    void setNeedsDisplay();
    const FloatRect& dirtyRect() const { return m_dirtyRect; }
    void resetNeedsDisplay();

    void setOpacity(float opacity) { m_opacity = opacity; setNeedsCommit(); }
    float opacity() const { return m_opacity; }

    void setOpaque(bool opaque) { m_opaque = opaque; setNeedsCommit(); }
    bool opaque() const { return m_opaque; }

    void setPosition(const FloatPoint& position) { m_position = position;  setNeedsCommit(); }
    FloatPoint position() const { return m_position; }

    void setSublayerTransform(const TransformationMatrix& transform) { m_sublayerTransform = transform; setNeedsCommit(); }
    const TransformationMatrix& sublayerTransform() const { return m_sublayerTransform; }

    TransformationMatrix zoomAnimatorTransform() const { return TransformationMatrix(); }

    void setTransform(const TransformationMatrix& transform) { m_transform = transform; setNeedsCommit(); }
    const TransformationMatrix& transform() const { return m_transform; }

    const IntRect& visibleLayerRect() const { return m_visibleLayerRect; }
    void setVisibleLayerRect(const IntRect& visibleLayerRect) { m_visibleLayerRect = visibleLayerRect; }

    const IntPoint& scrollPosition() const { return m_scrollPosition; }
    void setScrollPosition(const IntPoint& scrollPosition) { m_scrollPosition = scrollPosition; }

    bool scrollable() const { return m_scrollable; }
    void setScrollable(bool scrollable) { m_scrollable = true;  setNeedsCommit(); }

    IntSize scrollDelta() const { return IntSize(); }

    float pageScaleDelta() const { return 1; }

    bool doubleSided() const { return m_doubleSided; }
    void setDoubleSided(bool doubleSided) { m_doubleSided = doubleSided; setNeedsCommit(); }

    void setPreserves3D(bool preserve3D) { m_preserves3D = preserve3D; }
    bool preserves3D() const { return m_preserves3D; }

    void setUsesLayerClipping(bool usesLayerClipping) { m_usesLayerClipping = usesLayerClipping; }
    bool usesLayerClipping() const { return m_usesLayerClipping; }

    void setIsNonCompositedContent(bool isNonCompositedContent) { m_isNonCompositedContent = isNonCompositedContent; }
    bool isNonCompositedContent() const { return m_isNonCompositedContent; }

    virtual void setLayerTreeHost(CCLayerTreeHost*);

    void setDelegate(CCLayerDelegate* delegate) { m_delegate = delegate; }

    void setReplicaLayer(LayerChromium* layer) { m_replicaLayer = layer; }
    LayerChromium* replicaLayer() const { return m_replicaLayer.get(); }

    // These methods typically need to be overwritten by derived classes.
    virtual bool drawsContent() const { return false; }
    virtual void paintContentsIfDirty() { }
    virtual void updateCompositorResources(GraphicsContext3D*, CCTextureUpdater&) { }
    virtual void setIsMask(bool) { }
    virtual void unreserveContentsTexture() { }
    virtual void bindContentsTexture() { }
    virtual void pageScaleChanged() { m_pageScaleDirty = true; }
    virtual void protectVisibleTileTextures() { }
    virtual bool needsContentsScale() const { return false; }

    // These exist just for debugging (via drawDebugBorder()).
    void setDebugBorderColor(const Color&);
    void setDebugBorderWidth(float);
    void drawDebugBorder();

    virtual void pushPropertiesTo(CCLayerImpl*);

    typedef ProgramBinding<VertexShaderPos, FragmentShaderColor> BorderProgram;

    int id() const { return m_layerId; }

    void clearRenderSurface() { m_renderSurface.clear(); }
    RenderSurfaceChromium* renderSurface() const { return m_renderSurface.get(); }
    void createRenderSurface();

    float drawOpacity() const { return m_drawOpacity; }
    void setDrawOpacity(float opacity) { m_drawOpacity = opacity; }
    const IntRect& clipRect() const { return m_clipRect; }
    void setClipRect(const IntRect& clipRect) { m_clipRect = clipRect; }
    RenderSurfaceChromium* targetRenderSurface() const { return m_targetRenderSurface; }
    void setTargetRenderSurface(RenderSurfaceChromium* surface) { m_targetRenderSurface = surface; }
    const TransformationMatrix& drawTransform() const { return m_drawTransform; }
    void setDrawTransform(const TransformationMatrix& matrix) { m_drawTransform = matrix; }
    const TransformationMatrix& screenSpaceTransform() const { return m_screenSpaceTransform; }
    void setScreenSpaceTransform(const TransformationMatrix& matrix) { m_screenSpaceTransform = matrix; }
    const IntRect& drawableContentRect() const { return m_drawableContentRect; }
    void setDrawableContentRect(const IntRect& rect) { m_drawableContentRect = rect; }
    float contentsScale() const { return m_contentsScale; }
    void setContentsScale(float);

    // Returns true if any of the layer's descendants has content to draw.
    bool descendantDrawsContent();
    virtual void contentChanged() { }

    CCLayerTreeHost* layerTreeHost() const { return m_layerTreeHost.get(); }
    void cleanupResourcesRecursive();

protected:
    CCLayerDelegate* m_delegate;
    explicit LayerChromium(CCLayerDelegate*);

    // This is called to clean up resources being held in the same context as
    // layerRendererContext(). Subclasses should override this method if they
    // hold context-dependent resources such as textures.
    virtual void cleanupResources();

    void setNeedsCommit();

    // The dirty rect is the union of damaged regions that need repainting/updating.
    FloatRect m_dirtyRect;

    // The update rect is the region of the compositor resource that was actually updated by the compositor.
    // For layers that may do updating outside the compositor's control (i.e. plugin layers), this information
    // is not available and the update rect will remain empty.
    FloatRect m_updateRect;

    RefPtr<LayerChromium> m_maskLayer;

    friend class TreeSynchronizer;
    friend class CCLayerImpl;
    // Constructs a CCLayerImpl of the correct runtime type for this LayerChromium type.
    virtual PassRefPtr<CCLayerImpl> createCCLayerImpl();
    int m_layerId;

private:
    void setParent(LayerChromium*);
    bool hasAncestor(LayerChromium*) const;

    size_t numChildren() const
    {
        return m_children.size();
    }

    // Returns the index of the child or -1 if not found.
    int indexOfChild(const LayerChromium*);

    // This should only be called from removeFromParent.
    void removeChild(LayerChromium*);

    Vector<RefPtr<LayerChromium> > m_children;
    LayerChromium* m_parent;

    RefPtr<CCLayerTreeHost> m_layerTreeHost;

    // Layer properties.
    IntSize m_bounds;
    IntRect m_visibleLayerRect;
    IntPoint m_scrollPosition;
    bool m_scrollable;
    FloatPoint m_position;
    FloatPoint m_anchorPoint;
    Color m_backgroundColor;
    Color m_debugBorderColor;
    float m_debugBorderWidth;
    float m_opacity;
    float m_anchorPointZ;
    bool m_masksToBounds;
    bool m_opaque;
    bool m_doubleSided;
    bool m_usesLayerClipping;
    bool m_isNonCompositedContent;
    bool m_preserves3D;

    TransformationMatrix m_transform;
    TransformationMatrix m_sublayerTransform;

    // Replica layer used for reflections.
    RefPtr<LayerChromium> m_replicaLayer;

    // Transient properties.
    OwnPtr<RenderSurfaceChromium> m_renderSurface;
    float m_drawOpacity;
    IntRect m_clipRect;
    RenderSurfaceChromium* m_targetRenderSurface;
    TransformationMatrix m_drawTransform;
    TransformationMatrix m_screenSpaceTransform;
    IntRect m_drawableContentRect;
    float m_contentsScale;

    String m_name;

    bool m_pageScaleDirty;
};

void sortLayers(Vector<RefPtr<LayerChromium> >::iterator, Vector<RefPtr<LayerChromium> >::iterator, void*);

}
#endif // USE(ACCELERATED_COMPOSITING)

#endif
