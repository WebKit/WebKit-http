/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
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
 * @param {number} startLine
 * @param {number} startColumn
 * @param {number} endLine
 * @param {number} endColumn
 */
WebInspector.TextRange = function(startLine, startColumn, endLine, endColumn)
{
    this.startLine = startLine;
    this.startColumn = startColumn;
    this.endLine = endLine;
    this.endColumn = endColumn;
}

WebInspector.TextRange.createFromLocation = function(line, column)
{
    return new WebInspector.TextRange(line, column, line, column);
}

/**
 * @param {Object} serializedTextRange
 * @return {WebInspector.TextRange}
 */
WebInspector.TextRange.fromObject = function (serializedTextRange)
{
    return new WebInspector.TextRange(serializedTextRange.startLine, serializedTextRange.startColumn, serializedTextRange.endLine, serializedTextRange.endColumn);
}

WebInspector.TextRange.prototype = {
    /**
     * @return {boolean}
     */
    isEmpty: function()
    {
        return this.startLine === this.endLine && this.startColumn === this.endColumn;
    },

    /**
     * @return {number}
     */
    get linesCount()
    {
        return this.endLine - this.startLine;
    },

    collapseToEnd: function()
    {
        return new WebInspector.TextRange(this.endLine, this.endColumn, this.endLine, this.endColumn);
    },

    /**
     * @return {WebInspector.TextRange}
     */
    normalize: function()
    {
        if (this.startLine > this.endLine || (this.startLine === this.endLine && this.startColumn > this.endColumn))
            return new WebInspector.TextRange(this.endLine, this.endColumn, this.startLine, this.startColumn);
        else
            return this.clone();
    },

    /**
     * @return {WebInspector.TextRange}
     */
    clone: function()
    {
        return new WebInspector.TextRange(this.startLine, this.startColumn, this.endLine, this.endColumn);
    },

    /**
     * @return {Object}
     */
    serializeToObject: function()
    {
        var serializedTextRange = {};
        serializedTextRange.startLine = this.startLine;
        serializedTextRange.startColumn = this.startColumn;
        serializedTextRange.endLine = this.endLine;
        serializedTextRange.endColumn = this.endColumn;
        return serializedTextRange;
    },

    /**
     * @param {WebInspector.TextRange} other
     * @return {number}
     */
    compareTo: function(other)
    {
        if (this.startLine > other.startLine)
            return 1;
        if (this.startLine < other.startLine)
            return -1;
        if (this.startColumn > other.startColumn)
            return 1;
        if (this.startColumn < other.startColumn)
            return -1;
        return 0;
    },

    /**
     * @param {number} lineOffset
     * @return {WebInspector.TextRange}
     */
    shift: function(lineOffset)
    {
        return new WebInspector.TextRange(this.startLine + lineOffset, this.startColumn, this.endLine + lineOffset, this.endColumn);
    },

    toString: function()
    {
        return JSON.stringify(this);
    }
}

/**
 * @constructor
 * @param {WebInspector.TextRange} newRange
 * @param {string} originalText
 */
WebInspector.TextEditorCommand = function(newRange, originalText)
{
    this.newRange = newRange;
    this.originalText = originalText;
}

/**
 * @constructor
 * @extends {WebInspector.Object}
 */
WebInspector.TextEditorModel = function()
{
    this._lines = [""];
    this._attributes = [];
    /** @type {Array.<WebInspector.TextEditorCommand>} */
    this._undoStack = [];
    this._noPunctuationRegex = /[^ !%&()*+,-.:;<=>?\[\]\^{|}~]+/;
    this._lineBreak = "\n";
}

WebInspector.TextEditorModel.Indent = {
    TwoSpaces: "  ",
    FourSpaces: "    ",
    EightSpaces: "        ",
    TabCharacter: "\t"
}

WebInspector.TextEditorModel.Events = {
    TextChanged: "TextChanged"
}

