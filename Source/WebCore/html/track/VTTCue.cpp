/*
 * Copyright (C) 2011, 2013 Google Inc.  All rights reserved.
 * Copyright (C) 2011-2014 Apple Inc. All rights reserved.
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

#include "config.h"

#if ENABLE(VIDEO_TRACK)
#include "VTTCue.h"

#include "CSSPropertyNames.h"
#include "CSSValueKeywords.h"
#include "DocumentFragment.h"
#include "Event.h"
#include "HTMLDivElement.h"
#include "HTMLSpanElement.h"
#include "Logging.h"
#include "NodeTraversal.h"
#include "RenderVTTCue.h"
#include "Text.h"
#include "TextTrack.h"
#include "TextTrackCueList.h"
#include "VTTScanner.h"
#include "WebVTTElement.h"
#include "WebVTTParser.h"
#include <wtf/MathExtras.h>
#include <wtf/text/StringBuilder.h>

#if ENABLE(WEBVTT_REGIONS)
#include "VTTRegionList.h"
#endif

namespace WebCore {

// This constant should correspond with the percentage returned by CaptionUserPreferences::captionFontSizeScaleAndImportance.
const static double DEFAULTCAPTIONFONTSIZEPERCENTAGE = 5;

static const int undefinedPosition = -1;

static const CSSValueID displayWritingModeMap[] = {
    CSSValueHorizontalTb, CSSValueVerticalRl, CSSValueVerticalLr
};
COMPILE_ASSERT(WTF_ARRAY_LENGTH(displayWritingModeMap) == VTTCue::NumberOfWritingDirections, displayWritingModeMap_has_wrong_size);

static const CSSValueID displayAlignmentMap[] = {
    CSSValueStart, CSSValueCenter, CSSValueEnd, CSSValueLeft, CSSValueRight
};
COMPILE_ASSERT(WTF_ARRAY_LENGTH(displayAlignmentMap) == VTTCue::NumberOfAlignments, displayAlignmentMap_has_wrong_size);

static const String& startKeyword()
{
    DEPRECATED_DEFINE_STATIC_LOCAL(const String, start, (ASCIILiteral("start")));
    return start;
}

static const String& middleKeyword()
{
    DEPRECATED_DEFINE_STATIC_LOCAL(const String, middle, (ASCIILiteral("middle")));
    return middle;
}

static const String& endKeyword()
{
    DEPRECATED_DEFINE_STATIC_LOCAL(const String, end, (ASCIILiteral("end")));
    return end;
}

static const String& leftKeyword()
{
    DEPRECATED_DEFINE_STATIC_LOCAL(const String, left, ("left"));
    return left;
}

static const String& rightKeyword()
{
    DEPRECATED_DEFINE_STATIC_LOCAL(const String, right, ("right"));
    return right;
}

static const String& horizontalKeyword()
{
    return emptyString();
}

static const String& verticalGrowingLeftKeyword()
{
    DEPRECATED_DEFINE_STATIC_LOCAL(const String, verticalrl, (ASCIILiteral("rl")));
    return verticalrl;
}

static const String& verticalGrowingRightKeyword()
{
    DEPRECATED_DEFINE_STATIC_LOCAL(const String, verticallr, (ASCIILiteral("lr")));
    return verticallr;
}

// ----------------------------

PassRefPtr<VTTCueBox> VTTCueBox::create(Document& document, VTTCue& cue)
{
    VTTCueBox* cueBox = new VTTCueBox(document, cue);
    cueBox->setPseudo(VTTCueBox::vttCueBoxShadowPseudoId());
    return adoptRef(cueBox);
}

VTTCueBox::VTTCueBox(Document& document, VTTCue& cue)
    : HTMLElement(divTag, document)
    , m_cue(cue)
{
    setPseudo(vttCueBoxShadowPseudoId());
}

VTTCue* VTTCueBox::getCue() const
{
    return &m_cue;
}

void VTTCueBox::applyCSSProperties(const IntSize& videoSize)
{
    // FIXME: Apply all the initial CSS positioning properties. http://wkb.ug/79916
#if ENABLE(WEBVTT_REGIONS)
    if (!m_cue.regionId().isEmpty()) {
        setInlineStyleProperty(CSSPropertyPosition, CSSValueRelative);
        return;
    }
#endif

    // 3.5.1 On the (root) List of WebVTT Node Objects:

    // the 'position' property must be set to 'absolute'
    setInlineStyleProperty(CSSPropertyPosition, CSSValueAbsolute);

    //  the 'unicode-bidi' property must be set to 'plaintext'
    setInlineStyleProperty(CSSPropertyUnicodeBidi, CSSValueWebkitPlaintext);

    // the 'direction' property must be set to direction
    setInlineStyleProperty(CSSPropertyDirection, m_cue.getCSSWritingDirection());

    // the 'writing-mode' property must be set to writing-mode
    setInlineStyleProperty(CSSPropertyWebkitWritingMode, m_cue.getCSSWritingMode(), false);

    std::pair<float, float> position = m_cue.getCSSPosition();

    // the 'top' property must be set to top,
    setInlineStyleProperty(CSSPropertyTop, static_cast<double>(position.second), CSSPrimitiveValue::CSS_PERCENTAGE);

    // the 'left' property must be set to left
    setInlineStyleProperty(CSSPropertyLeft, static_cast<double>(position.first), CSSPrimitiveValue::CSS_PERCENTAGE);

    double authorFontSize = std::min(videoSize.width(), videoSize.height()) * DEFAULTCAPTIONFONTSIZEPERCENTAGE / 100.0;
    double multiplier = 1.0;
    if (authorFontSize)
        multiplier = m_fontSizeFromCaptionUserPrefs / authorFontSize;

    double textPosition = m_cue.position();
    double maxSize = 100.0;
    CSSValueID alignment = m_cue.getCSSAlignment();
    if (alignment == CSSValueEnd || alignment == CSSValueRight)
        maxSize = textPosition;
    else if (alignment == CSSValueStart || alignment == CSSValueLeft)
        maxSize = 100.0 - textPosition;

    double newCueSize = std::min(m_cue.getCSSSize() * multiplier, 100.0);
    // the 'width' property must be set to width, and the 'height' property  must be set to height
    if (m_cue.vertical() == horizontalKeyword()) {
        setInlineStyleProperty(CSSPropertyWidth, newCueSize, CSSPrimitiveValue::CSS_PERCENTAGE);
        setInlineStyleProperty(CSSPropertyHeight, CSSValueAuto);
        setInlineStyleProperty(CSSPropertyMinWidth, "-webkit-min-content");
        setInlineStyleProperty(CSSPropertyMaxWidth, maxSize, CSSPrimitiveValue::CSS_PERCENTAGE);
        if ((alignment == CSSValueMiddle || alignment == CSSValueCenter) && multiplier != 1.0)
            setInlineStyleProperty(CSSPropertyLeft, static_cast<double>(position.first - (newCueSize - m_cue.getCSSSize()) / 2), CSSPrimitiveValue::CSS_PERCENTAGE);
    } else {
        setInlineStyleProperty(CSSPropertyWidth, CSSValueAuto);
        setInlineStyleProperty(CSSPropertyHeight, newCueSize, CSSPrimitiveValue::CSS_PERCENTAGE);
        setInlineStyleProperty(CSSPropertyMinHeight, "-webkit-min-content");
        setInlineStyleProperty(CSSPropertyMaxHeight, maxSize, CSSPrimitiveValue::CSS_PERCENTAGE);
        if ((alignment == CSSValueMiddle || alignment == CSSValueCenter) && multiplier != 1.0)
            setInlineStyleProperty(CSSPropertyTop, static_cast<double>(position.second - (newCueSize - m_cue.getCSSSize()) / 2), CSSPrimitiveValue::CSS_PERCENTAGE);
    }

    // The 'text-align' property on the (root) List of WebVTT Node Objects must
    // be set to the value in the second cell of the row of the table below
    // whose first cell is the value of the corresponding cue's text track cue
    // alignment:
    setInlineStyleProperty(CSSPropertyTextAlign, m_cue.getCSSAlignment());
    
    if (!m_cue.snapToLines()) {
        // 10.13.1 Set up x and y:
        // Note: x and y are set through the CSS left and top above.

        // 10.13.2 Position the boxes in boxes such that the point x% along the
        // width of the bounding box of the boxes in boxes is x% of the way
        // across the width of the video's rendering area, and the point y%
        // along the height of the bounding box of the boxes in boxes is y%
        // of the way across the height of the video's rendering area, while
        // maintaining the relative positions of the boxes in boxes to each
        // other.
        setInlineStyleProperty(CSSPropertyWebkitTransform,
            String::format("translate(-%.2f%%, -%.2f%%)", position.first, position.second));

        setInlineStyleProperty(CSSPropertyWhiteSpace, CSSValuePre);
    }
}

const AtomicString& VTTCueBox::vttCueBoxShadowPseudoId()
{
    DEPRECATED_DEFINE_STATIC_LOCAL(const AtomicString, trackDisplayBoxShadowPseudoId, ("-webkit-media-text-track-display", AtomicString::ConstructFromLiteral));
    return trackDisplayBoxShadowPseudoId;
}

RenderPtr<RenderElement> VTTCueBox::createElementRenderer(PassRef<RenderStyle> style)
{
    return createRenderer<RenderVTTCue>(*this, WTF::move(style));
}

// ----------------------------

const AtomicString& VTTCue::cueBackdropShadowPseudoId()
{
    DEPRECATED_DEFINE_STATIC_LOCAL(const AtomicString, cueBackdropShadowPseudoId, ("-webkit-media-text-track-display-backdrop", AtomicString::ConstructFromLiteral));
    return cueBackdropShadowPseudoId;
}

PassRefPtr<VTTCue> VTTCue::create(ScriptExecutionContext& context, const WebVTTCueData& data)
{
    return adoptRef(new VTTCue(context, data));
}

VTTCue::VTTCue(ScriptExecutionContext& context, const MediaTime& start, const MediaTime& end, const String& content)
    : TextTrackCue(context, start, end)
    , m_content(content)
{
    initialize(context);
}

VTTCue::VTTCue(ScriptExecutionContext& context, const WebVTTCueData& cueData)
    : TextTrackCue(context, MediaTime::zeroTime(), MediaTime::zeroTime())
{
    initialize(context);
    setText(cueData.content());
    setStartTime(cueData.startTime());
    setEndTime(cueData.endTime());
    setId(cueData.id());
    setCueSettings(cueData.settings());
    m_originalStartTime = cueData.originalStartTime();
}

VTTCue::~VTTCue()
{
    if (!hasDisplayTree())
        return;

    displayTreeInternal()->remove(ASSERT_NO_EXCEPTION);
}

void VTTCue::initialize(ScriptExecutionContext& context)
{
    m_linePosition = undefinedPosition;
    m_computedLinePosition = undefinedPosition;
    m_textPosition = 50;
    m_cueSize = 100;
    m_writingDirection = Horizontal;
    m_cueAlignment = Middle;
    m_webVTTNodeTree = nullptr;
    m_cueBackdropBox = HTMLDivElement::create(toDocument(context));
    m_cueHighlightBox = HTMLSpanElement::create(spanTag, toDocument(context));
    m_displayDirection = CSSValueLtr;
    m_displaySize = 0;
    m_snapToLines = true;
    m_displayTreeShouldChange = true;
    m_notifyRegion = true;
    m_originalStartTime = MediaTime::zeroTime();
}

PassRefPtr<VTTCueBox> VTTCue::createDisplayTree()
{
    return VTTCueBox::create(ownerDocument(), *this);
}

VTTCueBox* VTTCue::displayTreeInternal()
{
    if (!m_displayTree)
        m_displayTree = createDisplayTree();
    return m_displayTree.get();
}

void VTTCue::didChange()
{
    TextTrackCue::didChange();
    m_displayTreeShouldChange = true;
}

const String& VTTCue::vertical() const
{
    switch (m_writingDirection) {
    case Horizontal: 
        return horizontalKeyword();
    case VerticalGrowingLeft:
        return verticalGrowingLeftKeyword();
    case VerticalGrowingRight:
        return verticalGrowingRightKeyword();
    default:
        ASSERT_NOT_REACHED();
        return emptyString();
    }
}

void VTTCue::setVertical(const String& value, ExceptionCode& ec)
{
    // http://www.whatwg.org/specs/web-apps/current-work/multipage/the-video-element.html#dom-texttrackcue-vertical
    // On setting, the text track cue writing direction must be set to the value given 
    // in the first cell of the row in the table above whose second cell is a 
    // case-sensitive match for the new value, if any. If none of the values match, then
    // the user agent must instead throw a SyntaxError exception.
    
    WritingDirection direction = m_writingDirection;
    if (value == horizontalKeyword())
        direction = Horizontal;
    else if (value == verticalGrowingLeftKeyword())
        direction = VerticalGrowingLeft;
    else if (value == verticalGrowingRightKeyword())
        direction = VerticalGrowingRight;
    else
        ec = SYNTAX_ERR;
    
    if (direction == m_writingDirection)
        return;

    willChange();
    m_writingDirection = direction;
    didChange();
}

void VTTCue::setSnapToLines(bool value)
{
    if (m_snapToLines == value)
        return;
    
    willChange();
    m_snapToLines = value;
    didChange();
}

void VTTCue::setLine(double position, ExceptionCode& ec)
{
    // http://www.whatwg.org/specs/web-apps/current-work/multipage/the-video-element.html#dom-texttrackcue-line
    // On setting, if the text track cue snap-to-lines flag is not set, and the new
    // value is negative or greater than 100, then throw an IndexSizeError exception.
    if (!m_snapToLines && (position < 0 || position > 100)) {
        ec = INDEX_SIZE_ERR;
        return;
    }

    // Otherwise, set the text track cue line position to the new value.
    if (m_linePosition == position)
        return;

    willChange();
    m_linePosition = position;
    m_computedLinePosition = calculateComputedLinePosition();
    didChange();
}

void VTTCue::setPosition(double position, ExceptionCode& ec)
{
    // http://www.whatwg.org/specs/web-apps/current-work/multipage/the-video-element.html#dom-texttrackcue-position
    // On setting, if the new value is negative or greater than 100, then throw an IndexSizeError exception.
    // Otherwise, set the text track cue text position to the new value.
    if (position < 0 || position > 100) {
        ec = INDEX_SIZE_ERR;
        return;
    }
    
    // Otherwise, set the text track cue line position to the new value.
    if (m_textPosition == position)
        return;
    
    willChange();
    m_textPosition = position;
    didChange();
}

void VTTCue::setSize(int size, ExceptionCode& ec)
{
    // http://www.whatwg.org/specs/web-apps/current-work/multipage/the-video-element.html#dom-texttrackcue-size
    // On setting, if the new value is negative or greater than 100, then throw an IndexSizeError
    // exception. Otherwise, set the text track cue size to the new value.
    if (size < 0 || size > 100) {
        ec = INDEX_SIZE_ERR;
        return;
    }
    
    // Otherwise, set the text track cue line position to the new value.
    if (m_cueSize == size)
        return;
    
    willChange();
    m_cueSize = size;
    didChange();
}

const String& VTTCue::align() const
{
    switch (m_cueAlignment) {
    case Start:
        return startKeyword();
    case Middle:
        return middleKeyword();
    case End:
        return endKeyword();
    case Left:
        return leftKeyword();
    case Right:
        return rightKeyword();
    default:
        ASSERT_NOT_REACHED();
        return emptyString();
    }
}

void VTTCue::setAlign(const String& value, ExceptionCode& ec)
{
    // http://www.whatwg.org/specs/web-apps/current-work/multipage/the-video-element.html#dom-texttrackcue-align
    // On setting, the text track cue alignment must be set to the value given in the 
    // first cell of the row in the table above whose second cell is a case-sensitive
    // match for the new value, if any. If none of the values match, then the user
    // agent must instead throw a SyntaxError exception.
    
    CueAlignment alignment = m_cueAlignment;
    if (value == startKeyword())
        alignment = Start;
    else if (value == middleKeyword())
        alignment = Middle;
    else if (value == endKeyword())
        alignment = End;
    else if (value == leftKeyword())
        alignment = Left;
    else if (value == rightKeyword())
        alignment = Right;
    else
        ec = SYNTAX_ERR;
    
    if (alignment == m_cueAlignment)
        return;

    willChange();
    m_cueAlignment = alignment;
    didChange();
}
    
void VTTCue::setText(const String& text)
{
    if (m_content == text)
        return;
    
    willChange();
    // Clear the document fragment but don't bother to create it again just yet as we can do that
    // when it is requested.
    m_webVTTNodeTree = 0;
    m_content = text;
    didChange();
}

void VTTCue::createWebVTTNodeTree()
{
    if (!m_webVTTNodeTree)
        m_webVTTNodeTree = WebVTTParser::createDocumentFragmentFromCueText(ownerDocument(), m_content);
}

void VTTCue::copyWebVTTNodeToDOMTree(ContainerNode* webVTTNode, ContainerNode* parent)
{
    for (Node* node = webVTTNode->firstChild(); node; node = node->nextSibling()) {
        RefPtr<Node> clonedNode;
        if (node->isWebVTTElement())
            clonedNode = toWebVTTElement(node)->createEquivalentHTMLElement(ownerDocument());
        else
            clonedNode = node->cloneNode(false);
        parent->appendChild(clonedNode, ASSERT_NO_EXCEPTION);
        if (node->isContainerNode())
            copyWebVTTNodeToDOMTree(toContainerNode(node), toContainerNode(clonedNode.get()));
    }
}

PassRefPtr<DocumentFragment> VTTCue::getCueAsHTML()
{
    createWebVTTNodeTree();
    if (!m_webVTTNodeTree)
        return 0;

    RefPtr<DocumentFragment> clonedFragment = DocumentFragment::create(ownerDocument());
    copyWebVTTNodeToDOMTree(m_webVTTNodeTree.get(), clonedFragment.get());
    return clonedFragment.release();
}

PassRefPtr<DocumentFragment> VTTCue::createCueRenderingTree()
{
    RefPtr<DocumentFragment> clonedFragment;
    createWebVTTNodeTree();
    if (!m_webVTTNodeTree)
        return 0;

    clonedFragment = DocumentFragment::create(ownerDocument());
    m_webVTTNodeTree->cloneChildNodes(clonedFragment.get());
    return clonedFragment.release();
}

#if ENABLE(WEBVTT_REGIONS)
void VTTCue::setRegionId(const String& regionId)
{
    if (m_regionId == regionId)
        return;

    willChange();
    m_regionId = regionId;
    didChange();
}

void VTTCue::notifyRegionWhenRemovingDisplayTree(bool notifyRegion)
{
    m_notifyRegion = notifyRegion;
}
#endif

void VTTCue::setIsActive(bool active)
{
    TextTrackCue::setIsActive(active);

    if (!active) {
        if (!hasDisplayTree())
            return;

        // Remove the display tree as soon as the cue becomes inactive.
        removeDisplayTree();
    }
}

int VTTCue::calculateComputedLinePosition()
{
    // http://www.whatwg.org/specs/web-apps/current-work/multipage/the-video-element.html#text-track-cue-computed-line-position

    // If the text track cue line position is numeric, then that is the text
    // track cue computed line position.
    if (m_linePosition != undefinedPosition)
        return m_linePosition;

    // If the text track cue snap-to-lines flag of the text track cue is not
    // set, the text track cue computed line position is the value 100;
    if (!m_snapToLines)
        return 100;

    // Otherwise, it is the value returned by the following algorithm:

    // If cue is not associated with a text track, return -1 and abort these
    // steps.
    if (!track())
        return -1;

    // Let n be the number of text tracks whose text track mode is showing or
    // showing by default and that are in the media element's list of text
    // tracks before track.
    int n = track()->trackIndexRelativeToRenderedTracks();

    // Increment n by one.
    n++;

    // Negate n.
    n = -n;

    return n;
}

static bool isCueParagraphSeparator(UChar character)
{
    // Within a cue, paragraph boundaries are only denoted by Type B characters,
    // such as U+000A LINE FEED (LF), U+0085 NEXT LINE (NEL), and U+2029 PARAGRAPH SEPARATOR.
    return u_charType(character) == U_PARAGRAPH_SEPARATOR;
}

void VTTCue::determineTextDirection()
{
    DEPRECATED_DEFINE_STATIC_LOCAL(const String, rtTag, (ASCIILiteral("rt")));
    createWebVTTNodeTree();
    if (!m_webVTTNodeTree)
        return;

    // Apply the Unicode Bidirectional Algorithm's Paragraph Level steps to the
    // concatenation of the values of each WebVTT Text Object in nodes, in a
    // pre-order, depth-first traversal, excluding WebVTT Ruby Text Objects and
    // their descendants.
    StringBuilder paragraphBuilder;
    for (Node* node = m_webVTTNodeTree->firstChild(); node; node = NodeTraversal::next(node, m_webVTTNodeTree.get())) {
        // FIXME: The code does not match the comment above. This does not actually exclude Ruby Text Object descendant.
        if (!node->isTextNode() || node->localName() == rtTag)
            continue;

        paragraphBuilder.append(node->nodeValue());
    }

    String paragraph = paragraphBuilder.toString();
    if (!paragraph.length())
        return;

    for (size_t i = 0; i < paragraph.length(); ++i) {
        UChar current = paragraph[i];
        if (!current || isCueParagraphSeparator(current))
            return;

        if (UChar current = paragraph[i]) {
            UCharDirection charDirection = u_charDirection(current);
            if (charDirection == U_LEFT_TO_RIGHT) {
                m_displayDirection = CSSValueLtr;
                return;
            }
            if (charDirection == U_RIGHT_TO_LEFT || charDirection == U_RIGHT_TO_LEFT_ARABIC) {
                m_displayDirection = CSSValueRtl;
                return;
            }
        }
    }
}

void VTTCue::calculateDisplayParameters()
{
    // Steps 10.2, 10.3
    determineTextDirection();

    // 10.4 If the text track cue writing direction is horizontal, then let
    // block-flow be 'tb'. Otherwise, if the text track cue writing direction is
    // vertical growing left, then let block-flow be 'lr'. Otherwise, the text
    // track cue writing direction is vertical growing right; let block-flow be
    // 'rl'.

    // The above step is done through the writing direction static map.

    // 10.5 Determine the value of maximum size for cue as per the appropriate
    // rules from the following list:
    int maximumSize = m_textPosition;
    if ((m_writingDirection == Horizontal && m_cueAlignment == Start && m_displayDirection == CSSValueLtr)
        || (m_writingDirection == Horizontal && m_cueAlignment == End && m_displayDirection == CSSValueRtl)
        || (m_writingDirection == Horizontal && m_cueAlignment == Left)
        || (m_writingDirection == VerticalGrowingLeft && (m_cueAlignment == Start || m_cueAlignment == Left))
        || (m_writingDirection == VerticalGrowingRight && (m_cueAlignment == Start || m_cueAlignment == Left))) {
        maximumSize = 100 - m_textPosition;
    } else if ((m_writingDirection == Horizontal && m_cueAlignment == End && m_displayDirection == CSSValueLtr)
        || (m_writingDirection == Horizontal && m_cueAlignment == Start && m_displayDirection == CSSValueRtl)
        || (m_writingDirection == Horizontal && m_cueAlignment == Right)
        || (m_writingDirection == VerticalGrowingLeft && (m_cueAlignment == End || m_cueAlignment == Right))
        || (m_writingDirection == VerticalGrowingRight && (m_cueAlignment == End || m_cueAlignment == Right))) {
        maximumSize = m_textPosition;
    } else if (m_cueAlignment == Middle) {
        maximumSize = m_textPosition <= 50 ? m_textPosition : (100 - m_textPosition);
        maximumSize = maximumSize * 2;
    } else
        ASSERT_NOT_REACHED();

    // 10.6 If the text track cue size is less than maximum size, then let size
    // be text track cue size. Otherwise, let size be maximum size.
    m_displaySize = std::min(m_cueSize, maximumSize);

    // FIXME: Understand why step 10.7 is missing (just a copy/paste error?)
    // Could be done within a spec implementation check - http://crbug.com/301580

    // 10.8 Determine the value of x-position or y-position for cue as per the
    // appropriate rules from the following list:
    if (m_writingDirection == Horizontal) {
        switch (m_cueAlignment) {
        case Start:
            if (m_displayDirection == CSSValueLtr)
                m_displayPosition.first = m_textPosition;
            else
                m_displayPosition.first = 100 - m_textPosition - m_displaySize;
            break;
        case End:
            if (m_displayDirection == CSSValueRtl)
                m_displayPosition.first = 100 - m_textPosition;
            else
                m_displayPosition.first = m_textPosition - m_displaySize;
            break;
        case Left:
            if (m_displayDirection == CSSValueLtr)
                m_displayPosition.first = m_textPosition;
            else
                m_displayPosition.first = 100 - m_textPosition;
            break;
        case Right:
            if (m_displayDirection == CSSValueLtr)
                m_displayPosition.first = m_textPosition - m_displaySize;
            else
                m_displayPosition.first = 100 - m_textPosition - m_displaySize;
            break;
        case Middle:
            if (m_displayDirection == CSSValueLtr)
                m_displayPosition.first = m_textPosition - m_displaySize / 2;
            else
                m_displayPosition.first = 100 - m_textPosition - m_displaySize / 2;
            break;
        case NumberOfAlignments:
            ASSERT_NOT_REACHED();
        }
    }

    // A text track cue has a text track cue computed line position whose value
    // is defined in terms of the other aspects of the cue.
    m_computedLinePosition = calculateComputedLinePosition();

    // 10.9 Determine the value of whichever of x-position or y-position is not
    // yet calculated for cue as per the appropriate rules from the following
    // list:
    if (m_snapToLines && m_displayPosition.second == undefinedPosition && m_writingDirection == Horizontal)
        m_displayPosition.second = 0;

    if (!m_snapToLines && m_displayPosition.second == undefinedPosition && m_writingDirection == Horizontal)
        m_displayPosition.second = m_computedLinePosition;

    if (m_snapToLines && m_displayPosition.first == undefinedPosition
        && (m_writingDirection == VerticalGrowingLeft || m_writingDirection == VerticalGrowingRight))
        m_displayPosition.first = 0;

    if (!m_snapToLines && (m_writingDirection == VerticalGrowingLeft || m_writingDirection == VerticalGrowingRight))
        m_displayPosition.first = m_computedLinePosition;
}
    
void VTTCue::markFutureAndPastNodes(ContainerNode* root, const MediaTime& previousTimestamp, const MediaTime& movieTime)
{
    DEPRECATED_DEFINE_STATIC_LOCAL(const String, timestampTag, (ASCIILiteral("timestamp")));
    
    bool isPastNode = true;
    MediaTime currentTimestamp = previousTimestamp;
    if (currentTimestamp > movieTime)
        isPastNode = false;
    
    for (Node* child = root->firstChild(); child; child = NodeTraversal::next(child, root)) {
        if (child->nodeName() == timestampTag) {
            MediaTime currentTimestamp;
            bool check = WebVTTParser::collectTimeStamp(child->nodeValue(), currentTimestamp);
            ASSERT_UNUSED(check, check);
            
            currentTimestamp += m_originalStartTime;
            if (currentTimestamp > movieTime)
                isPastNode = false;
        }
        
        if (child->isWebVTTElement()) {
            toWebVTTElement(child)->setIsPastNode(isPastNode);
            // Make an elemenet id match a cue id for style matching purposes.
            if (!id().isEmpty())
                toElement(child)->setIdAttribute(id());
        }
    }
}

void VTTCue::updateDisplayTree(const MediaTime& movieTime)
{
    // The display tree may contain WebVTT timestamp objects representing
    // timestamps (processing instructions), along with displayable nodes.

    if (!track()->isRendered())
        return;

    // Clear the contents of the set.
    m_cueHighlightBox->removeChildren();

    // Update the two sets containing past and future WebVTT objects.
    RefPtr<DocumentFragment> referenceTree = createCueRenderingTree();
    if (!referenceTree)
        return;

    markFutureAndPastNodes(referenceTree.get(), startMediaTime(), movieTime);
    m_cueHighlightBox->appendChild(referenceTree);
}

VTTCueBox* VTTCue::getDisplayTree(const IntSize& videoSize, int fontSize)
{
    RefPtr<VTTCueBox> displayTree = displayTreeInternal();
    if (!m_displayTreeShouldChange || !track()->isRendered())
        return displayTree.get();

    // 10.1 - 10.10
    calculateDisplayParameters();

    // 10.11. Apply the terms of the CSS specifications to nodes within the
    // following constraints, thus obtaining a set of CSS boxes positioned
    // relative to an initial containing block:
    displayTree->removeChildren();

    // The document tree is the tree of WebVTT Node Objects rooted at nodes.

    // The children of the nodes must be wrapped in an anonymous box whose
    // 'display' property has the value 'inline'. This is the WebVTT cue
    // background box.

    // Note: This is contained by default in m_cueHighlightBox.
    m_cueHighlightBox->setPseudo(cueShadowPseudoId());

    m_cueBackdropBox->setPseudo(cueBackdropShadowPseudoId());
    m_cueBackdropBox->appendChild(m_cueHighlightBox, ASSERT_NO_EXCEPTION);
    displayTree->appendChild(m_cueBackdropBox, ASSERT_NO_EXCEPTION);

    // FIXME(BUG 79916): Runs of children of WebVTT Ruby Objects that are not
    // WebVTT Ruby Text Objects must be wrapped in anonymous boxes whose
    // 'display' property has the value 'ruby-base'.

    displayTree->setFontSizeFromCaptionUserPrefs(fontSize);
    displayTree->applyCSSProperties(videoSize);

    m_displayTreeShouldChange = false;

    // 10.15. Let cue's text track cue display state have the CSS boxes in
    // boxes.
    return displayTree.get();
}

void VTTCue::removeDisplayTree()
{
#if ENABLE(WEBVTT_REGIONS)
    // The region needs to be informed about the cue removal.
    if (m_notifyRegion && track()) {
        if (VTTRegionList* regions = track()->regions()) {
            if (VTTRegion* region = regions->getRegionById(m_regionId))
                region->willRemoveTextTrackCueBox(m_displayTree.get());
        }
    }
#endif

    if (!hasDisplayTree())
        return;
    displayTreeInternal()->remove(ASSERT_NO_EXCEPTION);
}

std::pair<double, double> VTTCue::getPositionCoordinates() const
{
    // This method is used for setting x and y when snap to lines is not set.
    std::pair<double, double> coordinates;

    if (m_writingDirection == Horizontal && m_displayDirection == CSSValueLtr) {
        coordinates.first = m_textPosition;
        coordinates.second = m_computedLinePosition;

        return coordinates;
    }

    if (m_writingDirection == Horizontal && m_displayDirection == CSSValueRtl) {
        coordinates.first = 100 - m_textPosition;
        coordinates.second = m_computedLinePosition;

        return coordinates;
    }

    if (m_writingDirection == VerticalGrowingLeft) {
        coordinates.first = 100 - m_computedLinePosition;
        coordinates.second = m_textPosition;

        return coordinates;
    }

    if (m_writingDirection == VerticalGrowingRight) {
        coordinates.first = m_computedLinePosition;
        coordinates.second = m_textPosition;

        return coordinates;
    }

    ASSERT_NOT_REACHED();

    return coordinates;
}

VTTCue::CueSetting VTTCue::settingName(VTTScanner& input)
{
    CueSetting parsedSetting = None;
    if (input.scan("vertical"))
        parsedSetting = Vertical;
    else if (input.scan("line"))
        parsedSetting = Line;
    else if (input.scan("position"))
        parsedSetting = Position;
    else if (input.scan("size"))
        parsedSetting = Size;
    else if (input.scan("align"))
        parsedSetting = Align;
#if ENABLE(WEBVTT_REGIONS)
    else if (input.scan("region"))
        parsedSetting = RegionId;
#endif
    // Verify that a ':' follows.
    if (parsedSetting != None && input.scan(':'))
        return parsedSetting;

    return None;
}

void VTTCue::setCueSettings(const String& inputString)
{
    if (inputString.isEmpty())
        return;

    VTTScanner input(inputString);

    while (!input.isAtEnd()) {

        // The WebVTT cue settings part of a WebVTT cue consists of zero or more of the following components, in any order, 
        // separated from each other by one or more U+0020 SPACE characters or U+0009 CHARACTER TABULATION (tab) characters.
        input.skipWhile<WebVTTParser::isValidSettingDelimiter>();
        if (input.isAtEnd())
            break;

        // When the user agent is to parse the WebVTT settings given by a string input for a text track cue cue, 
        // the user agent must run the following steps:
        // 1. Let settings be the result of splitting input on spaces.
        // 2. For each token setting in the list settings, run the following substeps:
        //    1. If setting does not contain a U+003A COLON character (:), or if the first U+003A COLON character (:) 
        //       in setting is either the first or last character of setting, then jump to the step labeled next setting.
        //    2. Let name be the leading substring of setting up to and excluding the first U+003A COLON character (:) in that string.
        CueSetting name = settingName(input);

        // 3. Let value be the trailing substring of setting starting from the character immediately after the first U+003A COLON character (:) in that string.
        VTTScanner::Run valueRun = input.collectUntil<WebVTTParser::isValidSettingDelimiter>();

        // 4. Run the appropriate substeps that apply for the value of name, as follows:
        switch (name) {
        case Vertical: {
            // If name is a case-sensitive match for "vertical"
            // 1. If value is a case-sensitive match for the string "rl", then let cue's text track cue writing direction 
            //    be vertical growing left.
            if (input.scanRun(valueRun, verticalGrowingLeftKeyword()))
                m_writingDirection = VerticalGrowingLeft;
            
            // 2. Otherwise, if value is a case-sensitive match for the string "lr", then let cue's text track cue writing 
            //    direction be vertical growing right.
            else if (input.scanRun(valueRun, verticalGrowingRightKeyword()))
                m_writingDirection = VerticalGrowingRight;

            else
                LOG(Media, "VTTCue::setCueSettings, invalid Vertical");
            break;
        }
        case Line: {
            bool isValid = false;
            do {
                // 1-2 - Collect chars that are either '-', '%', or a digit.
                // 1. If value contains any characters other than U+002D HYPHEN-MINUS characters (-), U+0025 PERCENT SIGN
                //    characters (%), and characters in the range U+0030 DIGIT ZERO (0) to U+0039 DIGIT NINE (9), then jump
                //    to the step labeled next setting.
                float linePosition;
                bool isNegative;
                if (!input.scanFloat(linePosition, &isNegative))
                    break;

                bool isPercentage = input.scan('%');
                if (!input.isAt(valueRun.end()))
                    break;

                // 2. If value does not contain at least one character in the range U+0030 DIGIT ZERO (0) to U+0039 DIGIT
                //    NINE (9), then jump to the step labeled next setting.
                // 3. If any character in value other than the first character is a U+002D HYPHEN-MINUS character (-), then
                //    jump to the step labeled next setting.
                // 4. If any character in value other than the last character is a U+0025 PERCENT SIGN character (%), then
                //    jump to the step labeled next setting.
                // 5. If the first character in value is a U+002D HYPHEN-MINUS character (-) and the last character in value is a
                //    U+0025 PERCENT SIGN character (%), then jump to the step labeled next setting.
                if (isPercentage && isNegative)
                    break;

                // 6. Ignoring the trailing percent sign, if any, interpret value as a (potentially signed) integer, and
                //    let number be that number.
                // 7. If the last character in value is a U+0025 PERCENT SIGN character (%), but number is not in the range
                //    0 ≤ number ≤ 100, then jump to the step labeled next setting.
                // 8. Let cue's text track cue line position be number.
                // 9. If the last character in value is a U+0025 PERCENT SIGN character (%), then let cue's text track cue
                //    snap-to-lines flag be false. Otherwise, let it be true.
                if (isPercentage) {
                    if (linePosition < 0 || linePosition > 100)
                        break;

                    // 10 - If '%' then set snap-to-lines flag to false.
                    m_snapToLines = false;
                } else {
                    if (linePosition - static_cast<int>(linePosition))
                        break;

                    m_snapToLines = true;
                }
                
                m_linePosition = linePosition;
                isValid = true;
            } while (0);

            if (!isValid)
                LOG(Media, "VTTCue::setCueSettings, invalid Line");

            break;
        }
        case Position: {
            float position;
            if (WebVTTParser::parseFloatPercentageValue(input, position) && input.isAt(valueRun.end()))
                m_textPosition = position;
            else
                LOG(Media, "VTTCue::setCueSettings, invalid Position");
            break;
        }
        case Size: {
            float cueSize;
            if (WebVTTParser::parseFloatPercentageValue(input, cueSize) && input.isAt(valueRun.end()))
                m_cueSize = cueSize;
            else
                LOG(Media, "VTTCue::setCueSettings, invalid Size");
            break;
        }
        case Align: {
            // 1. If value is a case-sensitive match for the string "start", then let cue's text track cue alignment be start alignment.
            if (input.scanRun(valueRun, startKeyword()))
                m_cueAlignment = Start;

            // 2. If value is a case-sensitive match for the string "middle", then let cue's text track cue alignment be middle alignment.
            else if (input.scanRun(valueRun, middleKeyword()))
                m_cueAlignment = Middle;

            // 3. If value is a case-sensitive match for the string "end", then let cue's text track cue alignment be end alignment.
            else if (input.scanRun(valueRun, endKeyword()))
                m_cueAlignment = End;

            // 4. If value is a case-sensitive match for the string "left", then let cue's text track cue alignment be left alignment.
            else if (input.scanRun(valueRun, leftKeyword()))
                m_cueAlignment = Left;

            // 5. If value is a case-sensitive match for the string "right", then let cue's text track cue alignment be right alignment.
            else if (input.scanRun(valueRun, rightKeyword()))
                m_cueAlignment = Right;

            else
                LOG(Media, "VTTCue::setCueSettings, invalid Align");

            break;
        }
#if ENABLE(WEBVTT_REGIONS)
        case RegionId:
            m_regionId = input.extractString(valueRun);
            break;
#endif
        case None:
            break;
        }

        // Make sure the entire run is consumed.
        input.skipRun(valueRun);
    }
#if ENABLE(WEBVTT_REGIONS)
    // If cue's line position is not auto or cue's size is not 100 or cue's
    // writing direction is not horizontal, but cue's region identifier is not
    // the empty string, let cue's region identifier be the empty string.
    if (m_regionId.isEmpty())
        return;

    if (m_linePosition != undefinedPosition || m_cueSize != 100 || m_writingDirection != Horizontal)
        m_regionId = emptyString();
#endif
}

CSSValueID VTTCue::getCSSAlignment() const
{
    return displayAlignmentMap[m_cueAlignment];
}

CSSValueID VTTCue::getCSSWritingDirection() const
{
    return m_displayDirection;
}

CSSValueID VTTCue::getCSSWritingMode() const
{
    return displayWritingModeMap[m_writingDirection];
}

int VTTCue::getCSSSize() const
{
    return m_displaySize;
}

std::pair<double, double> VTTCue::getCSSPosition() const
{
    if (!m_snapToLines)
        return getPositionCoordinates();

    return m_displayPosition;
}

bool VTTCue::cueContentsMatch(const TextTrackCue& cue) const
{
    const VTTCue* vttCue = toVTTCue(&cue);
    if (text() != vttCue->text())
        return false;
    if (cueSettings() != vttCue->cueSettings())
        return false;
    if (position() != vttCue->position())
        return false;
    if (line() != vttCue->line())
        return false;
    if (size() != vttCue->size())
        return false;
    if (align() != vttCue->align())
        return false;
    
    return true;
}

bool VTTCue::isEqual(const TextTrackCue& cue, TextTrackCue::CueMatchRules match) const
{
    if (!TextTrackCue::isEqual(cue, match))
        return false;

    if (cue.cueType() != WebVTT)
        return false;

    return cueContentsMatch(cue);
}

bool VTTCue::doesExtendCue(const TextTrackCue& cue) const
{
    if (!cueContentsMatch(cue))
        return false;
    
    return TextTrackCue::doesExtendCue(cue);
}
    
void VTTCue::setFontSize(int fontSize, const IntSize&, bool important)
{
    if (!hasDisplayTree() || !fontSize)
        return;
    
    LOG(Media, "TextTrackCue::setFontSize - setting cue font size to %i", fontSize);

    m_displayTreeShouldChange = true;
    displayTreeInternal()->setInlineStyleProperty(CSSPropertyFontSize, fontSize, CSSPrimitiveValue::CSS_PX, important);
}

VTTCue* toVTTCue(TextTrackCue* cue)
{
    return const_cast<VTTCue*>(toVTTCue(const_cast<const TextTrackCue*>(cue)));
}

const VTTCue* toVTTCue(const TextTrackCue* cue)
{
    ASSERT_WITH_SECURITY_IMPLICATION(cue->isRenderable());
    return static_cast<const VTTCue*>(cue);
}

} // namespace WebCore

#endif
