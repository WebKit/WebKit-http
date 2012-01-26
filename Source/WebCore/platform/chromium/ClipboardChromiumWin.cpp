/*
 * Copyright (C) 2009 Apple Inc.  All rights reserved.
 * Copyright (C) 2009 Google Inc.
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

#include "config.h"
#include "ClipboardChromium.h"

#include <shlwapi.h>

namespace WebCore {

// FAT32 and NTFS both limit filenames to a maximum of 255 characters.
static const unsigned maxFilenameLength = 255;

// Returns true if the specified character is not valid in a file name. This
// is intended for use with removeCharacters.
static bool isInvalidFileCharacter(UChar c)
{
    return (PathGetCharType(c) & (GCT_LFNCHAR | GCT_SHORTCHAR)) == 0;
}

void ClipboardChromium::validateFilename(String& name, String& extension)
{
    // Remove any invalid file system characters.
    name = name.removeCharacters(&isInvalidFileCharacter);
    extension = extension.removeCharacters(&isInvalidFileCharacter);

    if (extension.length() >= maxFilenameLength)
        extension = String();

    // Truncate overly-long filenames, reserving one character for a dot.
    name.truncate(maxFilenameLength - extension.length() - 1);
}

}  // namespace WebCore
