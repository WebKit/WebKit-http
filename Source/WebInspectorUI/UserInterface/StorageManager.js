/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
 * Copyright (C) 2013 Samsung Electronics. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

WebInspector.StorageManager = function()
{
    WebInspector.Object.call(this);

    DOMStorageAgent.enable();
    DatabaseAgent.enable();

    WebInspector.Frame.addEventListener(WebInspector.Frame.Event.MainResourceDidChange, this._mainResourceDidChange, this);

    // COMPATIBILITY (iOS 6): DOMStorage was discovered via a DOMStorageObserver event. Now DOM Storage
    // is added whenever a new securityOrigin is discovered. Check for DOMStorageAgent.getDOMStorageItems,
    // which was renamed at the same time the change to start using securityOrigin was made.
    if (DOMStorageAgent.getDOMStorageItems)
        WebInspector.Frame.addEventListener(WebInspector.Frame.Event.SecurityOriginDidChange, this._securityOriginDidChange, this);

    this.initialize();
};

WebInspector.StorageManager.Event = {
    CookieStorageObjectWasAdded: "storage-manager-cookie-storage-object-was-added",
    DOMStorageObjectWasAdded: "storage-manager-dom-storage-object-was-added",
    DOMStorageObjectWasInspected: "storage-dom-object-was-inspected",
    DatabaseWasAdded: "storage-manager-database-was-added",
    DatabaseWasInspected: "storage-object-was-inspected",
    Cleared: "storage-manager-cleared"
};

