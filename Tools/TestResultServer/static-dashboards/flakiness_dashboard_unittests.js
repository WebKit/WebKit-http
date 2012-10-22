// Copyright (C) 2011 Google Inc. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

function resetGlobals()
{
    allExpectations = null;
    allTests = null;
    g_expectationsByPlatform = {};
    g_resultsByBuilder = {};
    g_builders = {};
    g_allExpectations = null;
    g_allTestsTrie = null;
    g_currentState = {};
    g_crossDashboardState = {};
    for (var key in g_defaultCrossDashboardStateValues)
        g_crossDashboardState[key] = g_defaultCrossDashboardStateValues[key];
}

function runExpectationsTest(builder, test, expectations, modifiers)
{
    g_builders[builder] = true;

    // Put in some dummy results. processExpectations expects the test to be
    // there.
    var tests = {};
    tests[test] = {'results': [[100, 'F']], 'times': [[100, 0]]};
    g_resultsByBuilder[builder] = {'tests': tests};

    processExpectations();
    var resultsForTest = createResultsObjectForTest(test, builder);
    populateExpectationsData(resultsForTest);

    var message = 'Builder: ' + resultsForTest.builder + ' test: ' + resultsForTest.test;
    equal(resultsForTest.expectations, expectations, message);
    equal(resultsForTest.modifiers, modifiers, message);
}

test('flattenTrie', 1, function() {
    resetGlobals();
    var tests = {
        'bar.html': {'results': [[100, 'F']], 'times': [[100, 0]]},
        'foo': {
            'bar': {
                'baz.html': {'results': [[100, 'F']], 'times': [[100, 0]]},
            }
        }
    };
    var expectedFlattenedTests = {
        'bar.html': {'results': [[100, 'F']], 'times': [[100, 0]]},
        'foo/bar/baz.html': {'results': [[100, 'F']], 'times': [[100, 0]]},
    };
    equal(JSON.stringify(flattenTrie(tests)), JSON.stringify(expectedFlattenedTests))
});

test('releaseFail', 2, function() {
    resetGlobals();
    var builder = 'WebKit Win';
    var test = 'foo/1.html';
    var expectationsArray = [
        {'modifiers': 'RELEASE', 'expectations': 'FAIL'}
    ];
    g_expectationsByPlatform['CHROMIUM'] = getParsedExpectations('[ Release ] ' + test + ' [ Failure ]');
    runExpectationsTest(builder, test, 'FAIL', 'RELEASE');
});

test('releaseFailDebugCrashReleaseBuilder', 2, function() {
    resetGlobals();
    var builder = 'WebKit Win';
    var test = 'foo/1.html';
    var expectationsArray = [
        {'modifiers': 'RELEASE', 'expectations': 'FAIL'},
        {'modifiers': 'DEBUG', 'expectations': 'CRASH'}
    ];
    g_expectationsByPlatform['CHROMIUM'] = getParsedExpectations('[ Release ] ' + test + ' [ Failure ]\n' +
        '[ Debug ] ' + test + ' [ Crash ]');
    runExpectationsTest(builder, test, 'FAIL', 'RELEASE');
});

test('releaseFailDebugCrashDebugBuilder', 2, function() {
    resetGlobals();
    var builder = 'WebKit Win (dbg)';
    var test = 'foo/1.html';
    var expectationsArray = [
        {'modifiers': 'RELEASE', 'expectations': 'FAIL'},
        {'modifiers': 'DEBUG', 'expectations': 'CRASH'}
    ];
    g_expectationsByPlatform['CHROMIUM'] = getParsedExpectations('[ Release ] ' + test + ' [ Failure ]\n' +
        '[ Debug ] ' + test + ' [ Crash ]');
    runExpectationsTest(builder, test, 'CRASH', 'DEBUG');
});

test('overrideJustBuildType', 12, function() {
    resetGlobals();
    var test = 'bar/1.html';
    g_expectationsByPlatform['CHROMIUM'] = getParsedExpectations('bar [ WontFix Failure Pass Timeout ]\n' +
        '[ Mac ] ' + test + ' [ WontFix Failure ]\n' +
        '[ Linux Debug ] ' + test + ' [ Crash ]');
    
    runExpectationsTest('WebKit Win', test, 'FAIL PASS TIMEOUT', 'WONTFIX');
    runExpectationsTest('WebKit Win (dbg)(3)', test, 'FAIL PASS TIMEOUT', 'WONTFIX');
    runExpectationsTest('WebKit Linux', test, 'FAIL PASS TIMEOUT', 'WONTFIX');
    runExpectationsTest('WebKit Linux (dbg)(3)', test, 'CRASH', 'LINUX DEBUG');
    runExpectationsTest('WebKit Mac10.7', test, 'FAIL', 'MAC WONTFIX');
    runExpectationsTest('WebKit Mac10.7 (dbg)(3)', test, 'FAIL', 'MAC WONTFIX');
});

