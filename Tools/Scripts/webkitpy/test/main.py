# Copyright (C) 2012 Google, Inc.
# Copyright (C) 2010 Chris Jerdonek (cjerdonek@webkit.org)
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1.  Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
# 2.  Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

"""unit testing code for webkitpy."""

import logging
import multiprocessing
import optparse
import StringIO
import sys
import traceback
import unittest

from webkitpy.common.system.filesystem import FileSystem
from webkitpy.test.finder import Finder
from webkitpy.test.printer import Printer
from webkitpy.test.runner import Runner

_log = logging.getLogger(__name__)


class Tester(object):
    def __init__(self, filesystem=None):
        self.finder = Finder(filesystem or FileSystem())
        self.printer = Printer(sys.stderr)
        self._options = None

    def add_tree(self, top_directory, starting_subdirectory=None):
        self.finder.add_tree(top_directory, starting_subdirectory)

    def _parse_args(self):
        parser = optparse.OptionParser(usage='usage: %prog [options] [args...]')
        parser.add_option('-a', '--all', action='store_true', default=False,
                          help='run all the tests')
        parser.add_option('-c', '--coverage', action='store_true', default=False,
                          help='generate code coverage info (requires http://pypi.python.org/pypi/coverage)')
        parser.add_option('-q', '--quiet', action='store_true', default=False,
                          help='run quietly (errors, warnings, and progress only)')
        parser.add_option('-t', '--timing', action='store_true', default=False,
                          help='display per-test execution time (implies --verbose)')
        parser.add_option('-v', '--verbose', action='count', default=0,
                          help='verbose output (specify once for individual test results, twice for debug messages)')
        parser.add_option('--skip-integrationtests', action='store_true', default=False,
                          help='do not run the integration tests')
        parser.add_option('-p', '--pass-through', action='store_true', default=False,
                          help='be debugger friendly by passing captured output through to the system')
        parser.add_option('-j', '--child-processes', action='store', type='int', default=(1 if sys.platform == 'win32' else multiprocessing.cpu_count()),
                          help='number of tests to run in parallel (default=%default)')

        parser.epilog = ('[args...] is an optional list of modules, test_classes, or individual tests. '
                         'If no args are given, all the tests will be run.')

        return parser.parse_args()

    def run(self):
        self._options, args = self._parse_args()
        self.printer.configure(self._options)

        self.finder.clean_trees()

        names = self.finder.find_names(args, self._options.skip_integrationtests, self._options.all, self._options.child_processes != 1)
        if not names:
            _log.error('No tests to run')
            return False

        return self._run_tests(names)

    def _run_tests(self, names):
        if self._options.coverage:
            try:
                import webkitpy.thirdparty.autoinstalled.coverage as coverage
            except ImportError:
                _log.error("Failed to import 'coverage'; can't generate coverage numbers.")
                return False
            cov = coverage.coverage()
            cov.start()

        # Make sure PYTHONPATH is set up properly.
        sys.path = self.finder.additional_paths(sys.path) + sys.path

        _log.debug("Loading the tests...")

        loader = unittest.defaultTestLoader
        suites = []
        for name in names:
            if self.finder.is_module(name):
                # if we failed to load a name and it looks like a module,
                # try importing it directly, because loadTestsFromName()
                # produces lousy error messages for bad modules.
                try:
                    __import__(name)
                except ImportError:
                    _log.fatal('Failed to import %s:' % name)
                    self._log_exception()
                    return False

            suites.append(loader.loadTestsFromName(name, None))

        test_suite = unittest.TestSuite(suites)
        test_runner = Runner(self.printer, self._options, loader)

        _log.debug("Running the tests.")
        result = test_runner.run(test_suite)
        if self._options.coverage:
            cov.stop()
            cov.save()
            cov.report(show_missing=False)
        return result.wasSuccessful()

    def _log_exception(self):
        s = StringIO.StringIO()
        traceback.print_exc(file=s)
        for l in s.buflist:
            _log.error('  ' + l.rstrip())
