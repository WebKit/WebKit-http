/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
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

#include "TouchDisambiguation.h"

#include "Document.h"
#include "Element.h"
#include "Frame.h"
#include "FrameView.h"
#include "HTMLNames.h"
#include "HitTestResult.h"
#include <algorithm>
#include <cmath>

using namespace std;

namespace WebCore {

static IntRect boundingBoxForEventNodes(Node* eventNode)
{
    if (!eventNode->document()->view())
        return IntRect();

    IntRect result;
    Node* node = eventNode;
    while (node) {
        // Skip the whole sub-tree if the node doesn't propagate events.
        if (node != eventNode && node->willRespondToMouseClickEvents()) {
            node = node->traverseNextSibling(eventNode);
            continue;
        }
        result.unite(node->pixelSnappedBoundingBox());
        node = node->traverseNextNode(eventNode);
    }
    return eventNode->document()->view()->contentsToWindow(result);
}

static float scoreTouchTarget(IntPoint touchPoint, int padding, IntRect boundingBox)
{
    if (boundingBox.isEmpty())
        return 0;

    float reciprocalPadding = 1.f / padding;
    float score = 1;

    IntSize distance = boundingBox.differenceToPoint(touchPoint);
    score *= max((padding - abs(distance.width())) * reciprocalPadding, 0.f);
    score *= max((padding - abs(distance.height())) * reciprocalPadding, 0.f);

    return score;
}

struct TouchTargetData {
    IntRect windowBoundingBox;
    float score;
};

void findGoodTouchTargets(const IntRect& touchBox, Frame* mainFrame, float pageScaleFactor, Vector<IntRect>& goodTargets)
{
    goodTargets.clear();

    int touchPointPadding = ceil(max(touchBox.width(), touchBox.height()) * 0.5);
    // FIXME: Rect-based hit test doesn't transform the touch point size.
    //        We have to pre-apply page scale factor here.
    int padding = ceil(touchPointPadding / pageScaleFactor);

    IntPoint touchPoint = touchBox.center();
    IntPoint contentsPoint = mainFrame->view()->windowToContents(touchPoint);

    HitTestResult result = mainFrame->eventHandler()->hitTestResultAtPoint(contentsPoint, false, false, DontHitTestScrollbars, HitTestRequest::Active | HitTestRequest::ReadOnly, IntSize(padding, padding));
    const ListHashSet<RefPtr<Node> >& hitResults = result.rectBasedTestResult();

    HashMap<Node*, TouchTargetData> touchTargets;
    float bestScore = 0;
    for (ListHashSet<RefPtr<Node> >::const_iterator it = hitResults.begin(); it != hitResults.end(); ++it) {
        for (Node* node = it->get(); node; node = node->parentNode()) {
            if (node->isDocumentNode() || node->hasTagName(HTMLNames::htmlTag) || node->hasTagName(HTMLNames::bodyTag))
                break;
            if (node->willRespondToMouseClickEvents()) {
                TouchTargetData& targetData = touchTargets.add(node, TouchTargetData()).iterator->second;
                targetData.windowBoundingBox = boundingBoxForEventNodes(node);
                targetData.score = scoreTouchTarget(touchPoint, touchPointPadding, targetData.windowBoundingBox);
                bestScore = max(bestScore, targetData.score);
                break;
            }
        }
    }

    for (HashMap<Node*, TouchTargetData>::iterator it = touchTargets.begin(); it != touchTargets.end(); ++it) {
        // Currently the scoring function uses the overlap area with the fat point as the score.
        // We ignore the candidates that has less than 1/2 overlap (we consider not really ambiguous enough) than the best candidate to avoid excessive popups.
        if (it->second.score < bestScore * 0.5)
            continue;
        goodTargets.append(it->second.windowBoundingBox);
    }
}

} // namespace WebCore
