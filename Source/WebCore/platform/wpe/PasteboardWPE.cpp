/*
 * Copyright (C) 2014 Igalia S.L.
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

#include <wtf/PassOwnPtr.h>

namespace WebCore {

PassOwnPtr<Pasteboard> Pasteboard::createForCopyAndPaste()
{
    return adoptPtr(new Pasteboard);
}

PassOwnPtr<Pasteboard> Pasteboard::createPrivate()
{
    return adoptPtr(new Pasteboard);
}

Pasteboard::Pasteboard()
{
}

bool Pasteboard::hasData()
{
    return false;
}

Vector<String> Pasteboard::types()
{
    return Vector<String>();
}

String Pasteboard::readString(const String&)
{
    return String();
}

void Pasteboard::writeString(const String&, const String&)
{
}

void Pasteboard::clear()
{
}

void Pasteboard::clear(const String&)
{
}

void Pasteboard::read(PasteboardPlainText&)
{
}

void Pasteboard::read(PasteboardWebContentReader&)
{
}

void Pasteboard::write(const PasteboardURL&)
{
}

void Pasteboard::write(const PasteboardImage&)
{
}

void Pasteboard::write(const PasteboardWebContent&)
{
}

Vector<String> Pasteboard::readFilenames()
{
    return Vector<String>();
}

bool Pasteboard::canSmartReplace()
{
    return false;
}

void Pasteboard::writeMarkup(const String&)
{
}

void Pasteboard::writePlainText(const String&, SmartReplaceOption)
{
}

void Pasteboard::writePasteboard(const Pasteboard&)
{
}

} // namespace WebCore
