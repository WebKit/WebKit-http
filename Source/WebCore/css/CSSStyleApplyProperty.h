/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef CSSStyleApplyProperty_h
#define CSSStyleApplyProperty_h

#include "CSSPropertyNames.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>

namespace WebCore {

class CSSStyleSelector;
class CSSValue;
class CSSStyleApplyProperty;

class PropertyHandler {
public:
    typedef void (*InheritFunction)(CSSStyleSelector*);
    typedef void (*InitialFunction)(CSSStyleSelector*);
    typedef void (*ApplyFunction)(CSSStyleSelector*, CSSValue*);
    PropertyHandler() : m_inherit(0), m_initial(0), m_apply(0) { }
    PropertyHandler(InheritFunction inherit, InitialFunction initial, ApplyFunction apply) : m_inherit(inherit), m_initial(initial), m_apply(apply) { }
    void applyInheritValue(CSSStyleSelector* selector) const { ASSERT(m_inherit); (*m_inherit)(selector); }
    void applyInitialValue(CSSStyleSelector* selector) const { ASSERT(m_initial); (*m_initial)(selector); }
    void applyValue(CSSStyleSelector* selector, CSSValue* value) const { ASSERT(m_apply); (*m_apply)(selector, value); }
    bool isValid() const { return m_inherit && m_initial && m_apply; }
    InheritFunction inheritFunction() const { return m_inherit; }
    InitialFunction initialFunction() { return m_initial; }
    ApplyFunction applyFunction() { return m_apply; }
private:
    InheritFunction m_inherit;
    InitialFunction m_initial;
    ApplyFunction m_apply;
};

class CSSStyleApplyProperty {
    WTF_MAKE_NONCOPYABLE(CSSStyleApplyProperty);
public:
    static const CSSStyleApplyProperty& sharedCSSStyleApplyProperty();

    const PropertyHandler& propertyHandler(CSSPropertyID property) const
    {
        ASSERT(valid(property));
        return m_propertyMap[index(property)];
    }
private:
    CSSStyleApplyProperty();
    static int index(CSSPropertyID property)
    {
        return property - firstCSSProperty;
    }

    static bool valid(CSSPropertyID property)
    {
        int i = index(property);
        return i >= 0 && i < numCSSProperties;
    }

    void setPropertyHandler(CSSPropertyID property, PropertyHandler handler)
    {
        ASSERT(valid(property));
        ASSERT(!propertyHandler(property).isValid());
        m_propertyMap[index(property)] = handler;
    }

    void setPropertyHandler(CSSPropertyID newProperty, CSSPropertyID equivalentProperty)
    {
        ASSERT(valid(newProperty));
        ASSERT(valid(equivalentProperty));
        ASSERT(!propertyHandler(newProperty).isValid());
        m_propertyMap[index(newProperty)] = m_propertyMap[index(equivalentProperty)];
    }

    PropertyHandler m_propertyMap[numCSSProperties];
};

}

#endif // CSSStyleApplyProperty_h