test('platformAndBuildType', 78, function() {
    var runPlatformAndBuildTypeTest = function(builder, expectedPlatform, expectedBuildType) {
        g_perBuilderPlatformAndBuildType = {};
        buildInfo = platformAndBuildType(builder);
        var message = 'Builder: ' + builder;
        equal(buildInfo.platform, expectedPlatform, message);
        equal(buildInfo.buildType, expectedBuildType, message);
    }
    runPlatformAndBuildTypeTest('WebKit Win (deps)', 'CHROMIUM_XP', 'RELEASE');
    runPlatformAndBuildTypeTest('WebKit Win (deps)(dbg)(1)', 'CHROMIUM_XP', 'DEBUG');
    runPlatformAndBuildTypeTest('WebKit Win (deps)(dbg)(2)', 'CHROMIUM_XP', 'DEBUG');
    runPlatformAndBuildTypeTest('WebKit Linux (deps)', 'CHROMIUM_LUCID', 'RELEASE');
    runPlatformAndBuildTypeTest('WebKit Linux (deps)(dbg)(1)', 'CHROMIUM_LUCID', 'DEBUG');
    runPlatformAndBuildTypeTest('WebKit Linux (deps)(dbg)(2)', 'CHROMIUM_LUCID', 'DEBUG');
    runPlatformAndBuildTypeTest('WebKit Mac10.6 (deps)', 'CHROMIUM_SNOWLEOPARD', 'RELEASE');
    runPlatformAndBuildTypeTest('WebKit Mac10.6 (deps)(dbg)(1)', 'CHROMIUM_SNOWLEOPARD', 'DEBUG');
    runPlatformAndBuildTypeTest('WebKit Mac10.6 (deps)(dbg)(2)', 'CHROMIUM_SNOWLEOPARD', 'DEBUG');
    runPlatformAndBuildTypeTest('WebKit Win', 'CHROMIUM_XP', 'RELEASE');
    runPlatformAndBuildTypeTest('WebKit Win7', 'CHROMIUM_WIN7', 'RELEASE');
    runPlatformAndBuildTypeTest('WebKit Win (dbg)(1)', 'CHROMIUM_XP', 'DEBUG');
    runPlatformAndBuildTypeTest('WebKit Win (dbg)(2)', 'CHROMIUM_XP', 'DEBUG');
    runPlatformAndBuildTypeTest('WebKit Linux', 'CHROMIUM_LUCID', 'RELEASE');
    runPlatformAndBuildTypeTest('WebKit Linux 32', 'CHROMIUM_LUCID', 'RELEASE');
    runPlatformAndBuildTypeTest('WebKit Linux (dbg)(1)', 'CHROMIUM_LUCID', 'DEBUG');
    runPlatformAndBuildTypeTest('WebKit Linux (dbg)(2)', 'CHROMIUM_LUCID', 'DEBUG');
    runPlatformAndBuildTypeTest('WebKit Mac10.6', 'CHROMIUM_SNOWLEOPARD', 'RELEASE');
    runPlatformAndBuildTypeTest('WebKit Mac10.6 (dbg)', 'CHROMIUM_SNOWLEOPARD', 'DEBUG');
    runPlatformAndBuildTypeTest('XP Tests', 'CHROMIUM_XP', 'RELEASE');
    runPlatformAndBuildTypeTest('Interactive Tests (dbg)', 'CHROMIUM_XP', 'DEBUG');
    
    g_crossDashboardState.group = '@ToT - webkit.org';
    g_crossDashboardState.testType = 'layout-tests';
    runPlatformAndBuildTypeTest('Chromium Win Release (Tests)', 'CHROMIUM_XP', 'RELEASE');
    runPlatformAndBuildTypeTest('Chromium Linux Release (Tests)', 'CHROMIUM_LUCID', 'RELEASE');
    runPlatformAndBuildTypeTest('Chromium Mac Release (Tests)', 'CHROMIUM_SNOWLEOPARD', 'RELEASE');
    
    // FIXME: These platforms should match whatever we use in the TestExpectations format.
    runPlatformAndBuildTypeTest('Lion Release (Tests)', 'APPLE_MAC_LION_WK1', 'RELEASE');
    runPlatformAndBuildTypeTest('Lion Debug (Tests)', 'APPLE_MAC_LION_WK1', 'DEBUG');
    runPlatformAndBuildTypeTest('SnowLeopard Intel Release (Tests)', 'APPLE_MAC_SNOWLEOPARD_WK1', 'RELEASE');
    runPlatformAndBuildTypeTest('SnowLeopard Intel Leaks', 'APPLE_MAC_SNOWLEOPARD_WK1', 'RELEASE');
    runPlatformAndBuildTypeTest('SnowLeopard Intel Debug (Tests)', 'APPLE_MAC_SNOWLEOPARD_WK1', 'DEBUG');
    runPlatformAndBuildTypeTest('GTK Linux 32-bit Release', 'GTK_LINUX_WK1', 'RELEASE');
    runPlatformAndBuildTypeTest('GTK Linux 32-bit Debug', 'GTK_LINUX_WK1', 'DEBUG');
    runPlatformAndBuildTypeTest('GTK Linux 64-bit Debug', 'GTK_LINUX_WK1', 'DEBUG');
    runPlatformAndBuildTypeTest('GTK Linux 64-bit Debug WK2', 'GTK_LINUX_WK2', 'DEBUG');
    runPlatformAndBuildTypeTest('Qt Linux Release', 'QT_LINUX', 'RELEASE');
    runPlatformAndBuildTypeTest('Windows 7 Release (Tests)', 'APPLE_WIN_WIN7', 'RELEASE');
    runPlatformAndBuildTypeTest('Windows XP Debug (Tests)', 'APPLE_WIN_XP', 'DEBUG');
    
    // FIXME: Should WebKit2 be it's own platform?
    runPlatformAndBuildTypeTest('SnowLeopard Intel Release (WebKit2 Tests)', 'APPLE_MAC_SNOWLEOPARD_WK2', 'RELEASE');
    runPlatformAndBuildTypeTest('SnowLeopard Intel Debug (WebKit2 Tests)', 'APPLE_MAC_SNOWLEOPARD_WK2', 'DEBUG');
    runPlatformAndBuildTypeTest('Windows 7 Release (WebKit2 Tests)', 'APPLE_WIN_WIN7', 'RELEASE');    
});

test('realModifiers', 3, function() {
    equal(realModifiers('BUG(Foo) LINUX LION WIN DEBUG SLOW'), 'SLOW');
    equal(realModifiers('BUG(Foo) LUCID MAC XP RELEASE SKIP'), 'SKIP');
    equal(realModifiers('BUG(Foo)'), '');
});

test('allTestsWithSamePlatformAndBuildType', 1, function() {
    // FIXME: test that allTestsWithSamePlatformAndBuildType actually returns the right set of tests.
    var expectedPlatformsList = ['CHROMIUM_LION', 'CHROMIUM_SNOWLEOPARD', 'CHROMIUM_XP', 'CHROMIUM_VISTA', 'CHROMIUM_WIN7', 'CHROMIUM_LUCID',
                                 'CHROMIUM_ANDROID', 'APPLE_MAC_LION_WK1', 'APPLE_MAC_LION_WK2', 'APPLE_MAC_SNOWLEOPARD_WK1', 'APPLE_MAC_SNOWLEOPARD_WK2',
                                 'APPLE_WIN_XP', 'APPLE_WIN_WIN7',  'GTK_LINUX_WK1', 'GTK_LINUX_WK2', 'QT_LINUX', 'EFL_LINUX_WK1', 'EFL_LINUX_WK2'];
    var actualPlatformsList = Object.keys(g_allTestsByPlatformAndBuildType);
    deepEqual(expectedPlatformsList, actualPlatformsList);
});

test('filterBugs',4, function() {
    var filtered = filterBugs('Skip crbug.com/123 webkit.org/b/123 Slow Bug(Tony) Debug')
    equal(filtered.modifiers, 'Skip Slow Debug');
    equal(filtered.bugs, 'crbug.com/123 webkit.org/b/123 Bug(Tony)');

    filtered = filterBugs('Skip Slow Debug')
    equal(filtered.modifiers, 'Skip Slow Debug');
    equal(filtered.bugs, '');
});

