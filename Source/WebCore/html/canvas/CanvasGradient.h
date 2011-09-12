/*
 * Copyright (C) 2006, 2007, 2008 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2007 Alp Toker <alp@atoker.com>
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

#ifndef CanvasGradient_h
#define CanvasGradient_h

#include "Gradient.h"
#include <wtf/Forward.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>

namespace WebCore {

    typedef int ExceptionCode;

    class CanvasGradient : public RefCounted<CanvasGradient> {
    public:
        static PassRefPtr<CanvasGradient> create(const FloatPoint& p0, const FloatPoint& p1)
        {
            return adoptRef(new CanvasGradient(p0, p1));
        }
        static PassRefPtr<CanvasGradient> create(const FloatPoint& p0, float r0, const FloatPoint& p1, float r1)
        {
            return adoptRef(new CanvasGradient(p0, r0, p1, r1));
        }
        
        Gradient* gradient() const { return m_gradient.get(); }

        void addColorStop(float value, const String& color, ExceptionCode&);

        void getColor(float value, float* r, float* g, float* b, float* a) const { m_gradient->getColor(value, r, g, b, a); }

#if ENABLE(DASHBOARD_SUPPORT)
        void setDashboardCompatibilityMode() { m_dashbardCompatibilityMode = true; }
#endif

    private:
        CanvasGradient(const FloatPoint& p0, const FloatPoint& p1);
        CanvasGradient(const FloatPoint& p0, float r0, const FloatPoint& p1, float r1);
        
        RefPtr<Gradient> m_gradient;
#if ENABLE(DASHBOARD_SUPPORT)
        bool m_dashbardCompatibilityMode;
#endif
    };

} //namespace

#endif
