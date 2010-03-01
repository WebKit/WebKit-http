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
#include "WebSettings.h"

#include "ApplicationCacheStorage.h"
#include "DatabaseTracker.h"
#include "IconDatabase.h"
#include "Image.h"
#include "IntSize.h"
#include "Settings.h"
#include "WebSettingsPrivate.h"
#include <Application.h>
#include <Bitmap.h>
#include <Directory.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <Path.h>
#include <stdio.h>

enum {
	HANDLE_SET_PERSISTENT_STORAGE_PATH = 'hspp',
	HANDLE_SET_ICON_DATABASE_PATH = 'hsip',
	HANDLE_CLEAR_ICON_DATABASE = 'hcli',
	HANDLE_SEND_ICON_FOR_URL = 'sifu',
	HANDLE_SET_OFFLINE_STORAGE_PATH = 'hsop',
	HANDLE_SET_OFFLINE_STORAGE_DEFAULT_QUOTA = 'hsoq',
	HANDLE_SET_OFFLINE_WEB_APPLICATION_CACHE_PATH = 'hsap',
	HANDLE_SET_OFFLINE_WEB_APPLICATION_CACHE_QUOTA = 'hsaq',
	HANDLE_SET_LOCAL_STORAGE_PATH = 'hslp'
};

BWebSettings::BWebSettings()
    : fData(new BPrivate::WebSettingsPrivate())
{
	// This constructor is used only for the default (global) settings.
	if (be_app->Lock()) {
		be_app->AddHandler(this);
		be_app->Unlock();
	}
}

BWebSettings::BWebSettings(WebCore::Settings* settings)
    : fData(new BPrivate::WebSettingsPrivate(settings))
{
	if (be_app->Lock()) {
		be_app->AddHandler(this);
		be_app->Unlock();
	}
}

BWebSettings::~BWebSettings()
{
	if (be_app->Lock()) {
		be_app->RemoveHandler(this);
		be_app->Unlock();
	}
	delete fData;
}

BWebSettings* BWebSettings::Default()
{
	static BWebSettings defaultInstance;
	return &defaultInstance;
}

void BWebSettings::SetPersistentStoragePath(const BString& path)
{
	_PostSetPath(Default(), HANDLE_SET_PERSISTENT_STORAGE_PATH, path);
}

void BWebSettings::SetIconDatabasePath(const BString& path)
{
	_PostSetPath(Default(), HANDLE_SET_ICON_DATABASE_PATH, path);
}

void BWebSettings::ClearIconDatabase()
{
	Default()->Looper()->PostMessage(HANDLE_CLEAR_ICON_DATABASE, Default());
}

void BWebSettings::SendIconForURL(const BString& url, const BMessage& reply,
    const BMessenger& target)
{
	BMessage message(HANDLE_SEND_ICON_FOR_URL);
	message.AddString("url", url.String());
	message.AddMessage("reply", &reply);
	message.AddMessenger("target", target);
	Default()->Looper()->PostMessage(&message, Default());
}

void BWebSettings::SetOfflineStoragePath(const BString& path)
{
	_PostSetPath(Default(), HANDLE_SET_OFFLINE_STORAGE_PATH, path);
}

void BWebSettings::SetOfflineStorageDefaultQuota(int64 maximumSize)
{
	_PostSetQuota(Default(), HANDLE_SET_OFFLINE_STORAGE_DEFAULT_QUOTA, maximumSize);
}

void BWebSettings::SetOfflineWebApplicationCachePath(const BString& path)
{
	_PostSetPath(Default(), HANDLE_SET_OFFLINE_WEB_APPLICATION_CACHE_PATH, path);
}

void BWebSettings::SetOfflineWebApplicationCacheQuota(int64 maximumSize)
{
	_PostSetQuota(Default(), HANDLE_SET_OFFLINE_WEB_APPLICATION_CACHE_QUOTA, maximumSize);
}

void BWebSettings::SetLocalStoragePath(const BString& path)
{
	_PostSetPath(this, HANDLE_SET_LOCAL_STORAGE_PATH, path);
}

// #pragma mark - private

void BWebSettings::_PostSetPath(BHandler* handler, uint32 what, const BString& path)
{
	BMessage message(what);
	message.AddString("path", path.String());
	if (find_thread(0) == handler->Looper()->Thread())
	    handler->MessageReceived(&message);
	else
	    handler->Looper()->PostMessage(&message, handler);
}

void BWebSettings::_PostSetQuota(BHandler* handler, uint32 what, int64 maximumSize)
{
	BMessage message(what);
	message.AddInt64("quota", maximumSize);
	if (find_thread(0) == handler->Looper()->Thread())
	    handler->MessageReceived(&message);
	else
	    handler->Looper()->PostMessage(&message, handler);
}

