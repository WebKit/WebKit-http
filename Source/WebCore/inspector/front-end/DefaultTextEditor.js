/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @constructor
 * @extends {WebInspector.View}
 * @implements {WebInspector.TextEditor}
 * @param {?string} url
 * @param {WebInspector.TextEditorDelegate} delegate
 */
WebInspector.DefaultTextEditor = function(url, delegate)
{
    WebInspector.View.call(this);
    this._delegate = delegate;
    this._url = url;

    this.registerRequiredCSS("textEditor.css");

    this.element.className = "text-editor monospace";

    // Prevent middle-click pasting in the editor unless it is explicitly enabled for certain component.
    this.element.addEventListener("mouseup", preventDefaultOnMouseUp.bind(this), false);
    function preventDefaultOnMouseUp(event)
    {
        if (event.button === 1)
            event.consume(true);
    }

    this._textModel = new WebInspector.TextEditorModel();
    this._textModel.addEventListener(WebInspector.TextEditorModel.Events.TextChanged, this._textChanged, this);

    var syncScrollListener = this._syncScroll.bind(this);
    var syncDecorationsForLineListener = this._syncDecorationsForLine.bind(this);
    var syncLineHeightListener = this._syncLineHeight.bind(this);
    this._mainPanel = new WebInspector.TextEditorMainPanel(this._delegate, this._textModel, url, syncScrollListener, syncDecorationsForLineListener);
    this._gutterPanel = new WebInspector.TextEditorGutterPanel(this._textModel, syncDecorationsForLineListener, syncLineHeightListener);

    this._mainPanel.element.addEventListener("scroll", this._handleScrollChanged.bind(this), false);
    this._mainPanel._container.addEventListener("focus", this._handleFocused.bind(this), false);

    this._gutterPanel.element.addEventListener("mousedown", this._onMouseDown.bind(this), true);

    // Explicitly enable middle-click pasting in the editor main panel.
    this._mainPanel.element.addEventListener("mouseup", consumeMouseUp.bind(this), false);
    function consumeMouseUp(event)
    {
        if (event.button === 1)
            event.consume(false);
    }

    this.element.appendChild(this._mainPanel.element);
    this.element.appendChild(this._gutterPanel.element);

    // Forward mouse wheel events from the unscrollable gutter to the main panel.
    function forwardWheelEvent(event)
    {
        var clone = document.createEvent("WheelEvent");
        clone.initWebKitWheelEvent(event.wheelDeltaX, event.wheelDeltaY,
                                   event.view,
                                   event.screenX, event.screenY,
                                   event.clientX, event.clientY,
                                   event.ctrlKey, event.altKey, event.shiftKey, event.metaKey);
        this._mainPanel.element.dispatchEvent(clone);
    }
    this._gutterPanel.element.addEventListener("mousewheel", forwardWheelEvent.bind(this), false);

    this.element.addEventListener("keydown", this._handleKeyDown.bind(this), false);
    this.element.addEventListener("cut", this._handleCut.bind(this), false);
    this.element.addEventListener("textInput", this._handleTextInput.bind(this), false);
    this.element.addEventListener("contextmenu", this._contextMenu.bind(this), true);

    this._registerShortcuts();
}

/**
 * @constructor
 * @param {WebInspector.TextRange} range
 * @param {string} text
 */
WebInspector.DefaultTextEditor.EditInfo = function(range, text)
{
    this.range = range;
    this.text = text;
}

