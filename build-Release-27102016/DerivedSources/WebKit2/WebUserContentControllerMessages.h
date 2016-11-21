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

#ifndef WebUserContentControllerMessages_h
#define WebUserContentControllerMessages_h

#include "Arguments.h"
#include "MessageEncoder.h"
#include "StringReference.h"
#include "WebCompiledContentExtensionData.h"
#include "WebUserContentControllerDataTypes.h"
#include <utility>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace WTF {
    class String;
}

namespace Messages {
namespace WebUserContentController {

static inline IPC::StringReference messageReceiverName()
{
    return IPC::StringReference("WebUserContentController");
}

class AddUserContentWorlds {
public:
    typedef std::tuple<Vector<std::pair<uint64_t, String>>> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("AddUserContentWorlds"); }
    static const bool isSync = false;

    explicit AddUserContentWorlds(const Vector<std::pair<uint64_t, String>>& worlds)
        : m_arguments(worlds)
    {
    }

    const std::tuple<const Vector<std::pair<uint64_t, String>>&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const Vector<std::pair<uint64_t, String>>&> m_arguments;
};

class RemoveUserContentWorlds {
public:
    typedef std::tuple<Vector<uint64_t>> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("RemoveUserContentWorlds"); }
    static const bool isSync = false;

    explicit RemoveUserContentWorlds(const Vector<uint64_t>& worldIdentifiers)
        : m_arguments(worldIdentifiers)
    {
    }

    const std::tuple<const Vector<uint64_t>&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const Vector<uint64_t>&> m_arguments;
};

class AddUserScripts {
public:
    typedef std::tuple<Vector<struct WebKit::WebUserScriptData>> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("AddUserScripts"); }
    static const bool isSync = false;

    explicit AddUserScripts(const Vector<struct WebKit::WebUserScriptData>& userScripts)
        : m_arguments(userScripts)
    {
    }

    const std::tuple<const Vector<struct WebKit::WebUserScriptData>&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const Vector<struct WebKit::WebUserScriptData>&> m_arguments;
};

class RemoveUserScript {
public:
    typedef std::tuple<uint64_t, uint64_t> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("RemoveUserScript"); }
    static const bool isSync = false;

    RemoveUserScript(uint64_t worldIdentifier, uint64_t identifier)
        : m_arguments(worldIdentifier, identifier)
    {
    }

    const std::tuple<uint64_t, uint64_t>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<uint64_t, uint64_t> m_arguments;
};

class RemoveAllUserScripts {
public:
    typedef std::tuple<Vector<uint64_t>> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("RemoveAllUserScripts"); }
    static const bool isSync = false;

    explicit RemoveAllUserScripts(const Vector<uint64_t>& worldIdentifiers)
        : m_arguments(worldIdentifiers)
    {
    }

    const std::tuple<const Vector<uint64_t>&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const Vector<uint64_t>&> m_arguments;
};

class AddUserStyleSheets {
public:
    typedef std::tuple<Vector<struct WebKit::WebUserStyleSheetData>> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("AddUserStyleSheets"); }
    static const bool isSync = false;

    explicit AddUserStyleSheets(const Vector<struct WebKit::WebUserStyleSheetData>& userStyleSheets)
        : m_arguments(userStyleSheets)
    {
    }

    const std::tuple<const Vector<struct WebKit::WebUserStyleSheetData>&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const Vector<struct WebKit::WebUserStyleSheetData>&> m_arguments;
};

class RemoveUserStyleSheet {
public:
    typedef std::tuple<uint64_t, uint64_t> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("RemoveUserStyleSheet"); }
    static const bool isSync = false;

    RemoveUserStyleSheet(uint64_t worldIdentifier, uint64_t identifier)
        : m_arguments(worldIdentifier, identifier)
    {
    }

    const std::tuple<uint64_t, uint64_t>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<uint64_t, uint64_t> m_arguments;
};

class RemoveAllUserStyleSheets {
public:
    typedef std::tuple<Vector<uint64_t>> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("RemoveAllUserStyleSheets"); }
    static const bool isSync = false;

    explicit RemoveAllUserStyleSheets(const Vector<uint64_t>& worldIdentifiers)
        : m_arguments(worldIdentifiers)
    {
    }

    const std::tuple<const Vector<uint64_t>&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const Vector<uint64_t>&> m_arguments;
};

class AddUserScriptMessageHandlers {
public:
    typedef std::tuple<Vector<struct WebKit::WebScriptMessageHandlerData>> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("AddUserScriptMessageHandlers"); }
    static const bool isSync = false;

    explicit AddUserScriptMessageHandlers(const Vector<struct WebKit::WebScriptMessageHandlerData>& scriptMessageHandlers)
        : m_arguments(scriptMessageHandlers)
    {
    }

    const std::tuple<const Vector<struct WebKit::WebScriptMessageHandlerData>&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const Vector<struct WebKit::WebScriptMessageHandlerData>&> m_arguments;
};

class RemoveUserScriptMessageHandler {
public:
    typedef std::tuple<uint64_t, uint64_t> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("RemoveUserScriptMessageHandler"); }
    static const bool isSync = false;

    RemoveUserScriptMessageHandler(uint64_t worldIdentifier, uint64_t identifier)
        : m_arguments(worldIdentifier, identifier)
    {
    }

    const std::tuple<uint64_t, uint64_t>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<uint64_t, uint64_t> m_arguments;
};

class RemoveAllUserScriptMessageHandlers {
public:
    typedef std::tuple<Vector<uint64_t>> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("RemoveAllUserScriptMessageHandlers"); }
    static const bool isSync = false;

    explicit RemoveAllUserScriptMessageHandlers(const Vector<uint64_t>& worldIdentifiers)
        : m_arguments(worldIdentifiers)
    {
    }

    const std::tuple<const Vector<uint64_t>&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const Vector<uint64_t>&> m_arguments;
};

#if ENABLE(CONTENT_EXTENSIONS)
class AddUserContentExtensions {
public:
    typedef std::tuple<Vector<std::pair<String, WebKit::WebCompiledContentExtensionData>>> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("AddUserContentExtensions"); }
    static const bool isSync = false;

    explicit AddUserContentExtensions(const Vector<std::pair<String, WebKit::WebCompiledContentExtensionData>>& userContentFilters)
        : m_arguments(userContentFilters)
    {
    }

    const std::tuple<const Vector<std::pair<String, WebKit::WebCompiledContentExtensionData>>&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const Vector<std::pair<String, WebKit::WebCompiledContentExtensionData>>&> m_arguments;
};
#endif

#if ENABLE(CONTENT_EXTENSIONS)
class RemoveUserContentExtension {
public:
    typedef std::tuple<String> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("RemoveUserContentExtension"); }
    static const bool isSync = false;

    explicit RemoveUserContentExtension(const String& name)
        : m_arguments(name)
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

#if ENABLE(CONTENT_EXTENSIONS)
class RemoveAllUserContentExtensions {
public:
    typedef std::tuple<> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("RemoveAllUserContentExtensions"); }
    static const bool isSync = false;

    const std::tuple<>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<> m_arguments;
};
#endif

} // namespace WebUserContentController
} // namespace Messages

#endif // WebUserContentControllerMessages_h
