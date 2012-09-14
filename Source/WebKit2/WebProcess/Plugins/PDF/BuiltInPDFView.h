/*
 * Copyright (C) 2011, 2012 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef BuiltInPDFView_h
#define BuiltInPDFView_h

#include "Plugin.h"
#include <WebCore/ScrollableArea.h>
#include <wtf/RetainPtr.h>

typedef const struct OpaqueJSContext* JSContextRef;
typedef struct OpaqueJSValue* JSObjectRef;
typedef const struct OpaqueJSValue* JSValueRef;

namespace WebCore {
    struct PluginInfo;
}

namespace WebKit {

class PluginView;
class WebFrame;

class BuiltInPDFView : public Plugin, private WebCore::ScrollableArea {
public:
    static PassRefPtr<BuiltInPDFView> create(WebFrame*);
    ~BuiltInPDFView();

    static WebCore::PluginInfo pluginInfo();

    // In-process PDFViews don't support asynchronous initialization.
    virtual bool isBeingAsynchronouslyInitialized() const { return false; }

private:
    explicit BuiltInPDFView(WebFrame*);

    // Regular plug-ins don't need access to view, but we add scrollbars to embedding FrameView for proper event handling.
    PluginView* pluginView();
    const PluginView* pluginView() const;

    void updateScrollbars();
    PassRefPtr<WebCore::Scrollbar> createScrollbar(WebCore::ScrollbarOrientation);
    void destroyScrollbar(WebCore::ScrollbarOrientation);
    void addArchiveResource();
    void pdfDocumentDidLoad();
    void calculateSizes();
    void paintBackground(WebCore::GraphicsContext*, const WebCore::IntRect& dirtyRect);
    void paintContent(WebCore::GraphicsContext*, const WebCore::IntRect& dirtyRect);
    void paintControls(WebCore::GraphicsContext*, const WebCore::IntRect& dirtyRect);

    // Plug-in methods
    virtual bool initialize(const Parameters&);
    virtual void destroy();
    virtual void paint(WebCore::GraphicsContext*, const WebCore::IntRect& dirtyRectInWindowCoordinates);
    virtual void updateControlTints(WebCore::GraphicsContext*);
    virtual PassRefPtr<ShareableBitmap> snapshot();
#if PLATFORM(MAC)
    virtual PlatformLayer* pluginLayer();
#endif
    virtual bool isTransparent();
    virtual bool wantsWheelEvents() OVERRIDE;
    virtual void geometryDidChange(const WebCore::IntSize& pluginSize, const WebCore::IntRect& clipRect, const WebCore::AffineTransform& pluginToRootViewTransform);
    virtual void visibilityDidChange();
    virtual void frameDidFinishLoading(uint64_t requestID);
    virtual void frameDidFail(uint64_t requestID, bool wasCancelled);
    virtual void didEvaluateJavaScript(uint64_t requestID, const String& result);
    virtual void streamDidReceiveResponse(uint64_t streamID, const WebCore::KURL& responseURL, uint32_t streamLength, uint32_t lastModifiedTime, const String& mimeType, const String& headers, const String& suggestedFileName);
    virtual void streamDidReceiveData(uint64_t streamID, const char* bytes, int length);
    virtual void streamDidFinishLoading(uint64_t streamID);
    virtual void streamDidFail(uint64_t streamID, bool wasCancelled);
    virtual void manualStreamDidReceiveResponse(const WebCore::KURL& responseURL, uint32_t streamLength, uint32_t lastModifiedTime, const WTF::String& mimeType, const WTF::String& headers, const String& suggestedFileName);
    virtual void manualStreamDidReceiveData(const char* bytes, int length);
    virtual void manualStreamDidFinishLoading();
    virtual void manualStreamDidFail(bool wasCancelled);
    virtual bool handleMouseEvent(const WebMouseEvent&);
    virtual bool handleWheelEvent(const WebWheelEvent&);
    virtual bool handleMouseEnterEvent(const WebMouseEvent&);
    virtual bool handleMouseLeaveEvent(const WebMouseEvent&);
    virtual bool handleContextMenuEvent(const WebMouseEvent&);
    virtual bool handleKeyboardEvent(const WebKeyboardEvent&);
    virtual void setFocus(bool);
    virtual NPObject* pluginScriptableNPObject();
#if PLATFORM(MAC)
    virtual void windowFocusChanged(bool);
    virtual void windowAndViewFramesChanged(const WebCore::IntRect& windowFrameInScreenCoordinates, const WebCore::IntRect& viewFrameInWindowCoordinates);
    virtual void windowVisibilityChanged(bool);
    virtual void contentsScaleFactorChanged(float);
    virtual uint64_t pluginComplexTextInputIdentifier() const;
    virtual void sendComplexTextInput(const String& textInput);
    virtual void setLayerHostingMode(LayerHostingMode) OVERRIDE;
#endif

    virtual void storageBlockingStateChanged(bool);
    virtual void privateBrowsingStateChanged(bool);
    virtual bool getFormValue(String& formValue);
    virtual bool handleScroll(WebCore::ScrollDirection, WebCore::ScrollGranularity);
    virtual WebCore::Scrollbar* horizontalScrollbar();
    virtual WebCore::Scrollbar* verticalScrollbar();

    virtual RetainPtr<PDFDocument> pdfDocumentForPrinting() const OVERRIDE { return m_pdfDocument; }

    // ScrollableArea methods.
    virtual WebCore::IntRect scrollCornerRect() const OVERRIDE;
    virtual WebCore::ScrollableArea* enclosingScrollableArea() const OVERRIDE;
    virtual WebCore::IntRect scrollableAreaBoundingBox() const OVERRIDE;
    virtual void setScrollOffset(const WebCore::IntPoint&) OVERRIDE;
    virtual int scrollSize(WebCore::ScrollbarOrientation) const OVERRIDE;
    virtual bool isActive() const OVERRIDE;
    virtual void invalidateScrollbarRect(WebCore::Scrollbar*, const WebCore::IntRect&) OVERRIDE;
    virtual void invalidateScrollCornerRect(const WebCore::IntRect&) OVERRIDE;
    virtual bool isScrollCornerVisible() const OVERRIDE;
    virtual int scrollPosition(WebCore::Scrollbar*) const OVERRIDE;
    virtual WebCore::IntPoint scrollPosition() const OVERRIDE;
    virtual WebCore::IntPoint minimumScrollPosition() const OVERRIDE;
    virtual WebCore::IntPoint maximumScrollPosition() const OVERRIDE;
    virtual int visibleHeight() const OVERRIDE;
    virtual int visibleWidth() const OVERRIDE;
    virtual WebCore::IntSize contentsSize() const OVERRIDE;
    virtual WebCore::Scrollbar* horizontalScrollbar() const OVERRIDE { return m_horizontalScrollbar.get(); }
    virtual WebCore::Scrollbar* verticalScrollbar() const OVERRIDE { return m_verticalScrollbar.get(); }
    virtual bool scrollbarsCanBeActive() const OVERRIDE;
    virtual bool shouldSuspendScrollAnimations() const OVERRIDE { return false; } // If we return true, ScrollAnimatorMac will keep cycling a timer forever, waiting for a good time to animate.
    virtual void scrollbarStyleChanged(int newStyle, bool forceUpdate) OVERRIDE;
    // FIXME: Implement the other conversion functions; this one is enough to get scrollbar hit testing working.
    virtual WebCore::IntPoint convertFromContainingViewToScrollbar(const WebCore::Scrollbar*, const WebCore::IntPoint& parentPoint) const OVERRIDE;

    JSObjectRef makeJSPDFDoc(JSContextRef);
    static JSValueRef jsPDFDocPrint(JSContextRef, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception);

    WebCore::IntSize m_pluginSize;

    WebCore::KURL m_sourceURL;

    String m_suggestedFilename;
    RetainPtr<CFMutableDataRef> m_dataBuffer;

    RetainPtr<PDFDocument> m_pdfDocument;
    Vector<WebCore::IntRect> m_pageBoxes;
    WebCore::IntSize m_pdfDocumentSize; // All pages, including gaps.

    RefPtr<WebCore::Scrollbar> m_horizontalScrollbar;
    RefPtr<WebCore::Scrollbar> m_verticalScrollbar;

    WebFrame* m_frame;

    WebCore::IntSize m_scrollOffset;
};

} // namespace WebKit

#endif // BuiltInPDFView_h
