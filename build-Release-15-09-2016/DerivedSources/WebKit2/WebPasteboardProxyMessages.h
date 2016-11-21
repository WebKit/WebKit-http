/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WebPasteboardProxyMessages_h
#define WebPasteboardProxyMessages_h

#include "Arguments.h"
#include "MessageEncoder.h"
#include "SharedMemory.h"
#include "StringReference.h"
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace WTF {
    class String;
}

namespace WebCore {
    struct PasteboardWebContent;
    struct PasteboardImage;
    class Color;
}

namespace Messages {
namespace WebPasteboardProxy {

static inline IPC::StringReference messageReceiverName()
{
    return IPC::StringReference("WebPasteboardProxy");
}

#if PLATFORM(IOS)
class WriteWebContentToPasteboard {
public:
    typedef std::tuple<WebCore::PasteboardWebContent> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("WriteWebContentToPasteboard"); }
    static const bool isSync = false;

    explicit WriteWebContentToPasteboard(const WebCore::PasteboardWebContent& content)
        : m_arguments(content)
    {
    }

    const std::tuple<const WebCore::PasteboardWebContent&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const WebCore::PasteboardWebContent&> m_arguments;
};
#endif

#if PLATFORM(IOS)
class WriteImageToPasteboard {
public:
    typedef std::tuple<WebCore::PasteboardImage> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("WriteImageToPasteboard"); }
    static const bool isSync = false;

    explicit WriteImageToPasteboard(const WebCore::PasteboardImage& pasteboardImage)
        : m_arguments(pasteboardImage)
    {
    }

    const std::tuple<const WebCore::PasteboardImage&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const WebCore::PasteboardImage&> m_arguments;
};
#endif

#if PLATFORM(IOS)
class WriteStringToPasteboard {
public:
    typedef std::tuple<String, String> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("WriteStringToPasteboard"); }
    static const bool isSync = false;

    WriteStringToPasteboard(const String& pasteboardType, const String& text)
        : m_arguments(pasteboardType, text)
    {
    }

    const std::tuple<const String&, const String&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const String&, const String&> m_arguments;
};
#endif

#if PLATFORM(IOS)
class ReadStringFromPasteboard {
public:
    typedef std::tuple<uint64_t, String> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ReadStringFromPasteboard"); }
    static const bool isSync = true;

    typedef IPC::Arguments<String&> Reply;
    ReadStringFromPasteboard(uint64_t index, const String& pasteboardType)
        : m_arguments(index, pasteboardType)
    {
    }

    const std::tuple<uint64_t, const String&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<uint64_t, const String&> m_arguments;
};
#endif

#if PLATFORM(IOS)
class ReadURLFromPasteboard {
public:
    typedef std::tuple<uint64_t, String> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ReadURLFromPasteboard"); }
    static const bool isSync = true;

    typedef IPC::Arguments<String&> Reply;
    ReadURLFromPasteboard(uint64_t index, const String& pasteboardType)
        : m_arguments(index, pasteboardType)
    {
    }

    const std::tuple<uint64_t, const String&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<uint64_t, const String&> m_arguments;
};
#endif

#if PLATFORM(IOS)
class ReadBufferFromPasteboard {
public:
    typedef std::tuple<uint64_t, String> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ReadBufferFromPasteboard"); }
    static const bool isSync = true;

    typedef IPC::Arguments<WebKit::SharedMemory::Handle&, uint64_t&> Reply;
    ReadBufferFromPasteboard(uint64_t index, const String& pasteboardType)
        : m_arguments(index, pasteboardType)
    {
    }

    const std::tuple<uint64_t, const String&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<uint64_t, const String&> m_arguments;
};
#endif

#if PLATFORM(IOS)
class GetPasteboardItemsCount {
public:
    typedef std::tuple<> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("GetPasteboardItemsCount"); }
    static const bool isSync = true;

    typedef IPC::Arguments<uint64_t&> Reply;
    const std::tuple<>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<> m_arguments;
};
#endif

#if PLATFORM(COCOA)
class GetPasteboardTypes {
public:
    typedef std::tuple<String> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("GetPasteboardTypes"); }
    static const bool isSync = true;

    typedef IPC::Arguments<Vector<String>&> Reply;
    explicit GetPasteboardTypes(const String& pasteboardName)
        : m_arguments(pasteboardName)
    {
    }

    const std::tuple<const String&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const String&> m_arguments;
};
#endif

