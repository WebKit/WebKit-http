/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
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

#ifndef InternalSettings_h
#define InternalSettings_h

#include "FrameDestructionObserver.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

typedef int ExceptionCode;

class Frame;
class Document;
class Page;
class Settings;

class InternalSettings : public RefCounted<InternalSettings>,
                         public FrameDestructionObserver {
public:
    static PassRefPtr<InternalSettings> create(Frame*, InternalSettings* old);
    virtual ~InternalSettings();

    void setInspectorResourcesDataSizeLimits(int maximumResourcesContentSize, int maximumSingleResourceContentSize, ExceptionCode&);
    void setForceCompositingMode(bool enabled, ExceptionCode&);
    void setEnableCompositingForFixedPosition(bool enabled, ExceptionCode&);
    void setEnableCompositingForScrollableFrames(bool enabled, ExceptionCode&);
    void setAcceleratedDrawingEnabled(bool enabled, ExceptionCode&);
    void setAcceleratedFiltersEnabled(bool enabled, ExceptionCode&);
    void setMockScrollbarsEnabled(bool enabled, ExceptionCode&);
    void setPasswordEchoEnabled(bool enabled, ExceptionCode&);
    void setPasswordEchoDurationInSeconds(double durationInSeconds, ExceptionCode&);
    void setFixedElementsLayoutRelativeToFrame(bool, ExceptionCode&);
    void setUnifiedTextCheckingEnabled(bool, ExceptionCode&);
    bool unifiedTextCheckingEnabled(ExceptionCode&);
    void setPageScaleFactor(float scaleFactor, int x, int y, ExceptionCode&);
    void setPerTileDrawingEnabled(bool enabled, ExceptionCode&);
    void setTouchEventEmulationEnabled(bool enabled, ExceptionCode&);

private:
    InternalSettings(Frame*, InternalSettings* old);

    Settings* settings() const;
    Document* document() const;
    Page* page() const;

    double m_passwordEchoDurationInSecondsBackup;
    bool m_passwordEchoEnabledBackup : 1;
    bool m_passwordEchoDurationInSecondsBackedUp : 1;
    bool m_passwordEchoEnabledBackedUp : 1;
};

} // namespace WebCore

#endif
