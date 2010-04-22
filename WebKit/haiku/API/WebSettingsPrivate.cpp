/*
 * Copyright (C) 2010 Stephan Aßmus <superstippi@gmx.de>
 *
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "WebSettingsPrivate.h"

#include "PageCache.h"
#include "PlatformString.h"
#include "Settings.h"
#include "WebSettings.h"

namespace BPrivate {

BList WebSettingsPrivate::sAllSettings(128);

WebSettingsPrivate::WebSettingsPrivate(WebCore::Settings* settings)
    : settings(settings)
    , localStoragePath()
    , offlineStorageDefaultQuota(5 * 1024 * 1024)
    , localStorageEnabled(false)
    , databasesEnabled(false)
    , offlineWebApplicationCacheEnabled(false)
    , serifFontFamilySet(false)
    , sansSerifFontFamilySet(false)
    , fixedFontFamilySet(false)
    , standardFontFamilySet(false)
    , defaultFontSizeSet(false)
    , defaultFixedFontSizeSet(false)
{
	apply();
	if (settings)
	    sAllSettings.AddItem(this);
	else {
		// Initialize some default settings
		// TODO: Get these from the system settings.
		serifFontFamily = "DejaVu Serif";
		sansSerifFontFamily = "DejaVu Sans";
		fixedFontFamily = "DejaVu Sans Mono";
		standardFontFamily = serifFontFamily;
		defaultFontSize = 14;
		defaultFixedFontSize = 14;
	}
}

WebSettingsPrivate::~WebSettingsPrivate()
{
	if (settings)
	    sAllSettings.RemoveItem(this);
}

void WebSettingsPrivate::apply()
{
	if (settings) {
		WebSettingsPrivate* global = BWebSettings::Default()->fData;
	    // Apply default values
	    settings->setLoadsImagesAutomatically(true);
	    settings->setMinimumFontSize(5);
	    settings->setMinimumLogicalFontSize(5);
	    settings->setShouldPrintBackgrounds(true);
	    settings->setJavaScriptEnabled(true);
//	    settings->setShowsURLsInToolTips(true);
	    settings->setShouldPaintCustomScrollbars(true);
	    settings->setEditingBehavior(WebCore::EditingMacBehavior);
	    settings->setLocalStorageEnabled(global->localStorageEnabled);
	    settings->setLocalStorageDatabasePath(global->localStoragePath);
	    settings->setDefaultTextEncodingName("UTF-8");
#if 0
// FIXME: Using the PageCache will reliably crash searching something
// on www.google.com, following a picture link which opens the website
// with the picture in a sub-frame, and then navigating back. This may
// either point to a problem with the PageCache implementation, or with
// the Haiku port. It is already a known issue that the Haiku port may
// instantiate more DOM Document instances for the same navigation sequence
// compared to the Gtk port. Perhaps there is a problem with the same data
// being referenced by more than one Document, leading to crashes eventually,
// when restoring those Document(s) from the PageCache.
        settings->setUsesPageCache(WebCore::pageCache()->capacity());
#else
        settings->setUsesPageCache(false);
#endif
        settings->setNeedsSiteSpecificQuirks(true);

	    // Apply local or global settings
		if (defaultFontSizeSet)
            settings->setDefaultFontSize(defaultFontSize);
		else
            settings->setDefaultFontSize(global->defaultFontSize);

		if (defaultFixedFontSizeSet)
            settings->setDefaultFixedFontSize(defaultFixedFontSize);
		else
            settings->setDefaultFixedFontSize(global->defaultFixedFontSize);

		if (serifFontFamilySet)
            settings->setSerifFontFamily(serifFontFamily.String());
		else
            settings->setSerifFontFamily(global->serifFontFamily.String());

		if (sansSerifFontFamilySet)
            settings->setSansSerifFontFamily(sansSerifFontFamily.String());
		else
            settings->setSansSerifFontFamily(global->sansSerifFontFamily.String());

		if (fixedFontFamilySet)
            settings->setFixedFontFamily(fixedFontFamily.String());
		else
            settings->setFixedFontFamily(global->fixedFontFamily.String());

		if (standardFontFamilySet)
            settings->setStandardFontFamily(standardFontFamily.String());
		else
            settings->setStandardFontFamily(global->standardFontFamily.String());
	} else {
	    int32 count = sAllSettings.CountItems();
	    for (int32 i = 0; i < count; i++) {
	        WebSettingsPrivate* webSettings = reinterpret_cast<WebSettingsPrivate*>(
	            sAllSettings.ItemAtFast(i));
	        if (webSettings != this)
	            webSettings->apply();
	    }
	}
}

} // namespace BPrivate