void BWebSettings::MessageReceived(BMessage* message)
{
	switch (message->what) {
	case HANDLE_SET_PERSISTENT_STORAGE_PATH: {
		BString path;
		if (message->FindString("path", &path) == B_OK)
		    _HandleSetPersistentStoragePath(path);
		break;
	}
	case HANDLE_SET_ICON_DATABASE_PATH: {
		BString path;
		if (message->FindString("path", &path) == B_OK)
		    _HandleSetIconDatabasePath(path);
		break;
	}
	case HANDLE_CLEAR_ICON_DATABASE:
	    _HandleClearIconDatabase();
		break;
	case HANDLE_SEND_ICON_FOR_URL: {
		_HandleSendIconForURL(message);
        break;
	}
	case HANDLE_SET_OFFLINE_STORAGE_PATH: {
		BString path;
		if (message->FindString("path", &path) == B_OK)
		    _HandleSetOfflineStoragePath(path);
		break;
	}
	case HANDLE_SET_OFFLINE_STORAGE_DEFAULT_QUOTA: {
		int64 maximumSize;
		if (message->FindInt64("quota", &maximumSize) == B_OK)
		    _HandleSetOfflineStorageDefaultQuota(maximumSize);
		break;
	}
	case HANDLE_SET_OFFLINE_WEB_APPLICATION_CACHE_PATH: {
		BString path;
		if (message->FindString("path", &path) == B_OK)
		    _HandleSetWebApplicationCachePath(path);
		break;
	}
	case HANDLE_SET_OFFLINE_WEB_APPLICATION_CACHE_QUOTA: {
		int64 maximumSize;
		if (message->FindInt64("quota", &maximumSize) == B_OK)
		    _HandleSetWebApplicationCacheQuota(maximumSize);
		break;
	}
	case HANDLE_SET_LOCAL_STORAGE_PATH: {
		BString path;
		if (message->FindString("path", &path) == B_OK)
		    _HandleSetLocalStoragePath(path);
		break;
	}
	default:
		BHandler::MessageReceived(message);
	}
}

void BWebSettings::_HandleSetPersistentStoragePath(const BString& path)
{
printf("BWebSettings::_HandleSetPersistentStoragePath(%s)\n", path.String());
    BPath storagePath;

    if (!path.Length())
        find_directory(B_USER_DATA_DIRECTORY, &storagePath);
    else
        storagePath.SetTo(path.String());

	create_directory(storagePath.Path(), 0777);

    _HandleSetIconDatabasePath(storagePath.Path());
    _HandleSetWebApplicationCachePath(storagePath.Path());
    BPath dataBasePath(storagePath);
    dataBasePath.Append("Databases");
    _HandleSetOfflineStoragePath(dataBasePath.Path());
    BPath localStoragePath(storagePath);
    dataBasePath.Append("LocalStorage");
    Default()->_HandleSetLocalStoragePath(localStoragePath.Path());

    Default()->fData->localStorageEnabled = true;
    Default()->fData->databasesEnabled = true;
    Default()->fData->offlineWebApplicationCacheEnabled = true;
    Default()->fData->apply();
}

void BWebSettings::_HandleSetIconDatabasePath(const BString& path)
{
    WebCore::iconDatabase()->delayDatabaseCleanup();

    if (path.Length()) {
        WebCore::iconDatabase()->setEnabled(true);
        BEntry entry(path.String());
        if (entry.IsDirectory())
            WebCore::iconDatabase()->open(path);
    } else {
        WebCore::iconDatabase()->setEnabled(false);
        WebCore::iconDatabase()->close();
    }
}

void BWebSettings::_HandleClearIconDatabase()
{
    if (WebCore::iconDatabase()->isEnabled() && WebCore::iconDatabase()->isOpen())
        WebCore::iconDatabase()->removeAllIcons();
}

void BWebSettings::_HandleSendIconForURL(BMessage* message)
{
	BString url;
	BMessage reply;
	BMessenger target;
	if (message->FindString("url", &url) != B_OK
	    || message->FindMessage("reply", &reply) != B_OK
		|| message->FindMessenger("target", &target) != B_OK) {
		return;
	}
    WebCore::Image* image = WebCore::iconDatabase()->iconForPageURL(url.String(),
        WebCore::IntSize(16, 16));

    if (image) {
        const BBitmap* bitmap = image->nativeImageForCurrentFrame();
        BMessage iconArchive;
        if (bitmap && bitmap->Archive(&iconArchive) == B_OK) {
            reply.AddString("url", url.String());
            reply.AddMessage("icon", &iconArchive);
            target.SendMessage(&reply);
        }
    }
}

void BWebSettings::_HandleSetOfflineStoragePath(const BString& path)
{
    WebCore::DatabaseTracker::tracker().setDatabaseDirectoryPath(path);
}

void BWebSettings::_HandleSetOfflineStorageDefaultQuota(int64 maximumSize)
{
    fData->offlineStorageDefaultQuota = maximumSize;
}

void BWebSettings::_HandleSetWebApplicationCachePath(const BString& path)
{
#if ENABLE(OFFLINE_WEB_APPLICATIONS)
    WebCore::cacheStorage().setCacheDirectory(path);
#endif
}

void BWebSettings::_HandleSetWebApplicationCacheQuota(int64 maximumSize)
{
#if ENABLE(OFFLINE_WEB_APPLICATIONS)
    WebCore::cacheStorage().setMaximumSize(maximumSize);
#endif
}

void BWebSettings::_HandleSetLocalStoragePath(const BString& path)
{
    if (!fData->settings)
        return;

    fData->localStoragePath = path;
    fData->apply();
}

