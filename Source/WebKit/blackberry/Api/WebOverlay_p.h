/*
 * Copyright (C) 2012 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef WebOverlay_p_h
#define WebOverlay_p_h

#if USE(ACCELERATED_COMPOSITING)

#include "GraphicsLayer.h"
#include "LayerCompositingThread.h"
#include "Texture.h"
#include "WebOverlay.h"
#include "WebOverlayOverride.h"

#include <SkBitmap.h>
#include <pthread.h>
#include <wtf/OwnPtr.h>
#include <wtf/RefPtr.h>

class SkCanvas;

namespace WTF {
class String;
}

namespace WebCore {
class Animation;
class KeyframeValueList;
}

namespace BlackBerry {
namespace WebKit {

class WebOverlayClient;
class WebPagePrivate;

class WebOverlayPrivate {
public:
    WebOverlayPrivate()
        : q(0)
        , parent(0)
        , m_page(0)
    {
        nativeThread = pthread_self();
    }

    virtual ~WebOverlayPrivate()
    {
        ASSERT(pthread_self() == nativeThread);
    }

    WebPagePrivate* page() const;
    void setPage(WebPagePrivate* page) { m_page = page; }

    virtual void setClient(WebOverlayClient* c) { client = c; }

    WebOverlayOverride* override();

    virtual WebCore::FloatPoint position() const = 0;
    virtual void setPosition(const WebCore::FloatPoint&) = 0;

    virtual WebCore::FloatPoint anchorPoint() const = 0;
    virtual void setAnchorPoint(const WebCore::FloatPoint&) = 0;

    virtual WebCore::FloatSize size() const = 0;
    virtual void setSize(const WebCore::FloatSize&) = 0;

    virtual bool sizeIsScaleInvariant() const = 0;
    virtual void setSizeIsScaleInvariant(bool) = 0;

    virtual WebCore::TransformationMatrix transform() const = 0;
    virtual void setTransform(const WebCore::TransformationMatrix&) = 0;

    virtual float opacity() const = 0;
    virtual void setOpacity(float) = 0;

    virtual void addAnimation(const WTF::String& name, WebCore::Animation*, const WebCore::KeyframeValueList&) = 0;
    virtual void removeAnimation(const WTF::String&) = 0;

    virtual void addChild(WebOverlayPrivate*) = 0;
    virtual void removeFromParent() = 0;

    virtual void setContentsToImage(const unsigned char* data, const WebCore::IntSize& imageSize, WebOverlay::ImageDataAdoptionType) = 0;
    virtual void setContentsToColor(const WebCore::Color&) = 0;
    virtual void setDrawsContent(bool) = 0;

    virtual void clear() = 0;
    virtual void invalidate() = 0;
    void drawContents(SkCanvas*);

    virtual void resetOverrides() = 0;

    void scheduleCompositingRun();

    // Never 0
    WebCore::LayerCompositingThread* layerCompositingThread() const { return m_layerCompositingThread.get(); }

    // Can be 0
    virtual WebCore::GraphicsLayer* graphicsLayer() const { return 0; }

    WebOverlay* q;
    WebOverlayClient* client;
    WebOverlay* parent;
    pthread_t nativeThread;

protected:
    RefPtr<WebCore::LayerCompositingThread> m_layerCompositingThread;
    OwnPtr<WebOverlayOverride> m_override;
    WebPagePrivate* m_page;
};

class WebOverlayPrivateWebKitThread : public WebOverlayPrivate, public WebCore::GraphicsLayerClient {
public:
    WebOverlayPrivateWebKitThread(WebCore::GraphicsLayerClient* = 0);

    virtual WebCore::FloatPoint position() const;
    virtual void setPosition(const WebCore::FloatPoint&);

    virtual WebCore::FloatPoint anchorPoint() const;
    virtual void setAnchorPoint(const WebCore::FloatPoint&);

    virtual WebCore::FloatSize size() const;
    virtual void setSize(const WebCore::FloatSize&);

    virtual bool sizeIsScaleInvariant() const;
    virtual void setSizeIsScaleInvariant(bool);

    virtual WebCore::TransformationMatrix transform() const;
    virtual void setTransform(const WebCore::TransformationMatrix&);

    virtual float opacity() const;
    virtual void setOpacity(float);

    virtual void addAnimation(const WTF::String& name, WebCore::Animation*, const WebCore::KeyframeValueList&);
    virtual void removeAnimation(const WTF::String& name);

    virtual void addChild(WebOverlayPrivate*);
    virtual void removeFromParent();

    virtual void setContentsToImage(const unsigned char* data, const WebCore::IntSize& imageSize, WebOverlay::ImageDataAdoptionType);
    virtual void setContentsToColor(const WebCore::Color&);
    virtual void setDrawsContent(bool);

    virtual void clear();
    virtual void invalidate();

    virtual void resetOverrides();

    virtual WebCore::GraphicsLayer* graphicsLayer() const { return m_layer.get(); }

    // GraphicsLayerClient
    virtual void notifyAnimationStarted(const WebCore::GraphicsLayer*, double time) { }
    virtual void notifySyncRequired(const WebCore::GraphicsLayer*);
    virtual void paintContents(const WebCore::GraphicsLayer*, WebCore::GraphicsContext&, WebCore::GraphicsLayerPaintingPhase, const WebCore::IntRect& inClip);
    virtual bool showDebugBorders(const WebCore::GraphicsLayer*) const { return false; }
    virtual bool showRepaintCounter(const WebCore::GraphicsLayer*) const { return false; }
    virtual bool contentsVisible(const WebCore::GraphicsLayer*, const WebCore::IntRect& contentRect) const { return true; }

private:
    OwnPtr<WebCore::GraphicsLayer> m_layer;
};

// The LayerCompositingThreadClient's life cycle is tied to the LayerCompositingThread,
// so it needs to be a separate object from the WebOverlayPrivateCompositingThread.
class WebOverlayLayerCompositingThreadClient : public WebCore::LayerCompositingThreadClient {
public:
    WebOverlayLayerCompositingThreadClient();
    virtual ~WebOverlayLayerCompositingThreadClient() { }

    void setLayer(WebCore::LayerCompositingThread* layer) { m_layerCompositingThread = layer; }
    void setClient(WebOverlay* owner, WebOverlayClient* client) { m_owner = owner; m_client = client; }

    bool drawsContent() const { return m_drawsContent; }
    void setDrawsContent(bool);
    void invalidate();

    const SkBitmap& contents() const { return m_contents; }
    void setContents(const SkBitmap&);

    void setContentsToColor(const WebCore::Color&);

    // LayerCompositingThreadClient
    virtual void layerCompositingThreadDestroyed(WebCore::LayerCompositingThread*);
    virtual void layerVisibilityChanged(WebCore::LayerCompositingThread*, bool visible);
    virtual void uploadTexturesIfNeeded(WebCore::LayerCompositingThread*);
    virtual void drawTextures(WebCore::LayerCompositingThread*, double scale, int positionLocation, int texCoordLocation);
    virtual void deleteTextures(WebCore::LayerCompositingThread*);

private:
    void clearUploadedContents();

private:
    RefPtr<WebCore::Texture> m_texture;
    bool m_drawsContent;
    SkBitmap m_contents;
    SkBitmap m_uploadedContents;
    WebCore::Color m_color;
    WebCore::LayerCompositingThread* m_layerCompositingThread;
    WebOverlay* m_owner;
    WebOverlayClient* m_client;
};

class WebOverlayPrivateCompositingThread : public WebOverlayPrivate {
public:
    WebOverlayPrivateCompositingThread(PassRefPtr<WebCore::LayerCompositingThread>);
    WebOverlayPrivateCompositingThread();
    ~WebOverlayPrivateCompositingThread();

    virtual void setClient(WebOverlayClient*);

    virtual WebCore::FloatPoint position() const;
    virtual void setPosition(const WebCore::FloatPoint&);

    virtual WebCore::FloatPoint anchorPoint() const;
    virtual void setAnchorPoint(const WebCore::FloatPoint&);

    virtual WebCore::FloatSize size() const;
    virtual void setSize(const WebCore::FloatSize&);

    virtual bool sizeIsScaleInvariant() const;
    virtual void setSizeIsScaleInvariant(bool);

    virtual WebCore::TransformationMatrix transform() const;
    virtual void setTransform(const WebCore::TransformationMatrix&);

    virtual float opacity() const;
    virtual void setOpacity(float);

    virtual void addAnimation(const WTF::String& name, WebCore::Animation*, const WebCore::KeyframeValueList&);
    virtual void removeAnimation(const WTF::String& name);

    virtual void addChild(WebOverlayPrivate*);
    virtual void removeFromParent();

    virtual void setContentsToImage(const unsigned char* data, const WebCore::IntSize& imageSize, WebOverlay::ImageDataAdoptionType);
    virtual void setContentsToColor(const WebCore::Color&);
    virtual void setDrawsContent(bool);

    virtual void clear();
    virtual void invalidate();

    virtual void resetOverrides();

private:
    WebOverlayLayerCompositingThreadClient* m_layerCompositingThreadClient;
};

}
}

#endif // USE(ACCELERATED_COMPOSITING)

#endif // WebOverlay_p_h
