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

#include "input-libxkbcommon.h"

#ifdef KEY_INPUT_HANDLING_XKB

#include "WindowsKeyboardCodes.h"
#include "TvKeyboardCodes.h"
#include <xkbcommon/xkbcommon-keysyms.h>

extern "C" {

struct wpe_input_key_mapper_interface libxkbcommon_input_key_mapper_interface = {
    // identifier_for_key_event
    [](struct wpe_input_keyboard_event* event) -> const char*
    {
        switch (event->keyCode) {
            case XKB_KEY_Menu:
            case XKB_KEY_Alt_L:
            case XKB_KEY_Alt_R:
                return "Alt";
            case XKB_KEY_Clear:
                return "Clear";
            case XKB_KEY_Down:
                return "Down";
            case XKB_KEY_End:
                return "End";
            case XKB_KEY_ISO_Enter:
            case XKB_KEY_KP_Enter:
            case XKB_KEY_Return:
                return "Enter";
            case XKB_KEY_Execute:
                return "Execute";
            case XKB_KEY_F1:
                return "F1";
            case XKB_KEY_F2:
                return "F2";
            case XKB_KEY_F3:
                return "F3";
            case XKB_KEY_F4:
                return "F4";
            case XKB_KEY_F5:
                return "F5";
            case XKB_KEY_F6:
                return "F6";
            case XKB_KEY_F7:
                return "F7";
            case XKB_KEY_F8:
                return "F8";
            case XKB_KEY_F9:
                return "F9";
            case XKB_KEY_F10:
                return "F10";
            case XKB_KEY_F11:
                return "F11";
            case XKB_KEY_F12:
                return "F12";
            case XKB_KEY_F13:
                return "F13";
            case XKB_KEY_F14:
                return "F14";
            case XKB_KEY_F15:
                return "F15";
            case XKB_KEY_F16:
                return "F16";
            case XKB_KEY_F17:
                return "F17";
            case XKB_KEY_F18:
                return "F18";
            case XKB_KEY_F19:
                return "F19";
            case XKB_KEY_F20:
                return "F20";
            case XKB_KEY_F21:
                return "F21";
            case XKB_KEY_F22:
                return "F22";
            case XKB_KEY_F23:
                return "F23";
            case XKB_KEY_F24:
                return "F24";
            case XKB_KEY_Help:
                return "Help";
            case XKB_KEY_Home:
                return "Home";
            case XKB_KEY_Insert:
                return "Insert";
            case XKB_KEY_Left:
                return "Left";
            case XKB_KEY_Page_Down:
                return "PageDown";
            case XKB_KEY_Page_Up:
                return "PageUp";
            case XKB_KEY_Pause:
                return "Pause";
            case XKB_KEY_3270_PrintScreen:
            case XKB_KEY_Print:
                return "PrintScreen";
            case XKB_KEY_Right:
                return "Right";
            case XKB_KEY_Select:
                return "Select";
            case XKB_KEY_Up:
                return "Up";
                // Standard says that DEL becomes U+007F.
            case XKB_KEY_Delete:
                return "U+007F";
            case XKB_KEY_BackSpace:
                return "U+0008";
            case XKB_KEY_ISO_Left_Tab:
            case XKB_KEY_3270_BackTab:
            case XKB_KEY_Tab:
                return "U+0009";

            // UI Keys
            case XKB_KEY_Cancel:
                return "Cancel";

            // Device Keys
            case XKB_KEY_XF86PowerOff:
                return "Power"; // the spec also mentions "PowerOff"
            case XKB_KEY_XF86Sleep:
                return "Standby";

            // Multimedia Keys
            case XKB_KEY_XF86AudioForward:
                return "MediaFastForward";
            case XKB_KEY_XF86AudioPause:
                return "MediaPause";
            case XKB_KEY_XF86AudioPlay:
                return "MediaPlay";
            case XKB_KEY_XF86AudioRecord:
                return "MediaRecord";
            case XKB_KEY_XF86AudioRewind:
                return "MediaRewind";
            case XKB_KEY_XF86AudioStop:
                return "MediaStop";
            case XKB_KEY_XF86AudioPrev:
                return "MediaTrackPrevious";
            case XKB_KEY_XF86AudioNext:
                return "MediaTrackNext";

            // Audio Keys
            case XKB_KEY_XF86AudioLowerVolume:
                return "AudioVolumeDown";
            case XKB_KEY_XF86AudioRaiseVolume:
                return "AudioVolumeUp";
            case XKB_KEY_XF86AudioMute:
                return "AudioVolumeMute";

            // Application Keys
            case XKB_KEY_XF86AudioMedia:
                return "LaunchMediaPlayer";

            // Browser Keys
            case XKB_KEY_XF86Back:
                return "BrowserBack";
            case XKB_KEY_XF86Favorites:
                return "BrowserFavorites";
            case XKB_KEY_XF86Forward:
                return "BrowserForward";
            case XKB_KEY_XF86HomePage:
                return "BrowserHome";
            case XKB_KEY_XF86Refresh:
                return "BrowserRefresh";
            case XKB_KEY_XF86Search:
                return "BrowserSearch";
            case XKB_KEY_XF86Stop:
                return "BrowserStop";

            // Media Controller Keys
            case XKB_KEY_XF86Red:
                return "ColorF0Red";
            case XKB_KEY_XF86Green:
                return "ColorF1Green";
            case XKB_KEY_XF86Yellow:
                return "ColorF2Yellow";
            case XKB_KEY_XF86Blue:
                return "ColorF3Blue";
            case XKB_KEY_XF86Display:
                return "DisplaySwap";
            case XKB_KEY_XF86Video:
                return "OnDemand";
            case XKB_KEY_XF86Subtitle:
                return "Subtitle";

            default:
                return nullptr;
        }
    },
    // windows_key_code_for_key_event
    [](struct wpe_input_keyboard_event* event) -> int
    {
        switch (event->keyCode) {
            case XKB_KEY_KP_0:
                return VK_NUMPAD0;// (60) Numeric keypad 0 key
            case XKB_KEY_KP_1:
                return VK_NUMPAD1;// (61) Numeric keypad 1 key
            case XKB_KEY_KP_2:
                return  VK_NUMPAD2; // (62) Numeric keypad 2 key
            case XKB_KEY_KP_3:
                return VK_NUMPAD3; // (63) Numeric keypad 3 key
            case XKB_KEY_KP_4:
                return VK_NUMPAD4; // (64) Numeric keypad 4 key
            case XKB_KEY_KP_5:
                return VK_NUMPAD5; //(65) Numeric keypad 5 key
            case XKB_KEY_KP_6:
                return VK_NUMPAD6; // (66) Numeric keypad 6 key
            case XKB_KEY_KP_7:
                return VK_NUMPAD7; // (67) Numeric keypad 7 key
            case XKB_KEY_KP_8:
                return VK_NUMPAD8; // (68) Numeric keypad 8 key
            case XKB_KEY_KP_9:
                return VK_NUMPAD9; // (69) Numeric keypad 9 key
            case XKB_KEY_KP_Multiply:
                return VK_MULTIPLY; // (6A) Multiply key
            case XKB_KEY_KP_Add:
                return VK_ADD; // (6B) Add key
            case XKB_KEY_KP_Subtract:
                return VK_SUBTRACT; // (6D) Subtract key
            case XKB_KEY_KP_Decimal:
                return VK_DECIMAL; // (6E) Decimal key
            case XKB_KEY_KP_Divide:
                return VK_DIVIDE; // (6F) Divide key

            case XKB_KEY_KP_Page_Up:
                return VK_PRIOR; // (21) PAGE UP key
            case XKB_KEY_KP_Page_Down:
                return VK_NEXT; // (22) PAGE DOWN key
            case XKB_KEY_KP_End:
                return VK_END; // (23) END key
            case XKB_KEY_KP_Home:
                return VK_HOME; // (24) HOME key
            case XKB_KEY_KP_Left:
                return VK_LEFT; // (25) LEFT ARROW key
            case XKB_KEY_KP_Up:
                return VK_UP; // (26) UP ARROW key
            case XKB_KEY_KP_Right:
                return VK_RIGHT; // (27) RIGHT ARROW key
            case XKB_KEY_KP_Down:
                return VK_DOWN; // (28) DOWN ARROW key

            case XKB_KEY_BackSpace:
                return VK_BACK; // (08) BACKSPACE key
            case XKB_KEY_ISO_Left_Tab:
            case XKB_KEY_3270_BackTab:
            case XKB_KEY_Tab:
                return VK_TAB; // (09) TAB key
            case XKB_KEY_Clear:
                return VK_CLEAR; // (0C) CLEAR key
            case XKB_KEY_ISO_Enter:
            case XKB_KEY_KP_Enter:
            case XKB_KEY_Return:
                return VK_RETURN; //(0D) Return key

                // VK_SHIFT (10) SHIFT key
                // VK_CONTROL (11) CTRL key

            case XKB_KEY_Menu:
                return VK_APPS;  // (5D) Applications key (Natural keyboard)

                // VK_MENU (12) ALT key

            case XKB_KEY_Pause:
                return VK_PAUSE; // (13) PAUSE key
            case XKB_KEY_XF86AudioPause:
                return VK_MEDIA_PLAY_PAUSE; // (B3) Windows 2000/XP: Play/Pause Media key
            case XKB_KEY_Caps_Lock:
                return VK_CAPITAL; // (14) CAPS LOCK key
            case XKB_KEY_Kana_Lock:
            case XKB_KEY_Kana_Shift:
                return VK_KANA; // (15) Input Method Editor (IME) Kana mode
            case XKB_KEY_Hangul:
                return VK_HANGUL; // VK_HANGUL (15) IME Hangul mode
                // VK_JUNJA (17) IME Junja mode
                // VK_FINAL (18) IME final mode
            case XKB_KEY_Hangul_Hanja:
                return VK_HANJA; // (19) IME Hanja mode
            case XKB_KEY_Kanji:
                return VK_KANJI; // (19) IME Kanji mode
            case XKB_KEY_Escape:
                return VK_ESCAPE; // (1B) ESC key
                // VK_CONVERT (1C) IME convert
                // VK_NONCONVERT (1D) IME nonconvert
                // VK_ACCEPT (1E) IME accept
                // VK_MODECHANGE (1F) IME mode change request
            case XKB_KEY_space:
                return VK_SPACE; // (20) SPACEBAR
            case XKB_KEY_Page_Up:
                return VK_PRIOR; // (21) PAGE UP key
            case XKB_KEY_Page_Down:
                return VK_NEXT; // (22) PAGE DOWN key
            case XKB_KEY_End:
                return VK_END; // (23) END key
            case XKB_KEY_Home:
                return VK_HOME; // (24) HOME key
            case XKB_KEY_Left:
                return VK_LEFT; // (25) LEFT ARROW key
            case XKB_KEY_Up:
                return VK_UP; // (26) UP ARROW key
            case XKB_KEY_Right:
                return VK_RIGHT; // (27) RIGHT ARROW key
            case XKB_KEY_Down:
                return VK_DOWN; // (28) DOWN ARROW key
            case XKB_KEY_Select:
                return VK_SELECT; // (29) SELECT key
            case XKB_KEY_Print:
                return VK_SNAPSHOT; // (2C) PRINT SCREEN key
            case XKB_KEY_Execute:
                return VK_EXECUTE;// (2B) EXECUTE key
            case XKB_KEY_Insert:
            case XKB_KEY_KP_Insert:
                return VK_INSERT; // (2D) INS key
            case XKB_KEY_Delete:
            case XKB_KEY_KP_Delete:
                return VK_DELETE; // (2E) DEL key
            case XKB_KEY_Help:
                return VK_HELP; // (2F) HELP key
            case XKB_KEY_0:
            case XKB_KEY_parenright:
                return VK_0;    //  (30) 0) key
            case XKB_KEY_1:
            case XKB_KEY_exclam:
                return VK_1; //  (31) 1 ! key
            case XKB_KEY_2:
            case XKB_KEY_at:
                return VK_2; //  (32) 2 & key
            case XKB_KEY_3:
            case XKB_KEY_numbersign:
                return VK_3; //case '3': case '#';
            case XKB_KEY_4:
            case XKB_KEY_dollar: //  (34) 4 key '$';
                return VK_4;
            case XKB_KEY_5:
            case XKB_KEY_percent:
                return VK_5; //  (35) 5 key  '%'
            case XKB_KEY_6:
            case XKB_KEY_asciicircum:
                return VK_6; //  (36) 6 key  '^'
            case XKB_KEY_7:
            case XKB_KEY_ampersand:
                return VK_7; //  (37) 7 key  case '&'
            case XKB_KEY_8:
            case XKB_KEY_asterisk:
                return VK_8; //  (38) 8 key  '*'
            case XKB_KEY_9:
            case XKB_KEY_parenleft:
                return VK_9; //  (39) 9 key '('
            case XKB_KEY_a:
            case XKB_KEY_A:
                return VK_A; //  (41) A key case 'a': case 'A': return 0x41;
            case XKB_KEY_b:
            case XKB_KEY_B:
                return VK_B; //  (42) B key case 'b': case 'B': return 0x42;
            case XKB_KEY_c:
            case XKB_KEY_C:
                return VK_C; //  (43) C key case 'c': case 'C': return 0x43;
            case XKB_KEY_d:
            case XKB_KEY_D:
                return VK_D; //  (44) D key case 'd': case 'D': return 0x44;
            case XKB_KEY_e:
            case XKB_KEY_E:
                return VK_E; //  (45) E key case 'e': case 'E': return 0x45;
            case XKB_KEY_f:
            case XKB_KEY_F:
                return VK_F; //  (46) F key case 'f': case 'F': return 0x46;
            case XKB_KEY_g:
            case XKB_KEY_G:
                return VK_G; //  (47) G key case 'g': case 'G': return 0x47;
            case XKB_KEY_h:
            case XKB_KEY_H:
                return VK_H; //  (48) H key case 'h': case 'H': return 0x48;
            case XKB_KEY_i:
            case XKB_KEY_I:
                return VK_I; //  (49) I key case 'i': case 'I': return 0x49;
            case XKB_KEY_j:
            case XKB_KEY_J:
                return VK_J; //  (4A) J key case 'j': case 'J': return 0x4A;
            case XKB_KEY_k:
            case XKB_KEY_K:
                return VK_K; //  (4B) K key case 'k': case 'K': return 0x4B;
            case XKB_KEY_l:
            case XKB_KEY_L:
                return VK_L; //  (4C) L key case 'l': case 'L': return 0x4C;
            case XKB_KEY_m:
            case XKB_KEY_M:
                return VK_M; //  (4D) M key case 'm': case 'M': return 0x4D;
            case XKB_KEY_n:
            case XKB_KEY_N:
                return VK_N; //  (4E) N key case 'n': case 'N': return 0x4E;
            case XKB_KEY_o:
            case XKB_KEY_O:
                return VK_O; //  (4F) O key case 'o': case 'O': return 0x4F;
            case XKB_KEY_p:
            case XKB_KEY_P:
                return VK_P; //  (50) P key case 'p': case 'P': return 0x50;
            case XKB_KEY_q:
            case XKB_KEY_Q:
                return VK_Q; //  (51) Q key case 'q': case 'Q': return 0x51;
            case XKB_KEY_r:
            case XKB_KEY_R:
                return VK_R; //  (52) R key case 'r': case 'R': return 0x52;
            case XKB_KEY_s:
            case XKB_KEY_S:
                return VK_S; //  (53) S key case 's': case 'S': return 0x53;
            case XKB_KEY_t:
            case XKB_KEY_T:
                return VK_T; //  (54) T key case 't': case 'T': return 0x54;
            case XKB_KEY_u:
            case XKB_KEY_U:
                return VK_U; //  (55) U key case 'u': case 'U': return 0x55;
            case XKB_KEY_v:
            case XKB_KEY_V:
                return VK_V; //  (56) V key case 'v': case 'V': return 0x56;
            case XKB_KEY_w:
            case XKB_KEY_W:
                return VK_W; //  (57) W key case 'w': case 'W': return 0x57;
            case XKB_KEY_x:
            case XKB_KEY_X:
                return VK_X; //  (58) X key case 'x': case 'X': return 0x58;
            case XKB_KEY_y:
            case XKB_KEY_Y:
                return VK_Y; //  (59) Y key case 'y': case 'Y': return 0x59;
            case XKB_KEY_z:
            case XKB_KEY_Z:
                return VK_Z; //  (5A) Z key case 'z': case 'Z': return 0x5A;
            case XKB_KEY_Meta_L:
                return VK_LWIN; // (5B) Left Windows key (Microsoft Natural keyboard)
            case XKB_KEY_Meta_R:
                return VK_RWIN; // (5C) Right Windows key (Natural keyboard)
            case XKB_KEY_XF86Sleep:
                return VK_SLEEP; // (5F) Computer Sleep key
                // VK_SEPARATOR (6C) Separator key
                // VK_SUBTRACT (6D) Subtract key
                // VK_DECIMAL (6E) Decimal key
                // VK_DIVIDE (6F) Divide key
                // handled by key code above

            case XKB_KEY_Num_Lock:
                return VK_NUMLOCK; // (90) NUM LOCK key

            case XKB_KEY_Scroll_Lock:
                return VK_SCROLL; // (91) SCROLL LOCK key

            case XKB_KEY_Shift_L:
                return VK_LSHIFT; // (A0) Left SHIFT key
            case XKB_KEY_Shift_R:
                return VK_RSHIFT; // (A1) Right SHIFT key
            case XKB_KEY_Control_L:
                return VK_LCONTROL; // (A2) Left CONTROL key
            case XKB_KEY_Control_R:
                return VK_RCONTROL; // (A3) Right CONTROL key
            case XKB_KEY_Alt_L:
                return VK_LMENU; // (A4) Left MENU key
            case XKB_KEY_Alt_R:
                return VK_RMENU; // (A5) Right MENU key

            case XKB_KEY_XF86Back:
                return VK_BROWSER_BACK; // VK_BROWSER_BACK (A6) Windows 2000/XP: Browser Back key
            case XKB_KEY_XF86Forward:
                return VK_BROWSER_FORWARD; // (A7) Windows 2000/XP: Browser Forward key
            case XKB_KEY_XF86Refresh:
                return VK_BROWSER_REFRESH; // (A8) Windows 2000/XP: Browser Refresh key
            case XKB_KEY_XF86Stop:
                return VK_BROWSER_STOP; // (A9) Windows 2000/XP: Browser Stop key
            case XKB_KEY_XF86Search:
                return VK_BROWSER_SEARCH; // (AA) Windows 2000/XP: Browser Search key
            case XKB_KEY_XF86Favorites:
                return VK_BROWSER_FAVORITES; // (AB) Windows 2000/XP: Browser Favorites key
            case XKB_KEY_XF86HomePage:
                return VK_BROWSER_HOME; // (AC) Windows 2000/XP: Browser Start and Home key
            case XKB_KEY_XF86AudioMute:
                return VK_VOLUME_MUTE; // (AD) Windows 2000/XP: Volume Mute key
            case XKB_KEY_XF86AudioLowerVolume:
                return VK_VOLUME_DOWN; // (AE) Windows 2000/XP: Volume Down key
            case XKB_KEY_XF86AudioRaiseVolume:
                return VK_VOLUME_UP; // (AF) Windows 2000/XP: Volume Up key
            case XKB_KEY_XF86AudioNext:
                return VK_MEDIA_NEXT_TRACK; // (B0) Windows 2000/XP: Next Track key
            case XKB_KEY_XF86AudioPrev:
                return VK_MEDIA_PREV_TRACK; // (B1) Windows 2000/XP: Previous Track key
            case XKB_KEY_XF86AudioStop:
                return VK_MEDIA_STOP; // (B2) Windows 2000/XP: Stop Media key
                // VK_MEDIA_PLAY_PAUSE (B3) Windows 2000/XP: Play/Pause Media key
                // VK_LAUNCH_MAIL (B4) Windows 2000/XP: Start Mail key
            case XKB_KEY_XF86AudioMedia:
                return VK_MEDIA_LAUNCH_MEDIA_SELECT; // (B5) Windows 2000/XP: Select Media key
                // VK_LAUNCH_APP1 (B6) Windows 2000/XP: Start Application 1 key
                // VK_LAUNCH_APP2 (B7) Windows 2000/XP: Start Application 2 key

                // VK_OEM_1 (BA) Used for miscellaneous characters; it can vary by keyboard. Windows 2000/XP: For the US standard keyboard, the ';:' key
            case XKB_KEY_semicolon:
            case XKB_KEY_colon:
                return VK_OEM_1; //case ';': case ':': return 0xBA;
                // VK_OEM_PLUS (BB) Windows 2000/XP: For any country/region, the '+' key
            case XKB_KEY_plus:
            case XKB_KEY_equal:
                return VK_OEM_PLUS; //case '=': case '+': return 0xBB;
                // VK_OEM_COMMA (BC) Windows 2000/XP: For any country/region, the ',' key
            case XKB_KEY_comma:
            case XKB_KEY_less:
                return VK_OEM_COMMA; //case ',': case '<': return 0xBC;
                // VK_OEM_MINUS (BD) Windows 2000/XP: For any country/region, the '-' key
            case XKB_KEY_minus:
            case XKB_KEY_underscore:
                return VK_OEM_MINUS; //case '-': case '_': return 0xBD;
                // VK_OEM_PERIOD (BE) Windows 2000/XP: For any country/region, the '.' key
            case XKB_KEY_period:
            case XKB_KEY_greater:
                return VK_OEM_PERIOD; //case '.': case '>': return 0xBE;
                // VK_OEM_2 (BF) Used for miscellaneous characters; it can vary by keyboard. Windows 2000/XP: For the US standard keyboard, the '/?' key
            case XKB_KEY_slash:
            case XKB_KEY_question:
                return VK_OEM_2; //case '/': case '?': return 0xBF;
                // VK_OEM_3 (C0) Used for miscellaneous characters; it can vary by keyboard. Windows 2000/XP: For the US standard keyboard, the '`~' key
            case XKB_KEY_asciitilde:
            case XKB_KEY_quoteleft:
                return VK_OEM_3; //case '`': case '~': return 0xC0;
                // VK_OEM_4 (DB) Used for miscellaneous characters; it can vary by keyboard. Windows 2000/XP: For the US standard keyboard, the '[{' key
            case XKB_KEY_bracketleft:
            case XKB_KEY_braceleft:
                return VK_OEM_4; //case '[': case '{': return 0xDB;
                // VK_OEM_5 (DC) Used for miscellaneous characters; it can vary by keyboard. Windows 2000/XP: For the US standard keyboard, the '\|' key
            case XKB_KEY_backslash:
            case XKB_KEY_bar:
                return VK_OEM_5; //case '\\': case '|': return 0xDC;
                // VK_OEM_6 (DD) Used for miscellaneous characters; it can vary by keyboard. Windows 2000/XP: For the US standard keyboard, the ']}' key
            case XKB_KEY_bracketright:
            case XKB_KEY_braceright:
                return VK_OEM_6; // case ']': case '}': return 0xDD;
                // VK_OEM_7 (DE) Used for miscellaneous characters; it can vary by keyboard. Windows 2000/XP: For the US standard keyboard, the 'single-quote/double-quote' key
            case XKB_KEY_quoteright:
            case XKB_KEY_quotedbl:
                return VK_OEM_7; // case '\'': case '"': return 0xDE;
                // VK_OEM_8 (DF) Used for miscellaneous characters; it can vary by keyboard.
                // VK_OEM_102 (E2) Windows 2000/XP: Either the angle bracket key or the backslash key on the RT 102-key keyboard
            case XKB_KEY_XF86AudioRewind:
                return 0xE3; // (E3) Android/GoogleTV: Rewind media key (Windows: VK_ICO_HELP Help key on 1984 Olivetti M24 deluxe keyboard)
            case XKB_KEY_XF86AudioForward:
                return 0xE4; // (E4) Android/GoogleTV: Fast forward media key  (Windows: VK_ICO_00 '00' key on 1984 Olivetti M24 deluxe keyboard)
                // VK_PROCESSKEY (E5) Windows 95/98/Me, Windows NT 4.0, Windows 2000/XP: IME PROCESS key
                // VK_PACKET (E7) Windows 2000/XP: Used to pass Unicode characters as if they were keystrokes. The VK_PACKET key is the low word of a 32-bit Virtual Key value used for non-keyboard input methods. For more information, see Remark in KEYBDINPUT,SendInput, WM_KEYDOWN, and WM_KEYUP
                // VK_ATTN (F6) Attn key
                // VK_CRSEL (F7) CrSel key
                // VK_EXSEL (F8) ExSel key
                // VK_EREOF (F9) Erase EOF key
            case XKB_KEY_XF86AudioPlay:
                return VK_MEDIA_PLAY_PAUSE; // (B3) Windows 2000/XP: Play/Pause Media key
                // VK_ZOOM (FB) Zoom key
                // VK_NONAME (FC) Reserved for future use
                // VK_PA1 (FD) PA1 key
                // VK_OEM_CLEAR (FE) Clear key
            case XKB_KEY_F1:
            case XKB_KEY_F2:
            case XKB_KEY_F3:
            case XKB_KEY_F4:
            case XKB_KEY_F5:
            case XKB_KEY_F6:
            case XKB_KEY_F7:
            case XKB_KEY_F8:
            case XKB_KEY_F9:
            case XKB_KEY_F10:
            case XKB_KEY_F11:
            case XKB_KEY_F12:
            case XKB_KEY_F13:
            case XKB_KEY_F14:
            case XKB_KEY_F15:
            case XKB_KEY_F16:
            case XKB_KEY_F17:
            case XKB_KEY_F18:
            case XKB_KEY_F19:
            case XKB_KEY_F20:
            case XKB_KEY_F21:
            case XKB_KEY_F22:
            case XKB_KEY_F23:
            case XKB_KEY_F24:
                return VK_F1 + (event->keyCode - XKB_KEY_F1);
            case XKB_KEY_VoidSymbol:
                return VK_PROCESSKEY;

            // TV keycodes from HAVi / DASE / OCAP / CE-HTML standards
            case XKB_KEY_Cancel:
                return VK_DASE_CANCEL;
            case XKB_KEY_XF86Red:
                return VK_HAVI_COLORED_KEY_0;
            case XKB_KEY_XF86Green:
                return VK_HAVI_COLORED_KEY_1;
            case XKB_KEY_XF86Yellow:
                return VK_HAVI_COLORED_KEY_2;
            case XKB_KEY_XF86Blue:
                return VK_HAVI_COLORED_KEY_3;
            case XKB_KEY_XF86PowerOff:
                return VK_HAVI_POWER;
            case XKB_KEY_XF86AudioRecord:
                return VK_HAVI_RECORD;
            case XKB_KEY_XF86Display:
                return VK_HAVI_DISPLAY_SWAP;
            case XKB_KEY_XF86Subtitle:
                return VK_HAVI_SUBTITLE;
            case XKB_KEY_XF86Video:
                return VK_OCAP_ON_DEMAND;

            default:
                return 0;
        }
    },
    // single_character_for_key_event
    [](struct wpe_input_keyboard_event* event) -> const char*
    {
        switch (event->keyCode) {
            case XKB_KEY_ISO_Enter:
            case XKB_KEY_KP_Enter:
            case XKB_KEY_Return:
                return "\r";
            case XKB_KEY_BackSpace:
                return "\x8";
            case XKB_KEY_Tab:
                return "\t";
            default:
                return nullptr;
        }
    },
};

}

#endif // KEY_INPUT_HANDLING_XKB
