#include "config.h"

#include "ResourceResponse.h"

#include "HTTPHeaderNames.h"
#include "HTTPParsers.h"

namespace WebCore {

String ResourceResponse::platformSuggestedFilename() const
{
    String contentDisposition(httpHeaderField(HTTPHeaderName::ContentDisposition));
    return filenameFromHTTPContentDisposition(String::fromUTF8WithLatin1Fallback(contentDisposition.characters8(), contentDisposition.length()));
}

}
