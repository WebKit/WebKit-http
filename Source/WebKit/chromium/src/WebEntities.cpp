/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
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

#include "config.h"
#include "WebEntities.h"

#include "platform/WebString.h"

#include <string.h>
#include <wtf/HashMap.h>
#include <wtf/text/StringBuilder.h>
#include <wtf/text/WTFString.h>

namespace WebKit {

WebEntities::WebEntities(bool xmlEntities)
{
    ASSERT(m_entitiesMap.isEmpty());
    m_entitiesMap.set(0x003c, "lt");
    m_entitiesMap.set(0x003e, "gt");
    m_entitiesMap.set(0x0026, "amp");
    m_entitiesMap.set(0x0027, "apos");
    m_entitiesMap.set(0x0022, "quot");
    // We add #39 for test-compatibility reason.
    if (!xmlEntities)
        m_entitiesMap.set(0x0027, String("#39"));
}

String WebEntities::entityNameByCode(int code) const
{
    if (m_entitiesMap.contains(code))
        return m_entitiesMap.get(code);
    return "";
}

String WebEntities::convertEntitiesInString(const String& value) const
{
    unsigned len = value.length();
    const UChar* startPos = value.characters();
    const UChar* curPos = startPos;

    // FIXME: Optimize - create StringBuilder only if value has any entities.
    StringBuilder result;
    while (len--) {
        if (m_entitiesMap.contains(*curPos)) {
            // Append content before entity code.
            if (curPos > startPos)
                result.append(String(startPos, curPos - startPos));
            result.append('&');
            result.append(m_entitiesMap.get(*curPos));
            result.append(';');
            startPos = ++curPos;
        } else
            curPos++;
    }
    // Append the remaining content.
    if (curPos > startPos)
        result.append(String(startPos, curPos - startPos));

    return result.toString();
}

} // namespace WebKit