test('getExpectations', 16, function() {
    resetGlobals();
    g_builders['WebKit Win'] = true;
    g_resultsByBuilder = {
        'WebKit Win': {
            'tests': {
                'foo/test1.html': {'results': [[100, 'F']], 'times': [[100, 0]]},
                'foo/test2.html': {'results': [[100, 'F']], 'times': [[100, 0]]},
                'foo/test3.html': {'results': [[100, 'F']], 'times': [[100, 0]]},
                'test1.html': {'results': [[100, 'F']], 'times': [[100, 0]]}
            }
        }
    }

    g_expectationsByPlatform['CHROMIUM'] = getParsedExpectations('Bug(123) foo [ Failure Pass Crash ]\n' +
        'Bug(Foo) [ Release ] foo/test1.html [ Failure ]\n' +
        '[ Debug ] foo/test1.html [ Crash ]\n' +
        'Bug(456) foo/test2.html [ Failure ]\n' +
        '[ Linux Debug ] foo/test2.html [ Crash ]\n' +
        '[ Release ] test1.html [ Failure ]\n' +
        '[ Debug ] test1.html [ Crash ]\n');
    g_expectationsByPlatform['CHROMIUM_ANDROID'] = getParsedExpectations('Bug(654) foo/test2.html [ Crash ]\n');

    g_expectationsByPlatform['GTK'] = getParsedExpectations('Bug(42) foo/test2.html [ ImageOnlyFailure ]\n' +
        '[ Debug ] test1.html [ Crash ]\n');
    g_expectationsByPlatform['GTK_LINUX_WK1'] = getParsedExpectations('[ Release ] foo/test1.html [ ImageOnlyFailure ]\n' +
        'Bug(789) foo/test2.html [ Crash ]\n');
    g_expectationsByPlatform['GTK_LINUX_WK2'] = getParsedExpectations('Bug(987) foo/test2.html [ Failure ]\n');

    processExpectations();
    
    var expectations = getExpectations('foo/test1.html', 'CHROMIUM_XP', 'DEBUG');
    equal(JSON.stringify(expectations), '{"modifiers":"DEBUG","expectations":"CRASH"}');

    var expectations = getExpectations('foo/test1.html', 'CHROMIUM_LUCID', 'RELEASE');
    equal(JSON.stringify(expectations), '{"modifiers":"Bug(Foo) RELEASE","expectations":"FAIL"}');

    var expectations = getExpectations('foo/test2.html', 'CHROMIUM_LUCID', 'RELEASE');
    equal(JSON.stringify(expectations), '{"modifiers":"Bug(456)","expectations":"FAIL"}');

    var expectations = getExpectations('foo/test2.html', 'CHROMIUM_LION', 'DEBUG');
    equal(JSON.stringify(expectations), '{"modifiers":"Bug(456)","expectations":"FAIL"}');

    var expectations = getExpectations('foo/test2.html', 'CHROMIUM_LUCID', 'DEBUG');
    equal(JSON.stringify(expectations), '{"modifiers":"LINUX DEBUG","expectations":"CRASH"}');

    var expectations = getExpectations('foo/test2.html', 'CHROMIUM_ANDROID', 'RELEASE');
    equal(JSON.stringify(expectations), '{"modifiers":"Bug(654)","expectations":"CRASH"}');

    var expectations = getExpectations('test1.html', 'CHROMIUM_ANDROID', 'RELEASE');
    equal(JSON.stringify(expectations), '{"modifiers":"RELEASE","expectations":"FAIL"}');

    var expectations = getExpectations('foo/test3.html', 'CHROMIUM_LUCID', 'DEBUG');
    equal(JSON.stringify(expectations), '{"modifiers":"Bug(123)","expectations":"FAIL PASS CRASH"}');

    var expectations = getExpectations('test1.html', 'CHROMIUM_XP', 'DEBUG');
    equal(JSON.stringify(expectations), '{"modifiers":"DEBUG","expectations":"CRASH"}');

    var expectations = getExpectations('test1.html', 'CHROMIUM_LUCID', 'RELEASE');
    equal(JSON.stringify(expectations), '{"modifiers":"RELEASE","expectations":"FAIL"}');

    var expectations = getExpectations('foo/test1.html', 'GTK_LINUX_WK1', 'RELEASE');
    equal(JSON.stringify(expectations), '{"modifiers":"RELEASE","expectations":"IMAGE"}');

    var expectations = getExpectations('foo/test2.html', 'GTK_LINUX_WK1', 'DEBUG');
    equal(JSON.stringify(expectations), '{"modifiers":"Bug(789)","expectations":"CRASH"}');

    var expectations = getExpectations('test1.html', 'GTK_LINUX_WK1', 'DEBUG');
    equal(JSON.stringify(expectations), '{"modifiers":"DEBUG","expectations":"CRASH"}');

    var expectations = getExpectations('foo/test2.html', 'GTK_LINUX_WK2', 'DEBUG');
    equal(JSON.stringify(expectations), '{"modifiers":"Bug(987)","expectations":"FAIL"}');

    var expectations = getExpectations('foo/test2.html', 'GTK_LINUX_WK2', 'RELEASE');
    equal(JSON.stringify(expectations), '{"modifiers":"Bug(987)","expectations":"FAIL"}');

    var expectations = getExpectations('test1.html', 'GTK_LINUX_WK2', 'DEBUG');
    equal(JSON.stringify(expectations), '{"modifiers":"DEBUG","expectations":"CRASH"}');
});

test('substringList', 2, function() {
    g_crossDashboardState.testType = 'gtest';
    g_currentState.tests = 'test.FLAKY_foo test.FAILS_foo1 test.DISABLED_foo2 test.MAYBE_foo3 test.foo4';
    equal(substringList().toString(), 'test.foo,test.foo1,test.foo2,test.foo3,test.foo4');

    g_crossDashboardState.testType = 'layout-tests';
    g_currentState.tests = 'foo/bar.FLAKY_foo.html';
    equal(substringList().toString(), 'foo/bar.FLAKY_foo.html');
});

test('htmlForTestsWithExpectationsButNoFailures', 4, function() {
    var builder = 'WebKit Win';
    g_perBuilderWithExpectationsButNoFailures[builder] = ['passing-test1.html', 'passing-test2.html'];
    g_perBuilderSkippedPaths[builder] = ['skipped-test1.html'];
    g_resultsByBuilder[builder] = { buildNumbers: [5, 4, 3, 1] };

    g_currentState.showUnexpectedPasses = true;
    g_currentState.showSkipped = true;

    g_crossDashboardState.group = '@ToT - chromium.org';
    g_crossDashboardState.testType = 'layout-tests';
    
    var container = document.createElement('div');
    container.innerHTML = htmlForTestsWithExpectationsButNoFailures(builder);
    equal(container.querySelectorAll('#passing-tests > div').length, 2);
    equal(container.querySelectorAll('#skipped-tests > div').length, 1);
    
    g_currentState.showUnexpectedPasses = false;
    g_currentState.showSkipped = false;
    
    var container = document.createElement('div');
    container.innerHTML = htmlForTestsWithExpectationsButNoFailures(builder);
    equal(container.querySelectorAll('#passing-tests > div').length, 0);
    equal(container.querySelectorAll('#skipped-tests > div').length, 0);
});

test('headerForTestTableHtml', 1, function() {
    var container = document.createElement('div');
    container.innerHTML = headerForTestTableHtml();
    equal(container.querySelectorAll('input').length, 5);
});

test('htmlForTestTypeSwitcherGroup', 6, function() {
    var container = document.createElement('div');
    g_crossDashboardState.testType = 'ui_tests';
    container.innerHTML = htmlForTestTypeSwitcher(true);
    var selects = container.querySelectorAll('select');
    equal(selects.length, 2);
    var group = selects[1];
    equal(group.parentNode.textContent.indexOf('Group:'), 0);
    equal(group.children.length, 3);

    g_crossDashboardState.testType = 'layout-tests';
    container.innerHTML = htmlForTestTypeSwitcher(true);
    var selects = container.querySelectorAll('select');
    equal(selects.length, 2);
    var group = selects[1];
    equal(group.parentNode.textContent.indexOf('Group:'), 0);
    equal(group.children.length, 4);
});

