/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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

WebInspector.ResourceSidebarPanel = function() {
    WebInspector.NavigationSidebarPanel.call(this, "resource", WebInspector.UIString("Resources"), "Images/NavigationItemStorage.svg", "1", true, false, true);

    var searchElement = document.createElement("div");
    searchElement.classList.add("search-bar");
    this.element.appendChild(searchElement);

    this._inputElement = document.createElement("input");
    this._inputElement.type = "search";
    this._inputElement.spellcheck = false;
    this._inputElement.addEventListener("search", this._searchFieldChanged.bind(this));
    this._inputElement.addEventListener("input", this._searchFieldInput.bind(this));
    this._inputElement.setAttribute("results", 5);
    this._inputElement.setAttribute("autosave", "inspector-search");
    this._inputElement.setAttribute("placeholder", WebInspector.UIString("Search Resource Content"));
    searchElement.appendChild(this._inputElement);

    this.filterBar.placeholder = WebInspector.UIString("Filter Resource List");

    this._waitingForInitialMainFrame = true;
    this._lastSearchedPageSetting = new WebInspector.Setting("last-searched-page", null);

    this._searchQuerySetting = new WebInspector.Setting("search-sidebar-query", "");
    this._inputElement.value = this._searchQuerySetting.value;

    this._searchKeyboardShortcut = new WebInspector.KeyboardShortcut(WebInspector.KeyboardShortcut.Modifier.CommandOrControl | WebInspector.KeyboardShortcut.Modifier.Shift, "F", this._focusSearchField.bind(this));

    this._localStorageRootTreeElement = null;
    this._sessionStorageRootTreeElement = null;

    this._databaseRootTreeElement = null;
    this._databaseHostTreeElementMap = {};

    this._indexedDatabaseRootTreeElement = null;
    this._indexedDatabaseHostTreeElementMap = {};

    this._cookieStorageRootTreeElement = null;

    this._applicationCacheRootTreeElement = null;
    this._applicationCacheURLTreeElementMap = {};

    WebInspector.storageManager.addEventListener(WebInspector.StorageManager.Event.CookieStorageObjectWasAdded, this._cookieStorageObjectWasAdded, this);
    WebInspector.storageManager.addEventListener(WebInspector.StorageManager.Event.DOMStorageObjectWasAdded, this._domStorageObjectWasAdded, this);
    WebInspector.storageManager.addEventListener(WebInspector.StorageManager.Event.DOMStorageObjectWasInspected, this._domStorageObjectWasInspected, this);
    WebInspector.storageManager.addEventListener(WebInspector.StorageManager.Event.DatabaseWasAdded, this._databaseWasAdded, this);
    WebInspector.storageManager.addEventListener(WebInspector.StorageManager.Event.DatabaseWasInspected, this._databaseWasInspected, this);
    WebInspector.storageManager.addEventListener(WebInspector.StorageManager.Event.IndexedDatabaseWasAdded, this._indexedDatabaseWasAdded, this);
    WebInspector.storageManager.addEventListener(WebInspector.StorageManager.Event.Cleared, this._storageCleared, this);

    WebInspector.applicationCacheManager.addEventListener(WebInspector.ApplicationCacheManager.Event.FrameManifestAdded, this._frameManifestAdded, this);
    WebInspector.applicationCacheManager.addEventListener(WebInspector.ApplicationCacheManager.Event.FrameManifestRemoved, this._frameManifestRemoved, this);

    WebInspector.frameResourceManager.addEventListener(WebInspector.FrameResourceManager.Event.MainFrameDidChange, this._mainFrameDidChange, this);
    WebInspector.frameResourceManager.addEventListener(WebInspector.FrameResourceManager.Event.FrameWasAdded, this._frameWasAdded, this);

    WebInspector.domTreeManager.addEventListener(WebInspector.DOMTreeManager.Event.DOMNodeWasInspected, this._domNodeWasInspected, this);

    WebInspector.debuggerManager.addEventListener(WebInspector.DebuggerManager.Event.ScriptAdded, this._scriptWasAdded, this);
    WebInspector.debuggerManager.addEventListener(WebInspector.DebuggerManager.Event.ScriptsCleared, this._scriptsCleared, this);

    this._resourcesContentTreeOutline = this.contentTreeOutline;
    this._searchContentTreeOutline = this.createContentTreeOutline();

    this._resourcesContentTreeOutline.onselect = this._treeElementSelected.bind(this);
    this._searchContentTreeOutline.onselect = this._treeElementSelected.bind(this);

    this._resourcesContentTreeOutline.includeSourceMapResourceChildren = true;

    if (WebInspector.debuggableType === WebInspector.DebuggableType.JavaScript)
        this._resourcesContentTreeOutline.element.classList.add(WebInspector.NavigationSidebarPanel.HideDisclosureButtonsStyleClassName);
};

