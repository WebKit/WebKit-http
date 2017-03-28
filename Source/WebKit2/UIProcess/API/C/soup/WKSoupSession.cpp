#include "config.h"
#include "WKSoupSession.h"

#include "WebProcessPool.h"

using namespace WebKit;

void WKSoupSessionSetIgnoreTLSErrors(WKContextRef context, bool ignore)
{
#if USE(SOUP)
    toImpl(context)->setIgnoreTLSErrors(ignore);
#endif
}