WebInspector.DefaultTextEditor.prototype = {
    /**
     * @param {string} mimeType
     */
    set mimeType(mimeType)
    {
        this._mainPanel.mimeType = mimeType;
    },

    /**
     * @param {boolean} readOnly
     */
    setReadOnly: function(readOnly)
    {
        if (this._mainPanel.readOnly() === readOnly)
            return;
        this._mainPanel.setReadOnly(readOnly, this.isShowing());
        WebInspector.markBeingEdited(this.element, !readOnly);
    },

    /**
     * @return {boolean}
     */
    readOnly: function()
    {
        return this._mainPanel.readOnly();
    },

    /**
     * @return {Element}
     */
    defaultFocusedElement: function()
    {
        return this._mainPanel.defaultFocusedElement();
    },

    /**
     * @param {number} lineNumber
     */
    revealLine: function(lineNumber)
    {
        this._mainPanel.revealLine(lineNumber);
    },

    _onMouseDown: function(event)
    {
        var target = event.target.enclosingNodeOrSelfWithClass("webkit-line-number");
        if (!target)
            return;
        this.dispatchEventToListeners(WebInspector.TextEditor.Events.GutterClick, { lineNumber: target.lineNumber, event: event });
    },

    /**
     * @param {number} lineNumber
     * @param {boolean} disabled
     * @param {boolean} conditional
     */
    addBreakpoint: function(lineNumber, disabled, conditional)
    {
        this.beginUpdates();
        this._gutterPanel.addDecoration(lineNumber, "webkit-breakpoint");
        if (disabled)
            this._gutterPanel.addDecoration(lineNumber, "webkit-breakpoint-disabled");
        else
            this._gutterPanel.removeDecoration(lineNumber, "webkit-breakpoint-disabled");
        if (conditional)
            this._gutterPanel.addDecoration(lineNumber, "webkit-breakpoint-conditional");
        else
            this._gutterPanel.removeDecoration(lineNumber, "webkit-breakpoint-conditional");
        this.endUpdates();
    },

    /**
     * @param {number} lineNumber
     */
    removeBreakpoint: function(lineNumber)
    {
        this.beginUpdates();
        this._gutterPanel.removeDecoration(lineNumber, "webkit-breakpoint");
        this._gutterPanel.removeDecoration(lineNumber, "webkit-breakpoint-disabled");
        this._gutterPanel.removeDecoration(lineNumber, "webkit-breakpoint-conditional");
        this.endUpdates();
    },

    /**
     * @param {number} lineNumber
     */
    setExecutionLine: function(lineNumber)
    {
        this._executionLineNumber = lineNumber;
        this._mainPanel.addDecoration(lineNumber, "webkit-execution-line");
        this._gutterPanel.addDecoration(lineNumber, "webkit-execution-line");
    },

    clearExecutionLine: function()
    {
        if (typeof this._executionLineNumber === "number") {
            this._mainPanel.removeDecoration(this._executionLineNumber, "webkit-execution-line");
            this._gutterPanel.removeDecoration(this._executionLineNumber, "webkit-execution-line");
        }
        delete this._executionLineNumber;
    },

    /**
     * @param {number} lineNumber
     * @param {Element} element
     */
    addDecoration: function(lineNumber, element)
    {
        this._mainPanel.addDecoration(lineNumber, element);
        this._gutterPanel.addDecoration(lineNumber, element);
        this._syncDecorationsForLine(lineNumber);
    },

    /**
     * @param {number} lineNumber
     * @param {Element} element
     */
    removeDecoration: function(lineNumber, element)
    {
        this._mainPanel.removeDecoration(lineNumber, element);
        this._gutterPanel.removeDecoration(lineNumber, element);
        this._syncDecorationsForLine(lineNumber);
    },

    /**
     * @param {WebInspector.TextRange} range
     */
    markAndRevealRange: function(range)
    {
        if (range)
            this.setSelection(range);
        this._mainPanel.markAndRevealRange(range);
    },

    /**
     * @param {number} lineNumber
     */
    highlightLine: function(lineNumber)
    {
        if (typeof lineNumber !== "number" || lineNumber < 0)
            return;

        lineNumber = Math.min(lineNumber, this._textModel.linesCount - 1);
        this._mainPanel.highlightLine(lineNumber);
    },

    clearLineHighlight: function()
    {
        this._mainPanel.clearLineHighlight();
    },

    _freeCachedElements: function()
    {
        this._mainPanel._freeCachedElements();
        this._gutterPanel._freeCachedElements();
    },

    /**
     * @return {Array.<Element>}
     */
    elementsToRestoreScrollPositionsFor: function()
    {
        return [this._mainPanel.element];
    },

    /**
     * @param {WebInspector.TextEditor} textEditor
     */
    inheritScrollPositions: function(textEditor)
    {
        this._mainPanel.element._scrollTop = textEditor._mainPanel.element.scrollTop;
        this._mainPanel.element._scrollLeft = textEditor._mainPanel.element.scrollLeft;
    },

    beginUpdates: function()
    {
        this._mainPanel.beginUpdates();
        this._gutterPanel.beginUpdates();
    },

    endUpdates: function()
    {
        this._mainPanel.endUpdates();
        this._gutterPanel.endUpdates();
        this._updatePanelOffsets();
    },

    onResize: function()
    {
        this._mainPanel.resize();
        this._gutterPanel.resize();
        this._updatePanelOffsets();
    },

    _textChanged: function(event)
    {
        this._mainPanel.textChanged(event.data.oldRange, event.data.newRange);
        this._gutterPanel.textChanged(event.data.oldRange, event.data.newRange);
        this._updatePanelOffsets();
        if (event.data.editRange)
            this._delegate.onTextChanged(event.data.oldRange, event.data.newRange);
    },

    /**
     * @param {WebInspector.TextRange} range
     * @param {string} text
     * @return {WebInspector.TextRange}
     */
    editRange: function(range, text)
    {
        return this._textModel.editRange(range, text);
    },

    _updatePanelOffsets: function()
    {
        var lineNumbersWidth = this._gutterPanel.element.offsetWidth;
        if (lineNumbersWidth)
            this._mainPanel.element.style.setProperty("left", (lineNumbersWidth + 2) + "px");
        else
            this._mainPanel.element.style.removeProperty("left"); // Use default value set in CSS.
    },

    _syncScroll: function()
    {
        var mainElement = this._mainPanel.element;
        var gutterElement = this._gutterPanel.element;
        // Handle horizontal scroll bar at the bottom of the main panel.
        this._gutterPanel.syncClientHeight(mainElement.clientHeight);
        gutterElement.scrollTop = mainElement.scrollTop;
    },

    /**
     * @param {number} lineNumber
     */
    _syncDecorationsForLine: function(lineNumber)
    {
        if (lineNumber >= this._textModel.linesCount)
            return;

        var mainChunk = this._mainPanel.chunkForLine(lineNumber);
        if (mainChunk.linesCount === 1 && mainChunk.isDecorated()) {
            var gutterChunk = this._gutterPanel.makeLineAChunk(lineNumber);
            var height = mainChunk.height;
            if (height)
                gutterChunk.element.style.setProperty("height", height + "px");
            else
                gutterChunk.element.style.removeProperty("height");
        } else {
            var gutterChunk = this._gutterPanel.chunkForLine(lineNumber);
            if (gutterChunk.linesCount === 1)
                gutterChunk.element.style.removeProperty("height");
        }
    },

    /**
     * @param {Element} gutterRow
     */
    _syncLineHeight: function(gutterRow)
    {
        if (this._lineHeightSynced)
            return;
        if (gutterRow && gutterRow.offsetHeight) {
            // Force equal line heights for the child panels.
            this.element.style.setProperty("line-height", gutterRow.offsetHeight + "px");
            this._lineHeightSynced = true;
        }
    },

    _registerShortcuts: function()
    {
        var keys = WebInspector.KeyboardShortcut.Keys;
        var modifiers = WebInspector.KeyboardShortcut.Modifiers;

        this._shortcuts = {};

        var handleEnterKey = this._mainPanel.handleEnterKey.bind(this._mainPanel);
        this._shortcuts[WebInspector.KeyboardShortcut.makeKey(keys.Enter.code, WebInspector.KeyboardShortcut.Modifiers.None)] = handleEnterKey;

        this._shortcuts[WebInspector.KeyboardShortcut.makeKey("z", modifiers.CtrlOrMeta)] = this._mainPanel.handleUndoRedo.bind(this._mainPanel, false);
        this._shortcuts[WebInspector.KeyboardShortcut.SelectAll] = this._handleSelectAll.bind(this);

        var handleRedo = this._mainPanel.handleUndoRedo.bind(this._mainPanel, true);
        this._shortcuts[WebInspector.KeyboardShortcut.makeKey("z", modifiers.Shift | modifiers.CtrlOrMeta)] = handleRedo;
        if (!WebInspector.isMac())
            this._shortcuts[WebInspector.KeyboardShortcut.makeKey("y", modifiers.CtrlOrMeta)] = handleRedo;

        var handleTabKey = this._mainPanel.handleTabKeyPress.bind(this._mainPanel, false);
        var handleShiftTabKey = this._mainPanel.handleTabKeyPress.bind(this._mainPanel, true);
        this._shortcuts[WebInspector.KeyboardShortcut.makeKey(keys.Tab.code)] = handleTabKey;
        this._shortcuts[WebInspector.KeyboardShortcut.makeKey(keys.Tab.code, modifiers.Shift)] = handleShiftTabKey;
    },

    _handleSelectAll: function()
    {
        this.setSelection(this._textModel.range());
        return true;
    },

    _handleTextInput: function(e)
    {
        this._mainPanel._textInputData = e.data;
    },

    _handleKeyDown: function(e)
    {
        // If the event was not triggered from the entire editor, then
        // ignore it. https://bugs.webkit.org/show_bug.cgi?id=102906
        if (e.target.enclosingNodeOrSelfWithClass("webkit-line-decorations"))
            return;

        var shortcutKey = WebInspector.KeyboardShortcut.makeKeyFromEvent(e);

        var handler = this._shortcuts[shortcutKey];
        if (handler && handler()) {
            e.consume(true);
            return;
        }
        this._mainPanel._keyDownCode = e.keyCode;
    },

    _handleCut: function(e)
    {
        this._mainPanel._keyDownCode = WebInspector.KeyboardShortcut.Keys.Delete.code;
    },

    _contextMenu: function(event)
    {
        var anchor = event.target.enclosingNodeOrSelfWithNodeName("a");
        if (anchor)
            return;
        var contextMenu = new WebInspector.ContextMenu(event);
        var target = event.target.enclosingNodeOrSelfWithClass("webkit-line-number");
        if (target)
            this._delegate.populateLineGutterContextMenu(contextMenu, target.lineNumber);
        else {
            target = this._mainPanel._enclosingLineRowOrSelf(event.target);
            this._delegate.populateTextAreaContextMenu(contextMenu, target && target.lineNumber);
        }
        contextMenu.show();
    },

    _handleScrollChanged: function(event)
    {
        var visibleFrom = this._mainPanel._scrollTop();
        var firstVisibleLineNumber = this._mainPanel._findFirstVisibleLineNumber(visibleFrom);
        this._delegate.scrollChanged(firstVisibleLineNumber);
    },

    /**
     * @param {number} lineNumber
     */
    scrollToLine: function(lineNumber)
    {
        this._mainPanel.scrollToLine(lineNumber);
    },

    /**
     * @return {WebInspector.TextRange}
     */
    selection: function(textRange)
    {
        return this._mainPanel._getSelection();
    },

    /**
     * @return {WebInspector.TextRange?}
     */
    lastSelection: function()
    {
        return this._mainPanel._lastSelection;
    },

    /**
     * @param {WebInspector.TextRange} textRange
     */
    setSelection: function(textRange)
    {
        this._mainPanel._lastSelection = textRange;
        if (this.element.isAncestor(document.activeElement))
            this._mainPanel._restoreSelection(textRange);
    },

    /**
     * @param {string} text 
     */
    setText: function(text)
    {
        this._textModel.setText(text);
    },

    /**
     * @return {string}
     */
    text: function()
    {
        return this._textModel.text();
    },

    /**
     * @return {WebInspector.TextRange}
     */
    range: function()
    {
        return this._textModel.range();
    },

    /**
     * @param {number} lineNumber
     * @return {string}
     */
    line: function(lineNumber)
    {
        return this._textModel.line(lineNumber);
    },

    /**
     * @return {number}
     */
    get linesCount()
    {
        return this._textModel.linesCount;
    },

    /**
     * @param {number} line
     * @param {string} name  
     * @param {Object?} value  
     */
    setAttribute: function(line, name, value)
    {
        this._textModel.setAttribute(line, name, value);
    },

    /**
     * @param {number} line
     * @param {string} name  
     * @return {Object|null} value  
     */
    getAttribute: function(line, name)
    {
        return this._textModel.getAttribute(line, name);
    },

    /**
     * @param {number} line
     * @param {string} name
     */
    removeAttribute: function(line, name)
    {
        this._textModel.removeAttribute(line, name);
    },

    wasShown: function()
    {
        if (!this.readOnly())
            WebInspector.markBeingEdited(this.element, true);

        this._boundSelectionChangeListener = this._mainPanel._handleSelectionChange.bind(this._mainPanel);
        document.addEventListener("selectionchange", this._boundSelectionChangeListener, false);
        this._mainPanel._wasShown();
    },

    _handleFocused: function()
    {
        if (this._mainPanel._lastSelection)
            this.setSelection(this._mainPanel._lastSelection);
    },

    willHide: function()
    {
        this._mainPanel._willHide();
        document.removeEventListener("selectionchange", this._boundSelectionChangeListener, false);
        delete this._boundSelectionChangeListener;

        if (!this.readOnly())
            WebInspector.markBeingEdited(this.element, false);
        this._freeCachedElements();
    },

    /**
     * @param {Element} element
     * @param {Array.<Object>} resultRanges
     * @param {string} styleClass
     * @param {Array.<Object>=} changes
     */
    highlightRangesWithStyleClass: function(element, resultRanges, styleClass, changes)
    {
        this._mainPanel.beginDomUpdates();
        WebInspector.highlightRangesWithStyleClass(element, resultRanges, styleClass, changes);
        this._mainPanel.endDomUpdates();
    },

    /**
     * @param {Element} element
     * @param {Object} skipClasses
     * @param {Object} skipTokens
     * @return {Element}
     */
    highlightExpression: function(element, skipClasses, skipTokens)
    {
        // Collect tokens belonging to evaluated expression.
        var tokens = [ element ];
        var token = element.previousSibling;
        while (token && (skipClasses[token.className] || skipTokens[token.textContent.trim()])) {
            tokens.push(token);
            token = token.previousSibling;
        }
        tokens.reverse();

        // Wrap them with highlight element.
        this._mainPanel.beginDomUpdates();
        var parentElement = element.parentElement;
        var nextElement = element.nextSibling;
        var container = document.createElement("span");
        for (var i = 0; i < tokens.length; ++i)
            container.appendChild(tokens[i]);
        parentElement.insertBefore(container, nextElement);
        this._mainPanel.endDomUpdates();
        return container;
    },

    /**
     * @param {Element} highlightElement
     */
    hideHighlightedExpression: function(highlightElement)
    {
        this._mainPanel.beginDomUpdates();
        var parentElement = highlightElement.parentElement;
        if (parentElement) {
            var child = highlightElement.firstChild;
            while (child) {
                var nextSibling = child.nextSibling;
                parentElement.insertBefore(child, highlightElement);
                child = nextSibling;
            }
            parentElement.removeChild(highlightElement);
        }
        this._mainPanel.endDomUpdates();
    },

    /**
     * @param {number} scrollTop
     * @param {number} clientHeight
     * @param {number} chunkSize
     */
    overrideViewportForTest: function(scrollTop, clientHeight, chunkSize)
    {
        this._mainPanel._scrollTopOverrideForTest = scrollTop;
        this._mainPanel._clientHeightOverrideForTest = clientHeight;
        this._mainPanel._defaultChunkSize = chunkSize;
    },

    __proto__: WebInspector.View.prototype
}

/**
 * @constructor
 * @param {WebInspector.TextEditorModel} textModel
 */
WebInspector.TextEditorChunkedPanel = function(textModel)
{
    this._textModel = textModel;

    this._defaultChunkSize = 50;
    this._paintCoalescingLevel = 0;
    this._domUpdateCoalescingLevel = 0;
}

