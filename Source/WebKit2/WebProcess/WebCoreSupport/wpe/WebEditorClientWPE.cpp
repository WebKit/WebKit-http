/*
 * Copyright (C) 2014 Igalia S.L.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "WebEditorClient.h"

#include "PlatformKeyboardEvent.h"
#include <WebCore/Document.h>
#include <WebCore/Editor.h>
#include <WebCore/Frame.h>
#include <WebCore/KeyboardEvent.h>
#include <WebCore/Node.h>
#include <WebCore/WindowsKeyboardCodes.h>

using namespace WebCore;

namespace WebKit {

static void handleKeyPress(Frame& frame, KeyboardEvent& event, const PlatformKeyboardEvent& platformEvent)
{
    // Don't insert null or control characters as they can result in unexpected behaviour
    {
        bool handled = true;
        int keyCode = event.keyCode();

        switch (keyCode) {
        case VK_BACK:
            frame.editor().command("DeleteBackward").execute();
            break;
        case VK_DELETE:
            frame.editor().command("DeleteForward").execute();
            break;
        default:
            handled = false;
            break;
        }
    }

    if (event.charCode() < ' ')
        return;

    // Don't insert anything if a modifier is pressed
    if (platformEvent.ctrlKey() || platformEvent.altKey())
        return;

    if (frame.editor().insertText(platformEvent.text(), &event))
        event.setDefaultHandled();
}

static void handleKeyDown(Frame& frame, KeyboardEvent& event, const PlatformKeyboardEvent&)
{
    bool handled = true;
    int keyCode = event.keyCode();

    switch (keyCode) {
    case VK_LEFT:
        frame.editor().command("MoveLeft").execute();
        break;
    case VK_RIGHT:
        frame.editor().command("MoveRight").execute();
        break;
    case VK_DELETE:
        frame.editor().command("DeleteForward").execute();
        break;
    default:
        handled = false;
        break;
    }

    if (handled)
        event.setDefaultHandled();
}

void WebEditorClient::handleKeyboardEvent(WebCore::KeyboardEvent* event)
{
    Node* node = event->target()->toNode();
    ASSERT(node);
    Frame* frame = node->document().frame();
    ASSERT(frame);

    // FIXME: Reorder the checks in a more sensible way.

    const PlatformKeyboardEvent* platformEvent = event->keyEvent();
    if (!platformEvent)
        return;

    // If this was an IME event don't do anything.
    if (platformEvent->windowsVirtualKeyCode() == VK_PROCESSKEY)
        return;

    // Don't allow text insertion for nodes that cannot edit.
    if (!frame->editor().canEdit())
        return;

    // This is just a normal text insertion, so wait to execute the insertion
    // until a keypress event happens. This will ensure that the insertion will not
    // be reflected in the contents of the field until the keyup DOM event.
    if (event->type() == eventNames().keypressEvent)
        return handleKeyPress(*frame, *event, *platformEvent);
    if (event->type() == eventNames().keydownEvent)
        return handleKeyDown(*frame, *event, *platformEvent);
}

void WebEditorClient::handleInputMethodKeydown(WebCore::KeyboardEvent* event)
{
    const PlatformKeyboardEvent* platformEvent = event->keyEvent();
    if (platformEvent && platformEvent->windowsVirtualKeyCode() == VK_PROCESSKEY)
        event->preventDefault();
}

} // namespace WebKit
