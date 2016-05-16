#include "config.h"

#include "ResourceResponse.h"

#include "HTTPHeaderNames.h"
#include "HTTPParsers.h"
#include "MIMETypeRegistry.h"
#include <QMimeDatabase>

namespace WebCore {

String ResourceResponse::platformSuggestedFilename() const
{
    // FIXME: Move to base class
    String contentDisposition(httpHeaderField(HTTPHeaderName::ContentDisposition));
    String suggestedFilename = filenameFromHTTPContentDisposition(String::fromUTF8WithLatin1Fallback(contentDisposition.characters8(), contentDisposition.length()));

    if (!suggestedFilename.isEmpty())
        return suggestedFilename;

    Vector<String> extensions = MIMETypeRegistry::getExtensionsForMIMEType(mimeType());
    if (extensions.isEmpty())
        return url().lastPathComponent();

    // If the suffix doesn't match the MIME type, correct the suffix.
    QString filename = url().lastPathComponent();
    const String suffix = QMimeDatabase().suffixForFileName(filename);
    if (!extensions.contains(suffix)) {
        filename.chop(suffix.length());
        filename += MIMETypeRegistry::getPreferredExtensionForMIMEType(mimeType());
    }
    return filename;
}

}
