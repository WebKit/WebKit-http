/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

const UserInitiatedProfileName = "org.webkit.profiles.user-initiated";

/**
 * @constructor
 */
WebInspector.ProfileType = function(id, name)
{
    this._id = id;
    this._name = name;
}

WebInspector.ProfileType.URLRegExp = /webkit-profile:\/\/(.+)\/(.+)#([0-9]+)/;

WebInspector.ProfileType.prototype = {
    get buttonTooltip()
    {
        return "";
    },

    get id()
    {
        return this._id;
    },

    get treeItemTitle()
    {
        return this._name;
    },

    get name()
    {
        return this._name;
    },

    buttonClicked: function()
    {
    },

    viewForProfile: function(profile)
    {
        if (!profile._profileView)
            profile._profileView = this.createView(profile);
        return profile._profileView;
    },

    reset: function()
    {
    },

    get description()
    {
        return "";
    },

    // Must be implemented by subclasses.
    createView: function(profile)
    {
        throw new Error("Needs implemented.");
    },

    // Must be implemented by subclasses.
    createSidebarTreeElementForProfile: function(profile)
    {
        throw new Error("Needs implemented.");
    },

    // Must be implemented by subclasses.
    /**
     * @return {WebInspector.ProfileHeader}
     */
    createTemporaryProfile: function()
    {
        throw new Error("Needs implemented.");
    },

    // Must be implemented by subclasses.
    /**
     * @param {ProfilerAgent.ProfileHeader} profile
     * @return {WebInspector.ProfileHeader}
     */
    createProfile: function(profile)
    {
        throw new Error("Needs implemented.");
    }
}

WebInspector.registerLinkifierPlugin(function(title)
{
    var profileStringMatches = WebInspector.ProfileType.URLRegExp.exec(title);
    if (profileStringMatches)
        title = WebInspector.panels.profiles.displayTitleForProfileLink(profileStringMatches[2], profileStringMatches[1]);
    return title;
});

/**
 * @constructor
 * @param {string} profileType
 * @param {string} title
 * @param {number=} uid
 */
WebInspector.ProfileHeader = function(profileType, title, uid)
{
    this.typeId = profileType,
    this.title = title;
    if (uid === undefined) {
        this.uid = -1;
        this.isTemporary = true;
    } else {
        this.uid = uid;
        this.isTemporary = false;
    }
}

/**
 * @constructor
 * @extends {WebInspector.ProfileHeader}
 * @param {string} profileType
 * @param {string} title
 * @param {number} uid
 * @param {number} maxJSObjectId
 */
WebInspector.HeapProfileHeader = function(profileType, title, uid, maxJSObjectId)
{
    WebInspector.ProfileHeader.call(this, profileType, title, uid);
    this.maxJSObjectId = maxJSObjectId;
}

/**
 * @constructor
 * @extends {WebInspector.Panel}
 */
