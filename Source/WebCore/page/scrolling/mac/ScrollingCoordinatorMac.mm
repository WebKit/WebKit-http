/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

#include "config.h"

#if ENABLE(THREADED_SCROLLING)

#import "ScrollingCoordinator.h"

#import "FrameView.h"
#import "Page.h"
#import "ScrollingThread.h"
#import "ScrollingTree.h"
#import "ScrollingTreeState.h"
#import <QuartzCore/QuartzCore.h>
#import <wtf/Functional.h>
#import <wtf/MainThread.h>
#import <wtf/RetainPtr.h>
#import <wtf/StdLibExtras.h>
#import <wtf/Vector.h>

namespace WebCore {

class ScrollingCoordinatorPrivate {
};

PassRefPtr<ScrollingCoordinator> ScrollingCoordinator::create(Page* page)
{
    return adoptRef(new ScrollingCoordinator(page));
}

ScrollingCoordinator::~ScrollingCoordinator()
{
    ASSERT(!m_page);
    ASSERT(!m_scrollingTree);
}

void ScrollingCoordinator::frameViewHorizontalScrollbarLayerDidChange(FrameView* frameView, GraphicsLayer*)
{
    ASSERT(isMainThread());
    ASSERT(m_page);

    if (frameView->frame() != m_page->mainFrame())
        return;

    // FIXME: Implement.
}

void ScrollingCoordinator::frameViewVerticalScrollbarLayerDidChange(FrameView* frameView, GraphicsLayer*)
{
    ASSERT(isMainThread());
    ASSERT(m_page);
    
    if (frameView->frame() != m_page->mainFrame())
        return;

    // FIXME: Implement.
}

} // namespace WebCore

#endif // ENABLE(THREADED_SCROLLING)
