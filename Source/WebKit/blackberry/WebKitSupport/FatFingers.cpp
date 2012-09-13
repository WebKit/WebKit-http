/*
 * Copyright (C) 2010, 2011, 2012 Research In Motion Limited. All rights reserved.
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

#include "config.h"
#include "FatFingers.h"

#include "BlackBerryPlatformLog.h"
#include "BlackBerryPlatformScreen.h"
#include "BlackBerryPlatformSettings.h"
#include "CSSComputedStyleDeclaration.h"
#include "CSSParser.h"
#include "DOMSupport.h"
#include "Document.h"
#include "Element.h"
#include "EventNames.h"
#include "ExceptionCode.h"
#include "FloatQuad.h"
#include "Frame.h"
#include "FrameView.h"
#include "HTMLFrameOwnerElement.h"
#include "HTMLInputElement.h"
#include "HTMLNames.h"
#include "HTMLTextAreaElement.h"
#include "Range.h"
#include "RenderObject.h"
#include "RenderView.h"
#include "Text.h"
#include "TextBreakIterator.h"
#include "WebPage_p.h"

#if DEBUG_FAT_FINGERS
#include "BackingStore.h"
#endif

using BlackBerry::Platform::LogLevelInfo;
using BlackBerry::Platform::log;
using BlackBerry::Platform::IntRectRegion;
using WTF::RefPtr;

using namespace WebCore;

// Lets make the top padding bigger than other directions, since it gets us more
// accurate clicking results.

namespace BlackBerry {
namespace WebKit {

#if DEBUG_FAT_FINGERS
IntRect FatFingers::m_debugFatFingerRect;
IntPoint FatFingers::m_debugFatFingerClickPosition;
IntPoint FatFingers::m_debugFatFingerAdjustedPosition;
#endif

IntRect FatFingers::fingerRectForPoint(const IntPoint& point) const
{
    unsigned topPadding, rightPadding, bottomPadding, leftPadding;
    getPaddings(topPadding, rightPadding, bottomPadding, leftPadding);

    return HitTestResult::rectForPoint(point, topPadding, rightPadding, bottomPadding, leftPadding);
}

static bool hasMousePressListener(Element* element)
{
    ASSERT(element);
    return element->hasEventListeners(eventNames().clickEvent)
        || element->hasEventListeners(eventNames().mousedownEvent)
        || element->hasEventListeners(eventNames().mouseupEvent);
}

bool FatFingers::isElementClickable(Element* element) const
{
    ASSERT(element);
    ASSERT(m_matchingApproach != Done);
    ASSERT(m_targetType == ClickableElement);

    switch (m_matchingApproach) {
    case ClickableByDefault: {
        ExceptionCode ec = 0;
        return element->webkitMatchesSelector("a[href],*:link,*:visited,*[role=button],button,input,select,label[for],area[href],textarea,embed,object", ec)
            || element->isMediaControlElement()
            || element->isContentEditable();
    }
    case MadeClickableByTheWebpage:

        // Elements within a shadow DOM can not be 'made clickable by the webpage', since
        // they are not accessible.
        if (element->isInShadowTree())
            return false;

        // FIXME: We fall back to checking for the presence of CSS style "cursor: pointer" to indicate whether the element A
        // can be clicked when A neither registers mouse events handlers nor is a hyperlink or form control. This workaround
        // ensures that we don't break various Google web apps, including <http://maps.google.com>. Ideally, we should walk
        // up the DOM hierarchy to determine the first parent element that accepts mouse events.
        // Consider the HTML snippet: <div id="A" onclick="..."><div id="B">Example</div></div>
        // Notice, B is not a hyperlink, or form control, and does not register any mouse event handler. Then B cannot
        // be clicked. Suppose B specified the CSS property "cursor: pointer". Then, B will be considered as clickable.
        return hasMousePressListener(element)
            || CSSComputedStyleDeclaration::create(element)->getPropertyValue(cssPropertyID("cursor")) == "pointer";
    default:
        ASSERT_NOT_REACHED();
    }

    return false;
}

// FIXME: Handle content editable nodes here too.
static inline bool isFieldWithText(Node* node)
{
    ASSERT(node);
    if (!node || !node->isElementNode())
        return false;

    Element* element = toElement(node);
    return !DOMSupport::inputElementText(element).isEmpty();
}

static inline int distanceBetweenPoints(const IntPoint& p1, const IntPoint& p2)
{
    int dx = p1.x() - p2.x();
    int dy = p1.y() - p2.y();
    return sqrt((double)((dx * dx) + (dy * dy)));
}

static bool compareDistanceBetweenPoints(const Platform::IntPoint& p, const Platform::IntRectRegion& r1, const Platform::IntRectRegion& r2)
{
    return distanceBetweenPoints(p, r1.extents().center()) > distanceBetweenPoints(p, r2.extents().center());
}

static bool isValidFrameOwner(WebCore::Element* element)
{
    ASSERT(element);
    return element->isFrameOwnerElement() && static_cast<HTMLFrameOwnerElement*>(element)->contentFrame();
}

// NOTE: 'contentPos' is in main frame contents coordinates.
FatFingers::FatFingers(WebPagePrivate* webPage, const WebCore::IntPoint& contentPos, TargetType targetType)
    : m_webPage(webPage)
    , m_contentPos(contentPos)
    , m_targetType(targetType)
    , m_matchingApproach(Done)
{
    ASSERT(webPage);

#if DEBUG_FAT_FINGERS
    m_debugFatFingerRect = IntRect(0, 0, 0, 0);
    m_debugFatFingerClickPosition = m_webPage->mapToTransformed(m_webPage->mapFromContentsToViewport(contentPos));
    m_debugFatFingerAdjustedPosition = m_webPage->mapToTransformed(m_webPage->mapFromContentsToViewport(contentPos));
#endif
}

FatFingers::~FatFingers()
{
}

const FatFingersResult FatFingers::findBestPoint()
{
    ASSERT(m_webPage);
    ASSERT(m_webPage->m_mainFrame);

    m_cachedRectHitTestResults.clear();

    FatFingersResult result(m_contentPos);
    m_matchingApproach = ClickableByDefault;

    // Lets set nodeUnderFatFinger to the result of a point based hit test here. If something
    // targable is actually found by ::findIntersectingRegions, then we might replace what we just set below later on.
    Element* elementUnderPoint;
    Element* clickableElementUnderPoint;
    getRelevantInfoFromPoint(m_webPage->m_mainFrame->document(), m_contentPos, elementUnderPoint, clickableElementUnderPoint);

    if (elementUnderPoint) {
        result.m_nodeUnderFatFinger = elementUnderPoint;

        // If we are looking for a Clickable Element and we found one, we can quit early.
        if (m_targetType == ClickableElement) {
            if (clickableElementUnderPoint) {
                setSuccessfulFatFingersResult(result, clickableElementUnderPoint, m_contentPos /*adjustedPosition*/);
                return result;
            }

            if (isElementClickable(elementUnderPoint)) {
                setSuccessfulFatFingersResult(result, elementUnderPoint, m_contentPos /*adjustedPosition*/);
                return result;
            }
        }
    }

