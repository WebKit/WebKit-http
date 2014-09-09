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

WebInspector.FrameTreeElement = function(frame, representedObject)
{
    console.assert(frame instanceof WebInspector.Frame);

    WebInspector.ResourceTreeElement.call(this, frame.mainResource, representedObject || frame);

    this._frame = frame;
    this._newChildQueue = [];

    this._updateExpandedSetting();

    frame.addEventListener(WebInspector.Frame.Event.MainResourceDidChange, this._mainResourceDidChange, this);
    frame.addEventListener(WebInspector.Frame.Event.ResourceWasAdded, this._resourceWasAdded, this);
    frame.addEventListener(WebInspector.Frame.Event.ResourceWasRemoved, this._resourceWasRemoved, this);
    frame.addEventListener(WebInspector.Frame.Event.ChildFrameWasAdded, this._childFrameWasAdded, this);
    frame.addEventListener(WebInspector.Frame.Event.ChildFrameWasRemoved, this._childFrameWasRemoved, this);

    frame.domTree.addEventListener(WebInspector.DOMTree.Event.ContentFlowWasAdded, this._childContentFlowWasAdded, this);
    frame.domTree.addEventListener(WebInspector.DOMTree.Event.ContentFlowWasRemoved, this._childContentFlowWasRemoved, this);
    frame.domTree.addEventListener(WebInspector.DOMTree.Event.RootDOMNodeInvalidated, this._rootDOMNodeInvalidated, this);

    if (this._frame.isMainFrame()) {
        this._downloadingPage = false;
        WebInspector.notifications.addEventListener(WebInspector.Notification.PageArchiveStarted, this._pageArchiveStarted, this);
        WebInspector.notifications.addEventListener(WebInspector.Notification.PageArchiveEnded, this._pageArchiveEnded, this);
    }

    this._updateParentStatus();
    this.shouldRefreshChildren = true;
};

WebInspector.FrameTreeElement.MediumChildCountThreshold = 5;
WebInspector.FrameTreeElement.LargeChildCountThreshold = 15;
WebInspector.FrameTreeElement.NumberOfMediumCategoriesThreshold = 2;
WebInspector.FrameTreeElement.NewChildQueueUpdateInterval = 500;

