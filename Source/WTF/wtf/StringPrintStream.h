/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef StringPrintStream_h
#define StringPrintStream_h

#include <wtf/PrintStream.h>
#include <wtf/text/CString.h>

namespace WTF {

class StringPrintStream : public PrintStream {
public:
    StringPrintStream();
    ~StringPrintStream();
    
    virtual void vprintf(const char* format, va_list) WTF_ATTRIBUTE_PRINTF(2, 0);
    
    CString toCString();
    
private:
    void increaseSize(size_t);
    
    char* m_buffer;
    size_t m_next;
    size_t m_size;
    char m_inlineBuffer[128];
};

// Stringify any type T that has a WTF::printInternal(PrintStream&, const T&)
template<typename T>
CString toCString(const T& value)
{
    StringPrintStream stream;
    stream.print(value);
    return stream.toCString();
}

} // namespace WTF

using WTF::StringPrintStream;
using WTF::toCString;

#endif // StringPrintStream_h