#if DEBUG_FAT_FINGERS
    // Force blit to make the fat fingers rects show up.
    if (!m_debugFatFingerRect.isEmpty())
        m_webPage->m_backingStore->repaint(0, 0, m_webPage->transformedViewportSize().width(), m_webPage->transformedViewportSize().height(), true, true);
#endif

    Vector<IntersectingRegion> intersectingRegions;
    Platform::IntRectRegion remainingFingerRegion = Platform::IntRectRegion(fingerRectForPoint(m_contentPos));

    bool foundOne = findIntersectingRegions(m_webPage->m_mainFrame->document(), intersectingRegions, remainingFingerRegion);
    if (!foundOne) {
        m_matchingApproach = MadeClickableByTheWebpage;
        remainingFingerRegion = Platform::IntRectRegion(fingerRectForPoint(m_contentPos));
        foundOne = findIntersectingRegions(m_webPage->m_mainFrame->document(), intersectingRegions, remainingFingerRegion);
    }

    m_matchingApproach = Done;
    m_cachedRectHitTestResults.clear();

    if (!foundOne)
        return result;

    Node* bestNode = 0;
    Platform::IntRectRegion largestIntersectionRegion;
    IntPoint bestPoint;
    int largestIntersectionRegionArea = 0;

    Vector<IntersectingRegion>::const_iterator endIt = intersectingRegions.end();
    for (Vector<IntersectingRegion>::const_iterator it = intersectingRegions.begin(); it != endIt; ++it) {
        Node* currentNode = it->first;
        Platform::IntRectRegion currentIntersectionRegion = it->second;

        int currentIntersectionRegionArea = currentIntersectionRegion.area();
        if (currentIntersectionRegionArea > largestIntersectionRegionArea
            || (currentIntersectionRegionArea == largestIntersectionRegionArea
            && compareDistanceBetweenPoints(m_contentPos, currentIntersectionRegion, largestIntersectionRegion))) {
            bestNode = currentNode;
            largestIntersectionRegion = currentIntersectionRegion;
            largestIntersectionRegionArea = currentIntersectionRegionArea;
        }
    }

    if (!bestNode || largestIntersectionRegion.isEmpty())
        return result;

