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

"""Factory method to retrieve the appropriate port implementation."""

import re
import sys

from webkitpy.layout_tests.port import builders


class BuilderOptions(object):
    def __init__(self, builder_name):
        self.configuration = "Debug" if re.search(r"[d|D](ebu|b)g", builder_name) else "Release"
        self.builder_name = builder_name


class PortFactory(object):
    def __init__(self, host=None):
        self._host = host

    def _port_name_from_arguments_and_options(self, **kwargs):
        port_to_use = kwargs.get('port_name', None)
        options = kwargs.get('options', None)
        if port_to_use is None:
            if sys.platform == 'win32' or sys.platform == 'cygwin':
                if options and hasattr(options, 'chromium') and options.chromium:
                    port_to_use = 'chromium-win'
                else:
                    port_to_use = 'win'
            elif sys.platform.startswith('linux'):
                port_to_use = 'chromium-linux'
            elif sys.platform == 'darwin':
                if options and hasattr(options, 'chromium') and options.chromium:
                    port_to_use = 'chromium-cg-mac'
                    # FIXME: Add a way to select the chromium-mac port.
                else:
                    port_to_use = 'mac'

        if port_to_use is None:
            raise NotImplementedError('unknown port; sys.platform = "%s"' % sys.platform)
        return port_to_use

    def _set_default_overriding_none(self, dictionary, key, default):
        # dict.setdefault almost works, but won't actually override None values, which we want.
        if not dictionary.get(key):
            dictionary[key] = default
        return dictionary[key]

    def _get_kwargs(self, host=None, **kwargs):
        port_to_use = self._port_name_from_arguments_and_options(**kwargs)

        if port_to_use.startswith('test'):
            import test
            maker = test.TestPort
            # FIXME: This is a hack to override the "default" filesystem
            # provided to the port.  The unit_test_filesystem has an extra
            # _tests attribute which a bunch of unittests depend on.
            from .test import unit_test_filesystem
            self._set_default_overriding_none(kwargs, 'filesystem', unit_test_filesystem())
        elif port_to_use.startswith('dryrun'):
            import dryrun
            maker = dryrun.DryRunPort
        elif port_to_use.startswith('mock-'):
            import mock_drt
            maker = mock_drt.MockDRTPort
        elif port_to_use.startswith('mac'):
            import mac
            maker = mac.MacPort
        elif port_to_use.startswith('win'):
            import win
            maker = win.WinPort
        elif port_to_use.startswith('gtk'):
            import gtk
            maker = gtk.GtkPort
        elif port_to_use.startswith('qt'):
            import qt
            maker = qt.QtPort
        elif port_to_use.startswith('chromium-gpu'):
            import chromium_gpu
            maker = chromium_gpu.get
        elif port_to_use.startswith('chromium-mac') or port_to_use.startswith('chromium-cg-mac'):
            import chromium_mac
            maker = chromium_mac.ChromiumMacPort
        elif port_to_use.startswith('chromium-linux'):
            import chromium_linux
            maker = chromium_linux.ChromiumLinuxPort
        elif port_to_use.startswith('chromium-win'):
            import chromium_win
            maker = chromium_win.ChromiumWinPort
        elif port_to_use.startswith('google-chrome'):
            import google_chrome
            maker = google_chrome.GetGoogleChromePort
        elif port_to_use.startswith('efl'):
            import efl
            maker = efl.EflPort
        else:
            raise NotImplementedError('unsupported port: %s' % port_to_use)

        host = host or self._host
        return maker(host, **kwargs)

    def all_port_names(self):
        """Return a list of all valid, fully-specified, "real" port names.

        This is the list of directories that are used as actual baseline_paths()
        by real ports. This does not include any "fake" names like "test"
        or "mock-mac", and it does not include any directories that are not ."""
        # FIXME: There's probably a better way to generate this list ...
        return builders.all_port_names()

    def get(self, port_name=None, options=None, **kwargs):
        """Returns an object implementing the Port interface. If
        port_name is None, this routine attempts to guess at the most
        appropriate port on this platform."""
        # Wrapped for backwards-compatibility
        if port_name:
            kwargs['port_name'] = port_name
        if options:
            kwargs['options'] = options
        return self._get_kwargs(**kwargs)

    def get_from_builder_name(self, builder_name):
        port_name = builders.port_name_for_builder_name(builder_name)
        assert(port_name)  # Need to update port_name_for_builder_name
        port = self.get(port_name, BuilderOptions(builder_name))
        assert(port)  # Need to update port_name_for_builder_name
        return port
