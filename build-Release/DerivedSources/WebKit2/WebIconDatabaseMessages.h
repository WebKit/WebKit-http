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

#pragma once

#include "ArgumentCoders.h"

namespace IPC {
    class DataReference;
}

namespace WTF {
    class String;
}

namespace Messages {
namespace WebIconDatabase {

static inline IPC::StringReference messageReceiverName()
{
    return IPC::StringReference("WebIconDatabase");
}

class SetIconURLForPageURL {
public:
    typedef std::tuple<const String&, const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetIconURLForPageURL"); }
    static const bool isSync = false;

    SetIconURLForPageURL(const String& iconURL, const String& pageURL)
        : m_arguments(iconURL, pageURL)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class SetIconDataForIconURL {
public:
    typedef std::tuple<const IPC::DataReference&, const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetIconDataForIconURL"); }
    static const bool isSync = false;

    SetIconDataForIconURL(const IPC::DataReference& iconData, const String& iconURL)
        : m_arguments(iconData, iconURL)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class SynchronousIconDataForPageURL {
public:
    typedef std::tuple<const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SynchronousIconDataForPageURL"); }
    static const bool isSync = true;

    typedef std::tuple<IPC::DataReference&> Reply;
    explicit SynchronousIconDataForPageURL(const String& pageURL)
        : m_arguments(pageURL)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class SynchronousIconURLForPageURL {
public:
    typedef std::tuple<const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SynchronousIconURLForPageURL"); }
    static const bool isSync = true;

    typedef std::tuple<String&> Reply;
    explicit SynchronousIconURLForPageURL(const String& pageURL)
        : m_arguments(pageURL)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class SynchronousIconDataKnownForIconURL {
public:
    typedef std::tuple<const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SynchronousIconDataKnownForIconURL"); }
    static const bool isSync = true;

    typedef std::tuple<bool&> Reply;
    explicit SynchronousIconDataKnownForIconURL(const String& iconURL)
        : m_arguments(iconURL)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class SynchronousLoadDecisionForIconURL {
public:
    typedef std::tuple<const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SynchronousLoadDecisionForIconURL"); }
    static const bool isSync = true;

    typedef std::tuple<int&> Reply;
    explicit SynchronousLoadDecisionForIconURL(const String& iconURL)
        : m_arguments(iconURL)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class GetLoadDecisionForIconURL {
public:
    typedef std::tuple<const String&, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("GetLoadDecisionForIconURL"); }
    static const bool isSync = false;

    GetLoadDecisionForIconURL(const String& iconURL, uint64_t callbackID)
        : m_arguments(iconURL, callbackID)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidReceiveIconForPageURL {
public:
    typedef std::tuple<const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidReceiveIconForPageURL"); }
    static const bool isSync = false;

    explicit DidReceiveIconForPageURL(const String& pageURL)
        : m_arguments(pageURL)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

} // namespace WebIconDatabase
} // namespace Messages
