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

Buildbot = function(baseURL, queuesInfo)
{
    BaseObject.call(this);

    console.assert(baseURL);
    console.assert(queuesInfo);

    this.baseURL = baseURL;
    this.queues = {};

    for (var id in queuesInfo)
        this.queues[id] = new BuildbotQueue(this, id, queuesInfo[id]);
};

BaseObject.addConstructorFunctions(Buildbot);

// Ordered importance/recency.
Buildbot.Platform = {
    MacOSXMavericks: { name: "mac-os-x-mavericks", readableName: "OS X Mavericks", order: 10 },
    MacOSXMountainLion: { name: "mac-os-x-mountain-lion", readableName: "OS X Mountain Lion", order: 20 },
    MacOSXLion: { name : "mac-os-x-lion", readableName: "Mac OS X Lion", order: 30 },
    Windows8: { name: "windows-8", readableName: "Windows 8", order: 40 },
    Windows7: { name: "windows-7", readableName: "Windows 7", order: 50 },
    WindowsXP: { name: "windows-xp", readableName: "Windows XP", order: 60 },
    LinuxQt: { name : "linux-qt", readableName: "Linux Qt", order: 70 },
    LinuxGTK: { name : "linux-gtk", readableName: "Linux GTK", order: 80 },
    LinuxEFL: { name: "linux-efl", readableName: "Linux EFL", order : 90 }
};

// Ordered importance.
Buildbot.TestCategory = {
    WebKit2: "webkit-2",
    WebKit1: "webkit-1"
};

// Ordered importance.
Buildbot.BuildArchitecture = {
    Universal: "universal",
    SixtyFourBit: "sixty-four-bit",
    ThirtyTwoBit: "thirty-two-bit"
};

Buildbot.prototype = {
    constructor: Buildbot,
    __proto__: BaseObject.prototype,

    buildPageURLForIteration: function(iteration)
    {
        return this.baseURL + "builders/" + encodeURIComponent(iteration.queue.id) + "/builds/" + iteration.id;
    },

    javascriptTestResultsURLForIteration: function(iteration)
    {
        return this.baseURL + "builders/" + encodeURIComponent(iteration.queue.id) + "/builds/" + iteration.id + "/steps/jscore-test/logs/stdio";
    },

    apiTestResultsURLForIteration: function(iteration)
    {
        return this.baseURL + "builders/" + encodeURIComponent(iteration.queue.id) + "/builds/" + iteration.id + "/steps/run-api-tests/logs/stdio";
    },

    webkitpyTestResultsURLForIteration: function(iteration)
    {
        return this.baseURL + "builders/" + encodeURIComponent(iteration.queue.id) + "/builds/" + iteration.id + "/steps/webkitpy-test/logs/stdio";
    },

    webkitperlTestResultsURLForIteration: function(iteration)
    {
        return this.baseURL + "builders/" + encodeURIComponent(iteration.queue.id) + "/builds/" + iteration.id + "/steps/webkitperl-test/logs/stdio";
    },

    bindingsTestResultsURLForIteration: function(iteration)
    {
        return this.baseURL + "builders/" + encodeURIComponent(iteration.queue.id) + "/builds/" + iteration.id + "/steps/bindings-generation-test/logs/stdio";
    }
};
