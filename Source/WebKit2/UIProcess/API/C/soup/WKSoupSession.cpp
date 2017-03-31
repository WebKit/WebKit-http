#include "config.h"
#include "WKSoupSession.h"

#include "WebProcessPool.h"
#include <WebCore/Language.h>

using namespace WebKit;

void WKSoupSessionSetIgnoreTLSErrors(WKContextRef context, bool ignore)
{
#if USE(SOUP)
    toImpl(context)->setIgnoreTLSErrors(ignore);
#endif
}

void WKSoupSessionSetPreferredLanguages(WKContextRef context, WKArrayRef languages)
{
    UNUSED_PARAM(context);

    API::Array* languagesArray = toImpl(languages);
    if (!languagesArray)
        return;

    size_t size = languagesArray->size();
    if (!size)
        return;

    Vector<String> languagesVector;
    languagesVector.reserveInitialCapacity(size);

    for (const auto& entry : languagesArray->elementsOfType<API::String>()) {
        auto string = entry->string();
        if (equalIgnoringASCIICase(string, "C") || equalIgnoringASCIICase(string, "POSIX"))
            languagesVector.uncheckedAppend(ASCIILiteral("en-us"));
        else
            languagesVector.uncheckedAppend(string.convertToASCIILowercase().replace("_", "-"));
    }

    WebCore::overrideUserPreferredLanguages(languagesVector);
}
