/*
 * Copyright (C) 2013 Haiku, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "PixelDumpSupportHaiku.h"

#include "DumpRenderTree.h"
#include "IntRect.h"
#include "NotImplemented.h"

#include <DumpRenderTreeClient.h>

#include <wtf/MD5.h>

PassRefPtr<BitmapContext> createBitmapContextFromWebView(bool, bool, bool, bool drawSelectionRect)
{
    notImplemented();
    return 0;
}

void computeMD5HashStringForBitmapContext(BitmapContext* context, char hashString[33])
{
    int pixelsWide = context->m_width;
    int pixelsHigh = context->m_height;
    int bytesPerRow = context->m_width * 4;
    unsigned char* pixelData = context->m_data;

    MD5 md5;
    for (int i = 0; i < pixelsHigh; ++i) {
        md5.addBytes(pixelData, 4 * pixelsWide);
        pixelData += bytesPerRow;
    }

    Vector<uint8_t, 16> hash;
    md5.checksum(hash);

    hashString[0] = '\0';
    for (int i = 0; i < 16; ++i)
        snprintf(hashString, 33, "%s%02x", hashString, hash[i]);
}

void dumpBitmap(BitmapContext* context, const char* checksum)
{
    notImplemented();
}
