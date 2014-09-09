/*
 * Copyright (C) 2013, 2014 Apple Inc. All rights reserved.
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

Trac = function(baseURL, options)
{
    BaseObject.call(this);

    console.assert(baseURL);

    this.baseURL = baseURL;
    this._needsAuthentication = (typeof options === "object") && options[Trac.NeedsAuthentication] === true;

    this.recordedCommits = []; // Will be sorted in ascending order.
};

BaseObject.addConstructorFunctions(Trac);

Trac.NeedsAuthentication = "needsAuthentication";
Trac.UpdateInterval = 45000; // 45 seconds

Trac.Event = {
    CommitsUpdated: "commits-updated",
    Loaded: "loaded"
};

Trac.prototype = {
    constructor: Trac,
    __proto__: BaseObject.prototype,

    get latestRecordedRevisionNumber()
    {
        if (!this.recordedCommits.length)
            return undefined;
        return this.recordedCommits[this.recordedCommits.length - 1].revisionNumber;
    },

    revisionURL: function(revision)
    {
        return this.baseURL + "changeset/" + encodeURIComponent(revision);
    },

    _xmlTimelineURL: function(fromDate, toDate)
    {
        if (typeof fromDate === "undefined") {
            fromDate = new Date();
            toDate = new Date(fromDate);
            // By default, get at least one full day of changesets, as the current day may have only begun.
            fromDate.setDate(fromDate.getDate() - 1);
        } else if (typeof toDate === "undefined")
            toDate = fromDate;

        console.assert(fromDate <= toDate);

        var fromDay = new Date(fromDate.getFullYear(), fromDate.getMonth(), fromDate.getDate());
        var toDay = new Date(toDate.getFullYear(), toDate.getMonth(), toDate.getDate());

        return this.baseURL + "timeline?changeset=on&format=rss&max=0" +
            "&from=" +  (toDay.getMonth() + 1) + "%2F" + toDay.getDate() + "%2F" + (toDay.getFullYear() % 100) +
            "&daysback=" + ((toDay - fromDay) / 1000 / 60 / 60 / 24);
    },

    _convertCommitInfoElementToObject: function(doc, commitElement)
    {
        var link = doc.evaluate("./link", commitElement, null, XPathResult.STRING_TYPE).stringValue;
        var revisionNumber = parseInt(/\d+$/.exec(link))

        function tracNSResolver(prefix)
        {
            if (prefix == "dc")
                return "http://purl.org/dc/elements/1.1/";
            return null;
        }

        var author = doc.evaluate("./author|dc:creator", commitElement, tracNSResolver, XPathResult.STRING_TYPE).stringValue;
        var date = doc.evaluate("./pubDate", commitElement, null, XPathResult.STRING_TYPE).stringValue;
        date = new Date(Date.parse(date));
        var description = doc.evaluate("./description", commitElement, null, XPathResult.STRING_TYPE).stringValue;

        var parsedDescription = document.createElement("div");
        parsedDescription.innerHTML = description;

        var location = "";
        if (parsedDescription.firstChild.className === "changes") {
            // We can extract branch information when trac.ini contains "changeset_show_files=location".
            location = doc.evaluate("//strong", parsedDescription.firstChild, null, XPathResult.STRING_TYPE).stringValue
            parsedDescription.removeChild(parsedDescription.firstChild);
        }

        // The feed contains a <title>, but it's not parsed as well as what we are getting from description.
        var title = document.createElement("div");
        var node = parsedDescription.firstChild.firstChild;
        while (node && node.tagName != "BR") {
            title.appendChild(node.cloneNode(true));
            node = node.nextSibling;
        }

        // For some reason, trac titles start with a newline. Delete it.
        if (title.firstChild && title.firstChild.nodeType == Node.TEXT_NODE && title.firstChild.textContent.length > 0 && title.firstChild.textContent[0] == "\n")
            title.firstChild.textContent = title.firstChild.textContent.substring(1);

        var result = {
            revisionNumber: revisionNumber,
            link: link,
            title: title,
            author: author,
            date: date,
            description: parsedDescription.innerHTML,
            containsBranchLocation: location !== ""
        };

        if (result.containsBranchLocation) {
            console.assert(location[location.length - 1] !== "/");
            location = location += "/";
            if (location.startsWith("tags/"))
                result.tag = location.substr(5, location.indexOf("/", 5) - 5);
            else if (location.startsWith("branches/"))
                result.branch = location.substr(9, location.indexOf("/", 9) - 9);
            else if (location.startsWith("releases/"))
                result.release = location.substr(9, location.indexOf("/", 9) - 9);
            else if (location.startsWith("trunk/"))
                result.branch = "trunk";
            else {
                console.assert(false);
                result.containsBranchLocation = false;
            }
        }

        return result;
    },

    _loaded: function(dataDocument)
    {
        if (!dataDocument)
            return;

        var recordedRevisionNumbers = this.recordedCommits.reduce(function(previousResult, commit) {
            previousResult[commit.revisionNumber] = commit;
            return previousResult;
        }, {});

        var knownCommitsWereUpdated = false;
        var newCommits = [];

        var commitInfoElements = dataDocument.evaluate("/rss/channel/item", dataDocument, null, XPathResult.ORDERED_NODE_ITERATOR_TYPE);
        var commitInfoElement;
        while (commitInfoElement = commitInfoElements.iterateNext()) {
            var commit = this._convertCommitInfoElementToObject(dataDocument, commitInfoElement);
            if (commit.revisionNumber in recordedRevisionNumbers) {
                // Author could have changed, as commit queue replaces it after the fact.
                console.assert(recordedRevisionNumbers[commit.revisionNumber].revisionNumber === commit.revisionNumber);
                if (recordedRevisionNumbers[commit.revisionNumber].author != commit.author) {
                    recordedRevisionNumbers[commit.revisionNumber].author = commit.author;
                    knownCommitWasUpdated = true;
                }
            } else
                newCommits.push(commit);
        }

        if (newCommits.length)
            this.recordedCommits = newCommits.concat(this.recordedCommits).sort(function(a, b) { return a.revisionNumber - b.revisionNumber; });

        if (newCommits.length || knownCommitsWereUpdated)
            this.dispatchEventToListeners(Trac.Event.CommitsUpdated, null);
    },

    load: function(fromDate, toDate)
    {
        loadXML(this._xmlTimelineURL(fromDate, toDate), function(dataDocument) {
            this._loaded(dataDocument);
            this.dispatchEventToListeners(Trac.Event.Loaded, [fromDate, toDate]);
        }.bind(this), this._needsAuthentication ? { withCredentials: true } : {});
    },

    update: function()
    {
        loadXML(this._xmlTimelineURL(), this._loaded.bind(this), this._needsAuthentication ? { withCredentials: true } : {});
    },

    startPeriodicUpdates: function()
    {
        this.update();
        this.updateTimer = setInterval(this.update.bind(this), Trac.UpdateInterval);
    }
};
