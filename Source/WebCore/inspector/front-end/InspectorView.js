/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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
 */
WebInspector.InspectorView = function()
{
    WebInspector.View.call(this);
    this.markAsRoot();
    this.element.id = "main-panels";
    this.element.setAttribute("spellcheck", false);
    this._history = [];
    this._historyIterator = -1;
    document.addEventListener("keydown", this._keyDown.bind(this), false);
    document.addEventListener("keypress", this._keyPress.bind(this), false);
    this._panelOrder = [];

    // Windows and Mac have two different definitions of '[', so accept both.
    this._openBracketIdentifiers = ["U+005B", "U+00DB"].keySet();
    this._openBracketCharCode = "[".charCodeAt(0);

    // Windows and Mac have two different definitions of ']', so accept both.
    this._closeBracketIdentifiers = ["U+005D", "U+00DD"].keySet();
    this._closeBracketCharCode = "]".charCodeAt(0);
}

WebInspector.InspectorView.Events = {
    PanelSelected: "panel-selected"
}

WebInspector.InspectorView.prototype = {
    addPanel: function(panel)
    {
        this._panelOrder.push(panel);
        WebInspector.toolbar.addPanel(panel);
    },

    currentPanel: function()
    {
        return this._currentPanel;
    },

    setCurrentPanel: function(x)
    {
        if (this._currentPanel === x)
            return;

        if (this._currentPanel)
            this._currentPanel.detach();

        this._currentPanel = x;

        if (x) {
            x.show();
            this.dispatchEventToListeners(WebInspector.InspectorView.Events.PanelSelected);
            // FIXME: remove search controller.
            WebInspector.searchController.activePanelChanged();
        }
        for (var panelName in WebInspector.panels) {
            if (WebInspector.panels[panelName] === x) {
                WebInspector.settings.lastActivePanel.set(panelName);
                this._pushToHistory(panelName);
                WebInspector.userMetrics.panelShown(panelName);
            }
        }
    },

    _keyPress: function(event)
    {
        if (!this._keyDownTimer)
            return;

        if (event.charCode === this._openBracketCharCode || event.charCode === this._closeBracketCharCode) {
            clearTimeout(this._keyDownTimer);
            delete this._keyDownTimer;
        }
    },

    _keyDown: function(event)
    {
        // BUG85312: On French AZERTY keyboards, AltGr-]/[ combinations (synonymous to Ctrl-Alt-]/[ on Windows) are used to enter ]/[,
        // so for a ]/[-related keydown we delay the panel switch using a timer, to see if there is a keypress event following this one.
        // If there is, we cancel the timer and do not consider this a panel switch.
        if (!WebInspector.isWin() || (!this._openBracketIdentifiers.hasOwnProperty(event.keyIdentifier) && !this._closeBracketIdentifiers.hasOwnProperty(event.keyIdentifier))) {
            this._keyDownInternal(event);
            return;
        }

        this._keyDownTimer = setTimeout(this._keyDownInternal.bind(this, event), 0);
    },

    _keyDownInternal: function(event)
    {
        if (this._openBracketIdentifiers.hasOwnProperty(event.keyIdentifier)) {
            var isRotateLeft = WebInspector.KeyboardShortcut.eventHasCtrlOrMeta(event) && !event.shiftKey && !event.altKey;
            if (isRotateLeft) {
                var index = this._panelOrder.indexOf(this.currentPanel());
                index = (index === 0) ? this._panelOrder.length - 1 : index - 1;
                this._panelOrder[index].toolbarItem.click();
                event.consume(true);
                return;
            }

            var isGoBack = WebInspector.KeyboardShortcut.eventHasCtrlOrMeta(event) && event.altKey;
            if (isGoBack && this._canGoBackInHistory()) {
                this._goBackInHistory();
                event.consume(true);
            }
            return;
        }

        if (this._closeBracketIdentifiers.hasOwnProperty(event.keyIdentifier)) {
            var isRotateRight = WebInspector.KeyboardShortcut.eventHasCtrlOrMeta(event) && !event.shiftKey && !event.altKey;
            if (isRotateRight) {
                var index = this._panelOrder.indexOf(this.currentPanel());
                index = (index + 1) % this._panelOrder.length;
                this._panelOrder[index].toolbarItem.click();
                event.consume(true);
                return;
            }

            var isGoForward = WebInspector.KeyboardShortcut.eventHasCtrlOrMeta(event) && event.altKey;
            if (isGoForward && this._canGoForwardInHistory()) {
                this._goForwardInHistory();
                event.consume(true);
            }
            return;
        }
    },

    _canGoBackInHistory: function()
    {
        return this._historyIterator > 0;
    },

    _goBackInHistory: function()
    {
        this._inHistory = true;
        this.setCurrentPanel(WebInspector.panels[this._history[--this._historyIterator]]);
        delete this._inHistory;
    },

    _canGoForwardInHistory: function()
    {
        return this._historyIterator < this._history.length - 1;
    },

    _goForwardInHistory: function()
    {
        this._inHistory = true;
        this.setCurrentPanel(WebInspector.panels[this._history[++this._historyIterator]]);
        delete this._inHistory;
    },

    _pushToHistory: function(panelName)
    {
        if (this._inHistory)
            return;

        this._history.splice(this._historyIterator + 1, this._history.length - this._historyIterator - 1);
        if (!this._history.length || this._history[this._history.length - 1] !== panelName)
            this._history.push(panelName);
        this._historyIterator = this._history.length - 1;
    }
}

WebInspector.InspectorView.prototype.__proto__ = WebInspector.View.prototype;

/**
 * @type {WebInspector.InspectorView}
 */
WebInspector.inspectorView = null;
