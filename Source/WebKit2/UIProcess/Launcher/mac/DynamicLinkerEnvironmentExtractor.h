/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

#ifndef DynamicLinkerEnvironmentExtractor_h
#define DynamicLinkerEnvironmentExtractor_h

#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1070

#include <mach/machine.h>
#include <wtf/Noncopyable.h>
#include <wtf/RetainPtr.h>
#include <wtf/Vector.h>
#include <wtf/text/CString.h>

namespace WebKit {

class EnvironmentVariables;

class DynamicLinkerEnvironmentExtractor {
    WTF_MAKE_NONCOPYABLE(DynamicLinkerEnvironmentExtractor);

public:
    DynamicLinkerEnvironmentExtractor(NSString *executablePath, cpu_type_t architecture);

    void getExtractedEnvironmentVariables(EnvironmentVariables&) const;

private:
    void processSingleArchitecture(const void* data, size_t length);
    void processFatFile(const void* data, size_t length);
    void processLoadCommands(const void* data, size_t length, int32_t numberOfCommands, bool shouldByteSwap);
    size_t processLoadCommand(const void* data, size_t length, bool shouldByteSwap);
    void processEnvironmentVariable(const char* environmentString);

    RetainPtr<NSString> m_executablePath;
    cpu_type_t m_architecture;

    Vector<std::pair<CString, CString> > m_extractedVariables;
};

} // namespace WebKit

#endif // __MAC_OS_X_VERSION_MIN_REQUIRED == 1060

#endif // DynamicLinkerEnvironmentExtractor_h
