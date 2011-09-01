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

import re

from webkitpy.common.system.executive import ScriptError


# If other ports/platforms decide to support --leaks, we should see about sharing as much of this code as possible.
# Right now this code is only used by Apple's MacPort.

class LeakDetector(object):
    def __init__(self, port):
        # We should operate on a "platform" not a port here.
        self._port = port
        self._executive = port._executive
        self._filesystem = port._filesystem

    # We exclude the following reported leaks so they do not get in our way when looking for WebKit leaks:
    # This allows us ignore known leaks and only be alerted when new leaks occur. Some leaks are in the old
    # versions of the system frameworks that are being used by the leaks bots. Even though a leak has been
    # fixed, it will be listed here until the bot has been updated with the newer frameworks.
    def _types_to_exlude_from_leaks(self):
        # Currently we don't have any type excludes from OS leaks, but we will likely again in the future.
        return []

    def _callstacks_to_exclude_from_leaks(self):
        callstacks = [
            "Flash_EnforceLocalSecurity",  # leaks in Flash plug-in code, rdar://problem/4449747
        ]
        if self._port.is_leopard():
            callstacks += [
                "CFHTTPMessageAppendBytes",  # leak in CFNetwork, rdar://problem/5435912
                "sendDidReceiveDataCallback",  # leak in CFNetwork, rdar://problem/5441619
                "_CFHTTPReadStreamReadMark",  # leak in CFNetwork, rdar://problem/5441468
                "httpProtocolStart",  # leak in CFNetwork, rdar://problem/5468837
                "_CFURLConnectionSendCallbacks",  # leak in CFNetwork, rdar://problem/5441600
                "DispatchQTMsg",  # leak in QuickTime, PPC only, rdar://problem/5667132
                "QTMovieContentView createVisualContext",  # leak in QuickTime, PPC only, rdar://problem/5667132
                "_CopyArchitecturesForJVMVersion",  # leak in Java, rdar://problem/5910823
            ]
        elif self._port.is_snowleopard():
            callstacks += [
                "readMakerNoteProps",  # <rdar://problem/7156432> leak in ImageIO
                "QTKitMovieControllerView completeUISetup",  # <rdar://problem/7155156> leak in QTKit
                "getVMInitArgs",  # <rdar://problem/7714444> leak in Java
                "Java_java_lang_System_initProperties",  # <rdar://problem/7714465> leak in Java
                "glrCompExecuteKernel",  # <rdar://problem/7815391> leak in graphics driver while using OpenGL
                "NSNumberFormatter getObjectValue:forString:errorDescription:",  # <rdar://problem/7149350> Leak in NSNumberFormatter
            ]
        return callstacks

    def _leaks_args(self, pid):
        leaks_args = []
        for callstack in self._callstacks_to_exclude_from_leaks():
            leaks_args += ['--exclude-callstack="%s"' % callstack]  # Callstacks can have spaces in them, so we quote the arg to prevent confusing perl's optparse.
        for excluded_type in self._types_to_exlude_from_leaks():
            leaks_args += ['--exclude-type="%s"' % excluded_type]
        leaks_args.append(pid)
        return leaks_args

    def _parse_leaks_output(self, leaks_output, process_pid):
        count, bytes = re.search(r'Process %s: (\d+) leaks? for (\d+) total' % process_pid, leaks_output).groups()
        excluded_match = re.search(r'(\d+) leaks? excluded', leaks_output)
        excluded = excluded_match.group(0) if excluded_match else 0
        return int(count), int(excluded), int(bytes)

    def leaks_files_in_directory(self, directory):
        return self._filesystem.glob(self._filesystem.join(directory, "*-leaks.txt"))

    def leaks_file_name(self, process_name, process_pid):
        # We include the number of files this worker has already written in the name to prevent overwritting previous leak results..
        return "%s-%s-leaks.txt" % (process_name, process_pid)

    def parse_leak_files(self, leak_files):
        merge_depth = 5  # ORWT had a --merge-leak-depth argument, but that seems out of scope for the run-webkit-tests tool.
        args = [
            '--merge-depth',
            merge_depth,
        ] + leak_files
        try:
            parse_malloc_history_output = self._port._run_script("parse-malloc-history", args, include_configuration_arguments=False)
        except ScriptError, e:
            _log.warn("Failed to parse leaks output: %s" % e.message_with_output())
            return

        # total: 5,888 bytes (0 bytes excluded).
        unique_leak_count = len(re.findall(r'^(\d*)\scalls', parse_malloc_history_output))
        total_bytes_string = re.search(r'^total\:\s(.+)\s\(', parse_malloc_history_output, re.MULTILINE).group(1)
        return (total_bytes_string, unique_leak_count)

    def check_for_leaks(self, process_name, process_pid):
        _log.debug("Checking for leaks in %s" % process_name)
        try:
            # Oddly enough, run-leaks (or the underlying leaks tool) does not seem to always output utf-8,
            # thus we pass decode_output=False.  Without this code we've seen errors like:
            # "UnicodeDecodeError: 'utf8' codec can't decode byte 0x88 in position 779874: unexpected code byte"
            leaks_output = self._port._run_script("run-leaks", self._leaks_args(process_pid), include_configuration_arguments=False, decode_output=False)
        except ScriptError, e:
            _log.warn("Failed to run leaks tool: %s" % e.message_with_output())
            return

        # FIXME: We should consider moving leaks parsing to the end when summarizing is done.
        count, excluded, bytes = self._parse_leaks_output(leaks_output, process_pid)
        adjusted_count = count - excluded
        if not adjusted_count:
            return

        leaks_filename = self.leaks_file_name(process_name, process_pid)
        leaks_output_path = self._filesystem.join(self._port.results_directory(), leaks_filename)
        self._filesystem.write_binary_file(leaks_output_path, leaks_output)

        # FIXME: Ideally we would not be logging from the worker process, but rather pass the leak
        # information back to the manager and have it log.
        if excluded:
            _log.info("%s leaks (%s bytes including %s excluded leaks) were found, details in %s" % (adjusted_count, bytes, excluded, leaks_output_path))
        else:
            _log.info("%s leaks (%s bytes) were found, details in %s" % (count, bytes, leaks_output_path))
