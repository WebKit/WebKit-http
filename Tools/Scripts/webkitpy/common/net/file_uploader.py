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

from __future__ import with_statement

import codecs
import mimetypes
import socket
import urllib2

from webkitpy.common.net.networktransaction import NetworkTransaction


def get_mime_type(filename):
    return mimetypes.guess_type(filename)[0] or 'application/octet-stream'


# FIXME: Rather than taking tuples, this function should take more structured data.
def _encode_multipart_form_data(fields, files):
    """Encode form fields for multipart/form-data.

    Args:
      fields: A sequence of (name, value) elements for regular form fields.
      files: A sequence of (name, filename, value) elements for data to be
             uploaded as files.
    Returns:
      (content_type, body) ready for httplib.HTTP instance.

    Source:
      http://code.google.com/p/rietveld/source/browse/trunk/upload.py
    """
    BOUNDARY = '-M-A-G-I-C---B-O-U-N-D-A-R-Y-'
    CRLF = '\r\n'
    lines = []

    for key, value in fields:
        lines.append('--' + BOUNDARY)
        lines.append('Content-Disposition: form-data; name="%s"' % key)
        lines.append('')
        if isinstance(value, unicode):
            value = value.encode('utf-8')
        lines.append(value)

    for key, filename, value in files:
        lines.append('--' + BOUNDARY)
        lines.append('Content-Disposition: form-data; name="%s"; filename="%s"' % (key, filename))
        lines.append('Content-Type: %s' % get_mime_type(filename))
        lines.append('')
        if isinstance(value, unicode):
            value = value.encode('utf-8')
        lines.append(value)

    lines.append('--' + BOUNDARY + '--')
    lines.append('')
    body = CRLF.join(lines)
    content_type = 'multipart/form-data; boundary=%s' % BOUNDARY
    return content_type, body


class FileUploader(object):
    def __init__(self, url, timeout_seconds):
        self._url = url
        self._timeout_seconds = timeout_seconds

    def upload_single_text_file(self, filesystem, content_type, filename):
        self._upload_data(content_type, filesystem.read_text_file(filename))

    def upload_as_multipart_form_data(self, filesystem, files, params, timeout_seconds):
        file_objs = []
        for filename, path in files:
            # FIXME: We should talk to the filesytem via a Host object.
            file_objs.append(('file', filename, filesystem.read_text_file(filename)))

        # FIXME: We should use the same variable names for the formal and actual parameters.
        content_type, data = _encode_multipart_form_data(attrs, file_objs)
        self._upload_data(content_type, data)

    def _upload_data(self, content_type, data):
        def callback():
            request = urllib2.Request(self._url, data, {"Content-Type": content_type})
            urllib2.urlopen(request)

        orig_timeout = socket.getdefaulttimeout()
        try:
            # FIXME: We shouldn't mutate global static state.
            socket.setdefaulttimeout(self._timeout_seconds)
            NetworkTransaction(timeout_seconds=self._timeout_seconds).run(callback)
        finally:
            socket.setdefaulttimeout(orig_timeout)
