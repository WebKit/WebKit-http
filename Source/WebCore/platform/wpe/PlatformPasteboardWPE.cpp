/*
 * Copyright (C) 2015 Igalia S.L.
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

#include "Pasteboard.h"
#include "PlatformPasteboard.h"
#include <WPE/Pasteboard/Pasteboard.h>
#include <wtf/text/WTFString.h>
#include <wtf/Assertions.h>

#include <map>

namespace WebCore {

PlatformPasteboard::PlatformPasteboard(const String&)
    : m_pasteboard(WPE::Pasteboard::Pasteboard::singleton())
{
    ASSERT(m_pasteboard);
}

PlatformPasteboard::PlatformPasteboard()
    : m_pasteboard(WPE::Pasteboard::Pasteboard::singleton())
{
    ASSERT(m_pasteboard);
}

void PlatformPasteboard::getTypes(Vector<String>& types)
{
    auto pasteboardTypes = m_pasteboard->getTypes();
    for (auto type: pasteboardTypes)
        types.append(type.c_str());
}

String PlatformPasteboard::readString(int, const String& pasteboardType)
{
    return String(m_pasteboard->getString(pasteboardType.utf8().data()).c_str());
}

void PlatformPasteboard::write(const PasteboardWebContent& content)
{
    std::map<std::string, std::string> contentMap;
    contentMap["text/plain;charset=utf-8"] = std::string(content.text.utf8().data());
    contentMap["text/html;charset=utf-8"] = std::string(content.markup.utf8().data());
    m_pasteboard->write(std::move(contentMap));
}

void PlatformPasteboard::write(const String& pasteboardType, const String& stringToWrite)
{
    m_pasteboard->write(pasteboardType.utf8().data(), stringToWrite.utf8().data());
}

} // namespace WebCore
