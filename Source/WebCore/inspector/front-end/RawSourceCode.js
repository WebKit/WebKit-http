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

// RawSourceCode represents JavaScript resource or HTML resource with inlined scripts
// as it came from network.

/**
 * @constructor
 * @extends {WebInspector.Object}
 */
WebInspector.RawSourceCode = function(id, script, resource, formatter, formatted)
{
    this.id = id;
    this.url = script.sourceURL;
    this.isContentScript = script.isContentScript;
    this._scripts = [script];
    this._formatter = formatter;
    this._formatted = formatted;
    this._resource = resource;
    this.messages = [];

    this._useTemporaryContent = this._resource && !this._resource.finished;
    this._hasNewScripts = true;
    if (!this._useTemporaryContent)
        this._updateSourceMapping();
    else if (this._resource)
        this._resource.addEventListener("finished", this._resourceFinished.bind(this));
}

WebInspector.RawSourceCode.Events = {
    SourceMappingUpdated: "source-mapping-updated"
}

WebInspector.RawSourceCode.prototype = {
    addScript: function(script)
    {
        this._scripts.push(script);
        this._hasNewScripts = true;
    },

    get sourceMapping()
    {
        return this._sourceMapping;
    },

    setFormatted: function(formatted)
    {
        if (this._formatted === formatted)
            return;
        this._formatted = formatted;
        this._updateSourceMapping();
    },

    setCompilerSourceMappingProvider: function(provider)
    {
        if (provider)
            this._useTemporaryContent = false;
        this._compilerSourceMappingProvider = provider;
        this._updateSourceMapping();
    },

    contentEdited: function()
    {
        this._updateSourceMapping();
    },

    _resourceFinished: function()
    {
        if (this._compilerSourceMappingProvider)
            return;
        this._useTemporaryContent = false;
        this._updateSourceMapping();
    },

    _scriptForRawLocation: function(lineNumber, columnNumber)
    {
        var closestScript = this._scripts[0];
        for (var i = 1; i < this._scripts.length; ++i) {
            var script = this._scripts[i];
            if (script.lineOffset > lineNumber || (script.lineOffset === lineNumber && script.columnOffset > columnNumber))
                continue;
            if (script.lineOffset > closestScript.lineOffset ||
                (script.lineOffset === closestScript.lineOffset && script.columnOffset > closestScript.columnOffset))
                closestScript = script;
        }
        return closestScript;
    },

    forceUpdateSourceMapping: function(script)
    {
        if (!this._useTemporaryContent || !this._hasNewScripts)
            return;
        this._hasNewScripts = false;
        this._updateSourceMapping();
    },

    _updateSourceMapping: function()
    {
        if (this._updatingSourceMapping) {
            this._updateNeeded = true;
            return;
        }
        this._updatingSourceMapping = true;
        this._updateNeeded = false;

        this._createSourceMapping(didCreateSourceMapping.bind(this));

        function didCreateSourceMapping(sourceMapping)
        {
            this._updatingSourceMapping = false;
            if (!this._updateNeeded)
                this._saveSourceMapping(sourceMapping);
            else
                this._updateSourceMapping();
        }
    },

    _createContentProvider: function()
    {
        if (this._resource && this._resource.finished)
            return new WebInspector.ResourceContentProvider(this._resource);
        if (this._scripts.length === 1 && !this._scripts[0].lineOffset && !this._scripts[0].columnOffset)
            return new WebInspector.ScriptContentProvider(this._scripts[0]);
        return new WebInspector.ConcatenatedScriptsContentProvider(this._scripts);
    },

    _createSourceMapping: function(callback)
    {
        if (this._compilerSourceMappingProvider) {
            function didLoadSourceMapping(compilerSourceMapping)
            {
                var uiSourceCodeList = [];
                var sourceURLs = compilerSourceMapping.sources();
                for (var i = 0; i < sourceURLs.length; ++i) {
                    var sourceURL = sourceURLs[i];
                    var contentProvider = new WebInspector.CompilerSourceMappingContentProvider(sourceURL, this._compilerSourceMappingProvider);
                    var uiSourceCode = new WebInspector.UISourceCode(sourceURL, sourceURL, this.isContentScript, this, contentProvider);
                    uiSourceCodeList.push(uiSourceCode);
                }
                var sourceMapping = new WebInspector.RawSourceCode.CompilerSourceMapping(this, uiSourceCodeList, compilerSourceMapping);
                callback(sourceMapping);
            }
            this._compilerSourceMappingProvider.loadSourceMapping(didLoadSourceMapping.bind(this));
            return;
        }

        var originalContentProvider = this._createContentProvider();
        if (!this._formatted) {
            var uiSourceCode = new WebInspector.UISourceCode(this.id, this.url, this.isContentScript, this, originalContentProvider);
            var sourceMapping = new WebInspector.RawSourceCode.PlainSourceMapping(this, uiSourceCode);
            callback(sourceMapping);
            return;
        }

        function didRequestContent(mimeType, content)
        {
            function didFormatContent(formattedContent, mapping)
            {
                var contentProvider = new WebInspector.StaticContentProvider(mimeType, formattedContent)
                var uiSourceCode = new WebInspector.UISourceCode("deobfuscated:" + this.id, this.url, this.isContentScript, this, contentProvider);
                var sourceMapping = new WebInspector.RawSourceCode.FormattedSourceMapping(this, uiSourceCode, mapping);
                callback(sourceMapping);
            }
            this._formatter.formatContent(mimeType, content, didFormatContent.bind(this));
        }
        originalContentProvider.requestContent(didRequestContent.bind(this));
    },

    _saveSourceMapping: function(sourceMapping)
    {
        var oldUISourceCode;
        if (this._sourceMapping)
            oldUISourceCode = this._sourceMapping.uiSourceCode;
        this._sourceMapping = sourceMapping;
        this.dispatchEventToListeners(WebInspector.RawSourceCode.Events.SourceMappingUpdated, { oldUISourceCode: oldUISourceCode });
    }
}

