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

(function() {

var kFastLoadingDEBUG = false;

var g_updateTimerId = 0;
var g_resultsDetailsIterator = null;
var g_treeState = {};

var kBuildFailedInfobarType = 'build-failed';

function dismissButterbar()
{
    $('.butterbar').fadeOut('fast');
    checkForLocalServer();
}

function displayOnButterbar(message)
{
    $('.butterbar .status').text(message);
    $('.butterbar').fadeIn('fast');
}

function hideInfobar()
{
    $('.infobar').animate({
        opacity: 'toggle',
        height: 'toggle',
    });
}

function hideInfobarIfOfType(type)
{
    if (!$('.infobar .status').is(':visible'))
        return;
    if ($('.infobar .status').attr(config.kInfobarTypeAttr) != type)
        return;
    hideInfobar();
}

function displayInfobar(message, type)
{
    $('.infobar .status').empty().attr(config.kInfobarTypeAttr, type).append(message);
    $('.infobar').animate({
        opacity: 'toggle',
        height: 'toggle',
    });
}

function setIconState(hasFailures)
{
    var faviconURL = 'favicon-' + (hasFailures ? 'red' : 'green') + '.png';
    $('#favicon').attr('href', faviconURL);
}

function toggleButton(button, isEnabled)
{
    if (isEnabled)
        button.removeAttr('disabled');
    else
        button.attr('disabled', true)
}

function rebaseline(failureInfoList)
{
    if (!failureInfoList.length)
        return;
    displayOnButterbar('Rebaselining...');
    checkout.rebaseline(failureInfoList, function() {
        dismissButterbar();
        // FIXME: We should use something like a lightbox rather than alert!
        alert('New results downloaded to your working copy. Please use "webkit-patch land-cowboy" to land the updated baselines.');
    });
}

function showResultsDetail(failureInfo)
{
    var failureTypes = failureInfo.failureTypeList.join(' ');

    var content = $('.results-detail .content');

    if ($('.failure-details', content).attr(config.kBuilderNameAttr) == failureInfo.builderName &&
        $('.failure-details', content).attr(config.kTestNameAttr) == failureInfo.testName &&
        $('.failure-details', content).attr(config.kFailureTypesAttr) == failureTypes)
        return;

    displayOnButterbar('Loading...');

    results.fetchResultsURLs(failureInfo, function(resultsURLs) {
        var status = $('.results-detail .toolbar .status');

        status.empty();
        content.empty();

        // FIXME: We should be passing the full list of failing builder names as the fourth argument.
        status.append(ui.failureDetailsStatus(failureInfo, [failureInfo.builderName]));
        content.append(ui.failureDetails(resultsURLs));

        toggleButton($('.results-detail .actions .next'), g_resultsDetailsIterator.hasNext());
        toggleButton($('.results-detail .actions .previous'), g_resultsDetailsIterator.hasPrevious());
        toggleButton($('.results-detail .actions .rebaseline'), results.canRebaseline(failureInfo.failureTypeList));

        $('.failure-details', content).attr(config.kTestNameAttr, failureInfo.testName);
        $('.failure-details', content).attr(config.kBuilderNameAttr, failureInfo.builderName);
        $('.failure-details', content).attr(config.kFailureTypesAttr, failureTypes);

        if (!$('.results-detail').is(":visible"))
            $('.results-detail').fadeIn('fast', dismissButterbar);
        else
            dismissButterbar();
    });
}

function hideResultsDetail()
{
    $('.results-detail').fadeOut('fast', function() {
        $('.results-detail .status').empty();
        $('.results-detail .content').empty();
        // Strictly speaking, we don't need to clear g_resultsDetailsIterator,
        // but doing so helps the garbage collector free memory.
        g_resultsDetailsIterator = null;
    });
    checkout.updateExpectations(model.takeExpectationUpdateQueue(), function() {
        // FIXME: Should we confirm with the use before executing the queue?
        rebaseline(model.takeRebaselineQueue());
    });
}

function nextResultsDetail()
{
    if (g_resultsDetailsIterator.hasNext())
        g_resultsDetailsIterator.callNext();
    else
        hideResultsDetail();
}

function previousResultsDetail()
{
    g_resultsDetailsIterator.callPrevious();
}

function failureInfoFromResultsDetail()
{
    var failureDetails = $('.results-detail .failure-details');
    return {
        'builderName': failureDetails.attr(config.kBuilderNameAttr),
        'testName': failureDetails.attr(config.kTestNameAttr),
        'failureTypeList': failureDetails.attr(config.kFailureTypesAttr).split(' '),
    }
}

function addToRebaselineQueue()
{
    model.queueForRebaseline(failureInfoFromResultsDetail());
    nextResultsDetail();
}

function addToExpectationUpdateQueue()
{
    model.queueForExpectationUpdate(failureInfoFromResultsDetail());
    nextResultsDetail();
}

function selectedFailures()
{
    var failureInfoList = [];

    $('.test input:checkbox').each(function() {
        if (!this.checked)
            return;
        failureInfoList = failureInfoList.concat(ui.failureInfoListForSummary($(this).parents('.result')));
    });

    return failureInfoList;
}

function rebaselineSelected()
{
    rebaseline(selectedFailures());
}

function updateExpectationsForSelected()
{
    checkout.updateExpectations(selectedFailures(), $.noop);
}

function showSelectedFailures()
{
    var listOfArgumentArrays = selectedFailures().map(function(failureInfo) { return [failureInfo]; });
    g_resultsDetailsIterator = new base.CallbackIterator(showResultsDetail, listOfArgumentArrays);
    g_resultsDetailsIterator.callNext();
}

function rowsInRevisionRange(earliestRevision, latestRevision)
{
    var earliestRevision = base.asInteger(earliestRevision);
    var latestRevision = base.asInteger(latestRevision);

    var rows = [];
    $('.recent-history tr').each(function() {
        var revision = parseInt($(this).attr(config.kRevisionAttr));
        if (earliestRevision <= revision && revision <= latestRevision)
            rows.push(this);
    });

    return $(rows);
}

function rowsBeforeRevision(latestRevision)
{
    return rowsInRevisionRange(0, latestRevision)
}

function showRecentCommits()
{
    $('.recent-history').empty().append(ui.changelog(model.state.recentCommits));
}

function showBuilderProgress()
{
    $.each(model.state.resultsByBuilder, function(builderName, resultsTree) {
        var builderIndex = config.kBuilders.indexOf(builderName);
        rowsBeforeRevision(resultsTree.revision).each(function() {
            $($(this).children()[builderIndex]).addClass('built');
        });
    });
}

function showUnexpectedFailure(failureAnalysis)
{
    var impliedFirstFailingRevision = failureAnalysis.newestPassingRevision + 1;
    var regressionRows = rowsInRevisionRange(impliedFirstFailingRevision, failureAnalysis.oldestFailingRevision);

    $('.entry', regressionRows).addClass('possible-regression');

    var failureSummary = ui.summarizeFailure(failureAnalysis).attr(config.kRevisionAttr, impliedFirstFailingRevision);
    (regressionRows.length ? regressionRows : $('.recent-history tr')).last().after(failureSummary);

    // FIXME: We should just compute this for failureSummary instead of recomputing the whole page.
    showBuilderProgress();
}

function showFailingBuildersInfobar(builderNameList)
{
    if (builderNameList.length)
        displayInfobar(ui.infobarMessageForCompileErrors(builderNameList), kBuildFailedInfobarType);
    else
        hideInfobarIfOfType(kBuildFailedInfobarType);
}

function checkForLocalServer()
{
    checkout.isAvailable(function(available) {
        if (!available)
            displayOnButterbar('Run "webkit-patch garden-o-matic" to enable rebaseline features.')
    });
}

function update()
{
    displayOnButterbar('Loading...');
    builders.buildersFailingStepRequredForTestCoverage(showFailingBuildersInfobar);
    base.callInParallel([model.updateRecentCommits, model.updateResultsByBuilder], function() {
        showRecentCommits();
        showBuilderProgress();
        model.analyzeUnexpectedFailures(showUnexpectedFailure);
        dismissButterbar();
        checkForLocalServer();
    });
}

$('.show-selected-failures').live('click', showSelectedFailures);
$('.rebaseline-selected').live('click', rebaselineSelected);
$('.update-expectations-selected').live('click', updateExpectationsForSelected);
$('.refresh').live('click', update);

$('.results-detail .actions .next').live('click', nextResultsDetail);
$('.results-detail .actions .previous').live('click', previousResultsDetail);
$('.results-detail .actions .rebaseline').live('click', addToRebaselineQueue);
$('.results-detail .actions .update-expectation').live('click', addToExpectationUpdateQueue);
$('.results-detail .actions .dismiss').live('click', hideResultsDetail);

$(document).ready(function() {
    g_updateTimerId = window.setInterval(update, config.kUpdateFrequency);
    update();
});

})();
