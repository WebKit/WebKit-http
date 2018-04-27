# Copyright (C) 2014-2018 Apple Inc. All rights reserved.
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

import os
import json
import sys
import subprocess
import shutil
import logging
from zipfile import ZipFile, BadZipfile
from urllib2 import urlopen, HTTPError, URLError
from webkitpy.common.webkit_finder import WebKitFinder
from webkitpy.port.ios import IOSPort


_log = logging.getLogger(__name__)

logging.basicConfig()

_log.setLevel(logging.INFO)



class BuildBinariesFetcher:
    """A class to which automates the fetching of the build binaries revisions."""

    def __init__(self, host, port_name, architecture, configuration, build_binaries_revision=None):
        """ Initialize the build url options needed to construct paths"""
        self.host = host
        self.port_name = port_name
        self.os_version_name = self._get_os_version_name()
        self.architecture = architecture
        self.configuration = configuration
        self.build_binaries_revision = build_binaries_revision
        self.s3_zip_url = None

        # FIXME find version of this endpoint which returns more than the latest 30 builds
        self.s3_build_binaries_api_base_path = 'https://q1tzqfy48e.execute-api.us-west-2.amazonaws.com/v2/latest'

    @property
    def downloaded_binaries_dir(self):
        webkit_finder = WebKitFinder(self.host.filesystem)
        return webkit_finder.path_from_webkit_base('WebKitBuild', 'downloaded_binaries')

    @property
    def should_default_to_latest_revision(self):
        return self.build_binaries_revision == None

    @property
    def s3_build_type(self):
        return "{self.port_name}-{self.os_version_name}-{self.architecture}-{self.configuration}".format(self=self).lower()

    @property
    def s3_build_binaries_url(self):
        return self.host.filesystem.join(self.s3_build_binaries_api_base_path, self.s3_build_type)

    @property
    def local_build_binaries_dir(self):
        return self.host.filesystem.join(self.downloaded_binaries_dir, self.s3_build_type, self.build_binaries_revision)

    @property
    def local_zip_path(self):
        return "{self.local_build_binaries_dir}.zip".format(self=self)

    def _get_os_version_name(self):
        if self.port_name == "mac":
            return self.host.platform.os_version_name().lower().replace(' ', '')
        elif self.port_name == "ios-simulator":
            return IOSPort.CURRENT_VERSION.major
        else:
            raise NotImplementedError('Downloading binaries for the %s is not currently supported' % self.port_name)

    def get_path(self):
        try:
            if not self.build_binaries_revision:
                self.build_binaries_revision = self._get_latest_build_revision()

            # check to see if previously downloaded local version exists before downloading
            if self.host.filesystem.exists(self.local_build_binaries_dir):
                _log.info('Build Binary has been previously dowloaded and can be found at: %s' % self.local_build_binaries_dir)
                return self.local_build_binaries_dir

            _log.info('Fetching %s builds' % self.s3_build_type)
            return self._fetch_build_binaries_json()
        except Exception as error:
            self.clean_up_on_error()
            raise error

    def _get_build_binaries_json(self):
        response = urlopen(self.s3_build_binaries_url)
        build_binaries_json = json.load(response)

        if build_binaries_json['Count'] == 0:
            raise Exception('No build revisions found at: %s' % self.s3_build_binaries_url)

        return build_binaries_json

    def _get_latest_build_revision(self):
        build_binaries_json = self._get_build_binaries_json()
        latest_build_revision = build_binaries_json['Items'][0]['revision']['N']

        _log.info('Defaulting to fetching the latest build revision: %s' % latest_build_revision)
        return build_binaries_json['Items'][0]['revision']['N']

    def _fetch_build_binaries_json(self):
        build_binaries_json = self._get_build_binaries_json()

        if self.should_default_to_latest_revision:
            self.s3_zip_url = build_binaries_json['Items'][0]['revision']['N']
        else:
            for item in build_binaries_json['Items']:
                revision = item['revision']['N']
                if revision == self.build_binaries_revision:
                    self.s3_zip_url = item['s3_url']['S']
                    break

        if self.s3_zip_url:
            return self._fetch_build_binaries_zip()
        else:
            raise Exception('Could not find revision %s in the constructed API path: %s'
                            % (self.build_binaries_revision, self.s3_build_binaries_url))

    def _fetch_build_binaries_zip(self):

        try:
            # attempt to download the zip file
            _log.info("Starting ZipFile Download: %s" % self.s3_zip_url)
            build_zip = urlopen(self.s3_zip_url)

            self.host.filesystem.maybe_make_directory(self.local_build_binaries_dir)

            with open(self.local_zip_path, "wb") as local_build_binaries:
                _log.info("Writing ZipFile To Local Drive: %s" % self.local_zip_path)
                local_build_binaries.write(build_zip.read())

            _log.info("Extracting ZipFile")

            extract_exception = Exception('Cannot extract zipfile %s' % self.local_zip_path)
            if sys.platform == 'darwin':
                if subprocess.call(["ditto", "-x", "-k", self.local_zip_path, self.local_build_binaries_dir]):
                    raise extract_exception
            elif sys.platform == 'cygwin' or sys.platform.startswith('linux'):
                if subprocess.call(["unzip", "-o", self.local_zip_path], cwd=self.local_build_binaries_dir):
                    raise extract_exception
            elif sys.platform == 'win32':
                archive = ZipFile(self.local_zip_path, "r")
                archive.extractall(self.local_build_binaries_dir)
                archive.close()

            _log.info("Deleting ZipFile Extracted Binaries Can Be Found Here: %s" % self.local_build_binaries_dir)
            os.remove(self.local_zip_path)

            return self.local_build_binaries_dir

        except BadZipfile:
            raise Exception('BadZipfile Error: could not exact ZipFile')
        except HTTPError:
            raise Exception('HTTP Error: internet connectivity is required fetch binary file')
        except URLError:
            raise Exception('URLError Error: please make sure %s is a valid link' % self.s3_build_binaries_url)

    def clean_up_on_error(self):
        """ Delete newly creating files & directories which may be corrupt or incomplete"""
        if self.host.filesystem.exists(self.local_build_binaries_dir):
            shutil.rmtree(self.local_build_binaries_dir)

        if self.host.filesystem.exists(self.local_zip_path):
            os.remove(self.local_zip_path)
