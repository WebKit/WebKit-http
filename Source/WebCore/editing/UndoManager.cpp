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
#include "UndoManager.h"

#if ENABLE(UNDO_MANAGER)

#include "DOMTransaction.h"
#include "Element.h"

namespace WebCore {

PassRefPtr<UndoManager> UndoManager::create(ScriptExecutionContext* context, Node* host)
{
    RefPtr<UndoManager> undoManager = adoptRef(new UndoManager(context, host));
    undoManager->suspendIfNeeded();
    return undoManager.release();
}

UndoManager::UndoManager(ScriptExecutionContext* context, Node* host)
    : ActiveDOMObject(context, this)
    , m_undoScopeHost(host)
    , m_isInProgress(false)
{
}

static void clearStack(UndoManagerStack& stack)
{
    for (size_t i = 0; i < stack.size(); ++i) {
        const UndoManagerEntry& entry = *stack[i];
        for (size_t j = 0; j < entry.size(); ++j) {
            UndoStep* step = entry[j].get();
            if (step->isDOMTransaction())
                static_cast<DOMTransaction*>(step)->setUndoManager(0);
        }
    }
    stack.clear();
}

void UndoManager::disconnect()
{
    m_undoScopeHost = 0;
    clearStack(m_undoStack);
    clearStack(m_redoStack);
}

void UndoManager::stop()
{
    disconnect();
}

UndoManager::~UndoManager()
{
    disconnect();
}

static inline PassOwnPtr<UndoManagerEntry> createUndoManagerEntry()
{
    return adoptPtr(new UndoManagerEntry);
}

void UndoManager::transact(PassRefPtr<DOMTransaction> transaction, bool merge, ExceptionCode& ec)
{
    if (m_isInProgress || !isConnected()) {
        ec = INVALID_ACCESS_ERR;
        return;
    }
    clearRedo(ASSERT_NO_EXCEPTION);
    transaction->setUndoManager(this);

    m_isInProgress = true;
    RefPtr<UndoManager> protect(this);
    transaction->apply();
    m_isInProgress = false;

    if (!m_undoScopeHost)
        return;
    if (!merge || m_undoStack.isEmpty())
        m_undoStack.append(createUndoManagerEntry());
    m_undoStack.last()->append(transaction);
}

void UndoManager::undo(ExceptionCode& ec)
{
    if (m_isInProgress || !isConnected()) {
        ec = INVALID_ACCESS_ERR;
        return;
    }
    if (m_undoStack.isEmpty())
        return;
    m_inProgressEntry = createUndoManagerEntry();

    m_isInProgress = true;
    RefPtr<UndoManager> protect(this);
    UndoManagerEntry entry = *m_undoStack.last();
    for (size_t i = entry.size(); i > 0; --i)
        entry[i - 1]->unapply();
    m_isInProgress = false;

    if (!m_undoScopeHost) {
        m_inProgressEntry.clear();
        return;
    }
    m_redoStack.append(m_inProgressEntry.release());
    m_undoStack.removeLast();
}

void UndoManager::redo(ExceptionCode& ec)
{
    if (m_isInProgress || !isConnected()) {
        ec = INVALID_ACCESS_ERR;
        return;
    }
    if (m_redoStack.isEmpty())
        return;
    m_inProgressEntry = createUndoManagerEntry();

    m_isInProgress = true;
    RefPtr<UndoManager> protect(this);
    UndoManagerEntry entry = *m_redoStack.last();
    for (size_t i = entry.size(); i > 0; --i)
        entry[i - 1]->reapply();
    m_isInProgress = false;

    if (!m_undoScopeHost) {
        m_inProgressEntry.clear();
        return;
    }
    m_undoStack.append(m_inProgressEntry.release());
    m_redoStack.removeLast();
}

void UndoManager::registerUndoStep(PassRefPtr<UndoStep> step)
{
    if (!m_isInProgress) {
        OwnPtr<UndoManagerEntry> entry = createUndoManagerEntry();
        entry->append(step);
        m_undoStack.append(entry.release());

        clearRedo(ASSERT_NO_EXCEPTION);
    } else
        m_inProgressEntry->append(step);
}

void UndoManager::registerRedoStep(PassRefPtr<UndoStep> step)
{
    if (!m_isInProgress) {
        OwnPtr<UndoManagerEntry> entry = createUndoManagerEntry();
        entry->append(step);
        m_redoStack.append(entry.release());
    } else
        m_inProgressEntry->append(step);
}

void UndoManager::clearUndo(ExceptionCode& ec)
{
    if (m_isInProgress || !isConnected()) {
        ec = INVALID_ACCESS_ERR;
        return;
    }
    clearStack(m_undoStack);
}

void UndoManager::clearRedo(ExceptionCode& ec)
{
    if (m_isInProgress || !isConnected()) {
        ec = INVALID_ACCESS_ERR;
        return;
    }
    clearStack(m_redoStack);
}

bool UndoManager::isConnected()
{
    if (!m_undoScopeHost)
        return false;
    if (!m_undoScopeHost->isElementNode())
        return true;
    Element* element = toElement(m_undoScopeHost);
    ASSERT(element->undoScope());
    if (element->isContentEditable() && !element->isRootEditableElement()) {
        element->disconnectUndoManager();
        return false;
    }
    return true;
}

}

#endif
