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

BuildbotTesterQueueView = function(debugQueues, releaseQueues)
{
    BuildbotQueueView.call(this, debugQueues, releaseQueues);

    this.update();
};

BaseObject.addConstructorFunctions(BuildbotTesterQueueView);

BuildbotTesterQueueView.prototype = {
    constructor: BuildbotTesterQueueView,
    __proto__: BuildbotQueueView.prototype,

    update: function()
    {
        BuildbotQueueView.prototype.update.call(this);

        this.element.removeChildren();

        function appendBuilderQueueStatus(queue)
        {
            var pendingRunsCount = queue.pendingIterationsCount;
            if (pendingRunsCount) {
                var message = pendingRunsCount === 1 ? "pending test run" : "pending test runs";
                var status = new StatusLineView(message, StatusLineView.Status.Neutral, null, pendingRunsCount);
                this.element.appendChild(status.element);
            }

            var appendedStatus = false;

            var limit = 2;
            for (var i = 0; i < queue.iterations.length && limit > 0; ++i) {
                var iteration = queue.iterations[i];
                if (!iteration.loaded || !iteration.finished)
                    continue;

                --limit;

                var messageLinkElement = this.revisionLinksForIteration(iteration);

                var layoutTestResults = iteration.layoutTestResults || {failureCount: 0};
                var javascriptTestResults = iteration.javascriptTestResults || {failureCount: 0};
                var apiTestResults = iteration.apiTestResults || {failureCount: 0};
                var pythonTestResults = iteration.pythonTestResults || {failureCount: 0};
                var perlTestResults = iteration.perlTestResults || {failureCount: 0};
                var bindingTestResults = iteration.bindingTestResults || {errorOccurred: false};

                if (!iteration.failed) {
                    var status = new StatusLineView(messageLinkElement, StatusLineView.Status.Good, "all tests passed");
                    limit = 0;
                } else if (!layoutTestResults.failureCount && !javascriptTestResults.failureCount && !apiTestResults.failureCount && !pythonTestResults.failureCount && !perlTestResults.failureCount && !bindingTestResults.errorOccurred) {
                    // Something wrong happened, but it was not a test failure.
                    var url = iteration.queue.buildbot.buildPageURLForIteration(iteration);
                    var status = new StatusLineView(messageLinkElement, StatusLineView.Status.Danger, iteration.text, undefined, url);
                } else if (layoutTestResults.failureCount && !javascriptTestResults.failureCount && !apiTestResults.failureCount && !pythonTestResults.failureCount && !perlTestResults.failureCount && !bindingTestResults.errorOccurred) {
                    var url = iteration.queue.buildbot.layoutTestResultsURLForIteration(iteration);
                    var status = new StatusLineView(messageLinkElement, StatusLineView.Status.Bad, layoutTestResults.failureCount === 1 ? "layout test failure" : "layout test failures", layoutTestResults.tooManyFailures ? layoutTestResults.failureCount + "\uff0b" : layoutTestResults.failureCount, url);
                } else if (!layoutTestResults.failureCount && javascriptTestResults.failureCount && !apiTestResults.failureCount && !pythonTestResults.failureCount && !perlTestResults.failureCount && !bindingTestResults.errorOccurred) {
                    var url = iteration.queue.buildbot.javascriptTestResultsURLForIteration(iteration);
                    var status = new StatusLineView(messageLinkElement, StatusLineView.Status.Bad, javascriptTestResults.failureCount === 1 ? "javascript test failure" : "javascript test failures", javascriptTestResults.failureCount, url);
                } else if (!layoutTestResults.failureCount && !javascriptTestResults.failureCount && apiTestResults.failureCount && !pythonTestResults.failureCount && !perlTestResults.failureCount && !bindingTestResults.errorOccurred) {
                    var url = iteration.queue.buildbot.apiTestResultsURLForIteration(iteration);
                    var status = new StatusLineView(messageLinkElement, StatusLineView.Status.Bad, apiTestResults.failureCount === 1 ? "api test failure" : "api test failures", apiTestResults.failureCount, url);
                } else if (!layoutTestResults.failureCount && !javascriptTestResults.failureCount && !apiTestResults.failureCount && pythonTestResults.failureCount && !perlTestResults.failureCount && !bindingTestResults.errorOccurred) {
                    var url = iteration.queue.buildbot.webkitpyTestResultsURLForIteration(iteration);
                    var status = new StatusLineView(messageLinkElement, StatusLineView.Status.Bad, pythonTestResults.failureCount === 1 ? "webkitpy test failure" : "webkitpy test failures", pythonTestResults.failureCount, url);
                } else if (!layoutTestResults.failureCount && !javascriptTestResults.failureCount && !apiTestResults.failureCount && !pythonTestResults.failureCount && perlTestResults.failureCount && !bindingTestResults.errorOccurred) {
                    var url = iteration.queue.buildbot.webkitperlTestResultsURLForIteration(iteration);
                    var status = new StatusLineView(messageLinkElement, StatusLineView.Status.Bad, perlTestResults.failureCount === 1 ? "webkitperl test failure" : "webkitperl test failures", perlTestResults.failureCount, url);
                } else if (!layoutTestResults.failureCount && !javascriptTestResults.failureCount && !apiTestResults.failureCount && !pythonTestResults.failureCount && !perlTestResults.failureCount && bindingTestResults.errorOccurred) {
                    var url = iteration.queue.buildbot.bindingsTestResultsURLForIteration(iteration);
                    var status = new StatusLineView(messageLinkElement, StatusLineView.Status.Bad, "bindings tests failed", undefined, url);
                } else {
                    var url = iteration.queue.buildbot.buildPageURLForIteration(iteration);
                    var totalFailures = layoutTestResults.failureCount + javascriptTestResults.failureCount + apiTestResults.failureCount + pythonTestResults.failureCount + perlTestResults.failureCount + bindingTestResults.errorOccurred;
                    var status = new StatusLineView(messageLinkElement, StatusLineView.Status.Bad, totalFailures === 1 ? "test failure" : "test failures", totalFailures, url);
                }

                this.element.appendChild(status.element);
                appendedStatus = true;
            }

            if (!appendedStatus) {
                var status = new StatusLineView("unknown", StatusLineView.Status.Neutral, "last passing build");            
                this.element.appendChild(status.element);
            }
        }

        function appendBuild(queues, label)
        {
            if (!queues.length)
                return;

            var releaseLabel = document.createElement("label");
            releaseLabel.textContent = label;
            this.element.appendChild(releaseLabel);

            queues.forEach(appendBuilderQueueStatus.bind(this));
        }

        appendBuild.call(this, this.releaseQueues, "Release");
        appendBuild.call(this, this.debugQueues, "Debug");
    },
};
