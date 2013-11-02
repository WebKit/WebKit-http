/*
 * Copyright (C) 2001 Peter Kelly (pmk@post.com)
 * Copyright (C) 2001 Tobias Anton (anton@stud.fbi.fh-darmstadt.de)
 * Copyright (C) 2006 Samuel Weinig (sam.weinig@gmail.com)
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2013 Apple Inc. All rights reserved.
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

#ifndef KeyboardEvent_h
#define KeyboardEvent_h

#include "UIEventWithKeyState.h"
#include <wtf/Vector.h>

namespace WebCore {

class Node;
class PlatformKeyboardEvent;

#if PLATFORM(MAC)
struct KeypressCommand {
    KeypressCommand() { }
    explicit KeypressCommand(const String& commandName) : commandName(commandName) { ASSERT(isASCIILower(commandName[0U])); }
    KeypressCommand(const String& commandName, const String& text) : commandName(commandName), text(text) { ASSERT(commandName == "insertText:"); }

    String commandName; // Actually, a selector name - it may have a trailing colon, and a name that can be different from an editor command name.
    String text;
};
#endif

struct KeyboardEventInit : public UIEventInit {
    KeyboardEventInit();

    String keyIdentifier;
    unsigned location;
    bool ctrlKey;
    bool altKey;
    bool shiftKey;
    bool metaKey;
};

class KeyboardEvent : public UIEventWithKeyState {
public:
    enum KeyLocationCode {
        DOM_KEY_LOCATION_STANDARD   = 0x00,
        DOM_KEY_LOCATION_LEFT       = 0x01,
        DOM_KEY_LOCATION_RIGHT      = 0x02,
        DOM_KEY_LOCATION_NUMPAD     = 0x03
        // FIXME: The following values are not supported yet (crbug.com/265446)
        // DOM_KEY_LOCATION_MOBILE     = 0x04,
        // DOM_KEY_LOCATION_JOYSTICK   = 0x05
    };
        
    static PassRefPtr<KeyboardEvent> create()
    {
        return adoptRef(new KeyboardEvent);
    }

    static PassRefPtr<KeyboardEvent> create(const PlatformKeyboardEvent& platformEvent, AbstractView* view)
    {
        return adoptRef(new KeyboardEvent(platformEvent, view));
    }

    static PassRefPtr<KeyboardEvent> create(const AtomicString& type, const KeyboardEventInit& initializer)
    {
        return adoptRef(new KeyboardEvent(type, initializer));
    }

    virtual ~KeyboardEvent();
    
    void initKeyboardEvent(const AtomicString& type, bool canBubble, bool cancelable, AbstractView*,
        const String& keyIdentifier, unsigned location,
        bool ctrlKey, bool altKey, bool shiftKey, bool metaKey, bool altGraphKey = false);
    
    const String& keyIdentifier() const { return m_keyIdentifier; }
    unsigned location() const { return m_location; }

    bool getModifierState(const String& keyIdentifier) const;

    bool altGraphKey() const { return m_altGraphKey; }
    
    const PlatformKeyboardEvent* keyEvent() const { return m_keyEvent.get(); }

    virtual int keyCode() const OVERRIDE; // key code for keydown and keyup, character for keypress
    virtual int charCode() const OVERRIDE; // character code for keypress, 0 for keydown and keyup

    virtual EventInterface eventInterface() const OVERRIDE;
    virtual bool isKeyboardEvent() const OVERRIDE;
    virtual int which() const OVERRIDE;

#if PLATFORM(MAC)
    // We only have this need to store keypress command info on the Mac.
    Vector<KeypressCommand>& keypressCommands() { return m_keypressCommands; }
#endif

private:
    KeyboardEvent();
    KeyboardEvent(const PlatformKeyboardEvent&, AbstractView*);
    KeyboardEvent(const AtomicString&, const KeyboardEventInit&);

    OwnPtr<PlatformKeyboardEvent> m_keyEvent;
    String m_keyIdentifier;
    unsigned m_location;
    bool m_altGraphKey : 1;

#if PLATFORM(MAC)
    // Commands that were sent by AppKit when interpreting the event. Doesn't include input method commands.
    Vector<KeypressCommand> m_keypressCommands;
#endif
};

KeyboardEvent* findKeyboardEvent(Event*);

} // namespace WebCore

#endif // KeyboardEvent_h
