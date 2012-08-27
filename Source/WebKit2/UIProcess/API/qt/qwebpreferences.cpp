/*
    Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "config.h"
#include "qwebpreferences_p.h"

#include "WKPageGroup.h"
#include "WKPreferences.h"
#include "WKPreferencesPrivate.h"
#include "WKRetainPtr.h"
#include "WKStringQt.h"
#include "qquickwebview_p_p.h"
#include "qwebpreferences_p_p.h"

QWebPreferences* QWebPreferencesPrivate::createPreferences(QQuickWebViewPrivate* webViewPrivate)
{
    QWebPreferences* prefs = new QWebPreferences;
    prefs->d->webViewPrivate = webViewPrivate;
    prefs->d->initializeDefaultFontSettings();
    return prefs;
}

bool QWebPreferencesPrivate::testAttribute(QWebPreferencesPrivate::WebAttribute attr) const
{
    switch (attr) {
    case AutoLoadImages:
        return WKPreferencesGetLoadsImagesAutomatically(preferencesRef());
#if ENABLE(FULLSCREEN_API)
    case FullScreenEnabled:
        return WKPreferencesGetFullScreenEnabled(preferencesRef());
#endif
    case JavascriptEnabled:
        return WKPreferencesGetJavaScriptEnabled(preferencesRef());
    case PluginsEnabled:
        return WKPreferencesGetPluginsEnabled(preferencesRef());
    case OfflineWebApplicationCacheEnabled:
        return WKPreferencesGetOfflineWebApplicationCacheEnabled(preferencesRef());
    case LocalStorageEnabled:
        return WKPreferencesGetLocalStorageEnabled(preferencesRef());
    case XSSAuditingEnabled:
        return WKPreferencesGetXSSAuditorEnabled(preferencesRef());
    case PrivateBrowsingEnabled:
        return WKPreferencesGetPrivateBrowsingEnabled(preferencesRef());
    case DnsPrefetchEnabled:
        return WKPreferencesGetDNSPrefetchingEnabled(preferencesRef());
    case FrameFlatteningEnabled:
        return WKPreferencesGetFrameFlatteningEnabled(preferencesRef());
    case DeveloperExtrasEnabled:
        return WKPreferencesGetDeveloperExtrasEnabled(preferencesRef());
#if ENABLE(WEBGL)
    case WebGLEnabled:
        return WKPreferencesGetWebGLEnabled(preferencesRef());
#if ENABLE(CSS_SHADERS)
    case CSSCustomFilterEnabled:
        return WKPreferencesGetCSSCustomFilterEnabled(preferencesRef());
#endif
#endif
#if ENABLE(WEB_AUDIO)
    case WebAudioEnabled:
        return WKPreferencesGetWebAudioEnabled(preferencesRef());
#endif
    default:
        ASSERT_NOT_REACHED();
        return false;
    }
}

void QWebPreferencesPrivate::setAttribute(QWebPreferencesPrivate::WebAttribute attr, bool enable)
{
    switch (attr) {
    case AutoLoadImages:
        WKPreferencesSetLoadsImagesAutomatically(preferencesRef(), enable);
        break;
#if ENABLE(FULLSCREEN_API)
    case FullScreenEnabled:
        WKPreferencesSetFullScreenEnabled(preferencesRef(), enable);
        break;
#endif
    case JavascriptEnabled:
        WKPreferencesSetJavaScriptEnabled(preferencesRef(), enable);
        break;
    case PluginsEnabled:
        WKPreferencesSetPluginsEnabled(preferencesRef(), enable);
        break;
    case OfflineWebApplicationCacheEnabled:
        WKPreferencesSetOfflineWebApplicationCacheEnabled(preferencesRef(), enable);
        break;
    case LocalStorageEnabled:
        WKPreferencesSetLocalStorageEnabled(preferencesRef(), enable);
        break;
    case XSSAuditingEnabled:
        WKPreferencesSetXSSAuditorEnabled(preferencesRef(), enable);
        break;
    case PrivateBrowsingEnabled:
        WKPreferencesSetPrivateBrowsingEnabled(preferencesRef(), enable);
        break;
    case DnsPrefetchEnabled:
        WKPreferencesSetDNSPrefetchingEnabled(preferencesRef(), enable);
        break;
    case FrameFlatteningEnabled:
        WKPreferencesSetFrameFlatteningEnabled(preferencesRef(), enable);
    case DeveloperExtrasEnabled:
        WKPreferencesSetDeveloperExtrasEnabled(preferencesRef(), enable);
        break;
#if ENABLE(WEBGL)
    case WebGLEnabled:
        WKPreferencesSetWebGLEnabled(preferencesRef(), enable);
        break;
#if ENABLE(CSS_SHADERS)
    case CSSCustomFilterEnabled:
        WKPreferencesSetCSSCustomFilterEnabled(preferencesRef(), enable);
        break;
#endif
#endif
#if ENABLE(WEB_AUDIO)
    case WebAudioEnabled:
        WKPreferencesSetWebAudioEnabled(preferencesRef(), enable);
        break;
#endif
    default:
        ASSERT_NOT_REACHED();
    }
}

void QWebPreferencesPrivate::initializeDefaultFontSettings()
{
    setFontSize(MinimumFontSize, 0);
    setFontSize(DefaultFontSize, 16);
    setFontSize(DefaultFixedFontSize, 13);

    QFont defaultFont;
    defaultFont.setStyleHint(QFont::Serif);
    setFontFamily(StandardFont, defaultFont.defaultFamily());
    setFontFamily(SerifFont, defaultFont.defaultFamily());

    defaultFont.setStyleHint(QFont::Fantasy);
    setFontFamily(FantasyFont, defaultFont.defaultFamily());

    defaultFont.setStyleHint(QFont::Cursive);
    setFontFamily(CursiveFont, defaultFont.defaultFamily());

    defaultFont.setStyleHint(QFont::SansSerif);
    setFontFamily(SansSerifFont, defaultFont.defaultFamily());

    defaultFont.setStyleHint(QFont::Monospace);
    setFontFamily(FixedFont, defaultFont.defaultFamily());
}

void QWebPreferencesPrivate::setFontFamily(QWebPreferencesPrivate::FontFamily which, const QString& family)
{
    switch (which) {
    case StandardFont:
        WKPreferencesSetStandardFontFamily(preferencesRef(), WKStringCreateWithQString(family));
        break;
    case FixedFont:
        WKPreferencesSetFixedFontFamily(preferencesRef(), WKStringCreateWithQString(family));
        break;
    case SerifFont:
        WKPreferencesSetSerifFontFamily(preferencesRef(), WKStringCreateWithQString(family));
        break;
    case SansSerifFont:
        WKPreferencesSetSansSerifFontFamily(preferencesRef(), WKStringCreateWithQString(family));
        break;
    case CursiveFont:
        WKPreferencesSetCursiveFontFamily(preferencesRef(), WKStringCreateWithQString(family));
        break;
    case FantasyFont:
        WKPreferencesSetFantasyFontFamily(preferencesRef(), WKStringCreateWithQString(family));
        break;
    default:
        break;
    }
}

QString QWebPreferencesPrivate::fontFamily(QWebPreferencesPrivate::FontFamily which) const
{
    switch (which) {
    case StandardFont: {
        WKRetainPtr<WKStringRef> stringRef(AdoptWK, WKPreferencesCopyStandardFontFamily(preferencesRef()));
        return WKStringCopyQString(stringRef.get());
    }
    case FixedFont: {
        WKRetainPtr<WKStringRef> stringRef(AdoptWK, WKPreferencesCopyFixedFontFamily(preferencesRef()));
        return WKStringCopyQString(stringRef.get());
    }
    case SerifFont: {
        WKRetainPtr<WKStringRef> stringRef(AdoptWK, WKPreferencesCopySerifFontFamily(preferencesRef()));
        return WKStringCopyQString(stringRef.get());
    }
    case SansSerifFont: {
        WKRetainPtr<WKStringRef> stringRef(AdoptWK, WKPreferencesCopySansSerifFontFamily(preferencesRef()));
        return WKStringCopyQString(stringRef.get());
    }
    case CursiveFont: {
        WKRetainPtr<WKStringRef> stringRef(AdoptWK, WKPreferencesCopyCursiveFontFamily(preferencesRef()));
        return WKStringCopyQString(stringRef.get());
    }
    case FantasyFont: {
        WKRetainPtr<WKStringRef> stringRef(AdoptWK, WKPreferencesCopyFantasyFontFamily(preferencesRef()));
        return WKStringCopyQString(stringRef.get());
    }
    default:
        return QString();
    }
}

void QWebPreferencesPrivate::setFontSize(QWebPreferencesPrivate::FontSizeType type, unsigned size)
{
    switch (type) {
    case MinimumFontSize:
         WKPreferencesSetMinimumFontSize(preferencesRef(), size);
         break;
    case DefaultFontSize:
         WKPreferencesSetDefaultFontSize(preferencesRef(), size);
         break;
    case DefaultFixedFontSize:
         WKPreferencesSetDefaultFixedFontSize(preferencesRef(), size);
         break;
    default:
        ASSERT_NOT_REACHED();
    }
}

unsigned QWebPreferencesPrivate::fontSize(QWebPreferencesPrivate::FontSizeType type) const
{
    switch (type) {
    case MinimumFontSize:
         return WKPreferencesGetMinimumFontSize(preferencesRef());
    case DefaultFontSize:
         return WKPreferencesGetDefaultFontSize(preferencesRef());
    case DefaultFixedFontSize:
         return WKPreferencesGetDefaultFixedFontSize(preferencesRef());
    default:
        ASSERT_NOT_REACHED();
        return false;
    }
}

QWebPreferences::QWebPreferences()
    : d(new QWebPreferencesPrivate)
{
}

QWebPreferences::~QWebPreferences()
{
    delete d;
}

bool QWebPreferences::autoLoadImages() const
{
    return d->testAttribute(QWebPreferencesPrivate::AutoLoadImages);
}

void QWebPreferences::setAutoLoadImages(bool enable)
{
    d->setAttribute(QWebPreferencesPrivate::AutoLoadImages, enable);
    emit autoLoadImagesChanged();
}

bool QWebPreferences::fullScreenEnabled() const
{
#if ENABLE(FULLSCREEN_API)
    return d->testAttribute(QWebPreferencesPrivate::FullScreenEnabled);
#else
    return false;
#endif
}

void QWebPreferences::setFullScreenEnabled(bool enable)
{
#if ENABLE(FULLSCREEN_API)
    d->setAttribute(QWebPreferencesPrivate::FullScreenEnabled, enable);
    emit fullScreenEnabledChanged();
#else
    UNUSED_PARAM(enable);
#endif
}

bool QWebPreferences::javascriptEnabled() const
{
    return d->testAttribute(QWebPreferencesPrivate::JavascriptEnabled);
}

void QWebPreferences::setJavascriptEnabled(bool enable)
{
    d->setAttribute(QWebPreferencesPrivate::JavascriptEnabled, enable);
    emit javascriptEnabledChanged();
}

bool QWebPreferences::pluginsEnabled() const
{
    return d->testAttribute(QWebPreferencesPrivate::PluginsEnabled);
}

void QWebPreferences::setPluginsEnabled(bool enable)
{
    d->setAttribute(QWebPreferencesPrivate::PluginsEnabled, enable);
    emit pluginsEnabledChanged();
}

bool QWebPreferences::offlineWebApplicationCacheEnabled() const
{
    return d->testAttribute(QWebPreferencesPrivate::OfflineWebApplicationCacheEnabled);
}

void QWebPreferences::setOfflineWebApplicationCacheEnabled(bool enable)
{
    d->setAttribute(QWebPreferencesPrivate::OfflineWebApplicationCacheEnabled, enable);
    emit offlineWebApplicationCacheEnabledChanged();
}

bool QWebPreferences::localStorageEnabled() const
{
    return d->testAttribute(QWebPreferencesPrivate::LocalStorageEnabled);
}

void QWebPreferences::setLocalStorageEnabled(bool enable)
{
    d->setAttribute(QWebPreferencesPrivate::LocalStorageEnabled, enable);
    emit localStorageEnabledChanged();
}

bool QWebPreferences::xssAuditingEnabled() const
{
    return d->testAttribute(QWebPreferencesPrivate::XSSAuditingEnabled);
}

void QWebPreferences::setXssAuditingEnabled(bool enable)
{
    d->setAttribute(QWebPreferencesPrivate::XSSAuditingEnabled, enable);
    emit xssAuditingEnabledChanged();
}

bool QWebPreferences::privateBrowsingEnabled() const
{
    return d->testAttribute(QWebPreferencesPrivate::PrivateBrowsingEnabled);
}

void QWebPreferences::setPrivateBrowsingEnabled(bool enable)
{
    d->setAttribute(QWebPreferencesPrivate::PrivateBrowsingEnabled, enable);
    emit privateBrowsingEnabledChanged();
}

bool QWebPreferences::dnsPrefetchEnabled() const
{
    return d->testAttribute(QWebPreferencesPrivate::DnsPrefetchEnabled);
}

void QWebPreferences::setDnsPrefetchEnabled(bool enable)
{
    d->setAttribute(QWebPreferencesPrivate::DnsPrefetchEnabled, enable);
    emit dnsPrefetchEnabledChanged();
}

bool QWebPreferences::developerExtrasEnabled() const
{
    return d->testAttribute(QWebPreferencesPrivate::DeveloperExtrasEnabled);
}

void QWebPreferences::setDeveloperExtrasEnabled(bool enable)
{
    d->setAttribute(QWebPreferencesPrivate::DeveloperExtrasEnabled, enable);
    emit developerExtrasEnabledChanged();
}

bool QWebPreferences::navigatorQtObjectEnabled() const
{
    return d->webViewPrivate->navigatorQtObjectEnabled();
}

void QWebPreferences::setNavigatorQtObjectEnabled(bool enable)
{
    if (enable == navigatorQtObjectEnabled())
        return;
    d->webViewPrivate->setNavigatorQtObjectEnabled(enable);
    emit navigatorQtObjectEnabledChanged();
}

bool QWebPreferences::frameFlatteningEnabled() const
{
    return d->testAttribute(QWebPreferencesPrivate::FrameFlatteningEnabled);
}

void QWebPreferences::setFrameFlatteningEnabled(bool enable)
{
    d->setAttribute(QWebPreferencesPrivate::FrameFlatteningEnabled, enable);
    emit frameFlatteningEnabledChanged();
}

QString QWebPreferences::standardFontFamily() const
{
    return d->fontFamily(QWebPreferencesPrivate::StandardFont);
}

void QWebPreferences::setStandardFontFamily(const QString& family)
{
    d->setFontFamily(QWebPreferencesPrivate::StandardFont, family);
    emit standardFontFamilyChanged();
}

QString QWebPreferences::fixedFontFamily() const
{
    return d->fontFamily(QWebPreferencesPrivate::FixedFont);
}

void QWebPreferences::setFixedFontFamily(const QString& family)
{
    d->setFontFamily(QWebPreferencesPrivate::FixedFont, family);
    emit fixedFontFamilyChanged();
}

QString QWebPreferences::serifFontFamily() const
{
    return d->fontFamily(QWebPreferencesPrivate::SerifFont);
}

void QWebPreferences::setSerifFontFamily(const QString& family)
{
    d->setFontFamily(QWebPreferencesPrivate::SerifFont, family);
    emit serifFontFamilyChanged();
}

QString QWebPreferences::sansSerifFontFamily() const
{
    return d->fontFamily(QWebPreferencesPrivate::SansSerifFont);
}

void QWebPreferences::setSansSerifFontFamily(const QString& family)
{
    d->setFontFamily(QWebPreferencesPrivate::SansSerifFont, family);
    emit sansSerifFontFamilyChanged();
}

QString QWebPreferences::cursiveFontFamily() const
{
    return d->fontFamily(QWebPreferencesPrivate::CursiveFont);
}

void QWebPreferences::setCursiveFontFamily(const QString& family)
{
    d->setFontFamily(QWebPreferencesPrivate::CursiveFont, family);
    emit cursiveFontFamilyChanged();
}

QString QWebPreferences::fantasyFontFamily() const
{
    return d->fontFamily(QWebPreferencesPrivate::FantasyFont);
}

void QWebPreferences::setFantasyFontFamily(const QString& family)
{
    d->setFontFamily(QWebPreferencesPrivate::FantasyFont, family);
    emit fantasyFontFamilyChanged();
}

unsigned QWebPreferences::minimumFontSize() const
{
    return d->fontSize(QWebPreferencesPrivate::MinimumFontSize);
}

void QWebPreferences::setMinimumFontSize(unsigned size)
{
    d->setFontSize(QWebPreferencesPrivate::MinimumFontSize, size);
    emit minimumFontSizeChanged();
}

unsigned QWebPreferences::defaultFontSize() const
{
    return d->fontSize(QWebPreferencesPrivate::DefaultFontSize);
}

void QWebPreferences::setDefaultFontSize(unsigned size)
{
    d->setFontSize(QWebPreferencesPrivate::DefaultFontSize, size);
    emit defaultFontSizeChanged();
}

unsigned QWebPreferences::defaultFixedFontSize() const
{
    return d->fontSize(QWebPreferencesPrivate::DefaultFixedFontSize);
}

void QWebPreferences::setDefaultFixedFontSize(unsigned size)
{
    d->setFontSize(QWebPreferencesPrivate::DefaultFixedFontSize, size);
    emit defaultFixedFontSizeChanged();
}

bool QWebPreferences::webGLEnabled() const
{
#if ENABLE(WEBGL)
    return d->testAttribute(QWebPreferencesPrivate::WebGLEnabled);
#else
    return false;
#endif
}

void QWebPreferences::setWebGLEnabled(bool enable)
{
#if ENABLE(WEBGL)
    d->setAttribute(QWebPreferencesPrivate::WebGLEnabled, enable);
    emit webGLEnabledChanged();
#else
    UNUSED_PARAM(enable);
#endif
}

bool QWebPreferences::webAudioEnabled() const
{
#if ENABLE(WEB_AUDIO)
    return d->testAttribute(QWebPreferencesPrivate::WebAudioEnabled);
#else
    return false;
#endif
}

void QWebPreferences::setWebAudioEnabled(bool enable)
{
#if ENABLE(WEB_AUDIO)
    d->setAttribute(QWebPreferencesPrivate::WebAudioEnabled, enable);
    emit webAudioEnabledChanged();
#else
    UNUSED_PARAM(enable);
#endif
}

WKPreferencesRef QWebPreferencesPrivate::preferencesRef() const
{
    WKPageGroupRef pageGroupRef = toAPI(webViewPrivate->webPageProxy->pageGroup());
    return WKPageGroupGetPreferences(pageGroupRef);
}

QWebPreferencesPrivate* QWebPreferencesPrivate::get(QWebPreferences* preferences)
{
    return preferences->d;
}