#if DEBUG_FAT_FINGERS
    m_debugFatFingerAdjustedPosition = m_webPage->mapToTransformed(m_webPage->mapFromContentsToViewport(largestIntersectionRegion.rects()[0].center()));
#endif

    setSuccessfulFatFingersResult(result, bestNode, largestIntersectionRegion.rects()[0].center() /*adjustedPosition*/);

    return result;
}

// 'region' is in contents coordinates relative to the frame containing 'node'
// 'remainingFingerRegion' and 'intersectingRegions' will always be in main frame contents
// coordinates.
// Thus, before comparing, we need to map the former to main frame contents coordinates.
bool FatFingers::checkFingerIntersection(const Platform::IntRectRegion& region,
                                         const Platform::IntRectRegion& remainingFingerRegion,
                                         Node* node, Vector<IntersectingRegion>& intersectingRegions)
{
    ASSERT(node);

    Platform::IntRectRegion regionCopy(region);
    WebCore::IntPoint framePos(m_webPage->frameOffset(node->document()->frame()));
    regionCopy.move(framePos.x(), framePos.y());

    Platform::IntRectRegion intersection = intersectRegions(regionCopy, remainingFingerRegion);
    if (intersection.isEmpty())
        return false;

#if DEBUG_FAT_FINGERS
    String nodeName;
    if (node->isTextNode())
        nodeName = "text node";
    else if (node->isElementNode())
        nodeName = String::format("%s node", toElement(node)->tagName().latin1().data());
    else
        nodeName = "unknown node";
    log(LogLevelInfo, "%s has region %s, intersecting at %s (area %d)", nodeName.latin1().data(),
        regionCopy.toString().c_str(), intersection.toString().c_str(), intersection.area());
#endif

    intersectingRegions.append(std::make_pair(node, intersection));
    return true;
}


// intersectingRegions and remainingFingerRegion are all in main frame contents coordinates,
// even on recursive calls of ::findIntersectingRegions.
bool FatFingers::findIntersectingRegions(Document* document,
                                         Vector<IntersectingRegion>& intersectingRegions, Platform::IntRectRegion& remainingFingerRegion)
{
    if (!document || !document->frame()->view())
        return false;

    // The layout needs to be up-to-date to determine if a node is focusable.
    document->updateLayoutIgnorePendingStylesheets();

    // Create fingerRect.
    IntPoint frameContentPos(document->frame()->view()->windowToContents(m_webPage->m_mainFrame->view()->contentsToWindow(m_contentPos)));

#if DEBUG_FAT_FINGERS
    IntRect fingerRect(fingerRectForPoint(frameContentPos));
    IntRect screenFingerRect = m_webPage->mapToTransformed(fingerRect);
    log(LogLevelInfo, "fat finger rect now %d, %d, %d, %d", screenFingerRect.x(), screenFingerRect.y(), screenFingerRect.width(), screenFingerRect.height());

    // only record the first finger rect
    if (document == m_webPage->m_mainFrame->document())
        m_debugFatFingerRect = m_webPage->mapToTransformed(m_webPage->mapFromContentsToViewport(fingerRect));
#endif

    bool foundOne = false;

    RenderLayer* lowestPositionedEnclosingLayerSoFar = 0;

    // Iterate over the list of nodes (and subrects of nodes where possible), for each saving the
    // intersection of the bounding box with the finger rect.
    ListHashSet<RefPtr<Node> > intersectedNodes;
    getNodesFromRect(document, frameContentPos, intersectedNodes);

    ListHashSet<RefPtr<Node> >::const_iterator it = intersectedNodes.begin();
    ListHashSet<RefPtr<Node> >::const_iterator end = intersectedNodes.end();
    for ( ; it != end; ++it) {
        Node* curNode = (*it).get();
        if (!curNode || !curNode->renderer())
            continue;

        if (remainingFingerRegion.isEmpty())
            break;

        bool isElement = curNode->isElementNode();
        if (isElement && isValidFrameOwner(toElement(curNode))) {

            HTMLFrameOwnerElement* owner = static_cast<HTMLFrameOwnerElement*>(curNode);
            Document* childDocument = owner && owner->contentFrame() ? owner->contentFrame()->document() : 0;
            if (!childDocument)
                continue;

            ASSERT(childDocument->frame()->view());

            foundOne |= findIntersectingRegions(childDocument, intersectingRegions, remainingFingerRegion);
        } else if (isElement && m_targetType == ClickableElement) {
            foundOne |= checkForClickableElement(toElement(curNode), intersectingRegions, remainingFingerRegion, lowestPositionedEnclosingLayerSoFar);
        } else if (m_targetType == Text)
            foundOne |= checkForText(curNode, intersectingRegions, remainingFingerRegion);
    }

    return foundOne;
}

