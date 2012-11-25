/*
 * Copyright (c) 2011 Motorola Mobility, Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY MOTOROLA MOBILITY, INC. AND ITS CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MOTOROLA MOBILITY, INC. OR ITS
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef MicroDataItemList_h
#define MicroDataItemList_h

#if ENABLE(MICRODATA)

#include "LiveNodeList.h"
#include "Node.h"
#include "SpaceSplitString.h"

namespace WebCore {

class MicroDataItemList : public LiveNodeList {
public:
    static PassRefPtr<MicroDataItemList> create(PassRefPtr<Node> rootNode, const String& typeNames)
    {
        return adoptRef(new MicroDataItemList(rootNode, typeNames));
    }

    virtual ~MicroDataItemList();

    static const String& undefinedItemType();

private:
    MicroDataItemList(PassRefPtr<Node> rootNode, const String& typeNames);

    virtual bool nodeMatches(Element*) const;

    SpaceSplitString m_typeNames;
    String m_originalTypeNames;
};

} // namespace WebCore

#endif // ENABLE(MICRODATA)
#endif // MicroDataItemList_h