WebInspector.TextEditorChunkedPanel.prototype = {
    /**
     * @param {number} lineNumber
     */
    scrollToLine: function(lineNumber)
    {
        if (lineNumber >= this._textModel.linesCount)
            return;

        var chunk = this.makeLineAChunk(lineNumber);
        this.element.scrollTop = chunk.offsetTop;
    },

    /**
     * @param {number} lineNumber
     */
    revealLine: function(lineNumber)
    {
        if (lineNumber >= this._textModel.linesCount)
            return;

        var chunk = this.makeLineAChunk(lineNumber);
        chunk.element.scrollIntoViewIfNeeded();
    },

    /**
     * @param {number} lineNumber
     * @param {string|Element} decoration
     */
    addDecoration: function(lineNumber, decoration)
    {
        if (lineNumber >= this._textModel.linesCount)
            return;

        var chunk = this.makeLineAChunk(lineNumber);
        chunk.addDecoration(decoration);
    },

    /**
     * @param {number} lineNumber
     * @param {string|Element} decoration
     */
    removeDecoration: function(lineNumber, decoration)
    {
        if (lineNumber >= this._textModel.linesCount)
            return;

        var chunk = this.chunkForLine(lineNumber);
        chunk.removeDecoration(decoration);
    },

    _buildChunks: function()
    {
        this.beginDomUpdates();

        this._container.removeChildren();

        this._textChunks = [];
        for (var i = 0; i < this._textModel.linesCount; i += this._defaultChunkSize) {
            var chunk = this._createNewChunk(i, i + this._defaultChunkSize);
            this._textChunks.push(chunk);
            this._container.appendChild(chunk.element);
        }

        this._repaintAll();

        this.endDomUpdates();
    },

    /**
     * @param {number} lineNumber
     */
    makeLineAChunk: function(lineNumber)
    {
        var chunkNumber = this._chunkNumberForLine(lineNumber);
        var oldChunk = this._textChunks[chunkNumber];

        if (!oldChunk) {
            console.error("No chunk for line number: " + lineNumber);
            return;
        }

        if (oldChunk.linesCount === 1)
            return oldChunk;

        return this._splitChunkOnALine(lineNumber, chunkNumber, true);
    },

    /**
     * @param {number} lineNumber
     * @param {number} chunkNumber
     * @param {boolean=} createSuffixChunk
     */
    _splitChunkOnALine: function(lineNumber, chunkNumber, createSuffixChunk)
    {
        this.beginDomUpdates();

        var oldChunk = this._textChunks[chunkNumber];
        var wasExpanded = oldChunk.expanded;
        oldChunk.expanded = false;

        var insertIndex = chunkNumber + 1;

        // Prefix chunk.
        if (lineNumber > oldChunk.startLine) {
            var prefixChunk = this._createNewChunk(oldChunk.startLine, lineNumber);
            this._textChunks.splice(insertIndex++, 0, prefixChunk);
            this._container.insertBefore(prefixChunk.element, oldChunk.element);
        }

        // Line chunk.
        var endLine = createSuffixChunk ? lineNumber + 1 : oldChunk.startLine + oldChunk.linesCount;
        var lineChunk = this._createNewChunk(lineNumber, endLine);
        this._textChunks.splice(insertIndex++, 0, lineChunk);
        this._container.insertBefore(lineChunk.element, oldChunk.element);

        // Suffix chunk.
        if (oldChunk.startLine + oldChunk.linesCount > endLine) {
            var suffixChunk = this._createNewChunk(endLine, oldChunk.startLine + oldChunk.linesCount);
            this._textChunks.splice(insertIndex, 0, suffixChunk);
            this._container.insertBefore(suffixChunk.element, oldChunk.element);
        }

        // Remove enclosing chunk.
        this._textChunks.splice(chunkNumber, 1);
        this._container.removeChild(oldChunk.element);

        if (wasExpanded) {
            if (prefixChunk)
                prefixChunk.expanded = true;
            lineChunk.expanded = true;
            if (suffixChunk)
                suffixChunk.expanded = true;
        }

        this.endDomUpdates();

        return lineChunk;
    },

    _scroll: function()
    {
        this._scheduleRepaintAll();
        if (this._syncScrollListener)
            this._syncScrollListener();
    },

    _scheduleRepaintAll: function()
    {
        if (this._repaintAllTimer)
            clearTimeout(this._repaintAllTimer);
        this._repaintAllTimer = setTimeout(this._repaintAll.bind(this), 50);
    },

    beginUpdates: function()
    {
        this._paintCoalescingLevel++;
    },

    endUpdates: function()
    {
        this._paintCoalescingLevel--;
        if (!this._paintCoalescingLevel)
            this._repaintAll();
    },

    beginDomUpdates: function()
    {
        this._domUpdateCoalescingLevel++;
    },

    endDomUpdates: function()
    {
        this._domUpdateCoalescingLevel--;
    },

    /**
     * @param {number} lineNumber
     */
    _chunkNumberForLine: function(lineNumber)
    {
        function compareLineNumbers(value, chunk)
        {
            return value < chunk.startLine ? -1 : 1;
        }
        var insertBefore = insertionIndexForObjectInListSortedByFunction(lineNumber, this._textChunks, compareLineNumbers);
        return insertBefore - 1;
    },

    /**
     * @param {number} lineNumber
     */
    chunkForLine: function(lineNumber)
    {
        return this._textChunks[this._chunkNumberForLine(lineNumber)];
    },

    /**
     * @param {number} visibleFrom
     */
    _findFirstVisibleChunkNumber: function(visibleFrom)
    {
        function compareOffsetTops(value, chunk)
        {
            return value < chunk.offsetTop ? -1 : 1;
        }
        var insertBefore = insertionIndexForObjectInListSortedByFunction(visibleFrom, this._textChunks, compareOffsetTops);
        return insertBefore - 1;
    },

    /**
     * @param {number} visibleFrom
     * @param {number} visibleTo
     */
    _findVisibleChunks: function(visibleFrom, visibleTo)
    {
        var from = this._findFirstVisibleChunkNumber(visibleFrom);
        for (var to = from + 1; to < this._textChunks.length; ++to) {
            if (this._textChunks[to].offsetTop >= visibleTo)
                break;
        }
        return { start: from, end: to };
    },

    /**
     * @param {number} visibleFrom
     */
    _findFirstVisibleLineNumber: function(visibleFrom)
    {
        var chunk = this._textChunks[this._findFirstVisibleChunkNumber(visibleFrom)];
        if (!chunk.expanded)
            return chunk.startLine;

        var lineNumbers = [];
        for (var i = 0; i < chunk.linesCount; ++i) {
            lineNumbers.push(chunk.startLine + i);
        }

        function compareLineRowOffsetTops(value, lineNumber)
        {
            var lineRow = chunk.expandedLineRow(lineNumber);
            return value < lineRow.offsetTop ? -1 : 1;
        }
        var insertBefore = insertionIndexForObjectInListSortedByFunction(visibleFrom, lineNumbers, compareLineRowOffsetTops);
        return lineNumbers[insertBefore - 1];
    },

    _repaintAll: function()
    {
        delete this._repaintAllTimer;

        if (this._paintCoalescingLevel)
            return;

        var visibleFrom = this._scrollTop();
        var visibleTo = visibleFrom + this._clientHeight();

        if (visibleTo) {
            var result = this._findVisibleChunks(visibleFrom, visibleTo);
            this._expandChunks(result.start, result.end);
        }
    },

    _scrollTop: function()
    {
        return typeof this._scrollTopOverrideForTest === "number" ? this._scrollTopOverrideForTest : this.element.scrollTop; 
    },

    _clientHeight: function()
    {
        return typeof this._clientHeightOverrideForTest === "number" ? this._clientHeightOverrideForTest : this.element.clientHeight; 
    },

    /**
     * @param {number} fromIndex
     * @param {number} toIndex
     */
    _expandChunks: function(fromIndex, toIndex)
    {
        // First collapse chunks to collect the DOM elements into a cache to reuse them later.
        for (var i = 0; i < fromIndex; ++i)
            this._textChunks[i].expanded = false;
        for (var i = toIndex; i < this._textChunks.length; ++i)
            this._textChunks[i].expanded = false;
        for (var i = fromIndex; i < toIndex; ++i)
            this._textChunks[i].expanded = true;
    },

    /**
     * @param {Element} firstElement
     * @param {Element=} lastElement
     */
    _totalHeight: function(firstElement, lastElement)
    {
        lastElement = (lastElement || firstElement).nextElementSibling;
        if (lastElement)
            return lastElement.offsetTop - firstElement.offsetTop;

        var offsetParent = firstElement.offsetParent;
        if (offsetParent && offsetParent.scrollHeight > offsetParent.clientHeight)
            return offsetParent.scrollHeight - firstElement.offsetTop;

        var total = 0;
        while (firstElement && firstElement !== lastElement) {
            total += firstElement.offsetHeight;
            firstElement = firstElement.nextElementSibling;
        }
        return total;
    },

    resize: function()
    {
        this._repaintAll();
    }
}

/**
 * @constructor
 * @extends {WebInspector.TextEditorChunkedPanel}
 * @param {WebInspector.TextEditorModel} textModel
 */
WebInspector.TextEditorGutterPanel = function(textModel, syncDecorationsForLineListener, syncLineHeightListener)
{
    WebInspector.TextEditorChunkedPanel.call(this, textModel);

    this._syncDecorationsForLineListener = syncDecorationsForLineListener;
    this._syncLineHeightListener = syncLineHeightListener;

    this.element = document.createElement("div");
    this.element.className = "text-editor-lines";

    this._container = document.createElement("div");
    this._container.className = "inner-container";
    this.element.appendChild(this._container);

    this.element.addEventListener("scroll", this._scroll.bind(this), false);

    this._freeCachedElements();
    this._buildChunks();
    this._decorations = {};
}

WebInspector.TextEditorGutterPanel.prototype = {
    _freeCachedElements: function()
    {
        this._cachedRows = [];
    },

    /**
     * @param {number} startLine
     * @param {number} endLine
     */
    _createNewChunk: function(startLine, endLine)
    {
        return new WebInspector.TextEditorGutterChunk(this, startLine, endLine);
    },

    /**
     * @param {WebInspector.TextRange} oldRange
     * @param {WebInspector.TextRange} newRange
     */
    textChanged: function(oldRange, newRange)
    {
        this.beginDomUpdates();

        var linesDiff = newRange.linesCount - oldRange.linesCount;
        if (linesDiff) {
            // Remove old chunks (if needed).
            for (var chunkNumber = this._textChunks.length - 1; chunkNumber >= 0 ; --chunkNumber) {
                var chunk = this._textChunks[chunkNumber];
                if (chunk.startLine + chunk.linesCount <= this._textModel.linesCount)
                    break;
                chunk.expanded = false;
                this._container.removeChild(chunk.element);
            }
            this._textChunks.length = chunkNumber + 1;

            // Add new chunks (if needed).
            var totalLines = 0;
            if (this._textChunks.length) {
                var lastChunk = this._textChunks[this._textChunks.length - 1];
                totalLines = lastChunk.startLine + lastChunk.linesCount;
            }

            for (var i = totalLines; i < this._textModel.linesCount; i += this._defaultChunkSize) {
                var chunk = this._createNewChunk(i, i + this._defaultChunkSize);
                this._textChunks.push(chunk);
                this._container.appendChild(chunk.element);
            }

            // Shift decorations if necessary
            for (var lineNumber in this._decorations) {
                lineNumber = parseInt(lineNumber, 10);

                // Do not move decorations before the start position.
                if (lineNumber < oldRange.startLine)
                    continue;
                // Decorations follow the first character of line.
                if (lineNumber === oldRange.startLine && oldRange.startColumn)
                    continue;

                var lineDecorationsCopy = this._decorations[lineNumber].slice();
                for (var i = 0; i < lineDecorationsCopy.length; ++i) {
                    var decoration = lineDecorationsCopy[i];
                    this.removeDecoration(lineNumber, decoration);

                    // Do not restore the decorations before the end position.
                    if (lineNumber < oldRange.endLine)
                        continue;

                    this.addDecoration(lineNumber + linesDiff, decoration);
                }
            }

            this._repaintAll();
        } else {
            // Decorations may have been removed, so we may have to sync those lines.
            var chunkNumber = this._chunkNumberForLine(newRange.startLine);
            var chunk = this._textChunks[chunkNumber];
            while (chunk && chunk.startLine <= newRange.endLine) {
                if (chunk.linesCount === 1)
                    this._syncDecorationsForLineListener(chunk.startLine);
                chunk = this._textChunks[++chunkNumber];
            }
        }

        this.endDomUpdates();
    },

    /**
     * @param {number} clientHeight
     */
    syncClientHeight: function(clientHeight)
    {
        if (this.element.offsetHeight > clientHeight)
            this._container.style.setProperty("padding-bottom", (this.element.offsetHeight - clientHeight) + "px");
        else
            this._container.style.removeProperty("padding-bottom");
    },

    /**
     * @param {number} lineNumber
     * @param {string|Element} decoration
     */
    addDecoration: function(lineNumber, decoration)
    {
        WebInspector.TextEditorChunkedPanel.prototype.addDecoration.call(this, lineNumber, decoration);
        var decorations = this._decorations[lineNumber];
        if (!decorations) {
            decorations = [];
            this._decorations[lineNumber] = decorations;
        }
        decorations.push(decoration);
    },

    /**
     * @param {number} lineNumber
     * @param {string|Element} decoration
     */
    removeDecoration: function(lineNumber, decoration)
    {
        WebInspector.TextEditorChunkedPanel.prototype.removeDecoration.call(this, lineNumber, decoration);
        var decorations = this._decorations[lineNumber];
        if (decorations) {
            decorations.remove(decoration);
            if (!decorations.length)
                delete this._decorations[lineNumber];
        }
    },

    __proto__: WebInspector.TextEditorChunkedPanel.prototype
}