WebInspector.FrameTreeElement.prototype = {
    constructor: WebInspector.FrameTreeElement,

    // Public

    get frame()
    {
        return this._frame;
    },

    descendantResourceTreeElementTypeDidChange: function(resourceTreeElement, oldType)
    {
        // Called by descendant ResourceTreeElements.

        // Add the tree element again, which will move it to the new location
        // based on sorting and possible folder changes.
        this._addTreeElement(resourceTreeElement);
    },

    descendantResourceTreeElementMainTitleDidChange: function(resourceTreeElement, oldMainTitle)
    {
        // Called by descendant ResourceTreeElements.

        // Add the tree element again, which will move it to the new location
        // based on sorting and possible folder changes.
        this._addTreeElement(resourceTreeElement);
    },

    // Overrides from SourceCodeTreeElement.

    updateSourceMapResources: function()
    {
        // Frames handle their own SourceMapResources.

        if (!this.treeOutline || !this.treeOutline.includeSourceMapResourceChildren)
            return;

        if (!this._frame)
            return;

        this._updateParentStatus();

        if (this.resource && this.resource.sourceMaps.length)
            this.shouldRefreshChildren = true;
    },

    onattach: function()
    {
        // Frames handle their own SourceMapResources.

        WebInspector.GeneralTreeElement.prototype.onattach.call(this);
    },

    // Called from ResourceTreeElement.

    updateStatusForMainFrame: function()
    {
        function loadedImages()
        {
            if (!this._reloadButton || !this._downloadButton)
                return;

            var fragment = document.createDocumentFragment("div");
            fragment.appendChild(this._downloadButton.element);
            fragment.appendChild(this._reloadButton.element);
            this.status = fragment;

            delete this._loadingMainFrameButtons;
        }

        if (this._reloadButton && this._downloadButton) {
            loadedImages.call(this);
            return;
        }

        if (!this._loadingMainFrameButtons) {
            this._loadingMainFrameButtons = true;

            var tooltip = WebInspector.UIString("Reload page (%s)\nReload ignoring cache (%s)").format(WebInspector._reloadPageKeyboardShortcut.displayName, WebInspector._reloadPageIgnoringCacheKeyboardShortcut.displayName);
            wrappedSVGDocument("Images/Reload.svg", null, tooltip, function(element) {
                this._reloadButton = new WebInspector.TreeElementStatusButton(element);
                this._reloadButton.addEventListener(WebInspector.TreeElementStatusButton.Event.Clicked, this._reloadPageClicked, this);
                loadedImages.call(this);
            }.bind(this));

            wrappedSVGDocument("Images/DownloadArrow.svg", null, WebInspector.UIString("Download Web Archive"), function(element) {
                this._downloadButton = new WebInspector.TreeElementStatusButton(element);
                this._downloadButton.addEventListener(WebInspector.TreeElementStatusButton.Event.Clicked, this._downloadButtonClicked, this);
                this._updateDownloadButton();
                loadedImages.call(this);
            }.bind(this));
        }
    },

    // Overrides from TreeElement (Private).

    onpopulate: function()
    {
        if (this.children.length && !this.shouldRefreshChildren)
            return;

        this.shouldRefreshChildren = false;

        this.removeChildren();
        this._clearNewChildQueue();

        if (this._shouldGroupIntoFolders() && !this._groupedIntoFolders)
            this._groupedIntoFolders = true;

        for (var i = 0; i < this._frame.childFrames.length; ++i)
            this._addTreeElementForRepresentedObject(this._frame.childFrames[i]);

        for (var i = 0; i < this._frame.resources.length; ++i)
            this._addTreeElementForRepresentedObject(this._frame.resources[i]);

        var sourceMaps = this.resource && this.resource.sourceMaps;
        for (var i = 0; i < sourceMaps.length; ++i) {
            var sourceMap = sourceMaps[i];
            for (var j = 0; j < sourceMap.resources.length; ++j)
            this._addTreeElementForRepresentedObject(sourceMap.resources[j]);
        }

        var flowMap = this._frame.domTree.flowMap;
        for (var flowKey in flowMap)
            this._addTreeElementForRepresentedObject(flowMap[flowKey]);
    },

    onexpand: function()
    {
        this._expandedSetting.value = true;
        this._frame.domTree.requestContentFlowList();
    },

    oncollapse: function()
    {
        // Only store the setting if we have children, since setting hasChildren to false will cause a collapse,
        // and we only care about user triggered collapses.
        if (this.hasChildren)
            this._expandedSetting.value = false;
    },

    removeChildren: function()
    {
        TreeElement.prototype.removeChildren.call(this);

        if (this._framesFolderTreeElement)
            this._framesFolderTreeElement.removeChildren();

        for (var type in this._resourceFoldersTypeMap)
            this._resourceFoldersTypeMap[type].removeChildren();

        delete this._resourceFoldersTypeMap;
        delete this._framesFolderTreeElement;
    },

    // Private

    _updateExpandedSetting: function()
    {
        this._expandedSetting = new WebInspector.Setting("frame-expanded-" + this._frame.url.hash, this._frame.isMainFrame() ? true : false);
        if (this._expandedSetting.value)
            this.expand();
        else
            this.collapse();
    },

    _updateParentStatus: function()
    {
        this.hasChildren = (this._frame.resources.length || this._frame.childFrames.length || (this.resource && this.resource.sourceMaps.length));
        if (!this.hasChildren)
            this.removeChildren();
    },

    _mainResourceDidChange: function(event)
    {
        this._updateResource(this._frame.mainResource);
        this._updateParentStatus();

        this._groupedIntoFolders = false;

        this._clearNewChildQueue();

        this.removeChildren();

        // Change the expanded setting since the frame URL has changed. Do this before setting shouldRefreshChildren, since
        // shouldRefreshChildren will call onpopulate if expanded is true.
        this._updateExpandedSetting();

        if (this._frame.isMainFrame())
            this._updateDownloadButton();

        this.shouldRefreshChildren = true;
    },

    _resourceWasAdded: function(event)
    {
        this._addRepresentedObjectToNewChildQueue(event.data.resource);
    },

    _resourceWasRemoved: function(event)
    {
        this._removeChildForRepresentedObject(event.data.resource);
    },

    _childFrameWasAdded: function(event)
    {
        this._addRepresentedObjectToNewChildQueue(event.data.childFrame);
    },

    _childFrameWasRemoved: function(event)
    {
        this._removeChildForRepresentedObject(event.data.childFrame);
    },

    _childContentFlowWasAdded: function(event)
    {
        this._addRepresentedObjectToNewChildQueue(event.data.flow);
    },

    _childContentFlowWasRemoved: function(event)
    {
        this._removeChildForRepresentedObject(event.data.flow);
    },

    _rootDOMNodeInvalidated: function() {
        if (this.expanded)
            this._frame.domTree.requestContentFlowList();
    },

    _addRepresentedObjectToNewChildQueue: function(representedObject)
    {
        // This queue reduces flashing as resources load and change folders when their type becomes known.

        this._newChildQueue.push(representedObject);
        if (!this._newChildQueueTimeoutIdentifier)
            this._newChildQueueTimeoutIdentifier = setTimeout(this._populateFromNewChildQueue.bind(this), WebInspector.FrameTreeElement.NewChildQueueUpdateInterval);
    },

    _removeRepresentedObjectFromNewChildQueue: function(representedObject)
    {
        this._newChildQueue.remove(representedObject);
    },

    _populateFromNewChildQueue: function()
    {
        for (var i = 0; i < this._newChildQueue.length; ++i)
            this._addChildForRepresentedObject(this._newChildQueue[i]);

        this._newChildQueue = [];
        this._newChildQueueTimeoutIdentifier = null;
    },

    _clearNewChildQueue: function()
    {
        this._newChildQueue = [];
        if (this._newChildQueueTimeoutIdentifier) {
            clearTimeout(this._newChildQueueTimeoutIdentifier);
            this._newChildQueueTimeoutIdentifier = null;
        }
    },

    _addChildForRepresentedObject: function(representedObject)
    {
        console.assert(representedObject instanceof WebInspector.Resource || representedObject instanceof WebInspector.Frame || representedObject instanceof WebInspector.ContentFlow);
        if (!(representedObject instanceof WebInspector.Resource || representedObject instanceof WebInspector.Frame || representedObject instanceof WebInspector.ContentFlow))
            return;

        this._updateParentStatus();

        if (!this.treeOutline) {
            // Just mark as needing to update to avoid doing work that might not be needed.
            this.shouldRefreshChildren = true;
            return;
        }

        if (this._shouldGroupIntoFolders() && !this._groupedIntoFolders) {
            // Mark as needing a refresh to rebuild the tree into folders.
            this._groupedIntoFolders = true;
            this.shouldRefreshChildren = true;
            return;
        }

        this._addTreeElementForRepresentedObject(representedObject);
    },

    _removeChildForRepresentedObject: function(representedObject)
    {
        console.assert(representedObject instanceof WebInspector.Resource || representedObject instanceof WebInspector.Frame || representedObject instanceof WebInspector.ContentFlow);
        if (!(representedObject instanceof WebInspector.Resource || representedObject instanceof WebInspector.Frame || representedObject instanceof WebInspector.ContentFlow))
            return;

        this._removeRepresentedObjectFromNewChildQueue(representedObject);

        this._updateParentStatus();

        if (!this.treeOutline) {
            // Just mark as needing to update to avoid doing work that might not be needed.
            this.shouldRefreshChildren = true;
            return;
        }

        // Find the tree element for the frame by using getCachedTreeElement
        // to only get the item if it has been created already.
        var childTreeElement = this.treeOutline.getCachedTreeElement(representedObject);
        if (!childTreeElement || !childTreeElement.parent)
            return;

        this._removeTreeElement(childTreeElement);
    },

    _addTreeElementForRepresentedObject: function(representedObject)
    {
        var childTreeElement = this.treeOutline.getCachedTreeElement(representedObject);
        if (!childTreeElement) {
            if (representedObject instanceof WebInspector.SourceMapResource)
                childTreeElement = new WebInspector.SourceMapResourceTreeElement(representedObject);
            else if (representedObject instanceof WebInspector.Resource)
                childTreeElement = new WebInspector.ResourceTreeElement(representedObject);
            else if (representedObject instanceof WebInspector.Frame)
                childTreeElement = new WebInspector.FrameTreeElement(representedObject);
            else if (representedObject instanceof WebInspector.ContentFlow)
                childTreeElement = new WebInspector.ContentFlowTreeElement(representedObject);
        }

        this._addTreeElement(childTreeElement);
    },

    _addTreeElement: function(childTreeElement)
    {
        console.assert(childTreeElement);
        if (!childTreeElement)
            return;

        var wasSelected = childTreeElement.selected;

        this._removeTreeElement(childTreeElement, true, true);

        var parentTreeElement = this._parentTreeElementForRepresentedObject(childTreeElement.representedObject);
        if (parentTreeElement !== this && !parentTreeElement.parent)
            this._insertFolderTreeElement(parentTreeElement);

        this._insertResourceTreeElement(parentTreeElement, childTreeElement);

        if (wasSelected)
            childTreeElement.revealAndSelect(true, false, true, true);
    },

    _compareTreeElementsByMainTitle: function(a, b)
    {
        return a.mainTitle.localeCompare(b.mainTitle);
    },

    _insertFolderTreeElement: function(folderTreeElement)
    {
        console.assert(this._groupedIntoFolders);
        console.assert(!folderTreeElement.parent);
        this.insertChild(folderTreeElement, insertionIndexForObjectInListSortedByFunction(folderTreeElement, this.children, this._compareTreeElementsByMainTitle));
    },

    _compareResourceTreeElements: function(a, b)
    {
        if (a === b)
            return 0;

        var aIsResource = a instanceof WebInspector.ResourceTreeElement;
        var bIsResource = b instanceof WebInspector.ResourceTreeElement;

        if (aIsResource && bIsResource)
            return WebInspector.ResourceTreeElement.compareResourceTreeElements(a, b);

        if (!aIsResource && !bIsResource) {
            // When both components are not resources then just compare the titles.
            return a.mainTitle.localeCompare(b.mainTitle);
        }

        // Non-resources should appear before the resources.
        // FIXME: There should be a better way to group the elements by their type.
        return aIsResource ? 1 : -1;
    },

    _insertResourceTreeElement: function(parentTreeElement, childTreeElement)
    {
        console.assert(!childTreeElement.parent);
        parentTreeElement.insertChild(childTreeElement, insertionIndexForObjectInListSortedByFunction(childTreeElement, parentTreeElement.children, this._compareResourceTreeElements));
    },

    _removeTreeElement: function(childTreeElement, suppressOnDeselect, suppressSelectSibling)
    {
        var oldParent = childTreeElement.parent;
        if (!oldParent)
            return;

        oldParent.removeChild(childTreeElement, suppressOnDeselect, suppressSelectSibling);

        if (oldParent === this)
            return;

        console.assert(oldParent instanceof WebInspector.FolderTreeElement);
        if (!(oldParent instanceof WebInspector.FolderTreeElement))
            return;

        // Remove the old parent folder if it is now empty.
        if (!oldParent.children.length)
            oldParent.parent.removeChild(oldParent);
    },

    _folderNameForResourceType: function(type)
    {
        return WebInspector.Resource.Type.displayName(type, true);
    },

    _parentTreeElementForRepresentedObject: function(representedObject)
    {
        if (!this._groupedIntoFolders)
            return this;

        function createFolderTreeElement(type, displayName)
        {
            var folderTreeElement = new WebInspector.FolderTreeElement(displayName);
            folderTreeElement._expandedSetting = new WebInspector.Setting(type + "-folder-expanded-" + this._frame.url.hash, false);
            if (folderTreeElement._expandedSetting.value)
                folderTreeElement.expand();
            folderTreeElement.onexpand = this._folderTreeElementExpandedStateChange.bind(this);
            folderTreeElement.oncollapse = this._folderTreeElementExpandedStateChange.bind(this);
            return folderTreeElement;
        }

        if (representedObject instanceof WebInspector.Frame) {
            if (!this._framesFolderTreeElement)
                this._framesFolderTreeElement = createFolderTreeElement.call(this, "frames", WebInspector.UIString("Frames"));
            return this._framesFolderTreeElement;
        }

        if (representedObject instanceof WebInspector.ContentFlow) {
            if (!this._flowsFolderTreeElement)
                this._flowsFolderTreeElement = createFolderTreeElement.call(this, "flows", WebInspector.UIString("Flows"));
            return this._flowsFolderTreeElement;
        }

        if (representedObject instanceof WebInspector.Resource) {
            var folderName = this._folderNameForResourceType(representedObject.type);
            if (!folderName)
                return this;

            if (!this._resourceFoldersTypeMap)
                this._resourceFoldersTypeMap = {};
            if (!this._resourceFoldersTypeMap[representedObject.type])
                this._resourceFoldersTypeMap[representedObject.type] = createFolderTreeElement.call(this, representedObject.type, folderName);
            return this._resourceFoldersTypeMap[representedObject.type];
        }

        console.error("Unknown representedObject: ", representedObject);
        return this;
    },

    _folderTreeElementExpandedStateChange: function(folderTreeElement)
    {
        console.assert(folderTreeElement._expandedSetting);
        folderTreeElement._expandedSetting.value = folderTreeElement.expanded;
    },

    _shouldGroupIntoFolders: function()
    {
        // Already grouped into folders, keep it that way.
        if (this._groupedIntoFolders)
            return true;

        // Resources and Frames are grouped into folders if one of two thresholds are met:
        // 1) Once the number of medium categories passes NumberOfMediumCategoriesThreshold.
        // 2) When there is a category that passes LargeChildCountThreshold and there are
        //    any resources in another category.

        // Folders are avoided when there is only one category or most categories are small.

        var numberOfSmallCategories = 0;
        var numberOfMediumCategories = 0;
        var foundLargeCategory = false;
        var frame = this._frame;

        function pushResourceType(type) {
            // There are some other properties on WebInspector.Resource.Type that we need to skip, like private data and functions
            if (type.charAt(0) === "_")
                return false;

            // Only care about the values that are strings, not functions, etc.
            var typeValue = WebInspector.Resource.Type[type];
            if (typeof typeValue !== "string")
                return false;

            return pushCategory(frame.resourcesWithType(typeValue).length);
        }

        function pushCategory(resourceCount)
        {
            if (!resourceCount)
                return false;

            // If this type has any resources and there is a known large category, make folders.
            if (foundLargeCategory)
                return true;

            // If there are lots of this resource type, then count it as a large category.
            if (resourceCount >= WebInspector.FrameTreeElement.LargeChildCountThreshold) {
                // If we already have other resources in other small or medium categories, make folders.
                if (numberOfSmallCategories || numberOfMediumCategories)
                    return true;

                foundLargeCategory = true;
                return false;
            }

            // Check if this is a medium category.
            if (resourceCount >= WebInspector.FrameTreeElement.MediumChildCountThreshold) {
                // If this is the medium category that puts us over the maximum allowed, make folders.
                return ++numberOfMediumCategories >= WebInspector.FrameTreeElement.NumberOfMediumCategoriesThreshold;
            }

            // This is a small category.
            ++numberOfSmallCategories;
            return false;
        }

        // Iterate over all the available resource types.
        return pushCategory(frame.childFrames.length) || pushCategory(frame.domTree.flowsCount) || Object.keys(WebInspector.Resource.Type).some(pushResourceType);
    },

    _reloadPageClicked: function(event)
    {
        // Ignore cache when the shift key is pressed.
        PageAgent.reload(event.data.shiftKey);
    },

    _downloadButtonClicked: function(event)
    {
        WebInspector.archiveMainFrame();
    },

    _updateDownloadButton: function()
    {
        console.assert(this._frame.isMainFrame());
        if (!this._downloadButton)
            return;

        if (!PageAgent.archive) {
            this._downloadButton.hidden = true;
            return;
        }

        if (this._downloadingPage) {
            this._downloadButton.enabled = false;
            return;
        }

        this._downloadButton.enabled = WebInspector.canArchiveMainFrame();
    },

    _pageArchiveStarted: function(event)
    {
        this._downloadingPage = true;
        this._updateDownloadButton();
    },

    _pageArchiveEnded: function(event)
    {
        this._downloadingPage = false;
        this._updateDownloadButton();
    }
};

WebInspector.FrameTreeElement.prototype.__proto__ = WebInspector.ResourceTreeElement.prototype;
