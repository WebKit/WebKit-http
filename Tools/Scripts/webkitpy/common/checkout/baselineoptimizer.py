# Copyright (C) 2011, Google Inc. All rights reserved.
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

from webkitpy.layout_tests.port import factory as port_factory


# Yes, it's a hypergraph.
# FIXME: Should this function live with the ports somewhere?
def _baseline_search_hypergraph(fs):
    hypergraph = {}

    # These edges in the hypergraph aren't visible on build.webkit.org,
    # but they impose constraints on how we optimize baselines.
    hypergraph['mac-future'] = ['LayoutTests/platform/mac-future', 'LayoutTests/platform/mac', 'LayoutTests']
    hypergraph['qt-unknown'] = ['LayoutTests/platform/qt-unknown', 'LayoutTests/platform/qt', 'LayoutTests']

    # FIXME: Should we get this constant from somewhere?
    fallback_path = ['LayoutTests']

    for port_name in port_factory.all_port_names():
        port = port_factory.get(port_name)
        webkit_base = port.webkit_base()
        search_path = port.baseline_search_path()
        if search_path:
            hypergraph[port_name] = [fs.relpath(path, webkit_base) for path in search_path] + fallback_path
    return hypergraph


# FIXME: Should this function be somewhere more general?
def _invert_dictionary(dictionary):
    inverted_dictionary = {}
    for key, value in dictionary.items():
        if inverted_dictionary.get(value):
            inverted_dictionary[value].append(key)
        else:
            inverted_dictionary[value] = [key]
    return inverted_dictionary


class BaselineOptimizer(object):
    def __init__(self, scm, filesystem):
        self._scm = scm
        self._filesystem = filesystem
        self._hypergraph = _baseline_search_hypergraph(self._filesystem)
        self._directories = reduce(set.union, map(set, self._hypergraph.values()))

    def _read_results_by_directory(self, baseline_name):
        results_by_directory = {}
        for directory in self._directories:
            path = self._filesystem.join(self._scm.checkout_root, directory, baseline_name)
            if self._filesystem.exists(path):
                results_by_directory[directory] = self._filesystem.sha1(path)
        return results_by_directory

    def _results_by_port_name(self, results_by_directory):
        results_by_port_name = {}
        for port_name, search_path in self._hypergraph.items():
            for directory in search_path:
                if directory in results_by_directory:
                    results_by_port_name[port_name] = results_by_directory[directory]
                    break
        return results_by_port_name

    def _most_specific_common_directory(self, port_names):
        paths = [self._hypergraph[port_name] for port_name in port_names]
        common_directories = reduce(set.intersection, map(set, paths))

        def score(directory):
            return sum([path.index(directory) for path in paths])

        _, directory = sorted([(score(directory), directory) for directory in common_directories])[0]
        return directory

    def _filter_port_names_by_result(self, predicate, port_names_by_result):
        filtered_port_names_by_result = {}
        for result, port_names in port_names_by_result.items():
            filtered_port_names = filter(predicate, port_names)
            if filtered_port_names:
                filtered_port_names_by_result[result] = filtered_port_names
        return filtered_port_names_by_result

    def _place_results_in_most_specific_common_directory(self, port_names_by_result, results_by_directory):
        for result, port_names in port_names_by_result.items():
            directory = self._most_specific_common_directory(port_names)
            results_by_directory[directory] = result

    def _find_optimal_result_placement(self, baseline_name):
        results_by_directory = self._read_results_by_directory(baseline_name)
        results_by_port_name = self._results_by_port_name(results_by_directory)
        port_names_by_result = _invert_dictionary(results_by_port_name)

        new_results_by_directory = {}
        unsatisfied_port_names_by_result = port_names_by_result
        while unsatisfied_port_names_by_result:
            self._place_results_in_most_specific_common_directory(unsatisfied_port_names_by_result, new_results_by_directory)
            new_results_by_port_name = self._results_by_port_name(new_results_by_directory)

            def is_unsatisfied(port_name):
                return results_by_port_name[port_name] != new_results_by_port_name[port_name]

            new_unsatisfied_port_names_by_result = self._filter_port_names_by_result(is_unsatisfied, port_names_by_result)

            if len(new_unsatisfied_port_names_by_result.values()) >= len(unsatisfied_port_names_by_result.values()):
                break  # Frowns. We do not appear to be converging.
            unsatisfied_port_names_by_result = new_unsatisfied_port_names_by_result

        return results_by_directory, new_results_by_directory

    def _move_baselines(self, baseline_name, results_by_directory, new_results_by_directory):
        data_for_result = {}
        for directory, result in results_by_directory.items():
            if not result in data_for_result:
                source = self._filesystem.join(self._scm.checkout_root, directory, baseline_name)
                data_for_result[result] = self._filesystem.read_binary_file(source)

        for directory, result in results_by_directory.items():
            if new_results_by_directory.get(directory) != result:
                file_name = self._filesystem.join(self._scm.checkout_root, directory, baseline_name)
                self._scm.delete(file_name)

        for directory, result in new_results_by_directory.items():
            if results_by_directory.get(directory) != result:
                destination = self._filesystem.join(self._scm.checkout_root, directory, baseline_name)
                self._filesystem.maybe_make_directory(self._filesystem.split(destination)[0])
                self._filesystem.write_binary_file(destination, data_for_result[result])
                self._scm.add(destination)

    def optimize(self, baseline_name):
        results_by_directory, new_results_by_directory = self._find_optimal_result_placement(baseline_name)
        if self._results_by_port_name(results_by_directory) != self._results_by_port_name(new_results_by_directory):
            return False
        self._move_baselines(baseline_name, results_by_directory, new_results_by_directory)
        return True
