/*
 * Copyright (C) 2011 Apple Inc.  All rights reserved.
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
 *
 * This file handles shared library symbol export decorations. It is recommended
 * that all WebKit projects use these definitions so that symbol exports work
 * properly on all platforms and compilers that WebKit builds under.
 */

#ifndef ExportMacros_h
#define ExportMacros_h

#include <wtf/Platform.h>

// Different platforms have different defaults for symbol visibility. Usually
// the compiler and the linker just take care of it. However for references to
// runtime routines from JIT stubs, it matters to be able to declare a symbol as
// being local to the target being generated, and thus not subject to (e.g.) ELF
// symbol interposition rules.

#if !PLATFORM(CHROMIUM) && OS(WINDOWS) && !COMPILER(GCC)
#define HAVE_INTERNAL_VISIBILITY 1
#define WTF_INTERNAL
#elif defined(__GNUC__) && !defined(__CC_ARM) && !defined(__ARMCC__)
#define HAVE_INTERNAL_VISIBILITY 1
#define WTF_INTERNAL __attribute__((visibility("hidden")))
#else
#define WTF_INTERNAL
#endif

// See note in wtf/Platform.h for more info on EXPORT_MACROS.
#if USE(EXPORT_MACROS)

#if !PLATFORM(CHROMIUM) && OS(WINDOWS) && !COMPILER(GCC)
#define WTF_EXPORT __declspec(dllexport)
#define WTF_IMPORT __declspec(dllimport)
#define WTF_HIDDEN
#elif defined(__GNUC__) && !defined(__CC_ARM) && !defined(__ARMCC__)
#define WTF_EXPORT __attribute__((visibility("default")))
#define WTF_IMPORT WTF_EXPORT
#define WTF_HIDDEN __attribute__((visibility("hidden")))
#else
#define WTF_EXPORT
#define WTF_IMPORT
#define WTF_HIDDEN
#endif

// Currently WTF is embedded statically in JSCore, which exports 
// WTF symbols in the JSCore shared library.
// Because of this, we need to make sure that we use WTF_EXPORT
// when building JavaScriptCore as well as WTF.

// FIXME: When all ports are using the export macros, we should replace
// WTF_EXPORTDATA with WTF_EXPORT_PRIVATE macros.
#if defined(BUILDING_WTF)  || defined(BUILDING_JavaScriptCore)
#define WTF_EXPORTDATA WTF_EXPORT
#else
#define WTF_EXPORTDATA WTF_IMPORT
#endif

#else // !USE(EXPORT_MACROS)

#if !PLATFORM(CHROMIUM) && OS(WINDOWS) && !COMPILER(GCC)
#if defined(BUILDING_WTF) || defined(BUILDING_JavaScriptCore)
#define WTF_EXPORTDATA __declspec(dllexport)
#else
#define WTF_EXPORTDATA __declspec(dllimport)
#endif
#else // PLATFORM(CHROMIUM) || !OS(WINDOWS) || COMPILER(GCC)
#define WTF_EXPORTDATA
#endif // !PLATFORM(CHROMIUM)...

#define WTF_EXPORTCLASS WTF_EXPORTDATA

#define WTF_EXPORT
#define WTF_IMPORT
#define WTF_HIDDEN

#endif // USE(EXPORT_MACROS)

#if defined(BUILDING_WTF)  || defined(BUILDING_JavaScriptCore)
#define WTF_EXPORT_PRIVATE WTF_EXPORT
#else
#define WTF_EXPORT_PRIVATE WTF_IMPORT
#endif

// wxWebKit uses RTTI because wx itself does, so use a special macro for
// extra exports it needs.
#if PLATFORM(WX)
#define WTF_EXPORT_PRIVATE_RTTI WTF_EXPORT_PRIVATE
#define WTF_EXPORT_PRIVATE_NO_RTTI
#else
#define WTF_EXPORT_PRIVATE_RTTI
#define WTF_EXPORT_PRIVATE_NO_RTTI WTF_EXPORT_PRIVATE
#endif

#define WTF_EXPORT_HIDDEN WTF_HIDDEN

#define HIDDEN_INLINE WTF_EXPORT_HIDDEN inline

#endif // ExportMacros_h
