/*
 * Copyright (C) 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2006 Alexey Proskuryakov (ap@nypop.com)
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#import "config.h"
#import "Frame.h"

#import "BlockExceptions.h"
#import "ColorMac.h"
#import "Cursor.h"
#import "DOMInternal.h"
#import "Event.h"
#import "FrameLoaderClient.h"
#import "FrameView.h"
#import "GraphicsContext.h"
#import "HTMLNames.h"
#import "HTMLTableCellElement.h"
#import "HitTestRequest.h"
#import "HitTestResult.h"
#import "KeyboardEvent.h"
#import "Logging.h"
#import "MouseEventWithHitTestResults.h"
#import "Page.h"
#import "PlatformKeyboardEvent.h"
#import "PlatformWheelEvent.h"
#import "RegularExpression.h"
#import "RenderTableCell.h"
#import "Scrollbar.h"
#import "SimpleFontData.h"
#import "visible_units.h"
#import <wtf/StdLibExtras.h>

@interface NSView (WebCoreHTMLDocumentView)
- (void)drawSingleRect:(NSRect)rect;
@end
 
using namespace std;

namespace WebCore {

using namespace HTMLNames;

// Either get cached regexp or build one that matches any of the labels.
// The regexp we build is of the form:  (STR1|STR2|STRN)
static RegularExpression* regExpForLabels(NSArray* labels)
{
    // All the ObjC calls in this method are simple array and string
    // calls which we can assume do not raise exceptions


    // Parallel arrays that we use to cache regExps.  In practice the number of expressions
    // that the app will use is equal to the number of locales is used in searching.
    static const unsigned int regExpCacheSize = 4;
    static NSMutableArray* regExpLabels = nil;
    DEFINE_STATIC_LOCAL(Vector<RegularExpression*>, regExps, ());
    DEFINE_STATIC_LOCAL(RegularExpression, wordRegExp, ("\\w", TextCaseSensitive));

    RegularExpression* result;
    if (!regExpLabels)
        regExpLabels = [[NSMutableArray alloc] initWithCapacity:regExpCacheSize];
    CFIndex cacheHit = [regExpLabels indexOfObject:labels];
    if (cacheHit != NSNotFound)
        result = regExps.at(cacheHit);
    else {
        String pattern("(");
        unsigned int numLabels = [labels count];
        unsigned int i;
        for (i = 0; i < numLabels; i++) {
            String label = [labels objectAtIndex:i];

            bool startsWithWordChar = false;
            bool endsWithWordChar = false;
            if (label.length() != 0) {
                startsWithWordChar = wordRegExp.match(label.substring(0, 1)) >= 0;
                endsWithWordChar = wordRegExp.match(label.substring(label.length() - 1, 1)) >= 0;
            }
            
            if (i != 0)
                pattern.append("|");
            // Search for word boundaries only if label starts/ends with "word characters".
            // If we always searched for word boundaries, this wouldn't work for languages
            // such as Japanese.
            if (startsWithWordChar)
                pattern.append("\\b");
            pattern.append(label);
            if (endsWithWordChar)
                pattern.append("\\b");
        }
        pattern.append(")");
        result = new RegularExpression(pattern, TextCaseInsensitive);
    }

    // add regexp to the cache, making sure it is at the front for LRU ordering
    if (cacheHit != 0) {
        if (cacheHit != NSNotFound) {
            // remove from old spot
            [regExpLabels removeObjectAtIndex:cacheHit];
            regExps.remove(cacheHit);
        }
        // add to start
        [regExpLabels insertObject:labels atIndex:0];
        regExps.insert(0, result);
        // trim if too big
        if ([regExpLabels count] > regExpCacheSize) {
            [regExpLabels removeObjectAtIndex:regExpCacheSize];
            RegularExpression* last = regExps.last();
            regExps.removeLast();
            delete last;
        }
    }
    return result;
}

NSString* Frame::searchForLabelsBeforeElement(NSArray* labels, Element* element, size_t* resultDistance, bool* resultIsInCellAbove)
{
    RegularExpression* regExp = regExpForLabels(labels);
    // We stop searching after we've seen this many chars
    const unsigned int charsSearchedThreshold = 500;
    // This is the absolute max we search.  We allow a little more slop than
    // charsSearchedThreshold, to make it more likely that we'll search whole nodes.
    const unsigned int maxCharsSearched = 600;
    // If the starting element is within a table, the cell that contains it
    HTMLTableCellElement* startingTableCell = 0;
    bool searchedCellAbove = false;
    
    if (resultDistance)
        *resultDistance = notFound;
    if (resultIsInCellAbove)
        *resultIsInCellAbove = false;

    // walk backwards in the node tree, until another element, or form, or end of tree
    unsigned lengthSearched = 0;
    Node* n;
    for (n = element->traversePreviousNode();
         n && lengthSearched < charsSearchedThreshold;
         n = n->traversePreviousNode())
    {
        if (n->hasTagName(formTag)
            || (n->isHTMLElement() && static_cast<Element*>(n)->isFormControlElement()))
        {
            // We hit another form element or the start of the form - bail out
            break;
        } else if (n->hasTagName(tdTag) && !startingTableCell) {
            startingTableCell = static_cast<HTMLTableCellElement*>(n);
        } else if (n->hasTagName(trTag) && startingTableCell) {
            NSString* result = searchForLabelsAboveCell(regExp, startingTableCell, resultDistance);
            if (result && [result length] > 0) {
                if (resultIsInCellAbove)
                    *resultIsInCellAbove = true;
                return result;
            }
            searchedCellAbove = true;
        } else if (n->isTextNode() && n->renderer() && n->renderer()->style()->visibility() == VISIBLE) {
            // For each text chunk, run the regexp
            String nodeString = n->nodeValue();
            // add 100 for slop, to make it more likely that we'll search whole nodes
            if (lengthSearched + nodeString.length() > maxCharsSearched)
                nodeString = nodeString.right(charsSearchedThreshold - lengthSearched);
            int pos = regExp->searchRev(nodeString);
            if (pos >= 0) {
                if (resultDistance)
                    *resultDistance = lengthSearched;
                return nodeString.substring(pos, regExp->matchedLength());
            }
            lengthSearched += nodeString.length();
        }
    }

    // If we started in a cell, but bailed because we found the start of the form or the
    // previous element, we still might need to search the row above us for a label.
    if (startingTableCell && !searchedCellAbove) {
        NSString* result = searchForLabelsAboveCell(regExp, startingTableCell, resultDistance);
        if (result && [result length] > 0) {
            if (resultIsInCellAbove)
                *resultIsInCellAbove = true;
            return result;
        }
    }
    
    return nil;
}

static NSString *matchLabelsAgainstString(NSArray *labels, const String& stringToMatch)
{
    if (stringToMatch.isEmpty())
        return nil;
    
    String mutableStringToMatch = stringToMatch;
    
    // Make numbers and _'s in field names behave like word boundaries, e.g., "address2"
    replace(mutableStringToMatch, RegularExpression("\\d", TextCaseSensitive), " ");
    mutableStringToMatch.replace('_', ' ');
    
    RegularExpression* regExp = regExpForLabels(labels);
    // Use the largest match we can find in the whole string
    int pos;
    int length;
    int bestPos = -1;
    int bestLength = -1;
    int start = 0;
    do {
        pos = regExp->match(mutableStringToMatch, start);
        if (pos != -1) {
            length = regExp->matchedLength();
            if (length >= bestLength) {
                bestPos = pos;
                bestLength = length;
            }
            start = pos + 1;
        }
    } while (pos != -1);
    
    if (bestPos != -1)
        return mutableStringToMatch.substring(bestPos, bestLength);
    return nil;
}

NSString* Frame::matchLabelsAgainstElement(NSArray* labels, Element* element)
{
    // Match against the name element, then against the id element if no match is found for the name element.
    // See 7538330 for one popular site that benefits from the id element check.
    // FIXME: This code is mirrored in Frame.cpp. It would be nice to make the Mac code call the platform-agnostic
    // code, which would require converting the NSArray of NSStrings to a Vector of Strings somewhere along the way.
    String resultFromNameAttribute = matchLabelsAgainstString(labels, element->getAttribute(nameAttr));
    if (!resultFromNameAttribute.isEmpty())
        return resultFromNameAttribute;
    
    return matchLabelsAgainstString(labels, element->getAttribute(idAttr));
}

NSImage* Frame::imageFromRect(NSRect rect) const
{
    PaintBehavior oldBehavior = m_view->paintBehavior();
    m_view->setPaintBehavior(oldBehavior | PaintBehaviorFlattenCompositingLayers);
    
    BEGIN_BLOCK_OBJC_EXCEPTIONS;
    
    NSImage* resultImage = [[[NSImage alloc] initWithSize:rect.size] autorelease];
    
    if (rect.size.width != 0 && rect.size.height != 0) {
        [resultImage setFlipped:YES];
        [resultImage lockFocus];

        GraphicsContext graphicsContext((CGContextRef)[[NSGraphicsContext currentContext] graphicsPort]);        
        graphicsContext.save();
        graphicsContext.translate(-rect.origin.x, -rect.origin.y);
        m_view->paintContents(&graphicsContext, IntRect(rect));
        graphicsContext.restore();

        [resultImage unlockFocus];
        [resultImage setFlipped:NO];
    }
    
    m_view->setPaintBehavior(oldBehavior);
    return resultImage;
    
    END_BLOCK_OBJC_EXCEPTIONS;
    
    m_view->setPaintBehavior(oldBehavior);
    return nil;
}

NSImage* Frame::selectionImage(bool forceBlackText) const
{
    m_view->setPaintBehavior(PaintBehaviorSelectionOnly | (forceBlackText ? PaintBehaviorForceBlackText : 0));
    m_doc->updateLayout();
    NSImage* result = imageFromRect(selection()->bounds());
    m_view->setPaintBehavior(PaintBehaviorNormal);
    return result;
}

NSImage* Frame::snapshotDragImage(Node* node, NSRect* imageRect, NSRect* elementRect) const
{
    RenderObject* renderer = node->renderer();
    if (!renderer)
        return nil;
    
    renderer->updateDragState(true);    // mark dragged nodes (so they pick up the right CSS)
    m_doc->updateLayout();        // forces style recalc - needed since changing the drag state might
                                        // imply new styles, plus JS could have changed other things
    LayoutRect topLevelRect;
    NSRect paintingRect = renderer->paintingRootRect(topLevelRect);

    m_view->setNodeToDraw(node);              // invoke special sub-tree drawing mode
    NSImage* result = imageFromRect(paintingRect);
    renderer->updateDragState(false);
    m_doc->updateLayout();
    m_view->setNodeToDraw(0);

    if (elementRect)
        *elementRect = topLevelRect;
    if (imageRect)
        *imageRect = paintingRect;
    return result;
}

DragImageRef Frame::nodeImage(Node* node)
{
    RenderObject* renderer = node->renderer();
    if (!renderer)
        return nil;

    m_doc->updateLayout(); // forces style recalc

    LayoutRect topLevelRect;
    NSRect paintingRect = renderer->paintingRootRect(topLevelRect);

    m_view->setNodeToDraw(node); // invoke special sub-tree drawing mode
    NSImage* result = imageFromRect(paintingRect);
    m_view->setNodeToDraw(0);

    return result;
}

DragImageRef Frame::dragImageForSelection()
{
    if (!selection()->isRange())
        return nil;
    return selectionImage();
}

} // namespace WebCore
