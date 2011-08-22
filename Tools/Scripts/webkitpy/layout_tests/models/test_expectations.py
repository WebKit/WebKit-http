#!/usr/bin/env python
# Copyright (C) 2010 Google Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#     * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#     * Neither the name of Google Inc. nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

"""A helper class for reading in and dealing with tests expectations
for layout tests.
"""

import itertools
import logging
import re

try:
    import json
except ImportError:
    # python 2.5 compatibility
    import webkitpy.thirdparty.simplejson as json

from webkitpy.layout_tests.models.test_configuration import TestConfiguration, TestConfigurationConverter

_log = logging.getLogger(__name__)


# Test expectation and modifier constants.
# FIXME: range() starts with 0 which makes if expectation checks harder
# as PASS is 0.
(PASS, FAIL, TEXT, IMAGE, IMAGE_PLUS_TEXT, AUDIO, TIMEOUT, CRASH, SKIP, WONTFIX,
 SLOW, REBASELINE, MISSING, FLAKY, NOW, NONE) = range(16)


def result_was_expected(result, expected_results, test_needs_rebaselining, test_is_skipped):
    """Returns whether we got a result we were expecting.
    Args:
        result: actual result of a test execution
        expected_results: set of results listed in test_expectations
        test_needs_rebaselining: whether test was marked as REBASELINE
        test_is_skipped: whether test was marked as SKIP"""
    if result in expected_results:
        return True
    if result in (IMAGE, TEXT, IMAGE_PLUS_TEXT) and FAIL in expected_results:
        return True
    if result == MISSING and test_needs_rebaselining:
        return True
    if result == SKIP and test_is_skipped:
        return True
    return False


def remove_pixel_failures(expected_results):
    """Returns a copy of the expected results for a test, except that we
    drop any pixel failures and return the remaining expectations. For example,
    if we're not running pixel tests, then tests expected to fail as IMAGE
    will PASS."""
    expected_results = expected_results.copy()
    if IMAGE in expected_results:
        expected_results.remove(IMAGE)
        expected_results.add(PASS)
    if IMAGE_PLUS_TEXT in expected_results:
        expected_results.remove(IMAGE_PLUS_TEXT)
        expected_results.add(TEXT)
    return expected_results


def has_pixel_failures(actual_results):
    return IMAGE in actual_results or IMAGE_PLUS_TEXT in actual_results


# FIXME: This method is no longer used here in this module. Remove remaining callsite in manager.py and this method.
def strip_comments(line):
    """Strips comments from a line and return None if the line is empty
    or else the contents of line with leading and trailing spaces removed
    and all other whitespace collapsed"""

    commentIndex = line.find('//')
    if commentIndex is -1:
        commentIndex = len(line)

    line = re.sub(r'\s+', ' ', line[:commentIndex].strip())
    if line == '':
        return None
    else:
        return line


class ParseError(Exception):
    def __init__(self, fatal, errors):
        self.fatal = fatal
        self.errors = errors

    def __str__(self):
        return '\n'.join(map(str, self.errors))

    def __repr__(self):
        return 'ParseError(fatal=%s, errors=%s)' % (self.fatal, self.errors)


class TestExpectationSerializer(object):
    """Provides means of serializing TestExpectationLine instances."""
    def __init__(self, test_configuration_converter=None):
        self._test_configuration_converter = test_configuration_converter
        self._parsed_expectation_to_string = dict([[parsed_expectation, expectation_string] for expectation_string, parsed_expectation in TestExpectations.EXPECTATIONS.items()])

    def to_string(self, expectation_line):
        if expectation_line.is_malformed():
            return expectation_line.original_string or ''

        if expectation_line.name is None:
            return '' if expectation_line.comment is None else "//%s" % expectation_line.comment

        if self._test_configuration_converter and expectation_line.parsed_bug_modifiers:
            specifiers_list = self._test_configuration_converter.to_specifiers_list(expectation_line.matching_configurations)
            result = []
            for specifiers in specifiers_list:
                modifiers = self._parsed_modifier_string(expectation_line, specifiers)
                expectations = self._parsed_expectations_string(expectation_line)
                result.append(self._format_result(modifiers, expectation_line.name, expectations, expectation_line.comment))
            return "\n".join(result) if result else None

        return self._format_result(" ".join(expectation_line.modifiers), expectation_line.name, " ".join(expectation_line.expectations), expectation_line.comment)

    def _parsed_expectations_string(self, expectation_line):
        result = []
        for index in TestExpectations.EXPECTATION_ORDER:
            if index in expectation_line.parsed_expectations:
                result.append(self._parsed_expectation_to_string[index])
        return ' '.join(result)

    def _parsed_modifier_string(self, expectation_line, specifiers):
        assert(self._test_configuration_converter)
        result = []
        if expectation_line.parsed_bug_modifiers:
            result.extend(sorted(expectation_line.parsed_bug_modifiers))
        result.extend(sorted(expectation_line.parsed_modifiers))
        result.extend(self._test_configuration_converter.specifier_sorter().sort_specifiers(specifiers))
        return ' '.join(result)

    @classmethod
    def _format_result(cls, modifiers, name, expectations, comment):
        result = "%s : %s = %s" % (modifiers.upper(), name, expectations.upper())
        if comment is not None:
            result += " //%s" % comment
        return result

    @classmethod
    def list_to_string(cls, expectation_lines, test_configuration_converter=None, reconstitute_only_these=None):
        serializer = cls(test_configuration_converter)

        def serialize(expectation_line):
            if not reconstitute_only_these or expectation_line in reconstitute_only_these:
                return serializer.to_string(expectation_line)
            return expectation_line.original_string

        def nones_out(expectation_line):
            return expectation_line is not None

        return "\n".join(filter(nones_out, map(serialize, expectation_lines)))


