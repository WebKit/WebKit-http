/*
 * Copyright (C) 2008, 2009, 2010, 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2012 Google Inc.
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

#ifndef RenderMediaControlElements_h
#define RenderMediaControlElements_h

#if ENABLE(VIDEO)

#include "MediaControlElements.h"
#include "RenderBlockFlow.h"
#include "RenderFlexibleBox.h"

namespace WebCore {

class RenderMediaVolumeSliderContainer FINAL : public RenderBlockFlow {
public:
    RenderMediaVolumeSliderContainer(Element&, PassRef<RenderStyle>);

private:
    virtual void layout();
};

// ----------------------------

class RenderMediaControlTimelineContainer FINAL : public RenderFlexibleBox {
public:
    RenderMediaControlTimelineContainer(Element&, PassRef<RenderStyle>);

private:
    virtual void layout();
};

// ----------------------------

#if ENABLE(VIDEO_TRACK)

class RenderTextTrackContainerElement FINAL : public RenderBlockFlow {
public:
    RenderTextTrackContainerElement(Element&, PassRef<RenderStyle>);

private:
    virtual void layout();
};

#endif // ENABLE(VIDEO_TRACK)

} // namespace WebCore

#endif // ENABLE(VIDEO)

#endif // RenderMediaControlElements_h

