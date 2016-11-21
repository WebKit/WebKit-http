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

#ifndef NPObjectMessageReceiverMessages_h
#define NPObjectMessageReceiverMessages_h

#if ENABLE(NETSCAPE_PLUGIN_API)

#include "Arguments.h"
#include "MessageEncoder.h"
#include "NPIdentifierData.h"
#include "NPVariantData.h"
#include "StringReference.h"
#include <wtf/Vector.h>

namespace WebKit {
    class NPIdentifierData;
    class NPVariantData;
}

namespace Messages {
namespace NPObjectMessageReceiver {

static inline IPC::StringReference messageReceiverName()
{
    return IPC::StringReference("NPObjectMessageReceiver");
}

class Deallocate {
public:
    typedef std::tuple<> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("Deallocate"); }
    static const bool isSync = true;

    typedef IPC::Arguments<> Reply;
    const std::tuple<>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<> m_arguments;
};

class HasMethod {
public:
    typedef std::tuple<WebKit::NPIdentifierData> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("HasMethod"); }
    static const bool isSync = true;

    typedef IPC::Arguments<bool&> Reply;
    explicit HasMethod(const WebKit::NPIdentifierData& methodName)
        : m_arguments(methodName)
    {
    }

    const std::tuple<const WebKit::NPIdentifierData&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const WebKit::NPIdentifierData&> m_arguments;
};

class Invoke {
public:
    typedef std::tuple<WebKit::NPIdentifierData, Vector<WebKit::NPVariantData>> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("Invoke"); }
    static const bool isSync = true;

    typedef IPC::Arguments<bool&, WebKit::NPVariantData&> Reply;
    Invoke(const WebKit::NPIdentifierData& methodName, const Vector<WebKit::NPVariantData>& argumentsData)
        : m_arguments(methodName, argumentsData)
    {
    }

    const std::tuple<const WebKit::NPIdentifierData&, const Vector<WebKit::NPVariantData>&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const WebKit::NPIdentifierData&, const Vector<WebKit::NPVariantData>&> m_arguments;
};

class InvokeDefault {
public:
    typedef std::tuple<Vector<WebKit::NPVariantData>> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("InvokeDefault"); }
    static const bool isSync = true;

    typedef IPC::Arguments<bool&, WebKit::NPVariantData&> Reply;
    explicit InvokeDefault(const Vector<WebKit::NPVariantData>& argumentsData)
        : m_arguments(argumentsData)
    {
    }

    const std::tuple<const Vector<WebKit::NPVariantData>&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const Vector<WebKit::NPVariantData>&> m_arguments;
};

class HasProperty {
public:
    typedef std::tuple<WebKit::NPIdentifierData> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("HasProperty"); }
    static const bool isSync = true;

    typedef IPC::Arguments<bool&> Reply;
    explicit HasProperty(const WebKit::NPIdentifierData& propertyName)
        : m_arguments(propertyName)
    {
    }

    const std::tuple<const WebKit::NPIdentifierData&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const WebKit::NPIdentifierData&> m_arguments;
};

class GetProperty {
public:
    typedef std::tuple<WebKit::NPIdentifierData> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("GetProperty"); }
    static const bool isSync = true;

    typedef IPC::Arguments<bool&, WebKit::NPVariantData&> Reply;
    explicit GetProperty(const WebKit::NPIdentifierData& propertyName)
        : m_arguments(propertyName)
    {
    }

    const std::tuple<const WebKit::NPIdentifierData&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const WebKit::NPIdentifierData&> m_arguments;
};

class SetProperty {
public:
    typedef std::tuple<WebKit::NPIdentifierData, WebKit::NPVariantData> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetProperty"); }
    static const bool isSync = true;

    typedef IPC::Arguments<bool&> Reply;
    SetProperty(const WebKit::NPIdentifierData& propertyName, const WebKit::NPVariantData& propertyValueData)
        : m_arguments(propertyName, propertyValueData)
    {
    }

    const std::tuple<const WebKit::NPIdentifierData&, const WebKit::NPVariantData&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const WebKit::NPIdentifierData&, const WebKit::NPVariantData&> m_arguments;
};

class RemoveProperty {
public:
    typedef std::tuple<WebKit::NPIdentifierData> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("RemoveProperty"); }
    static const bool isSync = true;

    typedef IPC::Arguments<bool&> Reply;
    explicit RemoveProperty(const WebKit::NPIdentifierData& propertyName)
        : m_arguments(propertyName)
    {
    }

    const std::tuple<const WebKit::NPIdentifierData&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const WebKit::NPIdentifierData&> m_arguments;
};

class Enumerate {
public:
    typedef std::tuple<> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("Enumerate"); }
    static const bool isSync = true;

    typedef IPC::Arguments<bool&, Vector<WebKit::NPIdentifierData>&> Reply;
    const std::tuple<>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<> m_arguments;
};

class Construct {
public:
    typedef std::tuple<Vector<WebKit::NPVariantData>> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("Construct"); }
    static const bool isSync = true;

    typedef IPC::Arguments<bool&, WebKit::NPVariantData&> Reply;
    explicit Construct(const Vector<WebKit::NPVariantData>& argumentsData)
        : m_arguments(argumentsData)
    {
    }

    const std::tuple<const Vector<WebKit::NPVariantData>&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const Vector<WebKit::NPVariantData>&> m_arguments;
};

} // namespace NPObjectMessageReceiver
} // namespace Messages

#endif // ENABLE(NETSCAPE_PLUGIN_API)

#endif // NPObjectMessageReceiverMessages_h
