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

#ifndef PrintStream_h
#define PrintStream_h

#include <stdarg.h>
#include <wtf/FastMalloc.h>
#include <wtf/Noncopyable.h>
#include <wtf/RawPointer.h>
#include <wtf/StdLibExtras.h>

namespace WTF {

class CString;
class String;
class StringImpl;

class PrintStream {
    WTF_MAKE_FAST_ALLOCATED; WTF_MAKE_NONCOPYABLE(PrintStream);
public:
    PrintStream();
    virtual ~PrintStream();

    WTF_EXPORT_PRIVATE void printf(const char* format, ...) WTF_ATTRIBUTE_PRINTF(2, 3);
    virtual void vprintf(const char* format, va_list) WTF_ATTRIBUTE_PRINTF(2, 0) = 0;

    // Typically a no-op for many subclasses of PrintStream, this is a hint that
    // the implementation should flush its buffers if it had not done so already.
    virtual void flush();

    template<typename T>
    void print(const T& value)
    {
        printInternal(*this, value);
    }

    template<typename T, typename... Types>
    void print(const T& value, const Types&... remainingValues)
    {
        printInternal(*this, value);
        print(remainingValues...);
    }
};

WTF_EXPORT_PRIVATE void printInternal(PrintStream&, const char*);
WTF_EXPORT_PRIVATE void printInternal(PrintStream&, const CString&);
WTF_EXPORT_PRIVATE void printInternal(PrintStream&, const String&);
WTF_EXPORT_PRIVATE void printInternal(PrintStream&, const StringImpl*);
inline void printInternal(PrintStream& out, char* value) { printInternal(out, static_cast<const char*>(value)); }
inline void printInternal(PrintStream& out, CString& value) { printInternal(out, static_cast<const CString&>(value)); }
inline void printInternal(PrintStream& out, String& value) { printInternal(out, static_cast<const String&>(value)); }
inline void printInternal(PrintStream& out, StringImpl* value) { printInternal(out, static_cast<const StringImpl*>(value)); }
WTF_EXPORT_PRIVATE void printInternal(PrintStream&, bool);
WTF_EXPORT_PRIVATE void printInternal(PrintStream&, signed char); // NOTE: this prints as a number, not as a character; use CharacterDump if you want the character
WTF_EXPORT_PRIVATE void printInternal(PrintStream&, unsigned char); // NOTE: see above.
WTF_EXPORT_PRIVATE void printInternal(PrintStream&, short);
WTF_EXPORT_PRIVATE void printInternal(PrintStream&, unsigned short);
WTF_EXPORT_PRIVATE void printInternal(PrintStream&, int);
WTF_EXPORT_PRIVATE void printInternal(PrintStream&, unsigned);
WTF_EXPORT_PRIVATE void printInternal(PrintStream&, long);
WTF_EXPORT_PRIVATE void printInternal(PrintStream&, unsigned long);
WTF_EXPORT_PRIVATE void printInternal(PrintStream&, long long);
WTF_EXPORT_PRIVATE void printInternal(PrintStream&, unsigned long long);
WTF_EXPORT_PRIVATE void printInternal(PrintStream&, float);
WTF_EXPORT_PRIVATE void printInternal(PrintStream&, double);
WTF_EXPORT_PRIVATE void printInternal(PrintStream&, RawPointer);

template<typename T>
void printInternal(PrintStream& out, const T& value)
{
    value.dump(out);
}

#define MAKE_PRINT_ADAPTOR(Name, Type, function) \
    class Name {                                 \
    public:                                      \
        Name(const Type& value)                  \
            : m_value(value)                     \
        {                                        \
        }                                        \
        void dump(PrintStream& out) const        \
        {                                        \
            function(out, m_value);              \
        }                                        \
    private:                                     \
        Type m_value;                            \
    }

#define MAKE_PRINT_METHOD_ADAPTOR(Name, Type, method) \
    class Name {                                 \
    public:                                      \
        Name(const Type& value)                  \
            : m_value(value)                     \
        {                                        \
        }                                        \
        void dump(PrintStream& out) const        \
        {                                        \
            m_value.method(out);                 \
        }                                        \
    private:                                     \
        const Type& m_value;                     \
    }

#define MAKE_PRINT_METHOD(Type, dumpMethod, method) \
    MAKE_PRINT_METHOD_ADAPTOR(DumperFor_##method, Type, dumpMethod);    \
    DumperFor_##method method() const { return DumperFor_##method(*this); }

// Use an adaptor-based dumper for characters to avoid situations where
// you've "compressed" an integer to a character and it ends up printing
// as ASCII when you wanted it to print as a number.
WTF_EXPORT_PRIVATE void dumpCharacter(PrintStream&, char);
MAKE_PRINT_ADAPTOR(CharacterDump, char, dumpCharacter);

template<typename T>
class PointerDump {
public:
    PointerDump(const T* ptr)
        : m_ptr(ptr)
    {
    }
    
    void dump(PrintStream& out) const
    {
        if (m_ptr)
            printInternal(out, *m_ptr);
        else
            out.print("(null)");
    }
private:
    const T* m_ptr;
};

template<typename T>
PointerDump<T> pointerDump(const T* ptr) { return PointerDump<T>(ptr); }

template<typename T, typename U>
class ValueInContext {
public:
    ValueInContext(const T& value, U* context)
        : m_value(&value)
        , m_context(context)
    {
    }
    
    void dump(PrintStream& out) const
    {
        m_value->dumpInContext(out, m_context);
    }

private:
    const T* m_value;
    U* m_context;
};

template<typename T, typename U>
ValueInContext<T, U> inContext(const T& value, U* context)
{
    return ValueInContext<T, U>(value, context);
}

template<typename T, typename U>
class PointerDumpInContext {
public:
    PointerDumpInContext(const T* ptr, U* context)
        : m_ptr(ptr)
        , m_context(context)
    {
    }
    
    void dump(PrintStream& out) const
    {
        if (m_ptr)
            m_ptr->dumpInContext(out, m_context);
        else
            out.print("(null)");
    }

private:
    const T* m_ptr;
    U* m_context;
};

template<typename T, typename U>
PointerDumpInContext<T, U> pointerDumpInContext(const T* ptr, U* context)
{
    return PointerDumpInContext<T, U>(ptr, context);
}

template<typename T, typename U>
class ValueIgnoringContext {
public:
    ValueIgnoringContext(const U& value)
        : m_value(&value)
    {
    }
    
    void dump(PrintStream& out) const
    {
        T context;
        m_value->dumpInContext(out, &context);
    }

private:
    const U* m_value;
};

template<typename T, typename U>
ValueIgnoringContext<T, U> ignoringContext(const U& value)
{
    return ValueIgnoringContext<T, U>(value);
}

} // namespace WTF

using WTF::CharacterDump;
using WTF::PointerDump;
using WTF::PrintStream;
using WTF::ignoringContext;
using WTF::inContext;
using WTF::pointerDump;
using WTF::pointerDumpInContext;

#endif // PrintStream_h

