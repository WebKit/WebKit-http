/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef config_h
#define config_h

// To avoid confict of LOG in wtf/Assertions.h and LOG in base/logging.h,
// skip base/loggin.h by defining BASE_LOGGING_H_ and define some macros
// provided by base/logging.h.
// FIXME: Remove this hack!
#include <iostream>
#define BASE_LOGGING_H_
#define CHECK(condition) while (false && (condition)) std::cerr
#define DCHECK(condition) while (false && (condition)) std::cerr
#define DCHECK_EQ(a, b) while (false && (a) == (b)) std::cerr
#define DCHECK_NE(a, b) while (false && (a) != (b)) std::cerr

#include <wtf/Platform.h>

#if OS(WINDOWS) && !COMPILER(GCC)
// Allow 'this' to be used in base member initializer list.
#pragma warning(disable : 4355)
// JS_EXPORTDATA is needed to inlucde wtf/WTFString.h.
#define JS_EXPORTDATA __declspec(dllimport)
#else
#define JS_EXPORTDATA
#endif

#define WTF_EXPORT_PRIVATE
#define JS_EXPORT_PRIVATE

#endif // config_h
