/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2009 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef DocumentFragment_h
#define DocumentFragment_h

#include "ContainerNode.h"
#include "FragmentScriptingPermission.h"

namespace WebCore {

class DocumentFragment : public ContainerNode {
public:
    static PassRefPtr<DocumentFragment> create(Document&);

    void parseHTML(const String&, Element* contextElement, ParserContentPolicy = AllowScriptingContent);
    bool parseXML(const String&, Element* contextElement, ParserContentPolicy = AllowScriptingContent);
    
    virtual bool canContainRangeEndPoint() const OVERRIDE { return true; }
    virtual bool isTemplateContent() const { return false; }

protected:
    DocumentFragment(Document*, ConstructionType = CreateContainer);
    virtual String nodeName() const OVERRIDE;

private:
    virtual NodeType nodeType() const OVERRIDE;
    virtual PassRefPtr<Node> cloneNode(bool deep) OVERRIDE;
    virtual bool childTypeAllowed(NodeType) const OVERRIDE;
};

inline bool isDocumentFragment(const Node& node) { return node.isDocumentFragment(); }
void isDocumentFragment(const DocumentFragment&); // Catch unnecessary runtime check of type known at compile time.

NODE_TYPE_CASTS(DocumentFragment)

} //namespace

#endif