#if PLATFORM(COCOA)
class GetPasteboardPathnamesForType {
public:
    typedef std::tuple<String, String> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("GetPasteboardPathnamesForType"); }
    static const bool isSync = true;

    typedef IPC::Arguments<Vector<String>&> Reply;
    GetPasteboardPathnamesForType(const String& pasteboardName, const String& pasteboardType)
        : m_arguments(pasteboardName, pasteboardType)
    {
    }

    const std::tuple<const String&, const String&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const String&, const String&> m_arguments;
};
#endif

#if PLATFORM(COCOA)
class GetPasteboardStringForType {
public:
    typedef std::tuple<String, String> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("GetPasteboardStringForType"); }
    static const bool isSync = true;

    typedef IPC::Arguments<String&> Reply;
    GetPasteboardStringForType(const String& pasteboardName, const String& pasteboardType)
        : m_arguments(pasteboardName, pasteboardType)
    {
    }

    const std::tuple<const String&, const String&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const String&, const String&> m_arguments;
};
#endif

#if PLATFORM(COCOA)
class GetPasteboardBufferForType {
public:
    typedef std::tuple<String, String> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("GetPasteboardBufferForType"); }
    static const bool isSync = true;

    typedef IPC::Arguments<WebKit::SharedMemory::Handle&, uint64_t&> Reply;
    GetPasteboardBufferForType(const String& pasteboardName, const String& pasteboardType)
        : m_arguments(pasteboardName, pasteboardType)
    {
    }

    const std::tuple<const String&, const String&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const String&, const String&> m_arguments;
};
#endif

#if PLATFORM(COCOA)
class PasteboardCopy {
public:
    typedef std::tuple<String, String> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("PasteboardCopy"); }
    static const bool isSync = true;

    typedef IPC::Arguments<uint64_t&> Reply;
    PasteboardCopy(const String& fromPasteboard, const String& toPasteboard)
        : m_arguments(fromPasteboard, toPasteboard)
    {
    }

    const std::tuple<const String&, const String&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const String&, const String&> m_arguments;
};
#endif

#if PLATFORM(COCOA)
class GetPasteboardChangeCount {
public:
    typedef std::tuple<String> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("GetPasteboardChangeCount"); }
    static const bool isSync = true;

    typedef IPC::Arguments<uint64_t&> Reply;
    explicit GetPasteboardChangeCount(const String& pasteboardName)
        : m_arguments(pasteboardName)
    {
    }

    const std::tuple<const String&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const String&> m_arguments;
};
#endif

#if PLATFORM(COCOA)
class GetPasteboardUniqueName {
public:
    typedef std::tuple<> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("GetPasteboardUniqueName"); }
    static const bool isSync = true;

    typedef IPC::Arguments<String&> Reply;
    const std::tuple<>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<> m_arguments;
};
#endif

#if PLATFORM(COCOA)
class GetPasteboardColor {
public:
    typedef std::tuple<String> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("GetPasteboardColor"); }
    static const bool isSync = true;

    typedef IPC::Arguments<WebCore::Color&> Reply;
    explicit GetPasteboardColor(const String& pasteboardName)
        : m_arguments(pasteboardName)
    {
    }

    const std::tuple<const String&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const String&> m_arguments;
};
#endif

#if PLATFORM(COCOA)
class GetPasteboardURL {
public:
    typedef std::tuple<String> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("GetPasteboardURL"); }
    static const bool isSync = true;

    typedef IPC::Arguments<String&> Reply;
    explicit GetPasteboardURL(const String& pasteboardName)
        : m_arguments(pasteboardName)
    {
    }

    const std::tuple<const String&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const String&> m_arguments;
};
#endif

#if PLATFORM(COCOA)
class AddPasteboardTypes {
public:
    typedef std::tuple<String, Vector<String>> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("AddPasteboardTypes"); }
    static const bool isSync = true;

    typedef IPC::Arguments<uint64_t&> Reply;
    AddPasteboardTypes(const String& pasteboardName, const Vector<String>& pasteboardTypes)
        : m_arguments(pasteboardName, pasteboardTypes)
    {
    }

    const std::tuple<const String&, const Vector<String>&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const String&, const Vector<String>&> m_arguments;
};
#endif

