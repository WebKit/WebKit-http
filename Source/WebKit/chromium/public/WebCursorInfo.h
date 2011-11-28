/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WebCursorInfo_h
#define WebCursorInfo_h

#include "platform/WebImage.h"
#include "platform/WebPoint.h"

#if WEBKIT_IMPLEMENTATION
namespace WebCore { class Cursor; }
#endif

#ifdef WIN32
typedef struct HICON__* HICON;
typedef HICON HCURSOR;
#endif

namespace WebKit {

struct WebCursorInfo {
    enum Type {
        TypePointer,
        TypeCross,
        TypeHand,
        TypeIBeam,
        TypeWait,
        TypeHelp,
        TypeEastResize,
        TypeNorthResize,
        TypeNorthEastResize,
        TypeNorthWestResize,
        TypeSouthResize,
        TypeSouthEastResize,
        TypeSouthWestResize,
        TypeWestResize,
        TypeNorthSouthResize,
        TypeEastWestResize,
        TypeNorthEastSouthWestResize,
        TypeNorthWestSouthEastResize,
        TypeColumnResize,
        TypeRowResize,
        TypeMiddlePanning,
        TypeEastPanning,
        TypeNorthPanning,
        TypeNorthEastPanning,
        TypeNorthWestPanning,
        TypeSouthPanning,
        TypeSouthEastPanning,
        TypeSouthWestPanning,
        TypeWestPanning,
        TypeMove,
        TypeVerticalText,
        TypeCell,
        TypeContextMenu,
        TypeAlias,
        TypeProgress,
        TypeNoDrop,
        TypeCopy,
        TypeNone,
        TypeNotAllowed,
        TypeZoomIn,
        TypeZoomOut,
        TypeGrab,
        TypeGrabbing,
        TypeCustom
    };

    Type type;
    WebPoint hotSpot;
    WebImage customImage;

#ifdef WIN32
    // On Windows, TypeCustom may alternatively reference an externally
    // defined HCURSOR.  If type is TypeCustom and externalHandle is non-
    // null, then customData should be ignored.  The WebCursorInfo is not
    // responsible for managing the lifetime of this cursor handle.
    HCURSOR externalHandle;
#endif

    explicit WebCursorInfo(Type type = TypePointer)
        : type(type)
    {
#ifdef WIN32
        externalHandle = 0;
#endif
    }

#if WEBKIT_IMPLEMENTATION
    explicit WebCursorInfo(const WebCore::Cursor&);
#endif
};

} // namespace WebKit

#endif
