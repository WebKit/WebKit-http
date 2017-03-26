/*
 * Copyright (C) 2017 Konstantin Tokarev <annulen@yandex.ru>
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
#include "QrcSchemeHandler.h"

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QNetworkReply>
#include <WebCore/ResourceError.h>
#include <WebCore/ResourceResponse.h>
#include <WebCore/SharedBuffer.h>

using namespace WebCore;

namespace WebKit {

Ref<QrcSchemeHandler> QrcSchemeHandler::create()
{
    return adoptRef(*new QrcSchemeHandler());
}

static void sendResponse(WebURLSchemeHandlerTask& task, const QString& fileName, const QByteArray& fileData)
{
    QMimeDatabase mimeDb;
    QMimeType mimeType = mimeDb.mimeTypeForFileNameAndData(fileName, fileData);

    WebCore::ResourceResponse response(task.request().url(), mimeType.name(), fileData.size(), String());
    auto result = task.didReceiveResponse(response);
    ASSERT_UNUSED(result, result == WebURLSchemeHandlerTask::ExceptionType::None);
}

static void sendError(WebURLSchemeHandlerTask& task)
{
    // QTFIXME: Move error templates to ErrorsQt
    WebCore::ResourceError error("QtNetwork", QNetworkReply::ContentNotFoundError, task.request().url(),
        QCoreApplication::translate("QWebFrame", "File does not exist"));

    auto result = task.didComplete(error);
    ASSERT_UNUSED(result, result == WebURLSchemeHandlerTask::ExceptionType::None);
}

void QrcSchemeHandler::platformStartTask(WebPageProxy& page, WebURLSchemeHandlerTask& task)
{
    QString fileName = ':' + QString(task.request().url().path());
    QByteArray fileData;

    {
        QFile file(fileName);
        QFileInfo fileInfo(file);
        if (fileInfo.isDir() || !file.open(QIODevice::ReadOnly | QIODevice::Unbuffered)) {
            sendError(task);
            return;
        }
        fileData = file.readAll();
    }

    sendResponse(task, fileName, fileData);

    // TODO: Wrap SharedBuffer around QByteArray when it's possible
    auto result = task.didReceiveData(*SharedBuffer::create(fileData.data(), fileData.size()));
    ASSERT_UNUSED(result, result == WebURLSchemeHandlerTask::ExceptionType::None);

    result = task.didComplete(WebCore::ResourceError());
    ASSERT_UNUSED(result, result == WebURLSchemeHandlerTask::ExceptionType::None);
}

void QrcSchemeHandler::platformStopTask(WebPageProxy&, WebURLSchemeHandlerTask&)
{
}

} // namespace WebKit
