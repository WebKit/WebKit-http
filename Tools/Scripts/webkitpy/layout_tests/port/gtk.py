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
#     * Neither the Google name nor the names of its
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

"""WebKit Gtk implementation of the Port interface."""

import logging
import os
import signal
import subprocess

from webkitpy.layout_tests.port import base, builders, server_process, webkit


_log = logging.getLogger(__name__)


class GtkDriver(webkit.WebKitDriver):
    def start(self):
        display_id = self._worker_number + 1
        run_xvfb = ["Xvfb", ":%d" % (display_id), "-screen",  "0", "800x600x24", "-nolisten", "tcp"]
        self._xvfb_process = subprocess.Popen(run_xvfb)
        environment = self._port.setup_environ_for_server()
        # We must do this here because the DISPLAY number depends on _worker_number
        environment['DISPLAY'] = ":%d" % (display_id)
        self._server_process = server_process.ServerProcess(self._port,
            self._port.driver_name(), self.cmd_line(), environment)

    def stop(self):
        webkit.WebKitDriver.stop(self)
        os.kill(self._xvfb_process.pid, signal.SIGTERM)
        self._xvfb_process.wait()


class GtkPort(webkit.WebKitPort):
    port_name = "gtk"

    def create_driver(self, worker_number):
        return GtkDriver(self, worker_number)

    def setup_environ_for_server(self):
        environment = webkit.WebKitPort.setup_environ_for_server(self)
        environment['GTK_MODULES'] = 'gail'
        environment['LIBOVERLAY_SCROLLBAR'] = '0'
        environment['WEBKIT_INSPECTOR_PATH'] = self._build_path('Programs/resources/inspector')

        return environment

    def _path_to_apache_config_file(self):
        # FIXME: This needs to detect the distribution and change config files.
        return self._filesystem.join(self.layout_tests_dir(), 'http', 'conf', 'apache2-debian-httpd.conf')

    def _path_to_driver(self):
        return self._build_path('Programs', self.driver_name())

    def check_build(self, needs_http):
        return self._check_driver()

    def _path_to_apache(self):
        if self._is_redhat_based():
            return '/usr/sbin/httpd'
        else:
            return '/usr/sbin/apache2'

    def _path_to_apache_config_file(self):
        if self._is_redhat_based():
            config_name = 'fedora-httpd.conf'
        else:
            config_name = 'apache2-debian-httpd.conf'

        return self._filesystem.join(self.layout_tests_dir(), 'http', 'conf', config_name)

    def _path_to_wdiff(self):
        if self._is_redhat_based():
            return '/usr/bin/dwdiff'
        else:
            return '/usr/bin/wdiff'

    def _is_redhat_based(self):
        return self._filesystem.exists(self._filesystem.join('/etc', 'redhat-release'))

    def _path_to_webcore_library(self):
        gtk_library_names = [
            "libwebkitgtk-1.0.so",
            "libwebkitgtk-3.0.so",
            "libwebkit2gtk-1.0.so",
        ]

        for library in gtk_library_names:
            full_library = self._build_path(".libs", library)
            if os.path.isfile(full_library):
                return full_library
        return None

    def _runtime_feature_list(self):
        return None
