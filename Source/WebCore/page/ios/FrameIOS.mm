/*
 * Copyright (C) 2006-2018 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#import "config.h"
#import "Frame.h"

#if PLATFORM(IOS_FAMILY)

#import "CommonVM.h"
#import "ComposedTreeIterator.h"
#import "DOMWindow.h"
#import "Document.h"
#import "DocumentMarkerController.h"
#import "Editor.h"
#import "EditorClient.h"
#import "EventHandler.h"
#import "EventNames.h"
#import "FormController.h"
#import "FrameSelection.h"
#import "FrameView.h"
#import "HTMLAreaElement.h"
#import "HTMLBodyElement.h"
#import "HTMLDocument.h"
#import "HTMLHtmlElement.h"
#import "HTMLNames.h"
#import "HTMLObjectElement.h"
#import "HitTestRequest.h"
#import "HitTestResult.h"
#import "Logging.h"
#import "NodeRenderStyle.h"
#import "NodeTraversal.h"
#import "Page.h"
#import "PageTransitionEvent.h"
#import "PlatformScreen.h"
#import "PropertySetCSSStyleDeclaration.h"
#import "Range.h"
#import "RenderLayer.h"
#import "RenderLayerCompositor.h"
#import "RenderLayerScrollableArea.h"
#import "RenderTextControl.h"
#import "RenderView.h"
#import "RenderedDocumentMarker.h"
#import "ShadowRoot.h"
#import "TextBoundaries.h"
#import "TextIterator.h"
#import "VisiblePosition.h"
#import "VisibleUnits.h"
#import "WAKWindow.h"
#import <JavaScriptCore/JSLock.h>
#import <wtf/BlockObjCExceptions.h>
#import <wtf/cocoa/VectorCocoa.h>
#import <wtf/text/TextStream.h>

using namespace WebCore::HTMLNames;
using namespace WTF::Unicode;

using JSC::JSLockHolder;

namespace WebCore {

// Create <html><body (style="...")></body></html> doing minimal amount of work.
void Frame::initWithSimpleHTMLDocument(const String& style, const URL& url)
{
    m_loader->initForSynthesizedDocument(url);

    auto document = HTMLDocument::createSynthesizedDocument(*this, url);
    document->setCompatibilityMode(DocumentCompatibilityMode::LimitedQuirksMode);
    document->createDOMWindow();
    setDocument(document.copyRef());

    auto rootElement = HTMLHtmlElement::create(document);

    auto body = HTMLBodyElement::create(document);
    if (!style.isEmpty())
        body->setAttribute(HTMLNames::styleAttr, style);

    rootElement->appendChild(body);
    document->appendChild(rootElement);
}

const ViewportArguments& Frame::viewportArguments() const
{
    return m_viewportArguments;
}

void Frame::setViewportArguments(const ViewportArguments& arguments)
{
    m_viewportArguments = arguments;
}

NSArray *Frame::wordsInCurrentParagraph() const
{
    document()->updateLayout();

    if (!page() || !page()->selection().isCaret())
        return nil;

    VisiblePosition position(page()->selection().start(), page()->selection().affinity());
    VisiblePosition end(position);

    if (!isStartOfParagraph(end)) {
        VisiblePosition previous = end.previous();
        UChar c(previous.characterAfter());
        // FIXME: Should use something from ICU or ASCIICType that is not subject to POSIX current language rather than iswpunct.
        if (!iswpunct(c) && !isSpaceOrNewline(c) && c != noBreakSpace)
            end = startOfWord(end);
    }
    VisiblePosition start(startOfParagraph(end));

    auto searchRange = makeSimpleRange(start, end);
    if (!searchRange || searchRange->collapsed())
        return nil;

    NSMutableArray *words = [NSMutableArray array];

    WordAwareIterator it(*searchRange);
    while (!it.atEnd()) {
        StringView text = it.text();
        int length = text.length();
        if (length > 1 || !isSpaceOrNewline(text[0])) {
            int startOfWordBoundary = 0;
            for (int i = 1; i < length; i++) {
                if (isSpaceOrNewline(text[i]) || text[i] == noBreakSpace) {
                    int wordLength = i - startOfWordBoundary;
                    if (wordLength > 0) {
                        RetainPtr<NSString> chunk = text.substring(startOfWordBoundary, wordLength).createNSString();
                        [words addObject:chunk.get()];
                    }
                    startOfWordBoundary += wordLength + 1;
                }
            }
            if (startOfWordBoundary < length) {
                RetainPtr<NSString> chunk = text.substring(startOfWordBoundary, length - startOfWordBoundary).createNSString();
                [words addObject:chunk.get()];
            }
        }
        it.advance();
    }

    if ([words count] > 0 && isEndOfParagraph(position) && !isStartOfParagraph(position)) {
        VisiblePosition previous = position.previous();
        UChar c(previous.characterAfter());
        if (!isSpaceOrNewline(c) && c != noBreakSpace)
            [words removeLastObject];
    }

    return words;
}

#define CHECK_FONT_SIZE 0
#define RECT_LOGGING 0

CGRect Frame::renderRectForPoint(CGPoint point, bool* isReplaced, float* fontSize) const
{
    *isReplaced = false;
    *fontSize = 0;

    if (!m_doc || !m_doc->renderBox())
        return CGRectZero;

    // FIXME: Why this layer check?
    RenderLayer* layer = m_doc->renderBox()->layer();
    if (!layer)
        return CGRectZero;

    constexpr OptionSet<HitTestRequest::RequestType> hitType { HitTestRequest::ReadOnly, HitTestRequest::Active, HitTestRequest::DisallowUserAgentShadowContent, HitTestRequest::AllowChildFrameContent };
    auto result = eventHandler().hitTestResultAtPoint(IntPoint(roundf(point.x), roundf(point.y)), hitType);

    Node* node = result.innerNode();
    if (!node)
        return CGRectZero;

    RenderObject* hitRenderer = node->renderer();
    RenderObject* renderer = hitRenderer;
#if RECT_LOGGING
    printf("\n%f %f\n", point.x, point.y);
#endif
    while (renderer && !renderer->isBody() && !renderer->isDocumentElementRenderer()) {
#if RECT_LOGGING
        CGRect rect = renderer->absoluteBoundingBoxRect(true);
        if (renderer->node()) {
            const char *nodeName = renderer->node()->nodeName().ascii().data();
            printf("%s %f %f %f %f\n", nodeName, rect.origin.x, rect.origin.y, rect.size.width, rect.size.height);
        }
#endif
        if (renderer->isRenderBlock() || renderer->isInlineBlockOrInlineTable() || renderer->isReplaced()) {
            *isReplaced = renderer->isReplaced();
#if CHECK_FONT_SIZE
            for (RenderObject* textRenderer = hitRenderer; textRenderer; textRenderer = textRenderer->traverseNext(hitRenderer)) {
                if (textRenderer->isText()) {
                    *fontSize = textRenderer->font(true).pixelSize();
                    break;
                }
            }
#endif
            IntRect targetRect = renderer->absoluteBoundingBoxRect(true);
            for (Widget* currView = &(renderer->view().frameView()); currView && currView != view(); currView = currView->parent())
                targetRect = currView->convertToContainingView(targetRect);

            return targetRect;
        }
        renderer = renderer->parent();
    }

    return CGRectZero;
}

int Frame::preferredHeight() const
{
    Document* document = this->document();
    if (!document)
        return 0;

    document->updateLayout();

    auto* body = document->bodyOrFrameset();
    if (!body)
        return 0;

    RenderObject* renderer = body->renderer();
    if (!is<RenderBlock>(renderer))
        return 0;

    RenderBlock& block = downcast<RenderBlock>(*renderer);
    return block.height() + block.marginTop() + block.marginBottom();
}

void Frame::updateLayout() const
{
    Document* document = this->document();
    if (!document)
        return;

    document->updateLayout();

    if (FrameView* view = this->view())
        view->adjustViewSize();
}

NSRect Frame::caretRect()
{
    VisibleSelection visibleSelection = selection().selection();
    if (visibleSelection.isNone())
        return CGRectZero;
    return visibleSelection.isCaret() ? selection().absoluteCaretBounds() : VisiblePosition(visibleSelection.end()).absoluteCaretBounds();
}

NSRect Frame::rectForScrollToVisible()
{
    VisibleSelection selection(this->selection().selection());

    if (selection.isNone())
        return CGRectZero;

    if (selection.isCaret())
        return caretRect();

    return unionRect(selection.visibleStart().absoluteCaretBounds(), selection.visibleEnd().absoluteCaretBounds());
}

unsigned Frame::formElementsCharacterCount() const
{
    Document* document = this->document();
    if (!document)
        return 0;
    return document->formController().formElementsCharacterCount();
}

void Frame::setTimersPaused(bool paused)
{
    if (!m_page)
        return;
    JSLockHolder lock(commonVM());
    if (paused)
        m_page->suspendActiveDOMObjectsAndAnimations();
    else
        m_page->resumeActiveDOMObjectsAndAnimations();
}

void Frame::dispatchPageHideEventBeforePause()
{
    ASSERT(isMainFrame());
    if (!isMainFrame())
        return;

    for (Frame* frame = this; frame; frame = frame->tree().traverseNext(this))
        frame->document()->domWindow()->dispatchEvent(PageTransitionEvent::create(eventNames().pagehideEvent, true), document());
}

void Frame::dispatchPageShowEventBeforeResume()
{
    ASSERT(isMainFrame());
    if (!isMainFrame())
        return;

    for (Frame* frame = this; frame; frame = frame->tree().traverseNext(this))
        frame->document()->domWindow()->dispatchEvent(PageTransitionEvent::create(eventNames().pageshowEvent, true), document());
}

void Frame::setRangedSelectionBaseToCurrentSelection()
{
    m_rangedSelectionBase = selection().selection();
}

void Frame::setRangedSelectionBaseToCurrentSelectionStart()
{
    const VisibleSelection& visibleSelection = selection().selection();
    m_rangedSelectionBase = VisibleSelection(visibleSelection.start(), visibleSelection.affinity());
}

void Frame::setRangedSelectionBaseToCurrentSelectionEnd()
{
    const VisibleSelection& visibleSelection = selection().selection();
    m_rangedSelectionBase = VisibleSelection(visibleSelection.end(), visibleSelection.affinity());
}

VisibleSelection Frame::rangedSelectionBase() const
{
    return m_rangedSelectionBase;
}

void Frame::clearRangedSelectionInitialExtent()
{
    m_rangedSelectionInitialExtent = VisibleSelection();
}

void Frame::setRangedSelectionInitialExtentToCurrentSelectionStart()
{
    const VisibleSelection& visibleSelection = selection().selection();
    m_rangedSelectionInitialExtent = VisibleSelection(visibleSelection.start(), visibleSelection.affinity());
}

void Frame::setRangedSelectionInitialExtentToCurrentSelectionEnd()
{
    const VisibleSelection& visibleSelection = selection().selection();
    m_rangedSelectionInitialExtent = VisibleSelection(visibleSelection.end(), visibleSelection.affinity());
}

VisibleSelection Frame::rangedSelectionInitialExtent() const
{
    return m_rangedSelectionInitialExtent;
}

void Frame::recursiveSetUpdateAppearanceEnabled(bool enabled)
{
    selection().setUpdateAppearanceEnabled(enabled);
    for (Frame* child = tree().firstChild(); child; child = child->tree().nextSibling())
        child->recursiveSetUpdateAppearanceEnabled(enabled);
}

// FIXME: Break this function up into pieces with descriptive function names so that it's easier to follow.
NSArray *Frame::interpretationsForCurrentRoot() const
{
    if (!document())
        return nil;

    auto* root = selection().isNone() ? document()->bodyOrFrameset() : selection().selection().rootEditableElement();
    auto rangeOfRootContents = makeRangeSelectingNodeContents(*root);

    auto markersInRoot = document()->markers().markersInRange(rangeOfRootContents, DocumentMarker::DictationPhraseWithAlternatives);

    // There are no phrases with alternatives, so there is just one interpretation.
    if (markersInRoot.isEmpty())
        return @[plainText(rangeOfRootContents)];

    // The number of interpretations will be i1 * i2 * ... * iN, where iX is the number of interpretations for the Xth phrase with alternatives.
    size_t interpretationsCount = 1;

    for (auto* marker : markersInRoot)
        interpretationsCount *= WTF::get<Vector<String>>(marker->data()).size() + 1;

    Vector<Vector<UChar>> interpretations;
    interpretations.grow(interpretationsCount);

    Position precedingTextStartPosition = makeDeprecatedLegacyPosition(root, 0);

    unsigned combinationsSoFar = 1;

    for (auto& node : intersectingNodes(rangeOfRootContents)) {
        for (auto* marker : document()->markers().markersFor(node, DocumentMarker::DictationPhraseWithAlternatives)) {
            auto& alternatives = WTF::get<Vector<String>>(marker->data());

            auto rangeForMarker = makeSimpleRange(node, *marker);

            if (auto precedingTextRange = makeSimpleRange(precedingTextStartPosition, rangeForMarker.start)) {
                String precedingText = plainText(*precedingTextRange);
                if (!precedingText.isEmpty()) {
                    for (auto& interpretation : interpretations)
                        append(interpretation, precedingText);
                }
            }

            String visibleTextForMarker = plainText(rangeForMarker);
            size_t interpretationsCountForCurrentMarker = alternatives.size() + 1;
            for (size_t i = 0; i < interpretationsCount; ++i) {
                size_t indexOfInterpretationForCurrentMarker = (i / combinationsSoFar) % interpretationsCountForCurrentMarker;
                if (!indexOfInterpretationForCurrentMarker)
                    append(interpretations[i], visibleTextForMarker);
                else
                    append(interpretations[i], alternatives[i % alternatives.size()]);
            }

            combinationsSoFar *= interpretationsCountForCurrentMarker;

            precedingTextStartPosition = makeDeprecatedLegacyPosition(rangeForMarker.end);
        }
    }

    // Finally, add any text after the last marker.
    if (auto range = makeSimpleRange(precedingTextStartPosition, rangeOfRootContents.end)) {
        String textAfterLastMarker = plainText(*range);
        if (!textAfterLastMarker.isEmpty()) {
            for (auto& interpretation : interpretations)
                append(interpretation, textAfterLastMarker);
        }
    }

    return createNSArray(interpretations, [] (auto& interpretation) {
        return adoptNS([[NSString alloc] initWithCharacters:reinterpret_cast<const unichar*>(interpretation.data()) length:interpretation.size()]);
    }).autorelease();
}

void Frame::viewportOffsetChanged(ViewportOffsetChangeType changeType)
{
    LOG_WITH_STREAM(Scrolling, stream << "Frame::viewportOffsetChanged - " << (changeType == IncrementalScrollOffset ? "incremental" : "completed"));

    if (changeType == IncrementalScrollOffset) {
        if (RenderView* root = contentRenderer())
            root->compositor().didChangeVisibleRect();
    }

    if (changeType == CompletedScrollOffset) {
        if (RenderView* root = contentRenderer())
            root->compositor().updateCompositingLayers(CompositingUpdateType::OnScroll);
    }
}

bool Frame::containsTiledBackingLayers() const
{
    if (RenderView* root = contentRenderer())
        return root->compositor().hasNonMainLayersWithTiledBacking();

    return false;
}

void Frame::overflowScrollPositionChangedForNode(const IntPoint& position, Node* node, bool isUserScroll)
{
    LOG_WITH_STREAM(Scrolling, stream << "Frame::overflowScrollPositionChangedForNode " << node << " position " << position);

    RenderObject* renderer = node->renderer();
    if (!renderer || !renderer->hasLayer())
        return;

    auto* layer = downcast<RenderBoxModelObject>(*renderer).layer();
    if (!layer)
        return;
    auto* scrollableArea = layer->ensureLayerScrollableArea();

    auto oldScrollType = scrollableArea->currentScrollType();
    scrollableArea->setCurrentScrollType(isUserScroll ? ScrollType::User : ScrollType::Programmatic);
    scrollableArea->scrollToOffsetWithoutAnimation(position);
    scrollableArea->setCurrentScrollType(oldScrollType);

    scrollableArea->didEndScroll(); // FIXME: Should we always call this?
}

void Frame::resetAllGeolocationPermission()
{
    if (document()->domWindow())
        document()->domWindow()->resetAllGeolocationPermission();

    for (Frame* child = tree().firstChild(); child; child = child->tree().nextSibling())
        child->resetAllGeolocationPermission();
}

} // namespace WebCore

#endif // PLATFORM(IOS_FAMILY)
