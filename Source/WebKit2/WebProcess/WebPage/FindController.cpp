/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

#include "config.h"
#include "FindController.h"

#include "ShareableBitmap.h"
#include "WKPage.h"
#include "WebCoreArgumentCoders.h"
#include "WebPage.h"
#include "WebPageProxyMessages.h"
#include "WebProcess.h"
#include <WebCore/DocumentMarkerController.h>
#include <WebCore/FocusController.h>
#include <WebCore/Frame.h>
#include <WebCore/FrameView.h>
#include <WebCore/GraphicsContext.h>
#include <WebCore/Page.h>

using namespace std;
using namespace WebCore;

namespace WebKit {

static WebCore::FindOptions core(FindOptions options)
{
    return (options & FindOptionsCaseInsensitive ? CaseInsensitive : 0)
        | (options & FindOptionsAtWordStarts ? AtWordStarts : 0)
        | (options & FindOptionsTreatMedialCapitalAsWordStart ? TreatMedialCapitalAsWordStart : 0)
        | (options & FindOptionsBackwards ? Backwards : 0)
        | (options & FindOptionsWrapAround ? WrapAround : 0);
}

FindController::FindController(WebPage* webPage)
    : m_webPage(webPage)
    , m_findPageOverlay(0)
    , m_isShowingFindIndicator(false)
{
}

FindController::~FindController()
{
}

void FindController::countStringMatches(const String& string, FindOptions options, unsigned maxMatchCount)
{
    if (maxMatchCount == numeric_limits<unsigned>::max())
        --maxMatchCount;
    
    unsigned matchCount = m_webPage->corePage()->markAllMatchesForText(string, core(options), false, maxMatchCount + 1);
    m_webPage->corePage()->unmarkAllTextMatches();

    // Check if we have more matches than allowed.
    if (matchCount > maxMatchCount)
        matchCount = static_cast<unsigned>(kWKMoreThanMaximumMatchCount);
    
    m_webPage->send(Messages::WebPageProxy::DidCountStringMatches(string, matchCount));
}

static Frame* frameWithSelection(Page* page)
{
    for (Frame* frame = page->mainFrame(); frame; frame = frame->tree()->traverseNext()) {
        if (frame->selection()->isRange())
            return frame;
    }

    return 0;
}

void FindController::findString(const String& string, FindOptions options, unsigned maxMatchCount)
{
    m_webPage->corePage()->unmarkAllTextMatches();

    bool found = m_webPage->corePage()->findString(string, core(options));

    Frame* selectedFrame = frameWithSelection(m_webPage->corePage());

    bool shouldShowOverlay = false;

    if (!found) {
        // Clear the selection.
        if (selectedFrame)
            selectedFrame->selection()->clear();

        hideFindIndicator();

        m_webPage->send(Messages::WebPageProxy::DidFailToFindString(string));
    } else {
        shouldShowOverlay = options & FindOptionsShowOverlay;
        bool shouldShowHighlight = options & FindOptionsShowHighlight;
        unsigned matchCount = 1;

        if (shouldShowOverlay || shouldShowHighlight) {

            if (maxMatchCount == numeric_limits<unsigned>::max())
                --maxMatchCount;

            matchCount = m_webPage->corePage()->markAllMatchesForText(string, core(options), shouldShowHighlight, maxMatchCount + 1);

            // Check if we have more matches than allowed.
            if (matchCount > maxMatchCount) {
                shouldShowOverlay = false;
                matchCount = static_cast<unsigned>(kWKMoreThanMaximumMatchCount);
            }
        }

        m_webPage->send(Messages::WebPageProxy::DidFindString(string, matchCount));

        if (!(options & FindOptionsShowFindIndicator) || !updateFindIndicator(selectedFrame, shouldShowOverlay)) {
            // Either we shouldn't show the find indicator, or we couldn't update it.
            hideFindIndicator();
        }
    }

    if (!shouldShowOverlay) {
        if (m_findPageOverlay) {
            // Get rid of the overlay.
            m_webPage->uninstallPageOverlay(m_findPageOverlay, false);
        }
        
        ASSERT(!m_findPageOverlay);
        return;
    }

    if (!m_findPageOverlay) {
        RefPtr<PageOverlay> findPageOverlay = PageOverlay::create(this);
        m_findPageOverlay = findPageOverlay.get();
        m_webPage->installPageOverlay(findPageOverlay.release());
    } else {
        // The page overlay needs to be repainted.
        m_findPageOverlay->setNeedsDisplay();
    }
}

void FindController::hideFindUI()
{
    if (m_findPageOverlay)
        m_webPage->uninstallPageOverlay(m_findPageOverlay, false);

    hideFindIndicator();
}

bool FindController::updateFindIndicator(Frame* selectedFrame, bool isShowingOverlay, bool shouldAnimate)
{
    if (!selectedFrame)
        return false;

    IntRect selectionRect = enclosingIntRect(selectedFrame->selection()->bounds());
    
    // Selection rect can be empty for matches that are currently obscured from view.
    if (selectionRect.isEmpty())
        return false;

    // We want the selection rect in window coordinates.
    IntRect selectionRectInWindowCoordinates = selectedFrame->view()->contentsToWindow(selectionRect);
    
    Vector<FloatRect> textRects;
    selectedFrame->selection()->getClippedVisibleTextRectangles(textRects);

    IntSize backingStoreSize = selectionRect.size();
    backingStoreSize.scale(m_webPage->corePage()->deviceScaleFactor());

    // Create a backing store and paint the find indicator text into it.
    RefPtr<ShareableBitmap> findIndicatorTextBackingStore = ShareableBitmap::createShareable(backingStoreSize, ShareableBitmap::SupportsAlpha);
    if (!findIndicatorTextBackingStore)
        return false;
    
    OwnPtr<GraphicsContext> graphicsContext = findIndicatorTextBackingStore->createGraphicsContext();
    graphicsContext->scale(FloatSize(m_webPage->corePage()->deviceScaleFactor(), m_webPage->corePage()->deviceScaleFactor()));

    IntRect paintRect = selectionRect;
    paintRect.move(selectedFrame->view()->frameRect().x(), selectedFrame->view()->frameRect().y());
    paintRect.move(-selectedFrame->view()->scrollOffset());

    graphicsContext->translate(-paintRect.x(), -paintRect.y());
    selectedFrame->view()->setPaintBehavior(PaintBehaviorSelectionOnly | PaintBehaviorForceBlackText | PaintBehaviorFlattenCompositingLayers);
    selectedFrame->document()->updateLayout();

    selectedFrame->view()->paint(graphicsContext.get(), paintRect);
    selectedFrame->view()->setPaintBehavior(PaintBehaviorNormal);
    
    ShareableBitmap::Handle handle;
    if (!findIndicatorTextBackingStore->createHandle(handle))
        return false;

    // We want the text rects in selection rect coordinates.
    Vector<FloatRect> textRectsInSelectionRectCoordinates;
    
    for (size_t i = 0; i < textRects.size(); ++i) {
        IntRect textRectInSelectionRectCoordinates = selectedFrame->view()->contentsToWindow(enclosingIntRect(textRects[i]));
        textRectInSelectionRectCoordinates.move(-selectionRectInWindowCoordinates.x(), -selectionRectInWindowCoordinates.y());

        textRectsInSelectionRectCoordinates.append(textRectInSelectionRectCoordinates);
    }            

    m_webPage->send(Messages::WebPageProxy::SetFindIndicator(selectionRectInWindowCoordinates, textRectsInSelectionRectCoordinates, m_webPage->corePage()->deviceScaleFactor(), handle, !isShowingOverlay, shouldAnimate));
    m_isShowingFindIndicator = true;

    return true;
}

void FindController::hideFindIndicator()
{
    if (!m_isShowingFindIndicator)
        return;

    ShareableBitmap::Handle handle;
    m_webPage->send(Messages::WebPageProxy::SetFindIndicator(FloatRect(), Vector<FloatRect>(), m_webPage->corePage()->deviceScaleFactor(), handle, false, true));
    m_isShowingFindIndicator = false;
}

void FindController::showFindIndicatorInSelection()
{
    Frame* selectedFrame = m_webPage->corePage()->focusController()->focusedOrMainFrame();
    if (!selectedFrame)
        return;
    
    updateFindIndicator(selectedFrame, false);
}

void FindController::deviceScaleFactorDidChange()
{
    ASSERT(isShowingOverlay());

    Frame* selectedFrame = frameWithSelection(m_webPage->corePage());
    if (!selectedFrame)
        return;

    updateFindIndicator(selectedFrame, true, false);
}

Vector<IntRect> FindController::rectsForTextMatches()
{
    Vector<IntRect> rects;

    for (Frame* frame = m_webPage->corePage()->mainFrame(); frame; frame = frame->tree()->traverseNext()) {
        Document* document = frame->document();
        if (!document)
            continue;

        IntRect visibleRect = frame->view()->visibleContentRect();
        Vector<IntRect> frameRects = document->markers()->renderedRectsForMarkers(DocumentMarker::TextMatch);
        IntPoint frameOffset(-frame->view()->scrollOffset().width(), -frame->view()->scrollOffset().height());
        frameOffset = frame->view()->convertToContainingWindow(frameOffset);

        for (Vector<IntRect>::iterator it = frameRects.begin(), end = frameRects.end(); it != end; ++it) {
            it->intersect(visibleRect);
            it->move(frameOffset.x(), frameOffset.y());
            rects.append(*it);
        }
    }

    return rects;
}

void FindController::pageOverlayDestroyed(PageOverlay*)
{
}

void FindController::willMoveToWebPage(PageOverlay*, WebPage* webPage)
{
    if (webPage)
        return;

    // The page overlay is moving away from the web page, reset it.
    ASSERT(m_findPageOverlay);
    m_findPageOverlay = 0;
}
    
void FindController::didMoveToWebPage(PageOverlay*, WebPage*)
{
}

static const float shadowOffsetX = 0.0;
static const float shadowOffsetY = 1.0;
static const float shadowBlurRadius = 2.0;
static const float whiteFrameThickness = 1.0;

static const float overlayBackgroundRed = 0.1;
static const float overlayBackgroundGreen = 0.1;
static const float overlayBackgroundBlue = 0.1;
static const float overlayBackgroundAlpha = 0.25;

static Color overlayBackgroundColor(float fractionFadedIn)
{
    return Color(overlayBackgroundRed, overlayBackgroundGreen, overlayBackgroundBlue, overlayBackgroundAlpha * fractionFadedIn);
}

static Color holeShadowColor(float fractionFadedIn)
{
    return Color(0.0f, 0.0f, 0.0f, fractionFadedIn);
}

static Color holeFillColor(float fractionFadedIn)
{
    return Color(1.0f, 1.0f, 1.0f, fractionFadedIn);
}

void FindController::drawRect(PageOverlay* pageOverlay, GraphicsContext& graphicsContext, const IntRect& dirtyRect)
{
    float fractionFadedIn = pageOverlay->fractionFadedIn();

    Vector<IntRect> rects = rectsForTextMatches();

    // Draw the background.
    graphicsContext.fillRect(dirtyRect, overlayBackgroundColor(fractionFadedIn), ColorSpaceSRGB);

    {
        GraphicsContextStateSaver stateSaver(graphicsContext);

        graphicsContext.setShadow(FloatSize(shadowOffsetX, shadowOffsetY), shadowBlurRadius, holeShadowColor(fractionFadedIn), ColorSpaceSRGB);
        graphicsContext.setFillColor(holeFillColor(fractionFadedIn), ColorSpaceSRGB);

        // Draw white frames around the holes.
        for (size_t i = 0; i < rects.size(); ++i) {
            IntRect whiteFrameRect = rects[i];
            whiteFrameRect.inflate(1);

            graphicsContext.fillRect(whiteFrameRect);
        }
    }

    graphicsContext.setFillColor(Color::transparent, ColorSpaceSRGB);

    // Clear out the holes.
    for (size_t i = 0; i < rects.size(); ++i)
        graphicsContext.fillRect(rects[i]);
}

bool FindController::mouseEvent(PageOverlay* pageOverlay, const WebMouseEvent& mouseEvent)
{
    // If we get a mouse down event inside the page overlay we should hide the find UI.
    if (mouseEvent.type() == WebEvent::MouseDown) {
        // Dismiss the overlay.
        hideFindUI();
    }

    return false;
}

} // namespace WebKit
