/*
 * Copyright (C) 2008, 2009, 2010 Apple Inc. All Rights Reserved.
 * Copyright (C) 2009 Jan Michael Alonzo
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
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
#include "AccessibilityController.h"

#include "AccessibilityCallbacks.h"
#include "AccessibilityUIElement.h"
#include "DumpRenderTree.h"
#include "WebCoreSupport/DumpRenderTreeSupportGtk.h"

#include <atk/atk.h>
#include <gtk/gtk.h>
#include <webkit/webkit.h>

static bool loggingAccessibilityEvents = false;

AccessibilityController::AccessibilityController()
{
}

AccessibilityController::~AccessibilityController()
{
}

AccessibilityUIElement AccessibilityController::elementAtPoint(int x, int y)
{
    // FIXME: implement
    return 0;
}

AccessibilityUIElement AccessibilityController::focusedElement()
{
    AtkObject* accessible =  DumpRenderTreeSupportGtk::getFocusedAccessibleElement(mainFrame);
    if (!accessible)
        return 0;

    return AccessibilityUIElement(accessible);
}

AccessibilityUIElement AccessibilityController::rootElement()
{
    AtkObject* accessible = DumpRenderTreeSupportGtk::getRootAccessibleElement(mainFrame);
    if (!accessible)
        return 0;

    return AccessibilityUIElement(accessible);
}

void AccessibilityController::setLogFocusEvents(bool)
{
}

void AccessibilityController::setLogScrollingStartEvents(bool)
{
}

void AccessibilityController::setLogValueChangeEvents(bool)
{
}

void AccessibilityController::setLogAccessibilityEvents(bool logAccessibilityEvents)
{
    if (logAccessibilityEvents == loggingAccessibilityEvents)
        return;

    if (!logAccessibilityEvents) {
        disconnectAccessibilityCallbacks();
        loggingAccessibilityEvents = false;
        return;
    }

    connectAccessibilityCallbacks();
    loggingAccessibilityEvents = true;
}

bool AccessibilityController::addNotificationListener(JSObjectRef)
{
    return false;
}

void AccessibilityController::removeNotificationListener()
{
}