/**
 * @constructor
 */
WebInspector.TextEditorGutterChunk = function(textEditor, startLine, endLine)
{
    this._textEditor = textEditor;
    this._textModel = textEditor._textModel;

    this.startLine = startLine;
    endLine = Math.min(this._textModel.linesCount, endLine);
    this.linesCount = endLine - startLine;

    this._expanded = false;

    this.element = document.createElement("div");
    this.element.lineNumber = startLine;
    this.element.className = "webkit-line-number";

    if (this.linesCount === 1) {
        // Single line chunks are typically created for decorations. Host line number in
        // the sub-element in order to allow flexible border / margin management.
        var innerSpan = document.createElement("span");
        innerSpan.className = "webkit-line-number-inner";
        innerSpan.textContent = startLine + 1;
        var outerSpan = document.createElement("div");
        outerSpan.className = "webkit-line-number-outer";
        outerSpan.appendChild(innerSpan);
        this.element.appendChild(outerSpan);
    } else {
        var lineNumbers = [];
        for (var i = startLine; i < endLine; ++i)
            lineNumbers.push(i + 1);
        this.element.textContent = lineNumbers.join("\n");
    }
}

WebInspector.TextEditorGutterChunk.prototype = {
    /**
     * @param {string} decoration
     */
    addDecoration: function(decoration)
    {
        this._textEditor.beginDomUpdates();
        if (typeof decoration === "string")
            this.element.addStyleClass(decoration);
        this._textEditor.endDomUpdates();
    },

    /**
     * @param {string} decoration
     */
    removeDecoration: function(decoration)
    {
        this._textEditor.beginDomUpdates();
        if (typeof decoration === "string")
            this.element.removeStyleClass(decoration);
        this._textEditor.endDomUpdates();
    },

    /**
     * @return {boolean}
     */
    get expanded()
    {
        return this._expanded;
    },

    set expanded(expanded)
    {
        if (this.linesCount === 1)
            this._textEditor._syncDecorationsForLineListener(this.startLine);

        if (this._expanded === expanded)
            return;

        this._expanded = expanded;

        if (this.linesCount === 1)
            return;

        this._textEditor.beginDomUpdates();

        if (expanded) {
            this._expandedLineRows = [];
            var parentElement = this.element.parentElement;
            for (var i = this.startLine; i < this.startLine + this.linesCount; ++i) {
                var lineRow = this._createRow(i);
                parentElement.insertBefore(lineRow, this.element);
                this._expandedLineRows.push(lineRow);
            }
            parentElement.removeChild(this.element);
            this._textEditor._syncLineHeightListener(this._expandedLineRows[0]);
        } else {
            var elementInserted = false;
            for (var i = 0; i < this._expandedLineRows.length; ++i) {
                var lineRow = this._expandedLineRows[i];
                var parentElement = lineRow.parentElement;
                if (parentElement) {
                    if (!elementInserted) {
                        elementInserted = true;
                        parentElement.insertBefore(this.element, lineRow);
                    }
                    parentElement.removeChild(lineRow);
                }
                this._textEditor._cachedRows.push(lineRow);
            }
            delete this._expandedLineRows;
        }

        this._textEditor.endDomUpdates();
    },

    /**
     * @return {number}
     */
    get height()
    {
        if (!this._expandedLineRows)
            return this._textEditor._totalHeight(this.element);
        return this._textEditor._totalHeight(this._expandedLineRows[0], this._expandedLineRows[this._expandedLineRows.length - 1]);
    },

    /**
     * @return {number}
     */
    get offsetTop()
    {
        return (this._expandedLineRows && this._expandedLineRows.length) ? this._expandedLineRows[0].offsetTop : this.element.offsetTop;
    },

    /**
     * @param {number} lineNumber
     * @return {Element}
     */
    _createRow: function(lineNumber)
    {
        var lineRow = this._textEditor._cachedRows.pop() || document.createElement("div");
        lineRow.lineNumber = lineNumber;
        lineRow.className = "webkit-line-number";
        lineRow.textContent = lineNumber + 1;
        return lineRow;
    }
}

/**
 * @constructor
 * @extends {WebInspector.TextEditorChunkedPanel}
 * @param {WebInspector.TextEditorDelegate} delegate
 * @param {WebInspector.TextEditorModel} textModel
 * @param {?string} url
 */
WebInspector.TextEditorMainPanel = function(delegate, textModel, url, syncScrollListener, syncDecorationsForLineListener)
{
    WebInspector.TextEditorChunkedPanel.call(this, textModel);

    this._delegate = delegate;
    this._syncScrollListener = syncScrollListener;
    this._syncDecorationsForLineListener = syncDecorationsForLineListener;

    this._url = url;
    this._highlighter = new WebInspector.TextEditorHighlighter(textModel, this._highlightDataReady.bind(this));
    this._readOnly = true;

    this.element = document.createElement("div");
    this.element.className = "text-editor-contents";
    this.element.tabIndex = 0;

    this._container = document.createElement("div");
    this._container.className = "inner-container";
    this._container.tabIndex = 0;
    this.element.appendChild(this._container);

    this.element.addEventListener("scroll", this._scroll.bind(this), false);
    this.element.addEventListener("focus", this._handleElementFocus.bind(this), false);

    this._freeCachedElements();
    this._buildChunks();
}