test('htmlForIndividualTestOnAllBuilders', 1, function() {
    resetGlobals();
    equal(htmlForIndividualTestOnAllBuilders('foo/nonexistant.html'), '<div class="not-found">Test not found. Either it does not exist, is skipped or passes on all platforms.</div>');
});

test('htmlForIndividualTestOnAllBuildersWithResultsLinksNonexistant', 1, function() {
    resetGlobals();
    equal(htmlForIndividualTestOnAllBuildersWithResultsLinks('foo/nonexistant.html'),
        '<div class="not-found">Test not found. Either it does not exist, is skipped or passes on all platforms.</div>' +
        '<div class=expectations test=foo/nonexistant.html>' +
            '<div>' +
                '<span class=link onclick="setQueryParameter(\'showExpectations\', true)">Show results</span> | ' +
                '<span class=link onclick="setQueryParameter(\'showLargeExpectations\', true)">Show large thumbnails</span> | ' +
                '<b>Only shows actual results/diffs from the most recent *failure* on each bot.</b>' +
            '</div>' +
        '</div>');
});

test('htmlForIndividualTestOnAllBuildersWithResultsLinks', 1, function() {
    resetGlobals();
    var test = 'dummytest.html';
    var builderName = 'dummyBuilder';
    BUILDER_TO_MASTER[builderName] = CHROMIUM_BUILDER_MASTER;
    g_testToResultsMap[test] = [createResultsObjectForTest(test, builderName)];

    equal(htmlForIndividualTestOnAllBuildersWithResultsLinks(test),
        '<table class=test-table><thead><tr>' +
                '<th sortValue=test><div class=table-header-content><span></span><span class=header-text>test</span></div></th>' +
                '<th sortValue=bugs><div class=table-header-content><span></span><span class=header-text>bugs</span></div></th>' +
                '<th sortValue=modifiers><div class=table-header-content><span></span><span class=header-text>modifiers</span></div></th>' +
                '<th sortValue=expectations><div class=table-header-content><span></span><span class=header-text>expectations</span></div></th>' +
                '<th sortValue=slowest><div class=table-header-content><span></span><span class=header-text>slowest run</span></div></th>' +
                '<th sortValue=flakiness colspan=10000><div class=table-header-content><span></span><span class=header-text>flakiness (numbers are runtimes in seconds)</span></div></th>' +
            '</tr></thead>' +
            '<tbody></tbody>' +
        '</table>' +
        '<div>The following builders either don\'t run this test (e.g. it\'s skipped) or all runs passed:</div>' +
        '<div class=skipped-builder-list>' +
            '<div class=skipped-builder>WebKit Linux</div><div class=skipped-builder>WebKit Linux (dbg)</div>' +
            '<div class=skipped-builder>WebKit Mac10.7</div><div class=skipped-builder>WebKit Win</div>' +
        '</div>' +
        '<div class=expectations test=dummytest.html>' +
            '<div><span class=link onclick="setQueryParameter(\'showExpectations\', true)">Show results</span> | ' +
            '<span class=link onclick="setQueryParameter(\'showLargeExpectations\', true)">Show large thumbnails</span> | ' +
            '<b>Only shows actual results/diffs from the most recent *failure* on each bot.</b></div>' +
        '</div>');
});

test('htmlForIndividualTestOnAllBuildersWithResultsLinksWebkitMaster', 1, function() {
    resetGlobals();
    var test = 'dummytest.html';
    var builderName = 'dummyBuilder';
    BUILDER_TO_MASTER[builderName] = WEBKIT_BUILDER_MASTER;
    g_testToResultsMap[test] = [createResultsObjectForTest(test, builderName)];

    equal(htmlForIndividualTestOnAllBuildersWithResultsLinks(test),
        '<table class=test-table><thead><tr>' +
                '<th sortValue=test><div class=table-header-content><span></span><span class=header-text>test</span></div></th>' +
                '<th sortValue=bugs><div class=table-header-content><span></span><span class=header-text>bugs</span></div></th>' +
                '<th sortValue=modifiers><div class=table-header-content><span></span><span class=header-text>modifiers</span></div></th>' +
                '<th sortValue=expectations><div class=table-header-content><span></span><span class=header-text>expectations</span></div></th>' +
                '<th sortValue=slowest><div class=table-header-content><span></span><span class=header-text>slowest run</span></div></th>' +
                '<th sortValue=flakiness colspan=10000><div class=table-header-content><span></span><span class=header-text>flakiness (numbers are runtimes in seconds)</span></div></th>' +
            '</tr></thead>' +
            '<tbody></tbody>' +
        '</table>' +
        '<div>The following builders either don\'t run this test (e.g. it\'s skipped) or all runs passed:</div>' +
        '<div class=skipped-builder-list>' +
            '<div class=skipped-builder>WebKit Linux</div><div class=skipped-builder>WebKit Linux (dbg)</div>' +
            '<div class=skipped-builder>WebKit Mac10.7</div><div class=skipped-builder>WebKit Win</div>' +
        '</div>' +
        '<div class=expectations test=dummytest.html>' +
            '<div><span class=link onclick="setQueryParameter(\'showExpectations\', true)">Show results</span> | ' +
            '<span class=link onclick="setQueryParameter(\'showLargeExpectations\', true)">Show large thumbnails</span>' +
            '<form onsubmit="setQueryParameter(\'revision\', revision.value);return false;">' +
                'Show results for WebKit revision: <input name=revision placeholder="e.g. 65540" value="" id=revision-input>' +
            '</form></div>' +
        '</div>');
});