WebInspector.ProfilesPanel = function()
{
    WebInspector.Panel.call(this, "profiles");
    this.registerRequiredCSS("panelEnablerView.css");
    this.registerRequiredCSS("heapProfiler.css");
    this.registerRequiredCSS("profilesPanel.css");

    this.createSplitViewWithSidebarTree();

    this.profilesItemTreeElement = new WebInspector.ProfilesSidebarTreeElement(this);
    this.sidebarTree.appendChild(this.profilesItemTreeElement);

    this._profileTypesByIdMap = {};

    var panelEnablerHeading = WebInspector.UIString("You need to enable profiling before you can use the Profiles panel.");
    var panelEnablerDisclaimer = WebInspector.UIString("Enabling profiling will make scripts run slower.");
    var panelEnablerButton = WebInspector.UIString("Enable Profiling");
    this.panelEnablerView = new WebInspector.PanelEnablerView("profiles", panelEnablerHeading, panelEnablerDisclaimer, panelEnablerButton);
    this.panelEnablerView.addEventListener("enable clicked", this.enableProfiler, this);

    this.profileViews = document.createElement("div");
    this.profileViews.id = "profile-views";
    this.splitView.mainElement.appendChild(this.profileViews);

    this.enableToggleButton = new WebInspector.StatusBarButton("", "enable-toggle-status-bar-item");
    this.enableToggleButton.addEventListener("click", this._toggleProfiling, this);
    if (!Capabilities.profilerCausesRecompilation)
        this.enableToggleButton.element.addStyleClass("hidden");

    this.recordButton = new WebInspector.StatusBarButton("", "record-profile-status-bar-item");
    this.recordButton.addEventListener("click", this.toggleRecordButton, this);

    this.clearResultsButton = new WebInspector.StatusBarButton(WebInspector.UIString("Clear all profiles."), "clear-status-bar-item");
    this.clearResultsButton.addEventListener("click", this._clearProfiles, this);

    this.profileViewStatusBarItemsContainer = document.createElement("div");
    this.profileViewStatusBarItemsContainer.className = "status-bar-items";

    this._profiles = [];
    this._profilerEnabled = !Capabilities.profilerCausesRecompilation;

    this._launcherView = new WebInspector.ProfileLauncherView(this);
    this._launcherView.addEventListener(WebInspector.ProfileLauncherView.EventTypes.ProfileTypeSelected, this._onProfileTypeSelected, this);
    this._reset();
    this._launcherView.setUpEventListeners();

    this._registerProfileType(new WebInspector.CPUProfileType());
    if (!WebInspector.WorkerManager.isWorkerFrontend())
        this._registerProfileType(new WebInspector.CSSSelectorProfileType());
    if (Capabilities.heapProfilerPresent)
        this._registerProfileType(new WebInspector.HeapSnapshotProfileType());

    InspectorBackend.registerProfilerDispatcher(new WebInspector.ProfilerDispatcher(this));

    if (!Capabilities.profilerCausesRecompilation || WebInspector.settings.profilerEnabled.get())
        ProfilerAgent.enable(this._profilerWasEnabled.bind(this));
}

WebInspector.ProfilesPanel.EventTypes = {
    ProfileStarted: "profile-started",
    ProfileFinished: "profile-finished"
}