WebInspector.TextEditorMainPanel.prototype = {
    _wasShown: function()
    {
        this._isShowing = true;
        this._attachMutationObserver();
    },

    _willHide: function()
    {
        this._detachMutationObserver();
        this._isShowing = false;
    },

    _attachMutationObserver: function()
    {
        if (!this._isShowing)
            return;

        if (this._mutationObserver)
            this._mutationObserver.disconnect();
        this._mutationObserver = new NonLeakingMutationObserver(this._handleMutations.bind(this));
        this._mutationObserver.observe(this._container, { subtree: true, childList: true, characterData: true });
    },

    _detachMutationObserver: function()
    {
        if (!this._isShowing)
            return;

        if (this._mutationObserver) {
            this._mutationObserver.disconnect();
            delete this._mutationObserver;
        }
    },

    /**
     * @param {string} mimeType
     */
    set mimeType(mimeType)
    {
        this._highlighter.mimeType = mimeType;
    },

    /**
     * @param {boolean} readOnly
     * @param {boolean} requestFocus
     */
    setReadOnly: function(readOnly, requestFocus)
    {
        if (this._readOnly === readOnly)
            return;

        this.beginDomUpdates();
        this._readOnly = readOnly;
        if (this._readOnly)
            this._container.removeStyleClass("text-editor-editable");
        else {
            this._container.addStyleClass("text-editor-editable");
            if (requestFocus)
                this._updateSelectionOnStartEditing();
        }
        this.endDomUpdates();
    },

    /**
     * @return {boolean}
     */
    readOnly: function()
    {
        return this._readOnly;
    },

    _handleElementFocus: function()
    {
        if (!this._readOnly)
            this._container.focus();
    },

    /**
     * @return {Element}
     */
    defaultFocusedElement: function()
    {
        if (this._readOnly)
            return this.element;
        return this._container;
    },

    _updateSelectionOnStartEditing: function()
    {
        // focus() needs to go first for the case when the last selection was inside the editor and
        // the "Edit" button was clicked. In this case we bail at the check below, but the
        // editor does not receive the focus, thus "Esc" does not cancel editing until at least
        // one change has been made to the editor contents.
        this._container.focus();
        var selection = window.getSelection();
        if (selection.rangeCount) {
            var commonAncestorContainer = selection.getRangeAt(0).commonAncestorContainer;
            if (this._container.isSelfOrAncestor(commonAncestorContainer))
                return;
        }

        selection.removeAllRanges();
        var range = document.createRange();
        range.setStart(this._container, 0);
        range.setEnd(this._container, 0);
        selection.addRange(range);
    },

    /**
     * @param {WebInspector.TextRange} range
     */
    markAndRevealRange: function(range)
    {
        if (this._rangeToMark) {
            var markedLine = this._rangeToMark.startLine;
            delete this._rangeToMark;
            // Remove the marked region immediately.
            this.beginDomUpdates();
            var chunk = this.chunkForLine(markedLine);
            var wasExpanded = chunk.expanded;
            chunk.expanded = false;
            chunk.updateCollapsedLineRow();
            chunk.expanded = wasExpanded;
            this.endDomUpdates();
        }

        if (range) {
            this._rangeToMark = range;
            this.revealLine(range.startLine);
            var chunk = this.makeLineAChunk(range.startLine);
            this._paintLine(chunk.element);
            if (this._markedRangeElement)
                this._markedRangeElement.scrollIntoViewIfNeeded();
        }
        delete this._markedRangeElement;
    },

    /**
     * @param {number} lineNumber
     */
    highlightLine: function(lineNumber)
    {
        this.clearLineHighlight();
        this._highlightedLine = lineNumber;
        this.revealLine(lineNumber);

        if (!this._readOnly)
            this._restoreSelection(WebInspector.TextRange.createFromLocation(lineNumber, 0), false);

        this.addDecoration(lineNumber, "webkit-highlighted-line");
    },

    clearLineHighlight: function()
    {
        if (typeof this._highlightedLine === "number") {
            this.removeDecoration(this._highlightedLine, "webkit-highlighted-line");
            delete this._highlightedLine;
        }
    },

    _freeCachedElements: function()
    {
        this._cachedSpans = [];
        this._cachedTextNodes = [];
        this._cachedRows = [];
    },

    /**
     * @param {boolean} redo
     */
    handleUndoRedo: function(redo)
    {
        if (this.readOnly())
            return false;

        this.beginUpdates();

        var range = redo ? this._textModel.redo() : this._textModel.undo();

        this.endUpdates();

        // Restore location post-repaint.
        if (range)
            this._restoreSelection(range, true);

        return true;
    },

    /**
     * @param {boolean} shiftKey
     */
    handleTabKeyPress: function(shiftKey)
    {
        if (this.readOnly())
            return false;

        var selection = this._getSelection();
        if (!selection)
            return false;

        var range = selection.normalize();

        this.beginUpdates();

        var newRange;
        var rangeWasEmpty = range.isEmpty();
        if (shiftKey)
            newRange = this._textModel.unindentLines(range);
        else {
            if (rangeWasEmpty)
                newRange = this._textModel.editRange(range, WebInspector.settings.textEditorIndent.get());
            else
                newRange = this._textModel.indentLines(range);
        }

        this.endUpdates();
        if (rangeWasEmpty)
            newRange.startColumn = newRange.endColumn;
        this._restoreSelection(newRange, true);
        return true;
    },

    handleEnterKey: function()
    {
        if (this.readOnly())
            return false;

        var range = this._getSelection();
        if (!range)
            return false;

        range = range.normalize();

        if (range.endColumn === 0)
            return false;

        var line = this._textModel.line(range.startLine);
        var linePrefix = line.substring(0, range.startColumn);
        var indentMatch = linePrefix.match(/^\s+/);
        var currentIndent = indentMatch ? indentMatch[0] : "";

        var textEditorIndent = WebInspector.settings.textEditorIndent.get();
        var indent = WebInspector.TextEditorModel.endsWithBracketRegex.test(linePrefix) ? currentIndent + textEditorIndent : currentIndent;

        if (!indent)
            return false;

        this.beginDomUpdates();

        var lineBreak = this._textModel.lineBreak;
        var newRange;
        if (range.isEmpty() && line.substr(range.endColumn - 1, 2) === '{}') {
            // {|}
            // becomes
            // {
            //     |
            // }
            newRange = this._textModel.editRange(range, lineBreak + indent + lineBreak + currentIndent);
            newRange.endLine--;
            newRange.endColumn += textEditorIndent.length;
        } else
            newRange = this._textModel.editRange(range, lineBreak + indent);

        this.endDomUpdates();
        this._restoreSelection(newRange.collapseToEnd(), true);

        return true;
    },

    /**
     * @param {number} lineNumber
     * @param {number} chunkNumber
     * @param {boolean=} createSuffixChunk
     */
    _splitChunkOnALine: function(lineNumber, chunkNumber, createSuffixChunk)
    {
        var selection = this._getSelection();
        var chunk = WebInspector.TextEditorChunkedPanel.prototype._splitChunkOnALine.call(this, lineNumber, chunkNumber, createSuffixChunk);
        this._restoreSelection(selection);
        return chunk;
    },

    beginDomUpdates: function()
    {
        if (!this._domUpdateCoalescingLevel)
            this._detachMutationObserver();
        WebInspector.TextEditorChunkedPanel.prototype.beginDomUpdates.call(this);
    },

    endDomUpdates: function()
    {
        WebInspector.TextEditorChunkedPanel.prototype.endDomUpdates.call(this);
        if (!this._domUpdateCoalescingLevel)
            this._attachMutationObserver();
    },

    _buildChunks: function()
    {
        for (var i = 0; i < this._textModel.linesCount; ++i)
            this._textModel.removeAttribute(i, "highlight");

        WebInspector.TextEditorChunkedPanel.prototype._buildChunks.call(this);
    },

    /**
     * @param {number} startLine
     * @param {number} endLine
     */
    _createNewChunk: function(startLine, endLine)
    {
        return new WebInspector.TextEditorMainChunk(this, startLine, endLine);
    },

    /**
     * @param {number} fromIndex
     * @param {number} toIndex
     */
    _expandChunks: function(fromIndex, toIndex)
    {
        var lastChunk = this._textChunks[toIndex - 1];
        var lastVisibleLine = lastChunk.startLine + lastChunk.linesCount;

        var selection = this._getSelection();

        this._muteHighlightListener = true;
        this._highlighter.highlight(lastVisibleLine);
        delete this._muteHighlightListener;

        this._restorePaintLinesOperationsCredit();
        WebInspector.TextEditorChunkedPanel.prototype._expandChunks.call(this, fromIndex, toIndex);
        this._adjustPaintLinesOperationsRefreshValue();

        this._restoreSelection(selection);
    },

    /**
     * @param {number} fromLine
     * @param {number} toLine
     */
    _highlightDataReady: function(fromLine, toLine)
    {
        if (this._muteHighlightListener)
            return;
        this._restorePaintLinesOperationsCredit();
        this._paintLines(fromLine, toLine, true /*restoreSelection*/);
    },

    /**
     * @param {number} startLine
     * @param {number} endLine
     */
    _schedulePaintLines: function(startLine, endLine)
    {
        if (startLine >= endLine)
            return;

        if (!this._scheduledPaintLines) {
            this._scheduledPaintLines = [ { startLine: startLine, endLine: endLine } ];
            this._paintScheduledLinesTimer = setTimeout(this._paintScheduledLines.bind(this), 0);
        } else {
            for (var i = 0; i < this._scheduledPaintLines.length; ++i) {
                var chunk = this._scheduledPaintLines[i];
                if (chunk.startLine <= endLine && chunk.endLine >= startLine) {
                    chunk.startLine = Math.min(chunk.startLine, startLine);
                    chunk.endLine = Math.max(chunk.endLine, endLine);
                    return;
                }
                if (chunk.startLine > endLine) {
                    this._scheduledPaintLines.splice(i, 0, { startLine: startLine, endLine: endLine });
                    return;
                }
            }
            this._scheduledPaintLines.push({ startLine: startLine, endLine: endLine });
        }
    },

    /**
     * @param {boolean} skipRestoreSelection
     */
    _paintScheduledLines: function(skipRestoreSelection)
    {
        if (this._paintScheduledLinesTimer)
            clearTimeout(this._paintScheduledLinesTimer);
        delete this._paintScheduledLinesTimer;

        if (!this._scheduledPaintLines)
            return;

        // Reschedule the timer if we can not paint the lines yet, or the user is scrolling.
        if (this._repaintAllTimer) {
            this._paintScheduledLinesTimer = setTimeout(this._paintScheduledLines.bind(this), 50);
            return;
        }

        var scheduledPaintLines = this._scheduledPaintLines;
        delete this._scheduledPaintLines;

        this._restorePaintLinesOperationsCredit();
        this._paintLineChunks(scheduledPaintLines, !skipRestoreSelection);
        this._adjustPaintLinesOperationsRefreshValue();
    },

    _restorePaintLinesOperationsCredit: function()
    {
        if (!this._paintLinesOperationsRefreshValue)
            this._paintLinesOperationsRefreshValue = 250;
        this._paintLinesOperationsCredit = this._paintLinesOperationsRefreshValue;
        this._paintLinesOperationsLastRefresh = Date.now();
    },

    _adjustPaintLinesOperationsRefreshValue: function()
    {
        var operationsDone = this._paintLinesOperationsRefreshValue - this._paintLinesOperationsCredit;
        if (operationsDone <= 0)
            return;
        var timePast = Date.now() - this._paintLinesOperationsLastRefresh;
        if (timePast <= 0)
            return;
        // Make the synchronous CPU chunk for painting the lines 50 msec.
        var value = Math.floor(operationsDone / timePast * 50);
        this._paintLinesOperationsRefreshValue = Number.constrain(value, 150, 1500);
    },

    /**
     * @param {number} fromLine
     * @param {number} toLine
     * @param {boolean=} restoreSelection
     */
    _paintLines: function(fromLine, toLine, restoreSelection)
    {
        this._paintLineChunks([ { startLine: fromLine, endLine: toLine } ], restoreSelection);
    },

    /**
     * @param {boolean=} restoreSelection
     */
    _paintLineChunks: function(lineChunks, restoreSelection)
    {
        // First, paint visible lines, so that in case of long lines we should start highlighting
        // the visible area immediately, instead of waiting for the lines above the visible area.
        var visibleFrom = this._scrollTop();
        var firstVisibleLineNumber = this._findFirstVisibleLineNumber(visibleFrom);

        var chunk;
        var selection;
        var invisibleLineRows = [];
        for (var i = 0; i < lineChunks.length; ++i) {
            var lineChunk = lineChunks[i];
            if (this._scheduledPaintLines) {
                this._schedulePaintLines(lineChunk.startLine, lineChunk.endLine);
                continue;
            }
            for (var lineNumber = lineChunk.startLine; lineNumber < lineChunk.endLine; ++lineNumber) {
                if (!chunk || lineNumber < chunk.startLine || lineNumber >= chunk.startLine + chunk.linesCount)
                    chunk = this.chunkForLine(lineNumber);
                var lineRow = chunk.expandedLineRow(lineNumber);
                if (!lineRow)
                    continue;
                if (lineNumber < firstVisibleLineNumber) {
                    invisibleLineRows.push(lineRow);
                    continue;
                }
                if (restoreSelection && !selection)
                    selection = this._getSelection();
                this._paintLine(lineRow);
                if (this._paintLinesOperationsCredit < 0) {
                    this._schedulePaintLines(lineNumber + 1, lineChunk.endLine);
                    break;
                }
            }
        }

        for (var i = 0; i < invisibleLineRows.length; ++i) {
            if (restoreSelection && !selection)
                selection = this._getSelection();
            this._paintLine(invisibleLineRows[i]);
        }

        if (restoreSelection)
            this._restoreSelection(selection);
    },

    /**
     * @param {Element} lineRow
     */
    _paintLine: function(lineRow)
    {
        var lineNumber = lineRow.lineNumber;

        this.beginDomUpdates();
        try {
            if (this._scheduledPaintLines || this._paintLinesOperationsCredit < 0) {
                this._schedulePaintLines(lineNumber, lineNumber + 1);
                return;
            }

            var highlight = this._textModel.getAttribute(lineNumber, "highlight");
            if (!highlight)
                return;

            var decorationsElement = lineRow.decorationsElement;
            if (!decorationsElement)
                lineRow.removeChildren();
            else {
                while (true) {
                    var child = lineRow.firstChild;
                    if (!child || child === decorationsElement)
                        break;
                    lineRow.removeChild(child);
                }
            }

            var line = this._textModel.line(lineNumber);
            if (!line)
                lineRow.insertBefore(document.createElement("br"), decorationsElement);

            var plainTextStart = -1;
            for (var j = 0; j < line.length;) {
                if (j > 1000) {
                    // This line is too long - do not waste cycles on minified js highlighting.
                    if (plainTextStart === -1)
                        plainTextStart = j;
                    break;
                }
                var attribute = highlight[j];
                if (!attribute || !attribute.tokenType) {
                    if (plainTextStart === -1)
                        plainTextStart = j;
                    j++;
                } else {
                    if (plainTextStart !== -1) {
                        this._insertTextNodeBefore(lineRow, decorationsElement, line.substring(plainTextStart, j));
                        plainTextStart = -1;
                        --this._paintLinesOperationsCredit;
                    }
                    this._insertSpanBefore(lineRow, decorationsElement, line.substring(j, j + attribute.length), attribute.tokenType);
                    j += attribute.length;
                    --this._paintLinesOperationsCredit;
                }
            }
            if (plainTextStart !== -1) {
                this._insertTextNodeBefore(lineRow, decorationsElement, line.substring(plainTextStart, line.length));
                --this._paintLinesOperationsCredit;
            }
        } finally {
            if (this._rangeToMark && this._rangeToMark.startLine === lineNumber)
                this._markedRangeElement = WebInspector.highlightSearchResult(lineRow, this._rangeToMark.startColumn, this._rangeToMark.endColumn - this._rangeToMark.startColumn);
            this.endDomUpdates();
        }
    },

    /**
     * @param {Element} lineRow
     */
    _releaseLinesHighlight: function(lineRow)
    {
        if (!lineRow)
            return;
        if ("spans" in lineRow) {
            var spans = lineRow.spans;
            for (var j = 0; j < spans.length; ++j)
                this._cachedSpans.push(spans[j]);
            delete lineRow.spans;
        }
        if ("textNodes" in lineRow) {
            var textNodes = lineRow.textNodes;
            for (var j = 0; j < textNodes.length; ++j)
                this._cachedTextNodes.push(textNodes[j]);
            delete lineRow.textNodes;
        }
        this._cachedRows.push(lineRow);
    },

    /**
     * @param {?Node=} lastUndamagedLineRow
     * @return {WebInspector.TextRange}
     */
    _getSelection: function(lastUndamagedLineRow)
    {
        var selection = window.getSelection();
        if (!selection.rangeCount)
            return null;
        // Selection may be outside of the editor.
        if (!this._container.isAncestor(selection.anchorNode) || !this._container.isAncestor(selection.focusNode))
            return null;
        var start = this._selectionToPosition(selection.anchorNode, selection.anchorOffset, lastUndamagedLineRow);
        var end = selection.isCollapsed ? start : this._selectionToPosition(selection.focusNode, selection.focusOffset, lastUndamagedLineRow);
        return new WebInspector.TextRange(start.line, start.column, end.line, end.column);
    },

    /**
     * @param {boolean=} scrollIntoView
     */
    _restoreSelection: function(range, scrollIntoView)
    {
        if (!range)
            return;

        var start = this._positionToSelection(range.startLine, range.startColumn);
        var end = range.isEmpty() ? start : this._positionToSelection(range.endLine, range.endColumn);
        window.getSelection().setBaseAndExtent(start.container, start.offset, end.container, end.offset);

        if (scrollIntoView) {
            for (var node = end.container; node; node = node.parentElement) {
                if (node.scrollIntoViewIfNeeded) {
                    node.scrollIntoViewIfNeeded();
                    break;
                }
            }
        }
        this._lastSelection = range;
    },

    /**
     * @param {Node} container
     * @param {number} offset
     * @param {?Node=} lastUndamagedLineRow
     */
    _selectionToPosition: function(container, offset, lastUndamagedLineRow)
    {
        if (container === this._container && offset === 0)
            return { line: 0, column: 0 };
        if (container === this._container && offset === 1)
            return { line: this._textModel.linesCount - 1, column: this._textModel.lineLength(this._textModel.linesCount - 1) };

        // This method can be called on the damaged DOM (when DOM does not match model).
        // We need to start counting lines from the first undamaged line if it is given.
        var lineNumber;
        var column = 0;
        var node;
        var scopeNode;
        if (lastUndamagedLineRow === null) {
             // Last undamaged row is given, but is null - force traverse from the beginning
            node = this._container.firstChild;
            scopeNode = this._container;
            lineNumber = 0;
        } else {
            var lineRow = this._enclosingLineRowOrSelf(container);
            if (!lastUndamagedLineRow || (typeof lineRow.lineNumber === "number" && lineRow.lineNumber <= lastUndamagedLineRow.lineNumber)) {
                // DOM is consistent (or we belong to the first damaged row)- lookup the row we belong to and start with it.
                node = lineRow;
                scopeNode = node;
                lineNumber = node.lineNumber;
            } else {
                // Start with the node following undamaged row. It corresponds to lineNumber + 1.
                node = lastUndamagedLineRow.nextSibling;
                scopeNode = this._container;
                lineNumber = lastUndamagedLineRow.lineNumber + 1;
            }
        }

        // Fast return the line start.
        if (container === node && offset === 0)
            return { line: lineNumber, column: 0 };

        // Traverse text and increment lineNumber / column.
        for (; node && node !== container; node = node.traverseNextNode(scopeNode)) {
            if (node.nodeName.toLowerCase() === "br") {
                lineNumber++;
                column = 0;
            } else if (node.nodeType === Node.TEXT_NODE) {
                var text = node.textContent;
                for (var i = 0; i < text.length; ++i) {
                    if (text.charAt(i) === "\n") {
                        lineNumber++;
                        column = 0;
                    } else
                        column++;
                }
            }
        }

        // We reached our container node, traverse within itself until we reach given offset.
        if (node === container && offset) {
            var text = node.textContent;
            // In case offset == 1 and lineRow is a chunk div, we need to traverse it all.
            var textOffset = (node._chunk && offset === 1) ? text.length : offset;
            for (var i = 0; i < textOffset; ++i) {
                if (text.charAt(i) === "\n") {
                    lineNumber++;
                    column = 0;
                } else
                    column++;
            }
        }
        return { line: lineNumber, column: column };
    },

    /**
     * @param {number} line
     * @param {number} column
     */
    _positionToSelection: function(line, column)
    {
        var chunk = this.chunkForLine(line);
        // One-lined collapsed chunks may still stay highlighted.
        var lineRow = chunk.linesCount === 1 ? chunk.element : chunk.expandedLineRow(line);
        if (lineRow)
            var rangeBoundary = lineRow.rangeBoundaryForOffset(column);
        else {
            var offset = column;
            for (var i = chunk.startLine; i < line && i < this._textModel.linesCount; ++i)
                offset += this._textModel.lineLength(i) + 1; // \n
            lineRow = chunk.element;
            if (lineRow.firstChild)
                var rangeBoundary = { container: lineRow.firstChild, offset: offset };
            else
                var rangeBoundary = { container: lineRow, offset: 0 };
        }
        return rangeBoundary;
    },

    /**
     * @param {Node} element
     */
    _enclosingLineRowOrSelf: function(element)
    {
        var lineRow = element.enclosingNodeOrSelfWithClass("webkit-line-content");
        if (lineRow)
            return lineRow;

        for (lineRow = element; lineRow; lineRow = lineRow.parentElement) {
            if (lineRow.parentElement === this._container)
                return lineRow;
        }
        return null;
    },

    /**
     * @param {Element} element
     * @param {Element} oldChild
     * @param {string} content
     * @param {string} className
     */
    _insertSpanBefore: function(element, oldChild, content, className)
    {
        if (className === "html-resource-link" || className === "html-external-link") {
            element.insertBefore(this._createLink(content, className === "html-external-link"), oldChild);
            return;
        }

        var span = this._cachedSpans.pop() || document.createElement("span");
        span.className = "webkit-" + className;
        if (WebInspector.FALSE) // For paint debugging.
            span.addStyleClass("debug-fadeout");
        span.textContent = content;
        element.insertBefore(span, oldChild);
        if (!("spans" in element))
            element.spans = [];
        element.spans.push(span);
    },

    /**
     * @param {Element} element
     * @param {Element} oldChild
     * @param {string} text
     */
    _insertTextNodeBefore: function(element, oldChild, text)
    {
        var textNode = this._cachedTextNodes.pop();
        if (textNode)
            textNode.nodeValue = text;
        else
            textNode = document.createTextNode(text);
        element.insertBefore(textNode, oldChild);
        if (!("textNodes" in element))
            element.textNodes = [];
        element.textNodes.push(textNode);
    },

    /**
     * @param {string} content
     * @param {boolean} isExternal
     */
    _createLink: function(content, isExternal)
    {
        var quote = content.charAt(0);
        if (content.length > 1 && (quote === "\"" ||   quote === "'"))
            content = content.substring(1, content.length - 1);
        else
            quote = null;

        var span = document.createElement("span");
        span.className = "webkit-html-attribute-value";
        if (quote)
            span.appendChild(document.createTextNode(quote));
        span.appendChild(this._delegate.createLink(content, isExternal));
        if (quote)
            span.appendChild(document.createTextNode(quote));
        return span;
    },

    /**
     * @param {Array.<WebKitMutation>} mutations
     */
    _handleMutations: function(mutations)
    {
        if (this._readOnly) {
            delete this._keyDownCode;
            return;
        }

        // Annihilate noop BR addition + removal that takes place upon line removal.
        var filteredMutations = mutations.slice();
        var addedBRs = new Map();
        for (var i = 0; i < mutations.length; ++i) {
            var mutation = mutations[i];
            if (mutation.type !== "childList")
                continue;
            if (mutation.addedNodes.length === 1 && mutation.addedNodes[0].nodeName === "BR")
                addedBRs.put(mutation.addedNodes[0], mutation);
            else if (mutation.removedNodes.length === 1 && mutation.removedNodes[0].nodeName === "BR") {
                var noopMutation = addedBRs.get(mutation.removedNodes[0]);
                if (noopMutation) {
                    filteredMutations.remove(mutation);
                    filteredMutations.remove(noopMutation);
                }
            }
        }

        var dirtyLines;
        for (var i = 0; i < filteredMutations.length; ++i) {
            var mutation = filteredMutations[i];
            var changedNodes = [];
            if (mutation.type === "childList" && mutation.addedNodes.length)
                changedNodes = Array.prototype.slice.call(mutation.addedNodes);
            else if (mutation.type === "childList" && mutation.removedNodes.length)
                changedNodes = Array.prototype.slice.call(mutation.removedNodes);
            changedNodes.push(mutation.target);

            for (var j = 0; j < changedNodes.length; ++j) {
                var lines = this._collectDirtyLines(mutation, changedNodes[j]);
                if (!lines)
                    continue;
                if (!dirtyLines) {
                    dirtyLines = lines;
                    continue;
                }
                dirtyLines.start = Math.min(dirtyLines.start, lines.start);
                dirtyLines.end = Math.max(dirtyLines.end, lines.end);
            }
        }
        if (dirtyLines) {
            delete this._rangeToMark;
            this._applyDomUpdates(dirtyLines);
        }

        this._assertDOMMatchesTextModel();

        delete this._keyDownCode;
    },

    /**
     * @param {WebKitMutation} mutation
     * @param {Node} target
     * @return {?Object}
     */
    _collectDirtyLines: function(mutation, target)
    {
        var lineRow = this._enclosingLineRowOrSelf(target);
        if (!lineRow)
            return null;

        if (lineRow.decorationsElement && lineRow.decorationsElement.isSelfOrAncestor(target)) {
            if (this._syncDecorationsForLineListener)
                this._syncDecorationsForLineListener(lineRow.lineNumber);
            return null;
        }

        if (typeof lineRow.lineNumber !== "number")
            return null;

        var startLine = lineRow.lineNumber;
        var endLine = lineRow._chunk ? lineRow._chunk.endLine - 1 : lineRow.lineNumber;
        return { start: startLine, end: endLine };
    },

    /**
     * @param {Object} dirtyLines
     */
    _applyDomUpdates: function(dirtyLines)
    {
        var lastUndamagedLineNumber = dirtyLines.start - 1; // Can be -1
        var firstUndamagedLineNumber = dirtyLines.end + 1; // Can be this._textModel.linesCount

        var lastUndamagedLineChunk = lastUndamagedLineNumber >= 0 ? this._textChunks[this._chunkNumberForLine(lastUndamagedLineNumber)] : null;
        var firstUndamagedLineChunk = firstUndamagedLineNumber  < this._textModel.linesCount ? this._textChunks[this._chunkNumberForLine(firstUndamagedLineNumber)] : null;

        var collectLinesFromNode = lastUndamagedLineChunk ? lastUndamagedLineChunk.lineRowContainingLine(lastUndamagedLineNumber) : null;
        var collectLinesToNode = firstUndamagedLineChunk ? firstUndamagedLineChunk.lineRowContainingLine(firstUndamagedLineNumber) : null;
        var lines = this._collectLinesFromDOM(collectLinesFromNode, collectLinesToNode);

        var startLine = dirtyLines.start;
        var endLine = dirtyLines.end;

        var editInfo = this._guessEditRangeBasedOnSelection(startLine, endLine, lines);
        if (!editInfo) {
            if (WebInspector.debugDefaultTextEditor)
                console.warn("Falling back to expensive edit");
            var range = new WebInspector.TextRange(startLine, 0, endLine, this._textModel.lineLength(endLine));
            if (!lines.length) {
                // Entire damaged area has collapsed. Replace everything between start and end lines with nothing.
                editInfo = new WebInspector.DefaultTextEditor.EditInfo(this._textModel.growRangeRight(range), "");
            } else
                editInfo = new WebInspector.DefaultTextEditor.EditInfo(range, lines.join("\n"));
        }

        var selection = this._getSelection(collectLinesFromNode);

        // Unindent after block
        if (editInfo.text === "}" && editInfo.range.isEmpty() && selection.isEmpty() && !this._textModel.line(editInfo.range.endLine).trim()) {
            var offset = this._closingBlockOffset(editInfo.range, selection);
            if (offset >= 0) {
                editInfo.range.startColumn = offset;
                selection.startColumn = offset + 1;
                selection.endColumn = offset + 1;
            }
        }

        this._textModel.editRange(editInfo.range, editInfo.text);
        this._paintScheduledLines(true);
        this._restoreSelection(selection);
    },

    /**
     * @param {number} startLine
     * @param {number} endLine
     * @param {Array.<string>} lines
     * @return {?WebInspector.DefaultTextEditor.EditInfo}
     */
    _guessEditRangeBasedOnSelection: function(startLine, endLine, lines)
    {
        // Analyze input data
        var textInputData = this._textInputData;
        delete this._textInputData;
        var isBackspace = this._keyDownCode === WebInspector.KeyboardShortcut.Keys.Backspace.code;
        var isDelete = this._keyDownCode === WebInspector.KeyboardShortcut.Keys.Delete.code;

        if (!textInputData && (isDelete || isBackspace))
            textInputData = "";

        // Return if there is no input data or selection
        if (typeof textInputData === "undefined" || !this._lastSelection)
            return null;

        // Adjust selection based on the keyboard actions (grow for backspace, etc.).
        textInputData = textInputData || "";
        var range = this._lastSelection.normalize();
        if (isBackspace && range.isEmpty())
            range = this._textModel.growRangeLeft(range);
        else if (isDelete && range.isEmpty())
            range = this._textModel.growRangeRight(range);

        // Test that selection intersects damaged lines
        if (startLine > range.endLine || endLine < range.startLine)
            return null;

        var replacementLineCount = textInputData.split("\n").length - 1;
        var lineCountDelta = replacementLineCount - range.linesCount;
        if (startLine + lines.length - endLine - 1 !== lineCountDelta)
            return null;

        // Clone text model of the size that fits both: selection before edit and the damaged lines after edit.
        var cloneFromLine = Math.min(range.startLine, startLine);
        var postLastLine = startLine + lines.length + lineCountDelta;
        var cloneToLine = Math.min(Math.max(postLastLine, range.endLine) + 1, this._textModel.linesCount);
        var domModel = this._textModel.slice(cloneFromLine, cloneToLine);
        domModel.editRange(range.shift(-cloneFromLine), textInputData);

        // Then we'll test if this new model matches the DOM lines.
        for (var i = 0;  i < lines.length; ++i) {
            if (domModel.line(i + startLine - cloneFromLine) !== lines[i])
                return null;
        }
        return new WebInspector.DefaultTextEditor.EditInfo(range, textInputData);
    },

    _assertDOMMatchesTextModel: function()
    {
        if (!WebInspector.debugDefaultTextEditor)
            return;

        console.assert(this.element.innerText === this._textModel.text() + "\n", "DOM does not match model.");
        for (var lineRow = this._container.firstChild; lineRow; lineRow = lineRow.nextSibling) {
            var lineNumber = lineRow.lineNumber;
            if (typeof lineNumber !== "number") {
                console.warn("No line number on line row");
                continue;
            }
            if (lineRow._chunk) {
                var chunk = lineRow._chunk;
                console.assert(lineNumber === chunk.startLine);
                var chunkText = this._textModel.copyRange(new WebInspector.TextRange(chunk.startLine, 0, chunk.endLine - 1, this._textModel.lineLength(chunk.endLine - 1)));
                if (chunkText !== lineRow.textContent)
                    console.warn("Chunk is not matching: %d %O", lineNumber, lineRow);
            } else if (this._textModel.line(lineNumber) !== lineRow.textContent)
                console.warn("Line is not matching: %d %O", lineNumber, lineRow);
        }
    },

    /**
     * @param {WebInspector.TextRange} oldRange
     * @param {WebInspector.TextRange} selection
     * @return {number}
     */
    _closingBlockOffset: function(oldRange, selection)
    {
        var nestingLevel = 1;
        for (var i = oldRange.endLine; i >= 0; --i) {
            var attribute = this._textModel.getAttribute(i, "highlight");
            if (!attribute)
                continue;
            var columnNumbers = Object.keys(attribute).reverse();
            for (var j = 0; j < columnNumbers.length; ++j) {
                var column = columnNumbers[j];
                if (attribute[column].tokenType === "block-start") {
                    if (!(--nestingLevel)) {
                        var lineContent = this._textModel.line(i);
                        return lineContent.length - lineContent.trimLeft().length;
                    }
                }
                if (attribute[column].tokenType === "block-end")
                    ++nestingLevel;
            }
        }
        return -1;
    },

    /**
     * @param {WebInspector.TextRange} oldRange
     * @param {WebInspector.TextRange} newRange
     */
    textChanged: function(oldRange, newRange)
    {
        this.beginDomUpdates();
        this._removeDecorationsInRange(oldRange);
        this._updateChunksForRanges(oldRange, newRange);
        this._updateHighlightsForRange(newRange);
        this.endDomUpdates();
    },

    /**
     * @param {WebInspector.TextRange} range
     */
    _removeDecorationsInRange: function(range)
    {
        for (var i = this._chunkNumberForLine(range.startLine); i < this._textChunks.length; ++i) {
            var chunk = this._textChunks[i];
            if (chunk.startLine > range.endLine)
                break;
            chunk.removeAllDecorations();
        }
    },

    /**
     * @param {WebInspector.TextRange} oldRange
     * @param {WebInspector.TextRange} newRange
     */
    _updateChunksForRanges: function(oldRange, newRange)
    {
        var firstDamagedChunkNumber = this._chunkNumberForLine(oldRange.startLine);
        var lastDamagedChunkNumber = firstDamagedChunkNumber;
        while (lastDamagedChunkNumber + 1 < this._textChunks.length) {
            if (this._textChunks[lastDamagedChunkNumber + 1].startLine > oldRange.endLine)
                break;
            ++lastDamagedChunkNumber;
        }

        var firstDamagedChunk = this._textChunks[firstDamagedChunkNumber];
        var lastDamagedChunk = this._textChunks[lastDamagedChunkNumber];

        var linesDiff = newRange.linesCount - oldRange.linesCount;

        // First, detect chunks that have not been modified and simply shift them.
        if (linesDiff) {
            for (var chunkNumber = lastDamagedChunkNumber + 1; chunkNumber < this._textChunks.length; ++chunkNumber)
                this._textChunks[chunkNumber].startLine += linesDiff;
        }

        // Remove damaged chunks from DOM and from textChunks model.
        var lastUndamagedChunk = firstDamagedChunkNumber > 0 ? this._textChunks[firstDamagedChunkNumber - 1] : null;
        var firstUndamagedChunk = lastDamagedChunkNumber + 1 < this._textChunks.length ? this._textChunks[lastDamagedChunkNumber + 1] : null;

        var removeDOMFromNode = lastUndamagedChunk ? lastUndamagedChunk.lastElement().nextSibling : this._container.firstChild;
        var removeDOMToNode = firstUndamagedChunk ? firstUndamagedChunk.firstElement() : null;

        // Fast case - patch single expanded chunk that did not grow / shrink during edit.
        if (!linesDiff && firstDamagedChunk === lastDamagedChunk && firstDamagedChunk._expandedLineRows) {
            var lastUndamagedLineRow = lastDamagedChunk.expandedLineRow(oldRange.startLine - 1);
            var firstUndamagedLineRow = firstDamagedChunk.expandedLineRow(oldRange.endLine + 1);
            var localRemoveDOMFromNode = lastUndamagedLineRow ? lastUndamagedLineRow.nextSibling : removeDOMFromNode;
            var localRemoveDOMToNode = firstUndamagedLineRow || removeDOMToNode;
            removeSubsequentNodes(localRemoveDOMFromNode, localRemoveDOMToNode);
            for (var i = newRange.startLine; i < newRange.endLine + 1; ++i) {
                var row = firstDamagedChunk._createRow(i);
                firstDamagedChunk._expandedLineRows[i - firstDamagedChunk.startLine] = row;
                this._container.insertBefore(row, localRemoveDOMToNode);
            }
            firstDamagedChunk.updateCollapsedLineRow();
            this._assertDOMMatchesTextModel();
            return;
        }

        removeSubsequentNodes(removeDOMFromNode, removeDOMToNode);
        this._textChunks.splice(firstDamagedChunkNumber, lastDamagedChunkNumber - firstDamagedChunkNumber + 1);

        // Compute damaged chunks span
        var startLine = firstDamagedChunk.startLine;
        var endLine = lastDamagedChunk.endLine + linesDiff;
        var lineSpan = endLine - startLine;

        // Re-create chunks for damaged area.
        var insertionIndex = firstDamagedChunkNumber;
        var chunkSize = Math.ceil(lineSpan / Math.ceil(lineSpan / this._defaultChunkSize));

        for (var i = startLine; i < endLine; i += chunkSize) {
            var chunk = this._createNewChunk(i, Math.min(endLine, i + chunkSize));
            this._textChunks.splice(insertionIndex++, 0, chunk);
            this._container.insertBefore(chunk.element, removeDOMToNode);
        }

        this._assertDOMMatchesTextModel();
    },

    /**
     * @param {WebInspector.TextRange} range
     */
    _updateHighlightsForRange: function(range)
    {
        var visibleFrom = this._scrollTop();
        var visibleTo = visibleFrom + this._clientHeight();

        var result = this._findVisibleChunks(visibleFrom, visibleTo);
        var chunk = this._textChunks[result.end - 1];
        var lastVisibleLine = chunk.startLine + chunk.linesCount;

        lastVisibleLine = Math.max(lastVisibleLine, range.endLine + 1);
        lastVisibleLine = Math.min(lastVisibleLine, this._textModel.linesCount);

        var updated = this._highlighter.updateHighlight(range.startLine, lastVisibleLine);
        if (!updated) {
            // Highlights for the chunks below are invalid, so just collapse them.
            for (var i = this._chunkNumberForLine(range.startLine); i < this._textChunks.length; ++i)
                this._textChunks[i].expanded = false;
        }

        this._repaintAll();
    },

    /**
     * @param {Node} from
     * @param {Node} to
     * @return {Array.<string>}
     */
    _collectLinesFromDOM: function(from, to)
    {
        var textContents = [];
        var hasContent = false;
        for (var node = from ? from.nextSibling : this._container; node && node !== to; node = node.traverseNextNode(this._container)) {
            if (node._isDecorationsElement) {
                // Skip all children of the decoration container.
                node = node.nextSibling;
                if (!node || node === to)
                    break;
            }
            hasContent = true;
            if (node.nodeName.toLowerCase() === "br")
                textContents.push("\n");
            else if (node.nodeType === Node.TEXT_NODE)
                textContents.push(node.textContent);
        }
        if (!hasContent)
            return [];

        var textContent = textContents.join("");
        // The last \n (if any) does not "count" in a DIV.
        textContent = textContent.replace(/\n$/, "");

        return textContent.split("\n");
    },

    _handleSelectionChange: function(event)
    {
        var textRange = this._getSelection();
        if (textRange)
            this._lastSelection = textRange;
        this._delegate.selectionChanged(textRange);
    },

    __proto__: WebInspector.TextEditorChunkedPanel.prototype
}

