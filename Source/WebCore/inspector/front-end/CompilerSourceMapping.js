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
 */
WebInspector.CompilerSourceMapping = function()
{
}

WebInspector.CompilerSourceMapping.prototype = {
    compiledLocationToSourceLocation: function(lineNumber, columnNumber)
    {
        // Should be implemented by subclasses.
    },

    sourceLocationToCompiledLocation: function(sourceURL, lineNumber, columnNumber)
    {
        // Should be implemented by subclasses.
    },

    sources: function()
    {
        // Should be implemented by subclasses.
    }
}

/**
 * @constructor
 */
WebInspector.ClosureCompilerSourceMappingPayload = function()
{
    this.mappings = "";
    this.sources = [];
}

/**
 * Implements Source Map V3 consumer. See http://code.google.com/p/closure-compiler/wiki/SourceMaps
 * for format description.
 * @extends {WebInspector.CompilerSourceMapping}
 * @constructor
 * @param {WebInspector.ClosureCompilerSourceMappingPayload} mappingPayload
 */
WebInspector.ClosureCompilerSourceMapping = function(mappingPayload)
{
    if (!WebInspector.ClosureCompilerSourceMapping.prototype._base64Map) {
        const base64Digits = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        WebInspector.ClosureCompilerSourceMapping.prototype._base64Map = {};
        for (var i = 0; i < base64Digits.length; ++i)
            WebInspector.ClosureCompilerSourceMapping.prototype._base64Map[base64Digits.charAt(i)] = i;
    }

    this._sources = mappingPayload.sources;
    this._mappings = [];
    this._reverseMappingsBySourceURL = {};
    for (var i = 0; i < this._sources.length; ++i)
        this._reverseMappingsBySourceURL[this._sources[i]] = [];
    this._parseMappings(mappingPayload);
}

WebInspector.ClosureCompilerSourceMapping.prototype = {
    compiledLocationToSourceLocation: function(lineNumber, columnNumber)
    {
        var mapping = this._findMapping(lineNumber, columnNumber);
        return { sourceURL: mapping[2], lineNumber: mapping[3], columnNumber: mapping[4] };
    },

    sourceLocationToCompiledLocation: function(sourceURL, lineNumber)
    {
        var mappings = this._reverseMappingsBySourceURL[sourceURL];
        for ( ; lineNumber < mappings.length; ++lineNumber) {
            var mapping = mappings[lineNumber];
            if (mapping)
                return { lineNumber: mapping[0], columnNumber: mapping[1] };
        }
    },

    sources: function()
    {
        return this._sources;
    },

    _findMapping: function(lineNumber, columnNumber)
    {
        var first = 0;
        var count = this._mappings.length;
        while (count > 1) {
          var step = count >> 1;
          var middle = first + step;
          var mapping = this._mappings[middle];
          if (lineNumber < mapping[0] || (lineNumber == mapping[0] && columnNumber < mapping[1]))
              count = step;
          else {
              first = middle;
              count -= step;
          }
        }
        return this._mappings[first];
    },

    _parseMappings: function(mappingPayload)
    {
        var stringCharIterator = new WebInspector.ClosureCompilerSourceMapping.StringCharIterator(mappingPayload.mappings);

        var lineNumber = 0;
        var columnNumber = 0;
        var sourceIndex = 0;
        var sourceLineNumber = 0;
        var sourceColumnNumber = 0;
        var nameIndex = 0;

        var sourceURL = this._sources[0];
        var reverseMappings = this._reverseMappingsBySourceURL[sourceURL];

        do {
            columnNumber += this._decodeVLQ(stringCharIterator);
            if (this._isSeparator(stringCharIterator.peek()))
                continue;
            var sourceIndexDelta = this._decodeVLQ(stringCharIterator);
            if (sourceIndexDelta) {
                sourceIndex += sourceIndexDelta;
                sourceURL = this._sources[sourceIndex];
                reverseMappings = this._reverseMappingsBySourceURL[sourceURL];
            }
            sourceLineNumber += this._decodeVLQ(stringCharIterator);
            sourceColumnNumber += this._decodeVLQ(stringCharIterator);
            if (!this._isSeparator(stringCharIterator.peek()))
                nameIndex += this._decodeVLQ(stringCharIterator);

            this._mappings.push([lineNumber, columnNumber, sourceURL, sourceLineNumber, sourceColumnNumber]);
            if (!reverseMappings[sourceLineNumber])
                reverseMappings[sourceLineNumber] = [lineNumber, columnNumber];

            if (stringCharIterator.next() === ";") {
                lineNumber += 1;
                columnNumber = 0;
            }
        } while(stringCharIterator.hasNext());
    },

    _isSeparator: function(char)
    {
        return char === "," || char === ";";
    },

    _decodeVLQ: function(stringCharIterator)
    {
        // Read unsigned value.
        var result = 0;
        var shift = 0;
        do {
            var digit = this._base64Map[stringCharIterator.next()];
            result += (digit & this._VLQ_BASE_MASK) << shift;
            shift += this._VLQ_BASE_SHIFT;
        } while (digit & this._VLQ_CONTINUATION_MASK);

        // Fix the sign.
        var negative = result & 1;
        result >>= 1;
        return negative ? -result : result;
    },

    _VLQ_BASE_SHIFT: 5,
    _VLQ_BASE_MASK: (1 << 5) - 1,
    _VLQ_CONTINUATION_MASK: 1 << 5
}

/**
 * @constructor
 */
WebInspector.ClosureCompilerSourceMapping.StringCharIterator = function(string)
{
    this._string = string;
    this._position = 0;
}

WebInspector.ClosureCompilerSourceMapping.StringCharIterator.prototype = {
    next: function()
    {
        return this._string.charAt(this._position++);
    },

    peek: function()
    {
        return this._string.charAt(this._position);
    },

    hasNext: function()
    {
        return this._position < this._string.length;
    }
}
