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

from datetime import datetime
from django.utils import simplejson
import logging

from model.testfile import TestFile

JSON_RESULTS_FILE = "results.json"
JSON_RESULTS_FILE_SMALL = "results-small.json"
JSON_RESULTS_PREFIX = "ADD_RESULTS("
JSON_RESULTS_SUFFIX = ");"
JSON_RESULTS_VERSION_KEY = "version"
JSON_RESULTS_BUILD_NUMBERS = "buildNumbers"
JSON_RESULTS_TESTS = "tests"
JSON_RESULTS_RESULTS = "results"
JSON_RESULTS_TIMES = "times"
JSON_RESULTS_PASS = "P"
JSON_RESULTS_NO_DATA = "N"
JSON_RESULTS_MIN_TIME = 1
JSON_RESULTS_HIERARCHICAL_VERSION = 4
JSON_RESULTS_MAX_BUILDS = 750
JSON_RESULTS_MAX_BUILDS_SMALL = 200


def _add_path_to_trie(path, value, trie):
    if not "/" in path:
        trie[path] = value
        return

    directory, slash, rest = path.partition("/")
    if not directory in trie:
        trie[directory] = {}
    _add_path_to_trie(rest, value, trie[directory])


def _trie_json_tests(tests):
    """Breaks a test name into chunks by directory and puts the test time as a value in the lowest part, e.g.
    foo/bar/baz.html: VALUE1
    foo/bar/baz1.html: VALUE2

    becomes
    foo: {
        bar: {
            baz.html: VALUE1,
            baz1.html: VALUE2
        }
    }
    """
    trie = {}
    for test, value in tests.iteritems():
        _add_path_to_trie(test, value, trie)
    return trie