/**
 * @constructor
 * @param {WebInspector.TextEditorChunkedPanel} chunkedPanel
 * @param {number} startLine
 * @param {number} endLine
 */
WebInspector.TextEditorMainChunk = function(chunkedPanel, startLine, endLine)
{
    this._chunkedPanel = chunkedPanel;
    this._textModel = chunkedPanel._textModel;

    this.element = document.createElement("div");
    this.element.lineNumber = startLine;
    this.element.className = "webkit-line-content";
    this.element._chunk = this;

    this._startLine = startLine;
    endLine = Math.min(this._textModel.linesCount, endLine);
    this.linesCount = endLine - startLine;

    this._expanded = false;

    this.updateCollapsedLineRow();
}

WebInspector.TextEditorMainChunk.prototype = {
    addDecoration: function(decoration)
    {
        this._chunkedPanel.beginDomUpdates();
        if (typeof decoration === "string")
            this.element.addStyleClass(decoration);
        else {
            if (!this.element.decorationsElement) {
                this.element.decorationsElement = document.createElement("div");
                this.element.decorationsElement.className = "webkit-line-decorations";
                this.element.decorationsElement._isDecorationsElement = true;
                this.element.appendChild(this.element.decorationsElement);
            }
            this.element.decorationsElement.appendChild(decoration);
        }
        this._chunkedPanel.endDomUpdates();
    },

    /**
     * @param {string|Element} decoration
     */
    removeDecoration: function(decoration)
    {
        this._chunkedPanel.beginDomUpdates();
        if (typeof decoration === "string")
            this.element.removeStyleClass(decoration);
        else if (this.element.decorationsElement)
            this.element.decorationsElement.removeChild(decoration);
        this._chunkedPanel.endDomUpdates();
    },

    removeAllDecorations: function()
    {
        this._chunkedPanel.beginDomUpdates();
        this.element.className = "webkit-line-content";
        if (this.element.decorationsElement) {
            this.element.removeChild(this.element.decorationsElement);
            delete this.element.decorationsElement;
        }
        this._chunkedPanel.endDomUpdates();
    },

    /**
     * @return {boolean}
     */
    isDecorated: function()
    {
        return this.element.className !== "webkit-line-content" || !!(this.element.decorationsElement && this.element.decorationsElement.firstChild);
    },

    /**
     * @return {number}
     */
    get startLine()
    {
        return this._startLine;
    },

    /**
     * @return {number}
     */
    get endLine()
    {
        return this._startLine + this.linesCount;
    },

    set startLine(startLine)
    {
        this._startLine = startLine;
        this.element.lineNumber = startLine;
        if (this._expandedLineRows) {
            for (var i = 0; i < this._expandedLineRows.length; ++i)
                this._expandedLineRows[i].lineNumber = startLine + i;
        }
    },

    /**
     * @return {boolean}
     */
    get expanded()
    {
        return this._expanded;
    },

    set expanded(expanded)
    {
        if (this._expanded === expanded)
            return;

        this._expanded = expanded;

        if (this.linesCount === 1) {
            if (expanded)
                this._chunkedPanel._paintLine(this.element);
            return;
        }

        this._chunkedPanel.beginDomUpdates();

        if (expanded) {
            this._expandedLineRows = [];
            var parentElement = this.element.parentElement;
            for (var i = this.startLine; i < this.startLine + this.linesCount; ++i) {
                var lineRow = this._createRow(i);
                parentElement.insertBefore(lineRow, this.element);
                this._expandedLineRows.push(lineRow);
            }
            parentElement.removeChild(this.element);
            this._chunkedPanel._paintLines(this.startLine, this.startLine + this.linesCount);
        } else {
            var elementInserted = false;
            for (var i = 0; i < this._expandedLineRows.length; ++i) {
                var lineRow = this._expandedLineRows[i];
                var parentElement = lineRow.parentElement;
                if (parentElement) {
                    if (!elementInserted) {
                        elementInserted = true;
                        parentElement.insertBefore(this.element, lineRow);
                    }
                    parentElement.removeChild(lineRow);
                }
                this._chunkedPanel._releaseLinesHighlight(lineRow);
            }
            delete this._expandedLineRows;
        }

        this._chunkedPanel.endDomUpdates();
    },

    /**
     * @return {number}
     */
    get height()
    {
        if (!this._expandedLineRows)
            return this._chunkedPanel._totalHeight(this.element);
        return this._chunkedPanel._totalHeight(this._expandedLineRows[0], this._expandedLineRows[this._expandedLineRows.length - 1]);
    },

    /**
     * @return {number}
     */
    get offsetTop()
    {
        return (this._expandedLineRows && this._expandedLineRows.length) ? this._expandedLineRows[0].offsetTop : this.element.offsetTop;
    },

    /**
     * @param {number} lineNumber
     * @return {Element}
     */
    _createRow: function(lineNumber)
    {
        var lineRow = this._chunkedPanel._cachedRows.pop() || document.createElement("div");
        lineRow.lineNumber = lineNumber;
        lineRow.className = "webkit-line-content";
        lineRow.textContent = this._textModel.line(lineNumber);
        if (!lineRow.textContent)
            lineRow.appendChild(document.createElement("br"));
        return lineRow;
    },

    /**
     * Called on potentially damaged / inconsistent chunk
     * @param {number} lineNumber
     * @return {?Node}
     */
    lineRowContainingLine: function(lineNumber)
    {
        if (!this._expanded)
            return this.element;
        return this.expandedLineRow(lineNumber);
    },

    /**
     * @param {number} lineNumber
     * @return {Element}
     */
    expandedLineRow: function(lineNumber)
    {
        if (!this._expanded || lineNumber < this.startLine || lineNumber >= this.startLine + this.linesCount)
            return null;
        if (!this._expandedLineRows)
            return this.element;
        return this._expandedLineRows[lineNumber - this.startLine];
    },

    updateCollapsedLineRow: function()
    {
        if (this.linesCount === 1 && this._expanded)
            return;

        var lines = [];
        for (var i = this.startLine; i < this.startLine + this.linesCount; ++i)
            lines.push(this._textModel.line(i));

        if (WebInspector.FALSE)
            console.log("Rebuilding chunk with " + lines.length + " lines");

        this.element.removeChildren();
        this.element.textContent = lines.join("\n");
        // The last empty line will get swallowed otherwise.
        if (!lines[lines.length - 1])
            this.element.appendChild(document.createElement("br"));
    },

    firstElement: function()
    {
        return this._expandedLineRows ? this._expandedLineRows[0] : this.element;
    },

    lastElement: function()
    {
        return this._expandedLineRows ? this._expandedLineRows[this._expandedLineRows.length - 1] : this.element;
    }
}

WebInspector.debugDefaultTextEditor = false;
