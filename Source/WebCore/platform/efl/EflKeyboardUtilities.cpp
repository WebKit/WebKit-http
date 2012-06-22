/*
 * Copyright (C) 2011 Samsung Electronics
 *
 * All rights reserved.
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

#include "config.h"
#include "EflKeyboardUtilities.h"

#include "KeyboardEvent.h"
#include "WindowsKeyboardCodes.h"
#include <wtf/HashMap.h>
#include <wtf/text/StringHash.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

typedef HashMap<String, String> KeyMap;
typedef HashMap<String, int> WindowsKeyMap;
typedef HashMap<int, const char*> KeyCommandMap;

static KeyMap& keyMap()
{
    DEFINE_STATIC_LOCAL(KeyMap, keyMap, ());
    return keyMap;
}

static WindowsKeyMap& windowsKeyMap()
{
    DEFINE_STATIC_LOCAL(WindowsKeyMap, windowsKeyMap, ());
    return windowsKeyMap;
}

static KeyCommandMap& keyDownCommandsMap()
{
    DEFINE_STATIC_LOCAL(KeyCommandMap, keyDownCommandsMap, ());
    return keyDownCommandsMap;
}

static KeyCommandMap& keyPressCommandsMap()
{
    DEFINE_STATIC_LOCAL(KeyCommandMap, keyPressCommandsMap, ());
    return keyPressCommandsMap;
}

static inline void addCharactersToKeyMap(const char from, const char to)
{
    for (char c = from; c <= to; c++)
        keyMap().set(String(&c, 1), String::format("U+%04X", c));
}

static void createKeyMap()
{
    for (unsigned int i = 1; i < 25; i++) {
        String key = "F" + String::number(i);
        keyMap().set(key, key);
    }
    keyMap().set("Alt_L", "Alt");
    keyMap().set("ISO_Level3_Shift", "Alt");
    keyMap().set("Menu", "Alt");
    keyMap().set("Shift_L", "Shift");
    keyMap().set("Shift_R", "Shift");
    keyMap().set("Down", "Down");
    keyMap().set("End", "End");
    keyMap().set("Return", "Enter");
    keyMap().set("KP_Enter", "Enter");
    keyMap().set("Home", "Home");
    keyMap().set("Insert", "Insert");
    keyMap().set("Left", "Left");
    keyMap().set("Down", "Down");
    keyMap().set("Next", "PageDown");
    keyMap().set("Prior", "PageUp");
    keyMap().set("Right", "Right");
    keyMap().set("Up", "Up");
    keyMap().set("Delete", "U+007F");
    keyMap().set("Tab", "U+0009");
    keyMap().set("ISO_Left_Tab", "U+0009");
    keyMap().set("BackSpace", "U+0008");
    keyMap().set("space", "U+0020");
    keyMap().set("Escape", "U+001B");
    keyMap().set("Print", "PrintScreen");
    // Keypad location
    keyMap().set("KP_Left", "Left");
    keyMap().set("KP_Right", "Right");
    keyMap().set("KP_Up", "Up");
    keyMap().set("KP_Down", "Down");
    keyMap().set("KP_Prior", "PageUp");
    keyMap().set("KP_Next", "PageDown");
    keyMap().set("KP_Home", "Home");
    keyMap().set("KP_End", "End");
    keyMap().set("KP_Insert", "Insert");
    keyMap().set("KP_Delete", "U+007F");
 
    addCharactersToKeyMap('a', 'z');
    addCharactersToKeyMap('A', 'Z');
    addCharactersToKeyMap('0', '9');
}

static inline void addCharactersToWinKeyMap(const char from, const char to, const int baseCode)
{
    int i = 0;
    for (char c = from; c <= to; c++, i++)
        windowsKeyMap().set(String(&c, 1), baseCode + i);
}

static void createWindowsKeyMap()
{
    windowsKeyMap().set("Return", VK_RETURN);
    windowsKeyMap().set("KP_Return", VK_RETURN);
    windowsKeyMap().set("Alt_L", VK_LMENU);
    windowsKeyMap().set("Alt_R", VK_RMENU);
    windowsKeyMap().set("ISO_Level3_Shift", VK_MENU);
    windowsKeyMap().set("Menu", VK_MENU);
    windowsKeyMap().set("Shift_L", VK_LSHIFT);
    windowsKeyMap().set("Shift_R", VK_RSHIFT);
    windowsKeyMap().set("Control_L", VK_LCONTROL);
    windowsKeyMap().set("Control_R", VK_RCONTROL);
    windowsKeyMap().set("Pause", VK_PAUSE);
    windowsKeyMap().set("Break", VK_PAUSE);
    windowsKeyMap().set("Caps_Lock", VK_CAPITAL);
    windowsKeyMap().set("Scroll_Lock", VK_SCROLL);
    windowsKeyMap().set("Num_Lock", VK_NUMLOCK);
    windowsKeyMap().set("Escape", VK_ESCAPE);
    windowsKeyMap().set("Tab", VK_TAB);
    windowsKeyMap().set("ISO_Left_Tab", VK_TAB);
    windowsKeyMap().set("BackSpace", VK_BACK);
    windowsKeyMap().set("space", VK_SPACE);
    windowsKeyMap().set("Next", VK_NEXT);
    windowsKeyMap().set("Prior", VK_PRIOR);
    windowsKeyMap().set("Home", VK_HOME);
    windowsKeyMap().set("End", VK_END);
    windowsKeyMap().set("Right", VK_RIGHT);
    windowsKeyMap().set("Left", VK_LEFT);
    windowsKeyMap().set("Up", VK_UP);
    windowsKeyMap().set("Down", VK_DOWN);
    windowsKeyMap().set("Print", VK_SNAPSHOT);
    windowsKeyMap().set("Insert", VK_INSERT);
    windowsKeyMap().set("Delete", VK_DELETE);

    windowsKeyMap().set("comma", VK_OEM_COMMA);
    windowsKeyMap().set("less", VK_OEM_COMMA);
    windowsKeyMap().set("period", VK_OEM_PERIOD);
    windowsKeyMap().set("greater", VK_OEM_PERIOD);
    windowsKeyMap().set("semicolon", VK_OEM_1);
    windowsKeyMap().set("colon", VK_OEM_1);
    windowsKeyMap().set("slash", VK_OEM_2);
    windowsKeyMap().set("question", VK_OEM_2);
    windowsKeyMap().set("grave", VK_OEM_3);
    windowsKeyMap().set("asciitilde", VK_OEM_3);
    windowsKeyMap().set("bracketleft", VK_OEM_4);
    windowsKeyMap().set("braceleft", VK_OEM_4);
    windowsKeyMap().set("backslash", VK_OEM_5);
    windowsKeyMap().set("bar", VK_OEM_5);
    windowsKeyMap().set("bracketright", VK_OEM_6);
    windowsKeyMap().set("braceright", VK_OEM_6);
    windowsKeyMap().set("apostrophe", VK_OEM_7);
    windowsKeyMap().set("quotedbl", VK_OEM_7);
    // Keypad location
    windowsKeyMap().set("KP_Left", VK_LEFT);
    windowsKeyMap().set("KP_Right", VK_RIGHT);
    windowsKeyMap().set("KP_Up", VK_UP);
    windowsKeyMap().set("KP_Down", VK_DOWN);
    windowsKeyMap().set("KP_Prior", VK_PRIOR);
    windowsKeyMap().set("KP_Next", VK_NEXT);
    windowsKeyMap().set("KP_Home", VK_HOME);
    windowsKeyMap().set("KP_End", VK_END);
    windowsKeyMap().set("KP_Insert", VK_INSERT);
    windowsKeyMap().set("KP_Delete", VK_DELETE);

    // Set alphabet to the windowsKeyMap.
    addCharactersToWinKeyMap('a', 'z', VK_A);
    addCharactersToWinKeyMap('A', 'Z', VK_A);

    // Set digits to the windowsKeyMap.
    addCharactersToWinKeyMap('0', '9', VK_0);

    // Set shifted digits to the windowsKeyMap.
    windowsKeyMap().set("exclam", VK_1);
    windowsKeyMap().set("at", VK_2);
    windowsKeyMap().set("numbersign", VK_3);
    windowsKeyMap().set("dollar", VK_4);
    windowsKeyMap().set("percent", VK_5);
    windowsKeyMap().set("asciicircum", VK_6);
    windowsKeyMap().set("ampersand", VK_7);
    windowsKeyMap().set("asterisk", VK_8);
    windowsKeyMap().set("parenleft", VK_9);
    windowsKeyMap().set("parenright", VK_0);
    windowsKeyMap().set("minus", VK_OEM_MINUS);
    windowsKeyMap().set("underscore", VK_OEM_MINUS);
    windowsKeyMap().set("equal", VK_OEM_PLUS);
    windowsKeyMap().set("plus", VK_OEM_PLUS);

    // Set F_XX keys to the windowsKeyMap.
    for (unsigned int i = 1; i < 25; i++) {
        String key = "F" + String::number(i);
        windowsKeyMap().set(key, VK_F1 + i - 1);
    }
}

static const unsigned CtrlKey = 1 << 0;
static const unsigned AltKey = 1 << 1;
static const unsigned ShiftKey = 1 << 2;

struct KeyDownEntry {
    unsigned virtualKey;
    unsigned modifiers;
    const char* name;
};

struct KeyPressEntry {
    unsigned charCode;
    unsigned modifiers;
    const char* name;
};

static const KeyDownEntry keyDownEntries[] = {
    { VK_LEFT,   0,                  "MoveLeft"                                    },
    { VK_LEFT,   ShiftKey,           "MoveLeftAndModifySelection"                  },
    { VK_LEFT,   CtrlKey,            "MoveWordLeft"                                },
    { VK_LEFT,   CtrlKey | ShiftKey, "MoveWordLeftAndModifySelection"              },
    { VK_RIGHT,  0,                  "MoveRight"                                   },
    { VK_RIGHT,  ShiftKey,           "MoveRightAndModifySelection"                 },
    { VK_RIGHT,  CtrlKey,            "MoveWordRight"                               },
    { VK_RIGHT,  CtrlKey | ShiftKey, "MoveWordRightAndModifySelection"             },
    { VK_UP,     0,                  "MoveUp"                                      },
    { VK_UP,     ShiftKey,           "MoveUpAndModifySelection"                    },
    { VK_PRIOR,  ShiftKey,           "MovePageUpAndModifySelection"                },
    { VK_DOWN,   0,                  "MoveDown"                                    },
    { VK_DOWN,   ShiftKey,           "MoveDownAndModifySelection"                  },
    { VK_NEXT,   ShiftKey,           "MovePageDownAndModifySelection"              },
    { VK_PRIOR,  0,                  "MovePageUp"                                  },
    { VK_NEXT,   0,                  "MovePageDown"                                },
    { VK_HOME,   0,                  "MoveToBeginningOfLine"                       },
    { VK_HOME,   ShiftKey,           "MoveToBeginningOfLineAndModifySelection"     },
    { VK_HOME,   CtrlKey,            "MoveToBeginningOfDocument"                   },
    { VK_HOME,   CtrlKey | ShiftKey, "MoveToBeginningOfDocumentAndModifySelection" },

    { VK_END,    0,                  "MoveToEndOfLine"                             },
    { VK_END,    ShiftKey,           "MoveToEndOfLineAndModifySelection"           },
    { VK_END,    CtrlKey,            "MoveToEndOfDocument"                         },
    { VK_END,    CtrlKey | ShiftKey, "MoveToEndOfDocumentAndModifySelection"       },

    { VK_BACK,   0,                  "DeleteBackward"                              },
    { VK_BACK,   ShiftKey,           "DeleteBackward"                              },
    { VK_DELETE, 0,                  "DeleteForward"                               },
    { VK_BACK,   CtrlKey,            "DeleteWordBackward"                          },
    { VK_DELETE, CtrlKey,            "DeleteWordForward"                           },

    { 'B',       CtrlKey,            "ToggleBold"                                  },
    { 'I',       CtrlKey,            "ToggleItalic"                                },

    { VK_ESCAPE, 0,                  "Cancel"                                      },
    { VK_OEM_PERIOD, CtrlKey,        "Cancel"                                      },
    { VK_TAB,    0,                  "InsertTab"                                   },
    { VK_TAB,    ShiftKey,           "InsertBacktab"                               },
    { VK_RETURN, 0,                  "InsertNewline"                               },
    { VK_RETURN, CtrlKey,            "InsertNewline"                               },
    { VK_RETURN, AltKey,             "InsertNewline"                               },
    { VK_RETURN, AltKey | ShiftKey,  "InsertNewline"                               },
};

static const KeyPressEntry keyPressEntries[] = {
    { '\t',   0,                  "InsertTab"                                   },
    { '\t',   ShiftKey,           "InsertBacktab"                               },
    { '\r',   0,                  "InsertNewline"                               },
    { '\r',   CtrlKey,            "InsertNewline"                               },
    { '\r',   AltKey,             "InsertNewline"                               },
    { '\r',   AltKey | ShiftKey,  "InsertNewline"                               },
};

static void createKeyDownCommandMap()
{
    for (size_t i = 0; i < WTF_ARRAY_LENGTH(keyDownEntries); ++i)
        keyDownCommandsMap().set(keyDownEntries[i].modifiers << 16 | keyDownEntries[i].virtualKey, keyDownEntries[i].name);
}

static void createKeyPressCommandMap()
{
    for (size_t i = 0; i < WTF_ARRAY_LENGTH(keyPressEntries); ++i)
        keyPressCommandsMap().set(keyPressEntries[i].modifiers << 16 | keyPressEntries[i].charCode, keyPressEntries[i].name);
}

String keyIdentifierForEvasKeyName(const String& keyName)
{
    if (keyMap().isEmpty())
        createKeyMap();

    if (keyMap().contains(keyName))
        return keyMap().get(keyName);

    return keyName;
}

int windowsKeyCodeForEvasKeyName(const String& keyName)
{
    if (windowsKeyMap().isEmpty())
        createWindowsKeyMap();

    if (windowsKeyMap().contains(keyName))
        return windowsKeyMap().get(keyName);

    return 0;
}

const char* getKeyDownCommandName(const KeyboardEvent* event)
{
    unsigned modifiers = 0;
    if (event->shiftKey())
        modifiers |= ShiftKey;
    if (event->altKey())
        modifiers |= AltKey;
    if (event->ctrlKey())
        modifiers |= CtrlKey;

    int mapKey = modifiers << 16 | event->keyCode();
    if (!mapKey)
        return 0;

    if (keyDownCommandsMap().isEmpty())
        createKeyDownCommandMap();

    return keyDownCommandsMap().get(mapKey);
}

const char* getKeyPressCommandName(const KeyboardEvent* event)
{
    unsigned modifiers = 0;
    if (event->shiftKey())
        modifiers |= ShiftKey;
    if (event->altKey())
        modifiers |= AltKey;
    if (event->ctrlKey())
        modifiers |= CtrlKey;

    int mapKey = modifiers << 16 | event->charCode();
    if (!mapKey)
        return 0;

    if (keyPressCommandsMap().isEmpty())
        createKeyPressCommandMap();

    return keyPressCommandsMap().get(mapKey);
}

} // namespace WebCore
