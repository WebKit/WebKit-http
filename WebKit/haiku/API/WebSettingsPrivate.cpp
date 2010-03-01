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
{
	apply();
	if (settings)
	    sAllSettings.AddItem(this);
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
	    // Default settings
	    // TODO: Get attributes from hash map and fall back to global settings if
	    // attributes are not set.
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
	
	    settings->setDefaultFixedFontSize(14);
	    settings->setDefaultFontSize(14);
	
	    settings->setSerifFontFamily("DejaVu Serif");
	    settings->setSansSerifFontFamily("DejaVu Sans");
	    settings->setFixedFontFamily("DejaVu Sans Mono");
	    settings->setStandardFontFamily("DejaVu Serif");
	    settings->setDefaultTextEncodingName("UTF-8");
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
