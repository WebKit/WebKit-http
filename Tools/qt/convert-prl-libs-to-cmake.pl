#!/usr/bin/env perl
# Copyright (C) 2016 Konstantin Tokarev <annulen@yandex.ru>
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

use File::Basename;
use File::Spec;
use Getopt::Long;
use Text::ParseWords;

use v5.10;

use strict;
use warnings;

my $qt_lib;
my $component_name;
my $out_name;
my $compiler;

processArgs();

sub processArgs {
    GetOptions (
        "lib=s" => \$qt_lib,
        "component=s" => \$component_name,
        "out=s" => \$out_name,
        "compiler=s" => \$compiler
    )
}

my $qt_lib_base = fileparse($qt_lib, qr{\..*});
my $qt_lib_dir = dirname($qt_lib);
my $prl_name = File::Spec->join($qt_lib_dir, "$qt_lib_base.prl");

my $qmake_prl_libs;

open(my $prl, '<', $prl_name) or die "Cannot open $prl_name: $!";
while (<$prl>) {
    next unless /^QMAKE_PRL_LIBS/;
    chomp;
    if (/^QMAKE_PRL_LIBS\s+=\s+(.*)$/) {
        $qmake_prl_libs = $1;
        last;
    }
}
close $prl;

unless ($qmake_prl_libs) {
    print "QMAKE_PRL_LIBS variable is undefined or empty\n";
    exit;
}

my $prl_libs = squash_prl_libs(shellwords($qmake_prl_libs));

my $template = <<'END_CMAKE';
get_target_property(_link_libs Qt5::${_component} INTERFACE_LINK_LIBRARIES)
if (_link_libs)
    set(_list_sep ";")
else ()
    set(_list_sep "")
endif ()
set_target_properties(Qt5::${_component} PROPERTIES
    "INTERFACE_LINK_LIBRARIES" "${_link_libs}${_list_sep}${_libs}")
set(Qt5${_component}_STATIC_LIB_DEPENDENCIES "${_libs}")
list(APPEND STATIC_LIB_DEPENDENCIES
    ${Qt5${_component}_STATIC_LIB_DEPENDENCIES}
)
unset(_component)
unset(_libs)
unset(_list_sep)

END_CMAKE

open(my $out, '>>', $out_name) or die "Cannot open $out_name for writing: $!";
print $out qq/set(_component "$component_name")\n/;
print $out qq/set(_libs "$prl_libs")\n/;
print $out $template;
close $out;

sub squash_prl_libs {
    my @libs = @_;
    my @result;
    for (my $i = 0; $i < scalar(@libs); ++$i) {
        my $lib = $libs[$i];
        if ($lib eq '-framework') {
            $lib = "$libs[$i] $libs[$i + 1]";
            ++$i;
        }
        $lib =~ s"\$\$\[QT_INSTALL_LIBS\]"$qt_lib_dir"g;

        if (lc($compiler) eq 'msvc') {
            # convert backslashes
            $lib =~ s"\\"/"g;

            # MSVC doesn't support -L and -l arguments
            if ($lib =~ /^-L(.*)$/) {
                $lib = "-LIBPATH:$1"
            } else {
                $lib =~ s/^-l//;
            }
        }

        push @result, $lib;
    }
    return join ';', @result;
}