WebInspector.ResourceSidebarPanel.prototype = {
    constructor: WebInspector.ResourceSidebarPanel,

    // Public

    showDefaultContentView: function()
    {
        if (WebInspector.frameResourceManager.mainFrame) {
            this.showMainFrameSourceCode();
            return;
        }

        var firstTreeElement = this._resourcesContentTreeOutline.children[0];
        if (firstTreeElement)
            firstTreeElement.revealAndSelect();
    },

    get contentTreeOutlineToAutoPrune()
    {
        return this._searchContentTreeOutline;
    },

    showMainFrameDOMTree: function(nodeToSelect, preventFocusChange)
    {
        var contentView = WebInspector.contentBrowser.contentViewForRepresentedObject(WebInspector.frameResourceManager.mainFrame);
        contentView.showDOMTree(nodeToSelect, preventFocusChange);
        WebInspector.contentBrowser.showContentView(contentView);
    },

    showMainFrameSourceCode: function()
    {
        var contentView = WebInspector.contentBrowser.contentViewForRepresentedObject(WebInspector.frameResourceManager.mainFrame);
        contentView.showSourceCode();
        WebInspector.contentBrowser.showContentView(contentView);
    },

    showContentFlowDOMTree: function(contentFlow, nodeToSelect, preventFocusChange)
    {
        var contentView = WebInspector.contentBrowser.contentViewForRepresentedObject(contentFlow);
        if (nodeToSelect)
            contentView.selectAndRevealDOMNode(nodeToSelect, preventFocusChange);
        WebInspector.contentBrowser.showContentView(contentView);
    },

    showSourceCodeForFrame: function(frameIdentifier, revealAndSelectTreeElement)
    {
        delete this._frameIdentifierToShowSourceCodeWhenAvailable;

        // We can't show anything until we have the main frame in the sidebar.
        // Otherwise the path components in the navigation bar would be missing.
        var frame = WebInspector.frameResourceManager.frameForIdentifier(frameIdentifier);
        if (!frame || !this._mainFrameTreeElement) {
            this._frameIdentifierToShowSourceCodeWhenAvailable = frameIdentifier;
            return;
        }

        var contentView = WebInspector.contentBrowser.contentViewForRepresentedObject(frame);
        console.assert(contentView);
        if (!contentView)
            return;

        contentView.showSourceCode();
        WebInspector.contentBrowser.showContentView(contentView);

        if (revealAndSelectTreeElement)
            this.treeElementForRepresentedObject(frame).revealAndSelect(true, true, true, true);
    },

    showSourceCode: function(sourceCode, positionToReveal, textRangeToSelect, forceUnformatted)
    {
        console.assert(!positionToReveal || positionToReveal instanceof WebInspector.SourceCodePosition, positionToReveal);
        var representedObject = sourceCode;

        if (representedObject instanceof WebInspector.Script) {
            // A script represented by a resource should always show the resource.
            representedObject = representedObject.resource || representedObject;
        }

        // A main resource is always represented by its parent frame.
        if (representedObject instanceof WebInspector.Resource && representedObject.isMainResource())
            representedObject = representedObject.parentFrame;

        var cookie = positionToReveal ? {lineNumber: positionToReveal.lineNumber, columnNumber: positionToReveal.columnNumber} : {};
        WebInspector.contentBrowser.showContentViewForRepresentedObject(representedObject, cookie);
    },

    showSourceCodeLocation: function(sourceCodeLocation)
    {
        this.showSourceCode(sourceCodeLocation.displaySourceCode, sourceCodeLocation.displayPosition());
    },

    showOriginalUnformattedSourceCodeLocation: function(sourceCodeLocation)
    {
        this.showSourceCode(sourceCodeLocation.sourceCode, sourceCodeLocation.position(), undefined, true);
    },

    showOriginalOrFormattedSourceCodeLocation: function(sourceCodeLocation)
    {
        this.showSourceCode(sourceCodeLocation.sourceCode, sourceCodeLocation.formattedPosition());
    },

    showSourceCodeTextRange: function(sourceCodeTextRange)
    {
        var textRangeToSelect = sourceCodeTextRange.displayTextRange;
        this.showSourceCode(sourceCodeTextRange.displaySourceCode, textRangeToSelect.startPosition(), textRangeToSelect);
    },

    showOriginalOrFormattedSourceCodeTextRange: function(sourceCodeTextRange)
    {
        var textRangeToSelect = sourceCodeTextRange.formattedTextRange;
        this.showSourceCode(sourceCodeTextRange.sourceCode, textRangeToSelect.startPosition(), textRangeToSelect);
    },

    showResource: function(resource)
    {
        WebInspector.contentBrowser.showContentViewForRepresentedObject(resource.isMainResource() ? resource.parentFrame : resource);
    },

    showResourceRequest: function(resource)
    {
        var contentView = WebInspector.contentBrowser.contentViewForRepresentedObject(resource.isMainResource() ? resource.parentFrame : resource);

        if (contentView instanceof WebInspector.FrameContentView)
            var resourceContentView = contentView.showResource();
        else if (contentView instanceof WebInspector.ResourceClusterContentView)
            var resourceContentView = contentView;

        console.assert(resourceContentView instanceof WebInspector.ResourceClusterContentView);
        if (!(resourceContentView instanceof WebInspector.ResourceClusterContentView))
            return;

        resourceContentView.showRequest();

        WebInspector.contentBrowser.showContentView(contentView);
    },

    treeElementForRepresentedObject: function(representedObject)
    {
        // A custom implementation is needed for this since the frames are populated lazily.

        // The Frame is used as the representedObject instead of the main resource in our tree.
        if (representedObject instanceof WebInspector.Resource && representedObject.parentFrame && representedObject.parentFrame.mainResource === representedObject)
            representedObject = representedObject.parentFrame;

        function isAncestor(ancestor, resourceOrFrame)
        {
            // SourceMapResources are descendants of another SourceCode object.
            if (resourceOrFrame instanceof WebInspector.SourceMapResource) {
                if (resourceOrFrame.sourceMap.originalSourceCode === ancestor)
                    return true;

                // Not a direct ancestor, so check the ancestors of the parent SourceCode object.
                resourceOrFrame = resourceOrFrame.sourceMap.originalSourceCode;
            }

            var currentFrame = resourceOrFrame.parentFrame;
            while (currentFrame) {
                if (currentFrame === ancestor)
                    return true;
                currentFrame = currentFrame.parentFrame;
            }

            return false;
        }

        function getParent(resourceOrFrame)
        {
            // SourceMapResources are descendants of another SourceCode object.
            if (resourceOrFrame instanceof WebInspector.SourceMapResource)
                return resourceOrFrame.sourceMap.originalSourceCode;
            return resourceOrFrame.parentFrame;
        }

        var treeElement = this._resourcesContentTreeOutline.findTreeElement(representedObject, isAncestor, getParent);
        if (treeElement)
            return treeElement;

        // Only special case Script objects.
        if (!(representedObject instanceof WebInspector.Script))
            return null;

        // If the Script has a URL we should have found it earlier.
        if (representedObject.url) {
            console.error("Didn't find a ScriptTreeElement for a Script with a URL.");
            return null;
        }

        // Since the Script does not have a URL we consider it an 'anonymous' script. These scripts happen from calls to
        // window.eval() or browser features like Auto Fill and Reader. They are not normally added to the sidebar, but since
        // we have a ScriptContentView asking for the tree element we will make a ScriptTreeElement on demand and add it.

        if (!this._anonymousScriptsFolderTreeElement)
            this._anonymousScriptsFolderTreeElement = new WebInspector.FolderTreeElement(WebInspector.UIString("Anonymous Scripts"));

        if (!this._anonymousScriptsFolderTreeElement.parent) {
            var index = insertionIndexForObjectInListSortedByFunction(this._anonymousScriptsFolderTreeElement, this._resourcesContentTreeOutline.children, this._compareTreeElements);
            this._resourcesContentTreeOutline.insertChild(this._anonymousScriptsFolderTreeElement, index);
        }

        var scriptTreeElement = new WebInspector.ScriptTreeElement(representedObject);
        this._anonymousScriptsFolderTreeElement.appendChild(scriptTreeElement);

        return scriptTreeElement;
    },

    performSearch: function(searchTerm)
    {
        // Before performing a new search, clear the old search.
        this._searchContentTreeOutline.removeChildren();

        this._inputElement.value = searchTerm;
        this._searchQuerySetting.value = searchTerm;
        this._lastSearchedPageSetting.value = searchTerm && WebInspector.frameResourceManager.mainFrame ? WebInspector.frameResourceManager.mainFrame.url.hash : null;

        this.hideEmptyContentPlaceholder();

        searchTerm = searchTerm.trim();
        if (!searchTerm.length) {
            this.filterBar.placeholder = WebInspector.UIString("Filter Resource List");
            this.contentTreeOutline = this._resourcesContentTreeOutline;
            return;
        }

        this.filterBar.placeholder = WebInspector.UIString("Filter Search Results");
        this.contentTreeOutline = this._searchContentTreeOutline;

        var updateEmptyContentPlaceholderTimeout = null;

        function updateEmptyContentPlaceholderSoon()
        {
            if (updateEmptyContentPlaceholderTimeout)
                return;
            updateEmptyContentPlaceholderTimeout = setTimeout(updateEmptyContentPlaceholder.bind(this), 100);
        }

        function updateEmptyContentPlaceholder()
        {
            if (updateEmptyContentPlaceholderTimeout) {
                clearTimeout(updateEmptyContentPlaceholderTimeout);
                updateEmptyContentPlaceholderTimeout = null;
            }

            this.updateEmptyContentPlaceholder(WebInspector.UIString("No Search Results"));
        }

        function resourcesCallback(error, result)
        {
            updateEmptyContentPlaceholderSoon.call(this);

            if (error)
                return;

            for (var i = 0; i < result.length; ++i) {
                var searchResult = result[i];
                if (!searchResult.url || !searchResult.frameId)
                    continue;

                function resourceCallback(url, error, resourceMatches)
                {
                    updateEmptyContentPlaceholderSoon.call(this);

                    if (error || !resourceMatches || !resourceMatches.length)
                        return;

                    var frame = WebInspector.frameResourceManager.frameForIdentifier(searchResult.frameId);
                    if (!frame)
                        return;

                    var resource = frame.url === url ? frame.mainResource : frame.resourceForURL(url);
                    if (!resource)
                        return;

                    var resourceTreeElement = this._searchTreeElementForResource(resource);

                    for (var i = 0; i < resourceMatches.length; ++i) {
                        var match = resourceMatches[i];

                        var lineMatch;
                        var searchRegex = new RegExp(searchTerm.escapeForRegExp(), "gi");
                        while ((searchRegex.lastIndex < match.lineContent.length) && (lineMatch = searchRegex.exec(match.lineContent))) {
                            var matchObject = new WebInspector.ResourceSearchMatchObject(resource, match.lineContent, searchTerm, new WebInspector.TextRange(match.lineNumber, lineMatch.index, match.lineNumber, searchRegex.lastIndex));
                            var matchTreeElement = new WebInspector.SearchResultTreeElement(matchObject);
                            resourceTreeElement.appendChild(matchTreeElement);
                        }
                    }

                    updateEmptyContentPlaceholder.call(this);
                }

                PageAgent.searchInResource(searchResult.frameId, searchResult.url, searchTerm, false, false, resourceCallback.bind(this, searchResult.url));
            }
        }

        function domCallback(error, searchId, resultsCount)
        {
            updateEmptyContentPlaceholderSoon.call(this);

            if (error || !resultsCount)
                return;

            this._domSearchIdentifier = searchId;

            function domSearchResults(error, nodeIds)
            {
                updateEmptyContentPlaceholderSoon.call(this);

                if (error)
                    return;

                for (var i = 0; i < nodeIds.length; ++i) {
                    // If someone started a new search, then return early and stop showing seach results from the old query.
                    if (this._domSearchIdentifier !== searchId)
                        return;

                    var domNode = WebInspector.domTreeManager.nodeForId(nodeIds[i]);
                    if (!domNode || !domNode.ownerDocument)
                        continue;

                    // We do not display the document node when the search query is "/". We don't have anything to display in the content view for it.
                    if (domNode.nodeType() === Node.DOCUMENT_NODE)
                        continue;

                    // FIXME: Use this should use a frame to do resourceForURL, but DOMAgent does not provide a frameId.
                    var resource = WebInspector.frameResourceManager.resourceForURL(domNode.ownerDocument.documentURL);
                    if (!resource)
                        continue;

                    var resourceTreeElement = this._searchTreeElementForResource(resource);

                    var domNodeTitle = WebInspector.DOMSearchMatchObject.titleForDOMNode(domNode);
                    var searchRegex = new RegExp(searchTerm.escapeForRegExp(), "gi");

                    // Textual matches.
                    var lineMatch;
                    var didFindTextualMatch = false;
                    while ((searchRegex.lastIndex < domNodeTitle.length) && (lineMatch = searchRegex.exec(domNodeTitle))) {
                        var matchObject = new WebInspector.DOMSearchMatchObject(resource, domNode, domNodeTitle, searchTerm, new WebInspector.TextRange(0, lineMatch.index, 0, searchRegex.lastIndex));
                        var matchTreeElement = new WebInspector.SearchResultTreeElement(matchObject);
                        resourceTreeElement.appendChild(matchTreeElement);
                        didFindTextualMatch = true;
                    }

                    // Non-textual matches are CSS Selector or XPath matches. In such cases, display the node entirely highlighted.
                    if (!didFindTextualMatch) {
                        var matchObject = new WebInspector.DOMSearchMatchObject(resource, domNode, domNodeTitle, domNodeTitle, new WebInspector.TextRange(0, 0, 0, domNodeTitle.length));
                        var matchTreeElement = new WebInspector.SearchResultTreeElement(matchObject);
                        resourceTreeElement.appendChild(matchTreeElement);
                    }

                    updateEmptyContentPlaceholder.call(this);
                }
            }

            DOMAgent.getSearchResults(searchId, 0, resultsCount, domSearchResults.bind(this));
        }

        if (window.DOMAgent)
            WebInspector.domTreeManager.requestDocument();

        // FIXME: Should we be searching for regexes or just plain text?
        if (window.PageAgent)
            PageAgent.searchInResources(searchTerm, false, false, resourcesCallback.bind(this));

        if (window.DOMAgent) {
            if ("_domSearchIdentifier" in this) {
                DOMAgent.discardSearchResults(this._domSearchIdentifier);
                delete this._domSearchIdentifier;
            }

            DOMAgent.performSearch(searchTerm, domCallback.bind(this));
        }

        // FIXME: Resource search should work in JSContext inspection.
        // <https://webkit.org/b/131252> Web Inspector: JSContext inspection Resource search does not work
        if (!window.DOMAgent && !window.PageAgent)
            updateEmptyContentPlaceholderSoon.call(this);
    },

    // Private

    _searchFieldChanged: function(event)
    {
        this.performSearch(event.target.value);
    },

    _searchFieldInput: function(event)
    {
        // If the search field is cleared, immediately clear the search results tree outline.
        if (!event.target.value.length && this.contentTreeOutline === this._searchContentTreeOutline)
            this.performSearch("");
    },

    _searchTreeElementForResource: function(resource)
    {
        // FIXME: This should take a frame ID (if one is available) - so we can differentiate between multiple resources
        // with the same URL.

        var resourceTreeElement = this._searchContentTreeOutline.getCachedTreeElement(resource);
        if (!resourceTreeElement) {
            resourceTreeElement = new WebInspector.ResourceTreeElement(resource);
            resourceTreeElement.hasChildren = true;
            resourceTreeElement.expand();

            this._searchContentTreeOutline.appendChild(resourceTreeElement);
        }

        return resourceTreeElement;
    },

    _focusSearchField: function(keyboardShortcut, event)
    {
        this.show();

        this._inputElement.select();
    },

    _mainFrameDidChange: function(event)
    {
        if (this._mainFrameTreeElement) {
            this._mainFrameTreeElement.frame.removeEventListener(WebInspector.Frame.Event.MainResourceDidChange, this._mainFrameMainResourceDidChange, this);
            this._resourcesContentTreeOutline.removeChild(this._mainFrameTreeElement);
            this._mainFrameTreeElement = null;
        }

        var newFrame = WebInspector.frameResourceManager.mainFrame;
        if (newFrame) {
            newFrame.addEventListener(WebInspector.Frame.Event.MainResourceDidChange, this._mainFrameMainResourceDidChange, this);
            this._mainFrameTreeElement = new WebInspector.FrameTreeElement(newFrame);
            this._resourcesContentTreeOutline.insertChild(this._mainFrameTreeElement, 0);

            // Select by default. Allow onselect if we aren't showing a content view.
            if (!this._resourcesContentTreeOutline.selectedTreeElement)
                this._mainFrameTreeElement.revealAndSelect(true, false, !!WebInspector.contentBrowser.currentContentView);

            if (this._frameIdentifierToShowSourceCodeWhenAvailable)
                this.showSourceCodeForFrame(this._frameIdentifierToShowSourceCodeWhenAvailable, true);
        }

        // We only care about the first time the main frame changes.
        if (!this._waitingForInitialMainFrame)
            return;

        // Only if there is a main frame.
        if (!newFrame)
            return;

        delete this._waitingForInitialMainFrame;

        // Only if the last page searched is the same as the current page.
        if (this._lastSearchedPageSetting.value !== newFrame.url.hash)
            return;

        // Search for whatever is in the input field. This was populated with the last used search term.
        this.performSearch(this._inputElement.value);
    },

    _mainFrameMainResourceDidChange: function(event)
    {
        var currentContentView = WebInspector.contentBrowser.currentContentView;
        var wasShowingResourceContentView = currentContentView instanceof WebInspector.ResourceContentView
            || currentContentView instanceof WebInspector.FrameContentView || currentContentView instanceof WebInspector.ScriptContentView;

        // Close all resource and frame content views since the main frame has navigated and all resources are cleared.
        WebInspector.contentBrowser.contentViewContainer.closeAllContentViewsOfPrototype(WebInspector.ResourceClusterContentView);
        WebInspector.contentBrowser.contentViewContainer.closeAllContentViewsOfPrototype(WebInspector.FrameContentView);
        WebInspector.contentBrowser.contentViewContainer.closeAllContentViewsOfPrototype(WebInspector.ScriptContentView);

        function delayedWork()
        {
            // Show the main frame since there is no content view showing or we were showing a resource before.
            // FIXME: We could try to select the same resource that was selected before in the case of a reload.
            if (!WebInspector.contentBrowser.currentContentView || wasShowingResourceContentView)
                this._mainFrameTreeElement.revealAndSelect(true, false);
        }

        // Delay this work because other listeners of this event might not have fired yet. So selecting the main frame
        // before those listeners do their work might cause the content of the old page to show instead of the new page.
        setTimeout(delayedWork.bind(this), 0);
    },

    _frameWasAdded: function(event)
    {
        if (!this._frameIdentifierToShowSourceCodeWhenAvailable)
            return;

        var frame = event.data.frame;
        if (frame.id !== this._frameIdentifierToShowSourceCodeWhenAvailable)
            return;

        this.showSourceCodeForFrame(frame.id, true);
    },

    _scriptWasAdded: function(event)
    {
        var script = event.data.script;

        // We don't add scripts without URLs here. Those scripts can quickly clutter the interface and
        // are usually more transient. They will get added if/when they need to be shown in a content view.
        if (!script.url)
            return;

        // Exclude inspector scripts.
        if (script.url.indexOf("__WebInspector") === 0)
            return;

        // If the script URL matches a resource we can assume it is part of that resource and does not need added.
        if (script.resource)
            return;

        var insertIntoTopLevel = false;

        if (script.injected) {
            if (!this._extensionScriptsFolderTreeElement)
                this._extensionScriptsFolderTreeElement = new WebInspector.FolderTreeElement(WebInspector.UIString("Extension Scripts"));
            var parentFolderTreeElement = this._extensionScriptsFolderTreeElement;
        } else {
            if (WebInspector.debuggableType === WebInspector.DebuggableType.JavaScript)
                insertIntoTopLevel = true;
            else {
                if (!this._extraScriptsFolderTreeElement)
                    this._extraScriptsFolderTreeElement = new WebInspector.FolderTreeElement(WebInspector.UIString("Extra Scripts"));
                var parentFolderTreeElement = this._extraScriptsFolderTreeElement;
            }
        }

        var scriptTreeElement = new WebInspector.ScriptTreeElement(script);

        if (insertIntoTopLevel) {
            var index = insertionIndexForObjectInListSortedByFunction(scriptTreeElement, this._resourcesContentTreeOutline.children, this._compareTreeElements);
            this._resourcesContentTreeOutline.insertChild(scriptTreeElement, index);
        } else {
            if (!parentFolderTreeElement.parent) {
                var index = insertionIndexForObjectInListSortedByFunction(parentFolderTreeElement, this._resourcesContentTreeOutline.children, this._compareTreeElements);
                this._resourcesContentTreeOutline.insertChild(parentFolderTreeElement, index);
            }

            parentFolderTreeElement.appendChild(scriptTreeElement);
        }
    },

    _scriptsCleared: function(event)
    {
        if (this._extensionScriptsFolderTreeElement) {
            if (this._extensionScriptsFolderTreeElement.parent)
                this._extensionScriptsFolderTreeElement.parent.removeChild(this._extensionScriptsFolderTreeElement);
            this._extensionScriptsFolderTreeElement = null;
        }

        if (this._extraScriptsFolderTreeElement) {
            if (this._extraScriptsFolderTreeElement.parent)
                this._extraScriptsFolderTreeElement.parent.removeChild(this._extraScriptsFolderTreeElement);
            this._extraScriptsFolderTreeElement = null;
        }

        if (this._anonymousScriptsFolderTreeElement) {
            if (this._anonymousScriptsFolderTreeElement.parent)
                this._anonymousScriptsFolderTreeElement.parent.removeChild(this._anonymousScriptsFolderTreeElement);
            this._anonymousScriptsFolderTreeElement = null;
        }
    },

    _treeElementSelected: function(treeElement, selectedByUser)
    {
        if (treeElement instanceof WebInspector.FolderTreeElement || treeElement instanceof WebInspector.DatabaseHostTreeElement ||
            treeElement instanceof WebInspector.IndexedDatabaseHostTreeElement || treeElement instanceof WebInspector.IndexedDatabaseTreeElement)
            return;

        if (treeElement instanceof WebInspector.ResourceTreeElement || treeElement instanceof WebInspector.ScriptTreeElement ||
            treeElement instanceof WebInspector.StorageTreeElement || treeElement instanceof WebInspector.DatabaseTableTreeElement ||
            treeElement instanceof WebInspector.DatabaseTreeElement || treeElement instanceof WebInspector.ApplicationCacheFrameTreeElement ||
            treeElement instanceof WebInspector.ContentFlowTreeElement || treeElement instanceof WebInspector.IndexedDatabaseObjectStoreTreeElement ||
            treeElement instanceof WebInspector.IndexedDatabaseObjectStoreIndexTreeElement) {
            WebInspector.contentBrowser.showContentViewForRepresentedObject(treeElement.representedObject);
            return;
        }

        console.assert(treeElement instanceof WebInspector.SearchResultTreeElement);
        if (!(treeElement instanceof WebInspector.SearchResultTreeElement))
            return;

        if (treeElement.representedObject instanceof WebInspector.DOMSearchMatchObject)
            this.showMainFrameDOMTree(treeElement.representedObject.domNode, true);
        else if (treeElement.representedObject instanceof WebInspector.ResourceSearchMatchObject)
            this.showOriginalOrFormattedSourceCodeTextRange(treeElement.representedObject.sourceCodeTextRange);
    },

    _domNodeWasInspected: function(event)
    {
        this.showMainFrameDOMTree(event.data.node);
    },

    _domStorageObjectWasAdded: function(event)
    {
        var domStorage = event.data.domStorage;
        var storageElement = new WebInspector.DOMStorageTreeElement(domStorage);

        if (domStorage.isLocalStorage())
            this._localStorageRootTreeElement = this._addStorageChild(storageElement, this._localStorageRootTreeElement, WebInspector.UIString("Local Storage"));
        else
            this._sessionStorageRootTreeElement = this._addStorageChild(storageElement, this._sessionStorageRootTreeElement, WebInspector.UIString("Session Storage"));
    },

    _domStorageObjectWasInspected: function(event)
    {
        var domStorage = event.data.domStorage;
        var treeElement = this.treeElementForRepresentedObject(domStorage);
        treeElement.revealAndSelect(true);
    },

    _databaseWasAdded: function(event)
    {
        var database = event.data.database;

        console.assert(database instanceof WebInspector.DatabaseObject);

        if (!this._databaseHostTreeElementMap[database.host]) {
            this._databaseHostTreeElementMap[database.host] = new WebInspector.DatabaseHostTreeElement(database.host);
            this._databaseRootTreeElement = this._addStorageChild(this._databaseHostTreeElementMap[database.host], this._databaseRootTreeElement, WebInspector.UIString("Databases"));
        }

        var databaseElement = new WebInspector.DatabaseTreeElement(database);
        this._databaseHostTreeElementMap[database.host].appendChild(databaseElement);
    },

    _databaseWasInspected: function(event)
    {
        var database = event.data.database;
        var treeElement = this.treeElementForRepresentedObject(database);
        treeElement.revealAndSelect(true);
    },

    _indexedDatabaseWasAdded: function(event)
    {
        var indexedDatabase = event.data.indexedDatabase;

        console.assert(indexedDatabase instanceof WebInspector.IndexedDatabase);

        if (!this._indexedDatabaseHostTreeElementMap[indexedDatabase.host]) {
            this._indexedDatabaseHostTreeElementMap[indexedDatabase.host] = new WebInspector.IndexedDatabaseHostTreeElement(indexedDatabase.host);
            this._indexedDatabaseRootTreeElement = this._addStorageChild(this._indexedDatabaseHostTreeElementMap[indexedDatabase.host], this._indexedDatabaseRootTreeElement, WebInspector.UIString("Indexed Databases"));
        }

        var indexedDatabaseElement = new WebInspector.IndexedDatabaseTreeElement(indexedDatabase);
        this._indexedDatabaseHostTreeElementMap[indexedDatabase.host].appendChild(indexedDatabaseElement);
    },

    _cookieStorageObjectWasAdded: function(event)
    {
        console.assert(event.data.cookieStorage instanceof WebInspector.CookieStorageObject);

        var cookieElement = new WebInspector.CookieStorageTreeElement(event.data.cookieStorage);
        this._cookieStorageRootTreeElement = this._addStorageChild(cookieElement, this._cookieStorageRootTreeElement, WebInspector.UIString("Cookies"));
    },

    _frameManifestAdded: function(event)
    {
        var frameManifest = event.data.frameManifest;
        console.assert(frameManifest instanceof WebInspector.ApplicationCacheFrame);

        var manifest = frameManifest.manifest;
        var manifestURL = manifest.manifestURL;
        if (!this._applicationCacheURLTreeElementMap[manifestURL]) {
            this._applicationCacheURLTreeElementMap[manifestURL] = new WebInspector.ApplicationCacheManifestTreeElement(manifest);
            this._applicationCacheRootTreeElement = this._addStorageChild(this._applicationCacheURLTreeElementMap[manifestURL], this._applicationCacheRootTreeElement, WebInspector.UIString("Application Cache"));
        }

        var frameCacheElement = new WebInspector.ApplicationCacheFrameTreeElement(frameManifest);
        this._applicationCacheURLTreeElementMap[manifestURL].appendChild(frameCacheElement);
    },

    _frameManifestRemoved: function(event)
    {
         // FIXME: Implement this.
    },

    _compareTreeElements: function(a, b)
    {
        // Always sort the main frame element first.
        if (a instanceof WebInspector.FrameTreeElement)
            return -1;
        if (b instanceof WebInspector.FrameTreeElement)
            return 1;

        console.assert(a.mainTitle);
        console.assert(b.mainTitle);

        return (a.mainTitle || "").localeCompare(b.mainTitle || "");
    },

    _addStorageChild: function(childElement, parentElement, folderName)
    {
        if (!parentElement) {
            childElement.flattened = true;

            this._resourcesContentTreeOutline.insertChild(childElement, insertionIndexForObjectInListSortedByFunction(childElement, this._resourcesContentTreeOutline.children, this._compareTreeElements));

            return childElement;
        }

        if (parentElement instanceof WebInspector.StorageTreeElement) {
            console.assert(parentElement.flattened);

            var previousOnlyChild = parentElement;
            previousOnlyChild.flattened = false;
            this._resourcesContentTreeOutline.removeChild(previousOnlyChild);

            var folderElement = new WebInspector.FolderTreeElement(folderName, null, null);
            this._resourcesContentTreeOutline.insertChild(folderElement, insertionIndexForObjectInListSortedByFunction(folderElement, this._resourcesContentTreeOutline.children, this._compareTreeElements));

            folderElement.appendChild(previousOnlyChild);
            folderElement.insertChild(childElement, insertionIndexForObjectInListSortedByFunction(childElement, folderElement.children, this._compareTreeElements));

            return folderElement;
        }

        console.assert(parentElement instanceof WebInspector.FolderTreeElement);
        parentElement.insertChild(childElement, insertionIndexForObjectInListSortedByFunction(childElement, parentElement.children, this._compareTreeElements));

        return parentElement;
    },

    _storageCleared: function(event)
    {
        // Close all DOM and cookie storage content views since the main frame has navigated and all storages are cleared.
        WebInspector.contentBrowser.contentViewContainer.closeAllContentViewsOfPrototype(WebInspector.CookieStorageContentView);
        WebInspector.contentBrowser.contentViewContainer.closeAllContentViewsOfPrototype(WebInspector.DOMStorageContentView);
        WebInspector.contentBrowser.contentViewContainer.closeAllContentViewsOfPrototype(WebInspector.DatabaseTableContentView);
        WebInspector.contentBrowser.contentViewContainer.closeAllContentViewsOfPrototype(WebInspector.DatabaseContentView);
        WebInspector.contentBrowser.contentViewContainer.closeAllContentViewsOfPrototype(WebInspector.ApplicationCacheFrameContentView);

        if (this._localStorageRootTreeElement && this._localStorageRootTreeElement.parent)
            this._localStorageRootTreeElement.parent.removeChild(this._localStorageRootTreeElement);

        if (this._sessionStorageRootTreeElement && this._sessionStorageRootTreeElement.parent)
            this._sessionStorageRootTreeElement.parent.removeChild(this._sessionStorageRootTreeElement);

        if (this._databaseRootTreeElement && this._databaseRootTreeElement.parent)
            this._databaseRootTreeElement.parent.removeChild(this._databaseRootTreeElement);

        if (this._indexedDatabaseRootTreeElement && this._indexedDatabaseRootTreeElement.parent)
            this._indexedDatabaseRootTreeElement.parent.removeChild(this._indexedDatabaseRootTreeElement);

        if (this._cookieStorageRootTreeElement && this._cookieStorageRootTreeElement.parent)
            this._cookieStorageRootTreeElement.parent.removeChild(this._cookieStorageRootTreeElement);

        if (this._applicationCacheRootTreeElement && this._applicationCacheRootTreeElement.parent)
            this._applicationCacheRootTreeElement.parent.removeChild(this._applicationCacheRootTreeElement);

        this._localStorageRootTreeElement = null;
        this._sessionStorageRootTreeElement = null;
        this._databaseRootTreeElement = null;
        this._databaseHostTreeElementMap = {};
        this._indexedDatabaseRootTreeElement = null;
        this._indexedDatabaseHostTreeElementMap = {};
        this._cookieStorageRootTreeElement = null;
        this._applicationCacheRootTreeElement = null;
        this._applicationCacheURLTreeElementMap = {};
    }
};

WebInspector.ResourceSidebarPanel.prototype.__proto__ = WebInspector.NavigationSidebarPanel.prototype;
