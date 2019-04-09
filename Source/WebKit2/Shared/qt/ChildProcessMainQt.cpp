/*
 * Copyright (C) 2016 Konstantin Tokarev <annulen@yandex.ru>
 * Copyright (C) 2014 Igalia S.L.
 * Copyright (C) 2013 University of Szeged
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
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

#include "config.h"
#include "ChildProcessMain.h"

#include <QDebug>

#if OS(DARWIN) && !USE(UNIX_DOMAIN_SOCKETS)
#include <servers/bootstrap.h>

extern "C" kern_return_t bootstrap_look_up2(mach_port_t, const name_t, mach_port_t*, pid_t, uint64_t);
#endif

namespace WebKit {

bool ChildProcessMainBase::parseCommandLine(int argc, char** argv)
{
    ASSERT(argc >= 2);
    if (argc < 2)
        return false;

#if USE(MACH_PORTS)
    QByteArray serviceName(argv[1]);

    // Get the server port.
    mach_port_t identifier;
    kern_return_t kr = bootstrap_look_up2(bootstrap_port, serviceName.data(), &identifier, 0, 0);
    if (kr) {
        printf("bootstrap_look_up2 result: %x", kr);
        return false;
    }
#else
    bool wasNumber = false;
    qulonglong id = QByteArray(argv[1]).toULongLong(&wasNumber, 10);
    if (!wasNumber) {
        qDebug() << "Error: connection identifier wrong.";
        return 1;
    }
    IPC::Connection::Identifier identifier;
#if OS(WINDOWS)
    // Convert to HANDLE
    identifier = reinterpret_cast<IPC::Connection::Identifier>(id);
#else
    // Convert to int
    identifier = static_cast<IPC::Connection::Identifier>(id);
#endif
#endif

    m_parameters.connectionIdentifier = identifier;
    return true;
}

} // namespace WebKit
