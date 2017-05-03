#ifndef WKSoupSession_h
#define WKSoupSession_h

#include <WebKit/WKBase.h>

#ifdef __cplusplus
extern "C" {
#endif

WK_EXPORT void WKSoupSessionSetIgnoreTLSErrors(WKContextRef context, bool ignore);

WK_EXPORT void WKSoupSessionSetPreferredLanguages(WKContextRef context, WKArrayRef languages);

#ifdef __cplusplus
}
#endif

#endif