#if PLATFORM(COCOA)
class SetPasteboardTypes {
public:
    typedef std::tuple<String, Vector<String>> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetPasteboardTypes"); }
    static const bool isSync = true;

    typedef IPC::Arguments<uint64_t&> Reply;
    SetPasteboardTypes(const String& pasteboardName, const Vector<String>& pasteboardTypes)
        : m_arguments(pasteboardName, pasteboardTypes)
    {
    }

    const std::tuple<const String&, const Vector<String>&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const String&, const Vector<String>&> m_arguments;
};
#endif

#if PLATFORM(COCOA)
class SetPasteboardPathnamesForType {
public:
    typedef std::tuple<String, String, Vector<String>> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetPasteboardPathnamesForType"); }
    static const bool isSync = true;

    typedef IPC::Arguments<uint64_t&> Reply;
    SetPasteboardPathnamesForType(const String& pasteboardName, const String& pasteboardType, const Vector<String>& pathnames)
        : m_arguments(pasteboardName, pasteboardType, pathnames)
    {
    }

    const std::tuple<const String&, const String&, const Vector<String>&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const String&, const String&, const Vector<String>&> m_arguments;
};
#endif

#if PLATFORM(COCOA)
class SetPasteboardStringForType {
public:
    typedef std::tuple<String, String, String> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetPasteboardStringForType"); }
    static const bool isSync = true;

    typedef IPC::Arguments<uint64_t&> Reply;
    SetPasteboardStringForType(const String& pasteboardName, const String& pasteboardType, const String& string)
        : m_arguments(pasteboardName, pasteboardType, string)
    {
    }

    const std::tuple<const String&, const String&, const String&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const String&, const String&, const String&> m_arguments;
};
#endif

#if PLATFORM(COCOA)
class SetPasteboardBufferForType {
public:
    typedef std::tuple<String, String, WebKit::SharedMemory::Handle, uint64_t> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetPasteboardBufferForType"); }
    static const bool isSync = true;

    typedef IPC::Arguments<uint64_t&> Reply;
    SetPasteboardBufferForType(const String& pasteboardName, const String& pasteboardType, const WebKit::SharedMemory::Handle& handle, uint64_t size)
        : m_arguments(pasteboardName, pasteboardType, handle, size)
    {
    }

    const std::tuple<const String&, const String&, const WebKit::SharedMemory::Handle&, uint64_t>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const String&, const String&, const WebKit::SharedMemory::Handle&, uint64_t> m_arguments;
};
#endif

#if PLATFORM(WPE)
class GetPasteboardTypes {
public:
    typedef std::tuple<> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("GetPasteboardTypes"); }
    static const bool isSync = true;

    typedef IPC::Arguments<Vector<String>&> Reply;
    const std::tuple<>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<> m_arguments;
};
#endif

#if PLATFORM(WPE)
class ReadStringFromPasteboard {
public:
    typedef std::tuple<uint64_t, String> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ReadStringFromPasteboard"); }
    static const bool isSync = true;

    typedef IPC::Arguments<String&> Reply;
    ReadStringFromPasteboard(uint64_t index, const String& pasteboardType)
        : m_arguments(index, pasteboardType)
    {
    }

    const std::tuple<uint64_t, const String&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<uint64_t, const String&> m_arguments;
};
#endif

#if PLATFORM(WPE)
class WriteWebContentToPasteboard {
public:
    typedef std::tuple<WebCore::PasteboardWebContent> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("WriteWebContentToPasteboard"); }
    static const bool isSync = false;

    explicit WriteWebContentToPasteboard(const WebCore::PasteboardWebContent& content)
        : m_arguments(content)
    {
    }

    const std::tuple<const WebCore::PasteboardWebContent&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const WebCore::PasteboardWebContent&> m_arguments;
};
#endif

#if PLATFORM(WPE)
class WriteStringToPasteboard {
public:
    typedef std::tuple<String, String> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("WriteStringToPasteboard"); }
    static const bool isSync = false;

    WriteStringToPasteboard(const String& pasteboardType, const String& text)
        : m_arguments(pasteboardType, text)
    {
    }

    const std::tuple<const String&, const String&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const String&, const String&> m_arguments;
};
#endif

} // namespace WebPasteboardProxy
} // namespace Messages

#endif // WebPasteboardProxyMessages_h
