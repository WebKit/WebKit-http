//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// debug.cpp: Debugging utilities.

#include "compiler/debug.h"

#include <stdarg.h>
#include <stdio.h>

#include "compiler/InitializeParseContext.h"
#include "compiler/ParseHelper.h"

static const int kTraceBufferLen = 1024;

#ifdef TRACE_ENABLED
extern "C" {
void Trace(const char *format, ...) {
    if (!format) return;

    TParseContext* parseContext = GetGlobalParseContext();
    if (parseContext) {
        char buf[kTraceBufferLen];
        va_list args;
        va_start(args, format);
        vsnprintf(buf, kTraceBufferLen, format, args);
        va_end(args);

        parseContext->trace(buf);
    }
}
}  // extern "C"
#endif  // TRACE_ENABLED

