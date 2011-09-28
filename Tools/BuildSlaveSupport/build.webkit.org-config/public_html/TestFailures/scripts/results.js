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

var results = results || {};

(function() {

var kLayoutTestResultsServer = 'http://build.chromium.org/f/chromium/layout_test_results/';
var kLayoutTestResultsPath = '/results/layout-test-results/';
var kResultsName = 'full_results.json';

var kBuildLinkRegexp = /a href="\d+\/"/g;
var kBuildNumberRegexp = /\d+/;

var PASS = 'PASS';
var TIMEOUT = 'TIMEOUT';
var TEXT = 'TEXT';
var CRASH = 'CRASH';
var IMAGE = 'IMAGE';
var IMAGE_TEXT = 'IMAGE+TEXT';

var kFailingResults = [TIMEOUT, TEXT, CRASH, IMAGE, IMAGE_TEXT];

var kExpectedImageSuffix = '-expected.png';
var kActualImageSuffix = '-actual.png';
var kImageDiffSuffix = '-diff.png';
var kExpectedTextSuffix = '-expected.txt';
var kActualTextSuffix = '-actual.txt';
var kDiffTextSuffix = '-diff.txt';
var kCrashLogSuffix = '-crash-log.txt';

var kPNGExtension = 'png';
var kTXTExtension = 'txt';

var kPreferredSuffixOrder = [
    kExpectedImageSuffix,
    kActualImageSuffix,
    kImageDiffSuffix,
    kExpectedTextSuffix,
    kActualTextSuffix,
    kDiffTextSuffix,
    kCrashLogSuffix,
    // FIXME: Add support for the rest of the result types.
];

// Kinds of results.
results.kActualKind = 'actual';
results.kExpectedKind = 'expected';
results.kDiffKind = 'diff';
results.kUnknownKind = 'unknown';

// Types of tests.
results.kImageType = 'image'
results.kTextType = 'text'
// FIXME: There are more types of tests.

function isFailure(result)
{
    return kFailingResults.indexOf(result) != -1;
}

function isSuccess(result)
{
    return result === PASS;
}

function possibleSuffixListFor(failureTypeList)
{
    var suffixList = [];

    function pushImageSuffixes()
    {
        suffixList.push(kExpectedImageSuffix);
        suffixList.push(kActualImageSuffix);
        suffixList.push(kImageDiffSuffix);
    }

    function pushTextSuffixes()
    {
        suffixList.push(kActualTextSuffix);
        suffixList.push(kExpectedTextSuffix);
        suffixList.push(kDiffTextSuffix);
        // '-wdiff.html',
        // '-pretty-diff.html',
    }

    $.each(failureTypeList, function(index, failureType) {
        switch(failureType) {
        case IMAGE:
            pushImageSuffixes();
            break;
        case TEXT:
            pushTextSuffixes();
            break;
        case IMAGE_TEXT:
            pushImageSuffixes();
            pushTextSuffixes();
            break;
        case CRASH:
            suffixList.push(kCrashLogSuffix);
            break;
        default:
            // FIXME: Add support for the rest of the result types.
            // '-expected.html',
            // '-expected-mismatch.html',
            // '-expected.wav',
            // '-actual.wav',
            // ... and possibly more.
            break;
        }
    });

    return base.uniquifyArray(suffixList);
}

results.failureTypeToExtensionList = function(failureType)
{
    switch(failureType) {
    case IMAGE:
        return [kPNGExtension];
    case TEXT:
        return [kTXTExtension];
    case IMAGE_TEXT:
        return [kTXTExtension, kPNGExtension];
    default:
        // FIXME: Add support for the rest of the result types.
        // '-expected.html',
        // '-expected-mismatch.html',
        // '-expected.wav',
        // '-actual.wav',
        // ... and possibly more.
        return [];
    }
};

results.failureTypeList = function(failureBlob)
{
    return failureBlob.split(' ');
};

results.canRebaseline = function(failureTypeList)
{
    return failureTypeList.some(function(element) {
        return results.failureTypeToExtensionList(element).length > 0;
    });
};

results.directoryForBuilder = function(builderName)
{
    return builderName.replace(/[ .()]/g, '_');
}

function resultsDirectoryURL(builderName)
{
    return kLayoutTestResultsServer + results.directoryForBuilder(builderName) + kLayoutTestResultsPath;
}

function resultsDirectoryListingURL(builderName)
{
    return kLayoutTestResultsServer + results.directoryForBuilder(builderName) + '/';
}

function resultsDirectoryURLForBuildNumber(builderName, buildNumber)
{
    return resultsDirectoryListingURL(builderName) + buildNumber + '/'
}

function resultsSummaryURL(builderName)
{
    return resultsDirectoryURL(builderName) + kResultsName;
}

function resultsSummaryURLForBuildNumber(builderName, buildNumber)
{
    return resultsDirectoryURLForBuildNumber(builderName, buildNumber) + kResultsName;
}

var g_resultsCache = new base.AsynchronousCache(function (key, callback) {
    net.jsonp(key, callback);
});

function anyIsFailure(resultsList)
{
    return $.grep(resultsList, isFailure).length > 0;
}

function anyIsSuccess(resultsList)
{
    return $.grep(resultsList, isSuccess).length > 0;
}

function addImpliedExpectations(resultsList)
{
    if (resultsList.indexOf('FAIL') == -1)
        return resultsList;
    return resultsList.concat(kFailingResults);
}

results.unexpectedResults = function(resultNode)
{
    var actualResults = results.failureTypeList(resultNode.actual);
    var expectedResults = addImpliedExpectations(results.failureTypeList(resultNode.expected))

    return $.grep(actualResults, function(result) {
        return expectedResults.indexOf(result) == -1;
    });
};

function isExpectedOrUnexpectedFailure(resultNode)
{
    if (!resultNode)
        return false;
    var actualResults = results.failureTypeList(resultNode.actual);
    if (anyIsSuccess(actualResults))
        return false;
    return anyIsFailure(actualResults);
}

function isUnexpectedFailure(resultNode)
{
    if (!resultNode)
        return false;
    if (anyIsSuccess(results.failureTypeList(resultNode.actual)))
        return false;
    return anyIsFailure(results.unexpectedResults(resultNode));
}

function isUnexpectedSuccesses(resultNode)
{
    if (!resultNode)
        return false;
    if (anyIsFailure(results.failureTypeList(resultNode.actual)))
        return false;
    return anyIsSuccess(results.unexpectedResults(resultNode));
}

function isResultNode(node)
{
    return !!node.actual;
}

results.expectedOrUnexpectedFailures = function(resultsTree)
{
    return base.filterTree(resultsTree.tests, isResultNode, isExpectedOrUnexpectedFailure);
};

results.unexpectedFailures = function(resultsTree)
{
    return base.filterTree(resultsTree.tests, isResultNode, isUnexpectedFailure);
};

results.unexpectedSuccesses = function(resultsTree)
{
    return base.filterTree(resultsTree.tests, isResultNode, isUnexpectedSuccesses);
};

function resultsByTest(resultsByBuilder, filter)
{
    var resultsByTest = {};

    $.each(resultsByBuilder, function(builderName, resultsTree) {
        $.each(filter(resultsTree), function(testName, resultNode) {
            resultsByTest[testName] = resultsByTest[testName] || {};
            resultsByTest[testName][builderName] = resultNode;
        });
    });

    return resultsByTest;
}

results.expectedOrUnexpectedFailuresByTest = function(resultsByBuilder)
{
    return resultsByTest(resultsByBuilder, results.expectedOrUnexpectedFailures);
};

results.unexpectedFailuresByTest = function(resultsByBuilder)
{
    return resultsByTest(resultsByBuilder, results.unexpectedFailures);
};

results.unexpectedSuccessesByTest = function(resultsByBuilder)
{
    return resultsByTest(resultsByBuilder, results.unexpectedSuccesses);
};

results.failureInfoForTestAndBuilder = function(resultsByTest, testName, builderName)
{
    return {
        'testName': testName,
        'builderName': builderName,
        'failureTypeList': results.failureTypeList(resultsByTest[testName][builderName].actual)
    }
};

results.collectUnexpectedResults = function(dictionaryOfResultNodes)
{
    var collectedResults = [];
    $.each(dictionaryOfResultNodes, function(key, resultNode) {
        collectedResults = collectedResults.concat(results.unexpectedResults(resultNode));
    });
    return base.uniquifyArray(collectedResults);
};

function historicalResultsSummaryURLs(builderName, callback)
{
    net.get(resultsDirectoryListingURL(builderName), function(directoryListing) {
        var summaryURLs = directoryListing.match(kBuildLinkRegexp).map(function(buildLink) {
            var buildNumber = parseInt(buildLink.match(kBuildNumberRegexp)[0]);
            return resultsSummaryURLForBuildNumber(builderName, buildNumber);
        }).reverse();
        callback(summaryURLs);
    });
}

function walkHistory(builderName, testName, callback)
{
    var indexOfNextKeyToFetch = 0;
    var keyList = [];

    function continueWalk()
    {
        if (indexOfNextKeyToFetch >= keyList.length) {
            processResultNode(0, null);
            return;
        }

        var key = keyList[indexOfNextKeyToFetch];
        ++indexOfNextKeyToFetch;
        g_resultsCache.get(key, function(resultsTree) {
            var resultNode = results.resultNodeForTest(resultsTree, testName);
            var revision = parseInt(resultsTree['revision'])
            if (isNaN(revision))
                revision = 0;
            processResultNode(revision, resultNode);
        });
    }

    function processResultNode(revision, resultNode)
    {
        var shouldContinue = callback(revision, resultNode);
        if (!shouldContinue)
            return;
        continueWalk();
    }

    historicalResultsSummaryURLs(builderName, function(summaryURLs) {
        keyList = summaryURLs;
        continueWalk();
    });
}

results.regressionRangeForFailure = function(builderName, testName, callback)
{
    var oldestFailingRevision = 0;
    var newestPassingRevision = 0;

    walkHistory(builderName, testName, function(revision, resultNode) {
        if (!revision) {
            callback(oldestFailingRevision, newestPassingRevision);
            return false;
        }
        if (!resultNode) {
            newestPassingRevision = revision;
            callback(oldestFailingRevision, newestPassingRevision);
            return false;
        }
        if (isUnexpectedFailure(resultNode)) {
            oldestFailingRevision = revision;
            return true;
        }
        if (!oldestFailingRevision)
            return true;  // We need to keep looking for a failing revision.
        newestPassingRevision = revision;
        callback(oldestFailingRevision, newestPassingRevision);
        return false;
    });
};

function mergeRegressionRanges(regressionRanges)
{
    var mergedRange = {};

    mergedRange.oldestFailingRevision = 0;
    mergedRange.newestPassingRevision = 0;

    $.each(regressionRanges, function(builderName, range) {
        if (!range.oldestFailingRevision || !range.newestPassingRevision)
            return

        if (!mergedRange.oldestFailingRevision)
            mergedRange.oldestFailingRevision = range.oldestFailingRevision;
        if (!mergedRange.newestPassingRevision)
            mergedRange.newestPassingRevision = range.newestPassingRevision;

        if (range.oldestFailingRevision < mergedRange.oldestFailingRevision)
            mergedRange.oldestFailingRevision = range.oldestFailingRevision;
        if (range.newestPassingRevision > mergedRange.newestPassingRevision)
            mergedRange.newestPassingRevision = range.newestPassingRevision;
    });
    return mergedRange;
}

results.unifyRegressionRanges = function(builderNameList, testName, callback)
{
    var queriesInFlight = builderNameList.length;
    if (!queriesInFlight)
        callback(0, 0);

    var regressionRanges = {};
    $.each(builderNameList, function(index, builderName) {
        results.regressionRangeForFailure(builderName, testName, function(oldestFailingRevision, newestPassingRevision) {
            var range = {};
            range.oldestFailingRevision = oldestFailingRevision;
            range.newestPassingRevision = newestPassingRevision;
            regressionRanges[builderName] = range;

            --queriesInFlight;
            if (!queriesInFlight) {
                var mergedRange = mergeRegressionRanges(regressionRanges);
                callback(mergedRange.oldestFailingRevision, mergedRange.newestPassingRevision);
            }
        });
    });
};

results.countFailureOccurances = function(builderNameList, testName, callback)
{
    var queriesInFlight = builderNameList.length;
    if (!queriesInFlight)
        callback(0);

    var failureCount = 0;
    $.each(builderNameList, function(index, builderName) {
        walkHistory(builderName, testName, function(revision, resultNode) {
            if (isUnexpectedFailure(resultNode)) {
                ++failureCount;
                return true;
            }

            --queriesInFlight;
            if (!queriesInFlight)
                callback(failureCount);
            return false;
        });
    });
};

results.resultNodeForTest = function(resultsTree, testName)
{
    var testNamePath = testName.split('/');
    var currentNode = resultsTree['tests'];
    $.each(testNamePath, function(index, segmentName) {
        if (!currentNode)
            return;
        currentNode = (segmentName in currentNode) ? currentNode[segmentName] : null;
    });
    return currentNode;
};

results.resultKind = function(url)
{
    if (/-actual\.[a-z]+$/.test(url))
        return results.kActualKind;
    else if (/-expected\.[a-z]+$/.test(url))
        return results.kExpectedKind;
    else if (/diff\.[a-z]+$/.test(url))
        return results.kDiffKind;
    return results.kUnknownKind;
}

results.resultType = function(url)
{
    if (/\.png$/.test(url))
        return results.kImageType;
    return results.kTextType;
}

function sortResultURLsBySuffix(urls)
{
    var sortedURLs = [];
    $.each(kPreferredSuffixOrder, function(i, suffix) {
        $.each(urls, function(j, url) {
            if (!base.endsWith(url, suffix))
                return;
            sortedURLs.push(url);
        });
    });
    if (sortedURLs.length != urls.length)
        throw "sortResultURLsBySuffix failed to return the same number of URLs."
    return sortedURLs;
}

results.fetchResultsURLs = function(failureInfo, callback)
{
    var stem = resultsDirectoryURL(failureInfo.builderName);
    var testNameStem = base.trimExtension(failureInfo.testName);

    var suffixList = possibleSuffixListFor(failureInfo.failureTypeList);

    var resultURLs = [];
    var requestsInFlight = suffixList.length;

    if (!requestsInFlight) {
        callback([]);
        return;
    }

    function checkComplete()
    {
        if (--requestsInFlight == 0)
            callback(sortResultURLsBySuffix(resultURLs));
    }

    $.each(suffixList, function(index, suffix) {
        var url = stem + testNameStem + suffix;
        net.probe(url, {
            success: function() {
                resultURLs.push(url);
                checkComplete();
            },
            error: checkComplete,
        });
    });
};

results.fetchResultsForBuilder = function(builderName, callback)
{
    net.jsonp(resultsSummaryURL(builderName), callback);
};

results.fetchResultsByBuilder = function(builderNameList, callback)
{
    var resultsByBuilder = {}
    var requestsInFlight = builderNameList.length;
    $.each(builderNameList, function(index, builderName) {
        results.fetchResultsForBuilder(builderName, function(resultsTree) {
            resultsByBuilder[builderName] = resultsTree;
            --requestsInFlight;
            if (!requestsInFlight)
                callback(resultsByBuilder);
        });
    });
};

})();