test('htmlForIndividualTests', 4, function() {
    resetGlobals();
    var test1 = 'foo/nonexistant.html';
    var test2 = 'bar/nonexistant.html';

    g_currentState.showChrome = false;

    var tests = [test1, test2];
    equal(htmlForIndividualTests(tests),
        '<h2><a href="http://trac.webkit.org/browser/trunk/LayoutTests/foo/nonexistant.html" target="_blank">foo/nonexistant.html</a></h2>' +
        htmlForIndividualTestOnAllBuilders(test1) + 
        '<div class=expectations test=foo/nonexistant.html>' +
            '<div><span class=link onclick=\"setQueryParameter(\'showExpectations\', true)\">Show results</span> | ' +
            '<span class=link onclick=\"setQueryParameter(\'showLargeExpectations\', true)\">Show large thumbnails</span> | ' +
            '<b>Only shows actual results/diffs from the most recent *failure* on each bot.</b></div>' +
        '</div>' +
        '<hr>' +
        '<h2><a href="http://trac.webkit.org/browser/trunk/LayoutTests/bar/nonexistant.html" target="_blank">bar/nonexistant.html</a></h2>' +
        htmlForIndividualTestOnAllBuilders(test2) +
        '<div class=expectations test=bar/nonexistant.html>' +
            '<div><span class=link onclick=\"setQueryParameter(\'showExpectations\', true)\">Show results</span> | ' +
            '<span class=link onclick=\"setQueryParameter(\'showLargeExpectations\', true)\">Show large thumbnails</span> | ' +
            '<b>Only shows actual results/diffs from the most recent *failure* on each bot.</b></div>' +
        '</div>');

    tests = [test1];
    equal(htmlForIndividualTests(tests), htmlForIndividualTestOnAllBuilders(test1) +
        '<div class=expectations test=foo/nonexistant.html>' +
            '<div><span class=link onclick=\"setQueryParameter(\'showExpectations\', true)\">Show results</span> | ' +
            '<span class=link onclick=\"setQueryParameter(\'showLargeExpectations\', true)\">Show large thumbnails</span> | ' +
            '<b>Only shows actual results/diffs from the most recent *failure* on each bot.</b></div>' +
        '</div>');

    g_currentState.showChrome = true;

    equal(htmlForIndividualTests(tests),
        '<h2><a href="http://trac.webkit.org/browser/trunk/LayoutTests/foo/nonexistant.html" target="_blank">foo/nonexistant.html</a></h2>' +
        htmlForIndividualTestOnAllBuildersWithResultsLinks(test1));

    tests = [test1, test2];
    equal(htmlForIndividualTests(tests),
        '<h2><a href="http://trac.webkit.org/browser/trunk/LayoutTests/foo/nonexistant.html" target="_blank">foo/nonexistant.html</a></h2>' +
        htmlForIndividualTestOnAllBuildersWithResultsLinks(test1) + '<hr>' +
        '<h2><a href="http://trac.webkit.org/browser/trunk/LayoutTests/bar/nonexistant.html" target="_blank">bar/nonexistant.html</a></h2>' +
        htmlForIndividualTestOnAllBuildersWithResultsLinks(test2));
});

test('htmlForSingleTestRow', 1, function() {
    resetGlobals();
    var builder = 'dummyBuilder';
    BUILDER_TO_MASTER[builder] = CHROMIUM_WEBKIT_BUILDER_MASTER;
    var test = createResultsObjectForTest('foo/exists.html', builder);
    g_currentState.showCorrectExpectations = true;
    g_resultsByBuilder[builder] = {buildNumbers: [2, 1], webkitRevision: [1234, 1233]};
    test.rawResults = [[1, 'F'], [2, 'I']];
    test.rawTimes = [[1, 0], [2, 5]];
    var expected = '<tr>' +
        '<td class="test-link"><span class="link" onclick="setQueryParameter(\'tests\',\'foo/exists.html\');">foo/exists.html</span>' +
        '<td class=options-container><a href="https://bugs.webkit.org/enter_bug.cgi?assigned_to=webkit-unassigned%40lists.webkit.org&product=WebKit&form_name=enter_bug&component=Tools%20%2F%20Tests&short_desc=Layout%20Test%20foo%2Fexists.html%20is%20failing&comment=The%20following%20layout%20test%20is%20failing%20on%20%5Binsert%20platform%5D%0A%0Afoo%2Fexists.html%0A%0AProbable%20cause%3A%0A%0A%5Binsert%20probable%20cause%5D" class="file-bug">FILE BUG</a>' +
        '<td class=options-container>' +
            '<td class=options-container>' +
                '<td><td title="TEXT. Click for more info." class="results F merge" onclick=\'showPopupForBuild(event, "dummyBuilder",0,"foo/exists.html")\'>&nbsp;' +
                '<td title="IMAGE. Click for more info." class="results I" onclick=\'showPopupForBuild(event, "dummyBuilder",1,"foo/exists.html")\'>5';

    equal(htmlForSingleTestRow(test), expected);
});

test('lookupVirtualTestSuite', 2, function() {
    equal(lookupVirtualTestSuite('fast/canvas/foo.html'), '');
    equal(lookupVirtualTestSuite('platform/chromium/virtual/gpu/fast/canvas/foo.html'), 'platform/chromium/virtual/gpu/fast/canvas');
});

test('baseTest', 2, function() {
    equal(baseTest('fast/canvas/foo.html', ''), 'fast/canvas/foo.html');
    equal(baseTest('platform/chromium/virtual/gpu/fast/canvas/foo.html', 'platform/chromium/virtual/gpu/fast/canvas'), 'fast/canvas/foo.html');
});

// FIXME: Create builders_tests.js and move this there.
test('generateChromiumDepsFyiGpuBuildersFromBuilderList', 1, function() {
    var builderList = ["Linux Audio", "Linux Release (ATI)", "Linux Release (Intel)", "Mac Release (ATI)", "Win7 Audio", "Win7 Release (ATI)", "Win7 Release (Intel)", "WinXP Debug (NVIDIA)", "WinXP Release (NVIDIA)"];
    var expectedBuilders = [["Linux Release (ATI)", 2], ["Linux Release (Intel)"], ["Mac Release (ATI)"], ["Win7 Release (ATI)"], ["Win7 Release (Intel)"], ["WinXP Debug (NVIDIA)"], ["WinXP Release (NVIDIA)"] ];
    deepEqual(generateBuildersFromBuilderList(builderList, isChromiumDepsFyiGpuTestRunner), expectedBuilders);
});

test('generateChromiumTipOfTreeGpuBuildersFromBuilderList', 1, function() {
    var builderList = ["Chrome Frame Tests", "GPU Linux (NVIDIA)", "GPU Linux (dbg) (NVIDIA)", "GPU Mac", "GPU Mac (dbg)", "GPU Win7 (NVIDIA)", "GPU Win7 (dbg) (NVIDIA)", "Linux Perf",
        "Linux Tests", "Linux Valgrind", "Mac Builder (dbg)", "Mac10.6 Perf", "Mac10.6 Tests", "Vista Perf", "Vista Tests", "WebKit Linux", "WebKit Linux ASAN", "WebKit Linux (dbg)", "WebKit Linux (deps)",
        "WebKit Linux 32", "WebKit Mac Builder", "WebKit Mac Builder (dbg)", "WebKit Mac Builder (deps)",
        "WebKit Mac10.6", "WebKit Mac10.6 (dbg)", "WebKit Mac10.6 (deps)", "WebKit Mac10.7", "WebKit Win", "WebKit Win (dbg)(1)", "WebKit Win (dbg)(2)",
        "WebKit Win (deps)", "WebKit Win Builder", "WebKit Win Builder (dbg)", "WebKit Win Builder (deps)", "WebKit Win7", "Win (dbg)", "Win Builder"];
    var expectedBuilders = [["GPU Linux (NVIDIA)", 2], ["GPU Linux (dbg) (NVIDIA)"], ["GPU Mac"], ["GPU Mac (dbg)"], ["GPU Win7 (NVIDIA)"], ["GPU Win7 (dbg) (NVIDIA)"]];
    deepEqual(generateBuildersFromBuilderList(builderList, isChromiumTipOfTreeGpuTestRunner), expectedBuilders);
});

