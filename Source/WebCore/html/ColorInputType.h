/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
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

#ifndef ColorInputType_h
#define ColorInputType_h

#include "ColorChooser.h"
#include "InputType.h"

#if ENABLE(INPUT_COLOR)

namespace WebCore {

class ColorInputType : public InputType, public ColorChooserClient {
public:
    static PassOwnPtr<InputType> create(HTMLInputElement*);
    virtual ~ColorInputType();

private:
    ColorInputType(HTMLInputElement* element) : InputType(element) { }
    virtual bool isColorControl() const;
    virtual const AtomicString& formControlType() const;
    virtual bool supportsRequired() const;
    virtual String fallbackValue() const OVERRIDE;
    virtual String sanitizeValue(const String&) const;
    virtual Color valueAsColor() const;
    virtual void createShadowSubtree();
    virtual void setValue(const String&, bool valueChanged, bool sendChangeEvent);
    virtual void handleDOMActivateEvent(Event*);
    virtual void detach();

    // ColorChooserClient implementation.
    virtual void didChooseColor(const Color&) OVERRIDE;
    virtual void didCleanup() OVERRIDE;

    void cleanupColorChooser();
    void updateColorSwatch();
    HTMLElement* shadowColorSwatch() const;
};

} // namespace WebCore

#endif // ENABLE(INPUT_COLOR)

#endif // ColorInputType_h
