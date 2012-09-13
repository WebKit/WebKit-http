# Copyright (C) 2011 Google Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
# 1.  Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
# 2.  Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
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

import BaseHTTPServer
import logging
import json
import os

from webkitpy.common.memoized import memoized
from webkitpy.tool.servers.reflectionhandler import ReflectionHandler
from webkitpy.layout_tests.port import builders


_log = logging.getLogger(__name__)


class BuildCoverageExtrapolator(object):
    def __init__(self, test_configuration_converter):
        self._test_configuration_converter = test_configuration_converter

    @memoized
    def _covered_test_configurations_for_builder_name(self):
        coverage = {}
        for builder_name in builders.all_builder_names():
            coverage[builder_name] = self._test_configuration_converter.to_config_set(builders.coverage_specifiers_for_builder_name(builder_name))
        return coverage

    def extrapolate_test_configurations(self, builder_name):
        return self._covered_test_configurations_for_builder_name()[builder_name]


class GardeningHTTPServer(BaseHTTPServer.HTTPServer):
    def __init__(self, httpd_port, config):
        server_name = ''
        self.tool = config['tool']
        BaseHTTPServer.HTTPServer.__init__(self, (server_name, httpd_port), GardeningHTTPRequestHandler)

    def url(self):
        return 'file://' + os.path.join(GardeningHTTPRequestHandler.STATIC_FILE_DIRECTORY, 'garden-o-matic.html')


class GardeningHTTPRequestHandler(ReflectionHandler):
    STATIC_FILE_NAMES = frozenset()

    STATIC_FILE_DIRECTORY = os.path.join(
        os.path.dirname(__file__),
        '..',
        '..',
        '..',
        '..',
        'BuildSlaveSupport',
        'build.webkit.org-config',
        'public_html',
        'TestFailures')

    allow_cross_origin_requests = True

    def _run_webkit_patch(self, args):
        return self.server.tool.executive.run_command([self.server.tool.path()] + args, cwd=self.server.tool.scm().checkout_root)

    def rollout(self):
        revision = self.query['revision'][0]
        reason = self.query['reason'][0]
        self._run_webkit_patch([
            'rollout',
            '--force-clean',
            '--non-interactive',
            revision,
            reason,
        ])
        self._serve_text('success')

    def ping(self):
        self._serve_text('pong')

    def rebaselineall(self):
        command = ['rebaseline-json']
        json_input = self.read_entity_body()
        _log.debug("rebaselining using '%s'" % json_input)

        def error_handler(script_error):
            _log.error("error from rebaseline-json: %s, input='%s', output='%s'" % (str(script_error), json_input, script_error.output))

        self.server.tool.executive.run_command([self.server.tool.path()] + command, input=json_input, cwd=self.server.tool.scm().checkout_root, return_stderr=True, error_handler=error_handler)
        self._serve_text('success')