class TestExpectationParser(object):
    """Provides parsing facilities for lines in the test_expectation.txt file."""

    BUG_MODIFIER_PREFIX = 'bug'
    BUG_MODIFIER_REGEX = 'bug\d+'
    REBASELINE_MODIFIER = 'rebaseline'
    SKIP_MODIFIER = 'skip'
    SLOW_MODIFIER = 'slow'
    WONTFIX_MODIFIER = 'wontfix'

    TIMEOUT_EXPECTATION = 'timeout'

    def __init__(self, port, full_test_list, allow_rebaseline_modifier):
        self._port = port
        self._test_configuration_converter = TestConfigurationConverter(set(port.all_test_configurations()), port.configuration_specifier_macros())
        self._full_test_list = full_test_list
        self._allow_rebaseline_modifier = allow_rebaseline_modifier

    def parse(self, expectation_line):
        if not expectation_line.name:
            return

        self._check_modifiers_against_expectations(expectation_line)
        if self._check_path_does_not_exist(expectation_line):
            return

        expectation_line.path = self._port.normalize_test_name(expectation_line.name)
        self._collect_matching_tests(expectation_line)

        self._parse_modifiers(expectation_line)
        self._parse_expectations(expectation_line)

    def _parse_modifiers(self, expectation_line):
        has_wontfix = False
        parsed_specifiers = set()
        for modifier in expectation_line.modifiers:
            if modifier in TestExpectations.MODIFIERS:
                expectation_line.parsed_modifiers.append(modifier)
                if modifier == self.WONTFIX_MODIFIER:
                    has_wontfix = True
            elif modifier.startswith(self.BUG_MODIFIER_PREFIX):
                if re.match(self.BUG_MODIFIER_REGEX, modifier):
                    expectation_line.errors.append('BUG\d+ is not allowed, must be one of BUGCR\d+, BUGWK\d+, BUGV8_\d+, or a non-numeric bug identifier.')
                else:
                    expectation_line.parsed_bug_modifiers.append(modifier)
            else:
                parsed_specifiers.add(modifier)

        if not expectation_line.parsed_bug_modifiers and not has_wontfix:
            expectation_line.warnings.append('Test lacks BUG modifier.')

        if self._allow_rebaseline_modifier and self.REBASELINE_MODIFIER in expectation_line.modifiers:
            expectation_line.errors.append('REBASELINE should only be used for running rebaseline.py. Cannot be checked in.')

        expectation_line.matching_configurations = self._test_configuration_converter.to_config_set(parsed_specifiers, expectation_line.errors)

    def _parse_expectations(self, expectation_line):
        result = set()
        for part in expectation_line.expectations:
            expectation = TestExpectations.expectation_from_string(part)
            if expectation is None:  # Careful, PASS is currently 0.
                expectation_line.errors.append('Unsupported expectation: %s' % part)
                continue
            result.add(expectation)
        expectation_line.parsed_expectations = result

    def _check_modifiers_against_expectations(self, expectation_line):
        if self.SLOW_MODIFIER in expectation_line.modifiers and self.TIMEOUT_EXPECTATION in expectation_line.expectations:
            expectation_line.errors.append('A test can not be both SLOW and TIMEOUT. If it times out indefinitely, then it should be just TIMEOUT.')

    def _check_path_does_not_exist(self, expectation_line):
        # WebKit's way of skipping tests is to add a -disabled suffix.
        # So we should consider the path existing if the path or the
        # -disabled version exists.
        if (not self._port.test_exists(expectation_line.name)
            and not self._port.test_exists(expectation_line.name + '-disabled')):
            # Log a warning here since you hit this case any
            # time you update test_expectations.txt without syncing
            # the LayoutTests directory
            expectation_line.warnings.append('Path does not exist.')
            return True
        return False

    def _collect_matching_tests(self, expectation_line):
        """Convert the test specification to an absolute, normalized
        path and make sure directories end with the OS path separator."""
        # FIXME: full_test_list can quickly contain a big amount of
        # elements. We should consider at some point to use a more
        # efficient structure instead of a list. Maybe a dictionary of
        # lists to represent the tree of tests, leaves being test
        # files and nodes being categories.

        if not self._full_test_list:
            expectation_line.matching_tests = [expectation_line.path]
            return

        if self._port.test_isdir(expectation_line.path):
            # this is a test category, return all the tests of the category.
            expectation_line.matching_tests = [test for test in self._full_test_list if test.startswith(expectation_line.path)]
            return

        # this is a test file, do a quick check if it's in the
        # full test suite.
        if expectation_line.path in self._full_test_list:
            expectation_line.matching_tests.append(expectation_line.path)

    @classmethod
    def tokenize(cls, expectation_string, line_number=None):
        """Tokenizes a line from test_expectations.txt and returns an unparsed TestExpectationLine instance.

        The format of a test expectation line is:

        [[<modifiers>] : <name> = <expectations>][ //<comment>]

        Any errant whitespace is not preserved.

        """
        expectation_line = TestExpectationLine()
        expectation_line.original_string = expectation_string
        expectation_line.line_number = line_number
        comment_index = expectation_string.find("//")
        if comment_index == -1:
            comment_index = len(expectation_string)
        else:
            expectation_line.comment = expectation_string[comment_index + 2:]

        remaining_string = re.sub(r"\s+", " ", expectation_string[:comment_index].strip())
        if len(remaining_string) == 0:
            return expectation_line

        parts = remaining_string.split(':')
        if len(parts) != 2:
            expectation_line.errors.append("Missing a ':'" if len(parts) < 2 else "Extraneous ':'")
        else:
            test_and_expectation = parts[1].split('=')
            if len(test_and_expectation) != 2:
                expectation_line.errors.append("Missing expectations" if len(test_and_expectation) < 2 else "Extraneous '='")

        if not expectation_line.is_malformed():
            expectation_line.modifiers = cls._split_space_separated(parts[0])
            expectation_line.name = test_and_expectation[0].strip()
            expectation_line.expectations = cls._split_space_separated(test_and_expectation[1])

        return expectation_line

    @classmethod
    def tokenize_list(cls, expectations_string):
        """Returns a list of TestExpectationLines, one for each line in expectations_string."""
        expectation_lines = []
        line_number = 0
        for line in expectations_string.split("\n"):
            line_number += 1
            expectation_lines.append(cls.tokenize(line, line_number))
        return expectation_lines

    @classmethod
    def _split_space_separated(cls, space_separated_string):
        """Splits a space-separated string into an array."""
        # FIXME: Lower-casing is necessary to support legacy code. Need to eliminate.
        return [part.strip().lower() for part in space_separated_string.strip().split(' ')]


