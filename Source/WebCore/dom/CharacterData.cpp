/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
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
 */

#include "config.h"
#include "CharacterData.h"

#include "Document.h"
#include "EventNames.h"
#include "ExceptionCode.h"
#include "InspectorInstrumentation.h"
#include "MutationEvent.h"
#include "MutationObserverInterestGroup.h"
#include "MutationRecord.h"
#include "NodeRenderingContext.h"
#include "RenderText.h"
#include "StyleInheritedData.h"
#include "TextBreakIterator.h"
#include "UndoManager.h"
#include "WebCoreMemoryInstrumentation.h"

using namespace std;

namespace WebCore {

void CharacterData::setData(const String& data, ExceptionCode&)
{
    const String& nonNullData = !data.isNull() ? data : emptyString();
    if (m_data == nonNullData)
        return;

    RefPtr<CharacterData> protect = this;

    unsigned oldLength = length();

    setDataAndUpdate(nonNullData, 0, oldLength, nonNullData.length());
    document()->textRemoved(this, 0, oldLength);
}

String CharacterData::substringData(unsigned offset, unsigned count, ExceptionCode& ec)
{
    checkCharDataOperation(offset, ec);
    if (ec)
        return String();

    return m_data.substring(offset, count);
}

unsigned CharacterData::parserAppendData(const UChar* data, unsigned dataLength, unsigned lengthLimit)
{
    unsigned oldLength = m_data.length();

    unsigned end = min(dataLength, lengthLimit - oldLength);

    // Check that we are not on an unbreakable boundary.
    // Some text break iterator implementations work best if the passed buffer is as small as possible, 
    // see <https://bugs.webkit.org/show_bug.cgi?id=29092>. 
    // We need at least two characters look-ahead to account for UTF-16 surrogates.
    if (end < dataLength) {
        NonSharedCharacterBreakIterator it(data, (end + 2 > dataLength) ? dataLength : end + 2);
        if (!isTextBreak(it, end))
            end = textBreakPreceding(it, end);
    }
    
    if (!end)
        return 0;

    m_data.append(data, end);

    updateRenderer(oldLength, 0);
    document()->incDOMTreeVersion();
    // We don't call dispatchModifiedEvent here because we don't want the
    // parser to dispatch DOM mutation events.
    if (parentNode())
        parentNode()->childrenChanged();
    
    return end;
}

void CharacterData::reportMemoryUsage(MemoryObjectInfo* memoryObjectInfo) const
{
    MemoryClassInfo info(memoryObjectInfo, this, WebCoreMemoryTypes::DOM);
    Node::reportMemoryUsage(memoryObjectInfo);
    info.addMember(m_data);
}

void CharacterData::appendData(const String& data, ExceptionCode&)
{
    String newStr = m_data;
    newStr.append(data);

    setDataAndUpdate(newStr, m_data.length(), 0, data.length());

    // FIXME: Should we call textInserted here?
}

void CharacterData::insertData(unsigned offset, const String& data, ExceptionCode& ec)
{
    checkCharDataOperation(offset, ec);
    if (ec)
        return;

    String newStr = m_data;
    newStr.insert(data, offset);

    setDataAndUpdate(newStr, offset, 0, data.length());

    document()->textInserted(this, offset, data.length());
}

void CharacterData::deleteData(unsigned offset, unsigned count, ExceptionCode& ec)
{
    checkCharDataOperation(offset, ec);
    if (ec)
        return;

    unsigned realCount;
    if (offset + count > length())
        realCount = length() - offset;
    else
        realCount = count;

    String newStr = m_data;
    newStr.remove(offset, realCount);

    setDataAndUpdate(newStr, offset, count, 0);

    document()->textRemoved(this, offset, realCount);
}

void CharacterData::replaceData(unsigned offset, unsigned count, const String& data, ExceptionCode& ec)
{
    checkCharDataOperation(offset, ec);
    if (ec)
        return;

    unsigned realCount;
    if (offset + count > length())
        realCount = length() - offset;
    else
        realCount = count;

    String newStr = m_data;
    newStr.remove(offset, realCount);
    newStr.insert(data, offset);

    setDataAndUpdate(newStr, offset, count, data.length());

    // update the markers for spell checking and grammar checking
    document()->textRemoved(this, offset, realCount);
    document()->textInserted(this, offset, data.length());
}

String CharacterData::nodeValue() const
{
    return m_data;
}

bool CharacterData::containsOnlyWhitespace() const
{
    return m_data.containsOnlyWhitespace();
}

void CharacterData::setNodeValue(const String& nodeValue, ExceptionCode& ec)
{
    setData(nodeValue, ec);
}

void CharacterData::setDataAndUpdate(const String& newData, unsigned offsetOfReplacedData, unsigned oldLength, unsigned newLength)
{
#if ENABLE(UNDO_MANAGER)
    if (UndoManager::isRecordingAutomaticTransaction(this)) {
        const String& replacingData = newData.substring(offsetOfReplacedData, newLength);
        const String& replacedData = m_data.substring(offsetOfReplacedData, oldLength);
        UndoManager::addTransactionStep(DataReplacingDOMTransactionStep::create(this, offsetOfReplacedData, oldLength, replacingData, replacedData));
    }
#endif
    String oldData = m_data;
    m_data = newData;

    updateRenderer(offsetOfReplacedData, oldLength);

    if (document()->frame())
        document()->frame()->selection()->textWasReplaced(this, offsetOfReplacedData, oldLength, newLength);

    document()->incDOMTreeVersion();
    dispatchModifiedEvent(oldData);
}

void CharacterData::updateRenderer(unsigned offsetOfReplacedData, unsigned lengthOfReplacedData)
{
    if ((!renderer() || !rendererIsNeeded(NodeRenderingContext(this, renderer()->style()))) && attached())
        reattach();
    else if (renderer())
        toRenderText(renderer())->setTextWithOffset(m_data.impl(), offsetOfReplacedData, lengthOfReplacedData);
}

void CharacterData::dispatchModifiedEvent(const String& oldData)
{
#if ENABLE(MUTATION_OBSERVERS)
    if (OwnPtr<MutationObserverInterestGroup> mutationRecipients = MutationObserverInterestGroup::createForCharacterDataMutation(this))
        mutationRecipients->enqueueMutationRecord(MutationRecord::createCharacterData(this, oldData));
#endif
    if (!isInShadowTree()) {
        if (parentNode())
            parentNode()->childrenChanged();
        if (document()->hasListenerType(Document::DOMCHARACTERDATAMODIFIED_LISTENER))
            dispatchScopedEvent(MutationEvent::create(eventNames().DOMCharacterDataModifiedEvent, true, 0, oldData, m_data));
        dispatchSubtreeModifiedEvent();
#if ENABLE(INSPECTOR)
        InspectorInstrumentation::characterDataModified(document(), this);
#endif
    }
}

void CharacterData::checkCharDataOperation(unsigned offset, ExceptionCode& ec)
{
    ec = 0;

    // INDEX_SIZE_ERR: Raised if the specified offset is negative or greater than the number of 16-bit
    // units in data.
    if (offset > length()) {
        ec = INDEX_SIZE_ERR;
        return;
    }
}

int CharacterData::maxCharacterOffset() const
{
    return static_cast<int>(length());
}

bool CharacterData::rendererIsNeeded(const NodeRenderingContext& context)
{
    if (!m_data || !length())
        return false;
    return Node::rendererIsNeeded(context);
}

bool CharacterData::offsetInCharacters() const
{
    return true;
}

} // namespace WebCore
