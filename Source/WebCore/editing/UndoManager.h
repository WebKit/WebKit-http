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

#ifndef UndoManager_h
#define UndoManager_h

#if ENABLE(UNDO_MANAGER)

#include "ActiveDOMObject.h"
#include "DOMTransaction.h"
#include "Document.h"
#include "ExceptionCodePlaceholder.h"
#include "UndoStep.h"
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>
#include <wtf/Vector.h>

namespace WebCore {

typedef Vector<RefPtr<UndoStep> > UndoManagerEntry;
typedef Vector<OwnPtr<UndoManagerEntry> > UndoManagerStack;

class UndoManager : public RefCounted<UndoManager>, public ActiveDOMObject {
public:
    static PassRefPtr<UndoManager> create(Document*);
    void disconnect();
    virtual void stop() OVERRIDE;
    virtual ~UndoManager();

    void transact(PassRefPtr<DOMTransaction>, bool merge, ExceptionCode&);

    void undo(ExceptionCode& = ASSERT_NO_EXCEPTION);
    void redo(ExceptionCode& = ASSERT_NO_EXCEPTION);

    UndoManagerEntry item(unsigned index) const;

    unsigned length() const { return m_undoStack.size() + m_redoStack.size(); }
    unsigned position() const { return m_redoStack.size(); }

    void clearUndo(ExceptionCode&);
    void clearRedo(ExceptionCode&);
    
    bool canUndo() const { return !m_undoStack.isEmpty(); }
    bool canRedo() const { return !m_redoStack.isEmpty(); }
    
    void registerUndoStep(PassRefPtr<UndoStep>);
    void registerRedoStep(PassRefPtr<UndoStep>);
    
    Document* document() const { return m_document; }
    Node* ownerNode() const { return m_document; }

    static void setRecordingDOMTransaction(DOMTransaction* transaction) { s_recordingDOMTransaction = transaction; }
    static bool isRecordingAutomaticTransaction(Node*);
    static void addTransactionStep(PassRefPtr<DOMTransactionStep>);

private:
    explicit UndoManager(Document*);
    
    Document* m_document;
    UndoManagerStack m_undoStack;
    UndoManagerStack m_redoStack;
    bool m_isInProgress;
    OwnPtr<UndoManagerEntry> m_inProgressEntry;

    static DOMTransaction* s_recordingDOMTransaction;
};
    
}
    
#endif

#endif