bool FatFingers::checkForClickableElement(Element* curElement,
                                          Vector<IntersectingRegion>& intersectingRegions,
                                          Platform::IntRectRegion& remainingFingerRegion,
                                          RenderLayer*& lowestPositionedEnclosingLayerSoFar)
{
    ASSERT(curElement);

    bool intersects = false;
    Platform::IntRectRegion elementRegion;

    bool isClickableElement = isElementClickable(curElement);
    if (isClickableElement) {
        if (curElement->isLink()) {
            // Links can wrap lines, and in such cases Node::getRect() can give us
            // not accurate rects, since it unites all InlineBox's rects. In these
            // cases, we can process each line of the link separately with our
            // intersection rect, getting a more accurate clicking.
            Vector<FloatQuad> quads;
            curElement->renderer()->absoluteFocusRingQuads(quads);

            size_t n = quads.size();
            ASSERT(n);

            for (size_t i = 0; i < n; ++i)
                elementRegion = unionRegions(elementRegion, Platform::IntRect(quads[i].enclosingBoundingBox()));
        } else
            elementRegion = Platform::IntRectRegion(curElement->renderer()->absoluteBoundingBoxRect(true /*use transforms*/));

    } else
        elementRegion = Platform::IntRectRegion(curElement->renderer()->absoluteBoundingBoxRect(true /*use transforms*/));

    if (lowestPositionedEnclosingLayerSoFar) {
        RenderLayer* curElementRenderLayer = m_webPage->enclosingPositionedAncestorOrSelfIfPositioned(curElement->renderer()->enclosingLayer());
        if (curElementRenderLayer != lowestPositionedEnclosingLayerSoFar) {

            // elementRegion will always be in contents coordinates of its container frame. It needs to be
            // mapped to main frame contents coordinates in order to subtract the fingerRegion, then.
            WebCore::IntPoint framePos(m_webPage->frameOffset(curElement->document()->frame()));
            Platform::IntRectRegion layerRegion(Platform::IntRect(lowestPositionedEnclosingLayerSoFar->renderer()->absoluteBoundingBoxRect(true/*use transforms*/)));
            layerRegion.move(framePos.x(), framePos.y());

            remainingFingerRegion = subtractRegions(remainingFingerRegion, layerRegion);

            lowestPositionedEnclosingLayerSoFar = curElementRenderLayer;
        }
    } else
        lowestPositionedEnclosingLayerSoFar = m_webPage->enclosingPositionedAncestorOrSelfIfPositioned(curElement->renderer()->enclosingLayer());

    if (isClickableElement)
        intersects = checkFingerIntersection(elementRegion, remainingFingerRegion, curElement, intersectingRegions);

    return intersects;
}

bool FatFingers::checkForText(Node* curNode, Vector<IntersectingRegion>& intersectingRegions, Platform::IntRectRegion& fingerRegion)
{
    ASSERT(curNode);
    if (isFieldWithText(curNode)) {
        // FIXME: Find all text in the field and find the best word.
        // For now, we will just select the whole field.
        IntRect boundingRect = curNode->renderer()->absoluteBoundingBoxRect(true /*use transforms*/);
        Platform::IntRectRegion nodeRegion(boundingRect);
        return checkFingerIntersection(nodeRegion, fingerRegion, curNode, intersectingRegions);
    }

    if (curNode->isTextNode()) {
        WebCore::Text* curText = static_cast<WebCore::Text*>(curNode);
        String allText = curText->wholeText();

        // Iterate through all words, breaking at whitespace, to find the bounding box of each word.
        TextBreakIterator* wordIterator = wordBreakIterator(allText.characters(), allText.length());

        int lastOffset = textBreakFirst(wordIterator);
        if (lastOffset == -1)
            return false;

        bool foundOne = false;
        int offset;
        Document* document = curNode->document();

        while ((offset = textBreakNext(wordIterator)) != -1) {
            RefPtr<Range> range = Range::create(document, curText, lastOffset, curText, offset);
            if (!range->text().stripWhiteSpace().isEmpty()) {
#if DEBUG_FAT_FINGERS
                log(LogLevelInfo, "Checking word '%s'", range->text().latin1().data());
#endif
                Platform::IntRectRegion rangeRegion(DOMSupport::transformedBoundingBoxForRange(*range));
                foundOne |= checkFingerIntersection(rangeRegion, fingerRegion, curNode, intersectingRegions);
            }
            lastOffset = offset;
        }
        return foundOne;
    }
    return false;
}

