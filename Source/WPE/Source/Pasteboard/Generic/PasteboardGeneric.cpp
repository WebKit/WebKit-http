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

#include "Config.h"
#include "PasteboardGeneric.h"

namespace WPE {

namespace Pasteboard {

PasteboardGeneric::PasteboardGeneric()
{
}

PasteboardGeneric::~PasteboardGeneric()
{
}

std::vector<std::string> PasteboardGeneric::getTypes()
{
    std::vector<std::string> types;
    for (auto dataType: m_dataMap)
        types.push_back(dataType.first);
    return types;
}

std::string PasteboardGeneric::getString(const std::string pasteboardType)
{
    if (m_dataMap.count(pasteboardType) == 0)
        return std::string();

    if (pasteboardType == "text/plain;charset=utf-8")
        return std::string(*static_cast<std::string*>(m_dataMap[pasteboardType]));

    return std::string();
}

void PasteboardGeneric::write(const std::map<std::string, void*> dataMap)
{
    m_dataMap = dataMap;

}

void PasteboardGeneric::write(const std::string pasteboardType, const std::string stringToWrite)
{
    m_dataMap.clear();
    m_dataMap[pasteboardType] = static_cast<void*>(new std::string(stringToWrite));
}

} // namespace Pasteboard

} // namespace WPE
