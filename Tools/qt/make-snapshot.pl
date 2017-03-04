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

use Cwd;
use File::Basename;
use strict;
use warnings;

sub usage {
    my $msg = shift || "";
    die $msg . "Usage: cd target/repo/path; $0 src/repo/path";
}

scalar @ARGV == 0 || usage();
-f "ChangeLog" && die "This script must be run in snapshots repository";
-d ".git" || usage();

`git status` =~ "nothing to commit, working tree clean" or die "Target working tree is dirty";

my $src_repo = dirname(dirname(dirname(__FILE__)));
my $target_repo = getcwd();

chdir $src_repo;
(-f "ChangeLog" && -x "Tools/gtk/make-dist.py") or usage("Target repository path is invalid!\n");
my $commit = `git rev-parse HEAD` or usage("Cannot get HEAD revision in target repo!\n");
chomp $commit;

my @commands = (
    "Tools/gtk/make-dist.py -t snapshot Tools/qt/manifest.txt",
    "cd $target_repo",
    "git rm -rf *",
    "tar -xf $src_repo/snapshot.tar --strip-components=1",
    "git add -A",
    "rm $src_repo/snapshot.tar",
    "git commit -m 'Import WebKit commit $commit'"
);

my $cmd = join " && ", @commands;
print "Executing $cmd\n";

exec $cmd;
