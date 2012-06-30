/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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

var config = config || {};

(function() {

config.kPlatforms = {
    'apple' : {
        label : 'Apple',
        buildConsoleURL: 'http://build.webkit.org',
        layoutTestResultsURL: 'http://build.webkit.org/results',
        waterfallURL: 'http://build.webkit.org/waterfall',
        builders: {
            'Apple Lion Release WK1 (Tests)' : {version: 'lion' },
            'Apple Lion Debug WK1 (Tests)' : {version: 'lion', debug: true},
            'Apple Lion Release WK2 (Tests)' : {version: 'lion' },
            'Apple Lion Debug WK2 (Tests)' : {version: 'lion', debug: true},
            // 'Apple Win XP Debug (Tests)' : {version: 'xp',debug: true},
            // 'Apple Win 7 Release (Tests)' : {version: 'win7'},
        },
        haveBuilderAccumulatedResults : false,
        useDirectoryListingForOldBuilds: false,
        useFlakinessDashboard: false,
        resultsDirectoryNameFromBuilderName: function(builderName) {
            return encodeURIComponent(builderName);
        },
        resultsDirectoryForBuildNumber: function(buildNumber, revision) {
            return encodeURIComponent('r' + revision + ' (' + buildNumber + ')');
        },
        builderApplies: function(builderName) {
            return builderName.indexOf('Apple') != -1;
        },
    },
    'chromium' : {
        label : 'Chromium',
        buildConsoleURL: 'http://build.chromium.org/p/chromium.webkit',
        layoutTestResultsURL: 'http://build.chromium.org/f/chromium/layout_test_results',
        waterfallURL: 'http://build.chromium.org/p/chromium.webkit/waterfall',
        builders: {
            'Webkit Win': {version: 'xp'},
            'Webkit Win7': {version: 'win7'},
            'Webkit Win (dbg)(1)': {version: 'xp', debug: true},
            'Webkit Win (dbg)(2)': {version: 'xp', debug: true},
            'Webkit Linux': {version: 'lucid', is64bit: true},
            'Webkit Linux 32': {version: 'lucid'},
            'Webkit Linux (dbg)': {version: 'lucid', is64bit: true, debug: true},
            'Webkit Mac10.6': {version: 'snowleopard'},
            'Webkit Mac10.6 (dbg)': {version: 'snowleopard', debug: true},
            'Webkit Mac10.7': {version: 'lion'},
        },
        haveBuilderAccumulatedResults : true,
        useDirectoryListingForOldBuilds: true,
        useFlakinessDashboard: true,
        resultsDirectoryNameFromBuilderName: function(builderName) {
            return base.underscoredBuilderName(builderName);
        },
        resultsDirectoryForBuildNumber: function(buildNumber, revision) {
            return buildNumber;
        },
        builderApplies: function(builderName) {
            // FIXME: Should garden-o-matic show these? I can imagine showing the deps bots being useful at least so
            // that the gardener only need to look at garden-o-matic and never at the waterfall. Not really sure who
            // watches the GPU bots.
            return builderName.indexOf('GPU') == -1 && builderName.indexOf('deps') == -1 && builderName.indexOf('ASAN') == -1;
        },
    },
    'gtk' : {
        label : 'GTK',
        buildConsoleURL: 'http://build.webkit.org',
        layoutTestResultsURL: 'http://build.webkit.org/results',
        waterfallURL: 'http://build.webkit.org/waterfall',
        builders: {
            'GTK Linux 32-bit Release' : {version: '32-bit release'},
            'GTK Linux 64-bit Release' : {version: '64-bit release'},
            'GTK Linux 64-bit Debug' : {version: '64-bit debug', debug: true},
        },
        haveBuilderAccumulatedResults : false,
        useDirectoryListingForOldBuilds: false,
        useFlakinessDashboard: false,
        resultsDirectoryNameFromBuilderName: function(builderName) {
            return encodeURIComponent(builderName);
        },
        resultsDirectoryForBuildNumber: function(buildNumber, revision) {
            return encodeURIComponent('r' + revision + ' (' + buildNumber + ')');
        },
        builderApplies: function(builderName) {
            return builderName.indexOf('GTK') != -1;
        },
    },
    'qt' : {
        label : 'Qt',
        buildConsoleURL: 'http://build.webkit.org',
        layoutTestResultsURL: 'http://build.webkit.org/results',
        waterfallURL: 'http://build.webkit.org/waterfall',
        builders: {
            'Qt Linux Release' : {version : '32-bit release'},
        },
        haveBuilderAccumulatedResults : false,
        useDirectoryListingForOldBuilds: false,
        useFlakinessDashboard: false,
        resultsDirectoryNameFromBuilderName: function(builderName) {
            return encodeURIComponent(builderName);
        },
        resultsDirectoryForBuildNumber: function(buildNumber, revision) {
            return encodeURIComponent('r' + revision + ' (' + buildNumber + ')');
        },
        builderApplies: function(builderName) {
            return builderName.indexOf('Qt') != -1;
        },
    },
};

config.kTracURL = 'http://trac.webkit.org';
config.kBugzillaURL = 'https://bugs.webkit.org';
config.kLocalServerURL = 'http://127.0.0.1:8127';

config.kRevisionAttr = 'data-revision';
config.kTestNameAttr = 'data-test-name';
config.kBuilderNameAttr = 'data-builder-name';
config.kFailureCountAttr = 'data-failure-count';
config.kFailureTypesAttr = 'data-failure-types';
config.kInfobarTypeAttr = 'data-infobar-type';

var kTenMinutesInMilliseconds = 10 * 60 * 1000;
config.kUpdateFrequency = kTenMinutesInMilliseconds;
config.kRelativeTimeUpdateFrequency = 1000 * 60;

config.kExperimentalFeatures = window.location.search.search('enableExperiments=1') != -1;

config.currentPlatform = 'chromium';

config.setPlatform = function(platform)
{
    if (!this.kPlatforms[platform]) {
        window.console.log(platform + ' is not a recognized platform');
        return;
    }
    config.currentPlatform = platform;
};

})();
