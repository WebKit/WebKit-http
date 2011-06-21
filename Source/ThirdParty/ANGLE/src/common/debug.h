//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// debug.h: Debugging utilities.

#ifndef COMMON_DEBUG_H_
#define COMMON_DEBUG_H_

#include <stdio.h>
#include <assert.h>

#include "common/angleutils.h"

#if !defined(TRACE_OUTPUT_FILE)
#define TRACE_OUTPUT_FILE "debug.txt"
#endif

namespace gl
{
    // Outputs text to the debugging log
    void trace(bool traceFileDebugOnly, const char *format, ...);

    // Returns whether D3DPERF is active.
    bool perfActive();

    // Pairs a D3D begin event with an end event.
    class ScopedPerfEventHelper
    {
      public:
        ScopedPerfEventHelper(const char* format, ...);
        ~ScopedPerfEventHelper();

      private:
        DISALLOW_COPY_AND_ASSIGN(ScopedPerfEventHelper);
    };
}

// A macro to output a trace of a function call and its arguments to the debugging log
#if defined(ANGLE_DISABLE_TRACE) && defined(ANGLE_DISABLE_PERF)
#define TRACE(message, ...) (void(0))
#else
#define TRACE(message, ...) gl::trace(true, "trace: %s(%d): "message"\n", __FUNCTION__, __LINE__, __VA_ARGS__)
#endif

// A macro to output a function call and its arguments to the debugging log, to denote an item in need of fixing.
#if defined(ANGLE_DISABLE_TRACE) && defined(ANGLE_DISABLE_PERF)
#define FIXME(message, ...) (void(0))
#else
#define FIXME(message, ...) gl::trace(false, "fixme: %s(%d): "message"\n", __FUNCTION__, __LINE__, __VA_ARGS__)
#endif

// A macro to output a function call and its arguments to the debugging log, in case of error.
#if defined(ANGLE_DISABLE_TRACE) && defined(ANGLE_DISABLE_PERF)
#define ERR(message, ...) (void(0))
#else
#define ERR(message, ...) gl::trace(false, "err: %s(%d): "message"\n", __FUNCTION__, __LINE__, __VA_ARGS__)
#endif

// A macro to log a performance event around a scope.
#if defined(ANGLE_DISABLE_TRACE) && defined(ANGLE_DISABLE_PERF)
#define EVENT(message, ...) (void(0))
#else
#define EVENT(message, ...) gl::ScopedPerfEventHelper scopedPerfEventHelper ## __LINE__(__FUNCTION__ message "\n", __VA_ARGS__);
#endif

// A macro asserting a condition and outputting failures to the debug log
#if !defined(NDEBUG)
#define ASSERT(expression) do { \
    if(!(expression)) \
        ERR("\t! Assert failed in %s(%d): "#expression"\n", __FUNCTION__, __LINE__); \
        assert(expression); \
    } while(0)
#else
#define ASSERT(expression) (void(0))
#endif

// A macro to indicate unimplemented functionality
#if !defined(NDEBUG)
#define UNIMPLEMENTED() do { \
    FIXME("\t! Unimplemented: %s(%d)\n", __FUNCTION__, __LINE__); \
    assert(false); \
    } while(0)
#else
    #define UNIMPLEMENTED() FIXME("\t! Unimplemented: %s(%d)\n", __FUNCTION__, __LINE__)
#endif

// A macro for code which is not expected to be reached under valid assumptions
#if !defined(NDEBUG)
#define UNREACHABLE() do { \
    ERR("\t! Unreachable reached: %s(%d)\n", __FUNCTION__, __LINE__); \
    assert(false); \
    } while(0)
#else
    #define UNREACHABLE() ERR("\t! Unreachable reached: %s(%d)\n", __FUNCTION__, __LINE__)
#endif

// A macro functioning as a compile-time assert to validate constant conditions
#define META_ASSERT(condition) typedef int COMPILE_TIME_ASSERT_##__LINE__[static_cast<bool>(condition)?1:-1]

#endif   // COMMON_DEBUG_H_
