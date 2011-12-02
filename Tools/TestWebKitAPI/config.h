/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

#include <wtf/Platform.h>

#if __APPLE__

#ifdef __OBJC__
#import <Cocoa/Cocoa.h>
#endif

#elif defined(WIN32) || defined(_WIN32)

#define NOMINMAX

#endif

/* FIXME: Define these properly once USE(EXPORT_MACROS) is set for ports using this */
#define JS_EXPORT_PRIVATE
#define WTF_EXPORT_PRIVATE

#define JS_EXPORTDATA

#include <stdint.h>

#if !PLATFORM(CHROMIUM)
#include <WebKit2/WebKit2.h>
#endif

#ifdef __clang__
// Work around the less strict coding standards of the gtest framework.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-variable"
#endif

#ifdef __cplusplus
#include <gtest/gtest.h>
#endif

#ifdef __clang__
// Finish working around the less strict coding standards of the gtest framework.
#pragma clang diagnostic pop
#endif

#if PLATFORM(MAC) && defined(__OBJC__)
#import <WebKit/WebKit.h>
#endif