WebInspector.StorageManager.prototype = {
    constructor: WebInspector.StorageManager,

    // Public

    initialize: function()
    {
        this._domStorageObjects = [];
        this._databaseObjects = [];
        this._cookieStorageObjects = {};
    },

    domStorageWasAdded: function(id, host, isLocalStorage)
    {
        var domStorage = new WebInspector.DOMStorageObject(id, host, isLocalStorage);

        this._domStorageObjects.push(domStorage);
        this.dispatchEventToListeners(WebInspector.StorageManager.Event.DOMStorageObjectWasAdded, {domStorage: domStorage});
    },

    databaseWasAdded: function(id, host, name, version)
    {
        var database = new WebInspector.DatabaseObject(id, host, name, version);

        this._databaseObjects.push(database);
        this.dispatchEventToListeners(WebInspector.StorageManager.Event.DatabaseWasAdded, {database: database});
    },

    domStorageWasUpdated: function(id)
    {
        var domStorageView = this._domStorageViewForId(id);
        if (!domStorageView)
            return;

        console.assert(domStorageView instanceof WebInspector.DOMStorageContentView);
        domStorageView.update();
    },

    domStorageItemsCleared: function(id)
    {
        var domStorageView = this._domStorageViewForId(id);
        if (!domStorageView)
            return;

        console.assert(domStorageView instanceof WebInspector.DOMStorageContentView);
        domStorageView.itemsCleared();
    },

    domStorageItemRemoved: function(id, key)
    {
        var domStorageView = this._domStorageViewForId(id);
        if (!domStorageView)
            return;

        console.assert(domStorageView instanceof WebInspector.DOMStorageContentView);
        domStorageView.itemRemoved(key);
    },

    domStorageItemAdded: function(id, key, value)
    {
        var domStorageView = this._domStorageViewForId(id);
        if (!domStorageView)
            return;

        console.assert(domStorageView instanceof WebInspector.DOMStorageContentView);
        domStorageView.itemAdded(key, value);
    },

    domStorageItemUpdated: function(id, key, oldValue, value)
    {
        var domStorageView = this._domStorageViewForId(id);
        if (!domStorageView)
            return;

        console.assert(domStorageView instanceof WebInspector.DOMStorageContentView);
        domStorageView.itemUpdated(key, oldValue, value);
    },

    inspectDatabase: function(id)
    {
        var database = this._databaseForId(id);
        console.assert(database);
        if (!database)
            return;
        this.dispatchEventToListeners(WebInspector.StorageManager.Event.DatabaseWasInspected, {database: database});
    },

    inspectDOMStorage: function(id)
    {
        var domStorage = this._domStorageForId(id);
        console.assert(domStorage);
        if (!domStorage)
            return;
        this.dispatchEventToListeners(WebInspector.StorageManager.Event.DOMStorageObjectWasInspected, {domStorage: domStorage});
    },

    objectForCookie: function(cookie, matchOnTypeAlone)
    {
        console.assert(cookie.type);

        var findMatchingObjectInArray = function (array, matchesFunction) {
            for (var i = 0; i < array.length; ++i)
                if (matchesFunction.call(null, array[i]))
                    return array[i];

            return matchOnTypeAlone && array.length ? array[0] : null;
        };

        if (cookie.type === WebInspector.ContentViewCookieType.CookieStorage) {
            if (this._cookieStorageObjects[cookie.host])
                return this._cookieStorageObjects[cookie.host];

            if (!matchOnTypeAlone)
                return null;

            // If we just want any cookie storage object, use the first one.
            for (var key in this._cookieStorageObjects)
                return this._cookieStorageObjects[key];

            return null;
        }

        if (cookie.type === WebInspector.ContentViewCookieType.DOMStorage) {
            return findMatchingObjectInArray(this._domStorageObjects, function(object) {
                return object.host === cookie.host && object.isLocalStorage() === cookie.isLocalStorage;
            });
        }

        if (cookie.type === WebInspector.ContentViewCookieType.Database) {
            return findMatchingObjectInArray(this._databaseObjects, function(object) {
                return object.host === cookie.host && object.name === cookie.name;
            });
        }

        // FIXME: This isn't easy to implement like the others since DatabaseTreeElement
        // creates database table objects, and they aren't known by StorageManager. Just
        // display the database instead.
        if (cookie.type === WebInspector.ContentViewCookieType.DatabaseTable) {
            return findMatchingObjectInArray(this._databaseObjects, function(object) {
                return object.host === cookie.host && object.database === cookie.name;
            });
        }

        console.assert("Unknown content view cookie: ", cookie);
        return null;
    },

    // Private

    _mainResourceDidChange: function(event)
    {
        console.assert(event.target instanceof WebInspector.Frame);

        if (event.target.isMainFrame()) {
            // If we are dealing with the main frame, we want to clear our list of objects, because we are navigating to a new page.
            this.initialize();
            this.dispatchEventToListeners(WebInspector.StorageManager.Event.Cleared);

            this._addDOMStorageIfNeeded(event.target);
        }

        // Add the host of the frame that changed the main resource to the list of hosts there could be cookies for.
        var host = parseURL(event.target.url).host;
        if (!host)
            return;

        if (this._cookieStorageObjects[host])
            return;

        this._cookieStorageObjects[host] = new WebInspector.CookieStorageObject(host);
        this.dispatchEventToListeners(WebInspector.StorageManager.Event.CookieStorageObjectWasAdded, {cookieStorage: this._cookieStorageObjects[host]});
    },

    _addDOMStorageIfNeeded: function(frame)
    {
        // Don't show storage if we don't have a security origin (about:blank).
        if (!frame.securityOrigin || frame.securityOrigin === "://")
            return;

        // FIXME: Consider passing the other parts of the origin along to domStorageWasAdded.

        var localStorageIdentifier = {securityOrigin: frame.securityOrigin, isLocalStorage: true};
        if (!this._domStorageForId(localStorageIdentifier))
            this.domStorageWasAdded(localStorageIdentifier, frame.mainResource.urlComponents.host, true);

        var sessionStorageIdentifier = {securityOrigin: frame.securityOrigin, isLocalStorage: false};
        if (!this._domStorageForId(sessionStorageIdentifier))
            this.domStorageWasAdded(sessionStorageIdentifier, frame.mainResource.urlComponents.host, false);
    },

    _securityOriginDidChange: function(event)
    {
        console.assert(event.target instanceof WebInspector.Frame);

        this._addDOMStorageIfNeeded(event.target);
    },

    _databaseForId: function(id)
    {
        for (var i = 0; i < this._databaseObjects.length; ++i) {
            if (this._databaseObjects[i].id === id)
                return this._databaseObjects[i];
        }

        return null;
    },

    _domStorageForId: function(id)
    {
        for (var i = 0; i < this._domStorageObjects.length; ++i) {
            // The id is an object, so we need to compare the properties using Object.shallowEqual.
            // COMPATIBILITY (iOS 6): The id was a string. Object.shallowEqual works for both.
            if (Object.shallowEqual(this._domStorageObjects[i].id, id))
                return this._domStorageObjects[i];
        }

        return null;
    },

    _domStorageViewForId: function(id)
    {
        var domStorage = this._domStorageForId(id);
        if (!domStorage)
            return null;

        return WebInspector.contentBrowser.contentViewContainer.contentViewForRepresentedObject(domStorage, true);
    }
};

WebInspector.StorageManager.prototype.__proto__ = WebInspector.Object.prototype;