class JsonResults(object):
    @classmethod
    def _strip_prefix_suffix(cls, data):
        # FIXME: Stop stripping jsonp callback once we upload pure json everywhere.
        if data.startswith(JSON_RESULTS_PREFIX) and data.endswith(JSON_RESULTS_SUFFIX):
            return data[len(JSON_RESULTS_PREFIX):len(data) - len(JSON_RESULTS_SUFFIX)]
        return data

    @classmethod
    def _generate_file_data(cls, json, sort_keys=False):
        return simplejson.dumps(json, separators=(',', ':'), sort_keys=sort_keys)

    @classmethod
    def _load_json(cls, file_data):
        json_results_str = cls._strip_prefix_suffix(file_data)
        if not json_results_str:
            logging.warning("No json results data.")
            return None

        try:
            return simplejson.loads(json_results_str)
        except Exception, err:
            logging.debug(json_results_str)
            logging.error("Failed to load json results: %s", str(err))
            return None

    @classmethod
    def _merge_json(cls, aggregated_json, incremental_json, num_runs):
        cls._merge_non_test_data(aggregated_json, incremental_json, num_runs)
        incremental_tests = incremental_json[JSON_RESULTS_TESTS]
        if incremental_tests:
            aggregated_tests = aggregated_json[JSON_RESULTS_TESTS]
            cls._merge_tests(aggregated_tests, incremental_tests, num_runs)

    @classmethod
    def _merge_non_test_data(cls, aggregated_json, incremental_json, num_runs):
        incremental_builds = incremental_json[JSON_RESULTS_BUILD_NUMBERS]
        aggregated_builds = aggregated_json[JSON_RESULTS_BUILD_NUMBERS]
        aggregated_build_number = int(aggregated_builds[0])

        for index in reversed(range(len(incremental_builds))):
            build_number = int(incremental_builds[index])
            logging.debug("Merging build %s, incremental json index: %d.", build_number, index)

            # Merge this build into aggreagated results.
            cls._merge_one_build(aggregated_json, incremental_json, index, num_runs)

    @classmethod
    def _merge_one_build(cls, aggregated_json, incremental_json, incremental_index, num_runs):
        for key in incremental_json.keys():
            # Merge json results except "tests" properties (results, times etc).
            # "tests" properties will be handled separately.
            if key == JSON_RESULTS_TESTS:
                continue

            if key in aggregated_json:
                aggregated_json[key].insert(0, incremental_json[key][incremental_index])
                aggregated_json[key] = aggregated_json[key][:num_runs]
            else:
                aggregated_json[key] = incremental_json[key]

    @classmethod
    def _merge_tests(cls, aggregated_json, incremental_json, num_runs):
        all_tests = set(aggregated_json.iterkeys())
        if incremental_json:
            all_tests |= set(incremental_json.iterkeys())

        for test_name in all_tests:
            if test_name not in aggregated_json:
                aggregated_json[test_name] = incremental_json[test_name]
                continue

            incremental_sub_result = incremental_json[test_name] if incremental_json and test_name in incremental_json else None
            if JSON_RESULTS_RESULTS not in aggregated_json[test_name]:
                cls._merge_tests(aggregated_json[test_name], incremental_sub_result, num_runs)
                continue

            if incremental_sub_result:
                results = incremental_sub_result[JSON_RESULTS_RESULTS]
                times = incremental_sub_result[JSON_RESULTS_TIMES]
            else:
                results = [[1, JSON_RESULTS_NO_DATA]]
                times = [[1, 0]]

            aggregated_test = aggregated_json[test_name]
            cls._insert_item_run_length_encoded(results, aggregated_test[JSON_RESULTS_RESULTS], num_runs)
            cls._insert_item_run_length_encoded(times, aggregated_test[JSON_RESULTS_TIMES], num_runs)
            cls._normalize_results_json(test_name, aggregated_json, num_runs)

    @classmethod
    def _insert_item_run_length_encoded(cls, incremental_item, aggregated_item, num_runs):
        for item in incremental_item:
            if len(aggregated_item) and item[1] == aggregated_item[0][1]:
                aggregated_item[0][0] = min(aggregated_item[0][0] + item[0], num_runs)
            else:
                aggregated_item.insert(0, item)

    @classmethod
    def _normalize_results_json(cls, test_name, aggregated_json, num_runs):
        aggregated_test = aggregated_json[test_name]
        aggregated_test[JSON_RESULTS_RESULTS] = cls._remove_items_over_max_number_of_builds(aggregated_test[JSON_RESULTS_RESULTS], num_runs)
        aggregated_test[JSON_RESULTS_TIMES] = cls._remove_items_over_max_number_of_builds(aggregated_test[JSON_RESULTS_TIMES], num_runs)

        is_all_pass = cls._is_results_all_of_type(aggregated_test[JSON_RESULTS_RESULTS], JSON_RESULTS_PASS)
        is_all_no_data = cls._is_results_all_of_type(aggregated_test[JSON_RESULTS_RESULTS], JSON_RESULTS_NO_DATA)

        max_time = max([time[1] for time in aggregated_test[JSON_RESULTS_TIMES]])
        if (is_all_no_data or (is_all_pass and max_time < JSON_RESULTS_MIN_TIME)):
            del aggregated_json[test_name]

    @classmethod
    def _remove_items_over_max_number_of_builds(cls, encoded_list, num_runs):
        num_builds = 0
        index = 0
        for result in encoded_list:
            num_builds = num_builds + result[0]
            index = index + 1
            if num_builds >= num_runs:
                return encoded_list[:index]

        return encoded_list

    @classmethod
    def _is_results_all_of_type(cls, results, type):
        return len(results) == 1 and results[0][1] == type

    @classmethod
    def _remove_gtest_modifiers(cls, builder, json):
        tests = json[builder][JSON_RESULTS_TESTS]
        new_tests = {}
        # FIXME: This is wrong. If the test exists in the incremental results as both values, then one will overwrite the other.
        # We should instead pick the one that doesn't have NO_DATA as its value.
        # Alternately we could fix this by having the JSON generation code on the buildbot only include the test
        # that was actually run.
        for name, test in tests.iteritems():
            new_name = name.replace('.FLAKY_', '.', 1)
            new_name = new_name.replace('.FAILS_', '.', 1)
            new_name = new_name.replace('.MAYBE_', '.', 1)
            new_name = new_name.replace('.DISABLED_', '.', 1)
            new_tests[new_name] = test
        json[builder][JSON_RESULTS_TESTS] = new_tests

    @classmethod
    def _check_json(cls, builder, json):
        version = json[JSON_RESULTS_VERSION_KEY]
        if version > JSON_RESULTS_HIERARCHICAL_VERSION:
            logging.error("Results JSON version '%s' is not supported.",
                version)
            return False

        if not builder in json:
            logging.error("Builder '%s' is not in json results.", builder)
            return False

        results_for_builder = json[builder]
        if not JSON_RESULTS_BUILD_NUMBERS in results_for_builder:
            logging.error("Missing build number in json results.")
            return False

        # FIXME: Once all the bots have cycled, we can remove this code since all the results will be heirarchical.
        if version < JSON_RESULTS_HIERARCHICAL_VERSION:
            json[builder][JSON_RESULTS_TESTS] = _trie_json_tests(results_for_builder[JSON_RESULTS_TESTS])
            json[JSON_RESULTS_VERSION_KEY] = JSON_RESULTS_HIERARCHICAL_VERSION

        return True

    @classmethod
    def merge(cls, builder, aggregated, incremental, num_runs, sort_keys=False):
        if not incremental:
            logging.warning("Nothing to merge.")
            return None

        logging.info("Loading incremental json...")
        incremental_json = cls._load_json(incremental)
        if not incremental_json:
            return None

        logging.info("Checking incremental json...")
        if not cls._check_json(builder, incremental_json):
            return None

        # FIXME: We should probably avoid doing this for layout tests.
        cls._remove_gtest_modifiers(builder, incremental_json)

        logging.info("Loading existing aggregated json...")
        aggregated_json = cls._load_json(aggregated)
        if not aggregated_json:
            return incremental

        logging.info("Checking existing aggregated json...")
        if not cls._check_json(builder, aggregated_json):
            return incremental

        logging.info("Merging json results...")
        try:
            cls._merge_json(aggregated_json[builder], incremental_json[builder], num_runs)
        except Exception, err:
            logging.error("Failed to merge json results: %s", str(err))
            return None

        aggregated_json[JSON_RESULTS_VERSION_KEY] = JSON_RESULTS_HIERARCHICAL_VERSION

        return cls._generate_file_data(aggregated_json, sort_keys)

    @classmethod
    def update(cls, master, builder, test_type, incremental):
        small_file_updated = cls.update_file(master, builder, test_type, incremental, JSON_RESULTS_FILE_SMALL, JSON_RESULTS_MAX_BUILDS_SMALL)
        large_file_updated = cls.update_file(master, builder, test_type, incremental, JSON_RESULTS_FILE, JSON_RESULTS_MAX_BUILDS)

        return small_file_updated and large_file_updated

    @classmethod
    def update_file(cls, master, builder, test_type, incremental, filename, num_runs):
        files = TestFile.get_files(master, builder, test_type, filename)
        if files:
            file = files[0]
            new_results = cls.merge(builder, file.data, incremental, num_runs)
        else:
            # Use the incremental data if there is no aggregated file to merge.
            file = TestFile()            
            file.master = master
            file.builder = builder
            file.test_type = test_type
            file.name = filename
            new_results = incremental
            logging.info("No existing json results, incremental json is saved.")

        if not new_results or not file.save(new_results):
            logging.info("Update failed, master: %s, builder: %s, test_type: %s, name: %s." % (master, builder, test_type, filename))
            return False

        return True

    @classmethod
    def get_test_list(cls, builder, json_file_data):
        logging.debug("Loading test results json...")
        json = cls._load_json(json_file_data)
        if not json:
            return None

        logging.debug("Checking test results json...")
        if not cls._check_json(builder, json):
            return None

        test_list_json = {}
        tests = json[builder][JSON_RESULTS_TESTS]
        test_list_json[builder] = {"tests": dict.fromkeys(tests, {})}

        return cls._generate_file_data(test_list_json)
