#!/usr/bin/perl
# Copyright (C) 2016 Konstantin Tokavev <annulen@yandex.ru>
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
# THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
# THE POSSIBILITY OF SUCH DAMAGE.

use v5.10;

use strict;
use warnings;

my $qtVer = shift @ARGV;
my $qtNamespace = shift @ARGV;

die "Usage: $0 <Qt version> [Qt namespace]\n" unless $qtVer;

$qtVer =~ /^(\d+)\.(\d+)\.\d+$/ or die "$qtVer is not valid Qt version\n";
my $major = $1;
my $minor = $2;

# Next code tries to mirror logic of mkspecs/features/qt_module.prf

say
"Qt_${major}_PRIVATE_API {
    qt_private_api_tag*;
};";


my $current = "Qt_$major";
my $previous;
say "$current { *; };";

my $tag_symbol = "qt_version_tag";
if ($qtNamespace) {
    $tag_symbol .= "_$qtNamespace";
}

for (my $i = 0; $i < $minor; ++$i) {
    $previous = $current;
    $current = "Qt_$major.$i";
    say "$current {} $previous;";
}

$previous = $current;
say "Qt_$major.$minor { $tag_symbol; } $previous;";
