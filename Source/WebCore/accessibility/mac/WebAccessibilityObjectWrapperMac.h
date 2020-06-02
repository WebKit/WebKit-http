/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#import "WebAccessibilityObjectWrapperBase.h"

#if PLATFORM(MAC)

#ifndef NSAccessibilityPrimaryScreenHeightAttribute
#define NSAccessibilityPrimaryScreenHeightAttribute @"_AXPrimaryScreenHeight"
#endif

@interface WebAccessibilityObjectWrapper : WebAccessibilityObjectWrapperBase

- (id)textMarkerRangeFromVisiblePositions:(const WebCore::VisiblePosition&)startPosition endPosition:(const WebCore::VisiblePosition&)endPosition;
- (id)textMarkerForVisiblePosition:(const WebCore::VisiblePosition&)visiblePos;
- (id)textMarkerForFirstPositionInTextControl:(WebCore::HTMLTextFormControlElement&)textControl;

// When a plugin uses a WebKit control to act as a surrogate view (e.g. PDF use WebKit to create text fields).
- (id)associatedPluginParent;

@end

#endif // PLATFORM(MAC)

namespace WebCore {

id textMarkerRangeFromMarkers(id textMarker1, id textMarker2);
id textMarkerForVisiblePosition(AXObjectCache*, const VisiblePosition&);
id textMarkerRangeFromVisiblePositions(AXObjectCache*, const VisiblePosition&, const VisiblePosition&);

}
