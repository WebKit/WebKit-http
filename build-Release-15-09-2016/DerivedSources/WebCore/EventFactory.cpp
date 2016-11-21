/*
 * THIS FILE WAS AUTOMATICALLY GENERATED, DO NOT EDIT.
 *
 * Copyright (C) 2011 Google Inc.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY GOOGLE, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "EventFactory.h"

#include "EventHeaders.h"
#include <runtime/StructureInlines.h>

namespace WebCore {

PassRefPtr<Event> EventFactory::create(const String& type)
{
    if (equalIgnoringASCIICase(type, "AnimationEvent"))
        return AnimationEvent::create();
#if ENABLE(WEB_AUDIO)
    if (equalIgnoringASCIICase(type, "AudioProcessingEvent"))
        return AudioProcessingEvent::create();
#endif
#if ENABLE(REQUEST_AUTOCOMPLETE)
    if (equalIgnoringASCIICase(type, "AutocompleteErrorEvent"))
        return AutocompleteErrorEvent::create();
#endif
    if (equalIgnoringASCIICase(type, "BeforeLoadEvent"))
        return BeforeLoadEvent::create();
    if (equalIgnoringASCIICase(type, "BeforeUnloadEvent"))
        return BeforeUnloadEvent::create();
#if ENABLE(FONT_LOAD_EVENTS)
    if (equalIgnoringASCIICase(type, "CSSFontFaceLoadEvent"))
        return CSSFontFaceLoadEvent::create();
#endif
    if (equalIgnoringASCIICase(type, "ClipboardEvent"))
        return ClipboardEvent::create();
    if (equalIgnoringASCIICase(type, "CloseEvent"))
        return CloseEvent::create();
    if (equalIgnoringASCIICase(type, "CompositionEvent"))
        return CompositionEvent::create();
    if (equalIgnoringASCIICase(type, "CustomEvent"))
        return CustomEvent::create();
#if ENABLE(DEVICE_ORIENTATION)
    if (equalIgnoringASCIICase(type, "DeviceMotionEvent"))
        return DeviceMotionEvent::create();
#endif
#if ENABLE(DEVICE_ORIENTATION)
    if (equalIgnoringASCIICase(type, "DeviceOrientationEvent"))
        return DeviceOrientationEvent::create();
#endif
#if ENABLE(PROXIMITY_EVENTS)
    if (equalIgnoringASCIICase(type, "DeviceProximityEvent"))
        return DeviceProximityEvent::create();
#endif
    if (equalIgnoringASCIICase(type, "ErrorEvent"))
        return ErrorEvent::create();
    if (equalIgnoringASCIICase(type, "Event"))
        return Event::create();
    if (equalIgnoringASCIICase(type, "Events"))
        return Event::create();
    if (equalIgnoringASCIICase(type, "FocusEvent"))
        return FocusEvent::create();
#if ENABLE(GAMEPAD)
    if (equalIgnoringASCIICase(type, "GamepadEvent"))
        return GamepadEvent::create();
#endif
#if ENABLE(IOS_GESTURE_EVENTS) || ENABLE(MAC_GESTURE_EVENTS)
    if (equalIgnoringASCIICase(type, "GestureEvent"))
        return GestureEvent::create();
#endif
    if (equalIgnoringASCIICase(type, "HTMLEvents"))
        return Event::create();
    if (equalIgnoringASCIICase(type, "HashChangeEvent"))
        return HashChangeEvent::create();
#if ENABLE(INDEXED_DATABASE)
    if (equalIgnoringASCIICase(type, "IDBVersionChangeEvent"))
        return IDBVersionChangeEvent::create();
#endif
    if (equalIgnoringASCIICase(type, "KeyboardEvent"))
        return KeyboardEvent::create();
    if (equalIgnoringASCIICase(type, "KeyboardEvents"))
        return KeyboardEvent::create();
#if ENABLE(ENCRYPTED_MEDIA)
    if (equalIgnoringASCIICase(type, "MediaKeyEvent"))
        return MediaKeyEvent::create();
#endif
#if ENABLE(ENCRYPTED_MEDIA_V2)
    if (equalIgnoringASCIICase(type, "MediaKeyMessageEvent"))
        return MediaKeyMessageEvent::create();
#endif
#if ENABLE(ENCRYPTED_MEDIA_V2)
    if (equalIgnoringASCIICase(type, "MediaKeyNeededEvent"))
        return MediaKeyNeededEvent::create();
#endif
#if ENABLE(MEDIA_STREAM)
    if (equalIgnoringASCIICase(type, "MediaStreamEvent"))
        return MediaStreamEvent::create();
#endif
#if ENABLE(MEDIA_STREAM)
    if (equalIgnoringASCIICase(type, "MediaStreamTrackEvent"))
        return MediaStreamTrackEvent::create();
#endif
    if (equalIgnoringASCIICase(type, "MessageEvent"))
        return MessageEvent::create();
    if (equalIgnoringASCIICase(type, "MouseEvent"))
        return MouseEvent::create();
    if (equalIgnoringASCIICase(type, "MouseEvents"))
        return MouseEvent::create();
    if (equalIgnoringASCIICase(type, "MutationEvent"))
        return MutationEvent::create();
    if (equalIgnoringASCIICase(type, "MutationEvents"))
        return MutationEvent::create();
#if ENABLE(WEB_AUDIO)
    if (equalIgnoringASCIICase(type, "OfflineAudioCompletionEvent"))
        return OfflineAudioCompletionEvent::create();
#endif
#if ENABLE(ORIENTATION_EVENTS)
    if (equalIgnoringASCIICase(type, "OrientationEvent"))
        return Event::create();
#endif
    if (equalIgnoringASCIICase(type, "OverflowEvent"))
        return OverflowEvent::create();
    if (equalIgnoringASCIICase(type, "PageTransitionEvent"))
        return PageTransitionEvent::create();
    if (equalIgnoringASCIICase(type, "PopStateEvent"))
        return PopStateEvent::create();
    if (equalIgnoringASCIICase(type, "ProgressEvent"))
        return ProgressEvent::create();
#if ENABLE(WEB_RTC)
    if (equalIgnoringASCIICase(type, "RTCDTMFToneChangeEvent"))
        return RTCDTMFToneChangeEvent::create();
#endif
#if ENABLE(WEB_RTC)
    if (equalIgnoringASCIICase(type, "RTCDataChannelEvent"))
        return RTCDataChannelEvent::create();
#endif
#if ENABLE(WEB_RTC)
    if (equalIgnoringASCIICase(type, "RTCIceCandidateEvent"))
        return RTCIceCandidateEvent::create();
#endif
#if ENABLE(WEB_RTC)
    if (equalIgnoringASCIICase(type, "RTCTrackEvent"))
        return RTCTrackEvent::create();
#endif
    if (equalIgnoringASCIICase(type, "SVGEvents"))
        return Event::create();
    if (equalIgnoringASCIICase(type, "SVGZoomEvent"))
        return SVGZoomEvent::create();
    if (equalIgnoringASCIICase(type, "SVGZoomEvents"))
        return SVGZoomEvent::create();
    if (equalIgnoringASCIICase(type, "SecurityPolicyViolationEvent"))
        return SecurityPolicyViolationEvent::create();
#if ENABLE(SPEECH_SYNTHESIS)
    if (equalIgnoringASCIICase(type, "SpeechSynthesisEvent"))
        return SpeechSynthesisEvent::create();
#endif
    if (equalIgnoringASCIICase(type, "StorageEvent"))
        return StorageEvent::create();
    if (equalIgnoringASCIICase(type, "TextEvent"))
        return TextEvent::create();
#if ENABLE(TOUCH_EVENTS)
    if (equalIgnoringASCIICase(type, "TouchEvent"))
        return TouchEvent::create();
#endif
#if ENABLE(VIDEO_TRACK)
    if (equalIgnoringASCIICase(type, "TrackEvent"))
        return TrackEvent::create();
#endif
    if (equalIgnoringASCIICase(type, "TransitionEvent"))
        return TransitionEvent::create();
    if (equalIgnoringASCIICase(type, "UIEvent"))
        return UIEvent::create();
    if (equalIgnoringASCIICase(type, "UIEvents"))
        return UIEvent::create();
#if ENABLE(INDIE_UI)
    if (equalIgnoringASCIICase(type, "UIRequestEvent"))
        return UIRequestEvent::create();
#endif
#if ENABLE(WEBGL)
    if (equalIgnoringASCIICase(type, "WebGLContextEvent"))
        return WebGLContextEvent::create();
#endif
    if (equalIgnoringASCIICase(type, "WebKitAnimationEvent"))
        return WebKitAnimationEvent::create();
#if ENABLE(WIRELESS_PLAYBACK_TARGET)
    if (equalIgnoringASCIICase(type, "WebKitPlaybackTargetAvailabilityEvent"))
        return WebKitPlaybackTargetAvailabilityEvent::create();
#endif
    if (equalIgnoringASCIICase(type, "WebKitTransitionEvent"))
        return WebKitTransitionEvent::create();
    if (equalIgnoringASCIICase(type, "WheelEvent"))
        return WheelEvent::create();
    if (equalIgnoringASCIICase(type, "XMLHttpRequestProgressEvent"))
        return XMLHttpRequestProgressEvent::create();
    return 0;
}

} // namespace WebCore
