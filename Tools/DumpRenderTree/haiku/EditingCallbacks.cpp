/*
 * Copyright (C) 2010 Igalia S.L.
 * Copyright (C) 2012 Samsung Electronics
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "EditingCallbacks.h"

#include "DumpRenderTree.h"
#include "EditorClientHaiku.h"
#include "EditorInsertAction.h"
#include "Node.h"
#include "Range.h"
#include "StyleProperties.h"
#include "TestRunner.h"
#include "TextAffinity.h"
#include "WebViewConstants.h"
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

static WTF::String dumpPath(WebCore::Node* node)
{
    WebCore::Node* parent = node->parentNode();
    WTF::String str = WTF::String::format("%s", node->nodeName().utf8().data());
    if (parent) {
        str.append(" > ");
        str.append(dumpPath(parent));
    }
    return str;
}

static WTF::String dumpRange(WebCore::Range* range)
{
    if (!range)
        return "(null)";
    return WTF::String::format("range from %d of %s to %d of %s", range->startOffset(), dumpPath(range->startContainer()).utf8().data(), range->endOffset(), dumpPath(range->endContainer()).utf8().data());
}

static const char* insertActionString(WebCore::EditorInsertAction action)
{
    switch (action) {
    case WebCore::EditorInsertActionTyped:
        return "WebViewInsertActionTyped";
    case WebCore::EditorInsertActionPasted:
        return "WebViewInsertActionPasted";
    case WebCore::EditorInsertActionDropped:
        return "WebViewInsertActionDropped";
    }
    ASSERT_NOT_REACHED();
    return "WebViewInsertActionTyped";
}

static const char* selectionAffinityString(WebCore::EAffinity affinity)
{
    switch (affinity) {
    case WebCore::UPSTREAM:
        return "NSSelectionAffinityUpstream";
    case WebCore::DOWNSTREAM:
        return "NSSelectionAffinityDownstream";
    }
    ASSERT_NOT_REACHED();
    return "NSSelectionAffinityUpstream";
}

static void shouldBeginEditing(WebCore::Range* range)
{
    if (!done && gTestRunner->dumpEditingCallbacks()) {
        printf("EDITING DELEGATE: shouldBeginEditingInDOMRange:%s\n", dumpRange(range).utf8().data());
    }
}

static void shouldEndEditing(WebCore::Range* range)
{
    if (!done && gTestRunner->dumpEditingCallbacks()) {
        printf("EDITING DELEGATE: shouldEndEditingInDOMRange:%s\n", dumpRange(range).utf8().data());
    }
}

static void shouldInsertNode(WebCore::Node* node, WebCore::Range* range,
    WebCore::EditorInsertAction action)
{
    if (!done && gTestRunner->dumpEditingCallbacks()) {
        printf("EDITING DELEGATE: shouldInsertNode:%s replacingDOMRange:%s givenAction:%s\n",
               dumpPath(node).utf8().data(), dumpRange(range).utf8().data(),
               insertActionString(action));
    }
}

static void shouldInsertText(BString text, WebCore::Range* range,
    WebCore::EditorInsertAction action)
{
    if (!done && gTestRunner->dumpEditingCallbacks()) {
        printf("EDITING DELEGATE: shouldInsertText:%s replacingDOMRange:%s givenAction:%s\n",
            text.String(), dumpRange(range).utf8().data(), insertActionString(action));
    }
}

static void shouldDeleteRange(WebCore::Range* range)
{
    if (!done && gTestRunner->dumpEditingCallbacks()) {
        printf("EDITING DELEGATE: shouldDeleteDOMRange:%s\n", dumpRange(range).utf8().data());
    }
}

static void shouldChangeSelectedRange(WebCore::Range* fromRange, WebCore::Range* toRange,
    WebCore::EAffinity affinity, bool stillSelecting)
{
    if (!done && gTestRunner->dumpEditingCallbacks()) {
        printf("EDITING DELEGATE: shouldChangeSelectedDOMRange:%s toDOMRange:%s affinity:%s stillSelecting:%s\n",
               dumpRange(fromRange).utf8().data(), dumpRange(toRange).utf8().data(),
               selectionAffinityString(affinity), stillSelecting ? "TRUE" : "FALSE");
    }
}

static void shouldApplyStyle(WebCore::StyleProperties* style, WebCore::Range* range)
{
    if (!done && gTestRunner->dumpEditingCallbacks()) {
        printf("EDITING DELEGATE: shouldApplyStyle:%s toElementsInDOMRange:%s\n",
                style->asText().utf8().data(), dumpRange(range).utf8().data());
    }
}

static void editingBegan()
{
    if (!done && gTestRunner->dumpEditingCallbacks())
        printf("EDITING DELEGATE: webViewDidBeginEditing:WebViewDidBeginEditingNotification\n");
}

static void userChangedContents()
{
    if (!done && gTestRunner->dumpEditingCallbacks())
        printf("EDITING DELEGATE: webViewDidChange:WebViewDidChangeNotification\n");
}

static void editingEnded()
{
    if (!done && gTestRunner->dumpEditingCallbacks())
        printf("EDITING DELEGATE: webViewDidEndEditing:WebViewDidEndEditingNotification\n");
}

static void selectionChanged()
{
    if (!done && gTestRunner->dumpEditingCallbacks())
        printf("EDITING DELEGATE: webViewDidChangeSelection:WebViewDidChangeSelectionNotification\n");
}

bool handleEditingCallback(BMessage* message)
{
    switch(message->what)
    {
        case EDITOR_DELETE_RANGE:
        {
            WebCore::Range* range = NULL;
            message->FindPointer("range", (void**)&range);
            shouldDeleteRange(range);
            return true;
        }
        case EDITOR_BEGIN_EDITING:
        {
            WebCore::Range* range = NULL;
            message->FindPointer("range", (void**)&range);
            shouldBeginEditing(range);
            return true;
        }
        case EDITOR_EDITING_BEGAN:
            editingBegan();
            return true;
        case EDITOR_EDITING_ENDED:
            editingEnded();
            return true;
        case EDITOR_END_EDITING:
        {
            WebCore::Range* range = NULL;
            message->FindPointer("range", (void**)&range);
            shouldEndEditing(range);
            return true;
        }
        case EDITOR_INSERT_NODE:
        {
            WebCore::Range* range = NULL;
            WebCore::Node* node = NULL;
            message->FindPointer("range", (void**)&range);
            message->FindPointer("node", (void**)&node);
            WebCore::EditorInsertAction action = (WebCore::EditorInsertAction)message->FindInt32("action");
            shouldInsertNode(node, range, action);
            return true;
        }
        case EDITOR_INSERT_TEXT:
        {
            WebCore::Range* range = NULL;
            message->FindPointer("range", (void**)&range);
            BString text = message->FindString("text");
            WebCore::EditorInsertAction action = (WebCore::EditorInsertAction)message->FindInt32("action");
            shouldInsertText(text, range, action);
            return true;
        }
        case EDITOR_CHANGE_SELECTED_RANGE:
        {
            WebCore::Range* fromRange = NULL;
            WebCore::Range* toRange = NULL;
            message->FindPointer("from", (void**)&fromRange);
            message->FindPointer("to", (void**)&toRange);
            WebCore::EAffinity affinity = (WebCore::EAffinity)message->FindInt32("affinity");
            bool stillSelecting = message->FindBool("stillSelecting");
            shouldChangeSelectedRange(fromRange, toRange, affinity, stillSelecting);
            return true;
        }
        case EDITOR_APPLY_STYLE:
        {
            WebCore::Range* range = NULL;
            WebCore::StyleProperties* style = NULL;
            message->FindPointer("range", (void**)&range);
            message->FindPointer("style", (void**)&style);
            shouldApplyStyle(style, range);
            return true;
        }
        case EDITOR_SELECTION_CHANGED:
        {
            selectionChanged();
            return true;
        }
        case EDITOR_CONTENTS_CHANGED:
        {
            userChangedContents();
            return true;
        }
    }
    return false;
}
