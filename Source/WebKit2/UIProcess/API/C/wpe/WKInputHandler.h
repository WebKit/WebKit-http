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

#ifndef WKInputHandler_h
#define WKInputHandler_h

#include <WebKit/WKBase.h>

#ifdef __cplusplus
extern "C" {
#endif

struct WKKeyboardKey {
    uint32_t time;
    uint32_t key;
    uint32_t state;
};
typedef struct WKKeyboardKey WKKeyboardKey;

struct WKKeyboardModifiers {
    uint32_t serial;
    uint32_t mods_depressed;
    uint32_t mods_latched;
    uint32_t mods_locked;
    uint32_t group;
};
typedef struct WKKeyboardModifiers WKKeyboardModifiers;

struct WKPointerMotion {
    uint32_t time;
    int32_t x;
    int32_t y;
};
typedef struct WKPointerMotion WKPointerMotion;

struct WKPointerButton {
    uint32_t time;
    uint32_t button;
    uint32_t state;
};
typedef struct WKPointerButton WKPointerButton;

struct WKTouchDown {
    uint32_t time;
    int id;
    int32_t x;
    int32_t y;
};
typedef struct WKTouchDown WKTouchDown;

struct WKTouchUp {
    uint32_t time;
    int id;
};
typedef struct WKTouchUp WKTouchUp;

struct WKTouchMotion {
    uint32_t time;
    int id;
    int32_t x;
    int32_t y;
};
typedef struct WKTouchMotion WKTouchMotion;

WK_EXPORT WKInputHandlerRef WKInputHandlerCreate(WKViewRef);

WK_EXPORT void WKInputHandlerNotifyKeyboardKey(WKInputHandlerRef, WKKeyboardKey);

WK_EXPORT void WKInputHandlerNotifyPointerMotion(WKInputHandlerRef, WKPointerMotion);
WK_EXPORT void WKInputHandlerNotifyPointerButton(WKInputHandlerRef, WKPointerButton);

WK_EXPORT void WKInputHandlerNotifyTouchDown(WKInputHandlerRef, WKTouchDown);
WK_EXPORT void WKInputHandlerNotifyTouchUp(WKInputHandlerRef, WKTouchUp);
WK_EXPORT void WKInputHandlerNotifyTouchMotion(WKInputHandlerRef, WKTouchMotion);

#ifdef __cplusplus
}
#endif

#endif // WKInputHandler_h
