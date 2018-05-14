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

REST_API_URL = 'https://q1tzqfy48e.execute-api.us-west-2.amazonaws.com/v2/'
REST_API_ENDPOINT = 'archives/'
REST_API_MINIFIED_ENDPOINT = 'minified-archives/'

_log = logging.getLogger(__name__)

logging.basicConfig()

_log.setLevel(logging.INFO)

class BuildBinariesFetcher:

    def __init__(self, host, platform, architecture='x86_64', configuration='release', full=False, revision=None, build_directory=None, delete_first=False, no_extract=False, url=None):
        self.host = host
        self.platform = platform
        self.architecture = architecture
        self.configuration = configuration
        self.revision = revision
        self.build_directory = build_directory
        self.should_use_download_unminified_url = full
        self.should_delete_first = delete_first
        self.should_not_extract = no_extract
        self.s3_zip_url = url

    @property
    def local_downloaded_binaries_directory(self):
        build_directory = ('WebkitBuild', )
        if self.build_directory:

            build_directory = ', '.join(self.build_directory.split('/')), self.platform + self.architecture + self.revision
        return WebKitFinder(self.host.filesystem).path_from_webkit_base(*build_directory)

    @property
    def local_extracted_directory(self):
        return WebKitFinder(self.host.filesystem).path_from_webkit_base(self.local_downloaded_binaries_directory, self.configuration.capitalize())

    @property
    def s3_build_type(self):
        return "{self.platform}-{self.architecture}-{self.configuration}".format(self=self).lower()

    @property
    def s3_build_binaries_url(self):
        s3_api_base_path = self.host.filesystem.join(REST_API_URL, REST_API_MINIFIED_ENDPOINT)
        if self.should_use_download_unminified_url:
            s3_api_base_path = self.host.filesystem.join(REST_API_URL, REST_API_ENDPOINT)
        return self.host.filesystem.join(s3_api_base_path, self.s3_build_type)

    @property
    def s3_build_binaries_single_revision_url(self):
        return self.host.filesystem.join(self.s3_build_binaries_url, self.revision)

    @property
    def local_zip_path(self):
        return self.host.filesystem.join(self.local_downloaded_binaries_directory, self.configuration + '.zip')

    @staticmethod
    def _get_archives_json(url): # TODO revisit this name and print statement

        print('fetching %s' % url)

        response = urlopen(url)
        build_binaries_json = json.load(response)

        return build_binaries_json

    def _prompt_user_to_delete_first(self):
        ans = raw_input('\n A build already exists at %s. Do you want to override it [y/n]: ' % self.local_extracted_directory)
        ans = ans.lower()
        if 'y' in ans:
            return True
        if 'n' in ans:
            return False
        else:
            self._prompt_user_to_delete_first()

    def get_path(self):
        try:
            if self.s3_zip_url:
                self.revision = os.path.basename(self.s3_zip_url).strip('.zip')

            if not self.revision:
                self.revision = self._get_latest_build_revision()

            # check to see if previously downloaded local version exists before downloading
            if self.host.filesystem.exists(self.local_extracted_directory) and not self.should_delete_first:

                if not self._prompt_user_to_delete_first():
                    print('\n Aborting... to download build in another directory use the --build-directory flag')
                    exit(0)

            return self._fetch_build_binaries()
        except KeyboardInterrupt as error:
            _log.error('\n User interrupted cmd')
        except Exception as error:
            #self.clean_up_on_error()
            raise error

    def get_sorted_revisions(self):
        build_binaries_json = self._get_archives_json(self.s3_build_binaries_url)

        revisions = [int(item['revision']['N']) for item in build_binaries_json['revisions']['Items']]
        return sorted(revisions)

    def _get_latest_build_revision(self):
        build_binaries_json = self._get_archives_json(self.s3_build_binaries_url)
        items = build_binaries_json['revisions']['Items']
        if not items:
            raise Exception('No build revisions found at: %s' % self.s3_build_binaries_url)

        latest_revision_index = len(items) - 1
        latest_build_revision = items[latest_revision_index]['revision']['N']

        _log.info('Defaulting to fetching the latest build revision: %s' % latest_build_revision)

        return latest_build_revision

    def _fetch_build_binaries(self):

        if not self.s3_zip_url:
            build_binaries_json = self._get_archives_json(self.s3_build_binaries_single_revision_url)

            if not build_binaries_json['archive']:
                raise Exception('No build revisions found at: %s' % self.s3_build_binaries_url)

            self.s3_zip_url = build_binaries_json['archive'][0]['s3_url']

        return self._fetch_build_binaries_zip()

    def _fetch_build_binaries_zip(self):

        try:
            # attempt to download the zip file
            _log.info("Starting ZipFile Download: %s" % self.s3_zip_url)
            build_zip = urlopen(self.s3_zip_url)

            self.host.filesystem.maybe_make_directory(self.local_downloaded_binaries_directory)

            with open(self.local_zip_path, "wb") as local_build_binaries:
                _log.info("Writing ZipFile To Local Drive: %s" % self.local_zip_path)
                local_build_binaries.write(build_zip.read())

            if not self.should_not_extract:
                _log.info("Extracting ZipFile")

                self._extract_zip_archive()

            _log.info("Extracted Binaries Can Be Found Here: %s" % self.local_downloaded_binaries_directory)

            return self.local_downloaded_binaries_directory

        except BadZipfile:
            raise Exception('BadZipfile Error: could not exact ZipFile')
        except HTTPError:
            raise Exception('HTTP Error: internet connectivity is required fetch binary file')
        except URLError:
            raise Exception('URLError Error: please make sure %s is a valid link' % self.s3_build_binaries_url)

    def _extract_zip_archive(self):
        command = ['python', 'Tools/BuildSlaveSupport/built-product-archive', '--platform', self.platform.lower(),
                   '--%s' % self.configuration.lower(), 'extract']

        if self.build_directory:
            command.extend(['--build-directory', self.local_downloaded_binaries_directory])

        subprocess.check_call(command)


# todo add flag for dir created to use in cleanup on error
# add config to does exsist check