WebInspector.ProfilesPanel.prototype = {
    get toolbarItemLabel()
    {
        return WebInspector.UIString("Profiles");
    },

    get statusBarItems()
    {
        return [this.enableToggleButton.element, this.recordButton.element, this.clearResultsButton.element, this.profileViewStatusBarItemsContainer];
    },

    toggleRecordButton: function()
    {
        this._selectedProfileType.buttonClicked();
    },

    wasShown: function()
    {
        WebInspector.Panel.prototype.wasShown.call(this);
        this._populateProfiles();
    },

    _profilerWasEnabled: function()
    {
        if (this._profilerEnabled)
            return;

        this._profilerEnabled = true;

        this._reset();
        if (this.isShowing())
            this._populateProfiles();
    },

    _profilerWasDisabled: function()
    {
        if (!this._profilerEnabled)
            return;

        this._profilerEnabled = false;
        this._reset();
    },

    _onProfileTypeSelected: function(event)
    {
        this._selectedProfileType = event.data;
        this.recordButton.title = this._selectedProfileType.buttonTooltip;
    },

    _reset: function()
    {
        WebInspector.Panel.prototype.reset.call(this);

        for (var i = 0; i < this._profiles.length; ++i) {
            var view = this._profiles[i]._profileView;
            if (view) {
                view.detach();
                if ("dispose" in view)
                    view.dispose();
            }

            delete this._profiles[i]._profileView;
        }
        delete this.visibleView;

        delete this.currentQuery;
        this.searchCanceled();

        for (var id in this._profileTypesByIdMap) {
            var profileType = this._profileTypesByIdMap[id];
            var treeElement = profileType.treeElement;
            treeElement.removeChildren();
            treeElement.hidden = true;
            profileType.reset();
        }

        this._profiles = [];
        this._profilesIdMap = {};
        this._profileGroups = {};
        this._profileGroupsForLinks = {};
        this._profilesWereRequested = false;

        this.sidebarTreeElement.removeStyleClass("some-expandable");

        this.profileViews.removeChildren();
        this.profileViewStatusBarItemsContainer.removeChildren();

        this.removeAllListeners();
        this._launcherView.setUpEventListeners();

        this._updateInterface();
        this.profilesItemTreeElement.select();
        this._showLauncherView();
    },

    _showLauncherView: function()
    {
        this.closeVisibleView();
        this.profileViewStatusBarItemsContainer.removeChildren();
        this._launcherView.show(this.splitView.mainElement);
        this.visibleView = this._launcherView;
    },

    _clearProfiles: function()
    {
        ProfilerAgent.clearProfiles();
        this._reset();
    },

    /**
     * @param {WebInspector.ProfileType} profileType
     */
    _registerProfileType: function(profileType)
    {
        this._profileTypesByIdMap[profileType.id] = profileType;
        this._launcherView.addProfileType(profileType);
        profileType.treeElement = new WebInspector.SidebarSectionTreeElement(profileType.treeItemTitle, null, true);
        profileType.treeElement.hidden = true;
        this.sidebarTree.appendChild(profileType.treeElement);
    },

    /**
     * @param {string} text
     * @param {string} profileTypeId
     * @return {string}
     */
    _makeTitleKey: function(text, profileTypeId)
    {
        return escape(text) + '/' + escape(profileTypeId);
    },

    /**
     * @param {number} id
     * @param {string} profileTypeId
     * @return {string}
     */
    _makeKey: function(id, profileTypeId)
    {
        return id + '/' + escape(profileTypeId);
    },

    /**
     * @param {WebInspector.ProfileHeader} profile
     */
    addProfileHeader: function(profile)
    {
        this._removeTemporaryProfile(profile.typeId);

        var typeId = profile.typeId;
        var profileType = this.getProfileType(typeId);
        var sidebarParent = profileType.treeElement;
        sidebarParent.hidden = false;
        var small = false;
        var alternateTitle;

        profile.__profilesPanelProfileType = profileType;
        this._profiles.push(profile);
        this._profilesIdMap[this._makeKey(profile.uid, typeId)] = profile;

        if (!profile.title.startsWith(UserInitiatedProfileName)) {
            var profileTitleKey = this._makeTitleKey(profile.title, typeId);
            if (!(profileTitleKey in this._profileGroups))
                this._profileGroups[profileTitleKey] = [];

            var group = this._profileGroups[profileTitleKey];
            group.push(profile);

            if (group.length === 2) {
                // Make a group TreeElement now that there are 2 profiles.
                group._profilesTreeElement = new WebInspector.ProfileGroupSidebarTreeElement(profile.title);

                // Insert at the same index for the first profile of the group.
                var index = sidebarParent.children.indexOf(group[0]._profilesTreeElement);
                sidebarParent.insertChild(group._profilesTreeElement, index);

                // Move the first profile to the group.
                var selected = group[0]._profilesTreeElement.selected;
                sidebarParent.removeChild(group[0]._profilesTreeElement);
                group._profilesTreeElement.appendChild(group[0]._profilesTreeElement);
                if (selected)
                    group[0]._profilesTreeElement.revealAndSelect();

                group[0]._profilesTreeElement.small = true;
                group[0]._profilesTreeElement.mainTitle = WebInspector.UIString("Run %d", 1);

                this.sidebarTreeElement.addStyleClass("some-expandable");
            }

            if (group.length >= 2) {
                sidebarParent = group._profilesTreeElement;
                alternateTitle = WebInspector.UIString("Run %d", group.length);
                small = true;
            }
        }

        var profileTreeElement = profileType.createSidebarTreeElementForProfile(profile);
        profile.sidebarElement = profileTreeElement;
        profileTreeElement.small = small;
        if (alternateTitle)
            profileTreeElement.mainTitle = alternateTitle;
        profile._profilesTreeElement = profileTreeElement;

        sidebarParent.appendChild(profileTreeElement);
        if (!profile.isTemporary) {
            if (!this.visibleView)
                this.showProfile(profile);
            this.dispatchEventToListeners("profile added");
            this.dispatchEventToListeners(WebInspector.ProfilesPanel.EventTypes.ProfileFinished);
            this.recordButton.toggled = false;
        } else {
            this.dispatchEventToListeners(WebInspector.ProfilesPanel.EventTypes.ProfileStarted);
            this.recordButton.toggled = true;
        }

        this.recordButton.title = this._selectedProfileType.buttonTooltip;
    },

    /**
     * @param {WebInspector.ProfileHeader} profile
     */
    _removeProfileHeader: function(profile)
    {
        var typeId = profile.typeId;
        var profileType = this.getProfileType(typeId);
        var sidebarParent = profileType.treeElement;

        for (var i = 0; i < this._profiles.length; ++i) {
            if (this._profiles[i].uid === profile.uid) {
                profile = this._profiles[i];
                this._profiles.splice(i, 1);
                break;
            }
        }
        delete this._profilesIdMap[this._makeKey(profile.uid, typeId)];

        var profileTitleKey = this._makeTitleKey(profile.title, typeId);
        delete this._profileGroups[profileTitleKey];

        sidebarParent.removeChild(profile._profilesTreeElement);

        if (!profile.isTemporary)
            ProfilerAgent.removeProfile(profile.typeId, profile.uid);

        // No other item will be selected if there aren't any other profiles, so
        // make sure that view gets cleared when the last profile is removed.
        if (!this._profiles.length)
            this.closeVisibleView();
    },

    /**
     * @param {WebInspector.ProfileHeader} profile
     */
    showProfile: function(profile)
    {
        if (!profile || profile.isTemporary)
            return;

        this.closeVisibleView();

        var view = profile.__profilesPanelProfileType.viewForProfile(profile);

        view.show(this.profileViews);

        profile._profilesTreeElement._suppressOnSelect = true;
        profile._profilesTreeElement.revealAndSelect();
        delete profile._profilesTreeElement._suppressOnSelect;

        this.visibleView = view;

        this.profileViewStatusBarItemsContainer.removeChildren();

        var statusBarItems = view.statusBarItems;
        if (statusBarItems)
            for (var i = 0; i < statusBarItems.length; ++i)
                this.profileViewStatusBarItemsContainer.appendChild(statusBarItems[i]);
    },

    /**
     * @param {string} typeId
     * @return {Array.<WebInspector.ProfileHeader>}
     */
    getProfiles: function(typeId)
    {
        var result = [];
        var profilesCount = this._profiles.length;
        for (var i = 0; i < profilesCount; ++i) {
            var profile = this._profiles[i];
            if (!profile.isTemporary && profile.typeId === typeId)
                result.push(profile);
        }
        return result;
    },

    /**
     * @param {string} typeId
     * @return {WebInspector.ProfileHeader}
     */
    findTemporaryProfile: function(typeId)
    {
        var profilesCount = this._profiles.length;
        for (var i = 0; i < profilesCount; ++i)
            if (this._profiles[i].typeId === typeId && this._profiles[i].isTemporary)
                return this._profiles[i];
        return null;
    },

    /**
     * @param {string} typeId
     */
    _removeTemporaryProfile: function(typeId)
    {
        var temporaryProfile = this.findTemporaryProfile(typeId);
        if (temporaryProfile)
            this._removeProfileHeader(temporaryProfile);
    },

    /**
     * @param {WebInspector.ProfileHeader} profile
     */
    hasProfile: function(profile)
    {
        return !!this._profilesIdMap[this._makeKey(profile.uid, profile.typeId)];
    },

    /**
     * @param {string} typeId
     * @param {number} uid
     */
    getProfile: function(typeId, uid)
    {
        return this._profilesIdMap[this._makeKey(uid, typeId)];
    },

    /**
     * @param {number} uid
     * @param {Function} callback
     */
    loadHeapSnapshot: function(uid, callback)
    {
        var profile = this._profilesIdMap[this._makeKey(uid, WebInspector.HeapSnapshotProfileType.TypeId)];
        if (!profile)
            return;

        if (!profile.proxy) {
            function setProfileWait(event) {
                profile.sidebarElement.wait = event.data;
            }
            var worker = new WebInspector.HeapSnapshotWorker();
            worker.addEventListener("wait", setProfileWait, this);
            profile.proxy = worker.createObject("WebInspector.HeapSnapshotLoader");
        }
        var proxy = profile.proxy;
        if (proxy.startLoading(callback)) {
            profile.sidebarElement.subtitle = WebInspector.UIString("Loading\u2026");
            profile.sidebarElement.wait = true;
            ProfilerAgent.getProfile(profile.typeId, profile.uid);
        }
    },

    /**
     * @param {number} uid
     * @param {string} chunk
     */
    _addHeapSnapshotChunk: function(uid, chunk)
    {
        var profile = this._profilesIdMap[this._makeKey(uid, WebInspector.HeapSnapshotProfileType.TypeId)];
        if (!profile || !profile.proxy)
            return;
        profile.proxy.pushJSONChunk(chunk);
    },

    /**
     * @param {number} uid
     */
    _finishHeapSnapshot: function(uid)
    {
        var profile = this._profilesIdMap[this._makeKey(uid, WebInspector.HeapSnapshotProfileType.TypeId)];
        if (!profile || !profile.proxy)
            return;
        var proxy = profile.proxy;
        function parsed(snapshotProxy)
        {
            profile.proxy = snapshotProxy;
            profile.sidebarElement.subtitle = Number.bytesToString(snapshotProxy.totalSize);
            profile.sidebarElement.wait = false;
            var worker = /** @type {WebInspector.HeapSnapshotWorker} */ snapshotProxy.worker;
            worker.startCheckingForLongRunningCalls();
        }
        if (proxy.finishLoading(parsed))
            profile.sidebarElement.subtitle = WebInspector.UIString("Parsing\u2026");
    },

    /**
     * @param {WebInspector.View} view
     */
    showView: function(view)
    {
        this.showProfile(view.profile);
    },

    /**
     * @param {string} typeId
     */
    getProfileType: function(typeId)
    {
        return this._profileTypesByIdMap[typeId];
    },

    /**
     * @param {string} url
     */
    showProfileForURL: function(url)
    {
        var match = url.match(WebInspector.ProfileType.URLRegExp);
        if (!match)
            return;
        this.showProfile(this._profilesIdMap[this._makeKey(Number(match[3]), match[1])]);
    },

    closeVisibleView: function()
    {
        if (this.visibleView)
            this.visibleView.detach();
        delete this.visibleView;
    },

    /**
     * @param {string} title
     * @param {string} typeId
     */
    displayTitleForProfileLink: function(title, typeId)
    {
        title = unescape(title);
        if (title.startsWith(UserInitiatedProfileName)) {
            title = WebInspector.UIString("Profile %d", title.substring(UserInitiatedProfileName.length + 1));
        } else {
            var titleKey = this._makeTitleKey(title, typeId);
            if (!(titleKey in this._profileGroupsForLinks))
                this._profileGroupsForLinks[titleKey] = 0;

            var groupNumber = ++this._profileGroupsForLinks[titleKey];

            if (groupNumber > 2)
                // The title is used in the console message announcing that a profile has started so it gets
                // incremented twice as often as it's displayed
                title += " " + WebInspector.UIString("Run %d", (groupNumber + 1) / 2);
        }

        return title;
    },

    performSearch: function(query)
    {
        this.searchCanceled();

        var searchableViews = this._searchableViews();
        if (!searchableViews || !searchableViews.length)
            return;

        var visibleView = this.visibleView;

        var matchesCountUpdateTimeout = null;

        function updateMatchesCount()
        {
            WebInspector.searchController.updateSearchMatchesCount(this._totalSearchMatches, this);
            matchesCountUpdateTimeout = null;
        }

        function updateMatchesCountSoon()
        {
            if (matchesCountUpdateTimeout)
                return;
            // Update the matches count every half-second so it doesn't feel twitchy.
            matchesCountUpdateTimeout = setTimeout(updateMatchesCount.bind(this), 500);
        }

        function finishedCallback(view, searchMatches)
        {
            if (!searchMatches)
                return;

            this._totalSearchMatches += searchMatches;
            this._searchResults.push(view);

            if (this.searchMatchFound)
                this.searchMatchFound(view, searchMatches);

            updateMatchesCountSoon.call(this);

            if (view === visibleView)
                view.jumpToFirstSearchResult();
        }

        var i = 0;
        var panel = this;
        var boundFinishedCallback = finishedCallback.bind(this);
        var chunkIntervalIdentifier = null;

        // Split up the work into chunks so we don't block the
        // UI thread while processing.

        function processChunk()
        {
            var view = searchableViews[i];

            if (++i >= searchableViews.length) {
                if (panel._currentSearchChunkIntervalIdentifier === chunkIntervalIdentifier)
                    delete panel._currentSearchChunkIntervalIdentifier;
                clearInterval(chunkIntervalIdentifier);
            }

            if (!view)
                return;

            view.currentQuery = query;
            view.performSearch(query, boundFinishedCallback);
        }

        processChunk();

        chunkIntervalIdentifier = setInterval(processChunk, 25);
        this._currentSearchChunkIntervalIdentifier = chunkIntervalIdentifier;
    },

    jumpToNextSearchResult: function()
    {
        if (!this.showView || !this._searchResults || !this._searchResults.length)
            return;

        var showFirstResult = false;

        this._currentSearchResultIndex = this._searchResults.indexOf(this.visibleView);
        if (this._currentSearchResultIndex === -1) {
            this._currentSearchResultIndex = 0;
            showFirstResult = true;
        }

        var currentView = this._searchResults[this._currentSearchResultIndex];

        if (currentView.showingLastSearchResult()) {
            if (++this._currentSearchResultIndex >= this._searchResults.length)
                this._currentSearchResultIndex = 0;
            currentView = this._searchResults[this._currentSearchResultIndex];
            showFirstResult = true;
        }

        if (currentView !== this.visibleView) {
            this.showView(currentView);
            WebInspector.searchController.focusSearchField();
        }

        if (showFirstResult)
            currentView.jumpToFirstSearchResult();
        else
            currentView.jumpToNextSearchResult();
    },

    jumpToPreviousSearchResult: function()
    {
        if (!this.showView || !this._searchResults || !this._searchResults.length)
            return;

        var showLastResult = false;

        this._currentSearchResultIndex = this._searchResults.indexOf(this.visibleView);
        if (this._currentSearchResultIndex === -1) {
            this._currentSearchResultIndex = 0;
            showLastResult = true;
        }

        var currentView = this._searchResults[this._currentSearchResultIndex];

        if (currentView.showingFirstSearchResult()) {
            if (--this._currentSearchResultIndex < 0)
                this._currentSearchResultIndex = (this._searchResults.length - 1);
            currentView = this._searchResults[this._currentSearchResultIndex];
            showLastResult = true;
        }

        if (currentView !== this.visibleView) {
            this.showView(currentView);
            WebInspector.searchController.focusSearchField();
        }

        if (showLastResult)
            currentView.jumpToLastSearchResult();
        else
            currentView.jumpToPreviousSearchResult();
    },

    _searchableViews: function()
    {
        var views = [];

        const visibleView = this.visibleView;
        if (visibleView && visibleView.performSearch)
            views.push(visibleView);

        var profilesLength = this._profiles.length;
        for (var i = 0; i < profilesLength; ++i) {
            var profile = this._profiles[i];
            var view = profile.__profilesPanelProfileType.viewForProfile(profile);
            if (!view.performSearch || view === visibleView)
                continue;
            views.push(view);
        }

        return views;
    },

    searchMatchFound: function(view, matches)
    {
        view.profile._profilesTreeElement.searchMatches = matches;
    },

    searchCanceled: function()
    {
        if (this._searchResults) {
            for (var i = 0; i < this._searchResults.length; ++i) {
                var view = this._searchResults[i];
                if (view.searchCanceled)
                    view.searchCanceled();
                delete view.currentQuery;
            }
        }

        WebInspector.Panel.prototype.searchCanceled.call(this);

        if (this._currentSearchChunkIntervalIdentifier) {
            clearInterval(this._currentSearchChunkIntervalIdentifier);
            delete this._currentSearchChunkIntervalIdentifier;
        }

        this._totalSearchMatches = 0;
        this._currentSearchResultIndex = 0;
        this._searchResults = [];

        if (!this._profiles)
            return;

        for (var i = 0; i < this._profiles.length; ++i) {
            var profile = this._profiles[i];
            profile._profilesTreeElement.searchMatches = 0;
        }
    },

    _updateInterface: function()
    {
        // FIXME: Replace ProfileType-specific button visibility changes by a single ProfileType-agnostic "combo-button" visibility change.
        if (this._profilerEnabled) {
            this.enableToggleButton.title = WebInspector.UIString("Profiling enabled. Click to disable.");
            this.enableToggleButton.toggled = true;
            this.recordButton.visible = true;
            this.profileViewStatusBarItemsContainer.removeStyleClass("hidden");
            this.clearResultsButton.element.removeStyleClass("hidden");
            this.panelEnablerView.detach();
        } else {
            this.enableToggleButton.title = WebInspector.UIString("Profiling disabled. Click to enable.");
            this.enableToggleButton.toggled = false;
            this.recordButton.visible = false;
            this.profileViewStatusBarItemsContainer.addStyleClass("hidden");
            this.clearResultsButton.element.addStyleClass("hidden");
            this.panelEnablerView.show(this.element);
        }
    },

    get profilerEnabled()
    {
        return this._profilerEnabled;
    },

    enableProfiler: function()
    {
        if (this._profilerEnabled)
            return;
        this._toggleProfiling(this.panelEnablerView.alwaysEnabled);
    },

    disableProfiler: function()
    {
        if (!this._profilerEnabled)
            return;
        this._toggleProfiling(this.panelEnablerView.alwaysEnabled);
    },

    _toggleProfiling: function(optionalAlways)
    {
        if (this._profilerEnabled) {
            WebInspector.settings.profilerEnabled.set(false);
            ProfilerAgent.disable(this._profilerWasDisabled.bind(this));
        } else {
            WebInspector.settings.profilerEnabled.set(!!optionalAlways);
            ProfilerAgent.enable(this._profilerWasEnabled.bind(this));
        }
    },

    _populateProfiles: function()
    {
        if (!this._profilerEnabled || this._profilesWereRequested)
            return;

        function populateCallback(error, profileHeaders) {
            if (error)
                return;
            profileHeaders.sort(function(a, b) { return a.uid - b.uid; });
            var profileHeadersLength = profileHeaders.length;
            for (var i = 0; i < profileHeadersLength; ++i) {
                var profileHeader = profileHeaders[i];
                if (!this.hasProfile(profileHeader)) {
                    var profileType = this.getProfileType(profileHeader.typeId);
                    this.addProfileHeader(profileType.createProfile(profileHeader));
                }
            }
        }

        ProfilerAgent.getProfileHeaders(populateCallback.bind(this));

        this._profilesWereRequested = true;
    },

    sidebarResized: function(event)
    {
        var width = event.data;
        // Min width = <number of buttons on the left> * 31
        this.profileViewStatusBarItemsContainer.style.left = Math.max(5 * 31, width) + "px";
    },

    /**
     * @param {string} profileType
     * @param {boolean} isProfiling
     */
    setRecordingProfile: function(profileType, isProfiling)
    {
        var profileTypeObject = this.getProfileType(profileType);
        profileTypeObject.setRecordingProfile(isProfiling);
        var temporaryProfile = this.findTemporaryProfile(profileType);
        if (!!temporaryProfile === isProfiling)
            return;
        if (!temporaryProfile)
            temporaryProfile = profileTypeObject.createTemporaryProfile();
        if (isProfiling)
            this.addProfileHeader(temporaryProfile);
        else
            this._removeTemporaryProfile(profileType);
    },

    takeHeapSnapshot: function()
    {
        var temporaryRecordingProfile = this.findTemporaryProfile(WebInspector.HeapSnapshotProfileType.TypeId);
        if (!temporaryRecordingProfile) {
            var profileTypeObject = this.getProfileType(WebInspector.HeapSnapshotProfileType.TypeId);
            this.addProfileHeader(profileTypeObject.createTemporaryProfile());
        }
        ProfilerAgent.takeHeapSnapshot();
        WebInspector.userMetrics.ProfilesHeapProfileTaken.record();
    },

    /**
     * @param {number} done
     * @param {number} total
     */
    _reportHeapSnapshotProgress: function(done, total)
    {
        var temporaryProfile = this.findTemporaryProfile(WebInspector.HeapSnapshotProfileType.TypeId);
        if (temporaryProfile) {
            temporaryProfile.sidebarElement.subtitle = WebInspector.UIString("%.2f%", (done / total) * 100);
            temporaryProfile.sidebarElement.wait = true;
            if (done >= total)
                this._removeTemporaryProfile(WebInspector.HeapSnapshotProfileType.TypeId);
        }
    }
}

