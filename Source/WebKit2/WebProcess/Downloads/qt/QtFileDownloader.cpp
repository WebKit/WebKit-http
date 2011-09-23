/*
 * Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this program; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */
#include "config.h"
#include "QtFileDownloader.h"

#include "DataReference.h"
#include "Download.h"
#include "HTTPParsers.h"
#include "MIMETypeRegistry.h"
#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <WebCore/QNetworkReplyHandler.h>
#include <WebCore/ResourceError.h>
#include <WebCore/ResourceResponse.h>

using namespace WebCore;
using namespace WTF;

namespace WebKit {

QtFileDownloader::QtFileDownloader(Download* download, PassOwnPtr<QNetworkReply> reply)
    : m_download(download)
    , m_reply(reply)
    , m_error(QNetworkReply::NoError)
    , m_headersRead(false)
{
}

QtFileDownloader::~QtFileDownloader()
{
    if (!m_destinationFile)
        return;

    abortDownloadWritingAndEmitError(QtFileDownloader::DownloadErrorAborted);
}

void QtFileDownloader::start()
{
    connect(m_reply.get(), SIGNAL(readyRead()), SLOT(onReadyRead()));
    connect(m_reply.get(), SIGNAL(finished()), SLOT(onFinished()));
    connect(m_reply.get(), SIGNAL(error(QNetworkReply::NetworkError)), SLOT(onError(QNetworkReply::NetworkError)));

    // Call onReadyRead just in case some data is already waiting.
    onReadyRead();
}

void QtFileDownloader::determineFilename()
{
    ASSERT(!m_destinationFile);

    QString fileNameCandidate = filenameFromHTTPContentDisposition(QString::fromLatin1(m_reply->rawHeader("Content-Disposition")));
    if (fileNameCandidate.isEmpty()) {
        KURL kurl = m_reply->url();
        fileNameCandidate = decodeURLEscapeSequences(kurl.lastPathComponent());
    }

    if (fileNameCandidate.isEmpty()) {
        abortDownloadWritingAndEmitError(QtFileDownloader::DownloadErrorCannotDetermineFilename);
        return;
    }

    // Make sure that we remove possible "../.." parts in the given file name.
    QFileInfo fileNameFilter(fileNameCandidate);
    QString fileName = fileNameFilter.fileName();

    if (fileName.isEmpty()) {
        abortDownloadWritingAndEmitError(QtFileDownloader::DownloadErrorCannotDetermineFilename);
        return;
    }

    bool allowOverwrite;
    m_download->decideDestinationWithSuggestedFilename(fileName, allowOverwrite);
}

void QtFileDownloader::decidedDestination(const QString& decidedFilePath, bool allowOverwrite)
{
    ASSERT(!m_destinationFile);

    // Error might have occured during destination query.
    if (m_error != QNetworkReply::NoError) {
        abortDownloadWritingAndEmitError(QtFileDownloader::DownloadErrorNetworkFailure);
        return;
    }

    if (decidedFilePath.isEmpty()) {
        abortDownloadWritingAndEmitError(QtFileDownloader::DownloadErrorCancelledByCaller);
        return;
    }

    OwnPtr<QFile> downloadFile = adoptPtr(new QFile(decidedFilePath));

    if (!allowOverwrite && downloadFile->exists()) {
        abortDownloadWritingAndEmitError(QtFileDownloader::DownloadErrorFileAlreadyExists);
        return;
    }

    if (!downloadFile->open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        abortDownloadWritingAndEmitError(QtFileDownloader::DownloadErrorCannotOpenFile);
        return;
    }

    // Assigning to m_destinationFile flags that either error or
    // finished shall be called in the end.
    m_destinationFile = downloadFile.release();

    m_download->didCreateDestination(m_destinationFile->fileName());

    // We might have gotten readyRead already even before this function
    // was called.
    if (m_reply->bytesAvailable())
        onReadyRead();

    // We might have gotten finished already even before this
    // function was called.
    if (m_reply->isFinished())
        onFinished();

}

void QtFileDownloader::abortDownloadWritingAndEmitError(QtFileDownloader::DownloadError errorCode)
{
    m_reply->abort();

    // On network failures it's QNetworkReplyHandler::errorForReply who will handle errors.
    if (errorCode == QtFileDownloader::DownloadErrorNetworkFailure) {
        m_download->didFail(QNetworkReplyHandler::errorForReply(m_reply.get()), CoreIPC::DataReference(0, 0));
        return;
    }

    QString translatedErrorMessage;
    switch (errorCode) {
    case QtFileDownloader::DownloadErrorAborted:
        translatedErrorMessage = QCoreApplication::translate("QtFileDownloader", "Download aborted");
        break;
    case QtFileDownloader::DownloadErrorCannotWriteToFile:
        translatedErrorMessage = QCoreApplication::translate("QtFileDownloader", "Cannot write to file");
        break;
    case QtFileDownloader::DownloadErrorCannotOpenFile:
        translatedErrorMessage = QCoreApplication::translate("QtFileDownloader", "Cannot open file for writing");
        break;
    case QtFileDownloader::DownloadErrorFileAlreadyExists:
        translatedErrorMessage = QCoreApplication::translate("QtFileDownloader", "File already exists");
        break;
    case QtFileDownloader::DownloadErrorCancelledByCaller:
        translatedErrorMessage = QCoreApplication::translate("QtFileDownloader", "Download cancelled by caller");
        break;
    case QtFileDownloader::DownloadErrorCannotDetermineFilename:
        translatedErrorMessage = QCoreApplication::translate("QtFileDownloader", "Cannot determine filename");
        break;
    default:
        ASSERT_NOT_REACHED();
    }

    ResourceError downloadError("Download", errorCode, m_reply->url().toString(), translatedErrorMessage);

    m_download->didFail(downloadError, CoreIPC::DataReference(0, 0));
}

void QtFileDownloader::onReadyRead()
{
    if (m_destinationFile) {
        QByteArray content = m_reply->readAll();
        if (content.size() <= 0)
            return;

        qint64 bytesWritten = m_destinationFile->write(content);

        if (bytesWritten == -1) {
            abortDownloadWritingAndEmitError(QtFileDownloader::DownloadErrorCannotWriteToFile);
            return;
        }

        // There might a corner case to be fixed here if bytesWritten != content.size()
        // does not actually represent an error.
        ASSERT(bytesWritten == content.size());

        m_download->didReceiveData(bytesWritten);
    } else if (!m_headersRead) {
        // By API contract, QNetworkReply::metaDataChanged cannot really be trusted.
        // Thus we need to call this upon receiving first data.
        String contentType = m_reply->header(QNetworkRequest::ContentTypeHeader).toString();
        String encoding = extractCharsetFromMediaType(contentType);
        String mimeType = extractMIMETypeFromMediaType(contentType);

        // Let's try to guess from the extension.
        if (mimeType.isEmpty())
            mimeType = MIMETypeRegistry::getMIMETypeForPath(m_reply->url().path());

        ResourceResponse response(m_reply->url(), mimeType, m_reply->header(QNetworkRequest::ContentLengthHeader).toLongLong(), encoding, String());
        m_download->didReceiveResponse(response);

        determineFilename();

        m_headersRead = true;
    }
}

void QtFileDownloader::onFinished()
{
    if (!m_destinationFile)
        return;

    m_destinationFile.clear();

    if (m_error == QNetworkReply::NoError)
        m_download->didFinish();
    else
        abortDownloadWritingAndEmitError(QtFileDownloader::DownloadErrorNetworkFailure);
}

void QtFileDownloader::onError(QNetworkReply::NetworkError code)
{
    m_error = code;
}

void QtFileDownloader::cancel()
{
    m_reply->abort();
}

} // namespace WebKit
#include "moc_QtFileDownloader.cpp"
