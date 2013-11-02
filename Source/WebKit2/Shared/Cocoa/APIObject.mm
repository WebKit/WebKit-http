/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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

#import "config.h"
#import "APIObject.h"

#if WK_API_ENABLED

#import "WKBackForwardListInternal.h"
#import "WKBackForwardListItemInternal.h"
#import "WKNSArray.h"
#import "WKNSString.h"
#import "WKNSURL.h"

namespace WebKit {

void APIObject::ref()
{
    [wrapper() retain];
}

void APIObject::deref()
{
    [wrapper() release];
}

void* APIObject::newObject(size_t size, Type type)
{
    NSObject <WKObject> *wrapper;

    // Wrappers that inherit from WKObject store the APIObject in their extra bytes, so they are
    // allocated using NSAllocatedObject. The other wrapper classes contain inline storage for the
    // APIObject, so they are allocated using +alloc.

    switch (type) {
    case TypeArray:
        wrapper = [WKNSArray alloc];
        break;

    case TypeBackForwardList:
        wrapper = [WKBackForwardList alloc];
        break;

    case TypeBackForwardListItem:
        wrapper = [WKBackForwardListItem alloc];
        break;

    case TypeString:
        wrapper = NSAllocateObject([WKNSString class], size, nullptr);
        break;

    case TypeURL:
        wrapper = NSAllocateObject([WKNSURL class], size, nullptr);
        break;

    default:
        wrapper = NSAllocateObject([WKObject class], size, nullptr);
        break;
    }

    APIObject* object = &wrapper._apiObject;
    object->m_wrapper = wrapper;
    return object;
}

} // namespace WebKit

#endif // WK_API_ENABLED