WebInspector.ProfilesPanel.prototype.__proto__ = WebInspector.Panel.prototype;

/**
 * @constructor
 * @implements {ProfilerAgent.Dispatcher}
 */
WebInspector.ProfilerDispatcher = function(profiler)
{
    this._profiler = profiler;
}

WebInspector.ProfilerDispatcher.prototype = {
    /**
     * @param {ProfilerAgent.ProfileHeader} profile
     */
    addProfileHeader: function(profile)
    {
        var profileType = this._profiler.getProfileType(profile.typeId);
        this._profiler.addProfileHeader(profileType.createProfile(profile));
    },

    /**
     * @override
     * @param {number} uid
     * @param {string} chunk
     */
    addHeapSnapshotChunk: function(uid, chunk)
    {
        this._profiler._addHeapSnapshotChunk(uid, chunk);
    },

    /**
     * @override
     * @param {number} uid
     */
    finishHeapSnapshot: function(uid)
    {
        this._profiler._finishHeapSnapshot(uid);
    },

    /**
     * @override
     * @param {boolean} isProfiling
     */
    setRecordingProfile: function(isProfiling)
    {
        this._profiler.setRecordingProfile(WebInspector.CPUProfileType.TypeId, isProfiling);
    },

    /**
     * @override
     */
    resetProfiles: function()
    {
        this._profiler._reset();
    },

    /**
     * @override
     * @param {number} done
     * @param {number} total
     */
    reportHeapSnapshotProgress: function(done, total)
    {
        this._profiler._reportHeapSnapshotProgress(done, total);
    }
}