class TestExpectationLine:
    """Represents a line in test expectations file."""

    def __init__(self):
        """Initializes a blank-line equivalent of an expectation."""
        self.original_string = None
        self.line_number = None
        self.name = None
        self.path = None
        self.modifiers = []
        self.parsed_modifiers = []
        self.parsed_bug_modifiers = []
        self.matching_configurations = set()
        self.expectations = []
        self.parsed_expectations = set()
        self.comment = None
        self.matching_tests = []
        self.errors = []
        self.warnings = []

    def is_malformed(self):
        return len(self.errors) > 0

    def is_invalid(self):
        return self.is_malformed() or len(self.warnings) > 0

    def is_flaky(self):
        return len(self.parsed_expectations) > 1

    @classmethod
    def create_passing_expectation(cls, test):
        expectation_line = TestExpectationLine()
        expectation_line.name = test
        expectation_line.path = test
        expectation_line.parsed_expectations = set([PASS])
        expectation_line.matching_tests = [test]
        return expectation_line


# FIXME: Refactor API to be a proper CRUD.
class TestExpectationsModel(object):
    """Represents relational store of all expectations and provides CRUD semantics to manage it."""

    def __init__(self):
        # Maps a test to its list of expectations.
        self._test_to_expectations = {}

        # Maps a test to list of its modifiers (string values)
        self._test_to_modifiers = {}

        # Maps a test to a TestExpectationLine instance.
        self._test_to_expectation_line = {}

        # List of tests that are in the overrides file (used for checking for
        # duplicates inside the overrides file itself). Note that just because
        # a test is in this set doesn't mean it's necessarily overridding a
        # expectation in the regular expectations; the test might not be
        # mentioned in the regular expectations file at all.
        self._overridding_tests = set()

        self._modifier_to_tests = self._dict_of_sets(TestExpectations.MODIFIERS)
        self._expectation_to_tests = self._dict_of_sets(TestExpectations.EXPECTATIONS)
        self._timeline_to_tests = self._dict_of_sets(TestExpectations.TIMELINES)
        self._result_type_to_tests = self._dict_of_sets(TestExpectations.RESULT_TYPES)

    def _dict_of_sets(self, strings_to_constants):
        """Takes a dict of strings->constants and returns a dict mapping
        each constant to an empty set."""
        d = {}
        for c in strings_to_constants.values():
            d[c] = set()
        return d

    def get_test_set(self, modifier, expectation=None, include_skips=True):
        if expectation is None:
            tests = self._modifier_to_tests[modifier]
        else:
            tests = (self._expectation_to_tests[expectation] &
                self._modifier_to_tests[modifier])

        if not include_skips:
            tests = tests - self.get_test_set(SKIP, expectation)

        return tests

    def get_tests_with_result_type(self, result_type):
        return self._result_type_to_tests[result_type]

    def get_tests_with_timeline(self, timeline):
        return self._timeline_to_tests[timeline]

    def get_modifiers(self, test):
        """This returns modifiers for the given test (the modifiers plus the BUGXXXX identifier). This is used by the LTTF dashboard."""
        return self._test_to_modifiers[test]

    def has_modifier(self, test, modifier):
        return test in self._modifier_to_tests[modifier]

    def has_test(self, test):
        return test in self._test_to_expectation_line

    def get_expectation_line(self, test):
        return self._test_to_expectation_line.get(test)

    def get_expectations(self, test):
        return self._test_to_expectations[test]

    def add_expectation_line(self, expectation_line, overrides_allowed):
        """Returns a list of errors, encountered while matching modifiers."""

        if expectation_line.is_invalid():
            return

        for test in expectation_line.matching_tests:
            if self._already_seen_better_match(test, expectation_line, overrides_allowed):
                continue

            self._clear_expectations_for_test(test, expectation_line)
            self._test_to_expectation_line[test] = expectation_line
            self._add_test(test, expectation_line, overrides_allowed)

    def _add_test(self, test, expectation_line, overrides_allowed):
        """Sets the expected state for a given test.

        This routine assumes the test has not been added before. If it has,
        use _clear_expectations_for_test() to reset the state prior to
        calling this.

        Args:
          test: test to add
          expectation_line: expectation to add
          overrides_allowed: whether we're parsing the regular expectations
              or the overridding expectations"""
        self._test_to_expectations[test] = expectation_line.parsed_expectations
        for expectation in expectation_line.parsed_expectations:
            self._expectation_to_tests[expectation].add(test)

        self._test_to_modifiers[test] = expectation_line.modifiers
        for modifier in expectation_line.parsed_modifiers:
            mod_value = TestExpectations.MODIFIERS[modifier]
            self._modifier_to_tests[mod_value].add(test)

        if TestExpectationParser.WONTFIX_MODIFIER in expectation_line.parsed_modifiers:
            self._timeline_to_tests[WONTFIX].add(test)
        else:
            self._timeline_to_tests[NOW].add(test)

        if TestExpectationParser.SKIP_MODIFIER in expectation_line.parsed_modifiers:
            self._result_type_to_tests[SKIP].add(test)
        elif expectation_line.parsed_expectations == set([PASS]):
            self._result_type_to_tests[PASS].add(test)
        elif expectation_line.is_flaky():
            self._result_type_to_tests[FLAKY].add(test)
        else:
            self._result_type_to_tests[FAIL].add(test)

        if overrides_allowed:
            self._overridding_tests.add(test)

    def _clear_expectations_for_test(self, test, expectation_line):
        """Remove prexisting expectations for this test.
        This happens if we are seeing a more precise path
        than a previous listing.
        """
        if self.has_test(test):
            self._test_to_expectations.pop(test, '')
            self._remove_from_sets(test, self._expectation_to_tests)
            self._remove_from_sets(test, self._modifier_to_tests)
            self._remove_from_sets(test, self._timeline_to_tests)
            self._remove_from_sets(test, self._result_type_to_tests)

        self._test_to_expectation_line[test] = expectation_line

    def _remove_from_sets(self, test, dict):
        """Removes the given test from the sets in the dictionary.

        Args:
          test: test to look for
          dict: dict of sets of files"""
        for set_of_tests in dict.itervalues():
            if test in set_of_tests:
                set_of_tests.remove(test)

    def _already_seen_better_match(self, test, expectation_line, overrides_allowed):
        """Returns whether we've seen a better match already in the file.

        Returns True if we've already seen a expectation_line.name that matches more of the test
            than this path does
        """
        # FIXME: See comment below about matching test configs and specificity.
        if not self.has_test(test):
            # We've never seen this test before.
            return False

        prev_expectation_line = self._test_to_expectation_line[test]

        if len(prev_expectation_line.path) > len(expectation_line.path):
            # The previous path matched more of the test.
            return True

        if len(prev_expectation_line.path) < len(expectation_line.path):
            # This path matches more of the test.
            return False

        if overrides_allowed and test not in self._overridding_tests:
            # We have seen this path, but that's okay because it is
            # in the overrides and the earlier path was in the
            # expectations (not the overrides).
            return False

        # At this point we know we have seen a previous exact match on this
        # base path, so we need to check the two sets of modifiers.

        if overrides_allowed:
            expectation_source = "override"
        else:
            expectation_source = "expectation"

        # FIXME: This code was originally designed to allow lines that matched
        # more modifiers to override lines that matched fewer modifiers.
        # However, we currently view these as errors.
        #
        # To use the "more modifiers wins" policy, change the errors for overrides
        # to be warnings and return False".

        if prev_expectation_line.matching_configurations == expectation_line.matching_configurations:
            expectation_line.errors.append('Duplicate or ambiguous %s.' % expectation_source)
            return True

        if prev_expectation_line.matching_configurations >= expectation_line.matching_configurations:
            expectation_line.errors.append('More specific entry on line %d overrides line %d' % (expectation_line.line_number, prev_expectation_line.line_number))
            # FIXME: return False if we want more specific to win.
            return True

        if prev_expectation_line.matching_configurations <= expectation_line.matching_configurations:
            expectation_line.errors.append('More specific entry on line %d overrides line %d' % (prev_expectation_line.line_number, expectation_line.line_number))
            return True

        if prev_expectation_line.matching_configurations & expectation_line.matching_configurations:
            expectation_line.errors.append('Entries on line %d and line %d match overlapping sets of configurations' % (prev_expectation_line.line_number, expectation_line.line_number))
            return True

        # Configuration sets are disjoint, then.
        return False


