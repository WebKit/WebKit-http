/*
 * Copyright (C) 2005, 2006, 2008 Apple Inc. All rights reserved.
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

#ifndef InsertParagraphSeparatorCommand_h
#define InsertParagraphSeparatorCommand_h

#include "CompositeEditCommand.h"

namespace WebCore {

class EditingStyle;

class InsertParagraphSeparatorCommand : public CompositeEditCommand {
public:
    static PassRefPtr<InsertParagraphSeparatorCommand> create(Document* document, bool useDefaultParagraphElement = false, bool pasteBlockqutoeIntoUnquotedArea = false)
    {
        return adoptRef(new InsertParagraphSeparatorCommand(document, useDefaultParagraphElement, pasteBlockqutoeIntoUnquotedArea));
    }

private:
    InsertParagraphSeparatorCommand(Document*, bool useDefaultParagraphElement, bool pasteBlockqutoeIntoUnquotedArea);

    virtual void doApply();

    void calculateStyleBeforeInsertion(const Position&);
    void applyStyleAfterInsertion(Node* originalEnclosingBlock);
    void getAncestorsInsideBlock(const Node* insertionNode, Element* outerBlock, Vector<Element*>& ancestors);
    PassRefPtr<Element> cloneHierarchyUnderNewBlock(const Vector<Element*>& ancestors, PassRefPtr<Element> blockToInsert);

    bool shouldUseDefaultParagraphElement(Node*) const;

    virtual bool preservesTypingStyle() const;

    RefPtr<EditingStyle> m_style;

    bool m_mustUseDefaultParagraphElement;
    bool m_pasteBlockqutoeIntoUnquotedArea;
};

}

#endif
