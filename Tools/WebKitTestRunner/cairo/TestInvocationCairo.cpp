/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
 *           (C) 2011 Brent Fulgham <bfulgham@webkit.org>. All rights reserved.
 *           (C) 2010, 2011 Igalia S.L
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
#include "TestInvocation.h"

#include <WebKit2/WKImageCairo.h>
#include <cairo/cairo.h>
#include <cstdio>
#include <wtf/Assertions.h>
#include <wtf/MD5.h>
#include <wtf/StringExtras.h>

namespace WTR {

void computeMD5HashStringForCairoSurface(cairo_surface_t* surface, char hashString[33])
{
    ASSERT(cairo_image_surface_get_format(surface) == CAIRO_FORMAT_ARGB32); // ImageDiff assumes 32 bit RGBA, we must as well.

    size_t pixelsHigh = cairo_image_surface_get_height(surface);
    size_t pixelsWide = cairo_image_surface_get_width(surface);
    size_t bytesPerRow = cairo_image_surface_get_stride(surface);

    MD5 md5Context;
    unsigned char* bitmapData = static_cast<unsigned char*>(cairo_image_surface_get_data(surface));
    for (size_t row = 0; row < pixelsHigh; ++row) {
        md5Context.addBytes(bitmapData, 4 * pixelsWide);
        bitmapData += bytesPerRow;
    }
    Vector<uint8_t, 16> hash;
    md5Context.checksum(hash);

    snprintf(hashString, 33, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
        hash[0], hash[1], hash[2], hash[3], hash[4], hash[5], hash[6], hash[7],
        hash[8], hash[9], hash[10], hash[11], hash[12], hash[13], hash[14], hash[15]);
}

static cairo_status_t writeFunction(void* closure, const unsigned char* data, unsigned int length)
{
    Vector<unsigned char>* in = reinterpret_cast<Vector<unsigned char>*>(closure);
    in->append(data, length);
    return CAIRO_STATUS_SUCCESS;
}

static void dumpBitmap(cairo_surface_t* surface)
{
    Vector<unsigned char> pixelData;
    cairo_surface_write_to_png_stream(surface, writeFunction, &pixelData);
    const size_t dataLength = pixelData.size();
    const unsigned char* data = pixelData.data();

    printf("Content-Type: %s\n", "image/png");
    printf("Content-Length: %lu\n", static_cast<unsigned long>(dataLength));

    const size_t bytesToWriteInOneChunk = 1 << 15;
    size_t dataRemainingToWrite = dataLength;
    while (dataRemainingToWrite) {
        size_t bytesToWriteInThisChunk = std::min(dataRemainingToWrite, bytesToWriteInOneChunk);
        size_t bytesWritten = fwrite(data, 1, bytesToWriteInThisChunk, stdout);
        if (bytesWritten != bytesToWriteInThisChunk)
            break;
        dataRemainingToWrite -= bytesWritten;
        data += bytesWritten;
    }
}

void TestInvocation::dumpPixelsAndCompareWithExpected(WKImageRef wkImage)
{
    cairo_surface_t* surface = WKImageCreateCairoSurface(wkImage);

    char actualHash[33];
    computeMD5HashStringForCairoSurface(surface, actualHash);
    if (!compareActualHashToExpectedAndDumpResults(actualHash))
        dumpBitmap(surface);

    cairo_surface_destroy(surface);
}

} // namespace WTR
