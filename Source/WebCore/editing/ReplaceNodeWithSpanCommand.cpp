/*
 * Copyright (c) 2009 Google Inc. All rights reserved.
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
#include "ReplaceNodeWithSpanCommand.h"

#include "htmlediting.h"
#include "HTMLElement.h"
#include "HTMLNames.h"

#include <wtf/Assertions.h>

namespace WebCore {

using namespace HTMLNames;

ReplaceNodeWithSpanCommand::ReplaceNodeWithSpanCommand(PassRefPtr<HTMLElement> element)
    : SimpleEditCommand(element->document())
    , m_elementToReplace(element)
{
    ASSERT(m_elementToReplace);
}

static void swapInNodePreservingAttributesAndChildren(HTMLElement* newNode, HTMLElement* nodeToReplace)
{
    ASSERT(nodeToReplace->inDocument());
    ExceptionCode ec = 0;
    RefPtr<ContainerNode> parentNode = nodeToReplace->parentNode();
    parentNode->insertBefore(newNode, nodeToReplace, ec);
    ASSERT(!ec);

    NodeVector children;
    getChildNodes(nodeToReplace, children);
    for (size_t i = 0; i < children.size(); ++i) {
        newNode->appendChild(children[i], ec);
        ASSERT(!ec);
    }

    // FIXME: Fix this to send the proper MutationRecords when MutationObservers are present.
    newNode->cloneDataFromElement(*nodeToReplace);

    parentNode->removeChild(nodeToReplace, ec);
    ASSERT(!ec);
}

void ReplaceNodeWithSpanCommand::doApply()
{
    if (!m_elementToReplace->inDocument())
        return;
    if (!m_spanElement)
        m_spanElement = createHTMLElement(m_elementToReplace->document(), spanTag);
    swapInNodePreservingAttributesAndChildren(m_spanElement.get(), m_elementToReplace.get());
}

void ReplaceNodeWithSpanCommand::doUnapply()
{
    if (!m_spanElement->inDocument())
        return;
    swapInNodePreservingAttributesAndChildren(m_elementToReplace.get(), m_spanElement.get());
}

#ifndef NDEBUG
void ReplaceNodeWithSpanCommand::getNodesInCommand(HashSet<Node*>& nodes)
{
    addNodeAndDescendants(m_elementToReplace.get(), nodes);
    addNodeAndDescendants(m_spanElement.get(), nodes);
}
#endif

} // namespace WebCore