class BugManager(object):
    """A simple interface for managing bugs from TestExpectationsEditor."""
    def close_bug(self, bug_ids, reference_bug_ids=None):
        raise NotImplementedError("BugManager.close_bug")

    def create_bug(self):
        """Should return a newly created bug id in the form of r"BUG[^\d].*"."""
        raise NotImplementedError("BugManager.create_bug")


class TestExpectationsEditor(object):
    """
    The editor assumes that the expectation data is error-free.
    """

    def __init__(self, expectation_lines, bug_manager):
        self._bug_manager = bug_manager
        self._expectation_lines = expectation_lines
        self._tests_with_directory_paths = set()
        # FIXME: Unify this with TestExpectationsModel.
        self._test_to_expectation_lines = {}
        for expectation_line in expectation_lines:
            for test in expectation_line.matching_tests:
                if test == expectation_line.path:
                    self._test_to_expectation_lines.setdefault(test, []).append(expectation_line)
                else:
                    self._tests_with_directory_paths.add(test)

    def remove_expectation(self, test, test_config_set, remove_flakes=False):
        """Removes existing expectations for {test} in the of test configurations {test_config_set}.
        If the test is flaky, the expectation is not removed, unless remove_flakes is True.

        In this context, removing expectations does not imply that the test is passing -- we are merely removing
        any information about this test from the expectations.

        We do not remove the actual expectation lines here. Instead, we adjust TestExpectationLine.matching_configurations.
        The serializer will figure out what to do:
        * An empty matching_configurations set means that the this line matches nothing and will serialize as None.
        * A matching_configurations set that can't be expressed as one line will be serialized as multiple lines.

        Also, we do only adjust matching_configurations for lines that match tests exactly, because expectation lines with
        better path matches are valid and always win.

        For example, the expectation with the path "fast/events/shadow/" will
        be ignored when removing expectations for the test "fast/event/shadow/awesome-crash.html", since we can just
        add a new expectation line for "fast/event/shadow/awesome-crash.html" to influence expected results.
        """
        expectation_lines = self._test_to_expectation_lines.get(test, [])
        for expectation_line in expectation_lines:
            if (not expectation_line.is_flaky() or remove_flakes) and expectation_line.matching_configurations & test_config_set:
                expectation_line.matching_configurations = expectation_line.matching_configurations - test_config_set
                if not expectation_line.matching_configurations:
                    self._bug_manager.close_bug(expectation_line.parsed_bug_modifiers)
                return

    def update_expectation(self, test, test_config_set, expectation_set, parsed_bug_modifiers=None):
        """Updates expectations for {test} in the set of test configuration {test_config_set} to the values of {expectation_set}.
        If {parsed_bug_modifiers} is supplied, it is used for updated expectations. Otherwise, a new bug is created.

        Here, we treat updating expectations to PASS as special: if possible, the corresponding lines are completely removed.
        """
        # FIXME: Allow specifying modifiers (SLOW, SKIP, WONTFIX).
        updated_expectations = []
        expectation_lines = self._test_to_expectation_lines.get(test, [])
        remaining_configurations = test_config_set.copy()
        bug_ids = self._get_valid_bug_ids(parsed_bug_modifiers)
        new_expectation_line_insertion_point = len(self._expectation_lines)
        remove_expectations = expectation_set == set([PASS]) and test not in self._tests_with_directory_paths

        for expectation_line in expectation_lines:
            if expectation_line.matching_configurations == remaining_configurations:
                # Tweak expectations on existing line.
                if expectation_line.parsed_expectations == expectation_set:
                    return updated_expectations
                self._bug_manager.close_bug(expectation_line.parsed_bug_modifiers, bug_ids)
                updated_expectations.append(expectation_line)
                if remove_expectations:
                    expectation_line.matching_configurations = set()
                else:
                    expectation_line.parsed_expectations = expectation_set
                    expectation_line.parsed_bug_modifiers = bug_ids
                return updated_expectations
            elif expectation_line.matching_configurations >= remaining_configurations:
                # 1) Split up into two expectation lines:
                # * one with old expectations (existing expectation_line)
                # * one with new expectations (new expectation_line)
                # 2) Finish looking, since there will be no more remaining configs to test for.
                expectation_line.matching_configurations -= remaining_configurations
                updated_expectations.append(expectation_line)
                new_expectation_line_insertion_point = self._expectation_lines.index(expectation_line) + 1
                break
            elif expectation_line.matching_configurations <= remaining_configurations:
                # Remove existing expectation line.
                self._bug_manager.close_bug(expectation_line.parsed_bug_modifiers, bug_ids)
                expectation_line.matching_configurations = set()
                updated_expectations.append(expectation_line)
            else:
                intersection = expectation_line.matching_configurations & remaining_configurations
                if intersection:
                    expectation_line.matching_configurations -= intersection
                    updated_expectations.append(expectation_line)
            new_expectation_line_insertion_point = self._expectation_lines.index(expectation_line) + 1

        if not remove_expectations:
            new_expectation_line = self._create_new_line(test, bug_ids, remaining_configurations, expectation_set)
            updated_expectations.append(new_expectation_line)
            self._expectation_lines.insert(new_expectation_line_insertion_point, new_expectation_line)

        return updated_expectations

    def _get_valid_bug_ids(self, suggested_bug_ids):
        # FIXME: Flesh out creating a bug properly (title, etc.)
        return suggested_bug_ids or [self._bug_manager.create_bug()]

    def _create_new_line(self, name, bug_ids, config_set, expectation_set):
        new_line = TestExpectationLine()
        new_line.name = name
        new_line.parsed_bug_modifiers = bug_ids
        new_line.matching_configurations = config_set
        new_line.parsed_expectations = expectation_set
        # Ensure index integrity for multiple operations.
        self._test_to_expectation_lines.setdefault(name, []).append(new_line)
        return new_line


