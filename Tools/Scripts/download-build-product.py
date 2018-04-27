#!/usr/bin/env python

# Copyright (C) 2018 Apple Inc.  All rights reserved.
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1.  Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
# 2.  Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
# 3.  Neither the name of Apple Inc. ("Apple") nor the names of
#     its contributors may be used to endorse or promote products derived
#     from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import sys
import optparse
import logging

from webkitpy.port import platform_options, configuration_options
from webkitpy.common.build_binaries_fetcher import BuildBinariesFetcher
from webkitpy.common.host import Host

EXCEPTIONAL_EXIT_STATUS = 254

_log = logging.getLogger(__name__)
logging.basicConfig()

def parse_args(args):

    option_group_definitions = [
        ("Platform options", platform_options()),
        ("Configuration options", configuration_options()),
        ("Build Binary options", [
            optparse.make_option("--revision", type="str", default=None,
                                 help="Pass in a build revision number from WebKit archives to download binaries."
                                      "Revision=None will download the latest build.")
        ])
    ]

    option_parser = optparse.OptionParser(usage="%prog [options] [<path>...]")

    for group_name, group_options in option_group_definitions:
        option_group = optparse.OptionGroup(option_parser, group_name)
        option_group.add_options(group_options)
        option_parser.add_option_group(option_group)

    return option_parser.parse_args(args)


def main(argv):
    options, args = parse_args(argv)
    host = Host()

    try:
        port = host.port_factory.get(options.platform, options)
        build_binaries_fetcher = BuildBinariesFetcher(host, port.port_name, options.architecture, options.configuration,
                                                      options.revision)
        build_binaries_fetcher.get_path()
    except Exception as error:
        _log.error('%s : %s' % (type(error), error))
        return EXCEPTIONAL_EXIT_STATUS

if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))

