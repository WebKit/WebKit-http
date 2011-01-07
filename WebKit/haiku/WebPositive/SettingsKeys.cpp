/*
 * Copyright (C) 2010 Stephan Aßmus <superstippi@gmx.de>
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *	notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *	notice, this list of conditions and the following disclaimer in the
 *	documentation and/or other materials provided with the distribution.
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

#include "SettingsKeys.h"

const char* kSettingsKeyDownloadPath = "download path";
const char* kSettingsKeyShowTabsIfSinglePageOpen
	= "show tabs if single page open";
const char* kSettingsKeyAutoHideInterfaceInFullscreenMode
	= "auto hide interface in full screen mode";
const char* kSettingsKeyAutoHidePointer = "auto hide pointer";
const char* kSettingsKeyShowHomeButton = "show home button";

const char* kSettingsKeyNewWindowPolicy = "new window policy";
const char* kSettingsKeyNewTabPolicy = "new tab policy";
const char* kSettingsKeyStartPageURL = "start page url";
const char* kSettingsKeySearchPageURL = "search page url";


const char* kDefaultDownloadPath = "/boot/home/Desktop/";
const char* kDefaultStartPageURL
	= "file:///boot/home/config/settings/WebPositive/LoaderPages/Welcome";
const char* kDefaultSearchPageURL = "http://www.google.com";

const char* kSettingsKeyUseProxy = "use http proxy";
const char* kSettingsKeyProxyAddress = "http proxy address";
const char* kSettingsKeyProxyPort = "http proxy port";
