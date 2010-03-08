/*
 * Copyright (C) 2007 Ryan Leavengood <leavengood@gmail.com>
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
#include "Cursor.h"

#include "NotImplemented.h"

namespace WebCore {

Cursor::Cursor(PlatformCursor cursor)
    : m_impl(cursor)
{
}

Cursor::Cursor(const Cursor& other)
    : m_impl(other.m_impl)
{
}

Cursor::~Cursor()
{
}

Cursor::Cursor(Image*, const IntPoint&)
    : m_impl(*B_CURSOR_SYSTEM_DEFAULT)
{
    notImplemented();
}

Cursor& Cursor::operator=(const Cursor& other)
{
    m_impl = other.m_impl;
    return *this;
}

const Cursor& pointerCursor()
{
	static Cursor sCursorSystemDefault(*B_CURSOR_SYSTEM_DEFAULT);
    return sCursorSystemDefault;
}

const Cursor& moveCursor()
{
    static Cursor sCursorMove = Cursor(BCursor(B_CURSOR_ID_MOVE));
    return sCursorMove;
}

const Cursor& crossCursor()
{
    static Cursor sCursorCrossHair = Cursor(BCursor(B_CURSOR_ID_CROSS_HAIR));
    return sCursorCrossHair;
}

const Cursor& handCursor()
{
    static Cursor sCursorFollowLink = Cursor(BCursor(B_CURSOR_ID_FOLLOW_LINK));
    return sCursorFollowLink;
}

const Cursor& iBeamCursor()
{
    static Cursor sCursorIBeam(*B_CURSOR_I_BEAM);
    return sCursorIBeam;
}

const Cursor& waitCursor()
{
    static Cursor sCursorProgress = Cursor(BCursor(B_CURSOR_ID_PROGRESS));
    return sCursorProgress;
}

const Cursor& helpCursor()
{
    static Cursor sCursorHelp = Cursor(BCursor(B_CURSOR_ID_HELP));
    return sCursorHelp;
}

const Cursor& eastResizeCursor()
{
    static Cursor sCursorResizeEast = Cursor(BCursor(B_CURSOR_ID_RESIZE_EAST));
    return sCursorResizeEast;
}

const Cursor& northResizeCursor()
{
    static Cursor sCursorResizeNorth = Cursor(BCursor(B_CURSOR_ID_RESIZE_NORTH));
    return sCursorResizeNorth;
}

const Cursor& northEastResizeCursor()
{
    static Cursor sCursorResizeNorthEast = Cursor(BCursor(B_CURSOR_ID_RESIZE_NORTH_EAST));
    return sCursorResizeNorthEast;
}

const Cursor& northWestResizeCursor()
{
    static Cursor sCursorResizeNorthWest = Cursor(BCursor(B_CURSOR_ID_RESIZE_NORTH_WEST));
    return sCursorResizeNorthWest;
}

const Cursor& southResizeCursor()
{
    static Cursor sCursorResizeSouth = Cursor(BCursor(B_CURSOR_ID_RESIZE_SOUTH));
    return sCursorResizeSouth;
}

const Cursor& southEastResizeCursor()
{
    static Cursor sCursorResizeSouthEast = Cursor(BCursor(B_CURSOR_ID_RESIZE_SOUTH_EAST));
    return sCursorResizeSouthEast;
}

const Cursor& southWestResizeCursor()
{
    static Cursor sCursorResizeSouthWest = Cursor(BCursor(B_CURSOR_ID_RESIZE_SOUTH_WEST));
    return sCursorResizeSouthWest;
}

const Cursor& westResizeCursor()
{
    static Cursor sCursorResizeWest = Cursor(BCursor(B_CURSOR_ID_RESIZE_WEST));
    return sCursorResizeWest;
}

const Cursor& northSouthResizeCursor()
{
    static Cursor sCursorResizeNorthSouth = Cursor(BCursor(B_CURSOR_ID_RESIZE_NORTH_SOUTH));
    return sCursorResizeNorthSouth;
}

const Cursor& eastWestResizeCursor()
{
    static Cursor sCursorResizeEastWest = Cursor(BCursor(B_CURSOR_ID_RESIZE_EAST_WEST));
    return sCursorResizeEastWest;
}

const Cursor& northEastSouthWestResizeCursor()
{
    static Cursor sCursorResizeNorthEastSouthWest = Cursor(BCursor(B_CURSOR_ID_RESIZE_NORTH_EAST_SOUTH_WEST));
    return sCursorResizeNorthEastSouthWest;
}

const Cursor& northWestSouthEastResizeCursor()
{
    static Cursor sCursorResizeNorthWestSouthEast = Cursor(BCursor(B_CURSOR_ID_RESIZE_NORTH_WEST_SOUTH_EAST));
    return sCursorResizeNorthWestSouthEast;
}

const Cursor& columnResizeCursor()
{
	return eastWestResizeCursor();
}

const Cursor& rowResizeCursor()
{
	return northSouthResizeCursor();
}

const Cursor& verticalTextCursor()
{
    static Cursor sCursorIBeamHorizontal = Cursor(BCursor(B_CURSOR_ID_I_BEAM_HORIZONTAL));
    return sCursorIBeamHorizontal;
}

const Cursor& cellCursor()
{
    return pointerCursor();
}

const Cursor& contextMenuCursor()
{
    static Cursor sCursorContextMenu = Cursor(BCursor(B_CURSOR_ID_CONTEXT_MENU));
    return sCursorContextMenu;
}

const Cursor& noDropCursor()
{
    static Cursor sCursorNotAllowed = Cursor(BCursor(B_CURSOR_ID_NOT_ALLOWED));
    return sCursorNotAllowed;
}

const Cursor& copyCursor()
{
    static Cursor sCursorCopy = Cursor(BCursor(B_CURSOR_ID_COPY));
    return sCursorCopy;
}

const Cursor& progressCursor()
{
    static Cursor sCursorProgress = Cursor(BCursor(B_CURSOR_ID_PROGRESS));
    return sCursorProgress;
}

const Cursor& aliasCursor()
{
	return handCursor();
}

const Cursor& noneCursor()
{
    static Cursor sCursorNoCursor = Cursor(BCursor(B_CURSOR_ID_NO_CURSOR));
    return sCursorNoCursor;
}

const Cursor& notAllowedCursor()
{
    static Cursor sCursorNotAllowed = Cursor(BCursor(B_CURSOR_ID_NOT_ALLOWED));
    return sCursorNotAllowed;
}

const Cursor& zoomInCursor()
{
    static Cursor sCursorZoomIn = Cursor(BCursor(B_CURSOR_ID_ZOOM_IN));
    return sCursorZoomIn;
}

const Cursor& zoomOutCursor()
{
    static Cursor sCursorZoomOut = Cursor(BCursor(B_CURSOR_ID_ZOOM_OUT));
    return sCursorZoomOut;
}

const Cursor& grabCursor()
{
    static Cursor sCursorGrab = Cursor(BCursor(B_CURSOR_ID_GRAB));
    return sCursorGrab;
}

const Cursor& grabbingCursor()
{
    static Cursor sCursorGrabbing = Cursor(BCursor(B_CURSOR_ID_GRABBING));
    return sCursorGrabbing;
}

} // namespace WebCore