class TestExpectations(object):
    """Test expectations consist of lines with specifications of what
    to expect from layout test cases. The test cases can be directories
    in which case the expectations apply to all test cases in that
    directory and any subdirectory. The format is along the lines of:

      LayoutTests/fast/js/fixme.js = FAIL
      LayoutTests/fast/js/flaky.js = FAIL PASS
      LayoutTests/fast/js/crash.js = CRASH TIMEOUT FAIL PASS
      ...

    To add modifiers:
      SKIP : LayoutTests/fast/js/no-good.js = TIMEOUT PASS
      DEBUG : LayoutTests/fast/js/no-good.js = TIMEOUT PASS
      DEBUG SKIP : LayoutTests/fast/js/no-good.js = TIMEOUT PASS
      LINUX DEBUG SKIP : LayoutTests/fast/js/no-good.js = TIMEOUT PASS
      LINUX WIN : LayoutTests/fast/js/no-good.js = TIMEOUT PASS

    SKIP: Doesn't run the test.
    SLOW: The test takes a long time to run, but does not timeout indefinitely.
    WONTFIX: For tests that we never intend to pass on a given platform.

    Notes:
      -A test cannot be both SLOW and TIMEOUT
      -A test should only be one of IMAGE, TEXT, IMAGE+TEXT, AUDIO, or FAIL.
       FAIL is a legacy value that currently means either IMAGE,
       TEXT, or IMAGE+TEXT. Once we have finished migrating the expectations,
       we should change FAIL to have the meaning of IMAGE+TEXT and remove the
       IMAGE+TEXT identifier.
      -A test can be included twice, but not via the same path.
      -If a test is included twice, then the more precise path wins.
      -CRASH tests cannot be WONTFIX
    """

    TEST_LIST = "test_expectations.txt"

    EXPECTATIONS = {'pass': PASS,
                    'fail': FAIL,
                    'text': TEXT,
                    'image': IMAGE,
                    'image+text': IMAGE_PLUS_TEXT,
                    'audio': AUDIO,
                    'timeout': TIMEOUT,
                    'crash': CRASH,
                    'missing': MISSING}

    EXPECTATION_DESCRIPTIONS = {SKIP: ('skipped', 'skipped'),
                                PASS: ('pass', 'passes'),
                                FAIL: ('failure', 'failures'),
                                TEXT: ('text diff mismatch',
                                       'text diff mismatch'),
                                IMAGE: ('image mismatch', 'image mismatch'),
                                IMAGE_PLUS_TEXT: ('image and text mismatch',
                                                  'image and text mismatch'),
                                AUDIO: ('audio mismatch', 'audio mismatch'),
                                CRASH: ('DumpRenderTree crash',
                                        'DumpRenderTree crashes'),
                                TIMEOUT: ('test timed out', 'tests timed out'),
                                MISSING: ('no expected result found',
                                          'no expected results found')}

    EXPECTATION_ORDER = (PASS, CRASH, TIMEOUT, MISSING, IMAGE_PLUS_TEXT, TEXT, IMAGE, AUDIO, FAIL, SKIP)

    BUILD_TYPES = ('debug', 'release')

    MODIFIERS = {TestExpectationParser.SKIP_MODIFIER: SKIP,
                 TestExpectationParser.WONTFIX_MODIFIER: WONTFIX,
                 TestExpectationParser.SLOW_MODIFIER: SLOW,
                 TestExpectationParser.REBASELINE_MODIFIER: REBASELINE,
                 'none': NONE}

    TIMELINES = {TestExpectationParser.WONTFIX_MODIFIER: WONTFIX,
                 'now': NOW}

    RESULT_TYPES = {'skip': SKIP,
                    'pass': PASS,
                    'fail': FAIL,
                    'flaky': FLAKY}

    @classmethod
    def expectation_from_string(cls, string):
        assert(' ' not in string)  # This only handles one expectation at a time.
        return cls.EXPECTATIONS.get(string.lower())

    def __init__(self, port, tests, expectations,
                 test_config, is_lint_mode=False, overrides=None):
        """Loads and parses the test expectations given in the string.
        Args:
            port: handle to object containing platform-specific functionality
            tests: list of all of the test files
            expectations: test expectations as a string
            test_config: specific values to check against when
                parsing the file (usually port.test_config(),
                but may be different when linting or doing other things).
            is_lint_mode: If True, just parse the expectations string
                looking for errors.
            overrides: test expectations that are allowed to override any
                entries in |expectations|. This is used by callers
                that need to manage two sets of expectations (e.g., upstream
                and downstream expectations).
        """
        self._full_test_list = tests
        self._test_config = test_config
        self._is_lint_mode = is_lint_mode
        self._model = TestExpectationsModel()
        self._parser = TestExpectationParser(port, tests, is_lint_mode)
        self._test_configuration_converter = TestConfigurationConverter(port.all_test_configurations(), port.configuration_specifier_macros())

        self._expectations = TestExpectationParser.tokenize_list(expectations)
        self._add_expectations(self._expectations, overrides_allowed=False)

        if overrides:
            overrides_expectations = TestExpectationParser.tokenize_list(overrides)
            self._add_expectations(overrides_expectations, overrides_allowed=True)
            self._expectations += overrides_expectations

        self._has_warnings = False
        self._report_errors()
        self._process_tests_without_expectations()

    # TODO(ojan): Allow for removing skipped tests when getting the list of
    # tests to run, but not when getting metrics.

    def get_rebaselining_failures(self):
        return (self._model.get_test_set(REBASELINE, FAIL) |
                self._model.get_test_set(REBASELINE, IMAGE) |
                self._model.get_test_set(REBASELINE, TEXT) |
                self._model.get_test_set(REBASELINE, IMAGE_PLUS_TEXT) |
                self._model.get_test_set(REBASELINE, AUDIO))

    # FIXME: Change the callsites to use TestExpectationsModel and remove.
    def get_expectations(self, test):
        return self._model.get_expectations(test)

    # FIXME: Change the callsites to use TestExpectationsModel and remove.
    def has_modifier(self, test, modifier):
        return self._model.has_modifier(test, modifier)

    # FIXME: Change the callsites to use TestExpectationsModel and remove.
    def get_tests_with_result_type(self, result_type):
        return self._model.get_tests_with_result_type(result_type)

    # FIXME: Change the callsites to use TestExpectationsModel and remove.
    def get_test_set(self, modifier, expectation=None, include_skips=True):
        return self._model.get_test_set(modifier, expectation, include_skips)

    # FIXME: Change the callsites to use TestExpectationsModel and remove.
    def get_modifiers(self, test):
        return self._model.get_modifiers(test)

    # FIXME: Change the callsites to use TestExpectationsModel and remove.
    def get_tests_with_timeline(self, timeline):
        return self._model.get_tests_with_timeline(timeline)

    def get_expectations_string(self, test):
        """Returns the expectatons for the given test as an uppercase string.
        If there are no expectations for the test, then "PASS" is returned."""
        expectations = self._model.get_expectations(test)
        retval = []

        for expectation in expectations:
            retval.append(self.expectation_to_string(expectation))

        return " ".join(retval)

    def expectation_to_string(self, expectation):
        """Return the uppercased string equivalent of a given expectation."""
        for item in self.EXPECTATIONS.items():
            if item[1] == expectation:
                return item[0].upper()
        raise ValueError(expectation)

    def matches_an_expected_result(self, test, result, pixel_tests_are_enabled):
        expected_results = self._model.get_expectations(test)
        if not pixel_tests_are_enabled:
            expected_results = remove_pixel_failures(expected_results)
        return result_was_expected(result,
                                   expected_results,
                                   self.is_rebaselining(test),
                                   self._model.has_modifier(test, SKIP))

    def is_rebaselining(self, test):
        return self._model.has_modifier(test, REBASELINE)

    def _report_errors(self):
        errors = []
        warnings = []
        for expectation in self._expectations:
            for error in expectation.errors:
                errors.append("Line:%s %s %s" % (expectation.line_number, error, expectation.name if expectation.expectations else expectation.original_string))
            for warning in expectation.warnings:
                warnings.append("Line:%s %s %s" % (expectation.line_number, warning, expectation.name if expectation.expectations else expectation.original_string))

        if len(errors) or len(warnings):
            _log.error("FAILURES FOR %s" % str(self._test_config))

            for error in errors:
                _log.error(error)
            for warning in warnings:
                _log.error(warning)

            if len(errors):
                raise ParseError(fatal=True, errors=errors)
            if len(warnings):
                self._has_warnings = True
                if self._is_lint_mode:
                    raise ParseError(fatal=False, errors=warnings)

    def _process_tests_without_expectations(self):
        if self._full_test_list:
            for test in self._full_test_list:
                if not self._model.has_test(test):
                    self._model.add_expectation_line(TestExpectationLine.create_passing_expectation(test), overrides_allowed=False)

    def has_warnings(self):
        return self._has_warnings

    def remove_rebaselined_tests(self, except_these_tests):
        """Returns a copy of the expectations with the tests removed."""
        def without_rebaseline_modifier(expectation):
            return not (not expectation.is_malformed() and expectation.name in except_these_tests and "rebaseline" in expectation.modifiers)

        return TestExpectationSerializer.list_to_string(filter(without_rebaseline_modifier, self._expectations))

    def _add_expectations(self, expectation_list, overrides_allowed):
        for expectation_line in expectation_list:
            if not expectation_line.expectations:
                continue

            self._parser.parse(expectation_line)
            if self._test_config in expectation_line.matching_configurations:
                self._model.add_expectation_line(expectation_line, overrides_allowed)
