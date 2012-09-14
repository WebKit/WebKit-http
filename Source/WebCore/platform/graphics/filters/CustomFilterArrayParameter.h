/*
 * Copyright (C) 2012 Adobe Systems Incorporated. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER “AS IS” AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef CustomFilterArrayParameter_h
#define CustomFilterArrayParameter_h

#if ENABLE(CSS_SHADERS)
#include "CustomFilterParameter.h"
#include <wtf/Vector.h>

namespace WebCore {

class CustomFilterArrayParameter : public CustomFilterParameter {
public:
    static PassRefPtr<CustomFilterArrayParameter> create(const String& name)
    {
        return adoptRef(new CustomFilterArrayParameter(name));
    }

    unsigned size() const { return m_data.size(); }
    double valueAt(unsigned index) const { return m_data.at(index); }

    void addValue(double value) { m_data.append(value); }

    virtual PassRefPtr<CustomFilterParameter> blend(const CustomFilterParameter* from, double progress, const LayoutSize&)
    {
        // FIXME: https://bugs.webkit.org/show_bug.cgi?id=96437
        UNUSED_PARAM(from);
        UNUSED_PARAM(progress);

        return this;
    }

    virtual bool operator==(const CustomFilterParameter& o) const
    {
        if (!isSameType(o))
            return false;

        const CustomFilterArrayParameter* other = static_cast<const CustomFilterArrayParameter*>(&o);
        return m_data == other->m_data;
    }

private:
    CustomFilterArrayParameter(const String& name)
        : CustomFilterParameter(ARRAY, name)
    {
    }

    Vector<double> m_data;
};

} // namespace WebCore

#endif // ENABLE(CSS_SHADERS)

#endif // CustomFilterArrayParameter_h