void FatFingers::getPaddings(unsigned& top, unsigned& right, unsigned& bottom, unsigned& left) const
{
    static unsigned topPadding = Platform::Settings::instance()->topFatFingerPadding();
    static unsigned rightPadding = Platform::Settings::instance()->rightFatFingerPadding();
    static unsigned bottomPadding = Platform::Settings::instance()->bottomFatFingerPadding();
    static unsigned leftPadding = Platform::Settings::instance()->leftFatFingerPadding();

    double currentScale = m_webPage->currentScale();
    top = topPadding / currentScale;
    right = rightPadding / currentScale;
    bottom = bottomPadding / currentScale;
    left = leftPadding / currentScale;
}

FatFingers::CachedResultsStrategy FatFingers::cachingStrategy() const
{
    switch (m_matchingApproach) {
    case ClickableByDefault:
        return GetFromRenderTree;
    case MadeClickableByTheWebpage:
        return GetFromCache;
    case Done:
    default:
        ASSERT_NOT_REACHED();
        return GetFromRenderTree;
    }
}

void FatFingers::getNodesFromRect(Document* document, const IntPoint& contentPos, ListHashSet<RefPtr<WebCore::Node> >& intersectedNodes)
{
    FatFingers::CachedResultsStrategy cacheResolvingStrategy = cachingStrategy();

    if (cacheResolvingStrategy == GetFromCache) {
        ASSERT(m_cachedRectHitTestResults.contains(document));
        intersectedNodes = m_cachedRectHitTestResults.get(document);
        return;
    }

    ASSERT(cacheResolvingStrategy == GetFromRenderTree);

    unsigned topPadding, rightPadding, bottomPadding, leftPadding;
    getPaddings(topPadding, rightPadding, bottomPadding, leftPadding);

    HitTestRequest request(HitTestRequest::ReadOnly | HitTestRequest::Active | HitTestRequest::IgnoreClipping);
    // The user functions checkForText() and findIntersectingRegions() uses the Node.wholeText() to checkFingerIntersection()
    // not the text in its shadow tree.
    ShadowContentFilterPolicy allowShadow = m_targetType == Text ? DoNotAllowShadowContent : AllowShadowContent;
    HitTestResult result(contentPos, topPadding, rightPadding, bottomPadding, leftPadding, allowShadow);

    document->renderView()->layer()->hitTest(request, result);
    intersectedNodes = result.rectBasedTestResult();
    m_cachedRectHitTestResults.add(document, intersectedNodes);
}

void FatFingers::getRelevantInfoFromPoint(Document* document, const IntPoint& contentPos, Element*& elementUnderPoint, Element*& clickableElementUnderPoint) const
{
    elementUnderPoint = 0;
    clickableElementUnderPoint = 0;

    if (!document || !document->renderer() || !document->frame())
        return;

    HitTestResult result  = document->frame()->eventHandler()->hitTestResultAtPoint(contentPos, true /*allowShadowContent*/);
    Node* node = result.innerNode();
    while (node && !node->isElementNode())
        node = node->parentNode();

    elementUnderPoint = static_cast<Element*>(node);
    clickableElementUnderPoint = result.URLElement();
}

void FatFingers::setSuccessfulFatFingersResult(FatFingersResult& result, Node* bestNode, const WebCore::IntPoint& adjustedPoint)
{
    result.m_nodeUnderFatFinger = bestNode;
    result.m_adjustedPosition = adjustedPoint;
    result.m_positionWasAdjusted = true;
    result.m_isValid = true;

    bool isTextInputElement = false;
    if (m_targetType == ClickableElement) {
        ASSERT(bestNode->isElementNode());
        Element* bestElement = static_cast<Element*>(bestNode);
        isTextInputElement = DOMSupport::isTextInputElement(bestElement);
    }
    result.m_isTextInput = isTextInputElement;
}

}
}

