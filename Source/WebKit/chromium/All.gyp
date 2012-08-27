#
# Copyright (C) 2011 Google Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#         * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#         * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#         * Neither the name of Google Inc. nor the names of its
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
#

{
    'includes': [
        'features.gypi',
    ],
    'targets': [
        {
            # These targets should be sufficient to cause everything
            # else to build (incl. webkit); if they aren't, we have our
            # dependencies wrong.
            'target_name': 'all_webkit',
            'type': 'none',
            'dependencies': [
                'WebKitUnitTests.gyp:webkit_unit_tests',
                '../../../Tools/DumpRenderTree/DumpRenderTree.gyp/DumpRenderTree.gyp:DumpRenderTree',
                '../../../Tools/TestWebKitAPI/TestWebKitAPI.gyp/TestWebKitAPI.gyp:TestWebKitAPI',
            ],
            'conditions': [
                ['OS=="android"', {
                    'dependencies': [
                        '../../../Tools/DumpRenderTree/DumpRenderTree.gyp/DumpRenderTree.gyp:DumpRenderTree_apk',
                    ],
                }],
                # Special target to wrap a gtest_target_type==shared_library
                # webkit_unit_tests and TestWebKitAPI into an android apk for
                # execution. See base.gyp for TODO(jrg)s about this strategy.
                ['OS=="android" and gtest_target_type == "shared_library"', {
                    'dependencies': [
                        'WebKitUnitTests.gyp:webkit_unit_tests_apk',
                        '../../../Tools/TestWebKitAPI/TestWebKitAPI.gyp/TestWebKitAPI.gyp:TestWebKitAPI_apk',
                    ],
                }],
            ],
        }
    ],
}
