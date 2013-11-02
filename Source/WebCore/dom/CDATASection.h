/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2003, 2009 Apple Inc. All rights reserved.
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

#ifndef CDATASection_h
#define CDATASection_h

#include "Text.h"

namespace WebCore {

class CDATASection FINAL : public Text {
public:
    static PassRefPtr<CDATASection> create(Document&, const String&);

private:
    CDATASection(Document&, const String&);

    virtual String nodeName() const OVERRIDE;
    virtual NodeType nodeType() const OVERRIDE;
    virtual PassRefPtr<Node> cloneNode(bool deep) OVERRIDE;
    virtual bool childTypeAllowed(NodeType) const OVERRIDE;
    virtual PassRefPtr<Text> virtualCreate(const String&) OVERRIDE;
};

inline bool isCDATASection(const Node& node) { return node.nodeType() == Node::CDATA_SECTION_NODE; }
void isCDATASection(const CDATASection&); // Catch unnecessary runtime check of type known at compile time.
void isCDATASection(const ContainerNode&); // Catch unnecessary runtime check of type known at compile time.

NODE_TYPE_CASTS(CDATASection)

} // namespace WebCore

#endif // CDATASection_h
