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
 * @param {string} src
 * @param {string=} className
 */
WebInspector.IFrameView = function(src, className)
{
    WebInspector.View.call(this);
    this.element.className = "fill";
    this._src = src;
    this._className = className;
}

WebInspector.IFrameView.prototype = {
    willDetach: function()
    {
        if (this._iframe) {
            if (!WebInspector.IFrameView._parentIframe) {
                WebInspector.IFrameView._parentIframe = document.createElement("iframe");
                WebInspector.IFrameView._parentIframe.style.display = "none";
                document.body.appendChild(WebInspector.IFrameView._parentIframe);
            }
            WebInspector.IFrameView._parentIframe.contentDocument.adoptNode(this._iframe);
            WebInspector.IFrameView._parentIframe.contentDocument.body.appendChild(this._iframe);
        }
    },

    onInsertedIntoDocument: function()
    {
        if (!this._iframe) {
            this._iframe = document.createElement("iframe");
            this._iframe.src = this._src;
            if (this._className)
                this._iframe.className = this._className;
        }

        document.adoptNode(this._iframe);
        this.element.appendChild(this._iframe);
    }
}

WebInspector.IFrameView.prototype.__proto__ = WebInspector.View.prototype;
