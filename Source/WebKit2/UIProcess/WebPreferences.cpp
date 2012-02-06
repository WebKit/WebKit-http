/*
 * Copyright (C) 2010, 2011 Apple Inc. All rights reserved.
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
#include "WebPreferences.h"

#include "WebPageGroup.h"

namespace WebKit {

WebPreferences::WebPreferences()
{
    platformInitializeStore();
}

WebPreferences::WebPreferences(const String& identifier)
    : m_identifier(identifier)
{
    platformInitializeStore();
}

WebPreferences::WebPreferences(const WebPreferences& other)
    : APIObject()
    , m_store(other.m_store)
{
    platformInitializeStore();
}

WebPreferences::~WebPreferences()
{
}

void WebPreferences::addPageGroup(WebPageGroup* pageGroup)
{
    m_pageGroups.add(pageGroup);
}

void WebPreferences::removePageGroup(WebPageGroup* pageGroup)
{
    m_pageGroups.remove(pageGroup);
}

void WebPreferences::update()
{
    for (HashSet<WebPageGroup*>::iterator it = m_pageGroups.begin(), end = m_pageGroups.end(); it != end; ++it)
        (*it)->preferencesDidChange();
}

void WebPreferences::updateStringValueForKey(const String& key, const String& value)
{
    platformUpdateStringValueForKey(key, value);
    update(); // FIXME: Only send over the changed key and value.
}

void WebPreferences::updateBoolValueForKey(const String& key, bool value)
{
    platformUpdateBoolValueForKey(key, value);
    update(); // FIXME: Only send over the changed key and value.
}

void WebPreferences::updateUInt32ValueForKey(const String& key, uint32_t value)
{
    platformUpdateUInt32ValueForKey(key, value);
    update(); // FIXME: Only send over the changed key and value.
}

void WebPreferences::updateDoubleValueForKey(const String& key, double value)
{
    platformUpdateDoubleValueForKey(key, value);
    update(); // FIXME: Only send over the changed key and value.
}

#define DEFINE_PREFERENCE_GETTER_AND_SETTERS(KeyUpper, KeyLower, TypeName, Type, DefaultValue) \
    void WebPreferences::set##KeyUpper(const Type& value) \
    { \
        if (!m_store.set##TypeName##ValueForKey(WebPreferencesKey::KeyLower##Key(), value)) \
            return; \
        update##TypeName##ValueForKey(WebPreferencesKey::KeyLower##Key(), value); \
        \
    } \
    \
    Type WebPreferences::KeyLower() const \
    { \
        return m_store.get##TypeName##ValueForKey(WebPreferencesKey::KeyLower##Key()); \
    } \

FOR_EACH_WEBKIT_PREFERENCE(DEFINE_PREFERENCE_GETTER_AND_SETTERS)

#undef DEFINE_PREFERENCE_GETTER_AND_SETTERS

} // namespace WebKit
