/*
 * Copyright (C) 2006, 2007 Apple Inc.  All rights reserved.
 * Copyright (C) 2007-2009 Torch Mobile, Inc.
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
#include "MIMETypeRegistry.h"

#include <wtf/Assertions.h>
#include <wtf/HashMap.h>
#include <wtf/MainThread.h>
#include <windows.h>
#include <winreg.h>

namespace WebCore {

static String mimeTypeForExtension(const String& extension)
{
    String ext = "." + extension;
    WCHAR contentTypeStr[256];
    DWORD contentTypeStrLen = sizeof(contentTypeStr);
    DWORD valueType;

    HKEY key;
    String result;
    if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_CLASSES_ROOT, ext.charactersWithNullTermination(), 0, 0, &key))
        return result;

    if (ERROR_SUCCESS == RegQueryValueEx(key, L"Content Type", 0, &valueType, (LPBYTE)contentTypeStr, &contentTypeStrLen) && valueType == REG_SZ)
        result = String(contentTypeStr, contentTypeStrLen / sizeof(contentTypeStr[0]) - 1);

    RegCloseKey(key);

    return result;
}

static HashMap<String, String> mimetypeMap;

static void initMIMETypeEntensionMap()
{
    if (mimetypeMap.isEmpty()) {
        //fill with initial values
        mimetypeMap.add("txt", "text/plain");
        mimetypeMap.add("pdf", "application/pdf");
        mimetypeMap.add("ps", "application/postscript");
        mimetypeMap.add("html", "text/html");
        mimetypeMap.add("htm", "text/html");
        mimetypeMap.add("xml", "text/xml");
        mimetypeMap.add("xsl", "text/xsl");
        mimetypeMap.add("js", "application/x-javascript");
        mimetypeMap.add("xhtml", "application/xhtml+xml");
        mimetypeMap.add("rss", "application/rss+xml");
        mimetypeMap.add("webarchive", "application/x-webarchive");
        mimetypeMap.add("svg", "image/svg+xml");
        mimetypeMap.add("svgz", "image/svg+xml");
        mimetypeMap.add("jpg", "image/jpeg");
        mimetypeMap.add("jpeg", "image/jpeg");
        mimetypeMap.add("png", "image/png");
        mimetypeMap.add("tif", "image/tiff");
        mimetypeMap.add("tiff", "image/tiff");
        mimetypeMap.add("ico", "image/ico");
        mimetypeMap.add("cur", "image/ico");
        mimetypeMap.add("bmp", "image/bmp");
        mimetypeMap.add("css", "text/css");
        // FIXME: Custom font works only when MIME is "text/plain"
        mimetypeMap.add("ttf", "text/plain"); // "font/ttf"
        mimetypeMap.add("otf", "text/plain"); // "font/otf"
    }
}

String MIMETypeRegistry::getPreferredExtensionForMIMEType(const String& type)
{
    if (type.isEmpty())
        return String();

    // Avoid conflicts with "ttf" and "otf"
    if (equalIgnoringCase(type, "text/plain"))
        return "txt";

    initMIMETypeEntensionMap();

    for (HashMap<String, String>::iterator i = mimetypeMap.begin(); i != mimetypeMap.end(); ++i) {
        if (equalIgnoringCase(i->second, type))
            return i->first;
    }

    return String();
}

String MIMETypeRegistry::getMIMETypeForExtension(const String &ext)
{
    ASSERT(isMainThread());

    if (ext.isEmpty())
        return String();

    initMIMETypeEntensionMap();

    String result = mimetypeMap.get(ext.lower());
    if (result.isEmpty()) {
        result = mimeTypeForExtension(ext);
        if (!result.isEmpty())
            mimetypeMap.add(ext, result);
    }
    return result.isEmpty() ? "unknown/unknown" : result;
}

bool MIMETypeRegistry::isApplicationPluginMIMEType(const String&)
{
    return false;
}

} // namespace WebCore