test('generateWebkitBuildersFromBuilderList', 1, function() {
    var builderList = ["Chromium Android Release", "Chromium Linux Release", "Chromium Linux Release (Grid Layout)", "Chromium Linux Release (Perf)", "Chromium Linux Release (Tests)",
        "Chromium Mac Release", "Chromium Mac Release (Perf)", "Chromium Mac Release (Tests)", "Chromium Win Release", "Chromium Win Release (Perf)", "Chromium Win Release (Tests)",
        "EFL Linux Release", "GTK Linux 32-bit Release", "GTK Linux 64-bit Debug", "GTK Linux 64-bit Release", "Lion Debug (Build)", "Lion Debug (Tests)", "Lion Debug (WebKit2 Tests)",
        "Lion Leaks", "Lion Release (Build)", "Lion Release (Perf)", "Lion Release (Tests)", "Lion Release (WebKit2 Tests)", "Qt Linux 64-bit Release (Perf)",
        "Qt Linux 64-bit Release (WebKit2 Perf)", "Qt Linux ARMv7 Release", "Qt Linux MIPS Release", "Qt Linux Release", "Qt Linux Release minimal", "Qt Linux SH4 Release",
        "Qt SnowLeopard Release", "Qt Windows 32-bit Debug", "Qt Windows 32-bit Release", "SnowLeopard Intel Debug (Build)", "SnowLeopard Intel Debug (Tests)",
        "SnowLeopard Intel Debug (WebKit2 Tests)", "SnowLeopard Intel Release (Build)", "SnowLeopard Intel Release (Tests)", "SnowLeopard Intel Release (WebKit2 Tests)",
        "WinCE Release (Build)", "WinCairo Release", "Windows 7 Release (Tests)", "Windows 7 Release (WebKit2 Tests)", "Windows Debug (Build)", "Windows Release (Build)", "Windows XP Debug (Tests)",
        "EFL Linux 32-bit Release (Build)", "EFL Linux 64-bit Debug"];
    var expectedBuilders = [["Chromium Linux Release (Tests)", 2], ["Chromium Mac Release (Tests)"], ["EFL Linux Release"], ["GTK Linux 32-bit Release"], ["GTK Linux 64-bit Debug"],
        ["GTK Linux 64-bit Release"], ["Lion Debug (Tests)"], ["Lion Debug (WebKit2 Tests)"], ["Lion Release (Tests)"], ["Lion Release (WebKit2 Tests)"], ["Qt Linux Release"],
        ["SnowLeopard Intel Debug (Tests)"], ["SnowLeopard Intel Debug (WebKit2 Tests)"], ["SnowLeopard Intel Release (Tests)"], ["SnowLeopard Intel Release (WebKit2 Tests)"], ["EFL Linux 64-bit Debug"]];
    deepEqual(generateBuildersFromBuilderList(builderList, isWebkitTestRunner), expectedBuilders);
});

test('generateChromiumWebkitTipOfTreeBuildersFromBuilderList', 1, function() {
    var builderList = ["Chrome Frame Tests", "GPU Linux (NVIDIA)", "GPU Linux (dbg) (NVIDIA)", "GPU Mac", "GPU Mac (dbg)", "GPU Win7 (NVIDIA)", "GPU Win7 (dbg) (NVIDIA)", "Linux Perf", "Linux Tests",
        "Linux Valgrind", "Mac Builder (dbg)", "Mac10.6 Perf", "Mac10.6 Tests", "Vista Perf", "Vista Tests", "WebKit Linux", "WebKit Linux ASAN",  "WebKit Linux (dbg)", "WebKit Linux (deps)", "WebKit Linux 32",
        "WebKit Mac Builder", "WebKit Mac Builder (dbg)", "WebKit Mac Builder (deps)", "WebKit Mac10.6", "WebKit Mac10.6 (dbg)",
        "WebKit Mac10.6 (deps)", "WebKit Mac10.7", "WebKit Win", "WebKit Win (dbg)(1)", "WebKit Win (dbg)(2)", "WebKit Win (deps)", "WebKit Win Builder", "WebKit Win Builder (dbg)",
        "WebKit Win Builder (deps)", "WebKit Win7", "Win (dbg)", "Win Builder",
        "Linux (Content Shell)"];
    var expectedBuilders = [["WebKit Linux", 2], ["WebKit Linux (dbg)"], ["WebKit Linux 32"], ["WebKit Mac10.6"],
        ["WebKit Mac10.6 (dbg)"], ["WebKit Mac10.7"], ["WebKit Win"], ["WebKit Win (dbg)(1)"], ["WebKit Win (dbg)(2)"], ["WebKit Win7"]];
    deepEqual(generateBuildersFromBuilderList(builderList, isChromiumWebkitTipOfTreeTestRunner), expectedBuilders);
});

test('generateChromiumWebkitDepsBuildersFromBuilderList', 1, function() {
    var builderList = ["Chrome Frame Tests", "GPU Linux (NVIDIA)", "GPU Linux (dbg) (NVIDIA)", "GPU Mac", "GPU Mac (dbg)", "GPU Win7 (NVIDIA)", "GPU Win7 (dbg) (NVIDIA)", "Linux Perf", "Linux Tests",
        "Linux Valgrind", "Mac Builder (dbg)", "Mac10.6 Perf", "Mac10.6 Tests", "Vista Perf", "Vista Tests", "WebKit Linux", "WebKit Linux ASAN",  "WebKit Linux (dbg)", "WebKit Linux (deps)", "WebKit Linux 32",
        "WebKit Mac Builder", "WebKit Mac Builder (dbg)", "WebKit Mac Builder (deps)", "WebKit Mac10.6", "WebKit Mac10.6 (dbg)",
        "WebKit Mac10.6 (deps)", "WebKit Mac10.7", "WebKit Win", "WebKit Win (dbg)(1)", "WebKit Win (dbg)(2)", "WebKit Win (deps)", "WebKit Win Builder", "WebKit Win Builder (dbg)",
        "WebKit Win Builder (deps)", "WebKit Win7", "Win (dbg)", "Win Builder"];
    var expectedBuilders = [["WebKit Linux (deps)", 2], ["WebKit Mac10.6 (deps)"], ["WebKit Win (deps)"]];
    deepEqual(generateBuildersFromBuilderList(builderList, isChromiumWebkitDepsTestRunner), expectedBuilders);
});

