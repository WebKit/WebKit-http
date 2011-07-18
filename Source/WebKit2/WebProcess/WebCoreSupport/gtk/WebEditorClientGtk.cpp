/*
 *  Copyright (C) 2011 Igalia S.L.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public License
 *  as published by the Free Software Foundation; either version 2 of
 *  the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301 USA
 */

#include "config.h"
#include "WebEditorClient.h"

#include "Frame.h"
#include "PlatformKeyboardEvent.h"
#include "WebPage.h"
#include "WebPageProxyMessages.h"
#include "WebProcess.h"
#include <WebCore/KeyboardEvent.h>
#include <WebCore/NotImplemented.h>

using namespace WebCore;

namespace WebKit {

void WebEditorClient::getEditorCommandsForKeyEvent(const KeyboardEvent* event, Vector<WTF::String>& pendingEditorCommands)
{
    ASSERT(event->type() == eventNames().keydownEvent || event->type() == eventNames().keypressEvent);

    /* First try to interpret the command in the UI and get the commands.
       UI needs to receive event type because only knows current NativeWebKeyboardEvent.*/
    WebProcess::shared().connection()->sendSync(Messages::WebPageProxy::GetEditorCommandsForKeyEvent(event->type()),
                                                Messages::WebPageProxy::GetEditorCommandsForKeyEvent::Reply(pendingEditorCommands),
                                                m_page->pageID(), CoreIPC::Connection::NoTimeout);
}

bool WebEditorClient::executePendingEditorCommands(Frame* frame, Vector<WTF::String> pendingEditorCommands, bool allowTextInsertion)
{
    Vector<Editor::Command> commands;
    for (size_t i = 0; i < pendingEditorCommands.size(); i++) {
        Editor::Command command = frame->editor()->command(pendingEditorCommands.at(i).utf8().data());
        if (command.isTextInsertion() && !allowTextInsertion)
            return false;

        commands.append(command);
    }

    for (size_t i = 0; i < commands.size(); i++) {
        if (!commands.at(i).execute())
            return false;
    }

    return true;
}

void WebEditorClient::handleKeyboardEvent(KeyboardEvent* event)
{
    Node* node = event->target()->toNode();
    ASSERT(node);
    Frame* frame = node->document()->frame();
    ASSERT(frame);

    const PlatformKeyboardEvent* platformEvent = event->keyEvent();
    if (!platformEvent)
        return;

    Vector<WTF::String> pendingEditorCommands;
    getEditorCommandsForKeyEvent(event, pendingEditorCommands);
    if (!pendingEditorCommands.isEmpty()) {

        // During RawKeyDown events if an editor command will insert text, defer
        // the insertion until the keypress event. We want keydown to bubble up
        // through the DOM first.
        if (platformEvent->type() == PlatformKeyboardEvent::RawKeyDown) {
            if (executePendingEditorCommands(frame, pendingEditorCommands, false))
                event->setDefaultHandled();

            return;
        }

        // Only allow text insertion commands if the current node is editable.
        if (executePendingEditorCommands(frame, pendingEditorCommands, frame->editor()->canEdit())) {
            event->setDefaultHandled();
            return;
        }
    }

    // Don't allow text insertion for nodes that cannot edit.
    if (!frame->editor()->canEdit())
        return;

    // This is just a normal text insertion, so wait to execute the insertion
    // until a keypress event happens. This will ensure that the insertion will not
    // be reflected in the contents of the field until the keyup DOM event.
    if (event->type() == eventNames().keypressEvent) {

        // FIXME: Add IM support
        // https://bugs.webkit.org/show_bug.cgi?id=55946
        frame->editor()->insertText(platformEvent->text(), event);
        event->setDefaultHandled();

    } else {
        // Don't insert null or control characters as they can result in unexpected behaviour
        if (event->charCode() < ' ')
            return;

        // Don't insert anything if a modifier is pressed
        if (platformEvent->ctrlKey() || platformEvent->altKey())
            return;

        if (frame->editor()->insertText(platformEvent->text(), event))
            event->setDefaultHandled();
    }
}

void WebEditorClient::handleInputMethodKeydown(KeyboardEvent*)
{
    notImplemented();
}

}
