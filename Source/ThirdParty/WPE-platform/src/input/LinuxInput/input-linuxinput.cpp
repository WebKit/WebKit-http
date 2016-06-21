/*
 * Copyright (C) 2015, 2016 Igalia S.L.
 * Copyright (C) 2015, 2016 Metrological
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "input-linuxinput.h"

#ifdef KEY_INPUT_HANDLING_LINUX_INPUT

#include "WindowsKeyboardCodes.h"
#include <linux/input.h>

extern "C" {

struct wpe_input_key_mapper_interface linuxinput_input_key_mapper_interface = {
    // identifier_for_key_event
    [](struct wpe_input_keyboard_event* event) -> const char*
    {
        switch (event->keyCode) {
    // "Accept" The Accept (Commit) key.
    // "Again" The Again key.
    // "AllCandidates" The All Candidates key.
    // "Alphanumeric" The Alphanumeric key.
    // "AltGraph" The Alt-Graph key.
            case KEY_MENU:
            case KEY_LEFTALT:
            case KEY_RIGHTALT:
                return "Alt";
    // "Apps" The Application key.
    // "Attn" The ATTN key.
    // "BrowserBack" The Browser Back key.
    // "BrowserFavorites" The Browser Favorites key.
    // "BrowserForward" The Browser Forward key.
    // "BrowserHome" The Browser Home key.
    // "BrowserRefresh" The Browser Refresh key.
    // "BrowserSearch" The Browser Search key.
    // "BrowserStop" The Browser Stop key.
    // "CapsLock" The Caps Lock (Capital) key.
            case KEY_CLEAR:
                return "Clear";
    // "CodeInput" The Code Input key.
    // "Compose" The Compose key.
    // "Control" The Control (Ctrl) key.
    // "Crsel" The Crsel key.
    // "Convert" The Convert key.
    // "Copy" The Copy key.
    // "Cut" The Cut key.
            case KEY_DOWN:
                return "Down";
            case KEY_END:
                return "End";
            case KEY_ENTER:
            case KEY_KPENTER:
                return "Enter";
    // "EraseEof" The Erase EOF key.
    // "Execute" The Execute key.
    // "Exsel" The Exsel key.
            case KEY_F1:
                return "F1";
            case KEY_F2:
                return "F2";
            case KEY_F3:
                return "F3";
            case KEY_F4:
                return "F4";
            case KEY_F5:
                return "F5";
            case KEY_F6:
                return "F6";
            case KEY_F7:
                return "F7";
            case KEY_F8:
                return "F8";
            case KEY_F9:
                return "F9";
            case KEY_F10:
                return "F10";
            case KEY_F11:
                return "F11";
            case KEY_F12:
                return "F12";
            case KEY_F13:
                return "F13";
            case KEY_F14:
                return "F14";
            case KEY_F15:
                return "F15";
            case KEY_F16:
                return "F16";
            case KEY_F17:
                return "F17";
            case KEY_F18:
                return "F18";
            case KEY_F19:
                return "F19";
            case KEY_F20:
                return "F20";
            case KEY_F21:
                return "F21";
            case KEY_F22:
                return "F22";
            case KEY_F23:
                return "F23";
            case KEY_F24:
                return "F24";
    // "FinalMode" The Final Mode (Final) key used on some asian keyboards.
    // "Find" The Find key.
    // "FullWidth" The Full-Width Characters key.
    // "HalfWidth" The Half-Width Characters key.
    // "HangulMode" The Hangul (Korean characters) Mode key.
    // "HanjaMode" The Hanja (Korean characters) Mode key.
            case KEY_HELP:
                return "Help";
    // "Hiragana" The Hiragana (Japanese Kana characters) key.
            case KEY_HOME:
                return "Home";
            case KEY_INSERT:
                return "Insert";
    // "JapaneseHiragana" The Japanese-Hiragana key.
    // "JapaneseKatakana" The Japanese-Katakana key.
    // "JapaneseRomaji" The Japanese-Romaji key.
    // "JunjaMode" The Junja Mode key.
    // "KanaMode" The Kana Mode (Kana Lock) key.
    // "KanjiMode" The Kanji (Japanese name for ideographic characters of Chinese origin) Mode key.
    // "Katakana" The Katakana (Japanese Kana characters) key.
    // "LaunchApplication1" The Start Application One key.
    // "LaunchApplication2" The Start Application Two key.
    // "LaunchMail" The Start Mail key.
            case KEY_LEFT:
                return "Left";
    // "Meta" The Meta key.
    // "MediaNextTrack" The Media Next Track key.
    // "MediaPlayPause" The Media Play Pause key.
    // "MediaPreviousTrack" The Media Previous Track key.
    // "MediaStop" The Media Stok key.
    // "ModeChange" The Mode Change key.
    // "Nonconvert" The Nonconvert (Don't Convert) key.
    // "NumLock" The Num Lock key.
            case KEY_PAGEDOWN:
                return "PageDown";
            case KEY_PAGEUP:
                return "PageUp";
    // "Paste" The Paste key.
            case KEY_PAUSE:
                return "Pause";
    // "Play" The Play key.
    // "PreviousCandidate" The Previous Candidate function key.
            case KEY_PRINT:
                return "PrintScreen";
    // "Process" The Process key.
    // "Props" The Props key.
            case KEY_RIGHT:
                return "Right";
    // "RomanCharacters" The Roman Characters function key.
    // "Scroll" The Scroll Lock key.
            case KEY_SELECT:
                return "Select";
    // "SelectMedia" The Select Media key.
            case KEY_LEFTSHIFT:
            case KEY_RIGHTSHIFT:
                return "Shift";
            case KEY_STOP:
                return "Stop";
            case KEY_UP:
                return "Up";
    // "Undo" The Undo key.
    // "VolumeDown" The Volume Down key.
    // "VolumeMute" The Volume Mute key.
    // "VolumeUp" The Volume Up key.
    // "Win" The Windows Logo key.
    // "Zoom" The Zoom key.
    // "U+0008" The Backspace (Back) key.
    // "U+0009" The Horizontal Tabulation (Tab) key.
    // "U+0018" The Cancel key.
    // "U+001B" The Escape (Esc) key.
    // "U+0020" The Space (Spacebar) key.
    // "U+0021" The Exclamation Mark (Factorial, Bang) key (!).
    // "U+0022" The Quotation Mark (Quote Double) key (").
    // "U+0023" The Number Sign (Pound Sign, Hash, Crosshatch, Octothorpe) key (#).
    // "U+0024" The Dollar Sign (milreis, escudo) key ($).
    // "U+0026" The Ampersand key (&).
    // "U+0027" The Apostrophe (Apostrophe-Quote, APL Quote) key (').
    // "U+0028" The Left Parenthesis (Opening Parenthesis) key (().
    // "U+0029" The Right Parenthesis (Closing Parenthesis) key ()).
    // "U+002A" The Asterix (Star) key (*).
    // "U+002B" The Plus Sign (Plus) key (+).
    // "U+002C" The Comma (decimal separator) sign key (,).
    // "U+002D" The Hyphen-minus (hyphen or minus sign) key (-).
    // "U+002E" The Full Stop (period, dot, decimal point) key (.).
    // "U+002F" The Solidus (slash, virgule, shilling) key (/).
    // "U+0030" The Digit Zero key (0).
    // "U+0031" The Digit One key (1).
    // "U+0032" The Digit Two key (2).
    // "U+0033" The Digit Three key (3).
    // "U+0034" The Digit Four key (4).
    // "U+0035" The Digit Five key (5).
    // "U+0036" The Digit Six key (6).
    // "U+0037" The Digit Seven key (7).
    // "U+0038" The Digit Eight key (8).
    // "U+0039" The Digit Nine key (9).
    // "U+003A" The Colon key (:).
    // "U+003B" The Semicolon key (;).
    // "U+003C" The Less-Than Sign key (<).
    // "U+003D" The Equals Sign key (=).
    // "U+003E" The Greater-Than Sign key (>).
    // "U+003F" The Question Mark key (?).
    // "U+0040" The Commercial At (@) key.
    // "U+0041" The Latin Capital Letter A key (A).
    // "U+0042" The Latin Capital Letter B key (B).
    // "U+0043" The Latin Capital Letter C key (C).
    // "U+0044" The Latin Capital Letter D key (D).
    // "U+0045" The Latin Capital Letter E key (E).
    // "U+0046" The Latin Capital Letter F key (F).
    // "U+0047" The Latin Capital Letter G key (G).
    // "U+0048" The Latin Capital Letter H key (H).
    // "U+0049" The Latin Capital Letter I key (I).
    // "U+004A" The Latin Capital Letter J key (J).
    // "U+004B" The Latin Capital Letter K key (K).
    // "U+004C" The Latin Capital Letter L key (L).
    // "U+004D" The Latin Capital Letter M key (M).
    // "U+004E" The Latin Capital Letter N key (N).
    // "U+004F" The Latin Capital Letter O key (O).
    // "U+0050" The Latin Capital Letter P key (P).
    // "U+0051" The Latin Capital Letter Q key (Q).
    // "U+0052" The Latin Capital Letter R key (R).
    // "U+0053" The Latin Capital Letter S key (S).
    // "U+0054" The Latin Capital Letter T key (T).
    // "U+0055" The Latin Capital Letter U key (U).
    // "U+0056" The Latin Capital Letter V key (V).
    // "U+0057" The Latin Capital Letter W key (W).
    // "U+0058" The Latin Capital Letter X key (X).
    // "U+0059" The Latin Capital Letter Y key (Y).
    // "U+005A" The Latin Capital Letter Z key (Z).
    // "U+005B" The Left Square Bracket (Opening Square Bracket) key ([).
    // "U+005C" The Reverse Solidus (Backslash) key (\).
    // "U+005D" The Right Square Bracket (Closing Square Bracket) key (]).
    // "U+005E" The Circumflex Accent key (^).
    // "U+005F" The Low Sign (Spacing Underscore, Underscore) key (_).
    // "U+0060" The Grave Accent (Back Quote) key (`).
    // "U+007B" The Left Curly Bracket (Opening Curly Bracket, Opening Brace, Brace Left) key ({).
    // "U+007C" The Vertical Line (Vertical Bar, Pipe) key (|).
    // "U+007D" The Right Curly Bracket (Closing Curly Bracket, Closing Brace, Brace Right) key (}).
    // "U+007F" The Delete (Del) Key.
    // "U+00A1" The Inverted Exclamation Mark key (¡).
    // "U+0300" The Combining Grave Accent (Greek Varia, Dead Grave) key.
    // "U+0301" The Combining Acute Accent (Stress Mark, Greek Oxia, Tonos, Dead Eacute) key.
    // "U+0302" The Combining Circumflex Accent (Hat, Dead Circumflex) key.
    // "U+0303" The Combining Tilde (Dead Tilde) key.
    // "U+0304" The Combining Macron (Long, Dead Macron) key.
    // "U+0306" The Combining Breve (Short, Dead Breve) key.
    // "U+0307" The Combining Dot Above (Derivative, Dead Above Dot) key.
    // "U+0308" The Combining Diaeresis (Double Dot Abode, Umlaut, Greek Dialytika, Double Derivative, Dead Diaeresis) key.
    // "U+030A" The Combining Ring Above (Dead Above Ring) key.
    // "U+030B" The Combining Double Acute Accent (Dead Doubleacute) key.
    // "U+030C" The Combining Caron (Hacek, V Above, Dead Caron) key.
    // "U+0327" The Combining Cedilla (Dead Cedilla) key.
    // "U+0328" The Combining Ogonek (Nasal Hook, Dead Ogonek) key.
    // "U+0345" The Combining Greek Ypogegrammeni (Greek Non-Spacing Iota Below, Iota Subscript, Dead Iota) key.
    // "U+20AC" The Euro Currency Sign key (€).
    // "U+3099" The Combining Katakana-Hiragana Voiced Sound Mark (Dead Voiced Sound) key.
    // "U+309A" The Combining Katakana-Hiragana Semi-Voiced Sound Mark (Dead Semivoiced Sound) key.
            default:
                return nullptr;
        }
    },
    // windows_key_code_for_key_event
    [](struct wpe_input_keyboard_event* event) -> int
    {
        switch (event->keyCode) {
            case KEY_BACKSPACE:
                return VK_BACK;       // (0x08) BACKSPACE key
            case KEY_TAB:
                return VK_TAB;        // (0x09) TAB key
            case KEY_CLEAR:
                return VK_CLEAR;      // (0x0C) CLEAR key
            case KEY_ENTER:
                return VK_RETURN;     // (0x0D) Return key
    // VK_SHIFT (0x10)
    // VK_CONTROL (0x11) // CTRL key
    // VK_MENU (0x12) // ALT key
            case KEY_PAUSE:
                return VK_PAUSE;      // (0x13) PAUSE key
            case KEY_CAPSLOCK:
                return VK_CAPITAL;    // (0x14) CAPS LOCK key
    // VK_KANA (0x15) // Input Method Editor (IME) Kana mode
    // VK_HANGUL (0x15) // IME Hangul mode
    // VK_JUNJA (0x17) // IME Junja mode
    // VK_FINAL (0x18) // IME final mode
    // VK_HANJA (0x19) // IME Hanja mode
    // VK_KANJI (0x19) // IME Kanji mode
            case KEY_ESC:
                return VK_ESCAPE;     // (0x1B) ESC key
    // VK_CONVERT (0x1C) // IME convert
    // VK_NONCONVERT (0x1D) // IME nonconvert
    // VK_ACCEPT (0x1E) // IME accept
    // VK_MODECHANGE (0x1F) // IME mode change request
            case KEY_SPACE:
                return VK_SPACE;      // (0x20) SPACEBAR
            case KEY_PAGEUP:
                return VK_PRIOR;      // (0x21) PAGE UP key
            case KEY_PAGEDOWN:
                return VK_NEXT;       // (0x22) PAGE DOWN key
            case KEY_END:
                return VK_END;        // (0x23) END key
            case KEY_HOME:
                return VK_HOME;       // (0x24) HOME key
            case KEY_LEFT:
                return VK_LEFT;       // (0x25) LEFT ARROW key
            case KEY_UP:
                return VK_UP;         // (0x26) UP ARROW key
            case KEY_RIGHT:
                return VK_RIGHT;      // (0x27) RIGHT ARROW key
            case KEY_DOWN:
                return VK_DOWN;       // (0x28) DOWN ARROW key
            case KEY_SELECT:
                return VK_SELECT;     // (0x29) SELECT key
    // VK_PRINT (0x2A) // PRINT key
    // VK_EXECUTE (0x2B) // EXECUTE key
            case KEY_PRINT:
                return VK_SNAPSHOT;   // (0x2C) PRINT SCREEN key
           case KEY_INSERT:
                return VK_INSERT;     // (0x2D) INS key
            case KEY_DELETE:
                return VK_DELETE;     // (0x2E) DEL key
            case KEY_HELP:
                return VK_HELP;       // (0x2F) HELP key
            case KEY_0:
                return VK_0;          // (0x30) 0 key
            case KEY_1:
                return VK_1;          // (0x31) 1 key
            case KEY_2:
                return VK_2;          // (0x32) 2 key
            case KEY_3:
                return VK_3;          // (0x33) 3 key
            case KEY_4:
                return VK_4;          // (0x34) 4 key
            case KEY_5:
                return VK_5;          // (0x35) 5 key
            case KEY_6:
                return VK_6;          // (0x36) 6 key
            case KEY_7:
                return VK_7;          // (0x37) 7 key
            case KEY_8:
                return VK_8;          // (0x38) 8 key
            case KEY_9:
                return VK_9;          // (0x39) 9 key
            case KEY_A:
                return VK_A;          // (0x41) A key
            case KEY_B:
                return VK_B;          // (0x42) B key
            case KEY_C:
                return VK_C;          // (0x43) C key
            case KEY_D:
                return VK_D;          // (0x44) D key
            case KEY_E:
                return VK_E;          // (0x45) E key
            case KEY_F:
                return VK_F;          // (0x46) F key
            case KEY_G:
                return VK_G;          // (0x47) G key
            case KEY_H:
                return VK_H;          // (0x48) H key
            case KEY_I:
                return VK_I;          // (0x49) I key
            case KEY_J:
                return VK_J;          // (0x4A) J key
            case KEY_K:
                return VK_K;          // (0x4B) K key
            case KEY_L:
                return VK_L;          // (0x4C) L key
            case KEY_M:
                return VK_M;          // (0x4D) M key
            case KEY_N:
                return VK_N;          // (0x4E) N key
            case KEY_O:
                return VK_O;          // (0x4F) O key
            case KEY_P:
                return VK_P;          // (0x50) P key
            case KEY_Q:
                return VK_Q;          // (0x51) Q key
            case KEY_R:
                return VK_R;          // (0x52) R key
            case KEY_S:
                return VK_S;          // (0x53) S key
            case KEY_T:
                return VK_T;          // (0x54) T key
            case KEY_U:
                return VK_U;          // (0x55) U key
            case KEY_V:
                return VK_V;          // (0x56) V key
            case KEY_W:
                return VK_W;          // (0x57) W key
            case KEY_X:
                return VK_X;          // (0x58) X key
            case KEY_Y:
                return VK_Y;          // (0x59) Y key
            case KEY_Z:
                return VK_Z;          // (0x5A) Z key
            case KEY_LEFTMETA:
                return VK_LWIN;       // (0x5B) Left Windows key (Microsoft Natural keyboard)
            case KEY_RIGHTMETA:
                return VK_RWIN;       // (0x5C) Right Windows key (Natural keyboard)
    // VK_APPS (0x5D) // Applications key (Natural keyboard)
    // VK_SLEEP (0x5F) // Computer Sleep key
            case KEY_KP0:
                return VK_NUMPAD0;    // (0x60) Numeric keypad 0 key
            case KEY_KP1:
                return VK_NUMPAD1;    // (0x61) Numeric keypad 1 key
            case KEY_KP2:
                return VK_NUMPAD2;    // (0x62) Numeric keypad 2 key
            case KEY_KP3:
                return VK_NUMPAD3;    // (0x63) Numeric keypad 3 key
            case KEY_KP4:
                return VK_NUMPAD4;    // (0x64) Numeric keypad 4 key
            case KEY_KP5:
                return VK_NUMPAD5;    // (0x65) Numeric keypad 5 key
            case KEY_KP6:
                return VK_NUMPAD6;    // (0x66) Numeric keypad 6 key
            case KEY_KP7:
                return VK_NUMPAD7;    // (0x67) Numeric keypad 7 key
            case KEY_KP8:
                return VK_NUMPAD8;    // (0x68) Numeric keypad 8 key
            case KEY_KP9:
                return VK_NUMPAD9;    // (0x69) Numeric keypad 9 key
            case KEY_KPASTERISK:
                return VK_MULTIPLY;   // (0x6A) Multiply key
            case KEY_KPPLUS:
                return VK_ADD;        // (0x6B) Add key
    // VK_SEPARATOR (0x6C)
            case KEY_KPMINUS:
                return VK_SUBTRACT;   // (0x6D) Subtract key
            case KEY_KPDOT:
                return VK_DECIMAL;    // (0x6E) Decimal key
            case KEY_KPSLASH:
                return VK_DIVIDE;     // (0x6F) Divide key
            case KEY_F1:              // VK_F1 0x70
            case KEY_F2:              // VK_F2 0x71
            case KEY_F3:              // VK_F3 0x72
            case KEY_F4:              // VK_F4 0x73
            case KEY_F5:              // VK_F5 0x74
            case KEY_F6:              // VK_F6 0x75
            case KEY_F7:              // VK_F7 0x76
            case KEY_F8:              // VK_F8 0x77
            case KEY_F9:              // VK_F9 0x78
            case KEY_F10:             // VK_F10 0x79
            case KEY_F11:             // VK_F11 0x7A
            case KEY_F12:             // VK_F12 0x7B
            case KEY_F13:             // VK_F13 0x7C
            case KEY_F14:             // VK_F14 0x7D
            case KEY_F15:             // VK_F15 0x7E
            case KEY_F16:             // VK_F16 0x7F
            case KEY_F17:             // VK_F17 0x80
            case KEY_F18:             // VK_F18 0x81
            case KEY_F19:             // VK_F19 0x82
            case KEY_F20:             // VK_F20 0x83
            case KEY_F21:             // VK_F21 0x84
            case KEY_F22:             // VK_F22 0x85
            case KEY_F23:             // VK_F23 0x86
            case KEY_F24:             // VK_F24 0x87
                return VK_F1 + (event->keyCode - KEY_F1);
            case KEY_NUMLOCK:
                return VK_NUMLOCK;    // (0x90) NUM LOCK key
            case KEY_SCROLLLOCK:
                return VK_SCROLL;     // (0x91) SCROLL LOCK key
            case KEY_LEFTSHIFT:
                return VK_LSHIFT;     // (0xA0) Left SHIFT key
            case KEY_RIGHTSHIFT:
                return VK_RSHIFT;     // (0xA1) Right SHIFT key
            case KEY_LEFTCTRL:
                return VK_LCONTROL;   // (0xA2) Left CONTROL key
            case KEY_RIGHTCTRL:
                return VK_RCONTROL;   // (0xA3) Right CONTROL key
            case KEY_LEFTALT:
                return VK_LMENU;      // (0xA4) Left MENU key
            case KEY_RIGHTALT:
                return VK_RMENU;      // (0xA5) Right MENU key
    // VK_BROWSER_BACK (0xA6) // Windows 2000/XP: Browser Back key
    // VK_BROWSER_FORWARD (0xA7) // Windows 2000/XP: Browser Forward key
    // VK_BROWSER_REFRESH (0xA8) // Windows 2000/XP: Browser Refresh key
    // VK_BROWSER_STOP (0xA9) // Windows 2000/XP: Browser Stop key
    // VK_BROWSER_SEARCH (0xAA) // Windows 2000/XP: Browser Search key
    // VK_BROWSER_FAVORITES (0xAB) // Windows 2000/XP: Browser Favorites key
    // VK_BROWSER_HOME (0xAC) // Windows 2000/XP: Browser Start and Home key
    // VK_VOLUME_MUTE (0xAD) // Windows 2000/XP: Volume Mute key
    // VK_VOLUME_DOWN (0xAE) // Windows 2000/XP: Volume Down key
    // VK_VOLUME_UP (0xAF) // Windows 2000/XP: Volume Up key
            case KEY_FORWARD:
                return VK_MEDIA_NEXT_TRACK;
            case KEY_REWIND:
                return VK_MEDIA_PREV_TRACK;
            case KEY_STOP:
                return VK_MEDIA_STOP;
            case KEY_PLAYPAUSE:
                return VK_MEDIA_PLAY_PAUSE;
    // VK_MEDIA_LAUNCH_MAIL (0xB4) // Windows 2000/XP: Start Mail key
    // VK_MEDIA_LAUNCH_MEDIA_SELECT (0xB5) // Windows 2000/XP: Select Media key
    //        case KEY_EPG:
    //            return VK_MEDIA_LAUNCH_APP1;
    //        case KEY_APPSELECT:
    //            return VK_MEDIA_LAUNCH_APP2;
            case KEY_SEMICOLON:
                return VK_OEM_1;      // (0xBA) Used for miscellaneous characters; it can vary by keyboard. Windows 2000/XP: For the US standard keyboard, the ';:' key
            case KEY_EQUAL:
                return VK_OEM_PLUS;   // (0xBB) Windows 2000/XP: For any country/region, the '+' key
            case KEY_COMMA:
                return VK_OEM_COMMA;  // (0xBC) Windows 2000/XP: For any country/region, the ',' key
            case KEY_MINUS:
                return  VK_OEM_MINUS; // (0xBD) Windows 2000/XP: For any country/region, the '-' key
            case KEY_DOT:
                return VK_OEM_PERIOD; // (0xBE) Windows 2000/XP: For any country/region, the '.' key
            case KEY_SLASH:
                return VK_OEM_2;      // (0xBF) Used for miscellaneous characters; it can vary by keyboard. Windows 2000/XP: For the US standard keyboard, the '/?' key
    // VK_OEM_2 (0xBF)
    // VK_OEM_3 (C0) Used for miscellaneous characters; it can vary by keyboard. Windows 2000/XP: For the US standard keyboard, the '`~' key
    // VK_OEM_3 (0xC0)
            case KEY_LEFTBRACE:
                return VK_OEM_4;      // (0xDB) Used for miscellaneous characters; it can vary by keyboard. Windows 2000/XP: For the US standard keyboard, the '[{' key
            case KEY_BACKSLASH:
            return VK_OEM_5;      // (0xDC) Used for miscellaneous characters; it can vary by keyboard. Windows 2000/XP: For the US standard keyboard, the '\|' key
            case KEY_RIGHTBRACE:
                return VK_OEM_6;      // (0xDD) Used for miscellaneous characters; it can vary by keyboard. Windows 2000/XP: For the US standard keyboard, the ']}' key
    // VK_OEM_7 (0xDE) Used for miscellaneous characters; it can vary by keyboard. Windows 2000/XP: For the US standard keyboard, the 'single-quote/double-quote' key
    // VK_OEM_8 (0xDF) Used for miscellaneous characters; it can vary by keyboard.
    // VK_OEM_102 (0xE2) Windows 2000/XP: Either the angle bracket key or the backslash key on the RT 102-key keyboard
    // VK_PROCESSKEY (0xE5)  Windows 95/98/Me, Windows NT 4.0, Windows 2000/XP: IME PROCESS key
    // VK_PACKET (0xE7)  Windows 2000/XP: Used to pass Unicode characters as if they were keystrokes. The VK_PACKET key is the low word of a 32-bit Virtual Key value used for non-keyboard input methods. For more information, see Remark in KEYBDINPUT,SendInput, WM_KEYDOWN, and WM_KEYUP
    // VK_ATTN (0xF6) // Attn key
    // VK_CRSEL (0xF7) // CrSel key
    // VK_EXSEL (0xF8) // ExSel key
    // VK_EREOF (0xF9) // Erase EOF key
    // VK_PLAY (0xFA) // Play key
    // VK_ZOOM (0xFB) // Zoom key
    // VK_NONAME (0xFC) // Reserved for future use
    // VK_PA1 (0xFD) // VK_PA1 (FD) PA1 key
    // VK_OEM_CLEAR (0xFE) // Clear key
            default:
                return 0;
        }
    },
    // single_character_for_key_event
    [](struct wpe_input_keyboard_event* event) -> const char*
    {
        switch (event->keyCode) {
            case KEY_KPENTER:
            case KEY_ENTER:
                return "\r";
            case KEY_BACKSPACE:
                return "\x8";
            case KEY_TAB:
                return "\t";
            default:
                return nullptr;
        }
    },
};

}

#endif // KEY_INPUT_HANDLING_LINUX_INPUT