test('generateChromiumDepsGTestBuildersFromBuilderList', 1, function() {
    var builderList = ["Android Builder", "Chrome Frame Tests (ie6)", "Chrome Frame Tests (ie7)", "Chrome Frame Tests (ie8)", "Interactive Tests (dbg)", "Linux", "Linux Builder (dbg)",
        "Linux Builder (dbg)(shared)", "Linux Builder x64", "Linux Clang (dbg)", "Linux Sync", "Linux Tests (dbg)(1)", "Linux Tests (dbg)(2)", "Linux Tests (dbg)(shared)", "Linux Tests x64",
        "Linux x64", "Mac", "Mac 10.6 Tests (dbg)(1)", "Mac 10.6 Tests (dbg)(2)",
        "Mac 10.6 Tests (dbg)(3)", "Mac 10.6 Tests (dbg)(4)", "Mac Builder", "Mac Builder (dbg)", "Mac10.6 Sync",
        "Mac10.6 Tests (1)", "Mac10.6 Tests (2)", "Mac10.6 Tests (3)", "NACL Tests", "NACL Tests (x64)", "Vista Tests (1)", "Vista Tests (2)", "Vista Tests (3)", "Win", "Win Aura",
        "Win Builder", "Win Builder (dbg)", "Win Builder 2010 (dbg)", "Win7 Sync", "Win7 Tests (1)", "Win7 Tests (2)", "Win7 Tests (3)", "Win7 Tests (dbg)(1)", "Win7 Tests (dbg)(2)",
        "Win7 Tests (dbg)(3)", "Win7 Tests (dbg)(4)", "Win7 Tests (dbg)(5)", "Win7 Tests (dbg)(6)", "XP Tests (1)", "XP Tests (2)", "XP Tests (3)", "XP Tests (dbg)(1)", "XP Tests (dbg)(2)",
        "XP Tests (dbg)(3)", "XP Tests (dbg)(4)", "XP Tests (dbg)(5)", "XP Tests (dbg)(6)"];
    var expectedBuilders = [["Interactive Tests (dbg)", 2], ["Linux Sync"], ["Linux Tests (dbg)(1)"], ["Linux Tests (dbg)(2)"], ["Linux Tests (dbg)(shared)"], ["Linux Tests x64"],
        ["Mac 10.6 Tests (dbg)(1)"], ["Mac 10.6 Tests (dbg)(2)"], ["Mac 10.6 Tests (dbg)(3)"],
        ["Mac 10.6 Tests (dbg)(4)"], ["Mac10.6 Sync"], ["Mac10.6 Tests (1)"], ["Mac10.6 Tests (2)"], ["Mac10.6 Tests (3)"], ["NACL Tests"],
        ["NACL Tests (x64)"], ["Vista Tests (1)"], ["Vista Tests (2)"], ["Vista Tests (3)"], ["Win7 Sync"], ["Win7 Tests (1)"], ["Win7 Tests (2)"], ["Win7 Tests (3)"], ["Win7 Tests (dbg)(1)"],
        ["Win7 Tests (dbg)(2)"], ["Win7 Tests (dbg)(3)"], ["Win7 Tests (dbg)(4)"], ["Win7 Tests (dbg)(5)"], ["Win7 Tests (dbg)(6)"], ["XP Tests (1)"], ["XP Tests (2)"], ["XP Tests (3)"],
        ["XP Tests (dbg)(1)"], ["XP Tests (dbg)(2)"], ["XP Tests (dbg)(3)"], ["XP Tests (dbg)(4)"], ["XP Tests (dbg)(5)"], ["XP Tests (dbg)(6)"]];
    deepEqual(generateBuildersFromBuilderList(builderList, isChromiumDepsGTestRunner), expectedBuilders);
});

test('generateChromiumDepsCrosGTestBuildersFromBuilderList', 1, function() {
    var builderList = ["ChromiumOS (amd64)", "ChromiumOS (arm)", "ChromiumOS (tegra2)", "ChromiumOS (x86)", "Linux ChromiumOS (Clang dbg)", "Linux ChromiumOS Builder", "Linux ChromiumOS Builder (dbg)",
        "Linux ChromiumOS Tests (1)", "Linux ChromiumOS Tests (2)", "Linux ChromiumOS Tests (dbg)(1)", "Linux ChromiumOS Tests (dbg)(2)", "Linux ChromiumOS Tests (dbg)(3)"];
    var expectedBuilders = [["Linux ChromiumOS Tests (1)", 2], ["Linux ChromiumOS Tests (2)"], ["Linux ChromiumOS Tests (dbg)(1)"], ["Linux ChromiumOS Tests (dbg)(2)"], ["Linux ChromiumOS Tests (dbg)(3)"]];
    deepEqual(generateBuildersFromBuilderList(builderList, isChromiumDepsCrosGTestRunner), expectedBuilders);
});

test('generateChromiumTipOfTreeGTestBuildersFromBuilderList', 1, function() {
    var builderList = ["Chrome Frame Tests", "GPU Linux (NVIDIA)", "GPU Linux (dbg) (NVIDIA)", "GPU Mac", "GPU Mac (dbg)", "GPU Win7 (NVIDIA)", "GPU Win7 (dbg) (NVIDIA)", "Linux Perf",
        "Linux Tests", "Linux Valgrind", "Mac Builder (dbg)", "Mac10.6 Perf", "Mac10.6 Tests", "Vista Perf", "Vista Tests", "WebKit Linux", "WebKit Linux (dbg)", "WebKit Linux (deps)",
        "WebKit Linux 32", "WebKit Mac Builder", "WebKit Mac Builder (dbg)", "WebKit Mac Builder (deps)",
        "WebKit Mac10.6", "WebKit Mac10.6 (dbg)", "WebKit Mac10.6 (deps)", "WebKit Mac10.7", "WebKit Win", "WebKit Win (dbg)(1)", "WebKit Win (dbg)(2)",
        "WebKit Win (deps)", "WebKit Win Builder", "WebKit Win Builder (dbg)", "WebKit Win Builder (deps)", "WebKit Win7", "Win (dbg)", "Win Builder"];
    var expectedBuilders = [['Linux Tests', BuilderGroup.DEFAULT_BUILDER], ['Mac10.6 Tests'], ['Vista Tests'], ['Win (dbg)']];
    deepEqual(generateBuildersFromBuilderList(builderList, isChromiumTipOfTreeGTestRunner), expectedBuilders);
});

test('queryHashAsMap', 2, function() {
    equal(window.location.hash, '#useTestData=true');
    deepEqual(queryHashAsMap(), {useTestData: 'true'});
});

test('parseCrossDashboardParameters', 2, function() {
    equal(window.location.hash, '#useTestData=true');
    parseCrossDashboardParameters();

    var expectedParameters = {};
    for (var key in g_defaultCrossDashboardStateValues)
        expectedParameters[key] = g_defaultCrossDashboardStateValues[key];
    expectedParameters.useTestData = true;

    deepEqual(g_crossDashboardState, expectedParameters);
});

test('diffStates', 5, function() {
    var newState = {a: 1, b: 2};
    deepEqual(diffStates(null, newState), newState);

    var oldState = {a: 1};
    deepEqual(diffStates(oldState, newState), {b: 2});

    // FIXME: This is kind of weird. I think the existing users of this code work correctly, but it's a confusing result.
    var oldState = {c: 1};
    deepEqual(diffStates(oldState, newState), {a:1, b: 2});

    var oldState = {a: 1, b: 2};
    deepEqual(diffStates(oldState, newState), {});

    var oldState = {a: 2, b: 3};
    deepEqual(diffStates(oldState, newState), {a: 1, b: 2});
});

test('addBuilderLoadErrors', 1, function() {
    clearErrors();
    g_hasDoneInitialPageGeneration = false;
    g_buildersThatFailedToLoad = ['builder1', 'builder2'];
    g_staleBuilders = ['staleBuilder1'];
    addBuilderLoadErrors();
    equal(g_errorMessages, 'ERROR: Failed to get data from builder1,builder2.<br>ERROR: Data from staleBuilder1 is more than 1 day stale.<br>');
});