/**
 * @constructor
 * @extends {WebInspector.SidebarTreeElement}
 */
WebInspector.ProfileSidebarTreeElement = function(profile, titleFormat, className)
{
    this.profile = profile;
    this._titleFormat = titleFormat;

    if (this.profile.title.startsWith(UserInitiatedProfileName))
        this._profileNumber = this.profile.title.substring(UserInitiatedProfileName.length + 1);

    WebInspector.SidebarTreeElement.call(this, className, "", "", profile, false);

    this.refreshTitles();
}

WebInspector.ProfileSidebarTreeElement.prototype = {
    onselect: function()
    {
        if (!this._suppressOnSelect)
            this.treeOutline.panel.showProfile(this.profile);
    },

    ondelete: function()
    {
        this.treeOutline.panel._removeProfileHeader(this.profile);
        return true;
    },

    get mainTitle()
    {
        if (this._mainTitle)
            return this._mainTitle;
        if (this.profile.title.startsWith(UserInitiatedProfileName))
            return WebInspector.UIString(this._titleFormat, this._profileNumber);
        return this.profile.title;
    },

    set mainTitle(x)
    {
        this._mainTitle = x;
        this.refreshTitles();
    },

    set searchMatches(matches)
    {
        if (!matches) {
            if (!this.bubbleElement)
                return;
            this.bubbleElement.removeStyleClass("search-matches");
            this.bubbleText = "";
            return;
        }

        this.bubbleText = matches;
        this.bubbleElement.addStyleClass("search-matches");
    }
}

