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

#ifndef RemoveNodeCommand_h
#define RemoveNodeCommand_h

#include "EditCommand.h"

namespace WebCore {

class RemoveNodeCommand : public SimpleEditCommand {
public:
    static PassRefPtr<RemoveNodeCommand> create(PassRefPtr<Node> node)
    {
        return adoptRef(new RemoveNodeCommand(node));
    }

private:
    explicit RemoveNodeCommand(PassRefPtr<Node>);

    virtual void doApply() OVERRIDE;
    virtual void doUnapply() OVERRIDE;

#ifndef NDEBUG
    void getNodesInCommand(HashSet<Node*>&) OVERRIDE;
#endif

    RefPtr<Node> m_node;
    RefPtr<ContainerNode> m_parent;
    RefPtr<Node> m_refChild;    
};

} // namespace WebCore

#endif // RemoveNodeCommand_h