WebInspector.RawSourceCode.prototype.__proto__ = WebInspector.Object.prototype;


/**
 * @constructor
 */
WebInspector.RawSourceCode.PlainSourceMapping = function(rawSourceCode, uiSourceCode)
{
    this._rawSourceCode = rawSourceCode;
    this._uiSourceCode = uiSourceCode;
}

WebInspector.RawSourceCode.PlainSourceMapping.prototype = {
    rawLocationToUILocation: function(rawLocation)
    {
        return new WebInspector.UILocation(this._uiSourceCode, rawLocation.lineNumber, rawLocation.columnNumber);
    },

    uiLocationToRawLocation: function(uiSourceCode, lineNumber)
    {
        console.assert(uiSourceCode === this._uiSourceCode);
        var rawLocation = { lineNumber: lineNumber, columnNumber: 0 };
        rawLocation.scriptId = this._rawSourceCode._scriptForRawLocation(rawLocation.lineNumber, rawLocation.columnNumber).scriptId;
        return rawLocation;
    },

    get uiSourceCode()
    {
        return this._uiSourceCode;
    }
}

/**
 * @constructor
 */
WebInspector.RawSourceCode.FormattedSourceMapping = function(rawSourceCode, uiSourceCode, mapping)
{
    this._rawSourceCode = rawSourceCode;
    this._uiSourceCode = uiSourceCode;
    this._mapping = mapping;
}

WebInspector.RawSourceCode.FormattedSourceMapping.prototype = {
    rawLocationToUILocation: function(rawLocation)
    {
        var location = this._mapping.originalToFormatted(rawLocation);
        return new WebInspector.UILocation(this._uiSourceCode, location.lineNumber, location.columnNumber);
    },

    uiLocationToRawLocation: function(uiSourceCode, lineNumber)
    {
        console.assert(uiSourceCode === this._uiSourceCode);
        var rawLocation = this._mapping.formattedToOriginal({ lineNumber: lineNumber, columnNumber: 0 });
        rawLocation.scriptId = this._rawSourceCode._scriptForRawLocation(rawLocation.lineNumber, rawLocation.columnNumber).scriptId;
        return rawLocation;
    },

    get uiSourceCode()
    {
        return this._uiSourceCode;
    }
}

/**
 * @constructor
 */
WebInspector.RawSourceCode.CompilerSourceMapping = function(rawSourceCode, uiSourceCodeList, mapping)
{
    this._rawSourceCode = rawSourceCode;
    this._uiSourceCodeList = uiSourceCodeList;
    this._mapping = mapping;
    this._uiSourceCodeByURL = {};
    for (var i = 0; i < uiSourceCodeList.length; ++i)
        this._uiSourceCodeByURL[uiSourceCodeList[i].url] = uiSourceCodeList[i];
}

WebInspector.RawSourceCode.CompilerSourceMapping.prototype = {
    rawLocationToUILocation: function(rawLocation)
    {
        var location = this._mapping.compiledLocationToSourceLocation(rawLocation.lineNumber, rawLocation.columnNumber);
        var uiSourceCode = this._uiSourceCodeByURL[location.sourceURL];
        return new WebInspector.UILocation(uiSourceCode, location.lineNumber, location.columnNumber);
    },

    uiLocationToRawLocation: function(uiSourceCode, lineNumber)
    {
        var rawLocation = this._mapping.sourceLocationToCompiledLocation(uiSourceCode.url, lineNumber);
        rawLocation.scriptId = this._rawSourceCode._scriptForRawLocation(rawLocation.lineNumber, rawLocation.columnNumber).scriptId;
        return rawLocation;
    },

    get uiSourceCodeList()
    {
        return this._uiSourceCodeList;
    }
}

/**
 * @constructor
 */
WebInspector.UILocation = function(uiSourceCode, lineNumber, columnNumber)
{
    this.uiSourceCode = uiSourceCode;
    this.lineNumber = lineNumber;
    this.columnNumber = columnNumber;
}