test('builderGroupIsToTWebKitAttribute', 2, function() {
    var dummyMaster = new BuilderMaster('dummy.org', 'http://build.dummy.org');
    var testBuilderGroups = {
        '@ToT - dummy.org': new BuilderGroup(BuilderGroup.TOT_WEBKIT),
        '@DEPS - dummy.org': new BuilderGroup(BuilderGroup.DEPS_WEBKIT),
    }
    testBuilderGroups['@ToT - dummy.org'].expectedGroups = 1;
    testBuilderGroups['@DEPS - dummy.org'].expectedGroups = 1;

    var testJSONData = "{ \"Dummy Builder 1\": null, \"Dummy Builder 2\": null }";
    onBuilderListLoad(testBuilderGroups, function() { return true; }, dummyMaster, '@ToT - dummy.org', {responseText: testJSONData});
    equal(testBuilderGroups['@ToT - dummy.org'].isToTWebKit, true);
    onBuilderListLoad(testBuilderGroups, function() { return true; }, dummyMaster, '@DEPS - dummy.org', {responseText: testJSONData});
    equal(testBuilderGroups['@DEPS - dummy.org'].isToTWebKit, false);
});

test('builderGroupExpectedGroups', 4, function() {
    var dummyMaster = new BuilderMaster('dummy.org', 'http://build.dummy.org');
    var testBuilderGroups = {
        '@ToT - dummy.org': new BuilderGroup(BuilderGroup.TOT_WEBKIT),
    }
    testBuilderGroups['@ToT - dummy.org'].expectedGroups = 3;

    var testJSONData = "{ \"Dummy Builder 1\": null }";
    equal(testBuilderGroups['@ToT - dummy.org'].expectedGroups, 3);
    onBuilderListLoad(testBuilderGroups,  function() { return true; }, dummyMaster, '@ToT - dummy.org', {responseText: testJSONData});
    equal(testBuilderGroups['@ToT - dummy.org'].groups, 1);
    var testJSONData = "{ \"Dummy Builder 2\": null }";
    onBuilderListLoad(testBuilderGroups,  function() { return true; }, dummyMaster, '@ToT - dummy.org', {responseText: testJSONData});
    equal(testBuilderGroups['@ToT - dummy.org'].groups, 2);
    onErrorLoadingBuilderList('http://build.dummy.org', testBuilderGroups,  '@ToT - dummy.org');
    equal(testBuilderGroups['@ToT - dummy.org'].groups, 3);
});

test('requestBuilderListAddsBuilderGroupEntry', 2, function() {
    var testBuilderGroups = { '@ToT - dummy.org': null };

    var requestFunction = loader.request;
    loader.request = function() {};

    try {
        var builderFilter = null;
        var master = { builderJsonPath: function() {} };
        var groupName = '@ToT - dummy.org';
        var builderGroup = { expectedGroups: 0 };
        requestBuilderList(testBuilderGroups, builderFilter, master, groupName, builderGroup);

        equal(testBuilderGroups['@ToT - dummy.org'], builderGroup);
        equal(testBuilderGroups['@ToT - dummy.org'].expectedGroups, 1);
    } finally {
        loader.request = requestFunction;
    }
})

test('sortTests', 4, function() {
    var test1 = createResultsObjectForTest('foo/test1.html', 'dummyBuilder');
    var test2 = createResultsObjectForTest('foo/test2.html', 'dummyBuilder');
    var test3 = createResultsObjectForTest('foo/test3.html', 'dummyBuilder');
    test1.modifiers = 'b';
    test2.modifiers = 'a';
    test3.modifiers = '';

    var tests = [test1, test2, test3];
    sortTests(tests, 'modifiers', FORWARD);
    deepEqual(tests, [test2, test1, test3]);
    sortTests(tests, 'modifiers', BACKWARD);
    deepEqual(tests, [test3, test1, test2]);

    test1.bugs = 'b';
    test2.bugs = 'a';
    test3.bugs = '';

    var tests = [test1, test2, test3];
    sortTests(tests, 'bugs', FORWARD);
    deepEqual(tests, [test2, test1, test3]);
    sortTests(tests, 'bugs', BACKWARD);
    deepEqual(tests, [test3, test1, test2]);
});

test('popup', 2, function() {
    showPopup(document.body, 'dummy content');
    ok(document.querySelector('#popup'));
    hidePopup();
    ok(!document.querySelector('#popup'));
});

test('gpuResultsPath', 3, function() {
  equal(gpuResultsPath('777777', 'Win7 Release (ATI)'), '777777_Win7_Release_ATI_');
  equal(gpuResultsPath('123', 'GPU Linux (dbg)(NVIDIA)'), '123_GPU_Linux_dbg_NVIDIA_');
  equal(gpuResultsPath('12345', 'GPU Mac'), '12345_GPU_Mac');
});

test('TestTrie', 3, function() {
    var builders = {
        "Dummy Chromium Windows Builder": true,
        "Dummy GTK Linux Builder": true,
        "Dummy Apple Mac Lion Builder": true
    };

    var resultsByBuilder = {
        "Dummy Chromium Windows Builder": {
            tests: {
                "foo": true,
                "foo/bar/1.html": true,
                "foo/bar/baz": true
            }
        },
        "Dummy GTK Linux Builder": {
            tests: {
                "bar": true,
                "foo/1.html": true,
                "foo/bar/2.html": true,
                "foo/bar/baz/1.html": true,
            }
        },
        "Dummy Apple Mac Lion Builder": {
            tests: {
                "foo/bar/3.html": true,
                "foo/bar/baz/foo": true,
            }
        }
    };
    var expectedTrie = {
        "foo": {
            "bar": {
                "1.html": true,
                "2.html": true,
                "3.html": true,
                "baz": {
                    "1.html": true,
                    "foo": true
                }
            },
            "1.html": true
        },
        "bar": true
    }

    var trie = new TestTrie(builders, resultsByBuilder);
    deepEqual(trie._trie, expectedTrie);

    var leafsOfCompleteTrieTraversal = [];
    var expectedLeafs = ["foo/bar/1.html", "foo/bar/baz/1.html", "foo/bar/baz/foo", "foo/bar/2.html", "foo/bar/3.html", "foo/1.html", "bar"];
    trie.forEach(function(triePath) {
        leafsOfCompleteTrieTraversal.push(triePath);
    });
    deepEqual(leafsOfCompleteTrieTraversal, expectedLeafs);

    var leafsOfPartialTrieTraversal = [];
    expectedLeafs = ["foo/bar/1.html", "foo/bar/baz/1.html", "foo/bar/baz/foo", "foo/bar/2.html", "foo/bar/3.html"];
    trie.forEach(function(triePath) {
        leafsOfPartialTrieTraversal.push(triePath);
    }, "foo/bar");
    deepEqual(leafsOfPartialTrieTraversal, expectedLeafs);
});