WebInspector.ProfileSidebarTreeElement.prototype.__proto__ = WebInspector.SidebarTreeElement.prototype;

/**
 * @constructor
 * @extends {WebInspector.SidebarTreeElement}
 * @param {string} title
 * @param {string=} subtitle
 */
WebInspector.ProfileGroupSidebarTreeElement = function(title, subtitle)
{
    WebInspector.SidebarTreeElement.call(this, "profile-group-sidebar-tree-item", title, subtitle, null, true);
}

WebInspector.ProfileGroupSidebarTreeElement.prototype = {
    onselect: function()
    {
        if (this.children.length > 0)
            WebInspector.panels.profiles.showProfile(this.children[this.children.length - 1].profile);
    }
}

WebInspector.ProfileGroupSidebarTreeElement.prototype.__proto__ = WebInspector.SidebarTreeElement.prototype;

/**
 * @constructor
 * @extends {WebInspector.SidebarTreeElement}
 */
WebInspector.ProfilesSidebarTreeElement = function(panel)
{
    this._panel = panel;
    this.small = false;

    WebInspector.SidebarTreeElement.call(this, "profile-launcher-view-tree-item", WebInspector.UIString("Profiles"), "", null, false);
}

WebInspector.ProfilesSidebarTreeElement.prototype = {
    onselect: function()
    {
        this._panel._showLauncherView();
    },

    get selectable()
    {
        return true;
    }
}

WebInspector.ProfilesSidebarTreeElement.prototype.__proto__ = WebInspector.SidebarTreeElement.prototype;
