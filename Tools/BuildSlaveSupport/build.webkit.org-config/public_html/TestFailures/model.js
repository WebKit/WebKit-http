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

var model = model || {};

(function () {

var kCommitLogLength = 50;

model.state = {};
model.state.failureAnalysisByTest = {};
model.state.rebaselineQueue = [];
model.state.expectationsUpdateQueue = [];

function findAndMarkRevertedRevisions(commitDataList)
{
    var revertedRevisions = {};
    $.each(commitDataList, function(index, commitData) {
        if (commitData.revertedRevision)
            revertedRevisions[commitData.revertedRevision] = true;
    });
    $.each(commitDataList, function(index, commitData) {
        if (commitData.revision in revertedRevisions)
            commitData.wasReverted = true;
    });
}

function fuzzyFind(testName, commitData)
{
    var indexOfLastDot = testName.lastIndexOf('.');
    var stem = indexOfLastDot == -1 ? testName : testName.substr(0, indexOfLastDot);
    return commitData.message.indexOf(stem) != -1;
}

function heuristicallyNarrowRegressionRange(failureAnalysis)
{
    var commitDataList = model.state.recentCommits;
    var commitDataIndex = commitDataList.length - 1;

    for(var revision = failureAnalysis.newestPassingRevision + 1; revision <= failureAnalysis.oldestFailingRevision; ++revision) {
        while (commitDataIndex >= 0 && commitDataList[commitDataIndex].revision < revision)
            --commitDataIndex;
        var commitData = commitDataList[commitDataIndex];
        if (commitData.revision != revision)
            continue;
        if (fuzzyFind(failureAnalysis.testName, commitData)) {
            failureAnalysis.oldestFailingRevision = revision;
            failureAnalysis.newestPassingRevision = revision - 1;
            return;
        }
    }
}

model.queueForRebaseline = function(failureInfo)
{
    model.state.rebaselineQueue.push(failureInfo);
};

model.takeRebaselineQueue = function()
{
    var queue = model.state.rebaselineQueue;
    model.state.rebaselineQueue = [];
    return queue;
};

model.queueForExpectationUpdate = function(failureInfo)
{
    model.state.expectationsUpdateQueue.push(failureInfo);
};

model.takeExpectationUpdateQueue = function()
{
    var queue = model.state.expectationsUpdateQueue;
    model.state.expectationsUpdateQueue = [];
    return queue;
};

model.updateRecentCommits = function(callback)
{
    trac.recentCommitData('trunk', kCommitLogLength, function(commitDataList) {
        model.state.recentCommits = commitDataList;
        findAndMarkRevertedRevisions(model.state.recentCommits);
        callback();
    });
};

model.updateResultsByBuilder = function(callback)
{
    results.fetchResultsByBuilder(config.kBuilders, function(resultsByBuilder) {
        model.state.resultsByBuilder = resultsByBuilder;
        callback();
    });
};

model.analyzeUnexpectedFailures = function(callback)
{
    var unexpectedFailures = results.unexpectedFailuresByTest(model.state.resultsByBuilder);

    $.each(model.state.failureAnalysisByTest, function(testName, failureAnalysis) {
        if (!(testName in unexpectedFailures))
            delete model.state.failureAnalysisByTest[testName];
    });

    $.each(unexpectedFailures, function(testName, resultNodesByBuilder) {
        var builderNameList = base.keys(resultNodesByBuilder);
        results.unifyRegressionRanges(builderNameList, testName, function(oldestFailingRevision, newestPassingRevision) {
            var failureAnalysis = {
                'testName': testName,
                'resultNodesByBuilder': resultNodesByBuilder,
                'oldestFailingRevision': oldestFailingRevision,
                'newestPassingRevision': newestPassingRevision,
            };

            heuristicallyNarrowRegressionRange(failureAnalysis);

            var previousFailureAnalysis = model.state.failureAnalysisByTest[testName];
            if (previousFailureAnalysis
                && previousFailureAnalysis.oldestFailingRevision <= failureAnalysis.oldestFailingRevision
                && previousFailureAnalysis.newestPassingRevision >= failureAnalysis.newestPassingRevision) {
                failureAnalysis.oldestFailingRevision = previousFailureAnalysis.oldestFailingRevision;
                failureAnalysis.newestPassingRevision = previousFailureAnalysis.newestPassingRevision;
            }

            model.state.failureAnalysisByTest[testName] = failureAnalysis;
            callback(failureAnalysis);
        });
    });
};

model.analyzeUnexpectedSuccesses = function(callback)
{
    var unexpectedSuccesses = results.unexpectedSuccessesByTest(model.state.resultsByBuilder);
    $.each(unexpectedSuccesses, function(testName, resultNodesByBuilder) {
        var successAnalysis = {
            'testName': testName,
            'resultNodesByBuilder': resultNodesByBuilder,
        };

        // FIXME: Consider looking at the history to see how long this test
        // has been unexpectedly passing.

        callback(successAnalysis);
    });
};

})();