WebInspector.TextEditorModel.endsWithBracketRegex = /[{(\[]\s*$/;

WebInspector.TextEditorModel.prototype = {
    /**
     * @return {number}
     */
    get linesCount()
    {
        return this._lines.length;
    },

    /**
     * @return {string}
     */
    text: function()
    {
        return this._lines.join(this._lineBreak);
    },

    /**
     * @return {WebInspector.TextRange}
     */
    range: function()
    {
        return new WebInspector.TextRange(0, 0, this._lines.length - 1, this._lines[this._lines.length - 1].length);
    },

    /**
     * @return {string}
     */
    get lineBreak()
    {
        return this._lineBreak;
    },

    /**
     * @param {number} lineNumber
     * @return {string}
     */
    line: function(lineNumber)
    {
        if (lineNumber >= this._lines.length)
            throw "Out of bounds:" + lineNumber;
        return this._lines[lineNumber];
    },

    /**
     * @param {number} lineNumber
     * @return {number}
     */
    lineLength: function(lineNumber)
    {
        return this._lines[lineNumber].length;
    },

    /**
     * @param {string} text 
     */
    setText: function(text)
    {
        this._resetUndoStack();
        text = text || "";
        var range = this.range();
        this._lineBreak = /\r\n/.test(text) ? "\r\n" : "\n";
        var newRange = this._innerSetText(range, text);
        this.dispatchEventToListeners(WebInspector.TextEditorModel.Events.TextChanged, { oldRange: range, newRange: newRange});
    },

    /**
     * @param {WebInspector.TextRange} range
     * @param {string} text
     * @return {WebInspector.TextRange}
     */
    editRange: function(range, text)
    {   
        if (this._lastEditedRange && (!text || text.indexOf("\n") !== -1 || this._lastEditedRange.endLine !== range.startLine || this._lastEditedRange.endColumn !== range.startColumn))
            this._markUndoableState();
        return this._innerEditRange(range, text);
    },

    /**
     * @param {WebInspector.TextRange} range
     * @param {string} text
     * @return {WebInspector.TextRange}
     */
    _innerEditRange: function(range, text)
    {
        var originalText = this.copyRange(range);
        this._lastEditedRange = range;
        var newRange = range;
        if (text !== originalText) {
            newRange = this._innerSetText(range, text);
            this._pushUndoableCommand(newRange, originalText);
        }

        this.dispatchEventToListeners(WebInspector.TextEditorModel.Events.TextChanged, { oldRange: range, newRange: newRange, editRange: true });
        return newRange;
    },

    /**
     * @param {WebInspector.TextRange} range
     * @param {string} text
     * @return {WebInspector.TextRange}
     */
    _innerSetText: function(range, text)
    {
        this._eraseRange(range);
        if (text === "")
            return new WebInspector.TextRange(range.startLine, range.startColumn, range.startLine, range.startColumn);

        var newLines = text.split(/\r?\n/);

        var prefix = this._lines[range.startLine].substring(0, range.startColumn);
        var suffix = this._lines[range.startLine].substring(range.startColumn);

        var postCaret = prefix.length;
        // Insert text.
        if (newLines.length === 1) {
            this._setLine(range.startLine, prefix + newLines[0] + suffix);
            postCaret += newLines[0].length;
        } else {
            this._setLine(range.startLine, prefix + newLines[0]);
            this._insertLines(range, newLines);
            this._setLine(range.startLine + newLines.length - 1, newLines[newLines.length - 1] + suffix);
            postCaret = newLines[newLines.length - 1].length;
        }

        return new WebInspector.TextRange(range.startLine, range.startColumn,
                                          range.startLine + newLines.length - 1, postCaret);
    },

    /**
     * @param {WebInspector.TextRange} range
     * @param {Array.<string>} newLines
     */
    _insertLines: function(range, newLines)
    {
        var lines = new Array(this._lines.length + newLines.length - 1);
        for (var i = 0; i <= range.startLine; ++i)
            lines[i] = this._lines[i];
        // Line at [0] is already set via setLine.
        for (var i = 1; i < newLines.length; ++i)
            lines[range.startLine + i] = newLines[i];
        for (var i = range.startLine + newLines.length; i < lines.length; ++i)
            lines[i] = this._lines[i - newLines.length + 1];
        this._lines = lines;

        // Adjust attributes, attributes move with the first character of line.
        var attributes = new Array(lines.length);
        var insertionIndex = range.startColumn ? range.startLine + 1 : range.startLine;
        for (var i = 0; i < insertionIndex; ++i)
            attributes[i] = this._attributes[i];
        for (var i = insertionIndex + newLines.length - 1; i < attributes.length; ++i)
            attributes[i] = this._attributes[i - newLines.length + 1];
        this._attributes = attributes;
    },

    /**
     * @param {WebInspector.TextRange} range
     */
    _eraseRange: function(range)
    {
        if (range.isEmpty())
            return;

        var prefix = this._lines[range.startLine].substring(0, range.startColumn);
        var suffix = this._lines[range.endLine].substring(range.endColumn);

        if (range.endLine > range.startLine) {
            this._lines.splice(range.startLine + 1, range.endLine - range.startLine);
            // Adjust attributes, attributes move with the first character of line.
            this._attributes.splice(range.startColumn ? range.startLine + 1 : range.startLine, range.endLine - range.startLine);
        }
        this._setLine(range.startLine, prefix + suffix);
    },

    /**
     * @param {number} lineNumber
     * @param {string} text
     */
    _setLine: function(lineNumber, text)
    {
        this._lines[lineNumber] = text;
    },

    /**
     * @param {number} lineNumber
     * @param {number} column
     * @return {WebInspector.TextRange}
     */
    wordRange: function(lineNumber, column)
    {
        return new WebInspector.TextRange(lineNumber, this.wordStart(lineNumber, column, true), lineNumber, this.wordEnd(lineNumber, column, true));
    },

    /**
     * @param {number} lineNumber
     * @param {number} column
     * @param {boolean} gapless
     * @return {number}
     */
    wordStart: function(lineNumber, column, gapless)
    {
        var line = this._lines[lineNumber];
        var prefix = line.substring(0, column).split("").reverse().join("");
        var prefixMatch = this._noPunctuationRegex.exec(prefix);
        return prefixMatch && (!gapless || prefixMatch.index === 0) ? column - prefixMatch.index - prefixMatch[0].length : column;
    },

    /**
     * @param {number} lineNumber
     * @param {number} column
     * @param {boolean} gapless
     * @return {number}
     */
    wordEnd: function(lineNumber, column, gapless)
    {
        var line = this._lines[lineNumber];
        var suffix = line.substring(column);
        var suffixMatch = this._noPunctuationRegex.exec(suffix);
        return suffixMatch && (!gapless || suffixMatch.index === 0) ? column + suffixMatch.index + suffixMatch[0].length : column;
    },

    /**
     * @param {WebInspector.TextRange} range
     * @return {string}  
     */
    copyRange: function(range)
    {
        if (!range)
            range = this.range();

        var clip = [];
        if (range.startLine === range.endLine) {
            clip.push(this._lines[range.startLine].substring(range.startColumn, range.endColumn));
            return clip.join(this._lineBreak);
        }
        clip.push(this._lines[range.startLine].substring(range.startColumn));
        for (var i = range.startLine + 1; i < range.endLine; ++i)
            clip.push(this._lines[i]);
        clip.push(this._lines[range.endLine].substring(0, range.endColumn));
        return clip.join(this._lineBreak);
    },

    /**
     * @param {number} line
     * @param {string} name  
     * @param {Object?} value  
     */
    setAttribute: function(line, name, value)
    {
        var attrs = this._attributes[line];
        if (!attrs) {
            attrs = {};
            this._attributes[line] = attrs;
        }
        attrs[name] = value;
    },

    /**
     * @param {number} line
     * @param {string} name  
     * @return {Object|null} value  
     */
    getAttribute: function(line, name)
    {
        var attrs = this._attributes[line];
        return attrs ? attrs[name] : null;
    },

    /**
     * @param {number} line
     * @param {string} name
     */
    removeAttribute: function(line, name)
    {
        var attrs = this._attributes[line];
        if (attrs)
            delete attrs[name];
    },

    /**
     * @param {WebInspector.TextRange} newRange
     * @param {string} originalText
     * @return {WebInspector.TextEditorCommand}
     */
    _pushUndoableCommand: function(newRange, originalText)
    {
        var command = new WebInspector.TextEditorCommand(newRange.clone(), originalText);
        if (this._inUndo)
            this._redoStack.push(command);
        else {
            if (!this._inRedo)
                this._redoStack = [];
            this._undoStack.push(command);
        }
        return command;
    },

    /**
     * @return {?WebInspector.TextRange}
     */
    undo: function()
    {
        if (!this._undoStack.length)
            return null;

        this._markRedoableState();

        this._inUndo = true;
        var range = this._doUndo(this._undoStack);
        delete this._inUndo;

        return range;
    },

    /**
     * @return {WebInspector.TextRange}
     */
    redo: function()
    {
        if (!this._redoStack || !this._redoStack.length)
            return null;
        this._markUndoableState();

        this._inRedo = true;
        var range = this._doUndo(this._redoStack);
        delete this._inRedo;

        return range;
    },

    /**
     * @param {Array.<WebInspector.TextEditorCommand>} stack
     * @return {WebInspector.TextRange}
     */
    _doUndo: function(stack)
    {
        var range = null;
        for (var i = stack.length - 1; i >= 0; --i) {
            var command = stack[i];
            stack.length = i;
            range = this._innerEditRange(command.newRange, command.originalText);
            if (i > 0 && stack[i - 1].explicit)
                return range;
        }
        return range;
    },

    _markUndoableState: function()
    {
        if (this._undoStack.length)
            this._undoStack[this._undoStack.length - 1].explicit = true;
    },

    _markRedoableState: function()
    {
        if (this._redoStack.length)
            this._redoStack[this._redoStack.length - 1].explicit = true;
    },

    _resetUndoStack: function()
    {
        this._undoStack = [];
    },

    /**
     * @param {WebInspector.TextRange} range
     * @return {WebInspector.TextRange}
     */
    indentLines: function(range)
    {
        this._markUndoableState();

        var indent = WebInspector.settings.textEditorIndent.get();
        var newRange = range.clone();
        // Do not change a selection start position when it is at the beginning of a line
        if (range.startColumn)
            newRange.startColumn += indent.length;

        var indentEndLine = range.endLine;
        if (range.endColumn)
            newRange.endColumn += indent.length;
        else
            indentEndLine--;

        for (var lineNumber = range.startLine; lineNumber <= indentEndLine; lineNumber++)
            this._innerEditRange(WebInspector.TextRange.createFromLocation(lineNumber, 0), indent);

        return newRange;
    },

    /**
     * @param {WebInspector.TextRange} range
     * @return {WebInspector.TextRange}
     */
    unindentLines: function(range)
    {
        this._markUndoableState();

        var indent = WebInspector.settings.textEditorIndent.get();
        var indentLength = indent === WebInspector.TextEditorModel.Indent.TabCharacter ? 4 : indent.length;
        var lineIndentRegex = new RegExp("^ {1," + indentLength + "}");
        var newRange = range.clone();

        var indentEndLine = range.endLine;
        if (!range.endColumn)
            indentEndLine--;

        for (var lineNumber = range.startLine; lineNumber <= indentEndLine; lineNumber++) {
            var line = this.line(lineNumber);
            var firstCharacter = line.charAt(0);
            var lineIndentLength;

            if (firstCharacter === " ")
                lineIndentLength = line.match(lineIndentRegex)[0].length;
            else if (firstCharacter === "\t")
                lineIndentLength = 1;
            else
                continue;

            this._innerEditRange(new WebInspector.TextRange(lineNumber, 0, lineNumber, lineIndentLength), "");

            if (lineNumber === range.startLine)
                newRange.startColumn = Math.max(0, newRange.startColumn - lineIndentLength);
            if (lineNumber === range.endLine)
                newRange.endColumn = Math.max(0, newRange.endColumn - lineIndentLength);
        }

        return newRange;
    },

    /**
     * @param {number=} from
     * @param {number=} to
     * @return {WebInspector.TextEditorModel}
     */
    slice: function(from, to)
    {
        var textModel = new WebInspector.TextEditorModel();
        textModel._lines = this._lines.slice(from, to);
        textModel._lineBreak = this._lineBreak;
        return textModel;
    },

    /**
     * @param {WebInspector.TextRange} range
     * @return {WebInspector.TextRange}
     */
    growRangeLeft: function(range)
    {
        var result = range.clone();
        if (result.startColumn)
            --result.startColumn;
        else if (result.startLine)
            result.startColumn = this.lineLength(--result.startLine);
        return result;
    },

    /**
     * @param {WebInspector.TextRange} range
     * @return {WebInspector.TextRange}
     */
    growRangeRight: function(range)
    {
        var result = range.clone();
        if (result.endColumn < this.lineLength(result.endLine))
            ++result.endColumn;
        else if (result.endLine < this.linesCount) {
            result.endColumn = 0;
            ++result.endLine;
        }
        return result;
    },

    __proto__: WebInspector.Object.prototype
}
