/*
 * Copyright (C) 2011 Nokia Inc. All rights reserved.
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
#include "PluginProcessProxy.h"

#if ENABLE(PLUGIN_PROCESS)

#include "ProcessExecutablePath.h"
#include <QByteArray>
#include <QCoreApplication>
#include <QDir>
#include <QEventLoop>
#include <QProcess>
#include <QString>

namespace WebKit {

class PluginProcessCreationParameters;

void PluginProcessProxy::platformInitializePluginProcess(PluginProcessCreationParameters&)
{
}

bool PluginProcessProxy::scanPlugin(const String& pluginPath, RawPluginMetaData& result)
{
    QString commandLine = QLatin1String("%1 %2 %3");
    commandLine = commandLine.arg(executablePathOfPluginProcess());
    commandLine = commandLine.arg(QStringLiteral("-scanPlugin")).arg(static_cast<QString>(pluginPath));

    QProcess process;
    process.setReadChannel(QProcess::StandardOutput);
    process.start(commandLine);

    if (!process.waitForFinished()
        || process.exitStatus() != QProcess::NormalExit
        || process.exitCode() != EXIT_SUCCESS) {
        process.kill();
        return false;
    }

    QByteArray outputBytes = process.readAll();
    ASSERT(!(outputBytes.size() % sizeof(UChar)));

    String output(reinterpret_cast<const UChar*>(outputBytes.constData()), outputBytes.size() / sizeof(UChar));
    Vector<String> lines;
    output.split(UChar('\n'), lines);
    ASSERT(lines.size() == 3);

    result.name.swap(lines[0]);
    result.description.swap(lines[1]);
    result.mimeDescription.swap(lines[2]);
    return !result.mimeDescription.isEmpty();
}

} // namespace WebKit

#endif // ENABLE(PLUGIN_PROCESS)
